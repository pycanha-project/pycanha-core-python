#pragma once
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <vector>

#include "pycanha-core/gmm/thermalmesh.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // for _a shorthand

using namespace pycanha;

using namespace gmm;

void ThermalMesh_b(nb::module_ &m) {
  nb::class_<ThermalMesh>(m, "ThermalMesh",
                          "Thermal properties for a geometry item.")
      .def(nb::init<>())
      .def_prop_rw("side1_activity", &ThermalMesh::get_side1_activity,
                   &ThermalMesh::set_side1_activity)
      .def_prop_rw("side2_activity", &ThermalMesh::get_side2_activity,
                   &ThermalMesh::set_side2_activity)
      .def_prop_rw("side1_thick", &ThermalMesh::get_side1_thick,
                   &ThermalMesh::set_side1_thick)
      .def_prop_rw("side2_thick", &ThermalMesh::get_side2_thick,
                   &ThermalMesh::set_side2_thick)
      .def_prop_rw("side1_color", &ThermalMesh::get_side1_color,
                   &ThermalMesh::set_side1_color)
      .def_prop_rw("side2_color", &ThermalMesh::get_side2_color,
                   &ThermalMesh::set_side2_color)
      .def_prop_rw("side1_material", &ThermalMesh::get_side1_material,
                   &ThermalMesh::set_side1_material)
      .def_prop_rw("side2_material", &ThermalMesh::get_side2_material,
                   &ThermalMesh::set_side2_material)
      .def_prop_rw("side1_optical", &ThermalMesh::get_side1_optical,
                   &ThermalMesh::set_side1_optical)
      .def_prop_rw("side2_optical", &ThermalMesh::get_side2_optical,
                   &ThermalMesh::set_side2_optical)
      .def_prop_rw("dir1_mesh", &ThermalMesh::get_dir1_mesh,
                   &ThermalMesh::set_dir1_mesh)
      .def_prop_rw("dir2_mesh", &ThermalMesh::get_dir2_mesh,
                   &ThermalMesh::set_dir2_mesh)
      .def("is_valid", &ThermalMesh::is_valid);
}
