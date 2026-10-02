"""Solver engine and factorisation options: SSLU, SSLU_CGS and TSCNRLDS."""

import numpy as np
import pycanha_core as pcc
import pytest

solvers = pcc.solvers
tmm = pcc.tmm

requires_mkl = pytest.mark.skipif(not solvers.MKL_ENABLED, reason="needs a build with MKL")

ENGINE_TYPES = [
    (solvers.SolverEngine.EIGEN, solvers.DirectSolverType.COLAMD),
    (solvers.SolverEngine.EIGEN, solvers.DirectSolverType.AMD),
    pytest.param(solvers.SolverEngine.MKL, solvers.DirectSolverType.TWO_LEVEL, marks=requires_mkl),
    pytest.param(solvers.SolverEngine.MKL, solvers.DirectSolverType.ONE_LEVEL, marks=requires_mkl),
    pytest.param(solvers.SolverEngine.MKL, solvers.DirectSolverType.MIN_DEGREE, marks=requires_mkl),
]

# Chain 1-2-3-4 to the boundary node 100, heat into node 1. Each link carries
# all of it, so node k sits (5 - k) links above the boundary.
CHAIN_G = 0.5
CHAIN_Q = 10.0
CHAIN_T_BOUNDARY = 300.0
CHAIN_EXPECTED = [CHAIN_T_BOUNDARY + (5 - k) * CHAIN_Q / CHAIN_G for k in (1, 2, 3, 4)]


def make_chain_tmm():
    """A conduction-only model with a known steady state, built with the bulk calls."""
    model = tmm.ThermalMathematicalModel("chain")
    model.add_nodes(
        np.array([1, 2, 3, 4], dtype=np.int32),
        T=np.full(4, 273.15),
        C=np.full(4, 10.0),
        qi=np.array([CHAIN_Q, 0.0, 0.0, 0.0]),
    )
    model.add_nodes(
        np.array([100], dtype=np.int32), type=tmm.NodeType.BOUNDARY, T=np.array([CHAIN_T_BOUNDARY])
    )
    model.add_conductive_couplings(
        np.array([1, 2, 3, 4], dtype=np.int32),
        np.array([2, 3, 4, 100], dtype=np.int32),
        np.full(4, CHAIN_G),
    )
    return model


def temperatures(model, numbers):
    return np.array([model.nodes.get_T(number) for number in numbers])


def solve_steady(solver):
    solver.max_iters = 100
    solver.abstol_temp = 1e-8
    solver.initialize()
    solver.solve()
    solver.deinitialize()


class TestModuleOptions:
    def test_enums_are_bound(self):
        assert {member.name for member in solvers.DirectSolverType} == {
            "DEFAULT",
            "TWO_LEVEL",
            "ONE_LEVEL",
            "MIN_DEGREE",
            "COLAMD",
            "AMD",
            "LDLT",
        }
        assert {member.name for member in solvers.IterativeSolverType} == {
            "MIN_DEGREE",
            "ONE_LEVEL",
        }
        assert {member.name for member in solvers.SolverEngine} == {"MKL", "EIGEN"}

    def test_default_engine_follows_the_build(self):
        assert isinstance(solvers.MKL_ENABLED, bool)
        expected = solvers.SolverEngine.MKL if solvers.MKL_ENABLED else solvers.SolverEngine.EIGEN
        assert solvers.default_solver_engine() == expected

    def test_resolve_solver_type(self):
        resolve = solvers.resolve_solver_type
        assert resolve(solvers.SolverEngine.MKL, solvers.DirectSolverType.DEFAULT) == (
            solvers.DirectSolverType.TWO_LEVEL
        )
        assert resolve(solvers.SolverEngine.EIGEN, solvers.DirectSolverType.DEFAULT) == (
            solvers.DirectSolverType.COLAMD
        )
        assert resolve(solvers.SolverEngine.EIGEN, solvers.DirectSolverType.LDLT) == (
            solvers.DirectSolverType.LDLT
        )

    def test_solver_defaults(self, basic_tmm):
        sslu = solvers.SSLU(basic_tmm)
        assert sslu.engine == solvers.default_solver_engine()
        assert sslu.solver_type == solvers.DirectSolverType.DEFAULT
        assert sslu.pardiso_iparm_3 == 0
        assert sslu.allow_cholesky is True
        assert sslu.mkl_threads == 0
        assert sslu.pardiso_verbose is False
        assert sslu.pardiso_iparm_overrides == {}

        transient = solvers.TSCNRLDS(basic_tmm)
        assert transient.engine == solvers.default_solver_engine()
        assert transient.solver_type == solvers.DirectSolverType.DEFAULT
        assert transient.pardiso_iparm_3 == 0

        cgs = solvers.SSLU_CGS(basic_tmm)
        assert cgs.solver_type == solvers.IterativeSolverType.MIN_DEGREE
        assert cgs.pardiso_iparm_3 == 61

    def test_pardiso_settings_round_trip(self, basic_tmm):
        solver = solvers.SSLU(basic_tmm)
        solver.pardiso_iparm_overrides = {1: 0, 23: 1}
        solver.mkl_threads = 2
        solver.pardiso_verbose = True
        assert solver.pardiso_iparm_overrides == {1: 0, 23: 1}
        assert solver.mkl_threads == 2
        assert solver.pardiso_verbose is True


class TestSSLUOptions:
    @pytest.mark.parametrize(("engine", "solver_type"), ENGINE_TYPES)
    def test_every_type_gives_the_exact_conduction_answer(self, engine, solver_type):
        model = make_chain_tmm()
        solver = solvers.SSLU(model)
        solver.engine = engine
        solver.solver_type = solver_type
        solve_steady(solver)
        np.testing.assert_allclose(temperatures(model, [1, 2, 3, 4]), CHAIN_EXPECTED, rtol=1e-12)

    @pytest.mark.parametrize(("engine", "solver_type"), ENGINE_TYPES)
    def test_every_type_agrees_on_a_radiative_model(self, engine, solver_type, basic_tmm_factory):
        reference_model = basic_tmm_factory()
        solve_steady(solvers.SSLU(reference_model))
        expected = temperatures(reference_model, [10, 15, 20, 25])

        model = basic_tmm_factory()
        solver = solvers.SSLU(model)
        solver.engine = engine
        solver.solver_type = solver_type
        solve_steady(solver)
        np.testing.assert_allclose(temperatures(model, [10, 15, 20, 25]), expected, atol=1e-6)

    def test_eigen_ldlt_on_conduction(self):
        model = make_chain_tmm()
        solver = solvers.SSLU(model)
        solver.engine = solvers.SolverEngine.EIGEN
        solver.solver_type = solvers.DirectSolverType.LDLT
        solve_steady(solver)
        np.testing.assert_allclose(temperatures(model, [1, 2, 3, 4]), CHAIN_EXPECTED, rtol=1e-12)

    def test_ldlt_refuses_radiation(self, basic_tmm):
        solver = solvers.SSLU(basic_tmm)
        solver.engine = solvers.SolverEngine.EIGEN
        solver.solver_type = solvers.DirectSolverType.LDLT
        with pytest.raises(ValueError, match="LDLT"):
            solver.initialize()

    def test_type_of_the_other_engine_is_refused(self, basic_tmm):
        solver = solvers.SSLU(basic_tmm)
        solver.engine = solvers.SolverEngine.EIGEN
        solver.solver_type = solvers.DirectSolverType.TWO_LEVEL
        with pytest.raises(ValueError):
            solver.initialize()

    @pytest.mark.skipif(solvers.MKL_ENABLED, reason="only a build without MKL refuses MKL")
    def test_mkl_refused_without_mkl(self, basic_tmm):
        solver = solvers.SSLU(basic_tmm)
        solver.engine = solvers.SolverEngine.MKL
        with pytest.raises(ValueError):
            solver.initialize()

    @requires_mkl
    def test_cholesky_only_without_radiation(self, basic_tmm):
        conduction = solvers.SSLU(make_chain_tmm())
        conduction.initialize()
        assert conduction.uses_cholesky is True
        conduction.deinitialize()

        radiative = solvers.SSLU(basic_tmm)
        radiative.initialize()
        assert radiative.uses_cholesky is False
        radiative.deinitialize()

        disallowed = solvers.SSLU(make_chain_tmm())
        disallowed.allow_cholesky = False
        disallowed.initialize()
        assert disallowed.uses_cholesky is False
        disallowed.deinitialize()

    def test_linear_model_factorises_once(self):
        solver = solvers.SSLU(make_chain_tmm())
        solver.max_iters = 100
        solver.abstol_temp = 1e-8
        solver.initialize()
        solver.solve()
        # The confirming second pass reuses the factors.
        assert solver.num_factorizations == 1
        assert solver.solver_converged
        solver.deinitialize()


class TestSSLU_CGS:
    def test_reachable_from_the_registry(self, basic_tm):
        assert isinstance(basic_tm.solvers.sslu_cgs, solvers.SSLU_CGS)
        assert basic_tm.solvers.sslu_cgs is basic_tm.solvers.sslu_cgs

    @requires_mkl
    @pytest.mark.parametrize(
        "solver_type", [solvers.IterativeSolverType.MIN_DEGREE, solvers.IterativeSolverType.ONE_LEVEL]
    )
    def test_agrees_with_sslu(self, solver_type, basic_tmm_factory):
        reference_model = basic_tmm_factory()
        solve_steady(solvers.SSLU(reference_model))
        expected = temperatures(reference_model, [10, 15, 20, 25])

        model = basic_tmm_factory()
        solver = solvers.SSLU_CGS(model)
        solver.solver_type = solver_type
        solver.mkl_threads = 2
        solve_steady(solver)
        np.testing.assert_allclose(temperatures(model, [10, 15, 20, 25]), expected, atol=1e-4)

    @requires_mkl
    def test_needs_an_iterative_step(self, basic_tmm):
        solver = solvers.SSLU_CGS(basic_tmm)
        solver.pardiso_iparm_3 = 0
        with pytest.raises(ValueError, match="iterative step"):
            solver.initialize()

    @pytest.mark.skipif(solvers.MKL_ENABLED, reason="only a build without MKL refuses SSLU_CGS")
    def test_refused_without_mkl(self, basic_tmm):
        with pytest.raises(ValueError):
            solvers.SSLU_CGS(basic_tmm).initialize()


class TestTSCNRLDSOptions:
    @pytest.mark.parametrize(("engine", "solver_type"), ENGINE_TYPES)
    def test_every_type_agrees(self, engine, solver_type, basic_tmm_factory):
        def run(configure):
            model = basic_tmm_factory()
            solver = solvers.TSCNRLDS(model)
            configure(solver)
            solver.set_simulation_time(0.0, 100.0, 10.0, 10.0)
            solver.initialize()
            solver.solve()
            solver.deinitialize()
            return temperatures(model, [10, 15, 20, 25])

        expected = run(lambda solver: None)

        def configure(solver):
            solver.engine = engine
            solver.solver_type = solver_type

        np.testing.assert_allclose(run(configure), expected, atol=1e-8)

    def test_ldlt_is_refused(self, basic_tmm):
        solver = solvers.TSCNRLDS(basic_tmm)
        solver.engine = solvers.SolverEngine.EIGEN
        solver.solver_type = solvers.DirectSolverType.LDLT
        solver.set_simulation_time(0.0, 10.0, 1.0, 1.0)
        with pytest.raises(ValueError):
            solver.initialize()
