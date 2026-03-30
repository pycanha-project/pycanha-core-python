#pragma once
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "pycanha-core/gmm/transformations.hpp"

namespace nb = nanobind;
using namespace pycanha::gmm;

void CoordinateTransformation_b(nb::module_ &m) {
  // Bind the TransformOrder enum class
  nb::enum_<TransformOrder>(m, "TransformOrder")
      .value("TRANSLATION_THEN_ROTATION",
             TransformOrder::TRANSLATION_THEN_ROTATION)
      .value("ROTATION_THEN_TRANSLATION",
             TransformOrder::ROTATION_THEN_TRANSLATION)
      .export_values();

  // Bind the CoordinateTransformation class
  nb::class_<CoordinateTransformation>(m, "CoordinateTransformation")
      .def(nb::init<>())
      .def(nb::init<Vector3D, Vector3D, TransformOrder>(),
           nb::arg("translation") = Vector3D::Zero(),
           nb::arg("rotation") = Vector3D::Zero(),
           nb::arg("order") = TransformOrder::TRANSLATION_THEN_ROTATION)
      .def("transform_point", &CoordinateTransformation::transform_point)
      .def_prop_rw("translation", &CoordinateTransformation::get_translation,
                   &CoordinateTransformation::set_translation)
      .def_prop_rw("rotation", &CoordinateTransformation::get_rotation_matrix,
                   &CoordinateTransformation::set_rotation_matrix)
      .def_prop_rw("order", &CoordinateTransformation::get_order,
                   &CoordinateTransformation::set_order);
}
