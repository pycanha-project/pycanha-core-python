"""Test that the pycanha_core package imports correctly and exposes submodules."""

import pycanha_core as pcc


def test_submodules_exist():
    assert hasattr(pcc, "gmm")
    assert hasattr(pcc, "tmm")
    assert hasattr(pcc, "parameters")
    assert hasattr(pcc, "solvers")


def test_nodetype_reexported():
    assert hasattr(pcc, "NodeType")
    assert pcc.NodeType is pcc.tmm.NodeType


def test_print_package_info():
    # TODO: print_package_info() writes to C++ stdout, not capturable by Python
    # Change to logging system in the C++ side
    pcc.print_package_info()
