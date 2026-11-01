"""Test callback registry and callback context bindings."""

import pycanha_core as pcc
import pytest

solvers = pcc.solvers


def run_transient_solver(solver):
    solver.set_settings(solvers.Settings(tolerance=1e-9, max_iterations=20))
    solver.set_simulation_time(0.0, 2.0, 0.1, 0.5)
    solver.initialize()
    solver.solve()
    solver.deinitialize()
    return solver


class TestCallbacks:
    def test_after_timestep_receives_callback_context(self, basic_tm):
        seen = []

        def after_timestep(context):
            seen.append(
                (
                    type(context).__name__,
                    context.tm.name,
                    context.tmm.name,
                    type(context.solver).__name__,
                    context.time,
                )
            )

        basic_tm.callbacks.after_timestep = after_timestep

        run_transient_solver(basic_tm.solvers.transient.crank_nicolson)

        assert seen
        assert seen[0][0] == "CallbackContext"
        assert seen[0][1] == basic_tm.name
        assert seen[0][2] == basic_tm.tmm.name
        assert seen[0][3] == "CrankNicolson"
        assert seen[-1][4] > 0.0

    @pytest.mark.parametrize(
        ("registry", "attribute", "cls"),
        [
            ("steady", "steady_state", solvers.steady.SteadyState),
            ("transient", "crank_nicolson", solvers.transient.CrankNicolson),
        ],
    )
    def test_the_context_solver_is_the_running_solver(self, registry, attribute, cls, basic_tm):
        solver = getattr(getattr(basic_tm.solvers, registry), attribute)
        seen = []
        basic_tm.callbacks.solver_loop = lambda context: seen.append(context.solver)
        if cls is solvers.transient.CrankNicolson:
            solver.set_simulation_time(0.0, 1.0, 0.5, 0.5)
        solver.initialize()
        solver.solve()
        assert seen
        assert all(type(s) is cls and s is solver for s in seen)

    def test_the_jacobian_solver_is_seen_as_its_own_type(self, basic_tm):
        tmm = basic_tm.tmm
        tmm.parameters.add_parameter("k", 0.1)
        tmm.formulas.add_parameter_formula(tmm.entities.conductive_coupling(10, 15), "k")
        tmm.formulas.parameters_with_derivatives.add_parameter("k")
        tmm.formulas.apply_formulas()

        seen = []
        basic_tm.callbacks.after_timestep = lambda context: seen.append(context.solver)
        solver = basic_tm.solvers.sensitivity.jacobian_propagation
        run_transient_solver(solver)

        assert seen
        assert all(type(s) is solvers.sensitivity.JacobianPropagation for s in seen)
        assert isinstance(seen[0], solvers.Solver)
        assert seen[0].derivative_parameter_names == ["k"]

    def test_inactive_callbacks_do_not_mutate_model(self, basic_tm):
        call_count = 0
        initial_qi = basic_tm.tmm.nodes.get_qi(15)

        def after_timestep(context):
            nonlocal call_count
            call_count += 1
            context.tmm.nodes.set_qi(15, 0.0)

        basic_tm.callbacks.active = False
        basic_tm.callbacks.after_timestep = after_timestep

        run_transient_solver(basic_tm.solvers.transient.crank_nicolson)

        assert call_count == 0
        assert basic_tm.tmm.nodes.get_qi(15) == pytest.approx(initial_qi)
