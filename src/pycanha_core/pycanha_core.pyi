import numpy as np

class Point3D(np.ndarray):
    """A 3D point represented as a numpy array of floats."""
    pass

class Vector3D(np.ndarray):
    """A 3D vector represented as a numpy array of floats."""
    pass

class Point2D(np.ndarray):
    """A 2D point represented as a numpy array of floats."""
    pass


class Triangle:
    """A class representing a triangle in 3D space."""

    def __init__(self, p1: Point3D, p2: Point3D, p3: Point3D) -> None:
        """Constructs a triangle with the three vertices."""
        pass

    @property
    def p1(self) -> Point3D:
        """The first vertex of the triangle."""
        pass

    @p1.setter
    def p1(self, p1: Point3D) -> None:
        pass

    @property
    def p2(self) -> Point3D:
        """The second vertex of the triangle."""
        pass

    @p2.setter
    def p2(self, p2: Point3D) -> None:
        pass

    @property
    def p3(self) -> Point3D:
        """The third vertex of the triangle."""
        pass

    @p3.setter
    def p3(self, p3: Point3D) -> None:
        pass

    def v1(self) -> Vector3D:
        """Returns the first direction of the triangle."""
        pass

    def v2(self) -> Vector3D:
        """Returns the second direction of the triangle."""
        pass

    def is_valid(self) -> bool:
        """Checks if the triangle is valid."""
        pass

    def distance(self, point: Point3D) -> float:
        pass

    def from_3D_to_2D(self, p3D: Point3D) -> Point2D:
        pass

    def from_2D_to_3D(self, p2D: Point2D) -> Point3D:
        pass

class Rectangle:
    """A class representing a rectangle in 3D space."""

    def __init__(self, p1: Point3D, p2: Point3D, p3: Point3D) -> None:
        """Constructs a rectangle with three of the vertices."""
        pass

    @property
    def p1(self) -> Point3D:
        """The first vertex of the rectangle."""
        pass

    @p1.setter
    def p1(self, p1: Point3D) -> None:
        """Updates the first vertex of the rectangle."""
        pass

    @property
    def p2(self) -> Point3D:
        """The second vertex of the rectangle."""
        pass

    @p2.setter
    def p2(self, p2: Point3D) -> None:
        """Updates the second vertex of the rectangle."""
        pass

    @property
    def p3(self) -> Point3D:
        """The third vertex of the rectangle."""
        pass

    @p3.setter
    def p3(self, p3: Point3D) -> None:
        """Updates the third vertex of the rectangle."""
        pass

    def v1(self) -> Vector3D:
        """Returns the first direction of the rectangle, v1 = p2 - p1."""
        pass

    def v2(self) -> Vector3D:
        """Returns the second direction of the rectangle, v2 = p3 - p2."""
        pass

    def is_valid(self) -> bool:
        """Checks if the rectangle is valid."""
        pass

    def distance(self, point: Point3D) -> float:
        """Returns the distance from a given point to the rectangle."""
        pass

    def from_3D_to_2D(self, p3D: Point3D) -> Point2D:
        """Maps a point from 3D space to the 2D space of the rectangle."""
        pass

    def from_2D_to_3D(self, p2D: Point2D) -> Point3D:
        """Maps a point from the 2D space of the rectangle to 3D space."""
        pass

class Quadrilateral:
    """A class representing a quadrilateral in 3D space."""

    def __init__(self, p1: Point3D, p2: Point3D, p3: Point3D, p4: Point3D) -> None:
        """Constructs a quadrilateral with the four vertices."""
        pass

    @property
    def p1(self) -> Point3D:
        """The first vertex of the quadrilateral."""
        pass

    @p1.setter
    def p1(self, p1: Point3D) -> None:
        """Updates the first vertex of the quadrilateral."""
        pass

    @property
    def p2(self) -> Point3D:
        """The second vertex of the quadrilateral."""
        pass

    @p2.setter
    def p2(self, p2: Point3D) -> None:
        """Updates the second vertex of the quadrilateral."""
        pass

    @property
    def p3(self) -> Point3D:
        """The third vertex of the quadrilateral."""
        pass

    @p3.setter
    def p3(self, p3: Point3D) -> None:
        """Updates the third vertex of the quadrilateral."""
        pass

    @property
    def p4(self) -> Point3D:
        """The fourth vertex of the quadrilateral."""
        pass

    @p4.setter
    def p4(self, p4: Point3D) -> None:
        """Updates the fourth vertex of the quadrilateral."""
        pass

    def v1(self) -> Vector3D:
        """Returns the first direction of the quadrilateral, v1 = p2 - p1."""
        pass

    def v2(self) -> Vector3D:
        """Returns the second direction of the quadrilateral, v2 = p3 - p2."""
        pass

    def is_valid(self) -> bool:
        """Checks if the quadrilateral is valid."""
        pass

    def distance(self, point: Point3D) -> float:
        """Returns the distance from a given point to the quadrilateral."""
        pass

    def from_3D_to_2D(self, p3D: Point3D) -> Point2D:
        """Maps a point from 3D space to the 2D space of the quadrilateral."""
        pass

    def from_2D_to_3D(self, p2D: Point2D) -> Point3D:
        """Maps a point from the 2D space of the quadrilateral to 3D space."""
        pass

class Cylinder:
    """A class representing a cylinder in 3D space."""

    def __init__(self, p1: Point3D, p2: Point3D, p3: Point3D, radius: float, start_angle: float, end_angle: float) -> None:
        """Constructs a cylinder with the three vertices and additional parameters."""
        pass

    @property
    def p1(self) -> Point3D:
        """The first vertex of the cylinder."""
        pass

    @p1.setter
    def p1(self, p1: Point3D) -> None:
        """Updates the first vertex of the cylinder."""
        pass

    @property
    def p2(self) -> Point3D:
        """The second vertex of the cylinder."""
        pass

    @p2.setter
    def p2(self, p2: Point3D) -> None:
        """Updates the second vertex of the cylinder."""
        pass

    @property
    def p3(self) -> Point3D:
        """The third vertex of the cylinder."""
        pass

    @p3.setter
    def p3(self, p3: Point3D) -> None:
        """Updates the third vertex of the cylinder."""
        pass

    @property
    def radius(self) -> float:
        """The radius of the cylinder."""
        pass

    @radius.setter
    def radius(self, radius: float) -> None:
        """Updates the radius of the cylinder."""
        pass

    @property
    def start_angle(self) -> float:
        """The start angle of the cylinder."""
        pass

    @start_angle.setter
    def start_angle(self, start_angle: float) -> None:
        """Updates the start angle of the cylinder."""
        pass

    @property
    def end_angle(self) -> float:
        """The end angle of the cylinder."""
        pass

    @end_angle.setter
    def end_angle(self, end_angle: float) -> None:
        """Updates the end angle of the cylinder."""
        pass

    def is_valid(self) -> bool:
        """Checks if the cylinder is valid."""
        pass

    def distance(self, point: Point3D) -> float:
        """Returns the distance from a given point to the cylinder."""
        pass

    def from_3D_to_2D(self, p3D: Point3D) -> Point2D:
        """Maps a point from 3D space to the 2D space of the cylinder."""
        pass

    def from_2D_to_3D(self, p2D: Point2D) -> Point3D:
        """Maps a point from the 2D space of the cylinder to 3D space."""
        pass

