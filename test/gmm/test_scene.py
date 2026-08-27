"""Tests for the 0.15 scene tree, GeometryModel, meshing and TriMesh views."""

import numpy as np
import pytest

import pycanha_core as pcc

gmm = pcc.gmm


def _rectangle():
    return gmm.Rectangle(
        np.array([0.0, 0.0, 0.0]),
        np.array([2.0, 0.0, 0.0]),
        np.array([0.0, 1.0, 0.0]),
    )


def _cylinder_cutter():
    return gmm.Cylinder(
        np.array([0.0, 0.0, -1.0]),
        np.array([0.0, 0.0, 1.0]),
        np.array([0.35, 0.0, -1.0]),
        radius=0.35,
        start_angle=0.0,
        end_angle=2.0 * np.pi,
    )


class TestGeometryItem:
    def test_construction_and_props(self):
        item = gmm.GeometryItem("panel", _rectangle(), gmm.ThermalMesh())
        assert item.name == "panel"
        assert isinstance(item.primitive, gmm.Rectangle)
        assert isinstance(item.thermal_mesh, gmm.ThermalMesh)
        assert item.children == []
        assert item.mesh_options_override is None

    def test_mesh_options_override(self):
        item = gmm.GeometryItem("panel", _rectangle(), gmm.ThermalMesh())
        item.mesh_options_override = gmm.MeshOptions(deviation_tolerance=5.0e-4)
        assert item.mesh_options_override is not None
        item.mesh_options_override = None
        assert item.mesh_options_override is None

    def test_item_mesh_is_trimeshd(self):
        item = gmm.GeometryItem("panel", _rectangle(), gmm.ThermalMesh())
        mesh = item.mesh
        assert isinstance(mesh, gmm.TriMeshD)
        assert mesh.vertices.shape[1] == 3
        assert mesh.vertices.dtype == np.float64
        assert mesh.triangles.shape[1] == 3
        assert mesh.nt() == mesh.triangles.shape[0]


class TestGeometryGroup:
    def test_add_children(self):
        strut = gmm.GeometryItem("strut", _rectangle(), gmm.ThermalMesh())
        brace = gmm.GeometryItem("brace", _rectangle(), gmm.ThermalMesh())
        group = gmm.GeometryGroup("rig")
        group.add(strut)
        group.add(brace)
        assert len(group.children) == 2

    def test_add_rejects_duplicate(self):
        strut = gmm.GeometryItem("strut", _rectangle(), gmm.ThermalMesh())
        group = gmm.GeometryGroup("rig")
        group.add(strut)
        with pytest.raises((ValueError, RuntimeError)):
            group.add(strut)


class TestGeometryModel:
    def test_add_and_lookup(self):
        model = gmm.GeometryModel("demo")
        item = gmm.GeometryItem("panel", _rectangle(), gmm.ThermalMesh())
        model.add(item)
        assert model.contains("panel")
        assert model.contains(item)
        assert model.get_item("panel") is not None
        assert model.get_group("panel") is None
        assert len(model.children) == 1

    def test_model_mesh_is_float32(self):
        model = gmm.GeometryModel("demo")
        model.add(gmm.GeometryItem("panel", _rectangle(), gmm.ThermalMesh()))
        model.create_mesh()
        mesh = model.mesh
        assert isinstance(mesh, gmm.TriMeshF)
        assert mesh.vertices.dtype == np.float32
        assert mesh.vertices.shape[1] == 3
        assert mesh.triangles.shape[0] == mesh.nt()
        assert mesh.face_ids.shape[0] == mesh.triangles.shape[0]

    def test_trimesh_views_are_readonly(self):
        model = gmm.GeometryModel("demo")
        model.add(gmm.GeometryItem("panel", _rectangle(), gmm.ThermalMesh()))
        model.create_mesh()
        verts = np.asarray(model.mesh.vertices)
        assert verts.flags.writeable is False

    def test_node_numbers_from_thermalmesh(self):
        tmesh = gmm.ThermalMesh()
        tmesh.node1_start = 100
        tmesh.node1_step = 0
        model = gmm.GeometryModel("demo")
        model.add(gmm.GeometryItem("panel", _rectangle(), tmesh))
        model.create_mesh()
        node_numbers = np.asarray(model.mesh.node_numbers)
        assert (node_numbers == 100).any()
        faces = model.faces_of_node(100)
        assert len(faces) >= 1


class TestUvMesherAndOps:
    def test_uv_mesher(self):
        mesher = gmm.UvMesher()
        mesh = mesher.mesh(_rectangle(), gmm.ThermalMesh(), gmm.MeshOptions())
        assert isinstance(mesh, gmm.TriMeshD)
        assert mesh.np() == mesh.vertices.shape[0]

    def test_mesh_ops_area(self):
        item = gmm.GeometryItem("panel", _rectangle(), gmm.ThermalMesh())
        areas = gmm.compute_areas(item.mesh)
        # Rectangle surface area is 2.0 (2 x 1); the triangle areas sum to it.
        assert float(np.sum(areas)) == pytest.approx(2.0, rel=1e-6)

    def test_mesh_ops_bounding_box(self):
        item = gmm.GeometryItem("panel", _rectangle(), gmm.ThermalMesh())
        bbox_min, bbox_max = gmm.bounding_box(item.mesh)
        np.testing.assert_allclose(bbox_min, [0.0, 0.0, 0.0], atol=1e-9)
        np.testing.assert_allclose(bbox_max, [2.0, 1.0, 0.0], atol=1e-9)


class TestCutGroup:
    def test_is_closed_solid(self):
        assert gmm.is_closed_solid(_cylinder_cutter()) is True
        assert gmm.is_closed_solid(_rectangle()) is False

    def test_cut_group_api(self):
        target = gmm.GeometryItem("panel", _rectangle(), gmm.ThermalMesh())
        cutter = gmm.GeometryItem("cutter", _cylinder_cutter(), gmm.ThermalMesh())
        cut = gmm.GeometryGroupCutted("panel_trimmed", [target], [cutter])
        assert len(cut.targets) == 1
        assert len(cut.cutters) == 1

    def test_cut_group_rejects_open_cutter(self):
        target = gmm.GeometryItem("panel", _rectangle(), gmm.ThermalMesh())
        open_cutter = gmm.GeometryItem("bad", _rectangle(), gmm.ThermalMesh())
        with pytest.raises((ValueError, RuntimeError)):
            gmm.GeometryGroupCutted("bad_cut", [target], [open_cutter])


def _plate():
    """A 4 m x 1 m plate along +x, one face pair."""
    return gmm.Rectangle(
        np.array([0.0, 0.0, 0.0]),
        np.array([4.0, 0.0, 0.0]),
        np.array([0.0, 1.0, 0.0]),
    )


def _box_cutter(x_center):
    """A box cutting clean through the plate, 1 m wide in x: removes 1 m2."""
    return gmm.Cube(
        np.array([x_center, 0.5, 0.0]),
        np.array([1.0, 2.0, 2.0]),
    )


def _mesh_area(node):
    mesh = node.mesh
    vertices = np.asarray(mesh.vertices)
    triangles = np.asarray(mesh.triangles)
    p0 = vertices[triangles[:, 0]]
    p1 = vertices[triangles[:, 1]]
    p2 = vertices[triangles[:, 2]]
    return float(0.5 * np.linalg.norm(np.cross(p1 - p0, p2 - p0), axis=1).sum())


class TestCutResolution:
    """Cuts resolve top-down from a resolution root, per primitive, at once."""

    def test_chained_cuts_resolve_together(self):
        plate = gmm.GeometryItem("plate", _plate(), gmm.ThermalMesh())
        inner = gmm.GeometryGroupCutted(
            "inner", [plate], [gmm.GeometryItem("c1", _box_cutter(0.5), gmm.ThermalMesh())]
        )
        outer = gmm.GeometryGroupCutted(
            "outer", [inner], [gmm.GeometryItem("c2", _box_cutter(2.5), gmm.ThermalMesh())]
        )
        # A cut group may now hold another cut group: the chain is one cut of
        # the original primitive, not a cut of a cut (which is impossible --
        # a cut result has no primitive to classify triangles against).
        assert _mesh_area(outer) == pytest.approx(2.0, abs=1e-6)

    def test_only_cutters_inside_the_subtree_apply(self):
        plate = gmm.GeometryItem("plate", _plate(), gmm.ThermalMesh())
        inner = gmm.GeometryGroupCutted(
            "inner", [plate], [gmm.GeometryItem("c1", _box_cutter(0.5), gmm.ThermalMesh())]
        )
        outer = gmm.GeometryGroupCutted(
            "outer", [inner], [gmm.GeometryItem("c2", _box_cutter(2.5), gmm.ThermalMesh())]
        )
        # node.mesh resolves the subtree with node as the root, so the outer
        # cutter is out of scope at `inner`.
        assert _mesh_area(inner) == pytest.approx(3.0, abs=1e-6)
        assert _mesh_area(outer) == pytest.approx(2.0, abs=1e-6)

    def test_cut_group_can_hold_a_group(self):
        left = gmm.GeometryItem("left", _plate(), gmm.ThermalMesh())
        group = gmm.GeometryGroup("both", [left])
        cut = gmm.GeometryGroupCutted(
            "cut", [group], [gmm.GeometryItem("c", _box_cutter(0.5), gmm.ThermalMesh())]
        )
        assert _mesh_area(cut) == pytest.approx(3.0, abs=1e-6)

    def test_a_cut_never_removes_faces(self):
        plate = gmm.GeometryItem("plate", _plate(), gmm.ThermalMesh())
        # Covers the whole plate, so every triangle goes; the face pair stays.
        cutter = gmm.GeometryItem(
            "c",
            gmm.Cube(np.array([2.0, 0.5, 0.0]), np.array([10.0, 2.0, 2.0])),
            gmm.ThermalMesh(),
        )
        cut = gmm.GeometryGroupCutted("cut", [plate], [cutter])
        assert cut.mesh.nf() == 2
        assert _mesh_area(cut) == pytest.approx(0.0, abs=1e-9)
