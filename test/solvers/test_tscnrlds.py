"""Test bindings for TSCNRLDS transient solver."""

import numpy as np
import pycanha_core as pcc
import pytest

DataModelAttribute = pcc.DataModelAttribute


class TestTSCNRLDS:
    def test_construction(self, basic_tmm):
        solver = pcc.solvers.TSCNRLDS(basic_tmm)
        assert solver is not None

    def test_set_simulation_time(self, basic_tmm):
        solver = pcc.solvers.TSCNRLDS(basic_tmm)
        solver.set_simulation_time(0.0, 1000.0, 100.0, 500.0)
        # No assertion needed — just verify it doesn't raise

    def test_solver_properties_settable(self, basic_tmm):
        solver = pcc.solvers.TSCNRLDS(basic_tmm)
        solver.max_iters = 50
        solver.abstol_temp = 1e-7
        assert solver.max_iters == 50
        assert solver.abstol_temp == pytest.approx(1e-7)

    def test_full_solve_lifecycle(self, basic_tmm):
        solver = pcc.solvers.TSCNRLDS(basic_tmm)
        solver.max_iters = 100
        solver.abstol_temp = 1e-6
        solver.set_simulation_time(0.0, 100000.0, 1000.0, 10000.0)

        solver.initialize()
        assert solver.solver_initialized is True

        solver.solve()

        solver.deinitialize()
        assert solver.solver_initialized is False

    def test_solve_output_model(self, basic_tmm):
        solver = pcc.solvers.TSCNRLDS(basic_tmm)
        solver.max_iters = 100
        solver.abstol_temp = 1e-6
        solver.set_simulation_time(0.0, 100000.0, 1000.0, 10000.0)

        solver.initialize()
        solver.solve()
        solver.deinitialize()

        model_name = solver.output_model_name
        assert basic_tmm.thermal_data.models.has_model(model_name) is True
        output_model = solver.output_model
        series = output_model.get_dense_attribute(DataModelAttribute.T)
        assert series.num_timesteps == 11  # 0, 10k, 20k, ..., 100k
        assert series.num_columns == 5  # 5 nodes (times stored separately)

    def test_solve_temperatures(self, basic_tmm):
        solver = pcc.solvers.TSCNRLDS(basic_tmm)
        solver.max_iters = 100
        solver.abstol_temp = 1e-6
        solver.set_simulation_time(0.0, 100000.0, 1000.0, 10000.0)

        solver.initialize()
        solver.solve()
        solver.deinitialize()

        output_model = solver.output_model
        series = output_model.get_dense_attribute(DataModelAttribute.T)
        calculated_times = series.times
        calculated_temps = series.values

        expected_times = np.array(
            [
                0.0,
                10000.0,
                20000.0,
                30000.0,
                40000.0,
                50000.0,
                60000.0,
                70000.0,
                80000.0,
                90000.0,
                100000.0,
            ]
        )
        expected_temps = np.array(
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

        np.testing.assert_allclose(calculated_times, expected_times, atol=1e-1)
        np.testing.assert_allclose(calculated_temps, expected_temps, atol=1e-2)
