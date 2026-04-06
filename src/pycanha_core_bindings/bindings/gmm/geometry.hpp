#pragma once
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <string>

#include "pycanha-core/gmm/geometry.hpp"

namespace nb = nanobind;
using namespace nanobind::literals;
using namespace pycanha::gmm;

void Geometry_b(nb::module_ &m) {
  nb::class_<Geometry>(m, "Geometry",
                      "Base class for all geometry elements.\n\n"
                      "Each geometry has a unique ID, a name, an optional\n"
                      "coordinate transformation, and an optional parent group.")
      .def(nb::init<>(), "Create a geometry with auto-generated name.")
      .def(nb::init<std::string>(), "name"_a,
           "Create a geometry with the given name.")
      .def(nb::init<std::string, TransformationPtr>(), "name"_a,
           "transformation"_a,
           "Create a geometry with name and transformation.")
      .def_prop_rw("name", &Geometry::get_name, &Geometry::set_name,
                   "Geometry name.")
      .def_prop_rw("transformation", &Geometry::get_transformation,
                   &Geometry::set_transformation,
                   "Coordinate transformation applied to this geometry.")
      .def_prop_rw("parent", &Geometry::get_parent, &Geometry::set_parent,
                   "Parent GeometryGroup (weak reference).");
}

void GeometryItem_b(nb::module_ &m) {
  nb::class_<GeometryItem, Geometry>(m, "GeometryItem",
                                    "Geometry element holding a single Primitive.")
      .def(nb::init<>(), "Create an empty geometry item.")
      .def(nb::init<std::string, PrimitivePtr, TransformationPtr>(), "name"_a,
           "primitive"_a, "transformation"_a,
           "Create a geometry item with a primitive and transformation.")
      .def_prop_rw("primitive", &GeometryItem::get_primitive,
                   &GeometryItem::set_primitive,
                   "The geometric Primitive of this item.");
}

void GeometryGroup_b(nb::module_ &m) {
  nb::class_<GeometryGroup, Geometry>(
      m, "GeometryGroup",
      "Hierarchical container of GeometryMeshedItems, sub-groups,\n"
      "and cutted groups. Applies a shared transformation to all children.")
      .def(nb::init<>(), "Create an empty geometry group.")
      .def(nb::init<std::string>(), "name"_a,
           "Create a geometry group with the given name.")
      .def(nb::init<std::string, GeometryPtrList, TransformationPtr>(),
           "name"_a, "geometry_items"_a, "transformation"_a,
           "Create a group with items and a shared transformation.")
      .def_prop_rw("geometry_items", &GeometryGroup::get_geometry_items,
                   &GeometryGroup::set_geometry_items,
                   "List of child GeometryMeshedItems.")
      .def_prop_rw("geometry_groups", &GeometryGroup::get_geometry_groups,
                   &GeometryGroup::set_geometry_groups,
                   "List of child GeometryGroups.")
      .def("add_geometry_item", &GeometryGroup::add_geometry_item,
           "Add a GeometryMeshedItem to this group.")
      .def("add_geometry_group", &GeometryGroup::add_geometry_group,
           "Add a child GeometryGroup.")
      .def("remove_geometry_item", &GeometryGroup::remove_geometry_item,
           "Remove a GeometryMeshedItem from this group.")
      .def("remove_geometry_group", &GeometryGroup::remove_geometry_group,
           "Remove a child GeometryGroup.")
      .def_prop_rw("geometry_groups_cutted",
                   &GeometryGroup::get_geometry_groups_cutted,
                   &GeometryGroup::set_geometry_groups_cutted,
                   "List of child GeometryGroupCutted objects.");
}

void GeometryGroupCutted_b(nb::module_ &m) {
  nb::class_<GeometryGroupCutted, GeometryGroup>(
      m, "GeometryGroupCutted",
      "Geometry group where child meshes are cut by\n"
      "cutting primitives (boolean subtraction).")
      .def_prop_rw("cutting_geometry_items",
                   &GeometryGroupCutted::get_cutting_geometry_items,
                   &GeometryGroupCutted::set_cutting_geometry_items,
                   "List of geometry items used as cutting tools.")
      .def("add_cutting_geometry_item",
           &GeometryGroupCutted::add_cutting_geometry_item,
           "Add a geometry item as a cutting tool.")
      .def("remove_cutting_geometry_item",
           &GeometryGroupCutted::remove_cutting_geometry_item,
           "Remove a cutting geometry item.")
      .def("create_cutted_mesh", &GeometryGroupCutted::create_cutted_mesh,
           "Perform the cutting operation and generate cut meshes.")
      .def_prop_rw("cutted_geometry_meshed_items",
                   &GeometryGroupCutted::get_cutted_geometry_meshed_items,
                   &GeometryGroupCutted::set_cutted_geometry_meshed_items,
                   "List of resulting cut GeometryMeshedItems.");
  // TODO: Check this
  //  .def_prop_rw("cutting_primitives",
  //                 &GeometryGroupCutted::get_cutting_primitives,
  //                 &GeometryGroupCutted::set_cutting_primitives);
}

void GeometryMeshedItem_b(nb::module_ &m) {
  nb::class_<GeometryMeshedItem, GeometryItem>(
      m, "GeometryMeshedItem",
      "Geometry item with thermal mesh properties and\n"
      "a generated triangle mesh.")
      .def(nb::init<>(), "Create an empty meshed geometry item.")
      .def(nb::init<std::string, PrimitivePtr, TransformationPtr,
                    ThermalMeshPtr>(),
           "name"_a, "primitive"_a, "transformation"_a, "thermal_mesh"_a,
           "Create a meshed item with primitive, transform, and thermal mesh.")
      .def_prop_rw("thermal_mesh", &GeometryMeshedItem::get_thermal_mesh,
                   &GeometryMeshedItem::set_thermal_mesh,
                   "ThermalMesh defining surface properties and mesh divisions.")
      .def_prop_rw("tri_mesh", &GeometryMeshedItem::get_tri_mesh,
                   &GeometryMeshedItem::set_tri_mesh,
                   "Generated TriMesh (triangle mesh data).")
      .def("triangulate_post_processed_cutted_mesh",
           &GeometryMeshedItem::triangulate_post_processed_cutted_mesh,
           "Triangulate the mesh after cutting post-processing.");
}
