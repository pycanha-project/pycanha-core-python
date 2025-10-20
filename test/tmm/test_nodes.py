import pytest

import pycanha_core as core


def test_nodes_add_and_get():
    nodes = core.tmm.Nodes()
    node = core.tmm.Node(101)
    node.T = 273.15
    node.C = 42.0

    nodes.add_node(node)

    assert nodes.num_nodes == 1
    assert nodes.get_T(101) == pytest.approx(273.15)
    assert nodes.get_C(101) == pytest.approx(42.0)
    assert nodes.get_idx_from_node_num(101) == 0


def test_couplings_store_values():
    nodes = core.tmm.Nodes()
    nodes.add_node(core.tmm.Node(1))
    nodes.add_node(core.tmm.Node(2))

    couplings = core.tmm.Couplings(nodes)
    couplings.add_coupling(1, 2, 15.0)

    assert couplings.get_coupling_value(1, 2) == pytest.approx(15.0)
    assert couplings.coupling_exists(1, 2)


def test_thermal_network_links_resources():
    network = core.tmm.ThermalNetwork()
    network.add_node(core.tmm.Node(1))
    network.add_node(core.tmm.Node(2))

    assert network.nodes.num_nodes == 2
    network.conductive_couplings.add_coupling(1, 2, 5.0)
    assert network.conductive_couplings.get_coupling_value(1, 2) == pytest.approx(5.0)
