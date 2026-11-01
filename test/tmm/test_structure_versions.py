"""Structure versions of the nodes and coupling patterns."""

import pycanha_core as pcc


def test_snapshot_of_a_model(basic_tmm):
    before = basic_tmm.structure_versions()
    assert isinstance(before, pcc.tmm.StructureVersions)
    assert before == basic_tmm.structure_versions()
    assert before == basic_tmm.network.structure_versions()
    assert before != "not a snapshot"


def test_a_new_coupling_changes_its_pattern_only(basic_tmm):
    before = basic_tmm.structure_versions()
    conductive = basic_tmm.conductive_couplings.matrices().structure_version
    basic_tmm.add_conductive_coupling(10, 20, 0.1)
    after = basic_tmm.structure_versions()
    assert after != before
    assert after.conductive != before.conductive
    assert after.radiative == before.radiative
    assert after.nodes == before.nodes
    assert basic_tmm.conductive_couplings.matrices().structure_version != conductive


def test_a_new_value_changes_nothing(basic_tmm):
    before = basic_tmm.structure_versions()
    basic_tmm.conductive_couplings.set_coupling_value(10, 15, 0.5)
    basic_tmm.nodes.set_qi(10, 1.0)
    assert basic_tmm.structure_versions() == before


def test_a_new_node_changes_the_nodes(basic_tmm):
    before = basic_tmm.structure_versions()
    basic_tmm.add_node(pcc.tmm.Node(30))
    assert basic_tmm.structure_versions().nodes != before.nodes
