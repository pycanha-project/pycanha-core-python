"""One Settings shared by solvers of every type."""

import logging

import pycanha_core as pcc
import pytest

from .models import make_jacobian_tmm

solvers = pcc.solvers
log = pcc.log


@pytest.fixture
def shared():
    return solvers.Settings(
        tolerance=1e-8,
        max_iterations=50,
        time_step=0.1,
        end_time=1.0,
        output_interval=0.5,
        reuse_tolerance=1e-3,
    )


def test_one_settings_runs_every_type(shared):
    model = make_jacobian_tmm()
    runs = [
        solvers.steady.SteadyState(model),
        solvers.transient.CrankNicolson(model),
        solvers.sensitivity.JacobianPropagation(model),
    ]
    for solver in runs:
        solver.set_settings(shared)
        solver.initialize()
        solver.solve()
        assert solver.converged
        assert solver.settings().solver_type == solver.solver_type


def test_fields_that_do_not_apply_warn_at_initialize(shared, caplog):
    # The time fields do not apply to a steady state, reuse_tolerance not to
    # a sensitivity run.
    model = make_jacobian_tmm()
    steady = solvers.steady.SteadyState(model)
    steady.set_settings(shared)
    with caplog.at_level(logging.WARNING, logger="pycanha"):
        steady.initialize()
    messages = [r.getMessage() for r in caplog.records if r.levelno == logging.WARNING]
    assert any("time_step" in m for m in messages)
    not_applied = {n.name for n in steady.resolved_settings().not_applied}
    assert {"end_time", "time_step", "output_interval"} <= not_applied

    caplog.clear()
    jacobian = solvers.sensitivity.JacobianPropagation(model)
    jacobian.set_settings(shared)
    with caplog.at_level(logging.WARNING, logger="pycanha"):
        jacobian.initialize()
    messages = [r.getMessage() for r in caplog.records if r.levelno == logging.WARNING]
    assert any("reuse_tolerance" in m for m in messages)


def test_the_warnings_are_kept_in_the_report(shared):
    steady = solvers.steady.SteadyState(make_jacobian_tmm())
    steady.set_settings(shared)
    steady.initialize()
    steady.solve()
    assert any("time_step" in warning for warning in steady.report().warnings)
