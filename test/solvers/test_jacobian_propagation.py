"""The JacobianPropagation solver."""

import numpy as np
import pycanha_core as pcc
import pytest

from .models import CONFIGURATIONS, RADIATIVE_FACTORIZATIONS, make_jacobian_tmm

solvers = pcc.solvers
Attribute = pcc.DataModelAttribute
JacobianPropagation = solvers.sensitivity.JacobianPropagation

# Temperatures [node 1, node 2] and derivatives [dT1/dk, dT1/dC] of
# make_jacobian_tmm(), the reference values of the 0.23 solver.
EXPECTED = [
    (0.0, [0.0, 1.0], [0.0, 0.0]),
    (1.0, [1.26424718, 1.0], [0.10364756, -0.73577107]),
    (2.0, [1.72933389, 1.0], [-0.32332125, -0.54134564]),
    (3.0, [1.90042832, 1.0], [-0.65149169, -0.29872244]),
    (4.0, [1.96336993, 1.0], [-0.83516103, -0.14652392]),
    (5.0, [1.98652466, 1.0], [-0.92588396, -0.06737837]),
]


def run(model, **settings):
    solver = JacobianPropagation(model)
    solver.set_settings(solvers.Settings(tolerance=1e-9, max_iterations=50, **settings))
    solver.set_simulation_time(0.0, 5.0, 0.01, 0.1)
    solver.initialize()
    solver.solve()
    return solver


def row_at(times, time):
    rows = np.where(np.isclose(times, time, atol=1e-12))[0]
    assert rows.size, f"time {time} was not produced by the solver"
    return int(rows[0])


def check_reference(solver):
    temperatures = solver.output_model.get_dense_attribute(Attribute.T)
    jacobian = solver.output_model.get_matrix_attribute(Attribute.JAC)
    assert jacobian.rows == 1
    assert jacobian.cols == 2
    for time, expected_t, expected_jac in EXPECTED:
        np.testing.assert_allclose(
            temperatures.values[row_at(temperatures.times, time)], expected_t, atol=5e-6
        )
        np.testing.assert_allclose(
            jacobian.at(row_at(jacobian.times, time))[0], expected_jac, atol=5e-6
        )


class TestJacobianPropagation:
    def test_construction(self):
        solver = JacobianPropagation(make_jacobian_tmm())
        assert solver.name == "JacobianPropagation"
        assert solver.solver_type == solvers.SolverType.JACOBIAN_PROPAGATION
        assert isinstance(solver, solvers.Solver)
        assert not isinstance(solver, solvers.transient.CrankNicolson)

    def test_registry_returns_one_solver(self, basic_tm):
        solver = basic_tm.solvers.sensitivity.jacobian_propagation
        assert isinstance(solver, JacobianPropagation)
        assert solver is basic_tm.solvers.sensitivity.jacobian_propagation

    def test_matches_the_reference(self):
        model = make_jacobian_tmm()
        solver = run(model)
        assert solver.parameter_names == ["k", "C"]
        assert solver.derivative_parameter_names == ["k", "C"]
        assert model.thermal_data.models.has_model("jacobian_propagation")
        check_reference(solver)
        solver.deinitialize()
        assert solver.initialized is False

    @pytest.mark.parametrize("factorization", RADIATIVE_FACTORIZATIONS)
    @pytest.mark.parametrize("config", CONFIGURATIONS)
    def test_every_configuration_matches_the_reference(self, config, factorization):
        check_reference(run(make_jacobian_tmm(), factorization=factorization, **config))

    def test_needs_a_derivative_parameter(self, basic_tmm):
        solver = JacobianPropagation(basic_tmm)
        solver.set_simulation_time(0.0, 10.0, 1.0, 1.0)
        with pytest.raises(ValueError):
            solver.initialize()
