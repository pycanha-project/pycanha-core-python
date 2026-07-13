"""Tests for the pycanha_core.radiative bindings (0.16).

The view-factor end-to-end runs on EVERY platform where the raytracer is built
(hardware GPU or software lavapipe) and is expected to succeed — a missing or
broken device is a real failure, not a skip. The only exception is macOS, where
the raytracer is compiled out until the Metal backend lands; there the whole
GPU-dependent class is skipped (and ``is_available()`` returns False).
"""

import platform

import numpy as np
import pytest

import pycanha_core as pcc

# macOS builds force PYCANHA_OPTION_RAYTRACING off (no Metal backend yet), so the
# radiative device is genuinely absent there. Everywhere else the raytracer is
# built and must work.
_RAYTRACER_DISABLED = platform.system() == "Darwin"

rad = pcc.radiative
gmm = pcc.gmm


def _densify(sparse):
    """Materialize a radiative.SparseF64 as a dense numpy array."""
    dense = np.zeros((sparse.rows, sparse.cols), dtype=np.float64)
    indptr = np.asarray(sparse.indptr)
    indices = np.asarray(sparse.indices)
    values = np.asarray(sparse.values)
    for row in range(sparse.rows):
        for k in range(indptr[row], indptr[row + 1]):
            dense[row, indices[k]] = values[k]
    return dense


class TestModuleSurface:
    def test_is_available_returns_bool(self):
        assert isinstance(rad.is_available(), bool)

    def test_enumerate_devices_returns_list(self):
        devices = rad.enumerate_devices()
        assert isinstance(devices, list)
        for info in devices:
            assert isinstance(info.name, str)
            assert isinstance(info.ray_tracing, bool)
            assert isinstance(info.index, int)

    def test_enums_present(self):
        assert rad.PartKind.Spacecraft != rad.PartKind.CelestialBody
        assert rad.PartKind.Articulated is not None
        assert rad.Band.IR != rad.Band.Solar
        assert rad.AccumLayout.Dense != rad.AccumLayout.Tiled


class TestValueTypes:
    def test_trace_settings_defaults(self):
        s = rad.TraceSettings()
        assert s.rays_per_face == 10_000
        assert s.seed == 0
        assert s.max_bounces == 64

    def test_trace_settings_kwargs(self):
        s = rad.TraceSettings(rays_per_face=500, seed=7, max_bounces=8)
        assert s.rays_per_face == 500
        assert s.seed == 7
        assert s.max_bounces == 8
        s.seed = 9
        assert s.seed == 9

    def test_accum_config(self):
        c = rad.AccumConfig(layout=rad.AccumLayout.Tiled, tile_rows=64)
        assert c.layout == rad.AccumLayout.Tiled
        assert c.tile_rows == 64

    def test_material_table_roundtrip(self):
        props = np.array([[0.9, 0.0, 0.0, 0.2, 0.0, 0.0]], dtype=np.float32)
        face_material = np.array([0, 0, -1], dtype=np.int32)
        face_active = np.array([True, True, False], dtype=bool)
        table = rad.MaterialTable(props, face_material, face_active)
        assert table.num_materials() == 1
        assert table.num_face_slots() == 3
        np.testing.assert_array_equal(np.asarray(table.face_material), face_material)
        np.testing.assert_array_equal(np.asarray(table.face_active), face_active)


class TestSparseF64:
    def test_construction_and_views(self):
        indptr = np.array([0, 1, 2], dtype=np.int64)
        indices = np.array([1, 0], dtype=np.int32)
        values = np.array([0.5, 0.4], dtype=np.float64)
        s = rad.SparseF64(indptr, indices, values, 2, 2)
        assert s.rows == 2
        assert s.cols == 2
        assert s.nnz() == 2
        assert s.shape == (2, 2)
        np.testing.assert_array_equal(np.asarray(s.indptr), indptr)
        np.testing.assert_array_equal(np.asarray(s.values), values)
        np.testing.assert_allclose(_densify(s), [[0.0, 0.5], [0.4, 0.0]])


class TestAggregation:
    def test_aggregate_nodes_sorted_unique_without_no_node(self):
        node_numbers = np.array([20, -1, 10, 20, -1, 10], dtype=np.int32)
        nodes = np.asarray(rad.aggregate_nodes(node_numbers))
        np.testing.assert_array_equal(nodes, [10, 20])

    def test_aggregate_matrix_area_weighted(self):
        # 2 faces on nodes 10 and 20; F = [[0, 0.5], [0.4, 0]].
        indptr = np.array([0, 1, 2], dtype=np.int64)
        indices = np.array([1, 0], dtype=np.int32)
        values = np.array([0.5, 0.4], dtype=np.float64)
        face_matrix = rad.SparseF64(indptr, indices, values, 2, 2)
        node_numbers = np.array([10, 20], dtype=np.int32)
        face_areas = np.array([2.0, 3.0], dtype=np.float64)

        out = rad.aggregate_matrix(face_matrix, node_numbers, face_areas)
        dense = _densify(out)
        # out(0,1) = area[0]*F(0,1) = 2*0.5 = 1.0; out(1,0) = area[1]*F(1,0) = 3*0.4.
        np.testing.assert_allclose(dense, [[0.0, 1.0], [1.2, 0.0]])

    def test_aggregate_matrix_merges_faces_of_same_node(self):
        # 2 faces on the SAME node 10, plus node 20; check the reduction sums.
        # F = identity-ish off-diagonals so each face sees node 20.
        indptr = np.array([0, 1, 2, 2], dtype=np.int64)
        indices = np.array([2, 2], dtype=np.int32)
        values = np.array([0.3, 0.7], dtype=np.float64)
        face_matrix = rad.SparseF64(indptr, indices, values, 3, 3)
        node_numbers = np.array([10, 10, 20], dtype=np.int32)
        face_areas = np.array([1.0, 2.0, 5.0], dtype=np.float64)

        out = rad.aggregate_matrix(face_matrix, node_numbers, face_areas)
        dense = _densify(out)
        # node 10 -> row 0, node 20 -> row 1.
        # out(0,1) = area0*F(0,2) + area1*F(1,2) = 1*0.3 + 2*0.7 = 1.7.
        np.testing.assert_allclose(dense, [[0.0, 1.7], [0.0, 0.0]])

    def test_aggregate_flux(self):
        face_flux = np.array([100.0, 200.0], dtype=np.float64)
        node_numbers = np.array([10, 20], dtype=np.int32)
        face_areas = np.array([2.0, 3.0], dtype=np.float64)
        node_flux = np.asarray(
            rad.aggregate_flux(face_flux, node_numbers, face_areas)
        )
        # W per node = flux * area: [100*2, 200*3].
        np.testing.assert_allclose(node_flux, [200.0, 600.0])


def _build_panel_model():
    """A single meshed rectangle with an optical material and node numbers."""
    tmesh = gmm.ThermalMesh()
    tmesh.side1_activity = True
    tmesh.side2_activity = True
    tmesh.side1_optical = gmm.OpticalMaterial("white", 0.9, 0.2)
    tmesh.side2_optical = gmm.OpticalMaterial("white", 0.9, 0.2)
    tmesh.node1_start = 100
    tmesh.node1_step = 0
    tmesh.node2_start = 200
    tmesh.node2_step = 0

    rectangle = gmm.Rectangle(
        np.array([0.0, 0.0, 0.0]),
        np.array([1.0, 0.0, 0.0]),
        np.array([0.0, 1.0, 0.0]),
    )
    model = gmm.GeometryModel("panel_demo")
    model.add(gmm.GeometryItem("panel", rectangle, tmesh))
    model.create_mesh()
    return model


class TestGmmRadiativeEntryPoints:
    def test_mesh_parts_default_single_remainder(self):
        model = _build_panel_model()
        parts = model.mesh_parts()
        assert len(parts) == 1
        part = parts[0]
        assert isinstance(part.mesh, gmm.TriMeshF)
        assert isinstance(part.kind, rad.PartKind)
        # The remainder part carries the whole (unsplit) mesh.
        assert part.mesh.nt() == model.mesh.nt()

    def test_material_table_shapes(self):
        model = _build_panel_model()
        table = model.material_table()
        nf = model.mesh.nf()
        assert table.num_face_slots() == nf
        assert np.asarray(table.properties).shape[1] == 6
        assert np.asarray(table.face_material).shape[0] == nf
        assert np.asarray(table.face_active).shape[0] == nf
        # At least one material was assigned (the white paint on both sides).
        assert table.num_materials() >= 1


@pytest.mark.skipif(
    _RAYTRACER_DISABLED,
    reason="raytracing is compiled out on macOS until the Metal backend lands",
)
class TestViewFactorEndToEnd:
    def test_device_available(self):
        # On every non-macOS platform a device (hardware or lavapipe) must exist.
        assert rad.is_available()

    def test_device_and_scene(self):
        info_list = rad.enumerate_devices()
        assert any(d.ray_tracing for d in info_list)

        device = rad.Device.create()
        assert device.info.ray_tracing
        assert device.memory_budget() > 0

        model = _build_panel_model()
        scene = rad.RadiativeScene(
            device, model.mesh_parts(), model.material_table()
        )
        nf = scene.num_face_slots()
        assert nf == model.mesh.nf()
        assert np.asarray(scene.face_areas()).shape[0] == nf

        acc = rad.VfAccumulator(scene)
        scene.accumulate_vf(acc, rad.TraceSettings(rays_per_face=1024, seed=0))
        result = acc.result()

        assert result.vf.rows == nf
        assert result.vf.cols == nf
        assert np.asarray(result.row_sums).shape[0] == nf
        # Row sums are view factors in [0, 1] (deficit is the VF to space).
        row_sums = np.asarray(result.row_sums)
        assert np.all(row_sums >= -1e-6)
        assert np.all(row_sums <= 1.0 + 1e-6)
        assert result.stats.total_rays > 0
