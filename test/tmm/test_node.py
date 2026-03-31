"""Test bindings for Node and NodeType."""

import pytest

import pycanha_core as pcc

Node = pcc.tmm.Node
NodeType = pcc.tmm.NodeType


class TestNodeType:
    def test_enum_values_exist(self):
        assert NodeType.DIFFUSIVE is not None
        assert NodeType.BOUNDARY is not None

    def test_enum_values_distinct(self):
        assert NodeType.DIFFUSIVE != NodeType.BOUNDARY


class TestNode:
    def test_construction(self):
        node = Node(1)
        assert node.node_num == 1

    def test_copy_construction(self):
        original = Node(10)
        original.T = 300.0
        original.C = 100.0
        copy = Node(original)
        assert copy.node_num == 10
        assert copy.T == pytest.approx(300.0)
        assert copy.C == pytest.approx(100.0)

    def test_node_num_setter(self):
        node = Node(1)
        node.node_num = 42
        assert node.node_num == 42

    def test_int_node_num_unassociated(self):
        # TODO: int_node_num() returns -1 for unassociated nodes instead of node_num
        node = Node(7)
        assert node.int_node_num() == -1

    def test_default_type_is_diffusive(self):
        node = Node(1)
        assert node.type == NodeType.DIFFUSIVE

    def test_type_setter(self):
        node = Node(1)
        node.type = NodeType.BOUNDARY
        assert node.type == NodeType.BOUNDARY

    def test_temperature_roundtrip(self):
        node = Node(1)
        node.T = 350.5
        assert node.T == pytest.approx(350.5)

    def test_capacity_roundtrip(self):
        node = Node(1)
        node.C = 1000.0
        assert node.C == pytest.approx(1000.0)

    def test_capacity_alias(self):
        node = Node(1)
        node.capacity = 500.0
        assert node.C == pytest.approx(500.0)
        assert node.capacity == pytest.approx(500.0)

    @pytest.mark.parametrize(
        "attr",
        ["qs", "qa", "qe", "qi", "qr", "a", "fx", "fy", "fz", "eps", "aph"],
    )
    def test_load_attributes_roundtrip(self, attr):
        node = Node(1)
        setattr(node, attr, 42.5)
        assert getattr(node, attr) == pytest.approx(42.5)

    def test_literal_C_roundtrip(self):
        # TODO: literal_C accepts only str, not float
        node = Node(1)
        node.literal_C = "99.0"
        assert node.literal_C == "99.0"

    def test_parent_pointer_unassociated(self):
        node = Node(1)
        assert node.parent_pointer() is None

    def test_parent_pointer_address_unassociated(self):
        node = Node(1)
        assert node.parent_pointer_address() == 0
