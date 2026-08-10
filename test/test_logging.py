"""Tests for the pycanha_core.log bindings.

The C++ core owns display and the log file. These tests drive the runtime
controls over both and check what actually lands on disk, because "record to
the file, stay quiet on the console" is the whole point of the two thresholds.
"""

import logging
import os
import subprocess
import sys
from datetime import date
from pathlib import Path

import pytest

import pycanha_core as pcc

log = pcc.log

# The development preset moves both thresholds, so the tests that assert the
# shipped defaults cannot run under it.
_DEV_MODE = log.dev_mode()


@pytest.fixture
def log_settings():
    """Restore every global the logging tests move, and start from an empty
    buffer so a record left over from another test cannot be mistaken for one
    this test produced."""
    saved = (
        log.record_level(),
        log.display_level(),
        log.file_output(),
        log.log_directory(),
        log.buffer_capacity(),
    )
    log.clear_records()
    yield
    log.set_record_level(saved[0])
    log.set_display_level(saved[1])
    log.set_log_directory(saved[3])
    log.set_file_output(saved[2])
    log.set_buffer_capacity(saved[4])
    log.clear_records()


def test_log_submodule_surface():
    for name in (
        "LogLevel",
        "LogRecord",
        "LogDrain",
        "set_record_level",
        "record_level",
        "set_display_level",
        "display_level",
        "set_file_output",
        "file_output",
        "set_log_directory",
        "log_directory",
        "current_log_file",
        "records",
        "drain",
        "set_buffer_capacity",
        "buffer_capacity",
        "clear_records",
        "flush",
        "write",
        "should_log",
        "compiled_level_floor",
        "dev_mode",
    ):
        assert hasattr(log, name), name


def test_retired_logger_api_is_gone():
    # The old binding handed out the spdlog logger itself, which let a caller
    # set a level underneath every threshold above it.
    for name in (
        "Logger",
        "get_logger",
        "get_python_logger",
        "set_logger_level",
        "set_python_logger_level",
    ):
        assert not hasattr(pcc, name), name


def test_log_level_is_re_exported_at_the_package_root():
    assert pcc.LogLevel is log.LogLevel


@pytest.mark.skipif(_DEV_MODE, reason="the dev-mode preset moves both defaults")
def test_default_thresholds_record_more_than_they_display():
    assert log.record_level() == log.LogLevel.INFO
    assert log.display_level() == log.LogLevel.WARN


def test_thresholds_round_trip(log_settings):
    log.set_record_level(log.LogLevel.INFO)
    assert log.record_level() == log.LogLevel.INFO
    log.set_display_level(log.LogLevel.OFF)
    assert log.display_level() == log.LogLevel.OFF


def test_a_threshold_below_the_compiled_floor_is_refused(log_settings):
    floor = log.compiled_level_floor()
    if floor == log.LogLevel.TRACE:
        pytest.skip("this build compiled every level in, so nothing is below")
    with pytest.raises(ValueError):
        log.set_record_level(log.LogLevel.TRACE)


def test_should_log_follows_the_record_threshold(log_settings):
    log.set_record_level(log.LogLevel.WARN)
    assert log.should_log(log.LogLevel.ERROR) is True
    assert log.should_log(log.LogLevel.INFO) is False
    log.set_record_level(log.LogLevel.INFO)
    assert log.should_log(log.LogLevel.INFO) is True


def _daily_log(directory):
    return directory / f"{date.today().isoformat()}.log"


def test_the_file_is_created_by_the_first_record_not_by_configuring(
    log_settings, tmp_path
):
    target = tmp_path / "logs"
    log.set_file_output(True)
    log.set_log_directory(target)
    log.set_record_level(log.LogLevel.INFO)

    assert log.current_log_file() is None
    assert not target.exists()

    log.write(log.LogLevel.WARN, "a recorded warning")
    log.flush()

    assert _daily_log(target).exists()
    assert log.current_log_file() == _daily_log(target)


def test_the_record_carries_the_message_the_pid_and_a_run_banner(
    log_settings, tmp_path
):
    target = tmp_path / "logs"
    log.set_file_output(True)
    log.set_log_directory(target)
    log.set_record_level(log.LogLevel.INFO)

    log.write(log.LogLevel.INFO, "a recorded message")
    log.flush()

    contents = _daily_log(target).read_text(encoding="utf-8")
    assert "a recorded message" in contents
    # Concurrent runs share the daily file, so every record carries the pid and
    # every process announces itself once.
    assert str(os.getpid()) in contents
    assert contents.count("=== pycanha run") == 1
    # The origin is what distinguishes a layer-3 record from a core one.
    assert "[pycanha]" in contents


def test_a_record_below_the_threshold_never_reaches_the_file(
    log_settings, tmp_path
):
    target = tmp_path / "logs"
    log.set_file_output(True)
    log.set_log_directory(target)
    log.set_record_level(log.LogLevel.WARN)

    log.write(log.LogLevel.INFO, "below the threshold")
    log.write(log.LogLevel.ERROR, "above the threshold")
    log.flush()

    contents = _daily_log(target).read_text(encoding="utf-8")
    assert "below the threshold" not in contents
    assert "above the threshold" in contents


def test_file_output_off_leaves_the_filesystem_untouched(log_settings, tmp_path):
    target = tmp_path / "logs"
    log.set_log_directory(target)
    log.set_file_output(False)
    log.set_record_level(log.LogLevel.INFO)

    log.write(log.LogLevel.ERROR, "not written anywhere")
    log.flush()

    assert not target.exists()
    assert log.current_log_file() is None
    assert log.file_output() is False


def test_the_directory_is_resolved_when_a_file_is_opened(log_settings, tmp_path):
    # A relative path follows the process rather than the install location.
    target = tmp_path / "logs"
    log.set_file_output(True)
    log.set_log_directory("logs")
    assert log.log_directory() == Path("logs")
    log.set_log_directory(target)
    assert log.log_directory() == target


class TestRecordBuffer:
    """The in-memory buffer: what a notebook queries when file output is off."""

    def test_a_record_is_structured_not_a_rendered_line(self, log_settings):
        log.set_record_level(log.LogLevel.INFO)
        log.write(log.LogLevel.WARN, "a buffered message")

        record = log.records(1)[0]
        assert record.message == "a buffered message"
        assert record.level == log.LogLevel.WARN
        assert record.origin == "pycanha"
        assert record.pid == os.getpid()
        assert record.timestamp > 0.0
        assert isinstance(record.thread_id, int)

    def test_the_origin_tells_a_core_record_from_a_layer_3_one(
        self, log_settings, basic_tmm
    ):
        log.set_record_level(log.LogLevel.INFO)
        log.clear_records()
        # Adding a node that already exists is refused silently, so the log is
        # the only signal — and it is made inside the C++ library.
        basic_tmm.add_node(pcc.tmm.Node(10))

        origins = {record.origin for record in log.records(50)}
        assert "pycanha-core" in origins

    def test_records_does_not_consume(self, log_settings):
        log.set_record_level(log.LogLevel.INFO)
        log.write(log.LogLevel.WARN, "read me twice")

        assert len(log.records(10)) == 1
        assert len(log.records(10)) == 1

    def test_drain_consumes_and_records_still_sees_it(self, log_settings):
        log.set_record_level(log.LogLevel.INFO)
        log.write(log.LogLevel.WARN, "taken once")

        drained = log.drain()
        assert [r.message for r in drained.records] == ["taken once"]
        assert drained.dropped == 0
        # A second take gets nothing, but looking still works: taking and
        # inspecting are independent.
        assert log.drain().records == []
        assert [r.message for r in log.records(10)] == ["taken once"]

    def test_overflow_drops_the_oldest_and_reports_how_many(self, log_settings):
        log.set_record_level(log.LogLevel.INFO)
        log.set_buffer_capacity(4)
        assert log.buffer_capacity() == 4
        for index in range(10):
            log.write(log.LogLevel.WARN, f"record {index}")

        kept = [r.message for r in log.records(10)]
        assert kept == [f"record {index}" for index in range(6, 10)]
        assert log.drain().dropped == 6

    def test_clearing_is_not_counted_as_a_drop(self, log_settings):
        log.set_record_level(log.LogLevel.INFO)
        log.write(log.LogLevel.WARN, "thrown away on purpose")
        log.clear_records()

        assert log.records(10) == []
        assert log.drain().dropped == 0


class TestLoggingBridge:
    """Records reach stdlib `logging` with no opt-in, by being pulled."""

    def test_the_bridge_is_installed_by_import(self):
        handlers = logging.getLogger("pycanha").handlers
        # A NullHandler and nothing else: the console belongs to the C++ sink,
        # and a handler here would print every record a second time. It also
        # stops stdlib falling back to its last-resort stderr handler.
        assert len(handlers) == 1
        assert isinstance(handlers[0], logging.NullHandler)

    def test_trace_has_a_name_rather_than_rendering_as_a_number(self):
        assert logging.getLevelName(5) == "TRACE"

    def test_caplog_captures_a_record_after_a_drain(self, log_settings, caplog):
        log.set_record_level(log.LogLevel.INFO)
        with caplog.at_level(logging.INFO, logger="pycanha"):
            log.write(log.LogLevel.INFO, "captured by caplog")
            # Delivery is deferred: liveness is the console sink's job, because
            # no Python code runs on a thread that is inside a C++ call.
            assert "captured by caplog" not in caplog.text
            log.flush()
        assert "captured by caplog" in caplog.text

    def test_the_delivered_record_carries_the_cpp_origin_and_process(
        self, log_settings, caplog
    ):
        log.set_record_level(log.LogLevel.INFO)
        with caplog.at_level(logging.INFO, logger="pycanha"):
            log.write(log.LogLevel.WARN, "with metadata")
            log.flush()

        record = next(r for r in caplog.records if r.message == "with metadata")
        assert record.name == "pycanha"
        assert record.origin == "pycanha"
        assert record.levelno == logging.WARNING
        assert record.process == os.getpid()

    def test_a_coarse_entry_point_drains_on_the_way_out(
        self, log_settings, caplog, basic_tmm
    ):
        log.set_record_level(log.LogLevel.INFO)
        solver = pcc.solvers.SSLU(basic_tmm)
        solver.initialize()
        with caplog.at_level(logging.INFO, logger="pycanha"):
            log.write(log.LogLevel.INFO, "queued before the solve")
            assert "queued before the solve" not in caplog.text
            solver.solve()
        # No flush() call: solving is one of the entry points that drains.
        assert "queued before the solve" in caplog.text

    def test_nothing_is_printed_twice_with_the_default_handler(
        self, log_settings, capsys
    ):
        log.set_record_level(log.LogLevel.INFO)
        log.set_display_level(log.LogLevel.OFF)
        log.write(log.LogLevel.ERROR, "silence expected")
        log.flush()

        captured = capsys.readouterr()
        assert "silence expected" not in captured.out
        assert "silence expected" not in captured.err

    def test_a_dropped_record_is_reported_to_logging(self, log_settings, caplog):
        log.set_record_level(log.LogLevel.INFO)
        log.set_buffer_capacity(2)
        with caplog.at_level(logging.INFO, logger="pycanha"):
            for index in range(5):
                log.write(log.LogLevel.WARN, f"record {index}")
            log.flush()

        assert any("3 records were discarded" in r.message for r in caplog.records)


@pytest.mark.skipif(_DEV_MODE, reason="already running under the dev preset")
def test_dev_mode_is_off_unless_the_environment_asks_for_it():
    assert log.dev_mode() is False


def test_dev_mode_preset_drops_both_thresholds_to_the_compiled_floor():
    # A fresh interpreter: the preset is read once, at import.
    script = (
        "import pycanha_core as pcc\n"
        "log = pcc.log\n"
        "floor = log.compiled_level_floor()\n"
        "print(log.dev_mode(), log.record_level() == floor,"
        " log.display_level() == floor)\n"
    )
    environment = dict(os.environ, PYCANHA_DEV_MODE="1")
    completed = subprocess.run(
        [sys.executable, "-c", script],
        env=environment,
        capture_output=True,
        text=True,
        check=True,
    )
    assert completed.stdout.split() == ["True", "True", "True"]
