#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include <nanobind/nanobind.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

#include "bindings/solvers/settings.hpp"
#include "pycanha-core/solvers/model_changed_error.hpp"
#include "pycanha-core/solvers/report.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::solvers {

inline void register_report(nb::module_ &m) {
  using namespace pycanha::solvers;

  nb::class_<SolverMemory>(
      m, "SolverMemory",
      "Memory of a solver [bytes]: its factors, its own matrices and vectors, "
      "and the output it stores in the thermal data.")
      .def_ro("factors", &SolverMemory::factors,
              "The factorization: PARDISO's own figure (analysis and "
              "factorization peak), or Eigen's factor non-zeros.")
      .def_ro("matrix", &SolverMemory::matrix,
              "The matrices, index tables and vectors the solver assembles.")
      .def_ro("output", &SolverMemory::output,
              "The output model of a transient (stored temperatures, loads, "
              "coupling matrices, Jacobian).")
      .def_ro("exact", &SolverMemory::exact,
              "The factor size is known, not predicted: PARDISO two-level and "
              "Eigen Cholesky from their analysis, PARDISO one-level (an upper "
              "bound before) and Eigen LU after their first factorization.")
      .def_prop_ro("total", &SolverMemory::total, "factors + matrix + output.")
      .def("__str__", &SolverMemory::to_string,
           "\"1.21 GB (factors 1.02 GB, matrix 121 MB, output 64.0 MB)\".");

  m.def("format_bytes", &format_bytes, "bytes"_a,
        "\"1.21 GB\", \"512 kB\", \"17 B\" (powers of 1000).");
  m.def("parse_memory_size", &parse_memory_size, "text"_a,
        "Bytes of \"16 GB\", \"512 MB\", \"1.5 GiB\" or \"2000000\". kB, MB, GB, TB "
        "are powers of 1000, KiB, MiB, GiB, TiB powers of 1024; case and the "
        "space before the unit do not matter. Raises ValueError for anything "
        "else, and for a size that is not positive.");

  nb::enum_<SolveStatus> status(m, "SolveStatus", "How the last solve() ended.");
  status.value("NOT_RUN", SolveStatus::NOT_RUN, "No solve() since initialize().")
      .value("CONVERGED", SolveStatus::CONVERGED,
             "Every iteration loop met the tolerance.")
      .value("NOT_CONVERGED", SolveStatus::NOT_CONVERGED,
             "Some loop stopped at max_iterations.")
      .value("FAILED", SolveStatus::FAILED,
             "solve() stopped early: an error or an exception.");
  add_enum_str(status);

  nb::class_<NonConvergedStep>(m, "NonConvergedStep",
                               "A time step (or the steady state) that stopped at "
                               "max_iterations.")
      .def_ro("step", &NonConvergedStep::step,
              "Time step, counted from 1; 0 for the steady state.")
      .def_ro("time", &NonConvergedStep::time, "Time at the end of the step [s].")
      .def_ro("iterations", &NonConvergedStep::iterations, "Iterations done.")
      .def_ro("max_dT", &NonConvergedStep::max_dT,
              "Largest change of the last iteration [K].")
      .def_ro("node", &NonConvergedStep::node, "Node number with that change.");

  nb::class_<SolverReport> report(
      m, "SolverReport",
      "What the last solve() did; Solver.report() returns a copy.\n\n"
      "The counters are aggregates kept during the run; the text is built only "
      "by __str__. Only the first max_listed_steps non-converged steps are "
      "listed, all of them are counted.");
  report.attr("max_listed_steps") = SolverReport::max_listed_steps;
  report.def_ro("solver", &SolverReport::solver, "The solver name, \"CrankNicolson\".")
      .def_ro("status", &SolverReport::status, "How the last solve() ended.")
      .def_ro("steps", &SolverReport::steps, "Time steps done; 0 for the steady state.")
      .def_ro("iterations", &SolverReport::iterations,
              "Iterations of every loop, added up.")
      .def_ro("max_step_iterations", &SolverReport::max_step_iterations,
              "The longest iteration loop.")
      .def_ro("nonconverged_steps", &SolverReport::nonconverged_steps,
              "Loops that stopped at max_iterations.")
      .def_ro("nonconverged", &SolverReport::nonconverged,
              "The first max_listed_steps of them.")
      .def_ro("max_dT", &SolverReport::max_dT,
              "Largest change of the last iteration [K].")
      .def_ro("reuse_error", &SolverReport::reuse_error,
              "Estimated error the iteration on old factors leaves in the "
              "temperatures [K]: reuse_tolerance times the largest change of the "
              "last iteration of the steady state, or of any time step; 0 without "
              "reuse_factors. An order of magnitude, not a bound: LAPACK's FGMRES, "
              "which stops on the residual without the factors, and long "
              "transients, where it adds up over the steps, can leave a few times "
              "more. Rounding errors, the same as a direct solve's, come on top.")
      .def_ro("factorizations", &SolverReport::factorizations,
              "Numerical factorizations.")
      .def_ro("solves", &SolverReport::solves,
              "Linear solves; those beyond the factorizations used existing or "
              "earlier factors.")
      .def_ro("initialize_time", &SolverReport::initialize_time,
              "Wall time of initialize() [s].")
      .def_ro("solve_time", &SolverReport::solve_time, "Wall time of solve() [s].")
      .def_ro("callback_time", &SolverReport::callback_time,
              "Time inside the callbacks [s].")
      .def_ro("linear_time", &SolverReport::linear_time,
              "Factorizations and solves [s].")
      .def_ro("memory", &SolverReport::memory,
              "The solver's own memory after the run (not the process peak).")
      .def_ro("warnings", &SolverReport::warnings,
              "The warnings of the last initialize() and solve().")
      .def_prop_ro("converged", &SolverReport::converged,
                   "status is CONVERGED.")
      .def_prop_ro("reuse_rate", &SolverReport::reuse_rate,
                   "Fraction of the solves done without a new factorization.")
      .def("to_dict", &SolverReport::to_dict,
           "Flat: the non-converged list as parallel lists (\"nonconverged.step\", "
           "\"nonconverged.time\", ...), memory as \"memory.factors\", ...")
      .def("__str__", &SolverReport::to_string, "The report as text.");

  nb::exception<ModelChangedError>(
      m, "ModelChangedError", PyExc_RuntimeError)
      .attr("__doc__") =
      "Raised by a solver when the model's nodes or coupling patterns changed "
      "after initialize(): nodes added, removed, retyped or reordered, or "
      "couplings inserted or removed. The solver's sizes, matrix patterns and "
      "index tables were built for the old structure, so it refuses to go on; "
      "call initialize() again. New values of existing couplings, capacities, "
      "loads or temperatures are not structural and never raise it.";
}

} // namespace pycanha::bindings::solvers
