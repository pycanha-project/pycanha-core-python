"""Test bindings for ThermalData."""

import numpy as np
import pycanha_core as pcc
import pytest

DataModel = pcc.tmm.DataModel
DataModelAttribute = pcc.DataModelAttribute
DenseTimeSeries = pcc.tmm.DenseTimeSeries
DenseMatrixTimeSeries = pcc.tmm.DenseMatrixTimeSeries
LookupTableVec1D = pcc.tmm.LookupTableVec1D
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

    def test_add_and_get_lookup_table(self, td_with_network):
        td = td_with_network
        table = LookupTableVec1D(
            np.array([0.0, 1.0]),
            np.array([[10.0, 20.0], [30.0, 40.0]]),
        )
        td.tables.add_table("test", table)

        assert td.tables.has_table("test") is True
        stored = td.tables.get_table("test")
        assert stored.size == 2
        assert stored.num_values == 2

    def test_has_lookup_table_false(self, td_with_network):
        assert td_with_network.tables.has_table("nonexistent") is False

    def test_remove_lookup_table(self, td_with_network):
        td = td_with_network
        table = LookupTableVec1D(
            np.array([0.0, 1.0]),
            np.array([[1.0], [2.0]]),
        )
        td.tables.add_table("to_remove", table)
        assert td.tables.has_table("to_remove") is True
        td.tables.remove_table("to_remove")
        assert td.tables.has_table("to_remove") is False

    def test_add_and_get_data_model(self, td_with_network):
        td = td_with_network
        td.models.add_model("transient", DataModel([1, 2]))

        assert td.models.has_model("transient") is True
        model = td.models.get_model("transient")
        assert model.node_numbers == [1, 2]

    def test_remove_data_model(self, td_with_network):
        td = td_with_network
        td.models.add_model("to_remove", DataModel([1, 2]))
        assert td.models.has_model("to_remove") is True
        td.models.remove_model("to_remove")
        assert td.models.has_model("to_remove") is False

    def test_dense_attribute_is_writable(self, td_with_network):
        td = td_with_network
        model = td.models.add_model("writable", DataModel([1, 2]))
        model.T.resize(2, 2)
        model.T.values[0, 0] = 42.0
        model.T.values[1, 1] = 99.0

        series = td.models.get_model("writable").get_dense_attribute(
            DataModelAttribute.T
        )
        assert series.values[0, 0] == pytest.approx(42.0)
        assert series.values[1, 1] == pytest.approx(99.0)

    def test_dense_matrix_time_series(self):
        series = DenseMatrixTimeSeries(2, 2)
        series.push_back(0.0, np.array([[1.0, 2.0], [3.0, 4.0]]))
        series.push_back(1.0, np.array([[5.0, 6.0], [7.0, 8.0]]))

        assert series.num_timesteps == 2
        assert series.rows == 2
        assert series.cols == 2
        np.testing.assert_allclose(series.times, [0.0, 1.0])
        np.testing.assert_allclose(series.at(1), [[5.0, 6.0], [7.0, 8.0]])

    def test_size(self, td_with_network):
        td = td_with_network
        td.tables.add_table(
            "table",
            LookupTableVec1D(np.array([0.0]), np.array([[1.0, 2.0]])),
        )
        td.models.add_model("model", DataModel([1, 2]))
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
        model = td.models.add_model("ts", DataModel([1, 2]))
        series = model.get_dense_attribute(DataModelAttribute.T)
        series.resize(3, 2)
        series.times[0] = 0.0
        series.times[1] = 1.0
        series.times[2] = 2.0
        series.values[0, 0] = 10.0
        series.values[1, 0] = 20.0
        series.values[2, 0] = 30.0

        np.testing.assert_allclose(series.times, [0.0, 1.0, 2.0])
        np.testing.assert_allclose(series.values[:, 0], [10.0, 20.0, 30.0])
