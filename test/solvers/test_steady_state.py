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
    make_jacobian_tmm,
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


# Parameters of the basic network: an internal conductive coupling, an
# internal and a boundary radiative coupling, a heat load, the boundary
# temperature, and one parameter in two entities.
DERIVATIVE_FORMULAS = {
    "g": (0.1, [("GL(10,15)", "g")]),
    "r": (0.2, [("GR(15,25)", "r")]),
    "rb": (1.0, [("GR(10,99)", "rb")]),
    "q": (500.0, [("QI15", "q")]),
    "tb": (150.0, [("T99", "tb")]),
    "s": (1.0, [("GL(20,25)", "0.1 * s * s"), ("QI20", "20 * s")]),
}


def make_parametric_tmm(basic_tmm):
    for name, (value, formulas) in DERIVATIVE_FORMULAS.items():
        basic_tmm.parameters.add_parameter(name, value)
        for entity, expression in formulas:
            basic_tmm.formulas.add_parameter_formula(entity, expression)
        basic_tmm.formulas.parameters_with_derivatives.add_parameter(name)
    basic_tmm.formulas.apply_formulas()
    return basic_tmm


def solve_tightly(model):
    solver = SteadyState(model)
    solver.set_settings(solvers.Settings(tolerance=1e-11, max_iterations=200))
    solver.initialize()
    solver.solve()
    return solver


class TestDerivatives:
    def test_one_link_is_exact(self):
        # T1 = Tb + Q / k = 2: dT1/dk = -Q / k^2 = -1, and C does not enter.
        model = make_jacobian_tmm()
        solver = solve(model)
        jacobian = solver.jacobian()
        assert jacobian.shape == (1, 2)
        assert solver.derivative_parameter_names == ["k", "C"]
        np.testing.assert_allclose(jacobian, [[-1.0, 0.0]], atol=1e-12)

    def test_matches_central_differences(self, basic_tmm):
        model = make_parametric_tmm(basic_tmm)
        solver = solve_tightly(model)
        jacobian = solver.jacobian()
        assert solver.derivative_parameter_names == list(DERIVATIVE_FORMULAS)

        differences = np.zeros_like(jacobian)
        for column, (name, (value, _)) in enumerate(DERIVATIVE_FORMULAS.items()):
            step = 1e-4 * value
            sides = []
            for sign in (1.0, -1.0):
                model.parameters.set_parameter(name, value + sign * step)
                model.formulas.apply_formulas()
                solver.solve()
                assert solver.converged
                sides.append(temperatures(model, BASIC_DIFFUSIVE))
            model.parameters.set_parameter(name, value)
            model.formulas.apply_formulas()
            differences[:, column] = (sides[0] - sides[1]) / (2.0 * step)
        scale = np.abs(differences).max(axis=0)
        assert np.all(np.abs(jacobian - differences).max(axis=0) < 1e-6 * scale)

    def test_linear_response_is_the_jacobian_of_a_load(self, basic_tmm):
        model = make_parametric_tmm(basic_tmm)
        solver = solve_tightly(model)
        jacobian = solver.jacobian()
        heat = np.zeros(4)
        heat[BASIC_DIFFUSIVE.index(15)] = 1.0
        response = solver.linear_response(heat)
        assert response.shape == (4,)
        np.testing.assert_allclose(response, jacobian[:, 3], rtol=1e-12, atol=0)
        cases = solver.linear_response(np.column_stack([heat, -2.5 * heat]))
        assert cases.shape == (4, 2)
        np.testing.assert_allclose(cases[:, 1], -2.5 * response, rtol=1e-12, atol=0)
        with pytest.raises(ValueError):
            solver.linear_response(np.zeros(5))

    def test_needs_a_converged_solve_and_a_parameter(self, basic_tmm):
        model = make_parametric_tmm(basic_tmm)
        solver = SteadyState(model)
        with pytest.raises(RuntimeError):
            solver.jacobian()
        solver.set_settings(solvers.Settings(tolerance=1e-11, max_iterations=1))
        solver.initialize()
        solver.solve()
        assert not solver.converged
        with pytest.raises(RuntimeError):
            solver.jacobian()
        solver.deinitialize()
        solver = solve(model)
        for name in DERIVATIVE_FORMULAS:
            model.formulas.parameters_with_derivatives.remove_parameter(name)
        with pytest.raises(ValueError):
            solver.jacobian()
