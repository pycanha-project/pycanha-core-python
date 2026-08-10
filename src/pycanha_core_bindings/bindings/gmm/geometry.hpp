#pragma once
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

#include <cstdint>
#include <memory>
#include <span>

#include "bindings/utils/logger.hpp"
#include "pycanha-core/gmm/scene/geometry.hpp"
#include "pycanha-core/gmm/scene/geometry_group.hpp"
#include "pycanha-core/gmm/scene/geometry_group_cutted.hpp"
#include "pycanha-core/gmm/scene/geometry_item.hpp"

namespace nb = nanobind;
using namespace nanobind::literals;
using namespace pycanha::gmm;

// Object-centric scene tree. Objects are always held by std::shared_ptr and are
// non-copyable (identity matters). Geometry is polymorphic, so nanobind returns
// the concrete Python type (GeometryItem / GeometryGroup / GeometryGroupCutted)
// for a shared_ptr<Geometry>. Operator sugar (+ / -) is intentionally NOT here;
// it belongs to the pure-Python layer-3 package.

namespace pycanha::bindings::gmm::detail {

// span<const shared_ptr<Geometry>> -> Python list.
// TODO(pycanha-core): expose a vector/binding-native children accessor in core
// so this copy is unnecessary.
inline nb::list geometry_span_to_list(
    std::span<const std::shared_ptr<Geometry>> items) {
  nb::list out;
  for (const auto& item : items) {
    out.append(item);
  }
  return out;
}

}  // namespace pycanha::bindings::gmm::detail

inline void Geometry_b(nb::module_& m) {
  nb::class_<Geometry>(m, "Geometry",
                       "Abstract base of the scene tree (GeometryItem, "
                       "GeometryGroup, GeometryGroupCutted).")
      .def_prop_ro("name", &Geometry::name, "Unique name within the model.")
      .def_prop_ro(
          "id",
          [](const Geometry& self) {
            return static_cast<std::uint64_t>(self.id());
          },
          "Ephemeral process-wide-unique runtime id (uint64).")
      .def_prop_rw("transform", &Geometry::transform, &Geometry::set_transform,
                   "Coordinate transformation in the parent frame.")
      .def_prop_ro(
          "children",
          [](const Geometry& self) {
            return pycanha::bindings::gmm::detail::geometry_span_to_list(
                self.children());
          },
          "Immediate children (empty for a GeometryItem).")
      .def_prop_ro("mesh", &Geometry::mesh, nb::rv_policy::reference_internal,
                   "Subtree mesh (TriMeshD) in the parent frame; lazily built.")
      .def("create_mesh", &Geometry::create_mesh, nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
           "Force a rebuild of this object's subtree mesh.")
      .def_prop_ro("owning_model", &Geometry::owning_model,
                   nb::rv_policy::reference,
                   "The GeometryModel this object is registered with, or None.");
}

inline void GeometryItem_b(nb::module_& m) {
  nb::class_<GeometryItem, Geometry>(
      m, "GeometryItem",
      "A single meshable primitive plus its ThermalMesh (a scene-tree leaf).")
      .def(nb::init<std::string, Primitive, ThermalMesh,
                    CoordinateTransformation>(),
           "name"_a, "primitive"_a, "thermal_mesh"_a,
           "transform"_a = CoordinateTransformation(),
           "Create a geometry item.")
      .def_prop_rw("primitive", &GeometryItem::primitive,
                   &GeometryItem::set_primitive, "The wrapped primitive.")
      .def_prop_rw("thermal_mesh", &GeometryItem::thermal_mesh,
                   &GeometryItem::set_thermal_mesh, "The thermal mesh.")
      .def_prop_rw("mesh_options_override",
                   &GeometryItem::mesh_options_override,
                   &GeometryItem::set_mesh_options_override,
                   "Per-item MeshOptions override (None to use model default).");
}

inline void GeometryGroup_b(nb::module_& m) {
  nb::class_<GeometryGroup, Geometry>(
      m, "GeometryGroup",
      "A transform applied to a collection of child geometries.")
      .def(nb::init<std::string, std::vector<std::shared_ptr<Geometry>>,
                    CoordinateTransformation>(),
           "name"_a, "children"_a = std::vector<std::shared_ptr<Geometry>>{},
           "transform"_a = CoordinateTransformation(),
           "Create a geometry group.")
      .def("add", &GeometryGroup::add, "child"_a,
           "Append a child (rejects nullptr, registered, or duplicate nodes).")
      .def("remove_child", &GeometryGroup::remove_child, "child"_a,
           "Remove a child; returns True if it was present.");
}

inline void GeometryGroupCutted_b(nb::module_& m) {
  nb::class_<GeometryGroupCutted, Geometry>(
      m, "GeometryGroupCutted",
      "Boolean-subtract group: every target is cut by the union of all "
      "cutters (each cutter must be a closed-solid GeometryItem).")
      .def(nb::init<std::string, std::vector<std::shared_ptr<Geometry>>,
                    std::vector<std::shared_ptr<GeometryItem>>,
                    CoordinateTransformation>(),
           "name"_a, "targets"_a, "cutters"_a,
           "transform"_a = CoordinateTransformation(),
           "Create a cut group from targets and cutters.")
      .def("cut_with", &GeometryGroupCutted::cut_with, "cutter"_a,
           "Add another cutter (must be a closed-solid GeometryItem).")
      .def_prop_ro(
          "targets",
          [](const GeometryGroupCutted& self) {
            return pycanha::bindings::gmm::detail::geometry_span_to_list(
                self.targets());
          },
          "The target geometries being cut.")
      .def_prop_ro(
          "cutters",
          [](const GeometryGroupCutted& self) {
            // TODO(pycanha-core): span<const shared_ptr<GeometryItem>> copied to
            // a list; a binding-native accessor in core would remove this.
            nb::list out;
            for (const auto& cutter : self.cutters()) {
              out.append(cutter);
            }
            return out;
          },
          "The cutter geometry items.");
}

inline void is_closed_solid_b(nb::module_& m) {
  m.def("is_closed_solid", &is_closed_solid, "primitive"_a,
        "Whether a primitive is a closed solid usable as a cutter.");
}
