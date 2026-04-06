"""Test bindings for the TSCNRLDS_JACOBIAN transient solver."""

import numpy as np
import pycanha_core as pcc


def make_jacobian_example_model():
    model = pcc.tmm.ThermalMathematicalModel("jacobian_python_example")

    diffusive_node = pcc.tmm.Node(1)
    diffusive_node.T = 0.0
    diffusive_node.capacity = 1.0
    diffusive_node.qi = 1.0

    boundary_node = pcc.tmm.Node(2)
    boundary_node.type = pcc.NodeType.BOUNDARY
    boundary_node.T = 1.0

    model.add_node(diffusive_node)
    model.add_node(boundary_node)
    model.add_conductive_coupling(1, 2, 1.0)

    model.parameters.add_parameter("k", 1.0)
    model.parameters.add_parameter("C", 1.0)

    conductive_entity = pcc.parameters.ConductiveCouplingEntity(model.network, 1, 2)
    capacity_entity = pcc.parameters.AttributeEntity(model.network, "C", 1)

    model.formulas.add_formula(
        pcc.parameters.ParameterFormula(conductive_entity, model.parameters, "k")
    )
    model.formulas.add_formula(
        pcc.parameters.ParameterFormula(capacity_entity, model.parameters, "C")
    )
    model.formulas.apply_formulas()

    return model


def find_time_row(table, time_value):
    matching_rows = np.where(np.isclose(table[:, 0], time_value, atol=1e-12))[0]
    if matching_rows.size == 0:
        raise AssertionError(f"Time sample {time_value} was not produced by the solver")
    return int(matching_rows[0])


class TestTSCNRLDSJacobian:
    def test_construction(self):
        solver = pcc.solvers.TSCNRLDS_JACOBIAN(make_jacobian_example_model())
        assert solver is not None

    def test_solve_python_example(self):
        model = make_jacobian_example_model()
        solver = pcc.solvers.TSCNRLDS_JACOBIAN(model)
        solver.MAX_ITERS = 50
        solver.abstol_temp = 1e-9
        solver.set_simulation_time(0.0, 5.0, 0.01, 0.1)

        solver.initialize()
        solver.solve()

        assert model.thermal_data.has_table("TSCNRLDS_OUTPUT") is True
        assert model.thermal_data.has_table("TSCNRLDS_JACOBIAN_OUTPUT") is True
        assert solver.parameter_names == ["k", "C"]

        temperature_output = model.thermal_data.get_table("TSCNRLDS_OUTPUT")
        jacobian_output = model.thermal_data.get_table("TSCNRLDS_JACOBIAN_OUTPUT")

        assert temperature_output.shape[1] == 3
        assert jacobian_output.shape[1] == 3
        assert jacobian_output.shape[0] >= 2

        expected_temperature_samples = np.array(
            [
                [0.0, 0.0, 1.0],
                [1.0, 1.26424718, 1.0],
                [2.0, 1.72933389, 1.0],
                [3.0, 1.90042832, 1.0],
                [4.0, 1.96336993, 1.0],
                [5.0, 1.98652466, 1.0],
            ]
        )
        expected_jacobian_samples = np.array(
            [
                [0.0, 0.0, 0.0],
                [1.0, 0.10364756, -0.73577107],
                [2.0, -0.32332125, -0.54134564],
                [3.0, -0.65149169, -0.29872244],
                [4.0, -0.83516103, -0.14652392],
                [5.0, -0.92588396, -0.06737837],
            ]
        )

        for expected_temperature, expected_jacobian in zip(
            expected_temperature_samples, expected_jacobian_samples, strict=True
        ):
            row = find_time_row(temperature_output, expected_temperature[0])
            np.testing.assert_allclose(
                temperature_output[row], expected_temperature, atol=5e-6
            )
            np.testing.assert_allclose(
                jacobian_output[row], expected_jacobian, atol=5e-6
            )

        solver.deinitialize()
        assert solver.solver_initialized is False
