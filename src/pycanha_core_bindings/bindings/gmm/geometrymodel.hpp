
#pragma once
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "pycanha-core/gmm/geometrymodel.hpp"

namespace nb = nanobind;
using namespace nanobind::literals;
using namespace pycanha::gmm;

void GeometryModel_b(nb::module_ &m) {
  nb::class_<GeometryModel>(m, "GeometryModel",
                            "Top-level container for the complete geometry model.\n\n"
                            "Manages a hierarchy of geometry items and groups,\n"
                            "provides factory methods for creating them, and\n"
                            "generates the unified TriMeshModel for analysis.")
      .def(nb::init<>(), "Create an empty geometry model.")
      .def(nb::init<std::string>(), "name"_a,
           "Create a geometry model with the given name.")
      .def("create_geometry_item", &GeometryModel::create_geometry_item,
           "name"_a, "primitive"_a, "transformation"_a, "thermal_mesh"_a,
           "Create and register a GeometryMeshedItem.")
      .def("create_geometry_group", &GeometryModel::create_geometry_group,
           "name"_a, "geometries"_a, "transformation"_a,
           "Create and register a GeometryGroup.")
      .def("create_geometry_group_cutted",
           &GeometryModel::create_geometry_group_cutted, "name"_a,
           "geometries"_a, "cutting_geometry_items"_a, "transformation"_a,
           "Create and register a GeometryGroupCutted.")
      .def("callback_primitive_changed",
           &GeometryModel::callback_primitive_changed,
           "Notify the model that a primitive has been modified.")
      .def("get_root_geometry_group", &GeometryModel::get_root_geometry_group,
           "Get the root GeometryGroup of the hierarchy.")
      .def("create_mesh", &GeometryModel::create_mesh,
           "Generate triangle meshes for all geometry items.")
      .def("copy_mesh", &GeometryModel::copy_mesh,
           "Copy meshes into the unified TriMeshModel.")
      .def("get_trimesh_model", &GeometryModel::get_trimesh_model,
           "Get the unified TriMeshModel for rendering/analysis.");
}
