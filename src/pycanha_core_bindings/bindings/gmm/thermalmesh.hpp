#pragma once
#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <vector>

#include "pycanha-core/gmm/thermalmesh.hpp"

namespace py = pybind11;
using namespace pybind11::literals;  // for _a shorthand

using namespace pycanha;

using namespace gmm;

void ThermalMesh_b(py::module_ &m) {
  py::class_<ThermalMesh, std::shared_ptr<ThermalMesh>>(m, "ThermalMesh")
      .def(py::init<>())
      .def_property("side1_activity", &ThermalMesh::get_side1_activity,
                    &ThermalMesh::set_side1_activity)
      .def_property("side2_activity", &ThermalMesh::get_side2_activity,
                    &ThermalMesh::set_side2_activity)
      .def_property("side1_thick", &ThermalMesh::get_side1_thick,
                    &ThermalMesh::set_side1_thick)
      .def_property("side2_thick", &ThermalMesh::get_side2_thick,
                    &ThermalMesh::set_side2_thick)
      .def_property("side1_color", &ThermalMesh::get_side1_color,
                    &ThermalMesh::set_side1_color)
      .def_property("side2_color", &ThermalMesh::get_side2_color,
                    &ThermalMesh::set_side2_color)
      .def_property("side1_material", &ThermalMesh::get_side1_material,
                    &ThermalMesh::set_side1_material)
      .def_property("side2_material", &ThermalMesh::get_side2_material,
                    &ThermalMesh::set_side2_material)
      .def_property("side1_optical", &ThermalMesh::get_side1_optical,
                    &ThermalMesh::set_side1_optical)
      .def_property("side2_optical", &ThermalMesh::get_side2_optical,
                    &ThermalMesh::set_side2_optical)
      .def_property("dir1_mesh", &ThermalMesh::get_dir1_mesh,
                    &ThermalMesh::set_dir1_mesh)
      .def_property("dir2_mesh", &ThermalMesh::get_dir2_mesh,
                    &ThermalMesh::set_dir2_mesh)
      .def("is_valid", &ThermalMesh::is_valid);
}
