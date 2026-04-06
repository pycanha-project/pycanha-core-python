#pragma once

#include <memory>
#include <string>

#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <spdlog/common.h>
#include <spdlog/logger.h>

#include "pycanha-core/utils/logger.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::utils {

inline void register_logging(nb::module_ &m) {
  using Logger = spdlog::logger;
  using LogLevel = spdlog::level::level_enum;

  nb::enum_<LogLevel>(m, "LogLevel", "Log verbosity level.")
      .value("TRACE", LogLevel::trace, "Most verbose level.")
      .value("DEBUG", LogLevel::debug, "Debug-level messages.")
      .value("INFO", LogLevel::info, "Informational messages.")
      .value("WARN", LogLevel::warn, "Warning messages.")
      .value("ERROR", LogLevel::err, "Error messages.")
      .value("CRITICAL", LogLevel::critical, "Critical/fatal messages.")
      .value("OFF", LogLevel::off, "Disable all logging.")
      .export_values();

  nb::class_<Logger>(m, "Logger", "pycanha-core logger.")
      .def_prop_ro(
          "name", [](const Logger &self) { return std::string(self.name()); },
          "Logger name.")
      .def_prop_rw(
          "level", [](const Logger &self) { return self.level(); },
          [](Logger &self, LogLevel level) { self.set_level(level); },
          "Runtime log level.")
      .def("set_level", &Logger::set_level, "level"_a,
           "Set the runtime log level.")
      .def("should_log", &Logger::should_log, "level"_a,
           "Return whether the logger currently emits the given level.")
      .def(
          "log",
          [](Logger &self, LogLevel level, const std::string &message) {
            self.log(level, message);
          },
          "level"_a, "message"_a, "Emit a message at the given level.")
      .def(
          "trace",
          [](Logger &self, const std::string &message) { self.trace(message); },
          "message"_a, "Log a message at TRACE level.")
      .def(
          "debug",
          [](Logger &self, const std::string &message) { self.debug(message); },
          "message"_a, "Log a message at DEBUG level.")
      .def(
          "info",
          [](Logger &self, const std::string &message) { self.info(message); },
          "message"_a, "Log a message at INFO level.")
      .def(
          "warn",
          [](Logger &self, const std::string &message) { self.warn(message); },
          "message"_a, "Log a message at WARN level.")
      .def(
          "warning",
          [](Logger &self, const std::string &message) { self.warn(message); },
          "message"_a, "Log a message at WARN level (alias for warn).")
      .def(
          "error",
          [](Logger &self, const std::string &message) { self.error(message); },
          "message"_a, "Log a message at ERROR level.")
      .def(
          "critical",
          [](Logger &self, const std::string &message) {
            self.critical(message);
          },
          "message"_a, "Log a message at CRITICAL level.")
      .def("flush", &Logger::flush, "Flush pending log output.");

  m.def("get_logger", &pycanha::get_logger,
        "Return the main pycanha-core logger.");
  m.def("get_profiling_logger", &pycanha::get_profiling_logger,
        "Return the profiling logger.");
  m.def("set_logger_level", &pycanha::set_logger_level, "level"_a,
        "Set the main logger runtime level.");
  m.def("set_profiling_logger_level", &pycanha::set_profiling_logger_level,
        "level"_a, "Set the profiling logger runtime level.");
}

} // namespace pycanha::bindings::utils