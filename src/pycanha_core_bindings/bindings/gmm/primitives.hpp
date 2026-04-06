#pragma once
#include <memory>
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>

#include "pycanha-core/globals.hpp"
#include "pycanha-core/gmm/primitives.hpp"

namespace nb = nanobind;
using namespace nanobind::literals;
using namespace pycanha::gmm;

void Primitive_b(nb::module_ &m) {
  nb::class_<Primitive>(m, "Primitive", "Base class for geometric primitives.")
      .def("distance", &Primitive::distance, "point"_a,
           "Signed distance from a point to the surface.")
      .def("distance_jacobian_cutted_surface",
           &Primitive::distance_jacobian_cutted_surface, "point"_a,
           "Distance Jacobian w.r.t. the cutted surface parameters.")
      .def("distance_jacobian_cutting_surface",
           &Primitive::distance_jacobian_cutting_surface, "point"_a,
           "Distance Jacobian w.r.t. the cutting surface parameters.")
      .def("is_valid", &Primitive::is_valid,
           "Check whether the primitive geometry is valid.")
      .def("from_2d_to_3d", &Primitive::from_2d_to_3d, "p2d"_a,
           "Map a 2D parametric point to 3D space.")
      .def("from_3d_to_2d", &Primitive::from_3d_to_2d, "p3d"_a,
           "Project a 3D point onto the 2D parametric space.");
}

void Triangle_b(nb::module_ &m) {
  nb::class_<Triangle, Primitive>(m, "Triangle",
                                  "Triangular flat surface defined by three 3D vertices.")
      .def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D>(),
           "p1"_a, "p2"_a, "p3"_a,
           "Create a triangle from three 3D vertex positions.")
      .def_prop_rw("p1", &Triangle::get_p1, &Triangle::set_p1,
                   "First vertex position.")
      .def_prop_rw("p2", &Triangle::get_p2, &Triangle::set_p2,
                   "Second vertex position.")
      .def_prop_rw("p3", &Triangle::get_p3, &Triangle::set_p3,
                   "Third vertex position.")
      .def("v1", &Triangle::v1, "Edge vector p2 - p1.")
      .def("v2", &Triangle::v2, "Edge vector p3 - p1.")
      .def("is_valid", &Triangle::is_valid)
      .def("distance", &Triangle::distance, "point"_a)
      .def("distance_jacobian_cutted_surface",
           &Triangle::distance_jacobian_cutted_surface, "point"_a)
      .def("distance_jacobian_cutting_surface",
           &Triangle::distance_jacobian_cutting_surface, "point"_a)
      .def("from_3d_to_2d", &Triangle::from_3d_to_2d, "p3d"_a)
      .def("from_2d_to_3d", &Triangle::from_2d_to_3d, "p2d"_a)
      .def("create_mesh", &Triangle::create_mesh,
           "Create a triangular mesh of this primitive.");
}

void Rectangle_b(nb::module_ &m) {
  nb::class_<Rectangle, Primitive>(m, "Rectangle",
                                   "Rectangular flat surface defined by three vertices.\n\n"
                                   "The fourth vertex is derived automatically.\n"
                                   "p1-p2 defines one edge, p1-p3 the adjacent edge.")
      .def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D>(),
           "p1"_a, "p2"_a, "p3"_a,
           "Create a rectangle from three corner positions.")
      .def_prop_rw("p1", &Rectangle::get_p1, &Rectangle::set_p1,
                   "Origin vertex position.")
      .def_prop_rw("p2", &Rectangle::get_p2, &Rectangle::set_p2,
                   "Second vertex (defines first edge from p1).")
      .def_prop_rw("p3", &Rectangle::get_p3, &Rectangle::set_p3,
                   "Third vertex (defines second edge from p1).")
      .def("v1", &Rectangle::v1, "Edge vector p2 - p1.")
      .def("v2", &Rectangle::v2, "Edge vector p3 - p2.")
      .def("is_valid", &Rectangle::is_valid)
      .def("distance", &Rectangle::distance, "point"_a)
      .def("distance_jacobian_cutted_surface",
           &Rectangle::distance_jacobian_cutted_surface, "point"_a)
      .def("distance_jacobian_cutting_surface",
           &Rectangle::distance_jacobian_cutting_surface, "point"_a)
      .def("from_3d_to_2d", &Rectangle::from_3d_to_2d, "p3d"_a)
      .def("from_2d_to_3d", &Rectangle::from_2d_to_3d, "p2d"_a)
      .def("create_mesh", &Rectangle::create_mesh,
           "Create a triangular mesh of this primitive.");
}

void Quadrilateral_b(nb::module_ &m) {
  nb::class_<Quadrilateral, Primitive>(
      m, "Quadrilateral",
      "General quadrilateral surface defined by four vertices\n"
      "(may be non-planar).")
      .def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D,
                    pycanha::Point3D>(),
           "p1"_a, "p2"_a, "p3"_a, "p4"_a,
           "Create a quadrilateral from four 3D vertex positions.")
      .def_prop_rw("p1", &Quadrilateral::get_p1, &Quadrilateral::set_p1,
                   "First vertex position.")
      .def_prop_rw("p2", &Quadrilateral::get_p2, &Quadrilateral::set_p2,
                   "Second vertex position.")
      .def_prop_rw("p3", &Quadrilateral::get_p3, &Quadrilateral::set_p3,
                   "Third vertex position.")
      .def_prop_rw("p4", &Quadrilateral::get_p4, &Quadrilateral::set_p4,
                   "Fourth vertex position.")
      .def("v1", &Quadrilateral::v1, "Edge vector p2 - p1.")
      .def("v2", &Quadrilateral::v2, "Edge vector p3 - p2.")
      .def("is_valid", &Quadrilateral::is_valid)
      .def("distance", &Quadrilateral::distance, "point"_a)
      .def("distance_jacobian_cutted_surface",
           &Quadrilateral::distance_jacobian_cutted_surface, "point"_a)
      .def("distance_jacobian_cutting_surface",
           &Quadrilateral::distance_jacobian_cutting_surface, "point"_a)
      .def("from_3d_to_2d", &Quadrilateral::from_3d_to_2d, "p3d"_a)
      .def("from_2d_to_3d", &Quadrilateral::from_2d_to_3d, "p2d"_a)
      .def("create_mesh", &Quadrilateral::create_mesh,
           "Create a triangular mesh of this primitive.");
}

void Cylinder_b(nb::module_ &m) {
  nb::class_<Cylinder, Primitive>(m, "Cylinder",
                                  "Cylindrical surface segment defined by axis,\n"
                                  "radius, and angular extent.")
      .def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D,
                    double, double, double>(),
           "p1"_a, "p2"_a, "p3"_a, "radius"_a, "start_angle"_a,
           "end_angle"_a,
           "Create a cylinder from axis points, reference, radius, and angles.")
      .def_prop_rw("p1", &Cylinder::get_p1, &Cylinder::set_p1,
                   "Base center position.")
      .def_prop_rw("p2", &Cylinder::get_p2, &Cylinder::set_p2,
                   "Top center position.")
      .def_prop_rw("p3", &Cylinder::get_p3, &Cylinder::set_p3,
                   "Reference point for angle origin.")
      .def_prop_rw("radius", &Cylinder::get_radius, &Cylinder::set_radius,
                   "Cylinder radius.")
      .def_prop_rw("start_angle", &Cylinder::get_start_angle,
                    &Cylinder::set_start_angle,
                    "Start angle [rad].")
      .def_prop_rw("end_angle", &Cylinder::get_end_angle,
                    &Cylinder::set_end_angle,
                    "End angle [rad].")
      .def("is_valid", &Cylinder::is_valid)
      .def("distance", &Cylinder::distance, "point"_a)
      .def("distance_jacobian_cutted_surface",
           &Cylinder::distance_jacobian_cutted_surface, "point"_a)
      .def("distance_jacobian_cutting_surface",
           &Cylinder::distance_jacobian_cutting_surface, "point"_a)
      .def("from_3d_to_2d", &Cylinder::from_3d_to_2d, "p3d"_a)
      .def("from_2d_to_3d", &Cylinder::from_2d_to_3d, "p2d"_a)
      .def("create_mesh", &Cylinder::create_mesh,
           "Create a triangular mesh of this primitive.");
}

void Disc_b(nb::module_ &m) {
  nb::class_<Disc, Primitive>(m, "Disc",
                            "Annular disc (flat ring) surface segment defined\n"
                            "by center, normal, inner/outer radii, and angular extent.")
      .def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D,
                    double, double, double, double>(),
           "p1"_a, "p2"_a, "p3"_a, "inner_radius"_a, "outer_radius"_a,
           "start_angle"_a, "end_angle"_a,
           "Create a disc from center, normal ref, radii, and angles.")
      .def_prop_rw("p1", &Disc::get_p1, &Disc::set_p1,
                   "Center position.")
      .def_prop_rw("p2", &Disc::get_p2, &Disc::set_p2,
                   "Normal direction reference point.")
      .def_prop_rw("p3", &Disc::get_p3, &Disc::set_p3,
                   "Reference point for angle origin.")
      .def_prop_rw("inner_radius", &Disc::get_inner_radius,
                    &Disc::set_inner_radius,
                    "Inner radius (0 for a full disc).")
      .def_prop_rw("outer_radius", &Disc::get_outer_radius,
                    &Disc::set_outer_radius,
                    "Outer radius.")
      .def_prop_rw("start_angle", &Disc::get_start_angle,
                    &Disc::set_start_angle,
                    "Start angle [rad].")
      .def_prop_rw("end_angle", &Disc::get_end_angle, &Disc::set_end_angle,
                   "End angle [rad].")
      .def("is_valid", &Disc::is_valid)
      .def("distance", &Disc::distance, "point"_a)
      .def("distance_jacobian_cutted_surface",
           &Disc::distance_jacobian_cutted_surface, "point"_a)
      .def("distance_jacobian_cutting_surface",
           &Disc::distance_jacobian_cutting_surface, "point"_a)
      .def("from_3d_to_2d", &Disc::from_3d_to_2d, "p3d"_a)
      .def("from_2d_to_3d", &Disc::from_2d_to_3d, "p2d"_a)
      .def("create_mesh", &Disc::create_mesh,
           "Create a triangular mesh of this primitive.");
}

void Cone_b(nb::module_ &m) {
  nb::class_<Cone, Primitive>(m, "Cone",
                              "Conical (frustum) surface segment defined by\n"
                              "axis, two radii, and angular extent.")
      .def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D,
                    double, double, double, double>(),
           "p1"_a, "p2"_a, "p3"_a, "radius1"_a, "radius2"_a,
           "start_angle"_a, "end_angle"_a,
           "Create a cone from axis points, reference, radii, and angles.")
      .def_prop_rw("p1", &Cone::get_p1, &Cone::set_p1,
                   "Base center position.")
      .def_prop_rw("p2", &Cone::get_p2, &Cone::set_p2,
                   "Top center position.")
      .def_prop_rw("p3", &Cone::get_p3, &Cone::set_p3,
                   "Reference point for angle origin.")
      .def_prop_rw("radius1", &Cone::get_radius1, &Cone::set_radius1,
                   "Radius at the base (p1 end).")
      .def_prop_rw("radius2", &Cone::get_radius2, &Cone::set_radius2,
                   "Radius at the top (p2 end).")
      .def_prop_rw("start_angle", &Cone::get_start_angle,
                    &Cone::set_start_angle,
                    "Start angle [rad].")
      .def_prop_rw("end_angle", &Cone::get_end_angle, &Cone::set_end_angle,
                   "End angle [rad].")
      .def("is_valid", &Cone::is_valid)
      .def("distance", &Cone::distance, "point"_a)
      .def("distance_jacobian_cutted_surface",
           &Cone::distance_jacobian_cutted_surface, "point"_a)
      .def("distance_jacobian_cutting_surface",
           &Cone::distance_jacobian_cutting_surface, "point"_a)
      .def("from_3d_to_2d", &Cone::from_3d_to_2d, "p3d"_a)
      .def("from_2d_to_3d", &Cone::from_2d_to_3d, "p2d"_a)
      .def("create_mesh", &Cone::create_mesh,
           "Create a triangular mesh of this primitive.");
}

void Sphere_b(nb::module_ &m) {
  nb::class_<Sphere, Primitive>(
      m, "Sphere",
      "Spherical surface segment with optional truncation\n"
      "at base and apex, and angular extent control.")
      .def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D,
                    double, double, double, double, double>(),
           "p1"_a, "p2"_a, "p3"_a, "radius"_a, "base_truncation"_a,
           "apex_truncation"_a, "start_angle"_a, "end_angle"_a,
           "Create a sphere segment from center, axis, reference, radius,\n"
           "truncation fractions, and angular extent.")
      .def_prop_rw("p1", &Sphere::get_p1, &Sphere::set_p1,
                   "Center position.")
      .def_prop_rw("p2", &Sphere::get_p2, &Sphere::set_p2,
                   "Axis direction reference point.")
      .def_prop_rw("p3", &Sphere::get_p3, &Sphere::set_p3,
                   "Reference point for angle origin.")
      .def_prop_rw("radius", &Sphere::get_radius, &Sphere::set_radius,
                   "Sphere radius.")
      .def_prop_rw("base_truncation", &Sphere::get_base_truncation,
                    &Sphere::set_base_truncation,
                    "Base truncation fraction (0 = no truncation).")
      .def_prop_rw("apex_truncation", &Sphere::get_apex_truncation,
                    &Sphere::set_apex_truncation,
                    "Apex truncation fraction (0 = no truncation).")
      .def_prop_rw("start_angle", &Sphere::get_start_angle,
                    &Sphere::set_start_angle,
                    "Start angle [rad].")
      .def_prop_rw("end_angle", &Sphere::get_end_angle, &Sphere::set_end_angle,
                   "End angle [rad].")
      .def("is_valid", &Sphere::is_valid)
      .def("distance", &Sphere::distance, "point"_a)
      .def("distance_jacobian_cutted_surface",
           &Sphere::distance_jacobian_cutted_surface, "point"_a)
      .def("distance_jacobian_cutting_surface",
           &Sphere::distance_jacobian_cutting_surface, "point"_a)
      .def("from_3d_to_2d", &Sphere::from_3d_to_2d, "p3d"_a)
      .def("from_2d_to_3d", &Sphere::from_2d_to_3d, "p2d"_a)
      .def("create_mesh", &Sphere::create_mesh,
           "Create a triangular mesh of this primitive.")
      .def("create_mesh2", &Sphere::create_mesh2,
           "Create a triangular mesh (alternative algorithm).");
}
