#pragma once

#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <nanobind/nanobind.h>
#include <nanobind/stl/filesystem.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <spdlog/common.h>
#include <spdlog/logger.h>

#include "pycanha-core/utils/log_record.hpp"
#include "pycanha-core/utils/logger.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::utils {

// The stdlib logger every record is delivered to. One name for both origins:
// the origin travels as a field of the record, so a consumer configures one
// logger rather than discovering a second one the day a message happens to
// come from the other side.
inline constexpr const char *k_bridge_logger_name = "pycanha";

// Environment switch that turns this build into a development build: every
// compiled-in record shown on the console, and nanobind's shutdown leak
// warnings enabled. Deliberately a single switch rather than one option per
// behaviour — it exists for CI and for working on pycanha itself, not for
// tuning a shipped wheel.
inline constexpr const char *k_dev_mode_env_var = "PYCANHA_DEV_MODE";

inline bool dev_mode_requested() {
  const char *const raw = std::getenv(k_dev_mode_env_var);
  if (raw == nullptr) {
    return false;
  }
  std::string value{raw};
  for (char &character : value) {
    character = static_cast<char>(
        std::tolower(static_cast<unsigned char>(character)));
  }
  return value == "1" || value == "true" || value == "on" || value == "yes";
}

/// The stdlib `logging` level a spdlog level maps to.
///
/// TRACE has no stdlib equivalent and takes 5, the conventional slot below
/// DEBUG; install_logging_bridge() gives it a name so it does not render as
/// "Level 5".
inline int to_python_level(const spdlog::level::level_enum level) {
  switch (level) {
  case spdlog::level::trace:
    return 5;
  case spdlog::level::debug:
    return 10;
  case spdlog::level::info:
    return 20;
  case spdlog::level::warn:
    return 30;
  case spdlog::level::err:
    return 40;
  case spdlog::level::critical:
    return 50;
  default:
    return 0;
  }
}

inline double epoch_seconds(
    const std::chrono::system_clock::time_point timestamp) {
  return std::chrono::duration<double>(timestamp.time_since_epoch()).count();
}

inline nb::object bridge_logger() {
  return nb::module_::import_("logging").attr("getLogger")(
      k_bridge_logger_name);
}

/// Builds one stdlib LogRecord and hands it to `logger`.
///
/// Built rather than pre-formatted so that a consumer can filter and format it
/// like any other record of its own. The C++ clock, pid and thread are copied
/// onto the record instead of letting stdlib fill in the values of whoever
/// happens to be draining: those describe the delivery, not the event.
inline void emit_one(const nb::object &logger, const int level,
                     const std::string &origin, const std::string &message,
                     const double timestamp, const int pid,
                     const std::size_t thread_id) {
  if (!nb::cast<bool>(logger.attr("isEnabledFor")(level))) {
    return;
  }
  nb::dict extra;
  extra["origin"] = origin;
  nb::object record = logger.attr("makeRecord")(
      k_bridge_logger_name, level, /*fn=*/"", /*lno=*/0, message,
      /*args=*/nb::make_tuple(), /*exc_info=*/nb::none(), "extra"_a = extra);
  record.attr("created") = timestamp;
  record.attr("msecs") = (timestamp - std::floor(timestamp)) * 1000.0;
  record.attr("process") = pid;
  record.attr("thread") = thread_id;
  logger.attr("handle")(record);
}

/// Takes everything the core has buffered and delivers it to stdlib `logging`.
///
/// This is the only direction the integration runs: the core appends to a
/// bounded buffer and never calls out, and this pulls from it on a Python
/// thread that already holds the GIL. Delivery is therefore deferred until a
/// C++ call returns, which is fine because being live is the console sink's
/// job — nothing else can be, since no Python code runs on a thread that is
/// inside a C++ call.
inline std::size_t deliver_records() {
  const pycanha::LogDrain drained = pycanha::drain_log_records();
  if (drained.records.empty() && drained.dropped == 0) {
    return 0;
  }
  const nb::object logger = bridge_logger();
  for (const pycanha::LogRecord &record : drained.records) {
    emit_one(logger, to_python_level(record.level), record.origin,
             record.message, epoch_seconds(record.timestamp), record.pid,
             record.thread_id);
  }
  if (drained.dropped > 0) {
    // Composed here rather than logged through the core, which would put it
    // in the very buffer that just overflowed.
    emit_one(logger, to_python_level(spdlog::level::warn), k_bridge_logger_name,
             "log buffer overflowed: " + std::to_string(drained.dropped) +
                 " records were discarded before delivery; raise the capacity "
                 "with set_buffer_capacity() or flush() more often",
             epoch_seconds(std::chrono::system_clock::now()), 0, 0);
  }
  return drained.records.size();
}

/// Drains on the way out of a binding call.
///
/// Attached to the coarse entry points only — the ones that do real work and
/// can produce a trail worth delivering. Per-call would cost a buffer check on
/// every attribute access for no benefit, since a getter that logs nothing has
/// nothing to deliver.
struct LogDrainGuard {
  LogDrainGuard() = default;
  LogDrainGuard(const LogDrainGuard &) = delete;
  LogDrainGuard(LogDrainGuard &&) = delete;
  LogDrainGuard &operator=(const LogDrainGuard &) = delete;
  LogDrainGuard &operator=(LogDrainGuard &&) = delete;

  ~LogDrainGuard() {
    // A destructor cannot throw, and a failure to hand records to a consumer
    // must not turn a completed thermal computation into an error. The records
    // stay in the buffer and the next drain picks them up.
    try {
      deliver_records();
    } catch (...) {
      // Clear whatever the failure left set, so the call this guard is
      // attached to does not return carrying an exception it never raised.
      if (PyErr_Occurred() != nullptr) {
        PyErr_Clear();
      }
    }
  }
};

/// Attaches the bridge to stdlib `logging`, once, at import.
///
/// The logger gets a NullHandler so nothing is printed by the Python side: the
/// console already belongs to the C++ sink, and a handler here would print
/// every record a second time. A NullHandler is also what stops stdlib from
/// falling back to its last-resort stderr handler. A user who adds a handler
/// of their own has asked for a second destination and can silence the C++
/// console with set_display_level(LogLevel.OFF).
inline void install_logging_bridge(nb::module_ &m) {
  nb::module_ logging = nb::module_::import_("logging");
  logging.attr("addLevelName")(to_python_level(spdlog::level::trace), "TRACE");
  bridge_logger().attr("addHandler")(logging.attr("NullHandler")());
  // Whatever is still buffered when the process ends is worth delivering: an
  // interpreter exiting is not a reason to lose the trail of what it did.
  nb::module_::import_("atexit").attr("register")(m.attr("flush"));
}

/// Registers the `log` submodule: the runtime controls over what pycanha
/// records and what it displays, the buffered records themselves, and the
/// entry point layer 3 logs through.
inline void register_logging(nb::module_ &m) {
  using LogLevel = spdlog::level::level_enum;

  nb::enum_<LogLevel>(m, "LogLevel", "Log verbosity level.")
      .value("TRACE", LogLevel::trace, "Per-iteration / per-element detail.")
      .value("DEBUG", LogLevel::debug, "Per-object bookkeeping.")
      .value("INFO", LogLevel::info,
             "One line per user-initiated top-level operation.")
      .value("WARN", LogLevel::warn,
             "The operation succeeded, but something was dropped or assumed.")
      .value("ERROR", LogLevel::err,
             "The operation failed or its result is wrong.")
      .value("CRITICAL", LogLevel::critical, "Critical/fatal messages.")
      .value("OFF", LogLevel::off, "Emit nothing at all.");

  m.def("compiled_level_floor", &pycanha::compiled_log_level,
        "The most verbose level compiled into this build. Levels below it were "
        "stripped from the binary, so neither threshold can be set under it "
        "and no runtime setting can make those records appear.");

  // --- thresholds ---------------------------------------------------------
  // Two thresholds, not one: the record threshold is what gets produced at all
  // and therefore what the log file and the buffer keep, the display threshold
  // is what additionally reaches the console. That split is what makes a run
  // keep a full trail while staying silent unless something is wrong.
  m.def("set_record_level", &pycanha::set_record_level, "level"_a,
        "Set what is produced at all, and therefore what the log file and the "
        "record buffer keep (default INFO). Raises ValueError for a level "
        "below compiled_level_floor().");
  m.def("record_level", &pycanha::record_level,
        "The current record threshold.");
  m.def("set_display_level", &pycanha::set_display_level, "level"_a,
        "Set what reaches the console (default WARN); LogLevel.OFF silences it "
        "entirely. Bounded by the record threshold, which is applied first, so "
        "asking the console for more than is recorded shows nothing extra.");
  m.def("display_level", &pycanha::display_level,
        "The current display threshold.");

  // --- the log file -------------------------------------------------------
  m.def("set_file_output", &pycanha::set_file_output, "enabled"_a,
        "Master switch for writing to disk. With it off nothing is created or "
        "written under the log directory while the console and the record "
        "buffer keep working, so a notebook can display or query records "
        "without touching the filesystem.");
  m.def("file_output", &pycanha::file_output,
        "Whether writing to disk is enabled.");
  m.def("set_log_directory", &pycanha::set_log_directory, "directory"_a,
        "Directory holding the daily log file (default 'logs'). A relative "
        "path is resolved against the current working directory every time a "
        "file is opened, so it follows the process rather than the install "
        "location.");
  m.def("log_directory", &pycanha::log_directory,
        "The configured log directory.");
  m.def(
      "current_log_file",
      []() -> std::optional<std::filesystem::path> {
        std::filesystem::path current = pycanha::current_log_file();
        if (current.empty()) {
          return std::nullopt;
        }
        return current;
      },
      "The log file currently open, or None when there is none: file output "
      "disabled, nothing logged yet (the file is created on the first record, "
      "not at import), or the directory could not be written to.");

  // --- the record buffer --------------------------------------------------
  nb::class_<pycanha::LogRecord>(
      m, "LogRecord",
      "One buffered record, kept structured rather than pre-formatted so it "
      "can be filtered and rendered by whoever receives it.")
      .def_prop_ro(
          "timestamp",
          [](const pycanha::LogRecord &r) { return epoch_seconds(r.timestamp); },
          "Seconds since the epoch, UTC, from one clock shared by every origin "
          "— so records made in C++ and records made above it order correctly "
          "against each other. datetime.datetime.fromtimestamp() converts it.")
      .def_ro("level", &pycanha::LogRecord::level, "LogLevel of the record.")
      .def_ro("origin", &pycanha::LogRecord::origin,
              "'pycanha-core' for a record made inside the C++ library, "
              "'pycanha' for one made by a layer above it.")
      .def_ro("message", &pycanha::LogRecord::message,
              "The already-composed message.")
      .def_ro("pid", &pycanha::LogRecord::pid,
              "Process that produced the record; concurrent runs share one log "
              "file, so this is what keeps them apart.")
      .def_ro("thread_id", &pycanha::LogRecord::thread_id,
              "Thread that produced the record.")
      .def("__repr__", [](const pycanha::LogRecord &r) {
        return "<LogRecord " + r.origin + " " + r.message + ">";
      });

  nb::class_<pycanha::LogDrain>(
      m, "LogDrain", "What one drain() took, plus what was lost before it.")
      .def_ro("records", &pycanha::LogDrain::records,
              "The records taken, oldest first.")
      .def_ro("dropped", &pycanha::LogDrain::dropped,
              "Records the buffer had to discard to stay bounded since the "
              "previous drain. Reported rather than kept silent, so a gap in "
              "the stream is visible.");

  m.def(
      "records",
      [](std::size_t count) { return pycanha::log_records(count); }, "count"_a,
      "The most recent `count` buffered records, oldest first, WITHOUT "
      "consuming them — fewer when the buffer holds fewer. This is how recent "
      "records are inspected when file output is off and the console threshold "
      "hides them, which is the normal configuration for a notebook.");
  m.def(
      "drain", [] { return pycanha::drain_log_records(); },
      "Take every record not yet taken. The installed `logging` bridge already "
      "consumes this stream on the way out of the coarse entry points, so a "
      "caller that drains as well diverts those records away from `logging`; "
      "use records() to look without taking.");
  m.def("set_buffer_capacity", &pycanha::set_log_buffer_capacity, "capacity"_a,
        "Resize the record buffer, discarding the oldest records when the new "
        "capacity is smaller.");
  m.def("buffer_capacity", &pycanha::log_buffer_capacity,
        "How many records the buffer holds before it starts discarding.");
  m.def("clear_records", &pycanha::clear_log_records,
        "Discard every buffered record. A deliberate reset is not counted as a "
        "drop: the caller asked for it.");

  m.def(
      "flush",
      [] {
        deliver_records();
        pycanha::flush();
      },
      "Deliver every buffered record to stdlib `logging` now and flush the log "
      "file. Delivery otherwise happens on the way out of the coarse entry "
      "points (solve, read, build) and at interpreter exit.");

  // --- emitting from Python -----------------------------------------------
  // Layer 3 logs through the C++ logger instead of keeping its own stream, so
  // that both origins land in one file with one clock and in the right order.
  // A record made here therefore travels Python -> C++ -> buffer -> Python
  // before it reaches `logging`. That is not a mistake and not a loop: it is
  // emitted to `logging` exactly once, and it is what buys the single ordered
  // stream.
  m.def(
      "write",
      [](LogLevel level, const std::string &message) {
        pycanha::get_python_logger()->log(level, message);
      },
      "level"_a, "message"_a,
      "Emit an already-formatted message through the shared logger, recorded "
      "with origin 'pycanha' rather than 'pycanha-core'.");
  m.def(
      "should_log",
      [](LogLevel level) {
        return pycanha::get_python_logger()->should_log(level);
      },
      "level"_a,
      "Whether a write() at this level would be recorded by anything. Cache it "
      "and re-read it after changing a threshold: formatting a message that is "
      "then dropped costs far more than the check.");

  // --- development build --------------------------------------------------
  // nanobind reports leaked instances on stderr at interpreter shutdown. The
  // residual leaks are known and benign, and the report is written after
  // Python-level machinery is gone so it cannot be captured or routed
  // anywhere; a wheel user gets noise they cannot act on. The flag is read
  // once at shutdown, so switching it off costs nothing per object.
  const bool dev_mode = dev_mode_requested();
  nb::set_leak_warnings(dev_mode);
  if (dev_mode) {
    pycanha::set_record_level(pycanha::compiled_log_level());
    pycanha::set_display_level(pycanha::compiled_log_level());
  }
  m.def("dev_mode", [dev_mode] { return dev_mode; },
        "Whether the PYCANHA_DEV_MODE environment variable asked for the "
        "development preset at import: both thresholds dropped to "
        "compiled_level_floor() and nanobind leak warnings enabled.");

  install_logging_bridge(m);
}

} // namespace pycanha::bindings::utils
