"""Tests for the pycanha_core.radiative bindings.

The kernel end-to-ends (view factors, exchange, solar) run on EVERY platform
where the raytracer is built (hardware GPU or software lavapipe) and are
expected to succeed — a missing or broken device is a real failure, not a skip.
The only exception is macOS, where the raytracer is compiled out until the Metal
backend lands; there the whole GPU-dependent class is skipped (and
``is_available()`` returns False).

The Gebhart and aggregation services are pure CPU and are tested everywhere.

Every sparse result crosses as a scipy.sparse.csr_matrix, and every sparse
argument accepts one. Matrix results hold the UPPER TRIANGLE of the symmetric,
extensive quantity (G = A_i F_ij for view factors), so a test that builds an
input by hand builds it in that form.
"""

import platform

import numpy as np
import pytest
from scipy.sparse import csr_matrix

import pycanha_core as pcc

# macOS builds force PYCANHA_OPTION_RAYTRACING off (no Metal backend yet), so the
# radiative device is genuinely absent there. Everywhere else the raytracer is
# built and must work.
_RAYTRACER_DISABLED = platform.system() == "Darwin"

rad = pcc.radiative
gmm = pcc.gmm


def _densify(sparse):
    """Materialize a scipy sparse matrix as a dense numpy array."""
    return np.asarray(sparse.todense())


def _csr(dense):
    """Build the csr_matrix a binding argument expects from a dense list."""
    return csr_matrix(np.asarray(dense, dtype=np.float64))


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
        # NONE rather than None: the C++ enumerator's name is a Python keyword.
        assert rad.TriangulationMode.NONE != rad.TriangulationMode.RayDensity
        assert rad.TriangulationMode.ConstrainedLeastSquares is not None

    def test_virtual_bucket_column_constants(self):
        # Every matrix result appends these columns after the real face slots.
        assert rad.num_virtual_columns == 3
        offsets = {
            rad.space_column_offset,
            rad.inactive_column_offset,
            rad.lost_column_offset,
        }
        assert offsets == {0, 1, 2}


class TestValueTypes:
    def test_trace_settings_defaults(self):
        s = rad.TraceSettings()
        assert s.rays_per_face == 10_000
        assert s.seed == 0
        assert s.max_bounces == 64
        assert s.normal_emission is False

    def test_trace_settings_kwargs(self):
        s = rad.TraceSettings(rays_per_face=500, seed=7, max_bounces=8)
        assert s.rays_per_face == 500
        assert s.seed == 7
        assert s.max_bounces == 8
        s.seed = 9
        assert s.seed == 9

    def test_trace_settings_normal_emission(self):
        s = rad.TraceSettings(rays_per_face=64, normal_emission=True)
        assert s.normal_emission is True
        s.normal_emission = False
        assert s.normal_emission is False

    def test_solar_state(self):
        sun = rad.SolarState(np.array([0.0, 0.0, -1.0]), 1361.0)
        np.testing.assert_allclose(np.asarray(sun.direction), [0.0, 0.0, -1.0])
        assert sun.irradiance == 1361.0
        sun.irradiance = 1322.0
        assert sun.irradiance == 1322.0

    def test_accum_config(self):
        c = rad.AccumConfig(layout=rad.AccumLayout.Tiled, tile_rows=64)
        assert c.layout == rad.AccumLayout.Tiled
        assert c.tile_rows == 64
        # The default triangulation travels with a default-built config.
        assert c.triangulation.mode == rad.TriangulationMode.RayDensity

    def test_triangulation_config_defaults(self):
        t = rad.TriangulationConfig()
        assert t.mode == rad.TriangulationMode.RayDensity
        assert t.exponent == pytest.approx(0.4)
        assert t.keep_full_matrix is False

    def test_triangulation_config_travels_into_accum_config(self):
        t = rad.TriangulationConfig(
            mode=rad.TriangulationMode.ConstrainedLeastSquares,
            keep_full_matrix=True,
        )
        c = rad.AccumConfig(triangulation=t)
        assert c.triangulation.mode == rad.TriangulationMode.ConstrainedLeastSquares
        assert c.triangulation.keep_full_matrix is True
        c.triangulation = rad.TriangulationConfig(exponent=1.0)
        assert c.triangulation.exponent == pytest.approx(1.0)

    def test_material_table_roundtrip(self):
        props = np.array([[0.9, 0.0, 0.0, 0.2, 0.0, 0.0]], dtype=np.float32)
        face_material = np.array([0, 0, -1], dtype=np.int32)
        face_active = np.array([True, True, False], dtype=bool)
        table = rad.MaterialTable(props, face_material, face_active)
        assert table.num_materials() == 1
        assert table.num_face_slots() == 3
        np.testing.assert_array_equal(np.asarray(table.face_material), face_material)
        np.testing.assert_array_equal(np.asarray(table.face_active), face_active)


class TestAggregation:
    def test_aggregate_nodes_sorted_unique_without_no_node(self):
        node_numbers = np.array([20, -1, 10, 20, -1, 10], dtype=np.int32)
        nodes = np.asarray(rad.aggregate_nodes(node_numbers))
        np.testing.assert_array_equal(nodes, [10, 20])

    def test_aggregate_matrix_sums_the_extensive_entries(self):
        # 2 faces on nodes 10 and 20, coupled by G(0,1) = 1.0 (m^2). The
        # reduction is a plain sum: the stored value is already extensive, so
        # there is no area weighting to apply.
        face_matrix = _csr([[0.0, 1.0], [0.0, 0.0]])
        node_numbers = np.array([10, 20], dtype=np.int32)

        out = rad.aggregate_matrix(face_matrix, node_numbers)
        np.testing.assert_allclose(
            _densify(out.matrix), [[0.0, 1.0], [0.0, 0.0]]
        )
        assert out.intra_node_total == 0.0

    def test_aggregate_matrix_merges_faces_of_same_node(self):
        # Faces 0 and 1 are both node 10, face 2 is node 20.
        face_matrix = _csr(
            [
                [0.0, 0.0, 0.3],
                [0.0, 0.0, 0.7],
                [0.0, 0.0, 0.0],
            ]
        )
        node_numbers = np.array([10, 10, 20], dtype=np.int32)

        out = rad.aggregate_matrix(face_matrix, node_numbers)
        # node 10 -> row 0, node 20 -> row 1: 0.3 + 0.7 lands on (0, 1).
        np.testing.assert_allclose(
            _densify(out.matrix), [[0.0, 1.0], [0.0, 0.0]]
        )

    def test_aggregate_matrix_reports_the_intra_node_couplings_it_drops(self):
        # G(0,1) couples two faces of the SAME node: a node is isothermal, so
        # that exchange transports no heat and has no slot in the network.
        face_matrix = _csr(
            [
                [0.0, 0.25, 0.3],
                [0.0, 0.00, 0.7],
                [0.0, 0.00, 0.0],
            ]
        )
        node_numbers = np.array([10, 10, 20], dtype=np.int32)

        out = rad.aggregate_matrix(face_matrix, node_numbers)
        np.testing.assert_allclose(
            _densify(out.matrix), [[0.0, 1.0], [0.0, 0.0]]
        )
        assert out.intra_node_total == pytest.approx(0.25)

    def test_aggregate_matrix_canonicalises_into_the_upper_triangle(self):
        # Node numbers are assigned independently of slot numbering, so the
        # face pair (0, 1) lands on the node pair (1, 0) and has to be flipped.
        face_matrix = _csr([[0.0, 1.0], [0.0, 0.0]])
        node_numbers = np.array([20, 10], dtype=np.int32)

        out = rad.aggregate_matrix(face_matrix, node_numbers)
        # Rows/cols are [10, 20]: the entry must land above the diagonal.
        np.testing.assert_allclose(
            _densify(out.matrix), [[0.0, 1.0], [0.0, 0.0]]
        )

    def test_aggregate_flux(self):
        face_flux = np.array([100.0, 200.0], dtype=np.float64)
        node_numbers = np.array([10, 20], dtype=np.int32)
        face_areas = np.array([2.0, 3.0], dtype=np.float64)
        node_flux = np.asarray(
            rad.aggregate_flux(face_flux, node_numbers, face_areas)
        )
        # W per node = flux * area: [100*2, 200*3].
        np.testing.assert_allclose(node_flux, [200.0, 600.0])

    def test_aggregate_matrix_row_col_maps_bucket_to_a_node(self):
        # 2 face rows, 2 face columns + the 3 virtual bucket columns. Row 0
        # couples to face 1 and to space; row 1 only to space.
        space_col = 2 + rad.space_column_offset
        dense = np.zeros((2, 5))
        dense[0, 1] = 1.0
        dense[0, space_col] = 0.5
        dense[1, space_col] = 0.6
        face_matrix = csr_matrix(dense)

        row_nodes = np.array([10, 20], dtype=np.int32)
        # Space maps to node 99; the inactive/lost buckets are dropped.
        col_nodes = np.array([10, 20, 99, -1, -1], dtype=np.int32)

        out = rad.aggregate_matrix(face_matrix, row_nodes, col_nodes)
        assert out.matrix.shape == (2, 3)  # rows [10, 20], cols [10, 20, 99]
        # Two independent label sets: every entry stays where it lands, with
        # no triangle to canonicalise into.
        np.testing.assert_allclose(
            _densify(out.matrix),
            [[0.0, 1.0, 0.5], [0.0, 0.0, 0.6]],
        )
        assert out.intra_node_total == 0.0

    def test_aggregate_matrix_plain_overload_drops_buckets(self):
        space_col = 2 + rad.space_column_offset
        dense = np.zeros((2, 5))
        dense[0, 1] = 1.0
        dense[0, space_col] = 0.5
        dense[1, space_col] = 1.0
        face_matrix = csr_matrix(dense)
        node_numbers = np.array([10, 20], dtype=np.int32)

        out = rad.aggregate_matrix(face_matrix, node_numbers)
        assert out.matrix.shape == (2, 2)
        np.testing.assert_allclose(
            _densify(out.matrix), [[0.0, 1.0], [0.0, 0.0]]
        )


class TestGebhart:
    """CPU diffuse-gray services: no GPU involved, so they run everywhere."""

    # Two infinite parallel gray plates (F12 = F21 = 1, closed enclosure) have
    # the closed-form Gebhart matrix below for equal emissivities. With unit
    # areas the stored G(0,1) = A_0 F_01 is 1.0, in the upper triangle only.
    _UNIT_AREAS = np.array([1.0, 1.0], dtype=np.float64)

    @staticmethod
    def _closed_pair_vf(cols=2):
        dense = np.zeros((2, cols))
        dense[0, 1] = 1.0
        return csr_matrix(dense)

    def test_gebhart_factors_two_plate_closed_form(self):
        eps = 0.5
        b = _densify(
            rad.gebhart_factors(
                self._closed_pair_vf(), np.array([eps, eps]), self._UNIT_AREAS
            )
        )
        # B = (I - F R)^-1 F E with R = 1 - eps: off-diagonal 2/3, diagonal 1/3.
        np.testing.assert_allclose(b, [[1 / 3, 2 / 3], [2 / 3, 1 / 3]])
        # A closed enclosure re-absorbs everything: rows sum to 1.
        np.testing.assert_allclose(b.sum(axis=1), [1.0, 1.0])
        # eps * B12 is the classic script-F = 1/(1/e1 + 1/e2 - 1).
        np.testing.assert_allclose(eps * b[0, 1], 1.0 / (1 / eps + 1 / eps - 1))

    def test_gebhart_factors_recovers_both_directions_from_the_areas(self):
        # Unequal areas: the single stored G(0,1) is A_0 F_01 = A_1 F_10, so
        # F_01 = 1 and F_10 = 0.5 come back from the areas alone.
        areas = np.array([1.0, 2.0], dtype=np.float64)
        dense = np.zeros((2, 2))
        dense[0, 1] = 1.0
        eps = np.array([1.0, 1.0])

        b = _densify(rad.gebhart_factors(csr_matrix(dense), eps, areas))
        # Black surfaces re-emit nothing, so B is F itself.
        np.testing.assert_allclose(b, [[0.0, 1.0], [0.5, 0.0]])

    def test_gebhart_rejects_a_lower_triangle_entry(self):
        # The transpose of a stored coupling is not a second coupling; folding
        # it in would count the pair twice.
        both_triangles = _csr([[0.0, 1.0], [1.0, 0.0]])
        with pytest.raises(ValueError):
            rad.gebhart_factors(
                both_triangles, np.array([0.5, 0.5]), self._UNIT_AREAS
            )

    def test_gebhart_factors_accepts_bucket_carrying_vf(self):
        # A traced VF result carries the virtual columns; the bucket columns
        # never re-emit, so both shapes solve the same system.
        eps = np.array([0.5, 0.5])
        square = _densify(
            rad.gebhart_factors(self._closed_pair_vf(), eps, self._UNIT_AREAS)
        )
        with_buckets = _densify(
            rad.gebhart_factors(
                self._closed_pair_vf(2 + rad.num_virtual_columns),
                eps,
                self._UNIT_AREAS,
            )
        )
        np.testing.assert_allclose(with_buckets, square)

    def test_gebhart_factors_space_policy_renormalizes(self):
        # An open enclosure: each plate sees the other with 0.5 only.
        dense = np.zeros((2, 2))
        dense[0, 1] = 0.5
        vf = csr_matrix(dense)
        eps = np.array([0.5, 0.5])

        as_space = _densify(rad.gebhart_factors(vf, eps, self._UNIT_AREAS, 1.0))
        # policy 1.0 keeps the deficit as a real view to space: rows lose it.
        assert as_space.sum(axis=1)[0] < 1.0
        # policy 0.0 treats the deficit as Monte-Carlo noise and renormalizes,
        # which recovers the closed two-plate answer.
        renormalized = _densify(
            rad.gebhart_factors(vf, eps, self._UNIT_AREAS, 0.0)
        )
        np.testing.assert_allclose(
            renormalized, [[1 / 3, 2 / 3], [2 / 3, 1 / 3]]
        )

    def test_gebhart_node_factors_reciprocity(self):
        eps = 0.5
        node_numbers = np.array([10, 20], dtype=np.int32)
        gr = _densify(
            rad.gebhart_node_factors(
                self._closed_pair_vf(),
                np.array([eps, eps]),
                node_numbers,
                self._UNIT_AREAS,
            )
        )
        # GR(m, n) = sum A_i eps_i B_ij: eps * B with unit areas.
        np.testing.assert_allclose(
            gr, [[eps / 3, 2 * eps / 3], [2 * eps / 3, eps / 3]]
        )
        # Reciprocity: GR is symmetric.
        np.testing.assert_allclose(gr, gr.T)

    def test_gebhart_rejects_bad_emissivity(self):
        with pytest.raises(ValueError):
            rad.gebhart_factors(
                self._closed_pair_vf(), np.array([0.5, 1.5]), self._UNIT_AREAS
            )

    def test_gebhart_rejects_mismatched_sizes(self):
        with pytest.raises(ValueError):
            rad.gebhart_factors(
                self._closed_pair_vf(), np.array([0.5]), self._UNIT_AREAS
            )


def _build_panel_model():
    """A single meshed rectangle with an optical material and node numbers."""
    tmesh = gmm.ThermalMesh()
    tmesh.radiative_active_side = gmm.ActiveSide.BOTH
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

        # Columns carry the virtual space/inactive/lost buckets.
        assert result.vf.shape == (nf, nf + rad.num_virtual_columns)
        assert np.asarray(result.row_sums).shape[0] == nf
        # With the buckets included every emitting row closes at exactly 1.
        row_sums = np.asarray(result.row_sums)
        np.testing.assert_allclose(row_sums, np.ones(nf), atol=1e-12)
        assert result.stats.total_rays > 0
        # Nothing asked for the raw both-triangle matrix.
        assert result.full_vf is None

        # An isolated panel sees nothing: all of it goes to the space bucket.
        # The stored value is extensive, so a fully escaping row carries its
        # own area there rather than a view factor of 1.
        areas = np.asarray(scene.face_areas())
        dense = _densify(result.vf)
        np.testing.assert_allclose(
            dense[:, nf + rad.space_column_offset], areas, atol=1e-12
        )
        # Only the upper triangle of the real face columns is stored.
        assert np.count_nonzero(np.tril(dense[:, :nf], k=-1)) == 0

    def test_keep_full_matrix_returns_both_triangles(self):
        device = rad.Device.create()
        model = _build_panel_model()
        scene = rad.RadiativeScene(
            device, model.mesh_parts(), model.material_table()
        )
        config = rad.AccumConfig(
            triangulation=rad.TriangulationConfig(keep_full_matrix=True)
        )
        acc = rad.VfAccumulator(scene, config)
        scene.accumulate_vf(acc, rad.TraceSettings(rays_per_face=256, seed=0))
        result = acc.result()

        assert result.full_vf is not None
        assert result.full_vf.shape == result.vf.shape

    def test_estimate_memory(self):
        device = rad.Device.create()
        model = _build_panel_model()
        scene = rad.RadiativeScene(
            device, model.mesh_parts(), model.material_table()
        )
        nf = scene.num_face_slots()

        estimate = rad.estimate_memory(scene)
        # u64 cells over nf x (nf + virtual columns).
        assert estimate.gpu_bytes_per_tile_row == (nf + rad.num_virtual_columns) * 8
        assert estimate.gpu_bytes_dense == nf * estimate.gpu_bytes_per_tile_row
        assert estimate.host_bytes_block == estimate.gpu_bytes_dense
        assert estimate.gpu_bytes_scene > 0

        tiled = rad.estimate_memory(
            scene,
            rad.AccumConfig(layout=rad.AccumLayout.Tiled, tile_rows=1),
        )
        assert tiled.host_bytes_block == tiled.gpu_bytes_per_tile_row

    def test_update_materials_without_rebuild(self):
        device = rad.Device.create()
        model = _build_panel_model()
        table = model.material_table()
        scene = rad.RadiativeScene(device, model.mesh_parts(), table)

        # Same mapping and activity, different property rows (an EOL swap).
        props = np.asarray(table.properties).copy()
        props[:, 0] = 0.5
        eol = rad.MaterialTable(
            props,
            np.asarray(table.face_material),
            np.asarray(table.face_active),
        )
        scene.update_materials(eol)
        np.testing.assert_allclose(
            np.asarray(scene.materials().properties)[:, 0], 0.5
        )


@pytest.mark.skipif(
    _RAYTRACER_DISABLED,
    reason="raytracing is compiled out on macOS until the Metal backend lands",
)
class TestExchangeEndToEnd:
    def test_isolated_panel_sends_everything_to_space(self):
        device = rad.Device.create()
        model = _build_panel_model()
        scene = rad.RadiativeScene(
            device, model.mesh_parts(), model.material_table()
        )
        nf = scene.num_face_slots()
        table = scene.materials()

        acc = rad.ExchangeAccumulator(scene, rad.Band.IR)
        scene.accumulate_exchange(
            acc, rad.TraceSettings(rays_per_face=1024, seed=0)
        )
        result = acc.result()

        assert result.band == rad.Band.IR
        assert result.factors.shape == (nf, nf + rad.num_virtual_columns)
        assert result.full_factors is None
        # The kernel flushes every ray's balance into a column, so the
        # conservation residue is zero by construction, not within noise.
        assert acc.conservation_error() == 0

        # The two sides of an isolated panel face away from each other and see
        # nothing: all emitted energy escapes. The stored value is extensive,
        # so a fully escaping row carries its emissive area A_i eps_i.
        eps = np.asarray(table.properties)[
            np.asarray(table.face_material), 0
        ]
        emissive_area = np.asarray(scene.face_areas()) * eps
        dense = _densify(result.factors)
        np.testing.assert_allclose(
            dense[:, nf + rad.space_column_offset], emissive_area, atol=1e-9
        )

    def test_reset_clears_the_accumulator(self):
        device = rad.Device.create()
        model = _build_panel_model()
        scene = rad.RadiativeScene(
            device, model.mesh_parts(), model.material_table()
        )
        acc = rad.ExchangeAccumulator(scene, rad.Band.Solar)
        scene.accumulate_exchange(
            acc, rad.TraceSettings(rays_per_face=256, seed=0)
        )
        assert acc.result().stats.total_rays > 0
        acc.reset()
        assert acc.result().stats.total_rays == 0


@pytest.mark.skipif(
    _RAYTRACER_DISABLED,
    reason="raytracing is compiled out on macOS until the Metal backend lands",
)
class TestSolarEndToEnd:
    def test_normal_incidence_cosine_law(self):
        device = rad.Device.create()
        model = _build_panel_model()
        table = model.material_table()
        scene = rad.RadiativeScene(device, model.mesh_parts(), table)

        # The panel is the unit square at z = 0 with side 1 (slot 0) facing +z.
        irradiance = 1361.0
        sun = rad.SolarState(np.array([0.0, 0.0, -1.0]), irradiance)
        acc = rad.SolarAccumulator(scene)
        scene.accumulate_solar(
            sun, acc, rad.TraceSettings(rays_per_face=1024, seed=0)
        )
        result = acc.result()

        nf = scene.num_face_slots()
        direct = np.asarray(result.direct)
        total = np.asarray(result.total)
        assert direct.shape[0] == nf
        assert total.shape[0] == nf

        # Results are WATTS: alpha_solar * S * area * cos(0) on the lit side.
        material_row = int(np.asarray(table.face_material)[0])
        alpha = float(np.asarray(table.properties)[material_row, 3])
        area = float(np.asarray(scene.face_areas())[0])
        expected = alpha * irradiance * area
        assert direct[0] == pytest.approx(expected, rel=1e-3)
        # The back side never sees the sun.
        assert direct[1] == 0.0
        assert total[1] == 0.0
