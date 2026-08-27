"""Tests for the 0.15 GMM primitive value classes.

Primitives are nine independent value classes (no base class). Each exposes
point / shape-parameter properties plus is_valid / to_uv / to_cartesian /
normal_at_uv / surface_area. Meshing is no longer on the primitive (see the
UvMesher / GeometryItem / GeometryModel tests).
"""

import numpy as np
import pytest

import pycanha_core as pcc

gmm = pcc.gmm


class TestTriangle:
    @pytest.fixture
    def tri(self):
        return gmm.Triangle(
            np.array([0.0, 0.0, 0.0]),
            np.array([1.0, 0.0, 0.0]),
            np.array([0.0, 1.0, 0.0]),
        )

    def test_construction(self, tri):
        assert isinstance(tri, gmm.Triangle)

    def test_vertices_roundtrip(self, tri):
        p = np.array([2.0, 3.0, 4.0])
        tri.p1 = p
        np.testing.assert_allclose(tri.p1, p, atol=1e-12)

    def test_is_valid(self, tri):
        assert tri.is_valid() is True

    def test_surface_area(self, tri):
        assert tri.surface_area() == pytest.approx(0.5)

    def test_uv_roundtrip(self, tri):
        uv = tri.to_uv(np.array([0.25, 0.25, 0.0]))
        back = tri.to_cartesian(uv)
        np.testing.assert_allclose(back, [0.25, 0.25, 0.0], atol=1e-9)

    def test_normal(self, tri):
        n = tri.normal_at_uv(np.array([0.25, 0.25]))
        np.testing.assert_allclose(np.abs(n), [0.0, 0.0, 1.0], atol=1e-9)


class TestRectangle:
    @pytest.fixture
    def rect(self):
        return gmm.Rectangle(
            np.array([0.0, 0.0, 0.0]),
            np.array([2.0, 0.0, 0.0]),
            np.array([0.0, 1.0, 0.0]),
        )

    def test_valid_and_area(self, rect):
        assert rect.is_valid() is True
        assert rect.surface_area() == pytest.approx(2.0)

    def test_vertices_roundtrip(self, rect):
        np.testing.assert_allclose(rect.p2, [2.0, 0.0, 0.0], atol=1e-12)


class TestQuadrilateral:
    def test_construction_and_props(self):
        quad = gmm.Quadrilateral(
            np.array([0.0, 0.0, 0.0]),
            np.array([2.0, 0.0, 0.0]),
            np.array([2.0, 1.0, 0.0]),
            np.array([0.0, 1.0, 0.0]),
        )
        assert quad.is_valid() is True
        np.testing.assert_allclose(quad.p4, [0.0, 1.0, 0.0], atol=1e-12)


class TestDisc:
    def test_props_roundtrip(self):
        disc = gmm.Disc(
            np.array([0.0, 0.0, 0.0]),
            np.array([0.0, 0.0, 1.0]),
            np.array([1.0, 0.0, 0.0]),
            inner_radius=0.0,
            outer_radius=1.0,
            start_angle=0.0,
            end_angle=2.0 * np.pi,
        )
        assert isinstance(disc.is_valid(), bool)
        disc.outer_radius = 2.0
        assert disc.outer_radius == pytest.approx(2.0)


class TestCylinder:
    def test_props_roundtrip(self):
        cyl = gmm.Cylinder(
            np.array([0.0, 0.0, 0.0]),
            np.array([0.0, 0.0, 1.0]),
            np.array([1.0, 0.0, 0.0]),
            radius=1.0,
            start_angle=0.0,
            end_angle=2.0 * np.pi,
        )
        assert isinstance(cyl.is_valid(), bool)
        cyl.radius = 3.0
        assert cyl.radius == pytest.approx(3.0)


class TestCone:
    def test_props_roundtrip(self):
        cone = gmm.Cone(
            np.array([0.0, 0.0, 0.0]),
            np.array([0.0, 0.0, 1.0]),
            np.array([1.0, 0.0, 0.0]),
            radius1=1.0,
            radius2=0.5,
            start_angle=0.0,
            end_angle=2.0 * np.pi,
        )
        assert isinstance(cone.is_valid(), bool)
        assert cone.radius1 == pytest.approx(1.0)
        assert cone.radius2 == pytest.approx(0.5)


class TestSphere:
    def test_props_roundtrip(self):
        sph = gmm.Sphere(
            np.array([0.0, 0.0, 0.0]),
            np.array([0.0, 0.0, 1.0]),
            np.array([1.0, 0.0, 0.0]),
            radius=1.0,
            base_truncation=0.0,
            apex_truncation=1.0,
            start_angle=0.0,
            end_angle=2.0 * np.pi,
        )
        assert isinstance(sph.is_valid(), bool)
        sph.radius = 2.0
        assert sph.radius == pytest.approx(2.0)


class TestParaboloid:
    def test_props_roundtrip(self):
        par = gmm.Paraboloid(
            np.array([0.0, 0.0, 0.0]),
            np.array([0.0, 0.0, 1.0]),
            np.array([1.0, 0.0, 0.0]),
            radius=1.0,
            start_angle=0.0,
            end_angle=2.0 * np.pi,
        )
        assert isinstance(par.is_valid(), bool)
        par.radius = 1.5
        assert par.radius == pytest.approx(1.5)


class TestCube:
    def test_construction_and_orientation(self):
        cube = gmm.Cube(
            center=np.array([0.0, 0.0, 0.0]),
            extent=np.array([1.0, 1.0, 1.0]),
        )
        assert isinstance(cube.is_valid(), bool)
        # Default orientation is the identity quaternion (w, x, y, z).
        np.testing.assert_allclose(cube.orientation, [1.0, 0.0, 0.0, 0.0], atol=1e-12)
        cube.center = np.array([1.0, 2.0, 3.0])
        np.testing.assert_allclose(cube.center, [1.0, 2.0, 3.0], atol=1e-12)


class TestPrimitiveVariant:
    """A concrete Python primitive round-trips through the Primitive variant."""

    def test_ops_distance_accepts_primitive(self):
        tri = gmm.Triangle(
            np.array([0.0, 0.0, 0.0]),
            np.array([1.0, 0.0, 0.0]),
            np.array([0.0, 1.0, 0.0]),
        )
        d = gmm.distance(tri, np.array([0.25, 0.25, 1.0]))
        assert d == pytest.approx(1.0, abs=1e-9)

    def test_ops_transform_returns_concrete_type(self):
        tri = gmm.Triangle(
            np.array([0.0, 0.0, 0.0]),
            np.array([1.0, 0.0, 0.0]),
            np.array([0.0, 1.0, 0.0]),
        )
        ct = gmm.CoordinateTransformation.from_translation(np.array([1.0, 0.0, 0.0]))
        moved = gmm.transform(tri, ct)
        assert isinstance(moved, gmm.Triangle)
        np.testing.assert_allclose(moved.p1, [1.0, 0.0, 0.0], atol=1e-9)


class TestQuadrilateral:
    """A quadrilateral is a bilinear patch on all four corners."""

    @staticmethod
    def _trapezoid():
        # Parallel edges of 4 m and 2 m, 2 m apart: area (4 + 2) / 2 * 2 = 6.
        return gmm.Quadrilateral(
            np.array([0.0, 0.0, 0.0]),
            np.array([4.0, 0.0, 0.0]),
            np.array([3.0, 2.0, 0.0]),
            np.array([1.0, 2.0, 0.0]),
        )

    def test_area_is_the_trapezoid_not_the_parallelogram(self):
        # The p3-free formula would give the 4 x 2 parallelogram, i.e. 8.
        assert self._trapezoid().surface_area() == pytest.approx(6.0, abs=1e-12)

    def test_p3_is_the_far_corner_of_the_uv_square(self):
        quad = self._trapezoid()
        np.testing.assert_allclose(quad.to_cartesian(np.array([1.0, 1.0])), quad.p3,
                                   atol=1e-12)

    def test_uv_is_normalised(self):
        quad = self._trapezoid()
        # Every planar primitive parametrises on [0, 1] x [0, 1] now.
        np.testing.assert_allclose(quad.to_uv(quad.p2), [1.0, 0.0], atol=1e-9)
        np.testing.assert_allclose(quad.to_uv(quad.p4), [0.0, 1.0], atol=1e-9)


class TestTriangularPrism:
    """Cutter-only solid, shaped like Cube."""

    @staticmethod
    def _wedge():
        # Half-square base with 1 m legs, extruded 2 m along +z: volume 1.
        return gmm.TriangularPrism(
            np.array([0.0, 0.0, 0.0]),
            np.array([1.0, 0.0, 0.0]),
            np.array([0.0, 1.0, 0.0]),
            np.array([0.0, 0.0, 2.0]),
        )

    def test_validity_and_area(self):
        wedge = self._wedge()
        assert wedge.is_valid()
        # Two 0.5 bases, two 1 x 2 walls, one sqrt(2) x 2 hypotenuse wall.
        assert wedge.surface_area() == pytest.approx(
            1.0 + 2.0 + 2.0 + 2.0 * np.sqrt(2.0), abs=1e-9
        )

    def test_extrusion_must_leave_the_base_plane(self):
        flat = gmm.TriangularPrism(
            np.array([0.0, 0.0, 0.0]),
            np.array([1.0, 0.0, 0.0]),
            np.array([0.0, 1.0, 0.0]),
            np.array([1.0, 1.0, 0.0]),
        )
        assert not flat.is_valid()

    def test_is_accepted_as_a_cutter(self):
        assert gmm.is_closed_solid(self._wedge())

    def test_uv_round_trips(self):
        wedge = self._wedge()
        for face in range(5):
            uv = np.array([face + 0.4, 0.6])
            np.testing.assert_allclose(
                wedge.to_uv(wedge.to_cartesian(uv)), uv, atol=1e-9
            )
