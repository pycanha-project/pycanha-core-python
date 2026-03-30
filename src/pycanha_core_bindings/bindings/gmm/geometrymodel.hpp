
#pragma once
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>

#include "pycanha-core/gmm/geometrymodel.hpp"

namespace nb = nanobind;
using namespace pycanha::gmm;

void GeometryModel_b(nb::module_ &m) {
  nb::class_<GeometryModel>(m, "GeometryModel")
      .def(nb::init<>())
      .def(nb::init<std::string>())
      .def("create_geometry_item", &GeometryModel::create_geometry_item)
      .def("create_geometry_group", &GeometryModel::create_geometry_group)
      .def("create_geometry_group_cutted",
           &GeometryModel::create_geometry_group_cutted)
      .def("callback_primitive_changed",
           &GeometryModel::callback_primitive_changed)
      .def("get_root_geometry_group", &GeometryModel::get_root_geometry_group)
      .def("create_mesh", &GeometryModel::create_mesh)
      .def("copy_mesh", &GeometryModel::copy_mesh)
      .def("get_trimesh_model", &GeometryModel::get_trimesh_model);
}
