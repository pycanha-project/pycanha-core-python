"""Test that the pycanha_core package imports correctly and exposes submodules."""

import pycanha_core as pcc


def test_submodules_exist():
    assert hasattr(pcc, "gmm")
    assert hasattr(pcc, "tmm")
    assert hasattr(pcc, "parameters")
    assert hasattr(pcc, "solvers")


def test_v014_owner_and_registry_exports_exist():
    assert hasattr(pcc.tmm, "ThermalModel")
    assert hasattr(pcc.tmm, "EntitiesHelper")
    assert hasattr(pcc.solvers, "SolverRegistry")
    assert hasattr(pcc.solvers, "CallbackRegistry")
    assert hasattr(pcc.solvers, "CallbackContext")
    assert hasattr(pcc.parameters, "DerivativeParameterRegistry")


def test_logging_exports_exist():
    assert hasattr(pcc, "log")
    assert hasattr(pcc, "LogLevel")


def test_nodetype_reexported():
    assert hasattr(pcc, "NodeType")
    assert pcc.NodeType is pcc.tmm.NodeType


def test_data_model_attribute_reexported():
    assert hasattr(pcc, "DataModelAttribute")
    assert pcc.DataModelAttribute is pcc.tmm.DataModelAttribute


def test_print_package_info():
    # TODO: print_package_info() writes to C++ stdout, not capturable by Python
    # Change to logging system in the C++ side
    pcc.print_package_info()


def test_build_constants_live_at_the_package_root():
    assert isinstance(pcc.MKL_ENABLED, bool)
    assert isinstance(pcc.ACCELERATE_ENABLED, bool)
    assert not hasattr(pcc.solvers, "MKL_ENABLED")


def test_submodules_are_importable_by_name():
    import importlib

    for name in (
        "pycanha_core.gmm",
        "pycanha_core.tmm",
        "pycanha_core.parameters",
        "pycanha_core.solvers",
        "pycanha_core.solvers.steady",
        "pycanha_core.solvers.transient",
        "pycanha_core.solvers.sensitivity",
        "pycanha_core.radiative",
        "pycanha_core.conduction",
        "pycanha_core.log",
    ):
        module = importlib.import_module(name)
        attribute = name.removeprefix("pycanha_core.").split(".")
        expected = pcc
        for part in attribute:
            expected = getattr(expected, part)
        assert module is expected


def test_from_imports_of_nested_names():
    from pycanha_core.solvers import Settings
    from pycanha_core.solvers.sensitivity import JacobianPropagation
    from pycanha_core.solvers.steady import SteadyState
    from pycanha_core.solvers.transient import CrankNicolson
    from pycanha_core.tmm import Node

    assert Settings is pcc.solvers.Settings
    assert SteadyState is pcc.solvers.steady.SteadyState
    assert CrankNicolson is pcc.solvers.transient.CrankNicolson
    assert JacobianPropagation is pcc.solvers.sensitivity.JacobianPropagation
    assert Node is pcc.tmm.Node


def test_the_0_23_solver_api_is_gone():
    for name in (
        "SSLU",
        "SSLU_CGS",
        "TSCN",
        "TSCNRL",
        "TSCNRLDS",
        "TSCNRLDS_JACOBIAN",
        "SteadyStateSolver",
        "TransientSolver",
        "SolverOutputConfig",
        "SolverEngine",
        "DirectSolverType",
        "IterativeSolverType",
        "default_solver_engine",
        "resolve_solver_type",
    ):
        assert not hasattr(pcc.solvers, name), name
