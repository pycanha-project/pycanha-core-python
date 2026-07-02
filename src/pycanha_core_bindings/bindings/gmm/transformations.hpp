#pragma once
#include <Eigen/Geometry>
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>

#include "pycanha-core/globals.hpp"
#include "pycanha-core/gmm/scene/coordinate_transformation.hpp"

namespace nb = nanobind;
using namespace nanobind::literals;
using namespace pycanha::gmm;

// Value-type rigid transformation (translation + rotation matrix). Built either
// directly or via the from_* named constructors, and composed with compose() /
// inverse().

inline void CoordinateTransformation_b(nb::module_& m) {
  nb::class_<CoordinateTransformation>(
      m, "CoordinateTransformation",
      "Rigid 3D transformation (translation + 3x3 rotation matrix).")
      .def(nb::init<>(), "Create an identity transformation.")
      .def(nb::init<pycanha::Vector3D, Eigen::Matrix3d>(), "translation"_a,
           "rotation"_a,
           "Create from a translation vector and a 3x3 rotation matrix.")
      .def_static("from_translation",
                  &CoordinateTransformation::from_translation, "translation"_a,
                  "Pure-translation transformation.")
      .def_static("from_euler", &CoordinateTransformation::from_euler,
                  "translation"_a, "euler_xyz"_a,
                  "Transformation from a translation and XYZ Euler angles.")
      // from_rotation takes an Eigen::Quaterniond, for which nanobind provides
      // no caster; accept a numpy (w, x, y, z) vector.
      // TODO(pycanha-core): a from_rotation overload taking a Matrix3d /
      // Vector4d would remove this quaternion glue.
      .def_static(
          "from_rotation",
          [](const Eigen::Vector4d& wxyz) {
            return CoordinateTransformation::from_rotation(
                Eigen::Quaterniond(wxyz[0], wxyz[1], wxyz[2], wxyz[3]));
          },
          "rotation"_a,
          "Pure-rotation transformation from an (w, x, y, z) quaternion.")
      .def_prop_rw("translation", &CoordinateTransformation::translation,
                   &CoordinateTransformation::set_translation,
                   "Translation vector [x, y, z].")
      .def_prop_rw("rotation", &CoordinateTransformation::rotation,
                   &CoordinateTransformation::set_rotation,
                   "3x3 rotation matrix.")
      .def("is_identity", &CoordinateTransformation::is_identity,
           "Whether this is the identity transformation.")
      .def("apply", &CoordinateTransformation::apply, "point"_a,
           "Apply the full transformation to a 3D point.")
      .def("apply_normal", &CoordinateTransformation::apply_normal, "normal"_a,
           "Apply only the rotation to a 3D direction/normal.")
      .def("compose", &CoordinateTransformation::compose, "outer"_a,
           "Compose with an outer transformation (outer applied after this).")
      .def("inverse", &CoordinateTransformation::inverse,
           "The inverse transformation.")
      .def("linear", &CoordinateTransformation::linear,
           "The linear (rotation) part as a 3x3 matrix.");
}
