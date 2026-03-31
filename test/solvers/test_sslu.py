"""Test bindings for SSLU steady-state solver."""

import numpy as np
import pytest

import pycanha_core as pcc


class TestSSLU:
    def test_construction(self, basic_tmm):
        solver = pcc.solvers.SSLU(basic_tmm)
        assert solver is not None

    def test_solver_name(self, basic_tmm):
        solver = pcc.solvers.SSLU(basic_tmm)
        assert isinstance(solver.solver_name, str)
        assert len(solver.solver_name) > 0

    def test_solver_properties_defaults(self, basic_tmm):
        solver = pcc.solvers.SSLU(basic_tmm)
        assert isinstance(solver.MAX_ITERS, int)
        assert isinstance(solver.abstol_temp, float)
        assert isinstance(solver.abstol_enrgy, float)
        assert isinstance(solver.eps_capacity, float)
        assert isinstance(solver.eps_coupling, float)

    def test_solver_properties_settable(self, basic_tmm):
        solver = pcc.solvers.SSLU(basic_tmm)
        solver.MAX_ITERS = 200
        solver.abstol_temp = 1e-8
        assert solver.MAX_ITERS == 200
        assert solver.abstol_temp == pytest.approx(1e-8)

    def test_solver_initialized_flag(self, basic_tmm):
        solver = pcc.solvers.SSLU(basic_tmm)
        assert solver.solver_initialized is False

    def test_full_solve_lifecycle(self, basic_tmm):
        solver = pcc.solvers.SSLU(basic_tmm)
        solver.MAX_ITERS = 100
        solver.abstol_temp = 1e-6

        solver.initialize()
        assert solver.solver_initialized is True

        solver.solve()
        assert solver.solver_converged is True

        solver.deinitialize()
        assert solver.solver_initialized is False

    def test_solve_temperatures(self, basic_tmm):
        solver = pcc.solvers.SSLU(basic_tmm)
        solver.MAX_ITERS = 100
        solver.abstol_temp = 1e-6

        solver.initialize()
        solver.solve()
        solver.deinitialize()

        calculated = np.array([
            basic_tmm.nodes.get_T(10),
            basic_tmm.nodes.get_T(15),
            basic_tmm.nodes.get_T(20),
            basic_tmm.nodes.get_T(25),
            basic_tmm.nodes.get_T(99),
        ])
        expected = np.array([132.38706, 306.56526, 111.78443, 200.32387, 3.14999])
        np.testing.assert_allclose(calculated, expected, atol=1e-2)
