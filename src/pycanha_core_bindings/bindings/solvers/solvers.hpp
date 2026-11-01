#pragma once

#include <nanobind/nanobind.h>

#include "bindings/solvers/registry.hpp"
#include "bindings/solvers/report.hpp"
#include "bindings/solvers/settings.hpp"
#include "bindings/solvers/solver.hpp"
#include "bindings/solvers/types.hpp"

namespace nb = nanobind;

namespace pycanha::bindings::solvers {

// The shared types (settings, enums, report, Solver, registry) in
// pycanha_core.solvers, the solver types in one submodule per question:
// steady, transient, sensitivity. The order matters: the settings types are
// used by default values and the property docstrings of the solver classes.
inline void register_solvers(nb::module_ &m) {
  register_settings(m);
  register_report(m);
  register_solver_base(m);

  nb::module_ steady = m.def_submodule("steady", "Steady-state solvers");
  register_steady(steady);
  nb::module_ transient = m.def_submodule("transient", "Transient solvers");
  register_transient(transient);
  nb::module_ sensitivity =
      m.def_submodule("sensitivity", "Transient solvers with parameter derivatives");
  register_sensitivity(sensitivity);

  register_registry(m);
  register_callbacks(m);
}

} // namespace pycanha::bindings::solvers
