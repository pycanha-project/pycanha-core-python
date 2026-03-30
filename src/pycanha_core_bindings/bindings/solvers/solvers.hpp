#pragma once

#include <memory>
#include <string>

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>

#include "pycanha-core/solvers/ss.hpp"
#include "pycanha-core/solvers/sslu.hpp"
#include "pycanha-core/solvers/ts.hpp"
#include "pycanha-core/solvers/tscn.hpp"
#include "pycanha-core/solvers/tscnrl.hpp"
#include "pycanha-core/solvers/tscnrlds.hpp"
#include "pycanha-core/tmm/thermalmathematicalmodel.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::solvers {

inline void register_solvers(nb::module_ &m) {
  using pycanha::Solver;
  using pycanha::SSLU;
  using pycanha::SteadyStateSolver;
  using pycanha::ThermalMathematicalModel;
  using pycanha::TransientSolver;
  using pycanha::TSCN;
  using pycanha::TSCNRL;
  using pycanha::TSCNRLDS;

  nb::class_<Solver>(m, "Solver")
      .def_rw("MAX_ITERS", &Solver::MAX_ITERS)
      .def_rw("abstol_temp", &Solver::abstol_temp)
      .def_rw("abstol_enrgy", &Solver::abstol_enrgy)
      .def_rw("eps_capacity", &Solver::eps_capacity)
      .def_rw("eps_time", &Solver::eps_time)
      .def_rw("eps_coupling", &Solver::eps_coupling)
      .def_rw("pardiso_iparm_3", &Solver::pardiso_iparm_3)
      .def_prop_ro(
          "solver_iter", [](const Solver &self) { return self.solver_iter; })
      .def_prop_ro(
          "solver_name",
          [](const Solver &self) -> const std::string & {
            return self.solver_name;
          },
          nb::rv_policy::reference_internal)
      .def_prop_ro(
          "solver_initialized",
          [](const Solver &self) { return self.solver_initialized; })
      .def_prop_ro("solver_converged", [](const Solver &self) {
        return self.solver_converged;
      });

  nb::class_<SteadyStateSolver, Solver>(
      m, "SteadyStateSolver");

  nb::class_<TransientSolver, Solver>(
      m, "TransientSolver")
      .def("set_simulation_time", &TransientSolver::set_simulation_time,
           "start_time"_a, "end_time"_a, "dtime"_a, "output_stride"_a);

  nb::class_<TSCN, TransientSolver>(m, "TSCN");
  nb::class_<TSCNRL, TSCN>(m, "TSCNRL");

  nb::class_<SSLU, SteadyStateSolver>(m, "SSLU")
      .def(nb::init<std::shared_ptr<ThermalMathematicalModel>>(), "tmm"_a,
           nb::keep_alive<1, 2>())
      .def("initialize", &SSLU::initialize)
      .def("solve", &SSLU::solve)
      .def("deinitialize", &SSLU::deinitialize);

  nb::class_<TSCNRLDS, TSCNRL>(m, "TSCNRLDS")
      .def(nb::init<std::shared_ptr<ThermalMathematicalModel>>(), "tmm"_a,
           nb::keep_alive<1, 2>())
      .def("initialize", &TSCNRLDS::initialize)
      .def("solve", &TSCNRLDS::solve)
      .def("deinitialize", &TSCNRLDS::deinitialize);
}

} // namespace pycanha::bindings::solvers
