#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <nanobind/nanobind.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/set.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>

#include "bindings/solvers/report.hpp"
#include "bindings/solvers/settings.hpp"
#include "bindings/utils/logger.hpp"
#include "pycanha-core/solvers/settings.hpp"
#include "pycanha-core/solvers/solver.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::solvers {

// The settings groups of a solver, as attributes that write through to it:
// solver.pardiso.level = ... replaces the solver's settings with a copy that
// has the new value, so the freeze check of set_settings() applies. The view
// keeps its solver alive.
struct PardisoView {
  pycanha::solvers::Solver *solver;
};
struct EigenView {
  pycanha::solvers::Solver *solver;
};
struct AccelerateView {
  pycanha::solvers::Solver *solver;
};

// Type of the field `Member` points to in `Owner`.
template <typename Owner, auto Member>
using member_type_t = std::remove_cvref_t<decltype(std::declval<Owner &>().*Member)>;

// One field of a settings group, on the group's view.
template <auto Group, auto Member, typename View>
void add_group_field(nb::class_<View> &cls, const char *name, std::string_view field) {
  using GroupType = member_type_t<pycanha::solvers::Settings, Group>;
  using Value = member_type_t<GroupType, Member>;
  cls.def_prop_rw(
      name,
      [](const View &view) -> Value { return (view.solver->settings().*Group).*Member; },
      [](const View &view, Value value) {
        pycanha::solvers::Settings settings = view.solver->settings();
        (settings.*Group).*Member = std::move(value);
        view.solver->set_settings(settings);
      },
      setting_doc(field));
}

inline void register_group_views(nb::module_ &m) {
  using pycanha::solvers::AccelerateSettings;
  using pycanha::solvers::EigenSettings;
  using pycanha::solvers::PardisoSettings;
  using pycanha::solvers::Settings;

  nb::class_<PardisoView> pardiso(m, "PardisoView",
                                  "The PARDISO settings of a solver. Every write goes "
                                  "through the solver, like Solver.set_settings(). "
                                  "None means unset.");
  add_group_field<&Settings::pardiso, &PardisoSettings::ordering>(pardiso, "ordering",
                                                                   "pardiso.ordering");
  add_group_field<&Settings::pardiso, &PardisoSettings::level>(pardiso, "level",
                                                                "pardiso.level");
  add_group_field<&Settings::pardiso, &PardisoSettings::iparm>(pardiso, "iparm",
                                                                "pardiso.iparm");
  add_group_field<&Settings::pardiso, &PardisoSettings::verbose>(pardiso, "verbose",
                                                                  "pardiso.verbose");

  nb::class_<EigenView> eigen(m, "EigenView",
                              "The Eigen settings of a solver. Every write goes "
                              "through the solver, like Solver.set_settings(). None "
                              "means unset.");
  add_group_field<&Settings::eigen, &EigenSettings::ordering>(eigen, "ordering",
                                                               "eigen.ordering");

  nb::class_<AccelerateView> accelerate(m, "AccelerateView",
                                        "The Accelerate settings of a solver. Every "
                                        "write goes through the solver, like "
                                        "Solver.set_settings(). None means unset.");
  add_group_field<&Settings::accelerate, &AccelerateSettings::ordering>(
      accelerate, "ordering", "accelerate.ordering");
}

// One top-level setting as an attribute of a solver class, only when it
// applies to the class's solver type (so steady.time_step is an
// AttributeError). Reading gives the solver's value (None when unset);
// writing replaces the solver's settings with a copy that has the new value,
// so an INIT field raises RuntimeError while the solver is initialised.
template <auto Member, typename Cls>
void add_setting_attribute(Cls &cls, const char *name, pycanha::solvers::SolverType type) {
  using pycanha::solvers::Settings;
  using pycanha::solvers::Solver;
  using Value = member_type_t<Settings, Member>;
  if (!pycanha::solvers::setting_field(name).applies_to(type)) {
    return;
  }
  cls.def_prop_rw(
      name, [](const Solver &solver) -> Value { return solver.settings().*Member; },
      [](Solver &solver, Value value) {
        Settings settings = solver.settings();
        settings.*Member = std::move(value);
        solver.set_settings(settings);
      },
      setting_doc(name));
}

// Whether any field of a settings group applies to a solver type.
inline bool group_applies(std::string_view group, pycanha::solvers::SolverType type) {
  for (const pycanha::solvers::SettingField &field : pycanha::solvers::setting_fields()) {
    if (field.group == group && field.applies_to(type)) {
      return true;
    }
  }
  return false;
}

template <typename View, typename Cls>
void add_group_view(Cls &cls, const char *name, const char *doc,
                    pycanha::solvers::SolverType type) {
  if (!group_applies(name, type)) {
    return;
  }
  cls.def_prop_ro(
      name, [](pycanha::solvers::Solver &solver) { return View{&solver}; },
      nb::keep_alive<0, 1>(), doc);
}

// Every setting of the core's field table that applies to `type`, as
// attributes of `cls`. A field the core adds must be added here too; the
// test suite compares these attributes with setting_fields().
template <typename Cls>
void add_setting_attributes(Cls &cls, pycanha::solvers::SolverType type) {
  using pycanha::solvers::Settings;
  add_setting_attribute<&Settings::tolerance>(cls, "tolerance", type);
  add_setting_attribute<&Settings::max_iterations>(cls, "max_iterations", type);
  add_setting_attribute<&Settings::storage>(cls, "storage", type);
  add_setting_attribute<&Settings::backend>(cls, "backend", type);
  add_setting_attribute<&Settings::factorization>(cls, "factorization", type);
  add_setting_attribute<&Settings::reuse_factors>(cls, "reuse_factors", type);
  add_setting_attribute<&Settings::reuse_tolerance>(cls, "reuse_tolerance", type);
  add_setting_attribute<&Settings::fixed_couplings>(cls, "fixed_couplings", type);
  add_setting_attribute<&Settings::threads>(cls, "threads", type);
  add_setting_attribute<&Settings::start_time>(cls, "start_time", type);
  add_setting_attribute<&Settings::end_time>(cls, "end_time", type);
  add_setting_attribute<&Settings::time_step>(cls, "time_step", type);
  add_setting_attribute<&Settings::output_interval>(cls, "output_interval", type);
  add_setting_attribute<&Settings::output_attributes>(cls, "output_attributes", type);
  add_setting_attribute<&Settings::output_name>(cls, "output_name", type);
  add_setting_attribute<&Settings::capacity_delta>(cls, "capacity_delta", type);
  add_group_view<PardisoView>(cls, "pardiso",
                              "The PARDISO settings (see PardisoView), written "
                              "through to this solver.",
                              type);
  add_group_view<EigenView>(cls, "eigen",
                            "The Eigen settings (see EigenView), written through "
                            "to this solver.",
                            type);
  add_group_view<AccelerateView>(cls, "accelerate",
                                 "The Accelerate settings (see AccelerateView), "
                                 "written through to this solver.",
                                 type);
}

inline void register_solver_base(nb::module_ &m) {
  using pycanha::bindings::utils::LogDrainGuard;
  using pycanha::solvers::Settings;
  using pycanha::solvers::Solver;
  using pycanha::solvers::SolverReport;

  register_group_views(m);

  nb::class_<Solver>(
      m, "Solver",
      "Base of every solver type.\n\n"
      "A solver works on one model with one Settings. initialize() resolves "
      "the settings (see resolve()), warns once about the set fields that do "
      "not apply, and builds everything the solver needs; solve() runs it. "
      "The INIT settings are frozen while the solver is initialised; the LIVE "
      "ones are read again at every solve(). Each solver class has an "
      "attribute for every setting that applies to its type.\n\n"
      "solve() raises ModelChangedError when the model's nodes or coupling "
      "patterns changed since initialize(), checked at the start and after "
      "every callback. An iteration loop that stops at max_iterations logs "
      "one WARN (and, in a transient, one summary WARN at the end); report() "
      "has the details.")
      .def("initialize", &Solver::initialize, nb::call_guard<LogDrainGuard>(),
           "Resolve the settings for the current model and build everything the "
           "solver needs. Raises ValueError for contradictory settings.")
      .def("solve", &Solver::solve, nb::call_guard<LogDrainGuard>(),
           "Run the solver. Raises ModelChangedError when the model structure "
           "changed since initialize().")
      .def("deinitialize", &Solver::deinitialize,
           "Release what initialize() built.")
      .def_prop_ro("solver_type", &Solver::solver_type, "The solver type.")
      .def_prop_ro("name", &Solver::name,
                   "\"SteadyState\", \"CrankNicolson\", \"JacobianPropagation\".")
      .def("settings", &Solver::settings,
           "A copy of the settings, with solver_type set to this solver's type.")
      .def("set_settings", &Solver::set_settings, "settings"_a,
           "Copy settings and set its solver_type to this solver's. Raises "
           "RuntimeError when an INIT field changes while the solver is "
           "initialised.")
      .def(
          "set",
          [](Solver &self, std::string_view name, nb::handle value) {
            self.set(name, to_setting_value(value));
          },
          "name"_a, "value"_a,
          "Change one setting by name; see Settings.set() and set_settings().")
      .def("unset", &Solver::unset, "name"_a,
           "Unset one setting by name, so that it takes its default.")
      .def("resolved_settings", &Solver::resolved_settings,
           "The settings resolved for the current model, as initialize() would.")
      .def_prop_ro("initialized", &Solver::initialized,
                   "Whether initialize() has run (and deinitialize() not since).")
      .def_prop_ro("converged", &Solver::converged,
                   "The last solve() converged in every iteration loop.")
      .def(
          "report", [](const Solver &self) -> SolverReport { return self.report(); },
          "A copy of what the last solve() did; initialize() clears it.")
      .def("explain", &Solver::explain,
           "Every setting that applies, with its effective value, where it comes "
           "from and why; the PARDISO iparm entries the settings give; the set "
           "fields that do not apply, the warnings and the contradictions "
           "(listed, not raised).")
      .def("origin", &Solver::origin, "name"_a,
           "DEFAULT, USER, or AUTO_CONFIGURE for a value auto_configure() set and "
           "nothing changed since; DEFAULT for a field that does not apply. "
           "Raises ValueError for an unknown name.")
      .def("estimate_memory", &Solver::estimate_memory, nb::call_guard<LogDrainGuard>(),
           "The memory this solver needs for the current model and settings.\n\n"
           "Without an initialized solver it builds the matrix pattern and runs "
           "the analysis of the factorization (see SolverMemory.exact for which "
           "figures are known and which predicted), and a following initialize() "
           "with the same settings and model structure reuses that work. On an "
           "initialized solver it gives the current figures. Raises ValueError "
           "for contradictory settings and RuntimeError when the analysis "
           "fails.")
      .def("auto_configure",
           nb::overload_cast<std::optional<std::int64_t>>(&Solver::auto_configure),
           "max_memory"_a = nb::none(), nb::call_guard<LogDrainGuard>(),
           "Choose the storage, backend, factorization, reuse_factors, threads, "
           "pardiso.ordering, pardiso.level and eigen.ordering for the current "
           "model, clear pardiso.iparm, and return the new settings.\n\n"
           "The other settings are kept. The candidates, in order of preference "
           "(with MKL / without MKL):\n\n"
           "- SteadyState: 1. DENSE LAPACK with reuse_factors (rule S) / DENSE "
           "EIGEN; 2. PARDISO TWO_LEVEL NESTED_DISSECTION / EIGEN COLAMD; 3. "
           "PARDISO ONE_LEVEL MIN_DEGREE / EIGEN AMD.\n"
           "- CrankNicolson: 1. DENSE LAPACK with reuse_factors (rule T) / DENSE "
           "EIGEN; 2. PARDISO ONE_LEVEL MIN_DEGREE with reuse_factors / EIGEN "
           "COLAMD; 3. PARDISO TWO_LEVEL NESTED_DISSECTION / EIGEN AMD.\n"
           "- JacobianPropagation: 1. DENSE LAPACK (rule T) / DENSE EIGEN; 2. "
           "PARDISO TWO_LEVEL NESTED_DISSECTION / EIGEN COLAMD; 3. PARDISO "
           "ONE_LEVEL MIN_DEGREE / EIGEN AMD.\n\n"
           "Candidates 2 and 3 are SPARSE. Candidate 1 exists only when its rule "
           "holds, with n the diffusive nodes and a coupling a conductive or "
           "radiative link between two of them: rule S, n <= 5000 and at least "
           "100 couplings per node on average; rule T, n <= 2000 and at least "
           "20 % of the n x n matrix filled. Every candidate sets factorization = "
           "AUTO and, with MKL, threads = 0. reuse_factors needs MKL and never "
           "applies to JacobianPropagation.\n\n"
           "Without max_memory the first candidate is taken. With max_memory "
           "[bytes, or a size such as \"16 GB\"], a candidate whose "
           "estimate_memory() exceeds it is skipped, and when none fits the "
           "smallest is taken with a WARN. Raises RuntimeError on an initialized "
           "solver.")
      .def("auto_configure",
           nb::overload_cast<std::string_view>(&Solver::auto_configure), "max_memory"_a,
           nb::call_guard<LogDrainGuard>(),
           "auto_configure() with max_memory as a size such as \"16 GB\" (see "
           "parse_memory_size()).");
}

} // namespace pycanha::bindings::solvers
