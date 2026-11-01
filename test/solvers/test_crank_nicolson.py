"""The CrankNicolson solver."""

import numpy as np
import pycanha_core as pcc
import pytest

from .models import (
    BASIC_DIFFUSIVE,
    CHAIN_NODES,
    CONDUCTION_FACTORIZATIONS,
    CONFIGURATIONS,
    RADIATIVE_FACTORIZATIONS,
    REUSE_CONFIGURATIONS,
    make_chain_tmm,
    temperatures,
)

solvers = pcc.solvers
Attribute = pcc.DataModelAttribute
CrankNicolson = solvers.transient.CrankNicolson

# Transient of the basic network (conftest.make_basic_tmm) over 1e5 s with a
# 1000 s step, every node including the boundary one, the reference values of
# the 0.23 solver.
EXPECTED_TIMES = np.arange(0.0, 100001.0, 10000.0)
EXPECTED_TEMPERATURES = np.array(
    [
        [273.14999, 273.14999, 273.14999, 273.14999, 3.14999],
        [259.03552, 283.85105, 258.98241, 262.06791, 3.14999],
        [247.56014, 291.67014, 247.37629, 253.45623, 3.14999],
        [237.98527, 297.25685, 237.62266, 246.62735, 3.14999],
        [229.83503, 301.16946, 229.26392, 241.11244, 3.14999],
        [222.78667, 303.85891, 221.98896, 236.58283, 3.14999],
        [216.61234, 305.67267, 215.57742, 232.80415, 3.14999],
        [211.14591, 306.86934, 209.86801, 229.60718, 3.14999],
        [206.26295, 307.63674, 204.73939, 226.86828, 3.14999],
        [201.86811, 308.10888, 200.09819, 224.49601, 3.14999],
        [197.88691, 308.38019, 195.87117, 222.42185, 3.14999],
    ]
)


def run(model, start=0.0, end=100000.0, step=1000.0, interval=10000.0, **settings):
    solver = CrankNicolson(model)
    solver.set_settings(solvers.Settings(tolerance=1e-6, max_iterations=100, **settings))
    solver.set_simulation_time(start, end, step, interval)
    solver.initialize()
    solver.solve()
    return solver


def temperature_series(solver):
    return solver.output_model.get_dense_attribute(Attribute.T)


class TestLifecycle:
    def test_construction(self, basic_tmm):
        solver = CrankNicolson(basic_tmm)
        assert solver.name == "CrankNicolson"
        assert solver.solver_type == solvers.SolverType.CRANK_NICOLSON
        assert solver.initialized is False

    def test_registry_returns_one_solver(self, basic_tm):
        solver = basic_tm.solvers.transient.crank_nicolson
        assert isinstance(solver, CrankNicolson)
        assert solver is basic_tm.solvers.transient.crank_nicolson

    def test_solve_before_initialize_raises(self, basic_tmm):
        with pytest.raises(ValueError):
            CrankNicolson(basic_tmm).solve()

    def test_time_step_is_required(self, basic_tmm):
        with pytest.raises(ValueError):
            CrankNicolson(basic_tmm).initialize()

    def test_set_simulation_time_sets_the_four_settings(self, basic_tmm):
        solver = CrankNicolson(basic_tmm)
        solver.set_simulation_time(5.0, 1000.0, 100.0, 500.0)
        assert (solver.start_time, solver.end_time, solver.time_step, solver.output_interval) == (
            5.0,
            1000.0,
            100.0,
            500.0,
        )

    @pytest.mark.parametrize(
        "times", [(10.0, 0.0, 1.0, 1.0), (0.0, 10.0, 0.0, 1.0), (0.0, 10.0, 1.0, -1.0)]
    )
    def test_set_simulation_time_refuses_bad_times(self, times, basic_tmm):
        with pytest.raises(ValueError):
            CrankNicolson(basic_tmm).set_simulation_time(*times)

    def test_full_lifecycle(self, basic_tmm):
        solver = run(basic_tmm)
        assert solver.initialized is True
        assert solver.converged is True
        solver.deinitialize()
        assert solver.initialized is False


class TestOutput:
    def test_matches_the_reference(self, basic_tmm):
        series = temperature_series(run(basic_tmm))
        np.testing.assert_allclose(series.times, EXPECTED_TIMES, atol=1e-6)
        np.testing.assert_allclose(series.values, EXPECTED_TEMPERATURES, atol=1e-4)

    def test_output_model_lives_in_the_thermal_data(self, basic_tmm):
        solver = run(basic_tmm)
        assert solver.resolved_settings().output_name == "crank_nicolson"
        assert basic_tmm.thermal_data.models.has_model("crank_nicolson")
        series = temperature_series(solver)
        assert series.num_timesteps == 11
        assert series.num_columns == 5

    def test_output_name_and_attributes_are_settings(self, basic_tmm):
        solver = run(
            basic_tmm,
            output_name="hot_case",
            output_attributes={Attribute.T, Attribute.QI},
        )
        assert basic_tmm.thermal_data.models.has_model("hot_case")
        qi = solver.output_model.get_dense_attribute(Attribute.QI)
        assert qi.num_timesteps == 11

    def test_every_dense_attribute(self, basic_tmm):
        solver = run(basic_tmm, output_attributes=solvers.all_dense_output_attributes())
        assert solver.output_model.get_dense_attribute(Attribute.C).num_timesteps == 11


class TestConfigurations:
    @pytest.mark.parametrize("factorization", RADIATIVE_FACTORIZATIONS)
    @pytest.mark.parametrize("config", CONFIGURATIONS)
    def test_every_configuration_matches_the_reference(self, config, factorization, basic_tmm):
        solver = run(basic_tmm, factorization=factorization, **config)
        assert solver.converged
        np.testing.assert_allclose(temperature_series(solver).values, EXPECTED_TEMPERATURES, atol=1e-4)

    @pytest.mark.parametrize("factorization", CONDUCTION_FACTORIZATIONS)
    @pytest.mark.parametrize("config", CONFIGURATIONS)
    def test_every_configuration_agrees_on_conduction(self, config, factorization):
        reference = make_chain_tmm()
        run(reference, end=1000.0, step=10.0, interval=100.0, backend=solvers.Backend.EIGEN)
        model = make_chain_tmm()
        run(model, end=1000.0, step=10.0, interval=100.0, factorization=factorization, **config)
        np.testing.assert_allclose(
            temperatures(model, CHAIN_NODES), temperatures(reference, CHAIN_NODES), atol=1e-8
        )

    @pytest.mark.parametrize("config", REUSE_CONFIGURATIONS)
    def test_reusing_factors_agrees(self, config, basic_tmm):
        solver = run(basic_tmm, reuse_factors=True, **config)
        assert solver.converged
        np.testing.assert_allclose(temperature_series(solver).values, EXPECTED_TEMPERATURES, atol=1e-4)

    def test_fixed_couplings_agrees_on_a_constant_model(self, basic_tmm):
        solver = run(basic_tmm, fixed_couplings=True)
        np.testing.assert_allclose(temperature_series(solver).values, EXPECTED_TEMPERATURES, atol=1e-4)

    def test_reference_end_state(self, basic_tmm):
        run(basic_tmm)
        np.testing.assert_allclose(
            temperatures(basic_tmm, BASIC_DIFFUSIVE), EXPECTED_TEMPERATURES[-1, :4], atol=1e-4
        )
