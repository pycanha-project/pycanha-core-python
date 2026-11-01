"""The SteadyState solver."""

import logging

import numpy as np
import pycanha_core as pcc
import pytest

from .models import (
    BASIC_DIFFUSIVE,
    CHAIN_EXPECTED,
    CHAIN_NODES,
    CONDUCTION_FACTORIZATIONS,
    CONFIGURATIONS,
    RADIATIVE_FACTORIZATIONS,
    REUSE_CONFIGURATIONS,
    make_chain_tmm,
    requires_accelerate,
    temperatures,
)

solvers = pcc.solvers
SteadyState = solvers.steady.SteadyState

# Steady state of the basic network (conftest.make_basic_tmm) with every node
# including the boundary one, the reference values of the 0.23 solver.
BASIC_EXPECTED = [132.38706, 306.56526, 111.78443, 200.32387, 3.14999]


def solve(model, **settings):
    solver = SteadyState(model)
    solver.set_settings(solvers.Settings(tolerance=1e-8, max_iterations=100, **settings))
    solver.initialize()
    solver.solve()
    return solver


class TestLifecycle:
    def test_construction(self, basic_tmm):
        solver = SteadyState(basic_tmm)
        assert solver.name == "SteadyState"
        assert solver.solver_type == solvers.SolverType.STEADY_STATE
        assert solver.initialized is False
        assert solver.converged is False
        assert isinstance(solver, solvers.Solver)

    def test_registry_returns_one_solver(self, basic_tm):
        solver = basic_tm.solvers.steady.steady_state
        assert isinstance(solver, SteadyState)
        assert solver is basic_tm.solvers.steady.steady_state

    def test_full_lifecycle(self, basic_tmm):
        solver = solve(basic_tmm)
        assert solver.initialized is True
        assert solver.converged is True
        solver.deinitialize()
        assert solver.initialized is False

    def test_solve_before_initialize_logs_an_error_and_does_nothing(self, basic_tmm, caplog):
        before = temperatures(basic_tmm, BASIC_DIFFUSIVE)
        solver = SteadyState(basic_tmm)
        with caplog.at_level(logging.ERROR, logger="pycanha"):
            solver.solve()
        assert any("initialize" in r.getMessage() for r in caplog.records)
        assert solver.converged is False
        np.testing.assert_array_equal(temperatures(basic_tmm, BASIC_DIFFUSIVE), before)

    def test_matches_the_reference(self, basic_tmm):
        solve(basic_tmm)
        np.testing.assert_allclose(
            temperatures(basic_tmm, [*BASIC_DIFFUSIVE, 99]), BASIC_EXPECTED, atol=1e-4
        )



class TestConfigurations:
    @pytest.mark.parametrize("factorization", CONDUCTION_FACTORIZATIONS)
    @pytest.mark.parametrize("config", CONFIGURATIONS)
    def test_every_configuration_gives_the_exact_conduction_answer(self, config, factorization):
        model = make_chain_tmm()
        solver = solve(model, factorization=factorization, **config)
        assert solver.converged
        np.testing.assert_allclose(temperatures(model, CHAIN_NODES), CHAIN_EXPECTED, rtol=1e-12)

    @pytest.mark.parametrize("factorization", RADIATIVE_FACTORIZATIONS)
    @pytest.mark.parametrize("config", CONFIGURATIONS)
    def test_every_configuration_matches_the_reference(self, config, factorization, basic_tmm):
        solver = solve(basic_tmm, factorization=factorization, **config)
        assert solver.converged
        assert solver.uses_cholesky is False
        np.testing.assert_allclose(
            temperatures(basic_tmm, [*BASIC_DIFFUSIVE, 99]), BASIC_EXPECTED, atol=1e-4
        )

    @pytest.mark.parametrize("config", REUSE_CONFIGURATIONS)
    def test_reusing_factors_agrees(self, config, basic_tmm):
        solver = solve(basic_tmm, reuse_factors=True, **config)
        assert solver.converged
        np.testing.assert_allclose(
            temperatures(basic_tmm, [*BASIC_DIFFUSIVE, 99]), BASIC_EXPECTED, atol=1e-4
        )

    def test_cholesky_is_refused_with_radiation(self, basic_tmm):
        solver = SteadyState(basic_tmm)
        solver.factorization = solvers.Factorization.CHOLESKY
        with pytest.raises(ValueError):
            solver.initialize()

    def test_auto_picks_cholesky_without_radiation(self, basic_tmm):
        assert solve(make_chain_tmm(), backend=solvers.Backend.EIGEN, storage=solvers.Storage.DENSE).uses_cholesky
        lu = solve(make_chain_tmm(), factorization=solvers.Factorization.LU)
        assert lu.uses_cholesky is False

    def test_linear_model_factorises_once(self):
        solver = solve(make_chain_tmm())
        # The confirming second iteration reuses the factors.
        assert solver.num_factorizations == 1
        assert solver.iterations == 2
        assert solver.converged

    @pytest.mark.skipif(pcc.MKL_ENABLED, reason="only a build without MKL refuses MKL")
    @pytest.mark.parametrize("backend", [solvers.Backend.PARDISO, solvers.Backend.LAPACK])
    def test_mkl_backends_are_refused_without_mkl(self, backend, basic_tmm):
        solver = SteadyState(basic_tmm)
        solver.backend = backend
        with pytest.raises(ValueError):
            solver.initialize()


class TestAccelerate:
    @pytest.mark.skipif(
        solvers.accelerate_available(), reason="only refused where Accelerate is not available"
    )
    def test_refused_where_not_available(self, basic_tmm):
        solver = SteadyState(basic_tmm)
        solver.backend = solvers.Backend.ACCELERATE
        with pytest.raises(ValueError):
            solver.initialize()

    @requires_accelerate
    def test_agrees_with_eigen(self, basic_tmm_factory):
        reference = basic_tmm_factory()
        solve(reference, backend=solvers.Backend.EIGEN)
        model = basic_tmm_factory()
        solve(model, backend=solvers.Backend.ACCELERATE)
        np.testing.assert_allclose(
            temperatures(model, BASIC_DIFFUSIVE), temperatures(reference, BASIC_DIFFUSIVE), atol=1e-8
        )

    @requires_accelerate
    def test_cholesky_without_radiation(self, basic_tmm):
        conduction = solve(make_chain_tmm(), backend=solvers.Backend.ACCELERATE)
        assert conduction.uses_cholesky is True
        radiative = solve(basic_tmm, backend=solvers.Backend.ACCELERATE)
        assert radiative.uses_cholesky is False
