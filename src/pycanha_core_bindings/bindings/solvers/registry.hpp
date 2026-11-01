#pragma once

#include <functional>
#include <initializer_list>
#include <memory>

#include <nanobind/nanobind.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/shared_ptr.h>

#include "pycanha-core/solvers/callback_registry.hpp"
#include "pycanha-core/solvers/crank_nicolson.hpp"
#include "pycanha-core/solvers/jacobian_propagation.hpp"
#include "pycanha-core/solvers/solver.hpp"
#include "pycanha-core/solvers/solver_registry.hpp"
#include "pycanha-core/solvers/steady_state.hpp"
#include "pycanha-core/tmm/thermalmathematicalmodel.hpp"
#include "pycanha-core/tmm/thermalmodel.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::solvers {

// CallbackRegistry stores Python callables inside C++ std::function members,
// holding strong references that Python's cyclic garbage collector cannot
// see. A callback that refers back to the registry (or its model) creates an
// uncollectable cycle that would leak until interpreter shutdown. These GC
// hooks expose (tp_traverse) and release (tp_clear) those hidden references
// so such cycles are collected normally.
inline int callback_registry_tp_traverse(PyObject *self, visitproc visit, void *arg) {
  Py_VISIT(Py_TYPE(self));
  if (!nb::inst_ready(self)) {
    return 0;
  }
  auto *registry = nb::inst_ptr<pycanha::CallbackRegistry>(self);
  using FunctionCaster =
      nb::detail::type_caster<std::function<void(pycanha::CallbackContext &)>>;
  for (const std::function<void(pycanha::CallbackContext &)> *slot :
       {&registry->solver_loop, &registry->time_change, &registry->after_timestep}) {
    if (const auto *wrapper = slot->target<FunctionCaster::pyfunc_wrapper_t>()) {
      Py_VISIT(wrapper->f);
    }
  }
  return 0;
}

inline int callback_registry_tp_clear(PyObject *self) {
  if (!nb::inst_ready(self)) {
    return 0;
  }
  auto *registry = nb::inst_ptr<pycanha::CallbackRegistry>(self);
  registry->solver_loop = {};
  registry->time_change = {};
  registry->after_timestep = {};
  return 0;
}

inline PyType_Slot callback_registry_gc_slots[] = {
    {Py_tp_traverse, reinterpret_cast<void *>(callback_registry_tp_traverse)},
    {Py_tp_clear, reinterpret_cast<void *>(callback_registry_tp_clear)},
    {0, nullptr}};

inline void register_registry(nb::module_ &m) {
  using pycanha::ThermalMathematicalModel;
  using pycanha::solvers::SensitivitySolvers;
  using pycanha::solvers::SolverRegistry;
  using pycanha::solvers::SteadySolvers;
  using pycanha::solvers::TransientSolvers;
  using pycanha::solvers::sensitivity::JacobianPropagation;
  using pycanha::solvers::steady::SteadyState;
  using pycanha::solvers::transient::CrankNicolson;

  nb::class_<SteadySolvers>(m, "SteadySolvers",
                            "Steady-state solvers of a model, created on first use.")
      .def_prop_ro(
          "steady_state",
          [](SteadySolvers &self) -> SteadyState & { return self.steady_state(); },
          nb::rv_policy::reference_internal, "The model's SteadyState solver.");

  nb::class_<TransientSolvers>(m, "TransientSolvers",
                               "Transient solvers of a model, created on first use.")
      .def_prop_ro(
          "crank_nicolson",
          [](TransientSolvers &self) -> CrankNicolson & { return self.crank_nicolson(); },
          nb::rv_policy::reference_internal, "The model's CrankNicolson solver.");

  nb::class_<SensitivitySolvers>(m, "SensitivitySolvers",
                                 "Sensitivity solvers of a model, created on first use.")
      .def_prop_ro(
          "jacobian_propagation",
          [](SensitivitySolvers &self) -> JacobianPropagation & {
            return self.jacobian_propagation();
          },
          nb::rv_policy::reference_internal, "The model's JacobianPropagation solver.");

  nb::class_<SolverRegistry>(m, "SolverRegistry",
                             "One solver of each type per model, grouped by the "
                             "question it answers: "
                             "tm.solvers.transient.crank_nicolson.")
      .def_prop_ro(
          "steady", [](SolverRegistry &self) -> SteadySolvers & { return self.steady(); },
          nb::rv_policy::reference_internal, "The steady-state solvers.")
      .def_prop_ro(
          "transient",
          [](SolverRegistry &self) -> TransientSolvers & { return self.transient(); },
          nb::rv_policy::reference_internal, "The transient solvers.")
      .def_prop_ro(
          "sensitivity",
          [](SolverRegistry &self) -> SensitivitySolvers & { return self.sensitivity(); },
          nb::rv_policy::reference_internal, "The sensitivity solvers.")
      .def_prop_ro(
          "tmm",
          [](const SolverRegistry &self) -> std::shared_ptr<ThermalMathematicalModel> {
            return self.tmm_ptr();
          },
          "The model the solvers work on.");
}

inline void register_callbacks(nb::module_ &m) {
  using pycanha::CallbackContext;
  using pycanha::CallbackRegistry;
  using pycanha::ThermalMathematicalModel;
  using pycanha::ThermalModel;
  using pycanha::solvers::Solver;

  nb::class_<CallbackContext>(m, "CallbackContext",
                              "Callback execution context for model-owned callbacks.")
      .def_prop_ro(
          "tm", [](CallbackContext &self) -> ThermalModel & { return self.tm(); },
          nb::rv_policy::reference_internal, "Reference to the owning ThermalModel.")
      .def_prop_ro(
          "tmm",
          [](CallbackContext &self) -> ThermalMathematicalModel & { return self.tmm(); },
          nb::rv_policy::reference_internal,
          "Reference to the owning ThermalMathematicalModel.")
      .def_prop_ro(
          "solver", [](CallbackContext &self) -> Solver & { return self.solver(); },
          nb::rv_policy::reference_internal,
          "The solver running the callback, as its own type (SteadyState, "
          "CrankNicolson, JacobianPropagation).")
      .def_prop_ro("time", &CallbackContext::time, "Current model time [s].");

  nb::class_<CallbackRegistry>(m, "CallbackRegistry",
                               "Model-owned callback registry for solver execution hooks.",
                               nb::type_slots(callback_registry_gc_slots))
      .def_rw("active", &CallbackRegistry::active,
              "Master switch enabling or disabling callback execution.")
      .def_rw("solver_loop", &CallbackRegistry::solver_loop,
              "Python callback invoked during solver iterations.")
      .def_rw("time_change", &CallbackRegistry::time_change,
              "Python callback invoked when transient time changes.")
      .def_rw("after_timestep", &CallbackRegistry::after_timestep,
              "Python callback invoked after each transient timestep.")
      .def("invoke_solver_loop", &CallbackRegistry::invoke_solver_loop,
           "Invoke the solver-loop callback immediately.")
      .def("invoke_time_change", &CallbackRegistry::invoke_time_change,
           "Invoke the time-change callback immediately.")
      .def("invoke_after_timestep", &CallbackRegistry::invoke_after_timestep,
           "Invoke the after-timestep callback immediately.");
}

} // namespace pycanha::bindings::solvers
