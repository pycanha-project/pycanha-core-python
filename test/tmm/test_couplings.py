"""Test bindings for Couplings, ConductiveCouplings, RadiativeCouplings."""

import pytest

import pycanha_core as pcc

Coupling = pcc.tmm.Coupling
ConductiveCouplings = pcc.tmm.ConductiveCouplings
Couplings = pcc.tmm.Couplings
Node = pcc.tmm.Node
Nodes = pcc.tmm.Nodes
RadiativeCouplings = pcc.tmm.RadiativeCouplings


@pytest.fixture
def two_node_network():
    """Return a Nodes container with two diffusive nodes (1, 2)."""
    nodes = Nodes()
    nodes.add_node(Node(1))
    nodes.add_node(Node(2))
    return nodes


# ---------------------------------------------------------------------------
# Couplings (generic)
# ---------------------------------------------------------------------------
class TestCouplings:
    def test_add_and_get_value(self, two_node_network):
        couplings = Couplings(two_node_network)
        couplings.add_coupling(1, 2, 15.0)
        assert couplings.get_coupling_value(1, 2) == pytest.approx(15.0)

    def test_coupling_exists(self, two_node_network):
        couplings = Couplings(two_node_network)
        couplings.add_coupling(1, 2, 5.0)
        assert couplings.coupling_exists(1, 2) is True
        assert couplings.coupling_exists(2, 1) is False

    def test_set_coupling_value(self, two_node_network):
        couplings = Couplings(two_node_network)
        couplings.add_coupling(1, 2, 5.0)
        couplings.set_coupling_value(1, 2, 20.0)
        assert couplings.get_coupling_value(1, 2) == pytest.approx(20.0)

    def test_add_coupling_from_object(self, two_node_network):
        couplings = Couplings(two_node_network)
        c = Coupling(1, 2, 33.0)
        couplings.add_coupling(c)
        assert couplings.get_coupling_value(1, 2) == pytest.approx(33.0)

    def test_add_ovw_coupling(self, two_node_network):
        couplings = Couplings(two_node_network)
        couplings.add_ovw_coupling(1, 2, 10.0)
        couplings.add_ovw_coupling(1, 2, 20.0)  # overwrites
        assert couplings.get_coupling_value(1, 2) == pytest.approx(20.0)

    def test_add_sum_coupling(self, two_node_network):
        couplings = Couplings(two_node_network)
        couplings.add_sum_coupling(1, 2, 10.0)
        couplings.add_sum_coupling(1, 2, 5.0)  # sums
        assert couplings.get_coupling_value(1, 2) == pytest.approx(15.0)

    def test_value_pointer_nonzero(self, two_node_network):
        couplings = Couplings(two_node_network)
        couplings.add_coupling(1, 2, 5.0)
        ptr = couplings.get_coupling_value_pointer(1, 2)
        assert ptr != 0

    def test_value_address_nonzero(self, two_node_network):
        couplings = Couplings(two_node_network)
        couplings.add_coupling(1, 2, 5.0)
        addr = couplings.get_coupling_value_address(1, 2)
        assert addr != 0

    def test_get_coupling_from_idx(self, two_node_network):
        couplings = Couplings(two_node_network)
        couplings.add_coupling(1, 2, 77.0)
        c = couplings.get_coupling_from_coupling_idx(0)
        assert isinstance(c, Coupling)
        assert c.value == pytest.approx(77.0)

    def test_get_coupling_matrices(self, two_node_network):
        couplings = Couplings(two_node_network)
        couplings.add_coupling(1, 2, 5.0)
        matrices = couplings.get_coupling_matrices()
        assert matrices.num_total_couplings >= 1


# ---------------------------------------------------------------------------
# ConductiveCouplings
# ---------------------------------------------------------------------------
class TestConductiveCouplings:
    def test_add_and_get(self, two_node_network):
        cc = ConductiveCouplings(two_node_network)
        cc.add_coupling(1, 2, 10.0)
        assert cc.get_coupling_value(1, 2) == pytest.approx(10.0)

    def test_set_coupling_value(self, two_node_network):
        cc = ConductiveCouplings(two_node_network)
        cc.add_coupling(1, 2, 10.0)
        cc.set_coupling_value(1, 2, 25.0)
        assert cc.get_coupling_value(1, 2) == pytest.approx(25.0)

    def test_add_from_coupling_object(self, two_node_network):
        cc = ConductiveCouplings(two_node_network)
        cc.add_coupling(Coupling(1, 2, 7.5))
        assert cc.get_coupling_value(1, 2) == pytest.approx(7.5)


# ---------------------------------------------------------------------------
# RadiativeCouplings
# ---------------------------------------------------------------------------
class TestRadiativeCouplings:
    def test_add_and_get(self, two_node_network):
        rc = RadiativeCouplings(two_node_network)
        rc.add_coupling(1, 2, 0.5)
        assert rc.get_coupling_value(1, 2) == pytest.approx(0.5)

    def test_set_coupling_value(self, two_node_network):
        rc = RadiativeCouplings(two_node_network)
        rc.add_coupling(1, 2, 0.5)
        rc.set_coupling_value(1, 2, 0.8)
        assert rc.get_coupling_value(1, 2) == pytest.approx(0.8)

    def test_add_from_coupling_object(self, two_node_network):
        rc = RadiativeCouplings(two_node_network)
        rc.add_coupling(Coupling(1, 2, 0.3))
        assert rc.get_coupling_value(1, 2) == pytest.approx(0.3)


# ---------------------------------------------------------------------------
# CouplingMatrices
# ---------------------------------------------------------------------------
class TestCouplingMatrices:
    def test_properties(self, two_node_network):
        couplings = Couplings(two_node_network)
        couplings.add_coupling(1, 2, 5.0)
        cm = couplings.get_coupling_matrices()

        assert cm.num_nodes == 2
        assert cm.num_diff_nodes == 2
        assert cm.num_bound_nodes == 0
        assert cm.num_total_couplings >= 1

    def test_sparse_copies(self, two_node_network):
        couplings = Couplings(two_node_network)
        couplings.add_coupling(1, 2, 5.0)
        cm = couplings.get_coupling_matrices()

        dd = cm.sparse_dd_copy()
        db = cm.sparse_db_copy()
        bb = cm.sparse_bb_copy()
        assert dd is not None
        assert db is not None
        assert bb is not None

    def test_coupling_exists_from_idxs(self, two_node_network):
        couplings = Couplings(two_node_network)
        couplings.add_coupling(1, 2, 5.0)
        cm = couplings.get_coupling_matrices()

        idx1 = two_node_network.get_idx_from_node_num(1)
        idx2 = two_node_network.get_idx_from_node_num(2)
        assert cm.coupling_exists_from_idxs(idx1, idx2) is True

    def test_get_set_conductor_value_from_idx(self, two_node_network):
        couplings = Couplings(two_node_network)
        couplings.add_coupling(1, 2, 5.0)
        cm = couplings.get_coupling_matrices()

        idx1 = two_node_network.get_idx_from_node_num(1)
        idx2 = two_node_network.get_idx_from_node_num(2)
        assert cm.get_conductor_value_from_idx(idx1, idx2) == pytest.approx(5.0)

        cm.set_conductor_value_from_idx(idx1, idx2, 12.0)
        assert cm.get_conductor_value_from_idx(idx1, idx2) == pytest.approx(12.0)

    def test_get_idxs_and_value_from_coupling_idx(self, two_node_network):
        couplings = Couplings(two_node_network)
        couplings.add_coupling(1, 2, 5.0)
        cm = couplings.get_coupling_matrices()

        result = cm.get_idxs_and_coupling_value_from_coupling_idx(0)
        assert len(result) == 3  # (idx1, idx2, value)
        assert result[2] == pytest.approx(5.0)
