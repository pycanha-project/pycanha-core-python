#pragma once
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <memory>
#include <vector>

#include "pycanha-core/gmm/trimesh.hpp"

namespace py = pybind11;
using namespace py::literals;

using namespace pycanha::gmm::trimesher;
// using namespace pycanha::TriMesh::;

void TriMesh_b(py::module_& m) {
  py::class_<TriMesh, std::shared_ptr<TriMesh>>(m, "TriMesh")
      .def(
          py::init<VerticesList, TrianglesList, FaceIdsList, EdgesList,
                   EdgesIdsList, FaceEdges>())
      .def("get_vertices",
           [](TriMesh& self) -> py::array_t<double> {
             return py::array_t<double>(
                 {self.get_vertices().rows(),
                  self.get_vertices().cols()},  // shape
                 {sizeof(double),
                  sizeof(double) *
                      self.get_vertices()
                          .rows()},  // C-style contiguous strides for double
                 self.get_vertices().data());  // the data pointer
           })
      .def("get_triangles",
           [](TriMesh& self) -> py::array_t<uint32_t> {
             return py::array_t<uint32_t>(
                 {self.get_triangles().rows(),
                  self.get_triangles().cols()},  // shape
                 {sizeof(uint32_t),
                  sizeof(uint32_t) *
                      self.get_triangles()
                          .rows()},  // C-style contiguous strides for uint32_t
                 self.get_triangles().data());  // the data pointer
           })
      .def("set_face_ids", &TriMesh::set_face_ids)
      .def("get_face_ids", py::overload_cast<>(&TriMesh::get_face_ids))
      .def("get_edges", py::overload_cast<>(&TriMesh::get_edges))
      .def("set_edges", &TriMesh::set_edges)
      .def("get_perimeter_edges", py::overload_cast<>(&TriMesh::get_perimeter_edges))
      .def("set_perimeter_edges", &TriMesh::set_perimeter_edges)
      .def("get_faces_edges", &TriMesh::get_faces_edges)
      .def("set_faces_edges", &TriMesh::set_faces_edges)
      .def("get_face_cumareas", &TriMesh::get_cumareas)
      .def_property("surface1_color", &TriMesh::get_surface1_color,
                                      &TriMesh::set_surface1_color)
      .def_property("surface2_color", &TriMesh::get_surface2_color,
                                      &TriMesh::set_surface2_color)
      .def_property("vertices", py::overload_cast<>(&TriMesh::get_vertices),
                                &TriMesh::set_vertices)
      .def_property("triangles", py::overload_cast<>(&TriMesh::get_triangles),
                                 &TriMesh::set_triangles)
      .def_property("face_ids", py::overload_cast<>(&TriMesh::get_face_ids),
                                &TriMesh::set_face_ids)
      .def_property("edges", py::overload_cast<>(&TriMesh::get_edges),
                             &TriMesh::set_edges)
      .def_property("perimeter_edges", py::overload_cast<>(&TriMesh::get_perimeter_edges),
                                       &TriMesh::set_perimeter_edges)
      .def_property("faces_edges", &TriMesh::get_faces_edges,
                                   &TriMesh::set_faces_edges);
}

void TriMeshModel_b(py::module_& m) {
  py::class_<TriMeshModel>(m, "TriMeshModel")
      .def(py::init<>())
      .def("get_face_cumareas", &TriMeshModel::get_cumareas)
      .def_property("vertices", py::overload_cast<>(&TriMeshModel::get_vertices),
                                &TriMeshModel::set_vertices)
      .def_property("triangles", py::overload_cast<>(&TriMeshModel::get_triangles),
                                 &TriMeshModel::set_triangles)
      .def_property("face_ids", py::overload_cast<>(&TriMeshModel::get_face_ids),
                                &TriMeshModel::set_face_ids)
      .def_property("face_activity", py::overload_cast<>(&TriMeshModel::get_face_activity),
                                     &TriMeshModel::set_face_activity)
      .def_property("opticals", py::overload_cast<>(&TriMeshModel::get_opticals),
                                &TriMeshModel::set_opticals)
      .def_property("n_faces", &TriMeshModel::get_number_of_faces,
                               &TriMeshModel::set_number_of_faces)
      .def_property("n_geometries", &TriMeshModel::get_number_of_geometries,
                                    &TriMeshModel::set_number_of_geometries)
      .def_property("front_colors", py::overload_cast<>(&TriMeshModel::get_front_colors),
                                    &TriMeshModel::set_front_colors)
      .def_property("back_colors", py::overload_cast<>(&TriMeshModel::get_back_colors),
                                   &TriMeshModel::set_back_colors)
      .def_property("geometries_triangles", py::overload_cast<>(&TriMeshModel::get_geometries_triangles),
                                            &TriMeshModel::set_geometries_triangles)
      .def_property("geometries_vertices", py::overload_cast<>(&TriMeshModel::get_geometries_vertices),
                                           &TriMeshModel::set_geometries_vertices)
      .def_property("geometries_edges", py::overload_cast<>(&TriMeshModel::get_geometries_edges),
                                        &TriMeshModel::set_geometries_edges)
      .def_property("geometries_perimeter_edges", py::overload_cast<>(&TriMeshModel::get_geometries_perimeter_edges),
                                                  &TriMeshModel::set_geometries_perimeter_edges)
      .def_property("geometries_id", py::overload_cast<>(&TriMeshModel::get_geometries_id),
                                     &TriMeshModel::set_geometries_id)
      .def_property("edges", py::overload_cast<>(&TriMeshModel::get_edges),
                             &TriMeshModel::set_edges)
      .def_property("perimeter_edges", py::overload_cast<>(&TriMeshModel::get_perimeter_edges),
                                       &TriMeshModel::set_perimeter_edges)
      .def_property("faces_edges", &TriMeshModel::get_faces_edges,
                                   &TriMeshModel::set_faces_edges)
      // .def_property("vertices", &TriMeshModel::_vertices)
      // .def_property("triangles", &TriMeshModel::_triangles)
      // .def_property("face_ids", &TriMeshModel::_face_ids)
      // .def_property("face_activity", &TriMeshModel::_face_activity)
      // .def_property("opticals", &TriMeshModel::_opticals)
      // .def_property("n_faces", &TriMeshModel::_n_faces)
      // .def_property("n_geometries", &TriMeshModel::_n_geometries)
      // .def_property("front_colors", &TriMeshModel::_front_colors)
      // .def_property("back_colors", &TriMeshModel::_back_colors)
      // .def_property("geometries_triangles", &TriMeshModel::_geometries_triangles)
      // .def_property("geometries_vertices", &TriMeshModel::_geometries_vertices)
      // .def_property("geometries_edges", &TriMeshModel::_geometries_edges)
      // .def_property("geometries_perimeter_edges", &TriMeshModel::_geometries_perimeter_edges)
      // .def_property("geometries_id", &TriMeshModel::_geometries_id)
      // .def_property("edges", &TriMeshModel::_edges)
      // .def_property("perimeter_edges", &TriMeshModel::_perimeter_edges)
      // .def_property("faces_edges", &TriMeshModel::_faces_edges)
      .def("get_geometry_mesh", &TriMeshModel::get_geometry_mesh)
      .def("add_mesh", &TriMeshModel::add_mesh)
      .def("clear", &TriMeshModel::clear);
}

void primitive_meshers_b(py::module_& m) {
  m.def("cdt_trimesher", &cdt_trimesher, "trimesh"_a);
  m.def("create_2d_rectangular_mesh", &create_2d_rectangular_mesh);
  m.def("create_2d_quadrilateral_mesh", &create_2d_quadrilateral_mesh);
  m.def("create_2d_triangular_only_mesh", &create_2d_triangular_only_mesh);
  m.def("create_2d_triangular_mesh", &create_2d_triangular_mesh);
  m.def("create_2d_disc_mesh", &create_2d_disc_mesh);
}
