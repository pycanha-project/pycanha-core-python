"""Network parts, their commit, and the node areas, from Python."""

import numpy as np
import pytest

import pycanha_core as pcc

cond = pcc.conduction
gmm = pcc.gmm


def _plate_item(name="plate", start=1, dir1=4, dir2=3):
    mesh = gmm.ThermalMesh(list(np.linspace(0.0, 1.0, dir1 + 1)), list(np.linspace(0.0, 1.0, dir2 + 1)))
    mesh.side1_material = gmm.BulkMaterial("alu", 2700.0, 160.0, 900.0)
    mesh.side1_thick = 0.002
    mesh.node1_start = start
    mesh.node1_step = 1
    mesh.conductive_active_side = gmm.ActiveSide.SIDE1
    mesh.radiative_active_side = gmm.ActiveSide.SIDE1
    rectangle = gmm.Rectangle(
        np.array([0.0, 0.0, 0.0]), np.array([1.0, 0.0, 0.0]), np.array([0.0, 1.0, 0.0])
    )
    return gmm.GeometryItem(name, rectangle, mesh)


class TestNetworkPart:
    def test_part_arrays_are_read_only_views(self):
        part = cond.build_network_part(_plate_item(), gmm.CoordinateTransformation())
        assert len(part.node_numbers) == 12
        np.testing.assert_array_equal(part.node_numbers, np.arange(1, 13))
        assert part.node_numbers.dtype == np.int32
        assert not part.thermal_capacity.flags.writeable
        # rho * c * t over the whole unit plate.
        assert part.thermal_capacity.sum() == pytest.approx(2700.0 * 900.0 * 0.002)
        # 3 horizontal links per row (3 rows) and 4 vertical per pair of rows.
        assert len(part.conductance) == 3 * 3 + 4 * 2
        np.testing.assert_array_equal(
            part.coupling_node_1, part.node_numbers[part.coupling_index_1]
        )
        assert part.report.nodes_created == 12

    def test_parts_commit_into_an_empty_tmm(self):
        parts = [
            cond.build_network_part(_plate_item("a", start=1), gmm.CoordinateTransformation()),
            cond.build_network_part(
                _plate_item("b", start=101),
                gmm.CoordinateTransformation.from_translation(np.array([0.0, 0.0, 2.0])),
            ),
        ]
        model = pcc.tmm.ThermalMathematicalModel("parts")
        report = cond.commit_network_parts(model, parts, cond.TmmBuildOptions(initial_temperature=300.0))
        assert report.items_processed == 2
        assert report.nodes_created == 24
        assert report.conductors_created == 34
        nodes = model.nodes
        assert nodes.get_T(101) == 300.0
        assert nodes.get_fz(101) == pytest.approx(2.0)
        with pytest.raises(ValueError):
            cond.commit_network_parts(model, parts)

    def test_node_areas_on_demand(self):
        model = pcc.tmm.ThermalModel("areas")
        model.gmm.add(_plate_item())
        report = model.build_tmm_from_gmm()
        assert report.nodes_created == 12
        assert model.tmm.nodes.get_a(1) == 0.0
        areas = cond.assign_node_areas(model)
        assert areas.accepted == 12
        assert sum(model.tmm.nodes.get_a(node) for node in range(1, 13)) == pytest.approx(1.0)

    def test_new_diagnostic_codes(self):
        assert cond.diagnostic_code_name(cond.DiagnosticCode.CutFacePairs) == "CutFacePairs"
        assert cond.diagnostic_code_name(cond.DiagnosticCode.UncoupledNodes) == "UncoupledNodes"


class TestNetworkPartSides:
    def test_node_sides_and_side_bulk(self):
        part = cond.build_network_part(_plate_item(), gmm.CoordinateTransformation())
        np.testing.assert_array_equal(part.node_sides, np.full(12, 1, dtype=np.uint8))
        side_1, side_2 = part.side_bulk
        assert side_2 is None
        assert side_1.density == pytest.approx(2700.0)

    def test_part_keeps_its_item_alive(self):
        part = cond.build_network_part(_plate_item(), gmm.CoordinateTransformation())
        import gc

        gc.collect()
        # The materials the part refers to still exist.
        assert part.side_bulk[0].conductivity == pytest.approx(160.0)


class TestStreamedLinks:
    def test_same_links_as_the_list(self):
        item = _plate_item()
        options = cond.TmmBuildOptions()
        streamed = []
        cond.for_each_intra_primitive_link(item.primitive, item.thermal_mesh, options, streamed.append)
        listed = cond.intra_primitive_links(item.primitive, item.thermal_mesh, options)
        assert len(streamed) == len(listed) == 17
        for got, want in zip(streamed, listed, strict=True):
            assert (got.face_pair_a, got.face_pair_b, got.side) == (
                want.face_pair_a,
                want.face_pair_b,
                want.side,
            )
            assert got.conductance == want.conductance

    def test_an_exception_stops_the_walk(self):
        item = _plate_item()
        seen = []

        def sink(link):
            seen.append(link)
            raise RuntimeError("stop")

        with pytest.raises(RuntimeError, match="stop"):
            cond.for_each_intra_primitive_link(
                item.primitive, item.thermal_mesh, cond.TmmBuildOptions(), sink
            )
        assert len(seen) == 1
