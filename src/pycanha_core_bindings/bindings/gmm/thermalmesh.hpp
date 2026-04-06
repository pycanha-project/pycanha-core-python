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
                          "Thermal and mesh properties for a geometry surface.\n\n"
                          "Defines two-sided surface properties (activity,\n"
                          "thickness, color, bulk material, optical material)\n"
                          "and mesh discretization in two parametric directions.")
      .def(nb::init<>(), "Create a ThermalMesh with default properties.")
      .def_prop_rw("side1_activity", &ThermalMesh::get_side1_activity,
                   &ThermalMesh::set_side1_activity,
                   "Whether side 1 is thermally active.")
      .def_prop_rw("side2_activity", &ThermalMesh::get_side2_activity,
                   &ThermalMesh::set_side2_activity,
                   "Whether side 2 is thermally active.")
      .def_prop_rw("side1_thick", &ThermalMesh::get_side1_thick,
                   &ThermalMesh::set_side1_thick,
                   "Thickness of side 1.")
      .def_prop_rw("side2_thick", &ThermalMesh::get_side2_thick,
                   &ThermalMesh::set_side2_thick,
                   "Thickness of side 2.")
      .def_prop_rw("side1_color", &ThermalMesh::get_side1_color,
                   &ThermalMesh::set_side1_color,
                   "RGB color of side 1.")
      .def_prop_rw("side2_color", &ThermalMesh::get_side2_color,
                   &ThermalMesh::set_side2_color,
                   "RGB color of side 2.")
      .def_prop_rw("side1_material", &ThermalMesh::get_side1_material,
                   &ThermalMesh::set_side1_material,
                   "Bulk material (density, specific heat, conductivity) of side 1.")
      .def_prop_rw("side2_material", &ThermalMesh::get_side2_material,
                   &ThermalMesh::set_side2_material,
                   "Bulk material (density, specific heat, conductivity) of side 2.")
      .def_prop_rw("side1_optical", &ThermalMesh::get_side1_optical,
                   &ThermalMesh::set_side1_optical,
                   "Optical material (emissivity/absorptivity) of side 1.")
      .def_prop_rw("side2_optical", &ThermalMesh::get_side2_optical,
                   &ThermalMesh::set_side2_optical,
                   "Optical material (emissivity/absorptivity) of side 2.")
      .def_prop_rw("dir1_mesh", &ThermalMesh::get_dir1_mesh,
                   &ThermalMesh::set_dir1_mesh,
                   "Mesh division points in direction 1 (list from 0 to 1).")
      .def_prop_rw("dir2_mesh", &ThermalMesh::get_dir2_mesh,
                   &ThermalMesh::set_dir2_mesh,
                   "Mesh division points in direction 2 (list from 0 to 1).")
      .def("is_valid", &ThermalMesh::is_valid,
           "Check whether the thermal mesh configuration is valid.");
}
