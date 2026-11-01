"""Solvers refuse a model whose structure changed since initialize()."""

import pycanha_core as pcc
import pytest

solvers = pcc.solvers
ModelChangedError = solvers.ModelChangedError


def transient(model):
    solver = solvers.transient.CrankNicolson(model)
    solver.set_simulation_time(0.0, 2.0, 0.1, 0.5)
    return solver


def test_is_a_runtime_error():
    assert issubclass(ModelChangedError, RuntimeError)
    assert ModelChangedError.__doc__


@pytest.mark.parametrize("make_solver", [solvers.steady.SteadyState, transient])
def test_a_coupling_added_after_initialize_raises(make_solver, basic_tmm):
    solver = make_solver(basic_tmm)
    solver.initialize()
    basic_tmm.add_conductive_coupling(10, 20, 0.1)
    with pytest.raises(ModelChangedError, match="initialize"):
        solver.solve()


def test_a_node_added_after_initialize_raises(basic_tmm):
    solver = solvers.steady.SteadyState(basic_tmm)
    solver.initialize()
    basic_tmm.add_node(pcc.tmm.Node(30))
    with pytest.raises(ModelChangedError):
        solver.solve()


def test_a_new_value_does_not_raise(basic_tmm):
    solver = transient(basic_tmm)
    solver.initialize()
    basic_tmm.conductive_couplings.set_coupling_value(10, 15, 0.2)
    basic_tmm.nodes.set_qi(15, 100.0)
    solver.solve()
    assert solver.converged


def test_initialize_again_recovers(basic_tmm):
    solver = solvers.steady.SteadyState(basic_tmm)
    solver.max_iterations = 100
    solver.initialize()
    basic_tmm.add_conductive_coupling(10, 20, 0.1)
    with pytest.raises(ModelChangedError):
        solver.solve()
    solver.initialize()
    solver.solve()
    assert solver.converged


def test_a_coupling_added_in_the_solver_loop_callback_raises(basic_tm):
    def solver_loop(context):
        context.tmm.add_conductive_coupling(10, 20, 0.1)

    basic_tm.callbacks.solver_loop = solver_loop
    solver = basic_tm.solvers.steady.steady_state
    solver.initialize()
    with pytest.raises(ModelChangedError, match="callback"):
        solver.solve()


def test_a_coupling_added_in_the_after_timestep_callback_raises(basic_tm):
    def after_timestep(context):
        if context.time > 0.5:
            context.tmm.add_conductive_coupling(10, 20, 0.1)

    basic_tm.callbacks.after_timestep = after_timestep
    solver = basic_tm.solvers.transient.crank_nicolson
    solver.set_simulation_time(0.0, 2.0, 0.1, 0.5)
    solver.initialize()
    with pytest.raises(ModelChangedError, match="callback"):
        solver.solve()
    assert solver.report().status == solvers.SolveStatus.FAILED
