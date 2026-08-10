#pragma once
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "bindings/utils/logger.hpp"
#include "pycanha-core/gmm/geometrymodel.hpp"

namespace nb = nanobind;
using namespace nanobind::literals;
using namespace pycanha::gmm;

// Object-centric scene container. HAS-A root GeometryGroup; group-style methods
// forward to it. Registration assigns ids/owning-model and enforces name
// uniqueness. Lookups return the concrete Python subclass (Geometry is
// polymorphic).

inline void GeometryModel_b(nb::module_& m) {
  nb::class_<GeometryModel>(
      m, "GeometryModel",
      "Top-level scene container: registers geometry, resolves names, owns the "
      "world mesh.")
      .def(nb::init<std::string>(), "name"_a, "Create a named geometry model.")
      .def_prop_ro("name", &GeometryModel::name, "Model name.")
      .def("add", &GeometryModel::add, "object"_a, "parent_name"_a = "",
           "Register an object (and its subtree) under parent_name (root when "
           "empty).")
      .def(
          "contains",
          [](const GeometryModel& self, const std::string& name) {
            return self.contains(name);
          },
          "name"_a, "Whether a geometry with this name is registered.")
      .def(
          "contains",
          [](const GeometryModel& self,
             const std::shared_ptr<Geometry>& object) {
            return self.contains(object);
          },
          "object"_a, "Whether this object is registered.")
      .def("get", &GeometryModel::get, "name"_a,
           "Get a geometry by name (None if absent).")
      .def("get_item", &GeometryModel::get_item, "name"_a,
           "Get a GeometryItem by name (None if absent or wrong kind).")
      .def("get_group", &GeometryModel::get_group, "name"_a,
           "Get a GeometryGroup by name (None if absent or wrong kind).")
      .def("get_cut_group", &GeometryModel::get_cut_group, "name"_a,
           "Get a GeometryGroupCutted by name (None if absent or wrong kind).")
      .def(
          "remove",
          [](GeometryModel& self, const std::string& name) {
            self.remove(name);
          },
          "name"_a, "Remove a geometry (and its subtree) by name.")
      .def(
          "remove",
          [](GeometryModel& self, const std::shared_ptr<Geometry>& object) {
            self.remove(object);
          },
          "object"_a, "Remove a geometry (and its subtree).")
      .def("rename", &GeometryModel::rename, "current_name"_a, "new_name"_a,
           "Rename a registered geometry.")
      .def("reparent", &GeometryModel::reparent, "name"_a, "new_parent_name"_a,
           "Move a geometry under a new parent group.")
      .def_prop_ro(
          "children",
          [](const GeometryModel& self) {
            nb::list out;
            for (const auto& child : self.children()) {
              out.append(child);
            }
            return out;
          },
          "Immediate children of the root group.")
      .def("children_recursive", &GeometryModel::children_recursive,
           "All geometries in the tree (depth-first).")
      .def_prop_rw("default_mesh_options", &GeometryModel::default_mesh_options,
                   &GeometryModel::set_default_mesh_options,
                   "Default MeshOptions applied to items without an override.")
      .def("get_structure_version", &GeometryModel::get_structure_version,
           "Monotonic version bumped on every structural / content change.")
      .def("notify_content_changed", &GeometryModel::notify_content_changed,
           "Mark caches dirty after an external content edit.")
      .def_prop_ro("mesh", &GeometryModel::mesh,
                   nb::rv_policy::reference_internal,
                   "World mesh as float32 (TriMeshF); lazily built and cached.")
      .def("create_mesh", &GeometryModel::create_mesh, nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
           "Force a full rebuild of the world mesh.")
      .def("invalidate_mesh", &GeometryModel::invalidate_mesh,
           "Mark the world mesh dirty without rebuilding.")
      .def_prop_ro("root_group_mesh", &GeometryModel::root_group_mesh,
                   nb::rv_policy::reference_internal,
                   "Float64 root-group mesh (TriMeshD), before float32 cast.")
      .def_prop_ro("root_group", &GeometryModel::root_group,
                   "The root GeometryGroup.")
      .def(
          "faces_of_node",
          [](const GeometryModel& self, pycanha::NodeNum node_num) {
            // TODO(pycanha-core): span<const FaceId> copied to a list of ints;
            // a binding-native accessor in core would remove this.
            nb::list out;
            for (const auto& face_id : self.faces_of_node(node_num)) {
              out.append(static_cast<pycanha::MeshIndex>(face_id));
            }
            return out;
          },
          "node_num"_a,
          "Face ids assigned to a node number (built at mesh build).")
      .def(
          "mesh_parts",
          [](const GeometryModel& self,
             const std::vector<std::string>& split) {
            return self.mesh_parts(std::span<const std::string>(split));
          },
          "split"_a = std::vector<std::string>{},
          "Split the model mesh into rigid ScenePart pieces for the raytracer: "
          "one part per named geometry in `split` (its subtree, in its local "
          "frame) plus a remainder part in the world frame. Face ids stay "
          "global across parts.")
      .def("material_table", &GeometryModel::material_table,
           "Build the per-face-slot MaterialTable (optical material + activity) "
           "from the per-side ThermalMesh data.");
}
