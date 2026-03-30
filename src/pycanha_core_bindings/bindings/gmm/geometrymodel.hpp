
#pragma once
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/shared_ptr.h>

#include "pycanha-core/gmm/geometrymodel.hpp"

namespace nb = nanobind;
using namespace nanobind::literals;
using namespace pycanha::gmm;

void GeometryModel_b(nb::module_ &m) {
  nb::class_<GeometryModel>(m, "GeometryModel",
                             "Top-level container for the geometry model.")
      .def(nb::init<>())
      .def(nb::init<std::string>(), "name"_a)
      .def("create_geometry_item", &GeometryModel::create_geometry_item,
           "name"_a, "primitive"_a, "transformation"_a, "thermal_mesh"_a)
      .def("create_geometry_group", &GeometryModel::create_geometry_group,
           "name"_a)
      .def("create_geometry_group_cutted",
           &GeometryModel::create_geometry_group_cutted, "name"_a)
      .def("callback_primitive_changed",
           &GeometryModel::callback_primitive_changed)
      .def("get_root_geometry_group", &GeometryModel::get_root_geometry_group)
      .def("create_mesh", &GeometryModel::create_mesh,
           "Mesh all geometry items.")
      .def("copy_mesh", &GeometryModel::copy_mesh,
           "Copy meshes into the unified TriMeshModel.")
      .def("get_trimesh_model", &GeometryModel::get_trimesh_model);
}
