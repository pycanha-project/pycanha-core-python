"""Test bindings for ThermalData."""

import numpy as np
import pytest

import pycanha_core as pcc

Node = pcc.tmm.Node
ThermalData = pcc.tmm.ThermalData
ThermalNetwork = pcc.tmm.ThermalNetwork


class TestThermalData:
    @pytest.fixture
    def td_with_network(self):
        network = ThermalNetwork()
        network.add_node(Node(1))
        network.add_node(Node(2))
        td = ThermalData(network)
        return td

    def test_default_construction(self):
        td = ThermalData()
        assert td.size == 0

    def test_construction_with_network(self, td_with_network):
        assert td_with_network is not None

    def test_create_and_get_table(self, td_with_network):
        td = td_with_network
        td.create_new_table("test", 5, 3)
        assert td.has_table("test") is True
        table = td.get_table("test")
        assert table.shape == (5, 3)

    def test_has_table_false(self, td_with_network):
        assert td_with_network.has_table("nonexistent") is False

    def test_remove_table(self, td_with_network):
        td = td_with_network
        td.create_new_table("to_remove", 2, 2)
        assert td.has_table("to_remove") is True
        td.remove_table("to_remove")
        assert td.has_table("to_remove") is False

    def test_create_reset_table(self, td_with_network):
        td = td_with_network
        td.create_new_table("resettable", 3, 3)
        table = td.get_table("resettable")
        table[0, 0] = 999.0
        td.create_reset_table("resettable", 3, 3)
        table2 = td.get_table("resettable")
        assert table2[0, 0] == pytest.approx(0.0)

    def test_table_is_writable(self, td_with_network):
        td = td_with_network
        td.create_new_table("writable", 2, 2)
        table = td.get_table("writable")
        table[0, 0] = 42.0
        table[1, 1] = 99.0
        # Re-fetch to confirm write went through
        table2 = td.get_table("writable")
        assert table2[0, 0] == pytest.approx(42.0)
        assert table2[1, 1] == pytest.approx(99.0)

    def test_size(self, td_with_network):
        td = td_with_network
        td.create_new_table("a", 1, 1)
        td.create_new_table("b", 1, 1)
        assert td.size == 2

    def test_network_property(self, td_with_network):
        td = td_with_network
        assert td.network.nodes.num_nodes == 2

    def test_associate(self):
        td = ThermalData()
        network = ThermalNetwork()
        network.add_node(Node(1))
        td.associate(network)
        assert td.network.nodes.num_nodes == 1
