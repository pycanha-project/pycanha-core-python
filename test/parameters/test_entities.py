"""Test bindings for ThermalEntity, AttributeEntity, CouplingEntity classes."""

import pytest

import pycanha_core as pcc

AttributeEntity = pcc.parameters.AttributeEntity
ConductiveCouplingEntity = pcc.parameters.ConductiveCouplingEntity
Node = pcc.tmm.Node
NodeType = pcc.tmm.NodeType
RadiativeCouplingEntity = pcc.parameters.RadiativeCouplingEntity
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
        entity = AttributeEntity(network_with_couplings, "T", 1)
        assert entity is not None

    def test_get_value(self, network_with_couplings):
        entity = AttributeEntity(network_with_couplings, "T", 1)
        assert entity.get_value() == pytest.approx(300.0)

    def test_set_value(self, network_with_couplings):
        entity = AttributeEntity(network_with_couplings, "T", 1)
        entity.set_value(350.0)
        assert entity.get_value() == pytest.approx(350.0)
        # Also reflected in the network
        assert network_with_couplings.nodes.get_T(1) == pytest.approx(350.0)

    def test_qi_attribute(self, network_with_couplings):
        entity = AttributeEntity(network_with_couplings, "QI", 1)
        assert entity.get_value() == pytest.approx(100.0)

    def test_string_representation(self, network_with_couplings):
        entity = AttributeEntity(network_with_couplings, "T", 1)
        rep = entity.string_representation()
        assert isinstance(rep, str)
        assert len(rep) > 0

    def test_type_property(self, network_with_couplings):
        entity = AttributeEntity(network_with_couplings, "T", 1)
        assert isinstance(entity.type, str)

    def test_value_pointer(self, network_with_couplings):
        entity = AttributeEntity(network_with_couplings, "T", 1)
        ptr = entity.get_value_pointer()
        assert ptr != 0

    def test_clone(self, network_with_couplings):
        entity = AttributeEntity(network_with_couplings, "T", 1)
        cloned = entity.clone()
        assert cloned.get_value() == pytest.approx(entity.get_value())


class TestConductiveCouplingEntity:
    def test_construction(self, network_with_couplings):
        entity = ConductiveCouplingEntity(network_with_couplings, 1, 2)
        assert entity is not None

    def test_get_value(self, network_with_couplings):
        entity = ConductiveCouplingEntity(network_with_couplings, 1, 2)
        assert entity.get_value() == pytest.approx(10.0)

    def test_set_value(self, network_with_couplings):
        entity = ConductiveCouplingEntity(network_with_couplings, 1, 2)
        entity.set_value(20.0)
        assert entity.get_value() == pytest.approx(20.0)
        assert network_with_couplings.conductive_couplings.get_coupling_value(
            1, 2
        ) == pytest.approx(20.0)

    def test_string_representation(self, network_with_couplings):
        entity = ConductiveCouplingEntity(network_with_couplings, 1, 2)
        rep = entity.string_representation()
        assert isinstance(rep, str)


class TestRadiativeCouplingEntity:
    def test_construction(self, network_with_couplings):
        entity = RadiativeCouplingEntity(network_with_couplings, 1, 2)
        assert entity is not None

    def test_get_value(self, network_with_couplings):
        entity = RadiativeCouplingEntity(network_with_couplings, 1, 2)
        assert entity.get_value() == pytest.approx(0.5)

    def test_set_value(self, network_with_couplings):
        entity = RadiativeCouplingEntity(network_with_couplings, 1, 2)
        entity.set_value(0.9)
        assert entity.get_value() == pytest.approx(0.9)

    def test_string_representation(self, network_with_couplings):
        entity = RadiativeCouplingEntity(network_with_couplings, 1, 2)
        rep = entity.string_representation()
        assert isinstance(rep, str)
