#pragma once
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "pycanha-core/gmm/transformations.hpp"

namespace py = pybind11;
using namespace pycanha::gmm;

void CoordinateTransformation_b(py::module_ &m) {
  // Bind the TransformOrder enum class
  py::enum_<TransformOrder>(m, "TransformOrder")
      .value("TRANSLATION_THEN_ROTATION",
             TransformOrder::TRANSLATION_THEN_ROTATION)
      .value("ROTATION_THEN_TRANSLATION",
             TransformOrder::ROTATION_THEN_TRANSLATION)
      .export_values();

  // Bind the CoordinateTransformation class
  py::class_<CoordinateTransformation,
             std::shared_ptr<CoordinateTransformation>>(
      m, "CoordinateTransformation")
      .def(py::init<>())
      .def(py::init<Vector3D, Vector3D, TransformOrder>(),
           py::arg("translation") = Vector3D::Zero(),
           py::arg("rotation") = Vector3D::Zero(),
           py::arg("order") = TransformOrder::TRANSLATION_THEN_ROTATION)
      .def("transform_point", &CoordinateTransformation::transform_point)
      .def_property("translation", &CoordinateTransformation::get_translation,
                    &CoordinateTransformation::set_translation)
      .def_property("rotation", &CoordinateTransformation::get_rotation_matrix,
                    &CoordinateTransformation::set_rotation_matrix)
      .def_property("order", &CoordinateTransformation::get_order,
                    &CoordinateTransformation::set_order);
}
