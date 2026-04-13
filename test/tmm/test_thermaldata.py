"""Test bindings for ThermalData."""

import numpy as np
import pytest

import pycanha_core as pcc

DenseTimeSeries = pcc.tmm.DenseTimeSeries
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

    def test_add_and_get_dense_time_series(self, td_with_network):
        td = td_with_network
        td.add_dense_time_series("test", 5, 3)
        assert td.has_dense_time_series("test") is True
        series = td.get_dense_time_series("test")
        assert series.num_timesteps == 5
        assert series.num_columns == 3

    def test_has_dense_time_series_false(self, td_with_network):
        assert td_with_network.has_dense_time_series("nonexistent") is False

    def test_remove_dense_time_series(self, td_with_network):
        td = td_with_network
        td.add_dense_time_series("to_remove", 2, 2)
        assert td.has_dense_time_series("to_remove") is True
        td.remove_dense_time_series("to_remove")
        assert td.has_dense_time_series("to_remove") is False

    def test_dense_time_series_reset(self, td_with_network):
        td = td_with_network
        td.add_dense_time_series("resettable", 3, 3)
        series = td.get_dense_time_series("resettable")
        series.values[0, 0] = 999.0
        series.reset()
        # After reset, re-add with same dimensions
        td.add_dense_time_series("resettable", 3, 3)
        series2 = td.get_dense_time_series("resettable")
        assert series2.values[0, 0] == pytest.approx(0.0)

    def test_dense_time_series_is_writable(self, td_with_network):
        td = td_with_network
        td.add_dense_time_series("writable", 2, 2)
        series = td.get_dense_time_series("writable")
        series.values[0, 0] = 42.0
        series.values[1, 1] = 99.0
        # Re-fetch to confirm write went through
        series2 = td.get_dense_time_series("writable")
        assert series2.values[0, 0] == pytest.approx(42.0)
        assert series2.values[1, 1] == pytest.approx(99.0)

    def test_size(self, td_with_network):
        td = td_with_network
        td.add_dense_time_series("a", 1, 1)
        td.add_dense_time_series("b", 1, 1)
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

    def test_dense_time_series_times_and_values(self, td_with_network):
        td = td_with_network
        td.add_dense_time_series("ts", 3, 2)
        series = td.get_dense_time_series("ts")
        series.times[0] = 0.0
        series.times[1] = 1.0
        series.times[2] = 2.0
        series.values[0, 0] = 10.0
        series.values[1, 0] = 20.0
        series.values[2, 0] = 30.0

        np.testing.assert_allclose(series.times, [0.0, 1.0, 2.0])
        np.testing.assert_allclose(series.values[:, 0], [10.0, 20.0, 30.0])
