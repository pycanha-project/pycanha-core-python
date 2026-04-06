#pragma once
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "pycanha-core/gmm/transformations.hpp"

namespace nb = nanobind;
using namespace nanobind::literals;
using namespace pycanha::gmm;

void CoordinateTransformation_b(nb::module_ &m) {
  nb::enum_<TransformOrder>(m, "TransformOrder",
                            "Order of translation and rotation operations.")
      .value("TRANSLATION_THEN_ROTATION",
             TransformOrder::TRANSLATION_THEN_ROTATION)
      .value("ROTATION_THEN_TRANSLATION",
             TransformOrder::ROTATION_THEN_TRANSLATION)
      .export_values();

  nb::class_<CoordinateTransformation>(
      m, "CoordinateTransformation",
      "3D coordinate transformation combining translation\n"
      "and rotation, with configurable application order.")
      .def(nb::init<>(), "Create an identity transformation.")
      .def(nb::init<Vector3D, Vector3D, TransformOrder>(),
           nb::arg("translation") = Vector3D::Zero(),
           nb::arg("rotation") = Vector3D::Zero(),
           nb::arg("order") = TransformOrder::TRANSLATION_THEN_ROTATION,
           "Create a transformation from translation vector, rotation\n"
           "angles (XYZ Euler), and application order.")
      .def("transform_point", &CoordinateTransformation::transform_point,
           "point"_a, "Apply the transformation to a 3D point.")
      .def_prop_rw("translation", &CoordinateTransformation::get_translation,
                   &CoordinateTransformation::set_translation,
                   "Translation vector [x, y, z].")
      .def_prop_rw("rotation", &CoordinateTransformation::get_rotation_matrix,
                   &CoordinateTransformation::set_rotation_matrix,
                   "3x3 rotation matrix.")
      .def_prop_rw("order", &CoordinateTransformation::get_order,
                   &CoordinateTransformation::set_order,
                   "Order of translation and rotation operations.");
}
