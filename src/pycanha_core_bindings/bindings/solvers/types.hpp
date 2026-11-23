#pragma once

#include <memory>
#include <string>
#include <vector>

#include <Eigen/Core>
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "bindings/solvers/solver.hpp"
#include "pycanha-core/solvers/crank_nicolson.hpp"
#include "pycanha-core/solvers/jacobian_propagation.hpp"
#include "pycanha-core/solvers/steady_state.hpp"
#include "pycanha-core/thermaldata/data_model.hpp"
#include "pycanha-core/tmm/thermalmathematicalmodel.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::solvers {

constexpr const char *set_simulation_time_doc =
    "Set start_time, end_time, time_step and output_interval [s] at once. "
    "Raises ValueError for an end before the start, a time step that is not "
    "positive or a negative output interval.";

constexpr const char *output_model_doc =
    "The output model of the last initialize(), in the model's thermal data.";

inline void register_steady(nb::module_ &m) {
  using pycanha::ThermalMathematicalModel;
  using pycanha::bindings::utils::LogDrainGuard;
  using pycanha::solvers::Solver;
  using pycanha::solvers::SolverType;
  using pycanha::solvers::steady::SteadyState;

  nb::class_<SteadyState, Solver> cls(
      m, "SteadyState",
      "Steady state of the network.\n\n"
      "Newton linearisation of the radiation, assembled into a sparse matrix "
      "whose pattern is built once in initialize(), or into a dense one, and "
      "solved with a direct factorization (see the settings storage, backend, "
      "factorization, reuse_factors, pardiso, eigen). Each iteration "
      "refactorizes only when the matrix values differ from those of the last "
      "factorization, so the second iteration of a linear model, or a new "
      "solve of an unchanged model, costs one solve with the existing factors. "
      "With fixed_couplings the couplings are read once per solve() and the "
      "matrices are not compared. With reuse_factors each iteration solves for "
      "the temperature change, A dT = b - A T, so the residual of the "
      "iteration on old factors stays in a change that shrinks as the solve "
      "converges.\n\n"
      "A singular matrix (a group of diffusive nodes without a path to a "
      "boundary node) is reported as an ERROR and leaves the temperatures of "
      "that solve untouched; a solve that does not converge within "
      "max_iterations logs a WARN and keeps the temperatures of its last "
      "iteration.");
  cls.def(nb::init<std::shared_ptr<ThermalMathematicalModel>>(), "tmm"_a,
          nb::keep_alive<1, 2>(), "Create a solver for a model, with unset settings.")
      .def_prop_ro("uses_cholesky", &SteadyState::uses_cholesky,
                   "The last initialize() chose a Cholesky factorization.")
      .def_prop_ro("num_factorizations", &SteadyState::num_factorizations,
                   "Numerical factorizations done by the last solve().")
      .def_prop_ro("iterations", &SteadyState::iterations,
                   "Iterations done by the last solve().")
      .def("jacobian", &SteadyState::jacobian, nb::call_guard<LogDrainGuard>(),
           "Derivatives of the diffusive temperatures of the last solve() with "
           "respect to the parameters selected in "
           "Formulas.parameters_with_derivatives [K per unit of the parameter]: "
           "one row per diffusive node, in the model's order, and one column "
           "per derivative_parameter_names entry.\n\n"
           "The network is linearized at the converged temperatures, which "
           "gives the exact tangent of the energy balance, factorized once, and "
           "solved for the change of the balance each parameter makes through "
           "the entities of its parameter formulas: conductive and radiative "
           "couplings, the heat loads (QI, QS, QA, QE, QR) and the temperatures "
           "of boundary nodes. Capacities, and the temperatures of diffusive "
           "nodes (a start value), do not enter a steady state. A coupling that "
           "a callback sets from the temperatures is taken at its converged "
           "value: its own temperature dependence is not in the tangent.\n\n"
           "Raises RuntimeError when the solver is not initialized or the last "
           "solve() did not converge, and ValueError when no derivative "
           "parameter is selected or a selected one enters no parameter "
           "formula.")
      .def_prop_ro("derivative_parameter_names",
                   &SteadyState::derivative_parameter_names,
                   "The parameters of the columns of the last jacobian().")
      .def(
          "linear_response",
          [](SteadyState &self, const Eigen::VectorXd &heat) -> Eigen::VectorXd {
            return self.linear_response(heat).col(0);
          },
          "heat"_a, nb::call_guard<LogDrainGuard>(),
          "First-order change of the diffusive temperatures [K] for heat [W] "
          "added to the diffusive nodes, at the temperatures of the last "
          "solve(): `heat` and the result have one entry per diffusive node, in "
          "the model's order (a 2-D `heat` gives one column per load case). The "
          "same factorization as jacobian(), one solve per column. Raises as "
          "jacobian(), and ValueError for a length other than the number of "
          "diffusive nodes.")
      .def("linear_response", &SteadyState::linear_response, "heat"_a,
           nb::call_guard<LogDrainGuard>(),
           "One column of temperature changes per column of `heat`.");
  add_setting_attributes(cls, SolverType::STEADY_STATE);
}

inline void register_transient(nb::module_ &m) {
  using pycanha::DataModel;
  using pycanha::ThermalMathematicalModel;
  using pycanha::solvers::Solver;
  using pycanha::solvers::SolverType;
  using pycanha::solvers::transient::CrankNicolson;

  nb::class_<CrankNicolson, Solver> cls(
      m, "CrankNicolson",
      "Crank-Nicolson transient with the radiation linearised at every inner "
      "iteration, solved with a sparse or dense direct factorization.\n\n"
      "The time and output settings (start_time, end_time, time_step, "
      "output_interval, output_attributes, output_name) are read by "
      "initialize(), which also creates the output model in the thermal data; "
      "time_step is required. The factorization is LU, or Cholesky for a model "
      "without radiative couplings (see the setting factorization). Every "
      "inner iteration reads the couplings and the capacities again, so "
      "formulas and callbacks may change them during the run; with "
      "fixed_couplings they are read once per solve() and the matrix is "
      "factorized again only when the temperatures change it. With "
      "reuse_factors each inner iteration solves for the temperature change, "
      "so the residual of the iteration on old factors does not stay in the "
      "temperatures. A step whose inner iterations do not converge within "
      "max_iterations keeps its last iteration and the run goes on: the first "
      "such step logs a WARN, the end of the run one summary WARN, and "
      "report() lists them.");
  cls.def(nb::init<std::shared_ptr<ThermalMathematicalModel>>(), "tmm"_a,
          nb::keep_alive<1, 2>(), "Create a solver for a model, with unset settings.")
      .def("set_simulation_time", &CrankNicolson::set_simulation_time, "start_time"_a,
           "end_time"_a, "time_step"_a, "output_interval"_a, set_simulation_time_doc)
      .def_prop_ro(
          "output_model",
          [](CrankNicolson &self) -> DataModel & { return self.output_model(); },
          nb::rv_policy::reference_internal, output_model_doc);
  add_setting_attributes(cls, SolverType::CRANK_NICOLSON);
}

// JacobianPropagation derives from CrankNicolson in C++, but its Python base
// is Solver: it must not inherit the attributes of the settings that do not
// apply to it (reuse_factors, reuse_tolerance). The CrankNicolson members it
// keeps are bound again here, through the derived type, because a method
// bound on CrankNicolson would not accept a Python object that does not
// derive from it.
inline void register_sensitivity(nb::module_ &m) {
  using pycanha::DataModel;
  using pycanha::ThermalMathematicalModel;
  using pycanha::solvers::Solver;
  using pycanha::solvers::SolverType;
  using pycanha::solvers::sensitivity::JacobianPropagation;

  nb::class_<JacobianPropagation, Solver> cls(
      m, "JacobianPropagation",
      "Crank-Nicolson transient that also propagates the derivatives of the "
      "diffusive temperatures with respect to the parameters selected in "
      "Formulas.parameters_with_derivatives.\n\n"
      "The derivatives are stored as the JAC attribute of the output model, at "
      "the same output times as the temperatures. The sensitivity solve uses "
      "the factors of the last step's matrix, so reuse_factors does not "
      "apply. initialize() raises ValueError when no derivative parameter is "
      "selected.");
  cls.def(nb::init<std::shared_ptr<ThermalMathematicalModel>>(), "tmm"_a,
          nb::keep_alive<1, 2>(), "Create a solver for a model, with unset settings.")
      .def(
          "set_simulation_time",
          [](JacobianPropagation &self, double start_time, double end_time,
             double time_step, double output_interval) {
            self.set_simulation_time(start_time, end_time, time_step, output_interval);
          },
          "start_time"_a, "end_time"_a, "time_step"_a, "output_interval"_a,
          set_simulation_time_doc)
      .def_prop_ro(
          "output_model",
          [](JacobianPropagation &self) -> DataModel & { return self.output_model(); },
          nb::rv_policy::reference_internal, output_model_doc)
      .def_prop_ro("parameter_names", &JacobianPropagation::parameter_names,
                   "Parameters the parameter formulas depend on, in first-use order.")
      .def_prop_ro("derivative_parameter_names",
                   &JacobianPropagation::derivative_parameter_names,
                   "Parameters whose derivatives are propagated (the columns of JAC).");
  add_setting_attributes(cls, SolverType::JACOBIAN_PROPAGATION);
}

} // namespace pycanha::bindings::solvers
