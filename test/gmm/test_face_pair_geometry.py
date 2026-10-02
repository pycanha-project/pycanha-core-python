"""Exact face-pair geometry: FacePairGeometryEvaluator and face_pair_geometry."""

import numpy as np
import pytest

import pycanha_core as pcc

gmm = pcc.gmm

ORIGIN = np.array([0.0, 0.0, 0.0])
AXIS = np.array([0.0, 0.0, 1.0])
REFERENCE = np.array([1.0, 0.0, 0.0])
FULL_TURN = 2.0 * np.pi


def _mesh(dir1, dir2):
    return gmm.ThermalMesh(list(dir1), list(dir2))


# Uneven cuts, so a wrong interval would show up in the sums.
UNEVEN = [0.0, 0.1, 0.35, 0.7, 1.0]

PRIMITIVES = {
    "rectangle": gmm.Rectangle(ORIGIN, np.array([2.0, 0.0, 0.0]), np.array([0.0, 1.0, 0.0])),
    "triangle": gmm.Triangle(ORIGIN, np.array([1.0, 0.0, 0.0]), np.array([0.0, 1.0, 0.0])),
    "quadrilateral": gmm.Quadrilateral(
        ORIGIN, np.array([2.0, 0.0, 0.0]), np.array([2.5, 1.0, 0.0]), np.array([0.0, 1.5, 0.0])
    ),
    "disc": gmm.Disc(
        ORIGIN, AXIS, REFERENCE, inner_radius=0.5, outer_radius=1.0, start_angle=0.0, end_angle=FULL_TURN
    ),
    "cylinder": gmm.Cylinder(ORIGIN, AXIS, REFERENCE, radius=0.4, start_angle=0.0, end_angle=FULL_TURN),
    "cone": gmm.Cone(
        ORIGIN, AXIS, REFERENCE, radius1=1.0, radius2=0.5, start_angle=0.0, end_angle=FULL_TURN
    ),
    "sphere": gmm.Sphere(
        ORIGIN,
        AXIS,
        REFERENCE,
        radius=1.0,
        base_truncation=0.0,
        apex_truncation=1.0,
        start_angle=0.0,
        end_angle=FULL_TURN,
    ),
    "paraboloid": gmm.Paraboloid(ORIGIN, AXIS, REFERENCE, radius=1.0, start_angle=0.0, end_angle=FULL_TURN),
}

REVOLUTIONS = ("disc", "cylinder", "cone", "sphere", "paraboloid")


@pytest.mark.parametrize("name", PRIMITIVES)
def test_face_pair_areas_sum_to_the_surface_area(name):
    primitive = PRIMITIVES[name]
    evaluator = gmm.FacePairGeometryEvaluator(primitive, _mesh(UNEVEN, UNEVEN))
    assert evaluator.is_supported()
    areas, centroids = evaluator.all()
    assert areas.shape == (16,)
    assert centroids.shape == (16, 3)
    assert areas.sum() == pytest.approx(primitive.surface_area(), rel=1e-12)
    assert (areas > 0.0).all()


@pytest.mark.parametrize("name", REVOLUTIONS)
def test_full_turns_centre_on_the_axis(name):
    areas, centroids = gmm.FacePairGeometryEvaluator(PRIMITIVES[name], _mesh(UNEVEN, UNEVEN)).all()
    moment = (areas[:, None] * centroids).sum(axis=0)
    np.testing.assert_allclose(moment[:2], [0.0, 0.0], atol=1e-12)


def test_rectangle_centroids_and_order():
    rectangle = PRIMITIVES["rectangle"]
    evaluator = gmm.FacePairGeometryEvaluator(rectangle, _mesh([0.0, 0.5, 1.0], [0.0, 0.25, 1.0]))
    assert (evaluator.dir1_count, evaluator.dir2_count) == (2, 2)
    areas, centroids = evaluator.all()
    # Direction 1 fastest: entry i + j * dir1_count is face pair (i, j).
    for j in range(2):
        for i in range(2):
            pair = evaluator(i, j)
            assert areas[i + 2 * j] == pair.area
            np.testing.assert_array_equal(centroids[i + 2 * j], pair.centroid)
    # The area-weighted centroid of the whole rectangle is its centre.
    np.testing.assert_allclose(
        (areas[:, None] * centroids).sum(axis=0) / areas.sum(), [1.0, 0.5, 0.0], atol=1e-14
    )
    np.testing.assert_allclose(areas, [0.25, 0.25, 0.75, 0.75], rtol=1e-14)


def test_function_matches_the_evaluator():
    mesh = _mesh(UNEVEN, UNEVEN)
    evaluator = gmm.FacePairGeometryEvaluator(PRIMITIVES["sphere"], mesh)
    single = gmm.face_pair_geometry(PRIMITIVES["sphere"], mesh, 3, 1)
    assert single.area == evaluator(3, 1).area
    np.testing.assert_array_equal(single.centroid, evaluator(3, 1).centroid)
    assert "FacePairGeometry" in repr(single)


def test_indices_outside_the_grid_raise():
    mesh = _mesh([0.0, 0.5, 1.0], [0.0, 1.0])
    evaluator = gmm.FacePairGeometryEvaluator(PRIMITIVES["rectangle"], mesh)
    with pytest.raises(IndexError):
        evaluator(2, 0)
    with pytest.raises(IndexError):
        evaluator(0, 1)
    with pytest.raises(IndexError):
        gmm.face_pair_geometry(PRIMITIVES["rectangle"], mesh, 0, 5)


def test_cutter_only_primitives_have_no_face_pairs():
    cube = gmm.Cube(center=ORIGIN, extent=np.array([1.0, 1.0, 1.0]))
    evaluator = gmm.FacePairGeometryEvaluator(cube, _mesh([0.0, 1.0], [0.0, 1.0]))
    assert not evaluator.is_supported()
    areas, centroids = evaluator.all()
    assert areas.shape == (0,)
    assert centroids.shape == (0, 3)
    assert evaluator(0, 0).area == 0.0


def test_capacities_of_a_network_part_follow_the_exact_areas():
    """The builder's capacities are rho c t times these areas."""
    mesh = _mesh(UNEVEN, UNEVEN)
    mesh.side1_material = gmm.BulkMaterial("alu", 2700.0, 160.0, 900.0)
    mesh.side1_thick = 0.002
    mesh.node1_start = 1
    mesh.node1_step = 1
    mesh.conductive_active_side = gmm.ActiveSide.SIDE1
    mesh.radiative_active_side = gmm.ActiveSide.SIDE1
    cone = PRIMITIVES["cone"]
    part = pcc.conduction.build_network_part(
        gmm.GeometryItem("cone", cone, mesh), gmm.CoordinateTransformation()
    )
    areas, _ = gmm.FacePairGeometryEvaluator(cone, mesh).all()
    np.testing.assert_allclose(part.thermal_capacity, 2700.0 * 900.0 * 0.002 * areas, rtol=1e-12)
