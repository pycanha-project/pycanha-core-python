#pragma once
#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <memory>

#include "pycanha-core/gmm/primitives.hpp"

namespace py = pybind11;
using namespace pycanha::gmm;

void Primitive_b(py::module_ &m) {
  py::class_<Primitive, std::shared_ptr<Primitive>>(m, "Primitive")
      .def("distance", &Primitive::distance)
      .def("distance_jacobian_cutted_surface",
           &Primitive::distance_jacobian_cutted_surface)
      .def("distance_jacobian_cutting_surface",
           &Primitive::distance_jacobian_cutting_surface)
      .def("is_valid", &Primitive::is_valid)
      .def("from_2d_to_3d", &Primitive::from_2d_to_3d)
      .def("from_3d_to_2d", &Primitive::from_3d_to_2d);
}

void Triangle_b(py::module_ &m) {
  py::class_<Triangle, Primitive, std::shared_ptr<Triangle>>(m, "Triangle")
      .def(py::init<Point3D, Point3D, Point3D>())
      .def_property("p1", &Triangle::get_p1, &Triangle::set_p1)
      .def_property("p2", &Triangle::get_p2, &Triangle::set_p2)
      .def_property("p3", &Triangle::get_p3, &Triangle::set_p3)
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

void Rectangle_b(py::module_ &m) {
  py::class_<Rectangle, Primitive, std::shared_ptr<Rectangle>>(m, "Rectangle")
      .def(py::init<Point3D, Point3D, Point3D>())
      .def_property("p1", &Rectangle::get_p1, &Rectangle::set_p1)
      .def_property("p2", &Rectangle::get_p2, &Rectangle::set_p2)
      .def_property("p3", &Rectangle::get_p3, &Rectangle::set_p3)
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

void Quadrilateral_b(py::module_ &m) {
  py::class_<Quadrilateral, Primitive, std::shared_ptr<Quadrilateral>>(
      m, "Quadrilateral")
      .def(py::init<Point3D, Point3D, Point3D, Point3D>())
      .def_property("p1", &Quadrilateral::get_p1, &Quadrilateral::set_p1)
      .def_property("p2", &Quadrilateral::get_p2, &Quadrilateral::set_p2)
      .def_property("p3", &Quadrilateral::get_p3, &Quadrilateral::set_p3)
      .def_property("p4", &Quadrilateral::get_p4, &Quadrilateral::set_p4)
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

void Cylinder_b(py::module_ &m) {
  py::class_<Cylinder, Primitive, std::shared_ptr<Cylinder>>(m, "Cylinder")
      .def(py::init<Point3D, Point3D, Point3D, double, double, double>())
      .def_property("p1", &Cylinder::get_p1, &Cylinder::set_p1)
      .def_property("p2", &Cylinder::get_p2, &Cylinder::set_p2)
      .def_property("p3", &Cylinder::get_p3, &Cylinder::set_p3)
      .def_property("radius", &Cylinder::get_radius, &Cylinder::set_radius)
      .def_property("start_angle", &Cylinder::get_start_angle,
                    &Cylinder::set_start_angle)
      .def_property("end_angle", &Cylinder::get_end_angle,
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

void Disc_b(py::module_ &m) {
  py::class_<Disc, Primitive, std::shared_ptr<Disc>>(m, "Disc")
      .def(py::init<Point3D, Point3D, Point3D, double, double, double, double>())
      .def_property("p1", &Disc::get_p1, &Disc::set_p1)
      .def_property("p2", &Disc::get_p2, &Disc::set_p2)
      .def_property("p3", &Disc::get_p3, &Disc::set_p3)
      .def_property("inner_radius", &Disc::get_inner_radius,
                    &Disc::set_inner_radius)
      .def_property("outer_radius", &Disc::get_outer_radius,
                    &Disc::set_outer_radius)
      .def_property("start_angle", &Disc::get_start_angle,
                    &Disc::set_start_angle)
      .def_property("end_angle", &Disc::get_end_angle,
                    &Disc::set_end_angle)
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

void Cone_b(py::module_ &m) {
  py::class_<Cone, Primitive, std::shared_ptr<Cone>>(m, "Cone")
      .def(py::init<Point3D, Point3D, Point3D, double, double, double, double>())
      .def_property("p1", &Cone::get_p1, &Cone::set_p1)
      .def_property("p2", &Cone::get_p2, &Cone::set_p2)
      .def_property("p3", &Cone::get_p3, &Cone::set_p3)
      .def_property("radius1", &Cone::get_radius1,
                    &Cone::set_radius1)
      .def_property("radius2", &Cone::get_radius2,
                    &Cone::set_radius2)
      .def_property("start_angle", &Cone::get_start_angle,
                    &Cone::set_start_angle)
      .def_property("end_angle", &Cone::get_end_angle,
                    &Cone::set_end_angle)
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

void Sphere_b(py::module_ &m) {
  py::class_<Sphere, Primitive, std::shared_ptr<Sphere>>(m, "Sphere")
      .def(py::init<Point3D, Point3D, Point3D, double, double, double, double, double>())
      .def_property("p1", &Sphere::get_p1, &Sphere::set_p1)
      .def_property("p2", &Sphere::get_p2, &Sphere::set_p2)
      .def_property("p3", &Sphere::get_p3, &Sphere::set_p3)
      .def_property("radius", &Sphere::get_radius,
                    &Sphere::set_radius)
      .def_property("base_truncation", &Sphere::get_base_truncation,
                    &Sphere::set_base_truncation)
      .def_property("apex_truncation", &Sphere::get_apex_truncation,
                    &Sphere::set_apex_truncation)
      .def_property("start_angle", &Sphere::get_start_angle,
                    &Sphere::set_start_angle)
      .def_property("end_angle", &Sphere::get_end_angle,
                    &Sphere::set_end_angle)
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
