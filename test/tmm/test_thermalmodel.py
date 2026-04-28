"""Test the v0.14 ThermalModel owner workflow."""

import pycanha_core as pcc
import pytest


class TestThermalModel:
    def test_default_owner_exposes_stable_subsystems(self):
        tm = pcc.tmm.ThermalModel("overview_root")

        assert tm.name == "overview_root"
        assert tm.tmm is tm.tmm
        assert tm.gmm is tm.gmm
        assert tm.solvers is tm.solvers
        assert tm.callbacks is tm.callbacks
        assert tm.solvers.sslu is tm.solvers.sslu

        tm.parameters.add_parameter("k", 10.0)
        assert tm.tmm.parameters.get_parameter("k") == pytest.approx(10.0)
        assert tm.solvers.tmm is tm.tmm

    def test_explicit_constructor_keeps_existing_objects(self):
        tmm = pcc.tmm.ThermalMathematicalModel("explicit_model")
        gmm = pcc.gmm.GeometryModel("explicit_geometry")

        tm = pcc.tmm.ThermalModel("wrapper_root", tmm, gmm)

        assert tm.tmm is tmm
        assert tm.gmm is gmm
        assert tm.solvers.tmm is tmm

    def test_entities_and_string_lookup_roundtrip(self, basic_tm):
        tmm = basic_tm.tmm

        assert tmm.entities.temperature(10).string_representation() == "T10"
        assert tmm.entities.capacity(10).string_representation() == "C10"
        assert tmm.entities.internal_heat(15).string_representation() == "QI15"
        assert (
            tmm.entities.conductive_coupling(15, 10).string_representation()
            == "GL(10,15)"
        )

        assert tmm.find_entity("QI15") is not None
        assert tmm.entity("QI15").string_representation() == "QI15"
        assert tmm.find_entity("not_an_entity") is None

        with pytest.raises(ValueError, match="Unknown thermal entity"):
            tmm.entity("not_an_entity")

    def test_flow_helpers_cover_pair_and_group_overloads(self):
        tmm = pcc.tmm.ThermalMathematicalModel("flow_demo")

        hot = pcc.tmm.Node(1)
        hot.T = 300.0
        cold = pcc.tmm.Node(2)
        cold.T = 280.0
        tmm.add_node(hot)
        tmm.add_node(cold)
        tmm.add_conductive_coupling(1, 2, 0.5)
        tmm.add_radiative_coupling(1, 2, 0.0)

        assert tmm.flow_conductive(1, 2) == pytest.approx(-10.0)
        assert tmm.flow_conductive([1], [2]) == pytest.approx(-10.0)
        assert tmm.flow_radiative(1, 2) == pytest.approx(0.0)
        assert tmm.flow_radiative([1], [2]) == pytest.approx(0.0)
