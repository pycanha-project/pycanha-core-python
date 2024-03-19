import pytest
import numpy as np

# Assuming `pycanha_core` is the module where Triangle and related classes are defined
from pycanha_core import Triangle


@pytest.fixture
def point3d_1():
    return np.array([1.0, 2.0, 3.0])


@pytest.fixture
def point3d_2():
    return np.array([4.0, 5.0, 6.0])


@pytest.fixture
def point3d_3():
    return np.array([7.0, 8.0, 9.0])


# Test initialization of Triangle with Point3D instances
def test_triangle_initialization(point3d_1, point3d_2, point3d_3):
    triangle = Triangle(point3d_1, point3d_2, point3d_3)
    assert isinstance(triangle, Triangle), "Triangle instance is not created properly."


# Test property getters and setters
def test_triangle_properties(point3d_1, point3d_2, point3d_3):
    triangle = Triangle(point3d_1, point3d_2, point3d_3)
    assert np.array_equal(
        triangle.p1, point3d_1
    ), "Triangle p1 property not working as expected."
    # Updating p1 to demonstrate setter
    new_point = np.array([10.0, 11.0, 12.0])
    triangle.p1 = new_point
    assert np.array_equal(
        triangle.p1, new_point
    ), "Triangle p1 setter not working as expected."


# Test method existence and callable
@pytest.mark.parametrize(
    "method_name",
    [
        "v1",
        "v2",
        "is_valid",
        "distance",
        "distance_jacobian_cutted_surface",
        "distance_jacobian_cutting_surface",
        "from_3d_to_2d",
        "from_2d_to_3d",
        "create_mesh",
    ],
)
def test_method_existence(point3d_1, point3d_2, point3d_3, method_name):
    triangle = Triangle(point3d_1, point3d_2, point3d_3)
    assert hasattr(
        triangle, method_name
    ), f"Triangle does not have method '{method_name}'."
    method = getattr(triangle, method_name)
    assert callable(method), f"Triangle method '{method_name}' is not callable."


# Example test for a specific method, assuming the method 'is_valid' returns a bool
def test_triangle_is_valid(point3d_1, point3d_2, point3d_3):
    triangle = Triangle(point3d_1, point3d_2, point3d_3)
    # Note: This assumes 'is_valid' returns a boolean; replace with appropriate logic
    assert isinstance(
        triangle.is_valid(), bool
    ), "Triangle.is_valid() should return a boolean."
