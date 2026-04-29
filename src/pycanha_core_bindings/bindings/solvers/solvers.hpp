#pragma once

#include <memory>
#include <string>

#include <nanobind/stl/function.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "pycanha-core/solvers/callback_registry.hpp"
#include "pycanha-core/solvers/ss.hpp"
#include "pycanha-core/solvers/sslu.hpp"
#include "pycanha-core/solvers/solver_registry.hpp"
#include "pycanha-core/thermaldata/data_model.hpp"
#include "pycanha-core/solvers/ts.hpp"
#include "pycanha-core/solvers/tscn.hpp"
#include "pycanha-core/solvers/tscnrl.hpp"
#include "pycanha-core/solvers/tscnrlds.hpp"
#include "pycanha-core/solvers/tscnrlds_jacobian.hpp"
#include "pycanha-core/tmm/thermalmathematicalmodel.hpp"
#include "pycanha-core/tmm/thermalmodel.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::solvers {

// Helper to access protected TransientSolver members via pointer-to-member.
// TransientSolverView inherits from TransientSolver and thus can name the
// protected members; the resulting pointer-to-member has base-class type
// and can be applied to any TransientSolver reference — fully well-defined.
struct TransientSolverView : pycanha::TransientSolver {
  TransientSolverView() = delete;

     static auto get_output_model_name(const pycanha::TransientSolver &s)
      -> const std::string & {
          constexpr auto ptr = &TransientSolverView::output_model_name;
    return s.*ptr;
  }
     static auto get_output_config(pycanha::TransientSolver &s)
               -> pycanha::SolverOutputConfig & {
          constexpr auto ptr = &TransientSolverView::output_config;
          return s.*ptr;
     }
  static auto get_time(const pycanha::TransientSolver &s) -> double {
    constexpr auto ptr = &TransientSolverView::time;
    return s.*ptr;
  }
  static auto get_time_iter(const pycanha::TransientSolver &s) -> int {
    constexpr auto ptr = &TransientSolverView::time_iter;
    return s.*ptr;
  }
};

inline void register_solvers(nb::module_ &m) {
     using pycanha::CallbackContext;
     using pycanha::CallbackRegistry;
     using pycanha::DataModel;
  using pycanha::Solver;
     using pycanha::SolverOutputConfig;
     using pycanha::SolverRegistry;
  using pycanha::SSLU;
  using pycanha::SteadyStateSolver;
  using pycanha::ThermalMathematicalModel;
     using pycanha::ThermalModel;
  using pycanha::TransientSolver;
  using pycanha::TSCN;
  using pycanha::TSCNRL;
  using pycanha::TSCNRLDS;
  using pycanha::TSCNRLDS_JACOBIAN;

  nb::class_<SolverOutputConfig>(
      m, "SolverOutputConfig",
      "Controls which transient solver attributes are written to the output model.")
      .def(nb::init<>(), "Create a default output configuration.")
      .def("output_all_dense", &SolverOutputConfig::output_all_dense,
           "Enable all dense node attributes.")
      .def("output_all", &SolverOutputConfig::output_all,
           "Enable all dense, sparse, and matrix output attributes.")
      .def("add", &SolverOutputConfig::add, "attr"_a,
           "Enable an output attribute.")
      .def("remove", &SolverOutputConfig::remove, "attr"_a,
           "Disable an output attribute.")
      .def("has", &SolverOutputConfig::has, "attr"_a,
           "Return whether the attribute is enabled.")
      .def_prop_ro(
          "attributes",
          [](const SolverOutputConfig &self) {
            return std::vector<pycanha::DataModelAttribute>(
                self.attributes.begin(), self.attributes.end());
          },
          "List of enabled output attributes.");

  nb::class_<Solver>(m, "Solver",
                    "Abstract base class for thermal solvers.\n\n"
                    "Lifecycle: initialize() -> solve() -> deinitialize().")
      .def_rw("max_iters", &Solver::max_iters,
              "Maximum number of solver iterations per step.")
      .def_rw("abstol_temp", &Solver::abstol_temp,
              "Absolute temperature convergence tolerance [K].")
      .def_rw("abstol_enrgy", &Solver::abstol_enrgy,
              "Absolute energy convergence tolerance [W].")
      .def_rw("eps_capacity", &Solver::eps_capacity,
              "Minimum thermal capacity threshold [J/K].")
      .def_rw("eps_time", &Solver::eps_time,
              "Time step epsilon [s].")
      .def_rw("eps_coupling", &Solver::eps_coupling,
              "Minimum coupling value threshold.")
      .def_rw("pardiso_iparm_3", &Solver::pardiso_iparm_3,
              "MKL PARDISO iparm[3] parameter (preconditioner control).")
      .def_prop_ro("solver_iter",
                   [](const Solver &self) { return self.solver_iter; },
                   "Current solver iteration count.")
      .def_prop_ro(
          "solver_name",
          [](const Solver &self) -> const std::string & {
            return self.solver_name;
          },
          nb::rv_policy::reference_internal,
          "Name of the solver.")
      .def_prop_ro("solver_initialized",
                   [](const Solver &self) { return self.solver_initialized; },
                   "Whether initialize() has been called.")
      .def_prop_ro("solver_converged",
                   [](const Solver &self) { return self.solver_converged; },
                   "Whether the solver has converged.");

  nb::class_<SteadyStateSolver, Solver>(m, "SteadyStateSolver",
                                        "Base class for steady-state (time-independent) solvers.");

  nb::class_<TransientSolver, Solver>(m, "TransientSolver",
                                      "Base class for transient (time-dependent) solvers.")
      .def("set_simulation_time", &TransientSolver::set_simulation_time,
           "start_time"_a, "end_time"_a, "dtime"_a, "output_stride"_a,
           "Configure the transient simulation time window and output interval.")
      .def_prop_ro(
                         "output_model_name",
          [](const TransientSolver &self) -> const std::string & {
                              return TransientSolverView::get_output_model_name(self);
          },
          nb::rv_policy::reference_internal,
                         "Name of the DataModel where output is stored.")
               .def_prop_ro(
                         "output_config",
                         [](TransientSolver &self) -> SolverOutputConfig & {
                              return TransientSolverView::get_output_config(self);
                         },
                         nb::rv_policy::reference_internal,
                         "Reference to the output configuration.")
                 .def_prop_ro(
                      "output_model",
                      [](TransientSolver &self) -> DataModel & { return self.output_model(); },
                      nb::rv_policy::reference_internal,
                      "Reference to the transient output model.")
      .def_prop_ro(
          "time",
          [](const TransientSolver &self) {
            return TransientSolverView::get_time(self);
          },
          "Current simulation time [s].")
      .def_prop_ro(
          "time_iter",
          [](const TransientSolver &self) {
            return TransientSolverView::get_time_iter(self);
          },
          "Current time iteration index.");

  nb::class_<TSCN, TransientSolver>(
      m, "TSCN",
      "Base class for Crank-Nicolson transient solvers.");
  nb::class_<TSCNRL, TSCN>(
      m, "TSCNRL",
      "Transient Crank-Nicolson solver with radiation linearization.");

  nb::class_<SSLU, SteadyStateSolver>(m, "SSLU",
                                      "Steady-state solver using sparse LU decomposition.\n\n"
                                      "Solves the non-linear steady-state thermal equation\n"
                                      "iteratively using Eigen SparseLU factorization.")
      .def(nb::init<std::shared_ptr<ThermalMathematicalModel>>(), "tmm"_a,
           nb::keep_alive<1, 2>(),
           "Create a solver bound to a ThermalMathematicalModel.")
      .def("initialize", &SSLU::initialize,
           "Allocate solver resources and prepare matrices.")
      .def("solve", &SSLU::solve,
           "Run the steady-state solve to convergence.")
      .def("deinitialize", &SSLU::deinitialize,
           "Release solver resources.");

  nb::class_<TSCNRLDS, TSCNRL>(
      m, "TSCNRLDS",
      "Transient Crank-Nicolson solver with radiation linearization\n"
      "and direct sparse factorization.\n\n"
      "Uses MKL PARDISO when available, otherwise Eigen SparseLU.")
      .def(nb::init<std::shared_ptr<ThermalMathematicalModel>>(), "tmm"_a,
           nb::keep_alive<1, 2>(),
           "Create a solver bound to a ThermalMathematicalModel.")
      .def("initialize", &TSCNRLDS::initialize,
           "Allocate solver resources and prepare matrices.")
      .def("solve", &TSCNRLDS::solve,
           "Run the transient simulation over the configured time window.")
      .def("deinitialize", &TSCNRLDS::deinitialize,
           "Release solver resources.");

  nb::class_<TSCNRLDS_JACOBIAN, TSCNRLDS>(
      m, "TSCNRLDS_JACOBIAN",
      "TSCNRLDS solver extended with Jacobian (sensitivity) output.\n\n"
      "Computes dT/dp for each parameter during the transient simulation.")
      .def(nb::init<std::shared_ptr<ThermalMathematicalModel>>(), "tmm"_a,
           nb::keep_alive<1, 2>(),
           "Create a Jacobian solver bound to a ThermalMathematicalModel.")
      .def("initialize", &TSCNRLDS_JACOBIAN::initialize,
           "Allocate solver resources and collect parameter names.")
      .def("solve", &TSCNRLDS_JACOBIAN::solve,
           "Run the transient simulation with Jacobian computation.")
      .def("deinitialize", &TSCNRLDS_JACOBIAN::deinitialize,
           "Release solver resources.")
      .def_prop_ro(
          "parameter_names",
          [](const TSCNRLDS_JACOBIAN &self) { return self.parameter_names(); },
                         "List of parameter names referenced by Jacobian-enabled formulas.")
               .def_prop_ro(
                         "derivative_parameter_names",
                         [](const TSCNRLDS_JACOBIAN &self) {
                              return self.derivative_parameter_names();
                         },
                         "Ordered derivative-parameter subset used for Jacobian columns.");

     nb::class_<CallbackContext>(m, "CallbackContext",
                                                                           "Callback execution context for model-owned callbacks.")
               .def_prop_ro(
                         "tm",
                         [](CallbackContext &self) -> ThermalModel & { return self.tm(); },
                         nb::rv_policy::reference_internal,
                         "Reference to the owning ThermalModel.")
               .def_prop_ro(
                         "tmm",
                         [](CallbackContext &self) -> ThermalMathematicalModel & {
                              return self.tmm();
                         },
                         nb::rv_policy::reference_internal,
                         "Reference to the owning ThermalMathematicalModel.")
               .def_prop_ro(
                         "solver",
                         [](CallbackContext &self) -> Solver & { return self.solver(); },
                         nb::rv_policy::reference_internal,
                         "Reference to the currently active solver.")
               .def_prop_ro("time", &CallbackContext::time,
                                              "Current model time [s].");

     nb::class_<SolverRegistry>(m, "SolverRegistry",
                                                                       "Lazy registry of persistent model-owned solver instances.")
               .def_prop_ro(
                         "sslu",
                         [](SolverRegistry &self) -> SSLU & { return self.sslu(); },
                         nb::rv_policy::reference_internal,
                         "Persistent steady-state sparse-LU solver.")
               .def_prop_ro(
                         "tscnrlds",
                         [](SolverRegistry &self) -> TSCNRLDS & { return self.tscnrlds(); },
                         nb::rv_policy::reference_internal,
                         "Persistent transient direct sparse solver.")
               .def_prop_ro(
                         "tscnrlds_jacobian",
                         [](SolverRegistry &self) -> TSCNRLDS_JACOBIAN & {
                              return self.tscnrlds_jacobian();
                         },
                         nb::rv_policy::reference_internal,
                         "Persistent transient Jacobian solver.")
               .def_prop_ro(
                         "tmm",
                         [](const SolverRegistry &self) -> std::shared_ptr<ThermalMathematicalModel> {
                              return self.tmm_ptr();
                         },
                         "Shared pointer to the associated ThermalMathematicalModel.");

     nb::class_<CallbackRegistry>(m, "CallbackRegistry",
                                                                            "Model-owned callback registry for solver execution hooks.")
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
