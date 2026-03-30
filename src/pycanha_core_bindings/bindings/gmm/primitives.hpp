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
using namespace pycanha::gmm;

void Primitive_b(nb::module_ &m) {
  nb::class_<Primitive>(m, "Primitive")
      .def("distance", &Primitive::distance)
      .def("distance_jacobian_cutted_surface",
           &Primitive::distance_jacobian_cutted_surface)
      .def("distance_jacobian_cutting_surface",
           &Primitive::distance_jacobian_cutting_surface)
      .def("is_valid", &Primitive::is_valid)
      .def("from_2d_to_3d", &Primitive::from_2d_to_3d)
      .def("from_3d_to_2d", &Primitive::from_3d_to_2d);
}

void Triangle_b(nb::module_ &m) {
  nb::class_<Triangle, Primitive>(m, "Triangle")
      .def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D>())
      .def_prop_rw("p1", &Triangle::get_p1, &Triangle::set_p1)
      .def_prop_rw("p2", &Triangle::get_p2, &Triangle::set_p2)
      .def_prop_rw("p3", &Triangle::get_p3, &Triangle::set_p3)
      .def("v1", &Triangle::v1)
      .def("v2", &Triangle::v2)
      .def("is_valid", &Triangle::is_valid)
      .def("distance", &Triangle::distance)
      .def("distance_jacobian_cutted_surface",
           &Triangle::distance_jacobian_cutted_surface)
      .def("distance_jacobian_cutting_surface",
           &Triangle::distance_jacobian_cutting_surface)
      .def("from_3d_to_2d", &Triangle::from_3d_to_2d)
      .def("from_2d_to_3d", &Triangle::from_2d_to_3d)
      .def("create_mesh", &Triangle::create_mesh);
}

void Rectangle_b(nb::module_ &m) {
  nb::class_<Rectangle, Primitive>(m, "Rectangle")
      .def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D>())
      .def_prop_rw("p1", &Rectangle::get_p1, &Rectangle::set_p1)
      .def_prop_rw("p2", &Rectangle::get_p2, &Rectangle::set_p2)
      .def_prop_rw("p3", &Rectangle::get_p3, &Rectangle::set_p3)
      .def("v1", &Rectangle::v1)
      .def("v2", &Rectangle::v2)
      .def("is_valid", &Rectangle::is_valid)
      .def("distance", &Rectangle::distance)
      .def("distance_jacobian_cutted_surface",
           &Rectangle::distance_jacobian_cutted_surface)
      .def("distance_jacobian_cutting_surface",
           &Rectangle::distance_jacobian_cutting_surface)
      .def("from_3d_to_2d", &Rectangle::from_3d_to_2d)
      .def("from_2d_to_3d", &Rectangle::from_2d_to_3d)
      .def("create_mesh", &Rectangle::create_mesh);
}

void Quadrilateral_b(nb::module_ &m) {
  nb::class_<Quadrilateral, Primitive>(
      m, "Quadrilateral")
      .def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D,
                    pycanha::Point3D>())
      .def_prop_rw("p1", &Quadrilateral::get_p1, &Quadrilateral::set_p1)
      .def_prop_rw("p2", &Quadrilateral::get_p2, &Quadrilateral::set_p2)
      .def_prop_rw("p3", &Quadrilateral::get_p3, &Quadrilateral::set_p3)
      .def_prop_rw("p4", &Quadrilateral::get_p4, &Quadrilateral::set_p4)
      .def("v1", &Quadrilateral::v1)
      .def("v2", &Quadrilateral::v2)
      .def("is_valid", &Quadrilateral::is_valid)
      .def("distance", &Quadrilateral::distance)
      .def("distance_jacobian_cutted_surface",
           &Quadrilateral::distance_jacobian_cutted_surface)
      .def("distance_jacobian_cutting_surface",
           &Quadrilateral::distance_jacobian_cutting_surface)
      .def("from_3d_to_2d", &Quadrilateral::from_3d_to_2d)
      .def("from_2d_to_3d", &Quadrilateral::from_2d_to_3d)
      .def("create_mesh", &Quadrilateral::create_mesh);
}

void Cylinder_b(nb::module_ &m) {
  nb::class_<Cylinder, Primitive>(m, "Cylinder")
      .def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D,
                    double, double, double>())
      .def_prop_rw("p1", &Cylinder::get_p1, &Cylinder::set_p1)
      .def_prop_rw("p2", &Cylinder::get_p2, &Cylinder::set_p2)
      .def_prop_rw("p3", &Cylinder::get_p3, &Cylinder::set_p3)
      .def_prop_rw("radius", &Cylinder::get_radius, &Cylinder::set_radius)
      .def_prop_rw("start_angle", &Cylinder::get_start_angle,
                    &Cylinder::set_start_angle)
      .def_prop_rw("end_angle", &Cylinder::get_end_angle,
                    &Cylinder::set_end_angle)
      .def("is_valid", &Cylinder::is_valid)
      .def("distance", &Cylinder::distance)
      .def("distance_jacobian_cutted_surface",
           &Cylinder::distance_jacobian_cutted_surface)
      .def("distance_jacobian_cutting_surface",
           &Cylinder::distance_jacobian_cutting_surface)
      .def("from_3d_to_2d", &Cylinder::from_3d_to_2d)
      .def("from_2d_to_3d", &Cylinder::from_2d_to_3d)
      .def("create_mesh", &Cylinder::create_mesh);
}

void Disc_b(nb::module_ &m) {
  nb::class_<Disc, Primitive>(m, "Disc")
      .def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D,
                    double, double, double, double>())
      .def_prop_rw("p1", &Disc::get_p1, &Disc::set_p1)
      .def_prop_rw("p2", &Disc::get_p2, &Disc::set_p2)
      .def_prop_rw("p3", &Disc::get_p3, &Disc::set_p3)
      .def_prop_rw("inner_radius", &Disc::get_inner_radius,
                    &Disc::set_inner_radius)
      .def_prop_rw("outer_radius", &Disc::get_outer_radius,
                    &Disc::set_outer_radius)
      .def_prop_rw("start_angle", &Disc::get_start_angle,
                    &Disc::set_start_angle)
      .def_prop_rw("end_angle", &Disc::get_end_angle, &Disc::set_end_angle)
      .def("is_valid", &Disc::is_valid)
      .def("distance", &Disc::distance)
      .def("distance_jacobian_cutted_surface",
           &Disc::distance_jacobian_cutted_surface)
      .def("distance_jacobian_cutting_surface",
           &Disc::distance_jacobian_cutting_surface)
      .def("from_3d_to_2d", &Disc::from_3d_to_2d)
      .def("from_2d_to_3d", &Disc::from_2d_to_3d)
      .def("create_mesh", &Disc::create_mesh);
}

void Cone_b(nb::module_ &m) {
  nb::class_<Cone, Primitive>(m, "Cone")
      .def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D,
                    double, double, double, double>())
      .def_prop_rw("p1", &Cone::get_p1, &Cone::set_p1)
      .def_prop_rw("p2", &Cone::get_p2, &Cone::set_p2)
      .def_prop_rw("p3", &Cone::get_p3, &Cone::set_p3)
      .def_prop_rw("radius1", &Cone::get_radius1, &Cone::set_radius1)
      .def_prop_rw("radius2", &Cone::get_radius2, &Cone::set_radius2)
      .def_prop_rw("start_angle", &Cone::get_start_angle,
                    &Cone::set_start_angle)
      .def_prop_rw("end_angle", &Cone::get_end_angle, &Cone::set_end_angle)
      .def("is_valid", &Cone::is_valid)
      .def("distance", &Cone::distance)
      .def("distance_jacobian_cutted_surface",
           &Cone::distance_jacobian_cutted_surface)
      .def("distance_jacobian_cutting_surface",
           &Cone::distance_jacobian_cutting_surface)
      .def("from_3d_to_2d", &Cone::from_3d_to_2d)
      .def("from_2d_to_3d", &Cone::from_2d_to_3d)
      .def("create_mesh", &Cone::create_mesh);
}

void Sphere_b(nb::module_ &m) {
  nb::class_<Sphere, Primitive>(m, "Sphere")
      .def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D,
                    double, double, double, double, double>())
      .def_prop_rw("p1", &Sphere::get_p1, &Sphere::set_p1)
      .def_prop_rw("p2", &Sphere::get_p2, &Sphere::set_p2)
      .def_prop_rw("p3", &Sphere::get_p3, &Sphere::set_p3)
      .def_prop_rw("radius", &Sphere::get_radius, &Sphere::set_radius)
      .def_prop_rw("base_truncation", &Sphere::get_base_truncation,
                    &Sphere::set_base_truncation)
      .def_prop_rw("apex_truncation", &Sphere::get_apex_truncation,
                    &Sphere::set_apex_truncation)
      .def_prop_rw("start_angle", &Sphere::get_start_angle,
                    &Sphere::set_start_angle)
      .def_prop_rw("end_angle", &Sphere::get_end_angle, &Sphere::set_end_angle)
      .def("is_valid", &Sphere::is_valid)
      .def("distance", &Sphere::distance)
      .def("distance_jacobian_cutted_surface",
           &Sphere::distance_jacobian_cutted_surface)
      .def("distance_jacobian_cutting_surface",
           &Sphere::distance_jacobian_cutting_surface)
      .def("from_3d_to_2d", &Sphere::from_3d_to_2d)
      .def("from_2d_to_3d", &Sphere::from_2d_to_3d)
      .def("create_mesh", &Sphere::create_mesh)
      .def("create_mesh2", &Sphere::create_mesh2);
}
