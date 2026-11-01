#pragma once

#include <cstdint>
#include <deque>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nanobind/nanobind.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/set.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

#include "bindings/utils/value_eq.hpp"
#include "pycanha-core/solvers/settings.hpp"
#include "pycanha-core/thermaldata/data_model.hpp"
#include "pycanha-core/tmm/thermalmathematicalmodel.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::solvers {

// nanobind keeps the docstring pointers it is given, so a docstring assembled
// at registration needs storage that outlives the module. A deque never moves
// its elements, so the pointers stay valid as more are added.
inline const char *stored_doc(std::string text) {
  static std::deque<std::string> docs;
  return docs.emplace_back(std::move(text)).c_str();
}

// The documentation of a setting, from the core's field table.
inline const char *setting_doc(std::string_view name) {
  const pycanha::solvers::SettingField &field =
      pycanha::solvers::setting_field(name);
  std::string doc(field.doc);
  if (field.mutability == pycanha::solvers::Mutability::INIT) {
    doc += "\n\nRead by initialize(): frozen while the solver is initialised.";
  }
  return stored_doc(std::move(doc));
}

// __str__ of a solver enum: its C++ name.
template <typename Enum> void add_enum_str(nb::enum_<Enum> &cls) {
  cls.def("__str__", [](Enum value) {
    return std::string(pycanha::solvers::to_string(value));
  });
}

// Settings::set and Settings::from_dict take enums and output attributes by
// name. A Python caller may pass the enum members themselves (one, or a
// collection of output attributes); they are converted to their names here,
// and everything else goes to the SettingValue caster unchanged.
inline pycanha::solvers::SettingValue to_setting_value(nb::handle value) {
  const nb::object enum_type = nb::module_::import_("enum").attr("Enum");
  if (nb::isinstance(value, enum_type)) {
    return nb::cast<std::string>(value.attr("name"));
  }
  if (nb::isinstance<nb::set>(value) || nb::isinstance<nb::frozenset>(value) ||
      nb::isinstance<nb::list>(value) || nb::isinstance<nb::tuple>(value)) {
    std::vector<std::string> names;
    for (nb::handle item : value) {
      if (nb::isinstance(item, enum_type)) {
        names.push_back(nb::cast<std::string>(item.attr("name")));
      } else {
        names.push_back(nb::cast<std::string>(item));
      }
    }
    return names;
  }
  return nb::cast<pycanha::solvers::SettingValue>(value);
}

inline void register_setting_enums(nb::module_ &m) {
  using namespace pycanha::solvers;

  nb::enum_<SolverType> solver_type(m, "SolverType",
                                    "The solver type a Settings is meant for.");
  solver_type.value("STEADY_STATE", SolverType::STEADY_STATE, "Steady state of the network.")
      .value("CRANK_NICOLSON", SolverType::CRANK_NICOLSON, "Crank-Nicolson transient.")
      .value("JACOBIAN_PROPAGATION", SolverType::JACOBIAN_PROPAGATION,
             "Crank-Nicolson transient with the derivatives of the temperatures "
             "with respect to parameters.");
  add_enum_str(solver_type);

  nb::enum_<Storage> storage(
      m, "Storage", "How the matrix of an implicit solver is stored and factorised.");
  storage.value("SPARSE", Storage::SPARSE,
                "Only the non-zero entries: the choice for large models.")
      .value("DENSE", Storage::DENSE,
             "Every entry of the n x n matrix [n^2 * 8 bytes each]: faster for "
             "small models whose nodes are coupled to most of the others "
             "(radiative enclosures).");
  add_enum_str(storage);

  nb::enum_<Backend> backend(m, "Backend",
                             "Library that factorises the matrix of an implicit solver.");
  backend
      .value("PARDISO", Backend::PARDISO,
             "Intel MKL PARDISO: multi-threaded supernodal factorisation, sparse "
             "storage only. Only in builds with MKL.")
      .value("EIGEN", Backend::EIGEN,
             "Eigen's sparse or dense solvers: in every build.")
      .value("LAPACK", Backend::LAPACK,
             "Intel MKL LAPACK: multi-threaded dense factorisation, dense storage "
             "only. Only in builds with MKL.")
      .value("ACCELERATE", Backend::ACCELERATE,
             "Apple Accelerate: multi-threaded supernodal factorisation, sparse "
             "storage only. Only in macOS builds with Accelerate, on macOS 15.5 "
             "or newer (see accelerate_available()).");
  add_enum_str(backend);

  nb::enum_<Factorization> factorization(m, "Factorization",
                                         "Factorisation of the matrix.");
  factorization
      .value("AUTO", Factorization::AUTO,
             "Cholesky when the matrix is symmetric (no radiative couplings) and "
             "the backend has a Cholesky factorisation it prefers: PARDISO, "
             "ACCELERATE, LAPACK and dense Eigen. LU otherwise.")
      .value("LU", Factorization::LU, "LU, for any matrix.")
      .value("CHOLESKY", Factorization::CHOLESKY,
             "Cholesky: faster and smaller than LU, only for symmetric matrices "
             "(models without radiative couplings).");
  add_enum_str(factorization);

  nb::enum_<PardisoOrdering> pardiso_ordering(m, "PardisoOrdering",
                                              "Fill-reducing ordering of PARDISO.");
  pardiso_ordering
      .value("NESTED_DISSECTION", PardisoOrdering::NESTED_DISSECTION,
             "Parallel nested dissection (METIS).")
      .value("MIN_DEGREE", PardisoOrdering::MIN_DEGREE,
             "Minimum degree. It seems faster on nearly dense models (many "
             "radiative couplings per node); its analysis is slow on models "
             "where a few nodes are coupled to many.");
  add_enum_str(pardiso_ordering);

  nb::enum_<PardisoLevel> pardiso_level(m, "PardisoLevel",
                                        "Numerical factorisation of PARDISO.");
  pardiso_level
      .value("TWO_LEVEL", PardisoLevel::TWO_LEVEL,
             "Two-level factorisation, with nested dissection only. The safe "
             "choice.")
      .value("ONE_LEVEL", PardisoLevel::ONE_LEVEL,
             "Classic one-level factorisation, needed to reuse factors. With "
             "nested dissection it can livelock (never finish) with MKL 2025.3 "
             "at 4 or more threads on some radiative models.");
  add_enum_str(pardiso_level);

  nb::enum_<EigenOrdering> eigen_ordering(m, "EigenOrdering",
                                          "Ordering of Eigen's sparse LU.");
  eigen_ordering.value("COLAMD", EigenOrdering::COLAMD, "COLAMD.")
      .value("AMD", EigenOrdering::AMD,
             "Approximate minimum degree. It seems faster than COLAMD only on "
             "models where a few nodes are coupled to many.");
  add_enum_str(eigen_ordering);

  nb::enum_<AccelerateOrdering> accelerate_ordering(
      m, "AccelerateOrdering", "Fill-reducing ordering of Accelerate.");
  accelerate_ordering.value("METIS", AccelerateOrdering::METIS, "Nested dissection (METIS).")
      .value("AMD", AccelerateOrdering::AMD, "Approximate minimum degree.");
  add_enum_str(accelerate_ordering);

  nb::enum_<Mutability> mutability(
      m, "Mutability", "Whether a setting may change while the solver is initialised.");
  mutability
      .value("LIVE", Mutability::LIVE,
             "Read at every solve(): it may change at any time.")
      .value("INIT", Mutability::INIT,
             "Read by initialize(): it is frozen while the solver is initialised.");
  add_enum_str(mutability);

  nb::enum_<Origin> origin(m, "Origin", "Where an effective setting value comes from.");
  origin.value("DEFAULT", Origin::DEFAULT, "The solver's default.")
      .value("USER", Origin::USER, "Set in the Settings.")
      .value("AUTO_CONFIGURE", Origin::AUTO_CONFIGURE,
             "Set by Solver.auto_configure() and unchanged since.");
  add_enum_str(origin);
}

inline void register_setting_groups(nb::module_ &m) {
  using namespace pycanha::solvers;

  nb::class_<PardisoSettings> pardiso(m, "PardisoSettings",
                              "PARDISO settings, used only with Backend.PARDISO. "
                              "None means unset (the solver's default).");
  pardiso
      .def(
          "__init__",
          [](PardisoSettings *self, std::optional<PardisoOrdering> ordering,
             std::optional<PardisoLevel> level, std::optional<std::map<int, int>> iparm,
             std::optional<bool> verbose) {
            new (self) PardisoSettings{ordering, level, std::move(iparm), verbose};
          },
          nb::kw_only(), "ordering"_a = nb::none(), "level"_a = nb::none(),
          "iparm"_a = nb::none(), "verbose"_a = nb::none(),
          "Create PARDISO settings; every field left out is unset.")
      .def(nb::init<const PardisoSettings &>(), "other"_a, "Copy.")
      .def_rw("ordering", &PardisoSettings::ordering, setting_doc("pardiso.ordering"))
      .def_rw("level", &PardisoSettings::level, setting_doc("pardiso.level"))
      .def_rw("iparm", &PardisoSettings::iparm,
              stored_doc(std::string(setting_doc("pardiso.iparm")) +
                         "\n\nReading returns a copy: assign a whole dict, item "
                         "assignment on the returned dict has no effect."))
      .def_rw("verbose", &PardisoSettings::verbose, setting_doc("pardiso.verbose"))
      .def("__copy__", [](const PardisoSettings &self) { return self; })
      .def("__deepcopy__",
           [](const PardisoSettings &self, nb::handle /*memo*/) { return self; }, "memo"_a);
  utils::def_value_eq<PardisoSettings>(pardiso);

  nb::class_<EigenSettings> eigen(m, "EigenSettings",
                            "Eigen settings, used only with Backend.EIGEN. None "
                            "means unset (the solver's default).");
  eigen
      .def(
          "__init__",
          [](EigenSettings *self, std::optional<EigenOrdering> ordering) {
            new (self) EigenSettings{ordering};
          },
          nb::kw_only(), "ordering"_a = nb::none(),
          "Create Eigen settings; every field left out is unset.")
      .def(nb::init<const EigenSettings &>(), "other"_a, "Copy.")
      .def_rw("ordering", &EigenSettings::ordering, setting_doc("eigen.ordering"))
      .def("__copy__", [](const EigenSettings &self) { return self; })
      .def("__deepcopy__",
           [](const EigenSettings &self, nb::handle /*memo*/) { return self; }, "memo"_a);
  utils::def_value_eq<EigenSettings>(eigen);

  nb::class_<AccelerateSettings> accelerate(m, "AccelerateSettings",
                                 "Accelerate settings, used only with "
                                 "Backend.ACCELERATE. None means unset (the "
                                 "solver's default).");
  accelerate
      .def(
          "__init__",
          [](AccelerateSettings *self, std::optional<AccelerateOrdering> ordering) {
            new (self) AccelerateSettings{ordering};
          },
          nb::kw_only(), "ordering"_a = nb::none(),
          "Create Accelerate settings; every field left out is unset.")
      .def(nb::init<const AccelerateSettings &>(), "other"_a, "Copy.")
      .def_rw("ordering", &AccelerateSettings::ordering,
              setting_doc("accelerate.ordering"))
      .def("__copy__", [](const AccelerateSettings &self) { return self; })
      .def("__deepcopy__",
           [](const AccelerateSettings &self, nb::handle /*memo*/) { return self; },
           "memo"_a);
  utils::def_value_eq<AccelerateSettings>(accelerate);
}

inline void register_settings_class(nb::module_ &m) {
  using namespace pycanha::solvers;
  using pycanha::DataModelAttribute;

  nb::class_<Settings> cls(
      m, "Settings",
      "The settings of every solver type.\n\n"
      "Each field is set (holds a value) or unset (None: the solver's default, "
      "which may depend on the solver type, the build, the other fields and "
      "the model). One Settings can be shared by solvers of different types: "
      "a set field that does not apply to a solver is ignored there with a "
      "warning at initialize() (see resolve()). setting_fields() lists every "
      "field with its documentation.\n\n"
      "The groups pardiso, eigen and accelerate are edited in place "
      "(s.pardiso.level = ...). Values are checked when the settings are "
      "resolved (Solver.initialize(), resolve()), and by set() and "
      "from_dict().");
  cls.def(
          "__init__",
          [](Settings *self, std::optional<SolverType> solver_type,
             std::optional<double> tolerance, std::optional<int> max_iterations,
             std::optional<Storage> storage, std::optional<Backend> backend,
             std::optional<Factorization> factorization,
             std::optional<bool> reuse_factors, std::optional<double> reuse_tolerance,
             std::optional<bool> fixed_couplings, std::optional<int> threads,
             std::optional<double> start_time, std::optional<double> end_time,
             std::optional<double> time_step, std::optional<double> output_interval,
             std::optional<std::set<DataModelAttribute>> output_attributes,
             std::optional<std::string> output_name,
             std::optional<double> capacity_delta,
             std::optional<PardisoSettings> pardiso,
             std::optional<EigenSettings> eigen,
             std::optional<AccelerateSettings> accelerate) {
            auto *settings = new (self) Settings{};
            settings->solver_type = solver_type;
            settings->tolerance = tolerance;
            settings->max_iterations = max_iterations;
            settings->storage = storage;
            settings->backend = backend;
            settings->factorization = factorization;
            settings->reuse_factors = reuse_factors;
            settings->reuse_tolerance = reuse_tolerance;
            settings->fixed_couplings = fixed_couplings;
            settings->threads = threads;
            settings->start_time = start_time;
            settings->end_time = end_time;
            settings->time_step = time_step;
            settings->output_interval = output_interval;
            settings->output_attributes = std::move(output_attributes);
            settings->output_name = std::move(output_name);
            settings->capacity_delta = capacity_delta;
            if (pardiso.has_value()) {
              settings->pardiso = std::move(*pardiso);
            }
            if (eigen.has_value()) {
              settings->eigen = *eigen;
            }
            if (accelerate.has_value()) {
              settings->accelerate = *accelerate;
            }
          },
          nb::kw_only(), "solver_type"_a = nb::none(), "tolerance"_a = nb::none(),
          "max_iterations"_a = nb::none(), "storage"_a = nb::none(),
          "backend"_a = nb::none(), "factorization"_a = nb::none(),
          "reuse_factors"_a = nb::none(), "reuse_tolerance"_a = nb::none(),
          "fixed_couplings"_a = nb::none(), "threads"_a = nb::none(),
          "start_time"_a = nb::none(), "end_time"_a = nb::none(),
          "time_step"_a = nb::none(), "output_interval"_a = nb::none(),
          "output_attributes"_a = nb::none(), "output_name"_a = nb::none(),
          "capacity_delta"_a = nb::none(), "pardiso"_a = nb::none(),
          "eigen"_a = nb::none(), "accelerate"_a = nb::none(),
          "Create settings; every field left out is unset.")
      .def(nb::init<const Settings &>(), "other"_a, "Copy.")
      .def_rw("solver_type", &Settings::solver_type,
              "The solver type these settings are for; None for a loose set of "
              "values.")
      .def_rw("tolerance", &Settings::tolerance, setting_doc("tolerance"))
      .def_rw("max_iterations", &Settings::max_iterations, setting_doc("max_iterations"))
      .def_rw("storage", &Settings::storage, setting_doc("storage"))
      .def_rw("backend", &Settings::backend, setting_doc("backend"))
      .def_rw("factorization", &Settings::factorization, setting_doc("factorization"))
      .def_rw("reuse_factors", &Settings::reuse_factors, setting_doc("reuse_factors"))
      .def_rw("reuse_tolerance", &Settings::reuse_tolerance, setting_doc("reuse_tolerance"))
      .def_rw("fixed_couplings", &Settings::fixed_couplings, setting_doc("fixed_couplings"))
      .def_rw("threads", &Settings::threads, setting_doc("threads"))
      .def_rw("start_time", &Settings::start_time, setting_doc("start_time"))
      .def_rw("end_time", &Settings::end_time, setting_doc("end_time"))
      .def_rw("time_step", &Settings::time_step, setting_doc("time_step"))
      .def_rw("output_interval", &Settings::output_interval, setting_doc("output_interval"))
      .def_rw("output_attributes", &Settings::output_attributes,
              stored_doc(std::string(setting_doc("output_attributes")) +
                         "\n\nReading returns a copy: assign a whole set, "
                         "add() on the returned set has no effect."))
      .def_rw("output_name", &Settings::output_name, setting_doc("output_name"))
      .def_rw("capacity_delta", &Settings::capacity_delta, setting_doc("capacity_delta"))
      .def_rw("pardiso", &Settings::pardiso,
              "PARDISO settings (see PardisoSettings), edited in place.")
      .def_rw("eigen", &Settings::eigen,
              "Eigen settings (see EigenSettings), edited in place.")
      .def_rw("accelerate", &Settings::accelerate,
              "Accelerate settings (see AccelerateSettings), edited in place.")
      .def("is_set", &Settings::is_set, "name"_a,
           "Whether the setting is set. Raises ValueError for an unknown name.")
      .def("get", &Settings::get, "name"_a,
           "The value of a setting by name (\"tolerance\", \"pardiso.level\", "
           "\"solver_type\"), None when unset. Enums and output attributes come "
           "back by name. Raises ValueError for an unknown name.")
      .def(
          "set",
          [](Settings &self, std::string_view name, nb::handle value) {
            self.set(name, to_setting_value(value));
          },
          "name"_a, "value"_a,
          "Set a setting by name. Enums are given as members or by name, output "
          "attributes as a collection of members or names. Unknown names, values "
          "of the wrong type and values out of range raise ValueError; the "
          "settings are then left unchanged.")
      .def("unset", &Settings::unset, "name"_a,
           "Unset a setting by name, so that it takes its default.")
      .def("to_dict", &Settings::to_dict,
           "The set fields only, by name, with enums and output attributes by "
           "name.")
      .def_static(
          "from_dict",
          [](const nb::dict &dict) {
            SettingsDict values;
            for (auto [key, value] : dict) {
              values.emplace(nb::cast<std::string>(key), to_setting_value(value));
            }
            return Settings::from_dict(values);
          },
          "values"_a,
          "Settings from a dict like the one to_dict() gives (enum members are "
          "accepted too). Raises ValueError for an unknown key or a value of the "
          "wrong type or out of range.")
      .def("__str__", &Settings::to_string,
           "Without a solver type, the set fields. With one, every field that "
           "applies to it with its effective value, then the set fields it "
           "ignores and the contradictions.")
      .def("__copy__", [](const Settings &self) { return self; })
      .def("__deepcopy__", [](const Settings &self, nb::handle /*memo*/) { return self; },
           "memo"_a);
  utils::def_value_eq<Settings>(cls);
}

inline void register_resolution(nb::module_ &m) {
  using namespace pycanha::solvers;
  using pycanha::ThermalMathematicalModel;

  nb::class_<SettingField>(m, "SettingField", "Documentation of one setting.")
      .def_prop_ro("name", [](const SettingField &self) { return std::string(self.name); },
                   "\"tolerance\", \"pardiso.level\".")
      .def_prop_ro("group", [](const SettingField &self) { return std::string(self.group); },
                   "\"\", \"pardiso\", \"eigen\" or \"accelerate\".")
      .def_prop_ro("doc", [](const SettingField &self) { return std::string(self.doc); },
                   "What the setting does.")
      .def_ro("mutability", &SettingField::mutability,
              "Whether it may change while the solver is initialised.")
      .def_ro("solver_types", &SettingField::solver_types,
              "One bit per SolverType (bit SolverType.X.value).")
      .def("applies_to", &SettingField::applies_to, "solver_type"_a,
           "Whether the setting applies to a solver type.");

  m.def(
      "setting_fields",
      []() {
        const auto fields = setting_fields();
        return std::vector<SettingField>(fields.begin(), fields.end());
      },
      "Every setting, in the order Settings.__str__ prints them.");
  m.def("setting_field", &setting_field, "name"_a,
        "One setting. Raises ValueError for an unknown name.");

  nb::class_<ResolveContext>(m, "ResolveContext",
                             "What the defaults and the checks depend on besides "
                             "the settings.")
      .def(nb::init<>(),
           "A steady-state context without a model, for this build.")
      .def_rw("solver_type", &ResolveContext::solver_type, "The solver type.")
      .def_rw("has_radiation", &ResolveContext::has_radiation,
              "Whether the model has radiative couplings; None when no model is "
              "known (factorization AUTO then stays AUTO and the Cholesky check "
              "is skipped).")
      .def_rw("mkl_available", &ResolveContext::mkl_available,
              "Whether this build has MKL (PARDISO and LAPACK).")
      .def_rw("accelerate_available", &ResolveContext::accelerate_available,
              "Whether ACCELERATE can be used (see accelerate_available()).");

  m.def("resolve_context", &resolve_context, "solver_type"_a, "model"_a,
        "The context of a solver of solver_type for model.");

  nb::class_<NotApplied>(m, "NotApplied", "A set field that does not apply, with the reason.")
      .def_ro("name", &NotApplied::name, "The setting.")
      .def_ro("reason", &NotApplied::reason, "Why it does not apply.");

  nb::class_<ResolvedPardiso>(m, "ResolvedPardiso", "Effective PARDISO settings.")
      .def_ro("ordering", &ResolvedPardiso::ordering, "Fill-reducing ordering.")
      .def_ro("level", &ResolvedPardiso::level, "Numerical factorisation.")
      .def_ro("iparm", &ResolvedPardiso::iparm, "iparm overrides {zero-based index: value}.")
      .def_ro("verbose", &ResolvedPardiso::verbose, "Print PARDISO statistics.");

  nb::class_<ResolvedEigen>(m, "ResolvedEigen", "Effective Eigen settings.")
      .def_ro("ordering", &ResolvedEigen::ordering, "Ordering of the sparse LU.");

  nb::class_<ResolvedAccelerate>(m, "ResolvedAccelerate", "Effective Accelerate settings.")
      .def_ro("ordering", &ResolvedAccelerate::ordering, "Fill-reducing ordering.");

  nb::class_<ResolvedSettings> resolved(
      m, "ResolvedSettings", "The effective value of every setting for one solver type.");

  nb::class_<ResolvedSettings::FieldState>(resolved, "FieldState",
                                           "Applicability and origin of one setting.")
      .def_ro("applies", &ResolvedSettings::FieldState::applies,
              "Whether the setting applies.")
      .def_ro("origin", &ResolvedSettings::FieldState::origin,
              "Where its value comes from.");

  resolved.def_ro("solver_type", &ResolvedSettings::solver_type, "The solver type.")
      .def_ro("tolerance", &ResolvedSettings::tolerance, setting_doc("tolerance"))
      .def_ro("max_iterations", &ResolvedSettings::max_iterations,
              setting_doc("max_iterations"))
      .def_ro("storage", &ResolvedSettings::storage, setting_doc("storage"))
      .def_ro("backend", &ResolvedSettings::backend, setting_doc("backend"))
      .def_ro("factorization", &ResolvedSettings::factorization,
              setting_doc("factorization"))
      .def_ro("reuse_factors", &ResolvedSettings::reuse_factors,
              setting_doc("reuse_factors"))
      .def_ro("reuse_tolerance", &ResolvedSettings::reuse_tolerance,
              setting_doc("reuse_tolerance"))
      .def_ro("fixed_couplings", &ResolvedSettings::fixed_couplings,
              setting_doc("fixed_couplings"))
      .def_ro("threads", &ResolvedSettings::threads, setting_doc("threads"))
      .def_ro("start_time", &ResolvedSettings::start_time, setting_doc("start_time"))
      .def_ro("end_time", &ResolvedSettings::end_time, setting_doc("end_time"))
      .def_ro("time_step", &ResolvedSettings::time_step, setting_doc("time_step"))
      .def_ro("output_interval", &ResolvedSettings::output_interval,
              setting_doc("output_interval"))
      .def_ro("output_attributes", &ResolvedSettings::output_attributes,
              setting_doc("output_attributes"))
      .def_ro("output_name", &ResolvedSettings::output_name, setting_doc("output_name"))
      .def_ro("capacity_delta", &ResolvedSettings::capacity_delta,
              setting_doc("capacity_delta"))
      .def_ro("pardiso", &ResolvedSettings::pardiso, "Effective PARDISO settings.")
      .def_ro("eigen", &ResolvedSettings::eigen, "Effective Eigen settings.")
      .def_ro("accelerate", &ResolvedSettings::accelerate, "Effective Accelerate settings.")
      .def_ro("not_applied", &ResolvedSettings::not_applied,
              "Set fields ignored for this solver type or backend.")
      .def_ro("warnings", &ResolvedSettings::warnings,
              "Accepted combinations that deserve a warning.")
      .def_ro("fields", &ResolvedSettings::fields,
              "One entry per setting_fields() entry.")
      .def("applies", &ResolvedSettings::applies, "name"_a,
           "Whether the field applies to the solver type and the selected "
           "backend. Raises ValueError for an unknown name.")
      .def("origin", &ResolvedSettings::origin, "name"_a,
           "Where the value comes from (DEFAULT when the field does not apply). "
           "Raises ValueError for an unknown name.")
      .def("value", &ResolvedSettings::value, "name"_a,
           "The effective value by name, with enums and output attributes by "
           "name. Raises ValueError for an unknown name.");

  m.def("resolve", &resolve, "settings"_a, "context"_a,
        "The effective settings of a solver: each unset field, and each set "
        "field that does not apply, takes its default.\n\n"
        "Raises ValueError listing every contradiction at once: a value out of "
        "range, a combination the backend cannot do (reuse_factors with Eigen "
        "or ACCELERATE, minimum degree with the two-level factorisation, "
        "PARDISO or ACCELERATE with dense storage), or one the build or the "
        "model cannot (PARDISO or LAPACK without MKL, ACCELERATE when "
        "accelerate_available() is false, Cholesky with radiation). The set "
        "fields that do not apply are listed in not_applied.");

  nb::class_<FieldExplanation>(m, "FieldExplanation",
                               "The effective value of one applicable setting and why "
                               "it has it.")
      .def_ro("name", &FieldExplanation::name, "The setting.")
      .def_ro("value", &FieldExplanation::value,
              "The effective value, with enums and output attributes by name.")
      .def_ro("value_text", &FieldExplanation::value_text,
              "The value as Settings.__str__ prints it.")
      .def_ro("origin", &FieldExplanation::origin,
              "DEFAULT or USER; a solver reports AUTO_CONFIGURE for the values its "
              "auto_configure() set.")
      .def_ro("reason", &FieldExplanation::reason,
              "Why a default has its value, or what a set value resolves to; "
              "empty when there is nothing to add.");

  nb::class_<SettingsExplanation>(m, "SettingsExplanation",
                                  "What resolve() gives, with a reason for every "
                                  "applicable field. The contradictions are listed "
                                  "instead of raised.")
      .def_ro("resolved", &SettingsExplanation::resolved,
              "The effective values (each contradicting field keeps its value).")
      .def_ro("fields", &SettingsExplanation::fields,
              "The applicable fields, in setting_fields() order.")
      .def_ro("contradictions", &SettingsExplanation::contradictions,
              "The contradictions resolve() would raise.");

  m.def("explain_settings", &explain_settings, "settings"_a, "context"_a,
        "What resolve() gives, with a reason for every applicable field and the "
        "contradictions listed instead of raised.");

  m.def(
      "changed_init_fields",
      [](const Settings &before, const Settings &after, const ResolveContext &context) {
        const auto names = changed_init_fields(before, after, context);
        return std::vector<std::string>(names.begin(), names.end());
      },
      "before"_a, "after"_a, "context"_a,
      "The INIT fields whose effective value differs between before and after "
      "(fields that do not apply under both are skipped).");
  m.def("check_settings_change", &check_settings_change, "before"_a, "after"_a,
        "context"_a, "initialized"_a,
        "Raise RuntimeError naming the INIT fields that after changes, when "
        "initialized: they are read by initialize() only.");

  m.def("accelerate_available", &accelerate_available,
        "Whether backend = ACCELERATE can be used: the build has Apple "
        "Accelerate and the running macOS has its sparse LU (macOS 15.5 or "
        "newer).");
  m.def("all_dense_output_attributes", &all_dense_output_attributes,
        "Every temperature, capacity, load and node attribute stored per node.");
  m.def("all_output_attributes", &all_output_attributes,
        "The dense attributes plus the coupling matrices and the Jacobian.");
}

// Registered before the solver classes: their constructors and setters take
// these types, and the property docstrings are read from the field table.
inline void register_settings(nb::module_ &m) {
  register_setting_enums(m);
  register_setting_groups(m);
  register_settings_class(m);
  register_resolution(m);
}

} // namespace pycanha::bindings::solvers
