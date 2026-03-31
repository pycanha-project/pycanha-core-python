"""Shared fixtures for pycanha_core binding tests."""

import numpy as np
import pytest

import pycanha_core as pcc


@pytest.fixture
def origin():
    return np.array([0.0, 0.0, 0.0])


@pytest.fixture
def point_x():
    return np.array([1.0, 0.0, 0.0])


@pytest.fixture
def point_y():
    return np.array([0.0, 1.0, 0.0])


@pytest.fixture
def point_z():
    return np.array([0.0, 0.0, 1.0])


@pytest.fixture
def point_xy():
    return np.array([1.0, 1.0, 0.0])


def make_basic_tmm():
    """Create a small TMM with 4 diffusive + 1 boundary node for solver tests."""
    tmm = pcc.tmm.ThermalMathematicalModel("test_model")

    node_10 = pcc.tmm.Node(10)
    node_15 = pcc.tmm.Node(15)
    node_20 = pcc.tmm.Node(20)
    node_25 = pcc.tmm.Node(25)
    env_node = pcc.tmm.Node(99)

    init_temp = 273.15
    for n in (node_10, node_15, node_20, node_25):
        n.T = init_temp
        n.capacity = 2e5

    env_node.T = 3.15
    env_node.type = pcc.NodeType.BOUNDARY

    node_15.qi = 500.0

    for n in (node_10, node_15, node_20, node_25, env_node):
        tmm.add_node(n)

    tmm.add_conductive_coupling(10, 15, 0.1)
    tmm.add_conductive_coupling(20, 25, 0.1)

    tmm.add_radiative_coupling(10, 99, 1.0)
    tmm.add_radiative_coupling(20, 99, 1.0)
    tmm.add_radiative_coupling(15, 25, 0.2)
    tmm.add_radiative_coupling(15, 99, 0.8)
    tmm.add_radiative_coupling(25, 99, 0.8)

    return tmm


@pytest.fixture
def basic_tmm():
    return make_basic_tmm()
