#pragma once
#include <Eigen/Geometry>
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>

#include <array>

#include "pycanha-core/globals.hpp"
#include "pycanha-core/gmm/primitives/primitive.hpp"

namespace nb = nanobind;
using namespace nanobind::literals;
using namespace pycanha::gmm;

// The 0.15 primitives are nine independent value classes (no common base). The
// Primitive variant (std::variant<Triangle, ...>) is handled transparently by
// <nanobind/stl/variant.h>, so a Python Triangle is accepted anywhere a
// Primitive is expected and getters returning a Primitive surface the concrete
// Python type. Each class exposes the same small surface: point / shape-param
// properties plus is_valid / to_uv / to_cartesian / normal_at_uv /
// surface_area.

namespace pycanha::bindings::gmm::detail {

// Attaches the surface interface shared by every primitive.
template <class C, class PyClass>
void add_common_surface(PyClass& cls) {
  cls.def("is_valid", &C::is_valid,
          "Whether the primitive geometry is valid.")
      .def("to_uv", &C::to_uv, "point"_a,
           "Project a 3D point onto the primitive's 2D parametric (uv) space.")
      .def("to_cartesian", &C::to_cartesian, "uv"_a,
           "Map a 2D parametric (uv) point back to 3D space.")
      .def("normal_at_uv", &C::normal_at_uv, "uv"_a,
           "Outward surface normal at a 2D parametric (uv) point.")
      .def("surface_area", &C::surface_area, "Total surface area.");
}

}  // namespace pycanha::bindings::gmm::detail

inline void Triangle_b(nb::module_& m) {
  auto cls = nb::class_<Triangle>(
      m, "Triangle", "Triangular flat surface defined by three 3D vertices.");
  cls.def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D>(),
          "p1"_a, "p2"_a, "p3"_a,
          "Create a triangle from three 3D vertex positions.")
      .def_prop_rw("p1", &Triangle::p1, &Triangle::set_p1, "First vertex.")
      .def_prop_rw("p2", &Triangle::p2, &Triangle::set_p2, "Second vertex.")
      .def_prop_rw("p3", &Triangle::p3, &Triangle::set_p3, "Third vertex.");
  pycanha::bindings::gmm::detail::add_common_surface<Triangle>(cls);
}

inline void Rectangle_b(nb::module_& m) {
  auto cls = nb::class_<Rectangle>(
      m, "Rectangle",
      "Rectangular flat surface defined by three vertices (the fourth is "
      "derived): p1->p2 is one edge, p1->p3 the adjacent edge.");
  cls.def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D>(),
          "p1"_a, "p2"_a, "p3"_a,
          "Create a rectangle from three corner positions.")
      .def_prop_rw("p1", &Rectangle::p1, &Rectangle::set_p1, "Origin vertex.")
      .def_prop_rw("p2", &Rectangle::p2, &Rectangle::set_p2,
                   "Second vertex (first edge from p1).")
      .def_prop_rw("p3", &Rectangle::p3, &Rectangle::set_p3,
                   "Third vertex (second edge from p1).");
  pycanha::bindings::gmm::detail::add_common_surface<Rectangle>(cls);
}

inline void Quadrilateral_b(nb::module_& m) {
  auto cls = nb::class_<Quadrilateral>(
      m, "Quadrilateral",
      "General quadrilateral surface defined by four vertices (may be "
      "non-planar).");
  cls.def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D,
                   pycanha::Point3D>(),
          "p1"_a, "p2"_a, "p3"_a, "p4"_a,
          "Create a quadrilateral from four 3D vertex positions.")
      .def_prop_rw("p1", &Quadrilateral::p1, &Quadrilateral::set_p1,
                   "First vertex.")
      .def_prop_rw("p2", &Quadrilateral::p2, &Quadrilateral::set_p2,
                   "Second vertex.")
      .def_prop_rw("p3", &Quadrilateral::p3, &Quadrilateral::set_p3,
                   "Third vertex.")
      .def_prop_rw("p4", &Quadrilateral::p4, &Quadrilateral::set_p4,
                   "Fourth vertex.");
  pycanha::bindings::gmm::detail::add_common_surface<Quadrilateral>(cls);
}

inline void Disc_b(nb::module_& m) {
  auto cls = nb::class_<Disc>(
      m, "Disc",
      "Annular disc (flat ring) segment defined by center, normal, "
      "inner/outer radii, and angular extent.");
  cls.def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D, double,
                   double, double, double>(),
          "p1"_a, "p2"_a, "p3"_a, "inner_radius"_a, "outer_radius"_a,
          "start_angle"_a, "end_angle"_a,
          "Create a disc from center, normal ref, radii, and angles.")
      .def_prop_rw("p1", &Disc::p1, &Disc::set_p1, "Center position.")
      .def_prop_rw("p2", &Disc::p2, &Disc::set_p2,
                   "Normal direction reference point.")
      .def_prop_rw("p3", &Disc::p3, &Disc::set_p3,
                   "Reference point for the angle origin.")
      .def_prop_rw("inner_radius", &Disc::inner_radius, &Disc::set_inner_radius,
                   "Inner radius (0 for a full disc).")
      .def_prop_rw("outer_radius", &Disc::outer_radius, &Disc::set_outer_radius,
                   "Outer radius.")
      .def_prop_rw("start_angle", &Disc::start_angle, &Disc::set_start_angle,
                   "Start angle [rad].")
      .def_prop_rw("end_angle", &Disc::end_angle, &Disc::set_end_angle,
                   "End angle [rad].");
  pycanha::bindings::gmm::detail::add_common_surface<Disc>(cls);
}

inline void Cylinder_b(nb::module_& m) {
  auto cls = nb::class_<Cylinder>(
      m, "Cylinder",
      "Cylindrical surface segment defined by axis, radius, and angular "
      "extent.");
  cls.def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D, double,
                   double, double>(),
          "p1"_a, "p2"_a, "p3"_a, "radius"_a, "start_angle"_a, "end_angle"_a,
          "Create a cylinder from axis points, reference, radius, and angles.")
      .def_prop_rw("p1", &Cylinder::p1, &Cylinder::set_p1,
                   "Base center position.")
      .def_prop_rw("p2", &Cylinder::p2, &Cylinder::set_p2,
                   "Top center position.")
      .def_prop_rw("p3", &Cylinder::p3, &Cylinder::set_p3,
                   "Reference point for the angle origin.")
      .def_prop_rw("radius", &Cylinder::radius, &Cylinder::set_radius,
                   "Cylinder radius.")
      .def_prop_rw("start_angle", &Cylinder::start_angle,
                   &Cylinder::set_start_angle, "Start angle [rad].")
      .def_prop_rw("end_angle", &Cylinder::end_angle, &Cylinder::set_end_angle,
                   "End angle [rad].");
  pycanha::bindings::gmm::detail::add_common_surface<Cylinder>(cls);
}

inline void Cone_b(nb::module_& m) {
  auto cls = nb::class_<Cone>(
      m, "Cone",
      "Conical (frustum) surface segment defined by axis, two radii, and "
      "angular extent.");
  cls.def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D, double,
                   double, double, double>(),
          "p1"_a, "p2"_a, "p3"_a, "radius1"_a, "radius2"_a, "start_angle"_a,
          "end_angle"_a,
          "Create a cone from axis points, reference, radii, and angles.")
      .def_prop_rw("p1", &Cone::p1, &Cone::set_p1, "Base center position.")
      .def_prop_rw("p2", &Cone::p2, &Cone::set_p2, "Top center position.")
      .def_prop_rw("p3", &Cone::p3, &Cone::set_p3,
                   "Reference point for the angle origin.")
      .def_prop_rw("radius1", &Cone::radius1, &Cone::set_radius1,
                   "Radius at the base (p1 end).")
      .def_prop_rw("radius2", &Cone::radius2, &Cone::set_radius2,
                   "Radius at the top (p2 end).")
      .def_prop_rw("start_angle", &Cone::start_angle, &Cone::set_start_angle,
                   "Start angle [rad].")
      .def_prop_rw("end_angle", &Cone::end_angle, &Cone::set_end_angle,
                   "End angle [rad].");
  pycanha::bindings::gmm::detail::add_common_surface<Cone>(cls);
}

inline void Sphere_b(nb::module_& m) {
  auto cls = nb::class_<Sphere>(
      m, "Sphere",
      "Spherical surface segment with optional base/apex truncation and "
      "angular extent.");
  cls.def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D, double,
                   double, double, double, double>(),
          "p1"_a, "p2"_a, "p3"_a, "radius"_a, "base_truncation"_a,
          "apex_truncation"_a, "start_angle"_a, "end_angle"_a,
          "Create a sphere segment from center, axis, reference, radius, "
          "truncations, and angular extent.")
      .def_prop_rw("p1", &Sphere::p1, &Sphere::set_p1, "Center position.")
      .def_prop_rw("p2", &Sphere::p2, &Sphere::set_p2,
                   "Axis direction reference point.")
      .def_prop_rw("p3", &Sphere::p3, &Sphere::set_p3,
                   "Reference point for the angle origin.")
      .def_prop_rw("radius", &Sphere::radius, &Sphere::set_radius,
                   "Sphere radius.")
      .def_prop_rw("base_truncation", &Sphere::base_truncation,
                   &Sphere::set_base_truncation,
                   "Base truncation (0 = no truncation).")
      .def_prop_rw("apex_truncation", &Sphere::apex_truncation,
                   &Sphere::set_apex_truncation,
                   "Apex truncation (0 = no truncation).")
      .def_prop_rw("start_angle", &Sphere::start_angle,
                   &Sphere::set_start_angle, "Start angle [rad].")
      .def_prop_rw("end_angle", &Sphere::end_angle, &Sphere::set_end_angle,
                   "End angle [rad].");
  pycanha::bindings::gmm::detail::add_common_surface<Sphere>(cls);
}

inline void Paraboloid_b(nb::module_& m) {
  auto cls = nb::class_<Paraboloid>(
      m, "Paraboloid",
      "Paraboloidal surface segment defined by axis, radius, and angular "
      "extent.");
  cls.def(nb::init<pycanha::Point3D, pycanha::Point3D, pycanha::Point3D, double,
                   double, double>(),
          "p1"_a, "p2"_a, "p3"_a, "radius"_a, "start_angle"_a, "end_angle"_a,
          "Create a paraboloid from axis points, reference, radius, and "
          "angles.")
      .def_prop_rw("p1", &Paraboloid::p1, &Paraboloid::set_p1,
                   "Base center position.")
      .def_prop_rw("p2", &Paraboloid::p2, &Paraboloid::set_p2,
                   "Apex direction reference point.")
      .def_prop_rw("p3", &Paraboloid::p3, &Paraboloid::set_p3,
                   "Reference point for the angle origin.")
      .def_prop_rw("radius", &Paraboloid::radius, &Paraboloid::set_radius,
                   "Rim radius.")
      .def_prop_rw("start_angle", &Paraboloid::start_angle,
                   &Paraboloid::set_start_angle, "Start angle [rad].")
      .def_prop_rw("end_angle", &Paraboloid::end_angle,
                   &Paraboloid::set_end_angle, "End angle [rad].");
  pycanha::bindings::gmm::detail::add_common_surface<Paraboloid>(cls);
}

inline void Cube_b(nb::module_& m) {
  // Cube carries an Eigen::Quaterniond orientation, for which nanobind's Eigen
  // support provides no caster. Expose it as a numpy (w, x, y, z) vector.
  // TODO(pycanha-core): consider a Cube overload taking a Matrix3d / Vector4d
  // so this quaternion glue can move out of the binding.
  auto to_quat = [](const Eigen::Vector4d& wxyz) {
    return Eigen::Quaterniond(wxyz[0], wxyz[1], wxyz[2], wxyz[3]);
  };
  auto from_quat = [](const Eigen::Quaterniond& q) {
    return Eigen::Vector4d(q.w(), q.x(), q.y(), q.z());
  };

  nb::class_<Cube>(m, "Cube",
                   "Axis-aligned box (rotated by an orientation quaternion) "
                   "usable as a closed-solid cutter.")
      .def(
          "__init__",
          [to_quat](Cube* self, pycanha::Point3D center, pycanha::Vector3D extent,
                    const Eigen::Vector4d& orientation_wxyz) {
            new (self) Cube(std::move(center), std::move(extent),
                            to_quat(orientation_wxyz));
          },
          "center"_a, "extent"_a,
          "orientation"_a = Eigen::Vector4d(1.0, 0.0, 0.0, 0.0),
          "Create a cube from center, full extents, and an (w, x, y, z) "
          "orientation quaternion.")
      .def_prop_rw("center", &Cube::center, &Cube::set_center,
                   "Box center position.")
      .def_prop_rw("extent", &Cube::extent, &Cube::set_extent,
                   "Full extents along the local x, y, z axes.")
      .def_prop_rw(
          "orientation",
          [from_quat](const Cube& self) { return from_quat(self.orientation()); },
          [to_quat](Cube& self, const Eigen::Vector4d& wxyz) {
            self.set_orientation(to_quat(wxyz));
          },
          "Orientation quaternion as a numpy (w, x, y, z) vector.")
      .def("is_valid", &Cube::is_valid, "Whether the cube geometry is valid.")
      .def("to_uv", &Cube::to_uv, "point"_a)
      .def("to_cartesian", &Cube::to_cartesian, "uv"_a)
      .def("normal_at_uv", &Cube::normal_at_uv, "uv"_a)
      .def("surface_area", &Cube::surface_area, "Total surface area.");
}
