"""Test bindings for GMM primitive shapes."""

import math

import numpy as np
import pytest

import pycanha_core as pcc

Cone = pcc.gmm.Cone
Cylinder = pcc.gmm.Cylinder
Disc = pcc.gmm.Disc
Quadrilateral = pcc.gmm.Quadrilateral
Rectangle = pcc.gmm.Rectangle
Sphere = pcc.gmm.Sphere
ThermalMesh = pcc.gmm.ThermalMesh
Triangle = pcc.gmm.Triangle


def _default_thermal_mesh():
    """Return a default ThermalMesh suitable for create_mesh calls."""
    tm = ThermalMesh()
    return tm


# ---------------------------------------------------------------------------
# Triangle
# ---------------------------------------------------------------------------
class TestTriangle:
    @pytest.fixture
    def tri(self):
        p1 = np.array([0.0, 0.0, 0.0])
        p2 = np.array([1.0, 0.0, 0.0])
        p3 = np.array([0.0, 1.0, 0.0])
        return Triangle(p1, p2, p3)

    def test_construction(self, tri):
        assert isinstance(tri, Triangle)

    def test_vertices_roundtrip(self, tri):
        np.testing.assert_array_equal(tri.p1, [0, 0, 0])
        np.testing.assert_array_equal(tri.p2, [1, 0, 0])
        np.testing.assert_array_equal(tri.p3, [0, 1, 0])

    def test_vertex_setter(self, tri):
        new_p = np.array([2.0, 3.0, 4.0])
        tri.p1 = new_p
        np.testing.assert_array_equal(tri.p1, new_p)

    def test_edge_vectors(self, tri):
        # v1 = p2 - p1, v2 = p3 - p1
        np.testing.assert_allclose(tri.v1(), [1, 0, 0])
        np.testing.assert_allclose(tri.v2(), [-1, 1, 0])

    def test_is_valid(self, tri):
        assert tri.is_valid() is True

    def test_collinear_is_invalid(self):
        p1 = np.array([0.0, 0.0, 0.0])
        p2 = np.array([1.0, 0.0, 0.0])
        p3 = np.array([2.0, 0.0, 0.0])
        tri = Triangle(p1, p2, p3)
        assert tri.is_valid() is False

    def test_distance(self, tri):
        point = np.array([0.25, 0.25, 1.0])
        d = tri.distance(point)
        assert isinstance(d, float)
        assert d == pytest.approx(1.0, abs=1e-10)

    def test_3d_2d_roundtrip(self, tri):
        p3d = np.array([0.25, 0.25, 0.0])
        p2d = tri.from_3d_to_2d(p3d)
        p3d_back = tri.from_2d_to_3d(p2d)
        np.testing.assert_allclose(p3d_back, p3d, atol=1e-10)

    # TODO: distance_jacobian_cutted/cutting_surface returns std::array<double,4> not convertible to Python
    def test_create_mesh(self, tri):
        tm = _default_thermal_mesh()
        mesh = tri.create_mesh(tm, 1.0)
        assert mesh is not None


# ---------------------------------------------------------------------------
# Rectangle
# ---------------------------------------------------------------------------
class TestRectangle:
    @pytest.fixture
    def rect(self):
        # Rectangle(p1, p2, p3): p1=origin, p2=end of first edge, p3=corner
        # For a valid rectangle, p1->p2 and p2->p3 must be perpendicular
        p1 = np.array([0.0, 0.0, 0.0])
        p2 = np.array([2.0, 0.0, 0.0])
        p3 = np.array([2.0, 1.0, 0.0])
        return Rectangle(p1, p2, p3)

    def test_construction(self, rect):
        assert isinstance(rect, Rectangle)

    def test_is_valid(self, rect):
        assert rect.is_valid() is True

    def test_vertices_roundtrip(self, rect):
        np.testing.assert_array_equal(rect.p1, [0, 0, 0])
        np.testing.assert_array_equal(rect.p2, [2, 0, 0])
        np.testing.assert_array_equal(rect.p3, [2, 1, 0])

    def test_vertex_setter(self, rect):
        new_p = np.array([5.0, 5.0, 5.0])
        rect.p2 = new_p
        np.testing.assert_array_equal(rect.p2, new_p)

    def test_edge_vectors(self, rect):
        v1 = rect.v1()
        v2 = rect.v2()
        assert len(v1) == 3
        assert len(v2) == 3

    def test_distance(self, rect):
        point = np.array([1.0, 0.5, 2.0])
        d = rect.distance(point)
        assert isinstance(d, float)

    def test_create_mesh(self, rect):
        tm = _default_thermal_mesh()
        mesh = rect.create_mesh(tm, 1.0)
        assert mesh is not None


# ---------------------------------------------------------------------------
# Quadrilateral
# ---------------------------------------------------------------------------
class TestQuadrilateral:
    @pytest.fixture
    def quad(self):
        p1 = np.array([0.0, 0.0, 0.0])
        p2 = np.array([1.0, 0.0, 0.0])
        p3 = np.array([1.0, 1.0, 0.0])
        p4 = np.array([0.0, 1.0, 0.0])
        return Quadrilateral(p1, p2, p3, p4)

    def test_construction_and_validity(self, quad):
        assert isinstance(quad, Quadrilateral)
        assert quad.is_valid() is True

    def test_vertices_roundtrip(self, quad):
        np.testing.assert_array_equal(quad.p1, [0, 0, 0])
        np.testing.assert_array_equal(quad.p4, [0, 1, 0])

    def test_p4_setter(self, quad):
        new_p = np.array([0.1, 1.1, 0.0])
        quad.p4 = new_p
        np.testing.assert_array_equal(quad.p4, new_p)

    def test_distance(self, quad):
        d = quad.distance(np.array([0.5, 0.5, 3.0]))
        assert isinstance(d, float)

    def test_create_mesh(self, quad):
        tm = _default_thermal_mesh()
        mesh = quad.create_mesh(tm, 1.0)
        assert mesh is not None


# ---------------------------------------------------------------------------
# Cylinder
# ---------------------------------------------------------------------------
class TestCylinder:
    @pytest.fixture
    def cyl(self):
        p1 = np.array([0.0, 0.0, 0.0])
        p2 = np.array([1.0, 0.0, 0.0])
        p3 = np.array([0.0, 1.0, 0.0])
        return Cylinder(p1, p2, p3, radius=1.0, start_angle=0.0,
                        end_angle=2 * math.pi)

    def test_construction_and_validity(self, cyl):
        assert isinstance(cyl, Cylinder)
        assert cyl.is_valid() is True

    def test_properties_roundtrip(self, cyl):
        assert cyl.radius == pytest.approx(1.0)
        assert cyl.start_angle == pytest.approx(0.0)
        assert cyl.end_angle == pytest.approx(2 * math.pi)

    def test_property_setters(self, cyl):
        cyl.radius = 2.5
        assert cyl.radius == pytest.approx(2.5)
        cyl.start_angle = 0.1
        assert cyl.start_angle == pytest.approx(0.1)

    def test_distance(self, cyl):
        d = cyl.distance(np.array([0.5, 2.0, 0.0]))
        assert isinstance(d, float)

    def test_create_mesh(self, cyl):
        tm = _default_thermal_mesh()
        mesh = cyl.create_mesh(tm, 1.0)
        assert mesh is not None


# ---------------------------------------------------------------------------
# Disc
# ---------------------------------------------------------------------------
class TestDisc:
    @pytest.fixture
    def disc(self):
        p1 = np.array([0.0, 0.0, 0.0])
        p2 = np.array([1.0, 0.0, 0.0])
        p3 = np.array([0.0, 1.0, 0.0])
        return Disc(p1, p2, p3, inner_radius=0.5, outer_radius=2.0,
                    start_angle=0.0, end_angle=2 * math.pi)

    def test_construction_and_validity(self, disc):
        assert isinstance(disc, Disc)
        assert disc.is_valid() is True

    def test_properties_roundtrip(self, disc):
        assert disc.inner_radius == pytest.approx(0.5)
        assert disc.outer_radius == pytest.approx(2.0)

    def test_property_setters(self, disc):
        disc.inner_radius = 0.3
        disc.outer_radius = 3.0
        assert disc.inner_radius == pytest.approx(0.3)
        assert disc.outer_radius == pytest.approx(3.0)

    def test_create_mesh(self, disc):
        tm = _default_thermal_mesh()
        mesh = disc.create_mesh(tm, 1.0)
        assert mesh is not None


# ---------------------------------------------------------------------------
# Cone
# ---------------------------------------------------------------------------
class TestCone:
    @pytest.fixture
    def cone(self):
        p1 = np.array([0.0, 0.0, 0.0])
        p2 = np.array([1.0, 0.0, 0.0])
        p3 = np.array([0.0, 1.0, 0.0])
        return Cone(p1, p2, p3, radius1=1.0, radius2=0.5,
                    start_angle=0.0, end_angle=2 * math.pi)

    # TODO: Cone.is_valid() raises RuntimeError("Not implemented")
    def test_construction(self, cone):
        assert isinstance(cone, Cone)

    def test_properties_roundtrip(self, cone):
        assert cone.radius1 == pytest.approx(1.0)
        assert cone.radius2 == pytest.approx(0.5)

    def test_property_setters(self, cone):
        cone.radius1 = 2.0
        cone.radius2 = 1.0
        assert cone.radius1 == pytest.approx(2.0)
        assert cone.radius2 == pytest.approx(1.0)

    def test_create_mesh(self, cone):
        tm = _default_thermal_mesh()
        mesh = cone.create_mesh(tm, 1.0)
        assert mesh is not None


# ---------------------------------------------------------------------------
# Sphere
# ---------------------------------------------------------------------------
class TestSphere:
    @pytest.fixture
    def sphere(self):
        p1 = np.array([0.0, 0.0, 0.0])
        p2 = np.array([1.0, 0.0, 0.0])
        p3 = np.array([0.0, 1.0, 0.0])
        # TODO: Sphere rejects base_truncation == apex_truncation (e.g. both 0.0)
        return Sphere(p1, p2, p3, radius=1.0, base_truncation=-0.9,
                      apex_truncation=0.9, start_angle=0.0,
                      end_angle=2 * math.pi)

    def test_construction_and_validity(self, sphere):
        assert isinstance(sphere, Sphere)
        assert sphere.is_valid() is True

    def test_properties_roundtrip(self, sphere):
        assert sphere.radius == pytest.approx(1.0)
        assert sphere.base_truncation == pytest.approx(-0.9)
        assert sphere.apex_truncation == pytest.approx(0.9)

    def test_property_setters(self, sphere):
        sphere.radius = 5.0
        assert sphere.radius == pytest.approx(5.0)

    def test_create_mesh(self, sphere):
        tm = _default_thermal_mesh()
        mesh = sphere.create_mesh(tm, 1.0)
        assert mesh is not None
