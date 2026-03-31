"""Test bindings for ThermalNetwork."""

import pytest

import pycanha_core as pcc

Node = pcc.tmm.Node
NodeType = pcc.tmm.NodeType
ThermalNetwork = pcc.tmm.ThermalNetwork


class TestThermalNetwork:
    def test_default_construction(self):
        network = ThermalNetwork()
        assert network.nodes.num_nodes == 0

    def test_add_node(self):
        network = ThermalNetwork()
        network.add_node(Node(1))
        network.add_node(Node(2))
        assert network.nodes.num_nodes == 2

    def test_remove_node(self):
        network = ThermalNetwork()
        network.add_node(Node(1))
        network.add_node(Node(2))
        network.remove_node(1)
        assert network.nodes.num_nodes == 1
        assert network.nodes.is_node(2) is True

    def test_conductive_couplings_via_network(self):
        network = ThermalNetwork()
        network.add_node(Node(1))
        network.add_node(Node(2))
        network.conductive_couplings.add_coupling(1, 2, 5.0)
        assert network.conductive_couplings.get_coupling_value(1, 2) == pytest.approx(
            5.0
        )

    def test_radiative_couplings_via_network(self):
        network = ThermalNetwork()
        network.add_node(Node(1))
        network.add_node(Node(2))
        network.radiative_couplings.add_coupling(1, 2, 0.5)
        assert network.radiative_couplings.get_coupling_value(1, 2) == pytest.approx(
            0.5
        )

    def test_nodes_property_reference(self):
        """nodes property should return a reference to the same underlying object."""
        network = ThermalNetwork()
        network.add_node(Node(1))
        network.nodes.set_T(1, 999.0)
        assert network.nodes.get_T(1) == pytest.approx(999.0)

    def test_diffusive_and_boundary_counts(self):
        network = ThermalNetwork()
        d = Node(1)
        d.type = NodeType.DIFFUSIVE
        b = Node(2)
        b.type = NodeType.BOUNDARY
        network.add_node(d)
        network.add_node(b)
        assert network.nodes.num_diff_nodes == 1
        assert network.nodes.num_bound_nodes == 1

    def test_nodes_ptr(self):
        network = ThermalNetwork()
        network.add_node(Node(1))
        assert network.nodes_ptr is not None
