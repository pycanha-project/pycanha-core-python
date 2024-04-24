
#pragma once
#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "pycanha-core/gmm/geometrymodel.hpp"

namespace py = pybind11;
using namespace pycanha::gmm;

void GeometryModel_b(py::module_ &m) {
  py::class_<GeometryModel, std::shared_ptr<GeometryModel>>(m, "GeometryModel")
      .def(py::init<>())
      .def(py::init<std::string>())
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
