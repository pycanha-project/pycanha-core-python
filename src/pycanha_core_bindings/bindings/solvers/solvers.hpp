#pragma once

#include <memory>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "pycanha-core/solvers/ss.hpp"
#include "pycanha-core/solvers/sslu.hpp"
#include "pycanha-core/solvers/ts.hpp"
#include "pycanha-core/solvers/tscn.hpp"
#include "pycanha-core/solvers/tscnrl.hpp"
#include "pycanha-core/solvers/tscnrlds.hpp"
#include "pycanha-core/tmm/thermalmathematicalmodel.hpp"

namespace py = pybind11;
using namespace pybind11::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::solvers {

inline void register_solvers(py::module_ &m) {
  using pycanha::Solver;
  using pycanha::SSLU;
  using pycanha::SteadyStateSolver;
  using pycanha::ThermalMathematicalModel;
  using pycanha::TransientSolver;
  using pycanha::TSCN;
  using pycanha::TSCNRL;
  using pycanha::TSCNRLDS;

  py::class_<Solver, std::shared_ptr<Solver>>(m, "Solver")
      .def_readwrite("MAX_ITERS", &Solver::MAX_ITERS)
      .def_readwrite("abstol_temp", &Solver::abstol_temp)
      .def_readwrite("abstol_enrgy", &Solver::abstol_enrgy)
      .def_readwrite("eps_capacity", &Solver::eps_capacity)
      .def_readwrite("eps_time", &Solver::eps_time)
      .def_readwrite("eps_coupling", &Solver::eps_coupling)
      .def_readwrite("pardiso_iparm_3", &Solver::pardiso_iparm_3)
      .def_property_readonly(
          "solver_iter", [](const Solver &self) { return self.solver_iter; })
      .def_property_readonly(
          "solver_name",
          [](const Solver &self) -> const std::string & {
            return self.solver_name;
          },
          py::return_value_policy::reference_internal)
      .def_property_readonly(
          "solver_initialized",
          [](const Solver &self) { return self.solver_initialized; })
      .def_property_readonly("solver_converged", [](const Solver &self) {
        return self.solver_converged;
      });

  py::class_<SteadyStateSolver, Solver, std::shared_ptr<SteadyStateSolver>>(
      m, "SteadyStateSolver");

  py::class_<TransientSolver, Solver, std::shared_ptr<TransientSolver>>(
      m, "TransientSolver")
      .def("set_simulation_time", &TransientSolver::set_simulation_time,
           "start_time"_a, "end_time"_a, "dtime"_a, "output_stride"_a);

  py::class_<TSCN, TransientSolver, std::shared_ptr<TSCN>>(m, "TSCN");
  py::class_<TSCNRL, TSCN, std::shared_ptr<TSCNRL>>(m, "TSCNRL");

  py::class_<SSLU, SteadyStateSolver, std::shared_ptr<SSLU>>(m, "SSLU")
      .def(py::init<std::shared_ptr<ThermalMathematicalModel>>(), "tmm"_a,
           py::keep_alive<1, 2>())
      .def("initialize", &SSLU::initialize)
      .def("solve", &SSLU::solve)
      .def("deinitialize", &SSLU::deinitialize);

  py::class_<TSCNRLDS, TSCNRL, std::shared_ptr<TSCNRLDS>>(m, "TSCNRLDS")
      .def(py::init<std::shared_ptr<ThermalMathematicalModel>>(), "tmm"_a,
           py::keep_alive<1, 2>())
      .def("initialize", &TSCNRLDS::initialize)
      .def("solve", &TSCNRLDS::solve)
      .def("deinitialize", &TSCNRLDS::deinitialize);
}

} // namespace pycanha::bindings::solvers
