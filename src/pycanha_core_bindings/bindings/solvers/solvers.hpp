#pragma once

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>

#include <nanobind/stl/function.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "bindings/utils/logger.hpp"
#include "pycanha-core/solvers/callback_registry.hpp"
#include "pycanha-core/solvers/linear_solver.hpp"
#include "pycanha-core/solvers/ss.hpp"
#include "pycanha-core/solvers/sslu.hpp"
#include "pycanha-core/solvers/sslu_cgs.hpp"
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

// CallbackRegistry stores Python callables inside C++ std::function members,
// holding strong references that Python's cyclic garbage collector cannot
// see. A callback that refers back to the registry (or its model) creates an
// uncollectable cycle that would leak until interpreter shutdown. These GC
// hooks expose (tp_traverse) and release (tp_clear) those hidden references
// so such cycles are collected normally.
inline int callback_registry_tp_traverse(PyObject *self, visitproc visit,
                                         void *arg) {
  Py_VISIT(Py_TYPE(self));
  if (!nb::inst_ready(self)) {
    return 0;
  }
  auto *registry = nb::inst_ptr<pycanha::CallbackRegistry>(self);
  using FunctionCaster = nb::detail::type_caster<
      std::function<void(pycanha::CallbackContext &)>>;
  for (const std::function<void(pycanha::CallbackContext &)> *slot :
       {&registry->solver_loop, &registry->time_change,
        &registry->after_timestep}) {
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

// The engine and factorisation enums, with the C++ documentation of every
// option, plus MKL_ENABLED and default_solver_engine(). Registered before the
// solver classes, whose engine/solver_type members default to them.
inline void register_solver_options(nb::module_ &m) {
  using pycanha::DirectSolverType;
  using pycanha::IterativeSolverType;
  using pycanha::SolverEngine;

  nb::enum_<SolverEngine>(m, "SolverEngine",
                          "Library that factorises the linearised system of a solver.")
      .value("MKL", SolverEngine::MKL,
             "Intel MKL PARDISO: multi-threaded supernodal factorisation. Only in "
             "builds with MKL (MKL_ENABLED).")
      .value("EIGEN", SolverEngine::EIGEN,
             "Eigen's sparse solvers: single-threaded, available in every build.");

  nb::enum_<DirectSolverType>(
      m, "DirectSolverType",
      "Factorisation of a direct solver (SSLU, TSCNRLDS).\n\n"
      "Every type computes a complete factorisation: the linear system is solved "
      "exactly (to rounding) at every pass, so all types give the same "
      "temperatures and differ only in time and memory. With MKL, models without "
      "radiative couplings are factorised with Cholesky when the solver allows "
      "it (allow_cholesky), the others with LU. A type of the other engine makes "
      "initialize() raise ValueError.")
      .value("DEFAULT", DirectSolverType::DEFAULT, "TWO_LEVEL with MKL, COLAMD with Eigen.")
      .value("TWO_LEVEL", DirectSolverType::TWO_LEVEL,
             "MKL: parallel nested dissection ordering (METIS) and PARDISO's "
             "two-level factorisation. The default with MKL, and the safe choice.")
      .value("ONE_LEVEL", DirectSolverType::ONE_LEVEL,
             "MKL: same ordering, PARDISO's classic one-level factorisation. It "
             "seems slightly faster on most models, but with MKL 2025.3 it can "
             "livelock (never finish) with 4 or more threads on some radiative "
             "models.")
      .value("MIN_DEGREE", DirectSolverType::MIN_DEGREE,
             "MKL: minimum degree ordering, one-level factorisation. It seems "
             "faster on nearly dense models (many radiative couplings per node), "
             "and its analysis is slow on models where a few nodes are coupled to "
             "many.")
      .value("COLAMD", DirectSolverType::COLAMD,
             "Eigen: SparseLU with COLAMD ordering. The default with Eigen.")
      .value("AMD", DirectSolverType::AMD,
             "Eigen: SparseLU with approximate minimum degree ordering. It seems "
             "faster than COLAMD only on models where a few nodes are coupled to "
             "many.")
      .value("LDLT", DirectSolverType::LDLT,
             "Eigen: SimplicialLDLT with AMD ordering, only for models without "
             "radiative couplings and only in SSLU. It seems faster than COLAMD on "
             "small and medium models, and uses less memory.");

  nb::enum_<IterativeSolverType>(
      m, "IterativeSolverType",
      "Factorisation of an iterative solver (SSLU_CGS, MKL PARDISO only).\n\n"
      "After the first factorisation, a changed matrix is first solved with CGS "
      "(or CG for Cholesky) preconditioned by the previous factors, and only "
      "refactorised when that iteration fails. The iteration stops at a "
      "relative residual of 10^-L, with L the tens digit of pardiso_iparm_3 "
      "(61: 1e-6), and that residual remains in the converged temperatures. "
      "PARDISO only iterates with its one-level factorisation, hence the two "
      "types.")
      .value("MIN_DEGREE", IterativeSolverType::MIN_DEGREE,
             "Minimum degree ordering. The default. Its analysis is slow on models "
             "where a few nodes are coupled to many.")
      .value("ONE_LEVEL", IterativeSolverType::ONE_LEVEL,
             "Parallel nested dissection ordering (METIS). It seems faster, but "
             "with MKL 2025.3 it can livelock with 4 or more threads on some "
             "radiative models; limit mkl_threads to 2 if it does.");

  m.def("default_solver_engine", &pycanha::default_solver_engine,
        "MKL when the library is built with it, EIGEN otherwise.");
  m.def("resolve_solver_type", &pycanha::resolve_solver_type, "engine"_a, "solver_type"_a,
        "The factorisation a solver_type means for an engine: DEFAULT replaced "
        "by the engine's default (TWO_LEVEL for MKL, COLAMD for Eigen), any "
        "other type returned unchanged.");
  // Read from the compiled core rather than from its config header, so it
  // reports the library this module is actually linked against.
  m.attr("MKL_ENABLED") = pycanha::default_solver_engine() == SolverEngine::MKL;
}

inline void register_solvers(nb::module_ &m) {
     using pycanha::CallbackContext;
     using pycanha::CallbackRegistry;
     using pycanha::DataModel;
  using pycanha::Solver;
     using pycanha::SolverOutputConfig;
     using pycanha::SolverRegistry;
  using pycanha::SSLU;
  using pycanha::SSLU_CGS;
  using pycanha::SteadyStateSolver;
  using pycanha::ThermalMathematicalModel;
     using pycanha::ThermalModel;
  using pycanha::TransientSolver;
  using pycanha::TSCN;
  using pycanha::TSCNRL;
  using pycanha::TSCNRLDS;
  using pycanha::TSCNRLDS_JACOBIAN;

  register_solver_options(m);

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
              "MKL PARDISO iterative step, read by initialize(). 0 (default) "
              "factorises every changed matrix; 10 * L + 1 first iterates on the "
              "previous factors down to a relative residual of 10^-L, and that "
              "residual stays in the result. Only with SSLU_CGS (default 61) or, "
              "in TSCNRLDS, with the ONE_LEVEL or MIN_DEGREE types.")
      .def_rw("pardiso_iparm_overrides", &Solver::pardiso_iparm_overrides,
              "PARDISO iparm entries {zero-based index: value} applied after the "
              "solver's own settings, read by initialize(). Index 3 replaces "
              "pardiso_iparm_3. For diagnosis and workarounds; the solver types "
              "cover the tested configurations. Reading returns a copy: assign a "
              "whole dict, item assignment on the returned dict has no effect.")
      .def_rw("mkl_threads", &Solver::mkl_threads,
              "Threads of the PARDISO calls, 0 for MKL's setting (MKL_NUM_THREADS).")
      .def_rw("pardiso_verbose", &Solver::pardiso_verbose,
              "Print PARDISO statistics to standard output.")
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

  nb::class_<SteadyStateSolver, Solver>(
      m, "SteadyStateSolver",
      "Base class of the steady-state solvers (SSLU, SSLU_CGS).\n\n"
      "Radiation is linearised around the current temperatures (Newton) and the "
      "passes repeat until the largest temperature change is below abstol_temp. "
      "Each pass refactorises only when the matrix values differ from those of "
      "the last factorisation, so the second pass of a linear model costs one "
      "solve with the existing factors.")
      .def_rw("allow_cholesky", &SteadyStateSolver::allow_cholesky,
              "With the MKL engine, factorise models without radiative couplings "
              "with Cholesky instead of LU (faster, less memory). With Eigen, "
              "Cholesky is chosen with DirectSolverType.LDLT instead.")
      .def_prop_ro("uses_cholesky", &SteadyStateSolver::uses_cholesky,
                   "Whether the last initialize() chose a Cholesky-type factorisation.")
      .def_prop_ro("num_factorizations", &SteadyStateSolver::num_factorizations,
                   "Numerical factorisations done by the last solve().");

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

  nb::class_<SSLU, SteadyStateSolver>(
      m, "SSLU",
      "Steady-state solver with a direct factorisation at every Newton pass.\n\n"
      "Radiation is linearised around the current temperatures, the linear "
      "system is factorised completely and solved, and the passes repeat until "
      "the largest temperature change is below abstol_temp. A model without "
      "radiative couplings is linear: the first pass gives the answer and the "
      "second only confirms it, reusing the factors.\n\n"
      "- engine: SolverEngine.MKL (PARDISO, multi-threaded) when the library is "
      "built with MKL, SolverEngine.EIGEN otherwise.\n"
      "- solver_type: the factorisation (see DirectSolverType). Every type is a "
      "complete factorisation, so all of them give the same temperatures.\n"
      "- allow_cholesky (MKL): Cholesky for models without radiative couplings.\n"
      "- pardiso_iparm_overrides, mkl_threads, pardiso_verbose: PARDISO "
      "settings. pardiso_iparm_3 must stay 0: the iterative variant is "
      "SSLU_CGS.\n\n"
      "initialize() raises ValueError for a combination that is not available. "
      "A singular matrix (a group of diffusive nodes with no path to a boundary "
      "node) is reported as an error and the temperatures are left unchanged.")
      .def(nb::init<std::shared_ptr<ThermalMathematicalModel>>(), "tmm"_a,
           nb::keep_alive<1, 2>(),
           "Create a solver bound to a ThermalMathematicalModel.")
      .def_rw("engine", &SSLU::engine,
              "Library that factorises the system (see SolverEngine).")
      .def_rw("solver_type", &SSLU::solver_type,
              "Factorisation (see DirectSolverType).")
      .def("initialize", &SSLU::initialize,
           nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
           "Build the matrix pattern and analyse it. Raises ValueError for an "
           "engine/solver_type combination that is not available.")
      .def("solve", &SSLU::solve, nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
           "Run the steady-state solve to convergence.")
      .def("deinitialize", &SSLU::deinitialize,
           "Release solver resources.");

  nb::class_<SSLU_CGS, SteadyStateSolver>(
      m, "SSLU_CGS",
      "Steady-state solver that reuses its factors as a preconditioner (MKL "
      "PARDISO only).\n\n"
      "Same Newton passes as SSLU. The first pass is factorised completely. In "
      "the next passes the changed matrix is first solved with CGS (CG for "
      "Cholesky) preconditioned by the previous factors, and only refactorised "
      "when that iteration fails. It seems faster than SSLU on radiative "
      "models, whose matrix changes a little at every pass.\n\n"
      "The iteration stops at a relative residual of 10^-L, with L the tens "
      "digit of pardiso_iparm_3 (default 61: L = 6), and that residual stays in "
      "the converged temperatures. Raise L, or use SSLU, for exact answers.\n\n"
      "- solver_type: see IterativeSolverType (MIN_DEGREE by default).\n"
      "- allow_cholesky: Cholesky and CG for models without radiative couplings.\n"
      "- pardiso_iparm_overrides, mkl_threads, pardiso_verbose: PARDISO "
      "settings.\n\n"
      "In a build without MKL, initialize() raises ValueError.")
      .def(nb::init<std::shared_ptr<ThermalMathematicalModel>>(), "tmm"_a,
           nb::keep_alive<1, 2>(),
           "Create a solver bound to a ThermalMathematicalModel.")
      .def_rw("solver_type", &SSLU_CGS::solver_type,
              "Factorisation (see IterativeSolverType).")
      .def("initialize", &SSLU_CGS::initialize,
           nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
           "Build the matrix pattern and analyse it. Raises ValueError without "
           "MKL or for an invalid pardiso_iparm_3.")
      .def("solve", &SSLU_CGS::solve,
           nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
           "Run the steady-state solve to convergence.")
      .def("deinitialize", &SSLU_CGS::deinitialize, "Release solver resources.");

  nb::class_<TSCNRLDS, TSCNRL>(
      m, "TSCNRLDS",
      "Transient solver: Crank-Nicolson with the radiation linearised at every "
      "inner iteration, solved with a sparse direct factorisation.\n\n"
      "- engine and solver_type: as for SSLU (see SolverEngine and "
      "DirectSolverType). DEFAULT is the MKL two-level factorisation, or "
      "Eigen's COLAMD LU without MKL. LDLT is not available: the transient "
      "matrix is factorised with LU.\n"
      "- pardiso_iparm_3 (MKL): 0 (default) factorises every changed matrix. "
      "10 * L + 1 first iterates on the previous factors down to a relative "
      "residual of 10^-L, and that residual stays in every step. It seems much "
      "faster per step, since the matrix changes little between steps, and it "
      "needs ONE_LEVEL or MIN_DEGREE.\n"
      "- pardiso_iparm_overrides, mkl_threads, pardiso_verbose: PARDISO "
      "settings.\n\n"
      "initialize() raises ValueError for a combination that is not available.")
      .def(nb::init<std::shared_ptr<ThermalMathematicalModel>>(), "tmm"_a,
           nb::keep_alive<1, 2>(),
           "Create a solver bound to a ThermalMathematicalModel.")
      .def_rw("engine", &TSCNRLDS::engine,
              "Library that factorises the system (see SolverEngine).")
      .def_rw("solver_type", &TSCNRLDS::solver_type,
              "Factorisation (see DirectSolverType; LDLT is not available).")
      .def("initialize", &TSCNRLDS::initialize,
           nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
           "Allocate solver resources and analyse the matrix pattern. Raises "
           "ValueError for a combination that is not available.")
      .def("solve", &TSCNRLDS::solve, nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
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
      .def("solve", &TSCNRLDS_JACOBIAN::solve, nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
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
                         "sslu_cgs",
                         [](SolverRegistry &self) -> SSLU_CGS & { return self.sslu_cgs(); },
                         nb::rv_policy::reference_internal,
                         "Persistent steady-state solver that iterates on its "
                         "previous factors (MKL only).")
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
