"""Each solver class has an attribute for every setting that applies to its type."""

import inspect

import pycanha_core as pcc
import pytest

from .models import make_chain_tmm

solvers = pcc.solvers
SolverType = solvers.SolverType

SOLVER_CLASSES = [
    (solvers.steady.SteadyState, SolverType.STEADY_STATE),
    (solvers.transient.CrankNicolson, SolverType.CRANK_NICOLSON),
    (solvers.sensitivity.JacobianPropagation, SolverType.JACOBIAN_PROPAGATION),
]
GROUP_VIEWS = {
    "pardiso": solvers.PardisoView,
    "eigen": solvers.EigenView,
    "accelerate": solvers.AccelerateView,
}
# Properties of the solver classes that are not settings.
OTHER_PROPERTIES = {
    "solver_type",
    "name",
    "initialized",
    "converged",
    "uses_cholesky",
    "num_factorizations",
    "iterations",
    "output_model",
    "parameter_names",
    "derivative_parameter_names",
}


def properties(cls):
    return {
        name
        for name in dir(cls)
        if not name.startswith("_") and isinstance(inspect.getattr_static(cls, name, None), property)
    }


@pytest.mark.parametrize(("cls", "solver_type"), SOLVER_CLASSES)
def test_the_attributes_follow_the_field_table(cls, solver_type):
    """A field the core adds must get an attribute; a field that does not apply must not."""
    fields = solvers.setting_fields()
    expected = {f.name for f in fields if not f.group and f.applies_to(solver_type)}
    expected |= {f.group for f in fields if f.group and f.applies_to(solver_type)}
    assert properties(cls) - OTHER_PROPERTIES == expected


@pytest.mark.parametrize("group", sorted(GROUP_VIEWS))
def test_the_group_views_follow_the_field_table(group):
    fields = [f for f in solvers.setting_fields() if f.group == group]
    assert properties(GROUP_VIEWS[group]) == {f.name.split(".", 1)[1] for f in fields}
    # A group field that applied to only some solver types would need a view
    # per type; none does today.
    for field in fields:
        for _, solver_type in SOLVER_CLASSES:
            assert field.applies_to(solver_type)


@pytest.mark.parametrize(("cls", "solver_type"), SOLVER_CLASSES)
def test_docstrings_come_from_the_field_table(cls, solver_type):
    for field in solvers.setting_fields():
        if field.group or not field.applies_to(solver_type):
            continue
        doc = getattr(cls, field.name).__doc__
        assert field.doc in doc
        assert ("frozen while the solver is initialised" in doc) == (
            field.mutability == solvers.Mutability.INIT
        )


def test_settings_that_do_not_apply_are_attribute_errors(basic_tmm):
    steady = solvers.steady.SteadyState(basic_tmm)
    jacobian = solvers.sensitivity.JacobianPropagation(basic_tmm)
    with pytest.raises(AttributeError):
        steady.time_step
    with pytest.raises(AttributeError):
        steady.time_step = 1.0
    with pytest.raises(AttributeError):
        jacobian.reuse_factors
    with pytest.raises(AttributeError):
        jacobian.reuse_tolerance = 1e-2


def test_an_attribute_reads_and_writes_the_solver_settings(basic_tmm):
    solver = solvers.transient.CrankNicolson(basic_tmm)
    assert solver.time_step is None
    solver.time_step = 10.0
    assert solver.time_step == 10.0
    assert solver.settings().time_step == 10.0
    solver.time_step = None
    assert not solver.settings().is_set("time_step")


def test_settings_returns_a_copy(basic_tmm):
    solver = solvers.steady.SteadyState(basic_tmm)
    settings = solver.settings()
    settings.tolerance = 1.0
    assert solver.tolerance is None
    assert settings.solver_type == SolverType.STEADY_STATE


def test_set_settings_stamps_the_solver_type(basic_tmm):
    solver = solvers.steady.SteadyState(basic_tmm)
    solver.set_settings(solvers.Settings(solver_type=SolverType.CRANK_NICOLSON, tolerance=1e-4))
    assert solver.settings().solver_type == SolverType.STEADY_STATE
    assert solver.tolerance == 1e-4


def test_set_and_unset_by_name(basic_tmm):
    solver = solvers.steady.SteadyState(basic_tmm)
    solver.set("backend", solvers.Backend.EIGEN)
    solver.set("eigen.ordering", "AMD")
    assert solver.backend == solvers.Backend.EIGEN
    assert solver.eigen.ordering == solvers.EigenOrdering.AMD
    solver.unset("backend")
    assert solver.backend is None
    with pytest.raises(ValueError):
        solver.set("no_such_setting", 1)


def test_init_attributes_are_frozen_while_initialised():
    # Eigen is in every build, so the same fields apply on every platform.
    solver = solvers.steady.SteadyState(make_chain_tmm())
    solver.backend = solvers.Backend.EIGEN
    solver.storage = solvers.Storage.SPARSE
    solver.initialize()
    with pytest.raises(RuntimeError, match="storage"):
        solver.storage = solvers.Storage.DENSE
    with pytest.raises(RuntimeError, match="eigen.ordering"):
        solver.eigen.ordering = solvers.EigenOrdering.AMD
    with pytest.raises(RuntimeError):
        solver.set_settings(solvers.Settings(backend=solvers.Backend.EIGEN, storage=solvers.Storage.DENSE))
    solver.deinitialize()
    solver.storage = solvers.Storage.DENSE
    assert solver.storage == solvers.Storage.DENSE


def test_a_group_the_backend_does_not_use_can_change_while_initialised():
    # A field counts as changed only if it applies before and after: the
    # PARDISO settings do nothing to an Eigen solver.
    solver = solvers.steady.SteadyState(make_chain_tmm())
    solver.backend = solvers.Backend.EIGEN
    solver.initialize()
    solver.pardiso.level = solvers.PardisoLevel.ONE_LEVEL
    assert solver.pardiso.level == solvers.PardisoLevel.ONE_LEVEL


def test_live_attributes_change_while_initialised():
    live = [
        f.name
        for f in solvers.setting_fields()
        if not f.group
        and f.mutability == solvers.Mutability.LIVE
        and f.applies_to(SolverType.STEADY_STATE)
    ]
    assert "tolerance" in live
    solver = solvers.steady.SteadyState(make_chain_tmm())
    solver.initialize()
    solver.tolerance = 1e-9
    solver.max_iterations = 50
    solver.solve()
    assert solver.resolved_settings().tolerance == 1e-9
    assert solver.converged


def test_group_views_write_through(basic_tmm):
    solver = solvers.transient.CrankNicolson(basic_tmm)
    solver.pardiso.level = solvers.PardisoLevel.ONE_LEVEL
    solver.pardiso.ordering = solvers.PardisoOrdering.MIN_DEGREE
    solver.pardiso.verbose = False
    solver.eigen.ordering = solvers.EigenOrdering.AMD
    solver.accelerate.ordering = solvers.AccelerateOrdering.AMD
    settings = solver.settings()
    assert settings.pardiso.level == solvers.PardisoLevel.ONE_LEVEL
    assert settings.pardiso.ordering == solvers.PardisoOrdering.MIN_DEGREE
    assert settings.pardiso.verbose is False
    assert settings.eigen.ordering == solvers.EigenOrdering.AMD
    assert settings.accelerate.ordering == solvers.AccelerateOrdering.AMD
    solver.pardiso.level = None
    assert not solver.settings().is_set("pardiso.level")


def test_iparm_is_assigned_as_a_whole_dict(basic_tmm):
    solver = solvers.transient.CrankNicolson(basic_tmm)
    solver.pardiso.iparm = {3: 61}
    assert solver.pardiso.iparm == {3: 61}
    solver.pardiso.iparm[4] = 1
    assert solver.pardiso.iparm == {3: 61}
    solver.pardiso.iparm = {**solver.pardiso.iparm, 4: 1}
    assert solver.settings().pardiso.iparm == {3: 61, 4: 1}


def test_a_view_keeps_its_solver_alive(basic_tmm):
    view = solvers.steady.SteadyState(basic_tmm).pardiso
    view.verbose = True
    assert view.verbose is True
