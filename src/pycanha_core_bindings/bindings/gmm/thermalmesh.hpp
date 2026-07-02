#pragma once
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/vector.h>

#include <utility>
#include <vector>

#include "pycanha-core/gmm/mesh/thermal_mesh.hpp"

namespace nb = nanobind;
using namespace nanobind::literals;
using namespace pycanha::gmm;

// Per-primitive thermal discretization: the two UV cut vectors plus per-side
// (front = side1, back = side2) activity, thickness, color, bulk/optical
// material, and the four int32 fields driving face -> tmm-node assignment.
// Validation is enforced in the C++ setters (invalid state throws).

inline void ThermalMesh_b(nb::module_& m) {
  nb::class_<ThermalMesh>(
      m, "ThermalMesh",
      "Per-primitive thermal discretization (UV cuts + per-side properties + "
      "tmm-node assignment).")
      .def(nb::init<>(), "Default unit square with one face pair.")
      .def(nb::init<std::vector<double>, std::vector<double>>(), "dir1_mesh"_a,
           "dir2_mesh"_a, "Build directly from the two UV cut vectors.")
      // --- UV cuts ---
      // TODO(pycanha-core): the C++ getters return std::span<const double>;
      // copy to a list here until a binding-native accessor exists in core.
      .def_prop_rw(
          "dir1_mesh",
          [](const ThermalMesh& self) {
            const auto span = self.get_dir1_mesh();
            return std::vector<double>(span.begin(), span.end());
          },
          [](ThermalMesh& self, std::vector<double> mesh) {
            self.set_dir1_mesh(std::move(mesh));
          },
          "UV cut positions along parametric direction 1.")
      .def_prop_rw(
          "dir2_mesh",
          [](const ThermalMesh& self) {
            const auto span = self.get_dir2_mesh();
            return std::vector<double>(span.begin(), span.end());
          },
          [](ThermalMesh& self, std::vector<double> mesh) {
            self.set_dir2_mesh(std::move(mesh));
          },
          "UV cut positions along parametric direction 2.")
      .def("is_valid", &ThermalMesh::is_valid,
           "Whether the mesh definition is valid.")
      .def_prop_ro("num_pair_faces", &ThermalMesh::get_number_of_pair_faces,
                   "(len(dir1_mesh) - 1) * (len(dir2_mesh) - 1) face pairs.")
      // --- Per-side activity ---
      .def_prop_rw("side1_activity", &ThermalMesh::get_side1_activity,
                   &ThermalMesh::set_side1_activity, "Front-side activity.")
      .def_prop_rw("side2_activity", &ThermalMesh::get_side2_activity,
                   &ThermalMesh::set_side2_activity, "Back-side activity.")
      // --- Per-side thickness ---
      .def_prop_rw("side1_thick", &ThermalMesh::get_side1_thick,
                   &ThermalMesh::set_side1_thick, "Front-side thickness [m].")
      .def_prop_rw("side2_thick", &ThermalMesh::get_side2_thick,
                   &ThermalMesh::set_side2_thick, "Back-side thickness [m].")
      // --- Per-side color ---
      .def_prop_rw("side1_color", &ThermalMesh::get_side1_color,
                   &ThermalMesh::set_side1_color, "Front-side color.")
      .def_prop_rw("side2_color", &ThermalMesh::get_side2_color,
                   &ThermalMesh::set_side2_color, "Back-side color.")
      // --- Per-side bulk material (nullable / None) ---
      .def_prop_rw("side1_material", &ThermalMesh::get_side1_material,
                   &ThermalMesh::set_side1_material,
                   "Front-side bulk material (None if unassigned).")
      .def_prop_rw("side2_material", &ThermalMesh::get_side2_material,
                   &ThermalMesh::set_side2_material,
                   "Back-side bulk material (None if unassigned).")
      // --- Per-side optical material (nullable / None) ---
      .def_prop_rw("side1_optical", &ThermalMesh::get_side1_optical,
                   &ThermalMesh::set_side1_optical,
                   "Front-side optical material (None if unassigned).")
      .def_prop_rw("side2_optical", &ThermalMesh::get_side2_optical,
                   &ThermalMesh::set_side2_optical,
                   "Back-side optical material (None if unassigned).")
      // --- Per-side tmm-node assignment ---
      .def_prop_rw("node1_start", &ThermalMesh::get_node1_start,
                   &ThermalMesh::set_node1_start,
                   "Front-side base node number (-1 = no node).")
      .def_prop_rw("node1_step", &ThermalMesh::get_node1_step,
                   &ThermalMesh::set_node1_step,
                   "Front-side per-cell node increment.")
      .def_prop_rw("node2_start", &ThermalMesh::get_node2_start,
                   &ThermalMesh::set_node2_start,
                   "Back-side base node number (-1 = no node).")
      .def_prop_rw("node2_step", &ThermalMesh::get_node2_step,
                   &ThermalMesh::set_node2_step,
                   "Back-side per-cell node increment.")
      .def("node_of", &ThermalMesh::node_of, "i"_a, "j"_a, "side"_a,
           "Node number for cell (i, j) on side 1 or 2.");
}
