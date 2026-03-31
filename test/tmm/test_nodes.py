"""Test bindings for Nodes container."""

import pytest

import pycanha_core as pcc

Node = pcc.tmm.Node
Nodes = pcc.tmm.Nodes
NodeType = pcc.tmm.NodeType


class TestNodesContainer:
    def test_empty_construction(self):
        nodes = Nodes()
        assert nodes.num_nodes == 0
        assert nodes.num_diff_nodes == 0
        assert nodes.num_bound_nodes == 0

    def test_add_single_node(self):
        nodes = Nodes()
        node = Node(101)
        node.T = 273.15
        node.C = 42.0
        nodes.add_node(node)

        assert nodes.num_nodes == 1
        assert nodes.get_T(101) == pytest.approx(273.15)
        assert nodes.get_C(101) == pytest.approx(42.0)

    def test_is_node(self):
        nodes = Nodes()
        nodes.add_node(Node(5))
        assert nodes.is_node(5) is True
        assert nodes.is_node(99) is False

    def test_remove_node(self):
        nodes = Nodes()
        nodes.add_node(Node(1))
        nodes.add_node(Node(2))
        assert nodes.num_nodes == 2
        nodes.remove_node(1)
        assert nodes.num_nodes == 1
        assert nodes.is_node(1) is False
        assert nodes.is_node(2) is True

    def test_diffusive_before_boundary(self):
        """Diffusive nodes should be sorted before boundary nodes."""
        nodes = Nodes()
        diff = Node(10)
        diff.type = NodeType.DIFFUSIVE

        bound = Node(5)
        bound.type = NodeType.BOUNDARY

        nodes.add_node(bound)
        nodes.add_node(diff)

        assert nodes.num_diff_nodes == 1
        assert nodes.num_bound_nodes == 1
        # Diffusive at index 0, boundary at index 1
        assert nodes.get_node_num_from_idx(0) == 10
        assert nodes.get_node_num_from_idx(1) == 5

    def test_index_mapping(self):
        nodes = Nodes()
        nodes.add_node(Node(100))
        nodes.add_node(Node(200))

        idx_100 = nodes.get_idx_from_node_num(100)
        idx_200 = nodes.get_idx_from_node_num(200)
        assert nodes.get_node_num_from_idx(idx_100) == 100
        assert nodes.get_node_num_from_idx(idx_200) == 200

    def test_type_get_set(self):
        nodes = Nodes()
        node = Node(1)
        nodes.add_node(node)
        assert nodes.get_type(1) == NodeType.DIFFUSIVE
        nodes.set_type(1, NodeType.BOUNDARY)
        assert nodes.get_type(1) == NodeType.BOUNDARY

    @pytest.mark.parametrize(
        "getter,setter",
        [
            ("get_T", "set_T"),
            ("get_C", "set_C"),
            ("get_qs", "set_qs"),
            ("get_qa", "set_qa"),
            ("get_qe", "set_qe"),
            ("get_qi", "set_qi"),
            ("get_qr", "set_qr"),
            ("get_a", "set_a"),
            ("get_fx", "set_fx"),
            ("get_fy", "set_fy"),
            ("get_fz", "set_fz"),
            ("get_eps", "set_eps"),
            ("get_aph", "set_aph"),
        ],
    )
    def test_attribute_getters_setters(self, getter, setter):
        nodes = Nodes()
        nodes.add_node(Node(1))
        getattr(nodes, setter)(1, 77.7)
        assert getattr(nodes, getter)(1) == pytest.approx(77.7)

    def test_literal_C_get_set(self):
        nodes = Nodes()
        nodes.add_node(Node(1))
        nodes.set_literal_C(1, 55.0)
        assert nodes.get_literal_C(1) == pytest.approx(55.0)

    def test_get_node_from_node_num(self):
        nodes = Nodes()
        n = Node(42)
        n.T = 500.0
        nodes.add_node(n)
        retrieved = nodes.get_node_from_node_num(42)
        assert retrieved.node_num == 42
        assert retrieved.T == pytest.approx(500.0)

    def test_get_node_from_idx(self):
        nodes = Nodes()
        n = Node(7)
        n.T = 123.0
        nodes.add_node(n)
        idx = nodes.get_idx_from_node_num(7)
        retrieved = nodes.get_node_from_idx(idx)
        assert retrieved.node_num == 7

    def test_value_pointers_are_nonzero(self):
        nodes = Nodes()
        nodes.add_node(Node(1))
        assert nodes.get_T_value_pointer(1) != 0
        assert nodes.get_C_value_pointer(1) != 0
        assert nodes.get_qi_value_pointer(1) != 0

    def test_estimated_number_of_nodes(self):
        nodes = Nodes()
        nodes.estimated_number_of_nodes = 100
        assert nodes.estimated_number_of_nodes == 100

    def test_is_mapped(self):
        nodes = Nodes()
        # Should be callable and return a bool
        result = nodes.is_mapped()
        assert isinstance(result, bool)
