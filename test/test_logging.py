"""Test bindings for pycanha-core logging accessors."""

import pycanha_core as pcc


def test_get_logger_roundtrip():
    logger = pcc.get_logger()
    assert logger.name == "pycanha-core"

    logger.set_level(pcc.LogLevel.DEBUG)
    assert logger.level == pcc.LogLevel.DEBUG
    assert logger.should_log(pcc.LogLevel.INFO) is True


def test_get_profiling_logger_roundtrip():
    logger = pcc.get_profiling_logger()
    assert logger.name == "pycanha-core.profiling"

    pcc.set_profiling_logger_level(pcc.LogLevel.INFO)
    assert logger.level == pcc.LogLevel.INFO

    logger.info("profiling logger smoke test")
    logger.flush()
