"""Test bindings for Entity class and EntityType enum."""

import pytest

import pycanha_core as pcc

Entity = pcc.parameters.Entity
EntityType = pcc.parameters.EntityType
Node = pcc.tmm.Node
NodeType = pcc.tmm.NodeType
ThermalNetwork = pcc.tmm.ThermalNetwork


@pytest.fixture
def network_with_couplings():
    """Network with nodes 1 (diff), 2 (diff) and couplings between them."""
    network = ThermalNetwork()
    n1 = Node(1)
    n1.T = 300.0
    n1.qi = 100.0
    n2 = Node(2)
    n2.T = 200.0
    network.add_node(n1)
    network.add_node(n2)
    network.conductive_couplings.add_coupling(1, 2, 10.0)
    network.radiative_couplings.add_coupling(1, 2, 0.5)
    return network


class TestAttributeEntity:
    def test_construction(self, network_with_couplings):
        entity = Entity.t(network_with_couplings, 1)
        assert entity is not None

    def test_get_value(self, network_with_couplings):
        entity = Entity.t(network_with_couplings, 1)
        assert entity.get_value() == pytest.approx(300.0)

    def test_set_value(self, network_with_couplings):
        entity = Entity.t(network_with_couplings, 1)
        entity.set_value(350.0)
        assert entity.get_value() == pytest.approx(350.0)
        # Also reflected in the network
        assert network_with_couplings.nodes.get_T(1) == pytest.approx(350.0)

    def test_qi_attribute(self, network_with_couplings):
        entity = Entity.qi(network_with_couplings, 1)
        assert entity.get_value() == pytest.approx(100.0)

    def test_string_representation(self, network_with_couplings):
        entity = Entity.t(network_with_couplings, 1)
        rep = entity.string_representation()
        assert isinstance(rep, str)
        assert len(rep) > 0

    def test_type_property(self, network_with_couplings):
        entity = Entity.t(network_with_couplings, 1)
        assert entity.type == EntityType.T

    def test_value_pointer(self, network_with_couplings):
        entity = Entity.t(network_with_couplings, 1)
        ptr = entity.get_value_pointer()
        assert ptr != 0

    def test_make_factory(self, network_with_couplings):
        entity = Entity.make(network_with_couplings, EntityType.T, 1)
        assert entity.get_value() == pytest.approx(300.0)

    def test_from_string(self, network_with_couplings):
        entity = Entity.from_string(network_with_couplings, "T1")
        assert entity is not None
        assert entity.get_value() == pytest.approx(300.0)

    def test_exists(self, network_with_couplings):
        entity = Entity.t(network_with_couplings, 1)
        assert entity.exists() is True

    def test_writable(self, network_with_couplings):
        entity = Entity.t(network_with_couplings, 1)
        assert entity.writable is True

    def test_node_properties(self, network_with_couplings):
        entity = Entity.t(network_with_couplings, 1)
        assert entity.node_1 == 1
        assert entity.node_count == 1

    def test_is_same_as(self, network_with_couplings):
        e1 = Entity.t(network_with_couplings, 1)
        e2 = Entity.t(network_with_couplings, 1)
        assert e1.is_same_as(e2)

    def test_eq(self, network_with_couplings):
        e1 = Entity.t(network_with_couplings, 1)
        e2 = Entity.t(network_with_couplings, 1)
        assert e1 == e2


class TestConductiveCouplingEntity:
    def test_construction(self, network_with_couplings):
        entity = Entity.gl(network_with_couplings, 1, 2)
        assert entity is not None

    def test_get_value(self, network_with_couplings):
        entity = Entity.gl(network_with_couplings, 1, 2)
        assert entity.get_value() == pytest.approx(10.0)

    def test_set_value(self, network_with_couplings):
        entity = Entity.gl(network_with_couplings, 1, 2)
        entity.set_value(20.0)
        assert entity.get_value() == pytest.approx(20.0)
        assert network_with_couplings.conductive_couplings.get_coupling_value(
            1, 2
        ) == pytest.approx(20.0)

    def test_string_representation(self, network_with_couplings):
        entity = Entity.gl(network_with_couplings, 1, 2)
        rep = entity.string_representation()
        assert isinstance(rep, str)

    def test_type_property(self, network_with_couplings):
        entity = Entity.gl(network_with_couplings, 1, 2)
        assert entity.type == EntityType.GL

    def test_node_properties(self, network_with_couplings):
        entity = Entity.gl(network_with_couplings, 1, 2)
        assert entity.node_1 == 1
        assert entity.node_2 == 2
        assert entity.node_count == 2


class TestRadiativeCouplingEntity:
    def test_construction(self, network_with_couplings):
        entity = Entity.gr(network_with_couplings, 1, 2)
        assert entity is not None

    def test_get_value(self, network_with_couplings):
        entity = Entity.gr(network_with_couplings, 1, 2)
        assert entity.get_value() == pytest.approx(0.5)

    def test_set_value(self, network_with_couplings):
        entity = Entity.gr(network_with_couplings, 1, 2)
        entity.set_value(0.9)
        assert entity.get_value() == pytest.approx(0.9)

    def test_string_representation(self, network_with_couplings):
        entity = Entity.gr(network_with_couplings, 1, 2)
        rep = entity.string_representation()
        assert isinstance(rep, str)

    def test_type_property(self, network_with_couplings):
        entity = Entity.gr(network_with_couplings, 1, 2)
        assert entity.type == EntityType.GR
