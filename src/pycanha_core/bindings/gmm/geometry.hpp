#pragma once
#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <string>

#include "pycanha-core/gmm/geometry.hpp"

namespace py = pybind11;
using namespace pycanha::gmm;

void Geometry_b(py::module_ &m) {
  py::class_<Geometry, std::shared_ptr<Geometry>>(m, "Geometry")
      .def(py::init<>())
      .def(py::init<std::string>())
      .def(py::init<std::string, TransformationPtr>())
      .def_property("name", &Geometry::get_name, &Geometry::set_name)
      .def_property("transformation", &Geometry::get_transformation,
                    &Geometry::set_transformation)
      .def_property("parent", &Geometry::get_parent, &Geometry::set_parent);
}

void GeometryItem_b(py::module_ &m) {
  py::class_<GeometryItem, Geometry, std::shared_ptr<GeometryItem>>(
      m, "GeometryItem")
      .def(py::init<>())
      .def(py::init<std::string, PrimitivePtr, TransformationPtr>())
      .def_property("primitive", &GeometryItem::get_primitive,
                    &GeometryItem::set_primitive);
}

void GeometryGroup_b(py::module_ &m) {
  py::class_<GeometryGroup, Geometry, std::shared_ptr<GeometryGroup>>(
      m, "GeometryGroup")
      .def(py::init<>())
      .def(py::init<std::string>())
      .def(py::init<std::string, GeometryPtrList, TransformationPtr>())
      .def_property("geometry_items", &GeometryGroup::get_geometry_items,
                    &GeometryGroup::set_geometry_items)
      .def_property("geometry_groups", &GeometryGroup::get_geometry_groups,
                    &GeometryGroup::set_geometry_groups)
      .def("add_geometry_item", &GeometryGroup::add_geometry_item)
      .def("add_geometry_group", &GeometryGroup::add_geometry_group)
      .def("remove_geometry_item", &GeometryGroup::remove_geometry_item)
      .def("remove_geometry_group", &GeometryGroup::remove_geometry_group)
      .def_property("geometry_groups_cutted",
                    &GeometryGroup::get_geometry_groups_cutted,
                    &GeometryGroup::set_geometry_groups_cutted);
}

void GeometryGroupCutted_b(py::module_ &m) {
  py::class_<GeometryGroupCutted, GeometryGroup,
             std::shared_ptr<GeometryGroupCutted>>(m, "GeometryGroupCutted")
      .def_property("cutting_geometry_items",
                    &GeometryGroupCutted::get_cutting_geometry_items,
                    &GeometryGroupCutted::set_cutting_geometry_items)
      .def("add_cutting_geometry_item",
           &GeometryGroupCutted::add_cutting_geometry_item)
      .def("remove_cutting_geometry_item",
           &GeometryGroupCutted::remove_cutting_geometry_item)
      .def("create_cutted_mesh", &GeometryGroupCutted::create_cutted_mesh)
      .def_property("cutted_geometry_meshed_items",
                     &GeometryGroupCutted::get_cutted_geometry_meshed_items,
                     &GeometryGroupCutted::set_cutted_geometry_meshed_items);
      //TODO: Check this
      // .def_property("cutting_primitives",
      //                &GeometryGroupCutted::get_cutting_primitives,
      //                &GeometryGroupCutted::set_cutting_primitives);
}

void GeometryMeshedItem_b(py::module_ &m) {
  py::class_<GeometryMeshedItem, GeometryItem,
             std::shared_ptr<GeometryMeshedItem>>(m, "GeometryMeshedItem")
      .def(py::init<>())
      .def(py::init<std::string, PrimitivePtr, TransformationPtr,
                    ThermalMeshPtr>())
      .def_property("thermal_mesh", &GeometryMeshedItem::get_thermal_mesh,
                    &GeometryMeshedItem::set_thermal_mesh)
      .def_property("tri_mesh", &GeometryMeshedItem::get_tri_mesh,
                    &GeometryMeshedItem::set_tri_mesh)
      .def("triangulate_post_processed_cutted_mesh",
           &GeometryMeshedItem::triangulate_post_processed_cutted_mesh);
}
