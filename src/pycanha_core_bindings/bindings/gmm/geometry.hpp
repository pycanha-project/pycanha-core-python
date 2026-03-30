#pragma once
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <string>

#include "pycanha-core/gmm/geometry.hpp"

namespace nb = nanobind;
using namespace pycanha::gmm;

void Geometry_b(nb::module_ &m) {
  nb::class_<Geometry>(m, "Geometry")
      .def(nb::init<>())
      .def(nb::init<std::string>())
      .def(nb::init<std::string, TransformationPtr>())
      .def_prop_rw("name", &Geometry::get_name, &Geometry::set_name)
      .def_prop_rw("transformation", &Geometry::get_transformation,
                   &Geometry::set_transformation)
      .def_prop_rw("parent", &Geometry::get_parent, &Geometry::set_parent);
}

void GeometryItem_b(nb::module_ &m) {
  nb::class_<GeometryItem, Geometry>(m, "GeometryItem")
      .def(nb::init<>())
      .def(nb::init<std::string, PrimitivePtr, TransformationPtr>())
      .def_prop_rw("primitive", &GeometryItem::get_primitive,
                   &GeometryItem::set_primitive);
}

void GeometryGroup_b(nb::module_ &m) {
  nb::class_<GeometryGroup, Geometry>(m, "GeometryGroup")
      .def(nb::init<>())
      .def(nb::init<std::string>())
      .def(nb::init<std::string, GeometryPtrList, TransformationPtr>())
      .def_prop_rw("geometry_items", &GeometryGroup::get_geometry_items,
                   &GeometryGroup::set_geometry_items)
      .def_prop_rw("geometry_groups", &GeometryGroup::get_geometry_groups,
                   &GeometryGroup::set_geometry_groups)
      .def("add_geometry_item", &GeometryGroup::add_geometry_item)
      .def("add_geometry_group", &GeometryGroup::add_geometry_group)
      .def("remove_geometry_item", &GeometryGroup::remove_geometry_item)
      .def("remove_geometry_group", &GeometryGroup::remove_geometry_group)
      .def_prop_rw("geometry_groups_cutted",
                   &GeometryGroup::get_geometry_groups_cutted,
                   &GeometryGroup::set_geometry_groups_cutted);
}

void GeometryGroupCutted_b(nb::module_ &m) {
  nb::class_<GeometryGroupCutted, GeometryGroup>(m, "GeometryGroupCutted")
      .def_prop_rw("cutting_geometry_items",
                   &GeometryGroupCutted::get_cutting_geometry_items,
                   &GeometryGroupCutted::set_cutting_geometry_items)
      .def("add_cutting_geometry_item",
           &GeometryGroupCutted::add_cutting_geometry_item)
      .def("remove_cutting_geometry_item",
           &GeometryGroupCutted::remove_cutting_geometry_item)
      .def("create_cutted_mesh", &GeometryGroupCutted::create_cutted_mesh)
      .def_prop_rw("cutted_geometry_meshed_items",
                   &GeometryGroupCutted::get_cutted_geometry_meshed_items,
                   &GeometryGroupCutted::set_cutted_geometry_meshed_items);
  // TODO: Check this
  //  .def_prop_rw("cutting_primitives",
  //                 &GeometryGroupCutted::get_cutting_primitives,
  //                 &GeometryGroupCutted::set_cutting_primitives);
}

void GeometryMeshedItem_b(nb::module_ &m) {
  nb::class_<GeometryMeshedItem, GeometryItem>(m, "GeometryMeshedItem")
      .def(nb::init<>())
      .def(nb::init<std::string, PrimitivePtr, TransformationPtr,
                    ThermalMeshPtr>())
      .def_prop_rw("thermal_mesh", &GeometryMeshedItem::get_thermal_mesh,
                   &GeometryMeshedItem::set_thermal_mesh)
      .def_prop_rw("tri_mesh", &GeometryMeshedItem::get_tri_mesh,
                   &GeometryMeshedItem::set_tri_mesh)
      .def("triangulate_post_processed_cutted_mesh",
           &GeometryMeshedItem::triangulate_post_processed_cutted_mesh);
}
