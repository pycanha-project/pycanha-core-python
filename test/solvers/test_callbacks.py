"""Test callback registry and callback context bindings."""

import pytest


def run_transient_solver(tm):
    solver = tm.solvers.tscnrlds
    solver.max_iters = 20
    solver.abstol_temp = 1e-9
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

        run_transient_solver(basic_tm)

        assert seen
        assert seen[0][0] == "CallbackContext"
        assert seen[0][1] == basic_tm.name
        assert seen[0][2] == basic_tm.tmm.name
        assert seen[0][3] == "TSCNRLDS"
        assert seen[-1][4] > 0.0

    def test_inactive_callbacks_do_not_mutate_model(self, basic_tm):
        call_count = 0
        initial_qi = basic_tm.tmm.nodes.get_qi(15)

        def after_timestep(context):
            nonlocal call_count
            call_count += 1
            context.tmm.nodes.set_qi(15, 0.0)

        basic_tm.callbacks.active = False
        basic_tm.callbacks.after_timestep = after_timestep

        run_transient_solver(basic_tm)

        assert call_count == 0
        assert basic_tm.tmm.nodes.get_qi(15) == pytest.approx(initial_qi)
