#pragma once

#include <memory>
#include <string>

#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "pycanha-core/solvers/ss.hpp"
#include "pycanha-core/solvers/sslu.hpp"
#include "pycanha-core/solvers/ts.hpp"
#include "pycanha-core/solvers/tscn.hpp"
#include "pycanha-core/solvers/tscnrl.hpp"
#include "pycanha-core/solvers/tscnrlds.hpp"
#include "pycanha-core/solvers/tscnrlds_jacobian.hpp"
#include "pycanha-core/tmm/thermalmathematicalmodel.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::solvers {

// Helper to access protected TransientSolver members via pointer-to-member.
// TransientSolverView inherits from TransientSolver and thus can name the
// protected members; the resulting pointer-to-member has base-class type
// and can be applied to any TransientSolver reference — fully well-defined.
struct TransientSolverView : pycanha::TransientSolver {
  TransientSolverView() = delete;

  static auto get_output_table_name(const pycanha::TransientSolver &s)
      -> const std::string & {
    constexpr auto ptr = &TransientSolverView::output_table_name;
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
  using pycanha::Solver;
  using pycanha::SSLU;
  using pycanha::SteadyStateSolver;
  using pycanha::ThermalMathematicalModel;
  using pycanha::TransientSolver;
  using pycanha::TSCN;
  using pycanha::TSCNRL;
  using pycanha::TSCNRLDS;
  using pycanha::TSCNRLDS_JACOBIAN;

  nb::class_<Solver>(m, "Solver",
                    "Abstract base class for thermal solvers.\n\n"
                    "Lifecycle: initialize() -> solve() -> deinitialize().")
      .def_rw("MAX_ITERS", &Solver::MAX_ITERS,
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
          "output_table_name",
          [](const TransientSolver &self) -> const std::string & {
            return TransientSolverView::get_output_table_name(self);
          },
          nb::rv_policy::reference_internal,
          "Name of the DenseTimeSeries table where output is stored.")
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
          "List of parameter names for which sensitivities are computed.")
      .def_rw("output_jacobian_table_name",
              &TSCNRLDS_JACOBIAN::output_jacobian_table_name,
              "Name of the ThermalData table where Jacobian results are stored.");
}

} // namespace pycanha::bindings::solvers
