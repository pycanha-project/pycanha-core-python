"""Tests for the 0.18 gmm -> tmm conduction builder."""

import numpy as np
import pytest

import pycanha_core as pcc

cond = pcc.conduction
gmm = pcc.gmm


def _unit_bulk():
    """Conductivity 1 W/(m K), so a conductance reads as the shape factor."""
    return gmm.BulkMaterial("unit", 1.0, 1.0, 1.0)


def _unit_square():
    return gmm.Rectangle(
        np.array([0.0, 0.0, 0.0]),
        np.array([1.0, 0.0, 0.0]),
        np.array([0.0, 1.0, 0.0]),
    )


def _side1_shell(dir1, dir2, start, step):
    mesh = gmm.ThermalMesh(dir1, dir2)
    mesh.side1_material = _unit_bulk()
    mesh.side1_thick = 1.0
    mesh.node1_start = start
    mesh.node1_step = step
    mesh.conductive_active_side = gmm.ActiveSide.SIDE1
    return mesh


def _split_plate_model(name="split_plate"):
    """One 1x1 plate cut in two along direction 1: nodes 100 and 101."""
    model = pcc.tmm.ThermalModel(name)
    mesh = _side1_shell([0.0, 0.5, 1.0], [0.0, 1.0], 100, 1)
    model.gmm.add(gmm.GeometryItem("plate", _unit_square(), mesh))
    return model


class TestBuild:
    def test_split_plate_produces_one_node_pair_and_one_conductor(self):
        model = _split_plate_model()
        report = model.build_tmm_from_gmm()

        assert report.items_processed == 1
        assert report.nodes_created == 2
        assert report.conductors_created == 1
        assert report.cell_links_computed == 1
        assert model.tmm.nodes.num_nodes == 2
        # 1 m shared edge over a 0.5 m distance between the two references.
        assert model.tmm.conductive_couplings.get_coupling_value(
            100, 101
        ) == pytest.approx(2.0)

    def test_free_function_matches_the_method(self):
        model = _split_plate_model()
        report = cond.build_tmm_from_gmm(model)
        assert report.nodes_created == 2
        assert model.tmm.conductive_couplings.get_coupling_value(
            100, 101
        ) == pytest.approx(2.0)

    def test_initial_temperature_reaches_the_nodes(self):
        model = _split_plate_model()
        model.build_tmm_from_gmm(cond.TmmBuildOptions(initial_temperature=250.0))
        assert model.tmm.nodes.get_T(100) == pytest.approx(250.0)

    def test_intra_primitive_conductors_can_be_switched_off(self):
        model = _split_plate_model()
        report = model.build_tmm_from_gmm(
            cond.TmmBuildOptions(intra_primitive_conductors=False)
        )
        assert report.nodes_created == 2
        assert report.conductors_created == 0

    def test_min_conductance_drops_the_conductor(self):
        model = _split_plate_model()
        options = cond.TmmBuildOptions()
        options.min_conductance = 10.0
        report = model.build_tmm_from_gmm(options)
        assert report.nodes_created == 2
        assert report.conductors_created == 0

    def test_a_second_build_is_rejected(self):
        model = _split_plate_model()
        model.build_tmm_from_gmm()
        # No merge semantics: the tmm must be empty.
        with pytest.raises((ValueError, RuntimeError)):
            model.build_tmm_from_gmm()


class TestDiagnostics:
    def test_a_side_without_bulk_material_is_reported(self):
        model = pcc.tmm.ThermalModel("no_bulk")
        mesh = gmm.ThermalMesh([0.0, 0.5, 1.0], [0.0, 1.0])
        mesh.node1_start = 100
        mesh.node1_step = 1
        mesh.conductive_active_side = gmm.ActiveSide.SIDE1
        model.gmm.add(gmm.GeometryItem("plate", _unit_square(), mesh))

        report = model.build_tmm_from_gmm()
        codes = [d.code for d in report.diagnostics]
        assert cond.DiagnosticCode.MissingBulk in codes
        # The nodes still exist, they just carry no capacitance.
        assert report.nodes_created == 2
        assert report.conductors_created == 0

    def test_diagnostic_carries_the_geometry_name(self):
        model = pcc.tmm.ThermalModel("named")
        mesh = gmm.ThermalMesh([0.0, 0.5, 1.0], [0.0, 1.0])
        mesh.node1_start = 100
        mesh.node1_step = 1
        mesh.conductive_active_side = gmm.ActiveSide.SIDE1
        model.gmm.add(gmm.GeometryItem("panel", _unit_square(), mesh))

        report = model.build_tmm_from_gmm()
        assert report.diagnostics
        assert report.diagnostics[0].geometry_name == "panel"
        assert report.diagnostics[0].message

    def test_diagnostic_code_name(self):
        assert (
            cond.diagnostic_code_name(cond.DiagnosticCode.AxisSingularity)
            == "AxisSingularity"
        )


class TestOptions:
    def test_defaults(self):
        options = cond.TmmBuildOptions()
        assert options.initial_temperature == pytest.approx(0.0)
        assert options.intra_primitive_conductors is True
        assert options.through_thickness_conductors is True
        assert options.close_full_revolution is True
        assert options.min_conductance == pytest.approx(0.0)

    def test_keyword_constructor_and_setters(self):
        options = cond.TmmBuildOptions(
            initial_temperature=300.0, close_full_revolution=False
        )
        assert options.initial_temperature == pytest.approx(300.0)
        assert options.close_full_revolution is False
        options.min_conductance = 1.0e-9
        assert options.min_conductance == pytest.approx(1.0e-9)


class TestLinks:
    def test_intra_primitive_links_of_a_split_plate(self):
        mesh = _side1_shell([0.0, 0.5, 1.0], [0.0, 1.0], 100, 1)
        links = cond.intra_primitive_links(_unit_square(), mesh)
        assert len(links) == 1
        assert links[0].side == 1
        assert {links[0].cell_a, links[0].cell_b} == {0, 1}
        assert links[0].conductance == pytest.approx(2.0)

    def test_an_inactive_side_produces_no_links(self):
        mesh = _side1_shell([0.0, 0.5, 1.0], [0.0, 1.0], 100, 1)
        mesh.conductive_active_side = gmm.ActiveSide.NONE
        assert cond.intra_primitive_links(_unit_square(), mesh) == []

    def test_through_thickness_is_two_half_slabs_in_series(self):
        mesh = gmm.ThermalMesh()
        mesh.side1_material = _unit_bulk()
        mesh.side2_material = _unit_bulk()
        mesh.side1_thick = 1.0
        mesh.side2_thick = 1.0
        # A / (t1/k1 + t2/k2) = 1 / 2.
        assert cond.through_thickness_conductance(mesh, 1.0) == pytest.approx(0.5)

    def test_through_thickness_needs_both_sides_conductive(self):
        mesh = gmm.ThermalMesh()
        mesh.side1_material = _unit_bulk()
        mesh.side2_material = _unit_bulk()
        mesh.side1_thick = 1.0
        mesh.side2_thick = 1.0
        mesh.conductive_active_side = gmm.ActiveSide.SIDE1
        assert cond.through_thickness_conductance(mesh, 1.0) == pytest.approx(0.0)


class TestMeridianProfile:
    def test_a_rectangle_is_planar(self):
        profile = cond.profile_of(_unit_square())
        assert profile is not None
        assert profile.kind == cond.MeridianProfile.Kind.Planar
        assert profile.closes_ring is False
        # Planar is the degenerate rho == 1 case, where dir1 is a length.
        assert profile.rho(0.5) == pytest.approx(1.0)
        assert profile.dir1_extent == pytest.approx(1.0)
        assert profile.on_axis(0.0) is False

    def test_a_full_cylinder_closes_its_ring(self):
        cylinder = gmm.Cylinder(
            np.array([0.0, 0.0, 0.0]),
            np.array([0.0, 0.0, 2.0]),
            np.array([1.0, 0.0, 0.0]),
            radius=1.0,
            start_angle=0.0,
            end_angle=2.0 * np.pi,
        )
        profile = cond.profile_of(cylinder)
        assert profile is not None
        assert profile.kind == cond.MeridianProfile.Kind.Cylinder
        assert profile.closes_ring is True
        # dir1 is the angle in radians for a surface of revolution.
        assert profile.dir1_extent == pytest.approx(2.0 * np.pi)
        assert profile.rho(0.5) == pytest.approx(1.0)
        # Phi spans the full height of the wall at radius 1.
        assert profile.potential(1.0) - profile.potential(0.0) == pytest.approx(2.0)
        assert profile.meridian_length(0.0, 1.0) == pytest.approx(2.0)

    def test_a_triangle_has_no_closed_form(self):
        triangle = gmm.Triangle(
            np.array([0.0, 0.0, 0.0]),
            np.array([1.0, 0.0, 0.0]),
            np.array([0.0, 1.0, 0.0]),
        )
        assert cond.profile_of(triangle) is None
