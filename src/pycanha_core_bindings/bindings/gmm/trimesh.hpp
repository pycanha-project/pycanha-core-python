#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "pycanha-core/gmm/trimesh.hpp"

namespace nb = nanobind;
using namespace nanobind::literals;

using namespace pycanha::gmm::trimesher;

void TriMesh_b(nb::module_ &m) {
  nb::class_<TriMesh>(m, "TriMesh")
      .def(nb::init<VerticesList, TrianglesList, FaceIdsList, EdgesList,
                    EdgesIdsList, FaceEdges>())
      .def("get_vertices",
           [](TriMesh &self)
               -> nb::ndarray<nb::numpy, const double, nb::ndim<2>> {
             auto &vertices = self.get_vertices();
             std::size_t shape[2] = {static_cast<std::size_t>(vertices.rows()),
                                     static_cast<std::size_t>(vertices.cols())};
             return nb::ndarray<nb::numpy, const double, nb::ndim<2>>(
                 vertices.data(), 2, shape, nb::handle());
           })
      .def("get_triangles",
           [](TriMesh &self)
               -> nb::ndarray<nb::numpy, const uint32_t, nb::ndim<2>> {
             auto &triangles = self.get_triangles();
             std::size_t shape[2] = {
                 static_cast<std::size_t>(triangles.rows()),
                 static_cast<std::size_t>(triangles.cols())};
             return nb::ndarray<nb::numpy, const uint32_t, nb::ndim<2>>(
                 triangles.data(), 2, shape, nb::handle());
           })
      .def("set_face_ids", &TriMesh::set_face_ids)
      .def("get_face_ids", nb::overload_cast<>(&TriMesh::get_face_ids))
      .def("get_edges", nb::overload_cast<>(&TriMesh::get_edges))
      .def("set_edges", &TriMesh::set_edges)
      .def("get_perimeter_edges",
           nb::overload_cast<>(&TriMesh::get_perimeter_edges))
      .def("set_perimeter_edges", &TriMesh::set_perimeter_edges)
      .def("get_faces_edges", &TriMesh::get_faces_edges)
      .def("set_faces_edges", &TriMesh::set_faces_edges)
      .def("get_face_cumareas", &TriMesh::get_cumareas)
      .def_prop_rw("surface1_color", &TriMesh::get_surface1_color,
                   &TriMesh::set_surface1_color)
      .def_prop_rw("surface2_color", &TriMesh::get_surface2_color,
                   &TriMesh::set_surface2_color)
      .def_prop_rw("vertices", nb::overload_cast<>(&TriMesh::get_vertices),
                   &TriMesh::set_vertices)
      .def_prop_rw("triangles", nb::overload_cast<>(&TriMesh::get_triangles),
                   &TriMesh::set_triangles)
      .def_prop_rw("face_ids", nb::overload_cast<>(&TriMesh::get_face_ids),
                   &TriMesh::set_face_ids)
      .def_prop_rw("edges", nb::overload_cast<>(&TriMesh::get_edges),
                   &TriMesh::set_edges)
      .def_prop_rw("perimeter_edges",
                   nb::overload_cast<>(&TriMesh::get_perimeter_edges),
                   &TriMesh::set_perimeter_edges)
      .def_prop_rw("faces_edges", &TriMesh::get_faces_edges,
                   &TriMesh::set_faces_edges);
}

void TriMeshModel_b(nb::module_ &m) {
  nb::class_<TriMeshModel>(m, "TriMeshModel")
      .def(nb::init<>())
      .def("get_face_cumareas", &TriMeshModel::get_cumareas)
      .def_prop_rw("vertices", nb::overload_cast<>(&TriMeshModel::get_vertices),
                   &TriMeshModel::set_vertices)
      .def_prop_rw("triangles",
                   nb::overload_cast<>(&TriMeshModel::get_triangles),
                   &TriMeshModel::set_triangles)
      .def_prop_rw("face_ids", nb::overload_cast<>(&TriMeshModel::get_face_ids),
                   &TriMeshModel::set_face_ids)
      .def_prop_rw("face_activity",
                   nb::overload_cast<>(&TriMeshModel::get_face_activity),
                   &TriMeshModel::set_face_activity)
      .def_prop_rw("opticals", nb::overload_cast<>(&TriMeshModel::get_opticals),
                   &TriMeshModel::set_opticals)
      .def_prop_rw("n_faces", &TriMeshModel::get_number_of_faces,
                   &TriMeshModel::set_number_of_faces)
      .def_prop_rw("n_geometries", &TriMeshModel::get_number_of_geometries,
                   &TriMeshModel::set_number_of_geometries)
      .def_prop_rw("front_colors",
                   nb::overload_cast<>(&TriMeshModel::get_front_colors),
                   &TriMeshModel::set_front_colors)
      .def_prop_rw("back_colors",
                   nb::overload_cast<>(&TriMeshModel::get_back_colors),
                   &TriMeshModel::set_back_colors)
      .def_prop_rw("geometries_triangles",
                   nb::overload_cast<>(&TriMeshModel::get_geometries_triangles),
                   &TriMeshModel::set_geometries_triangles)
      .def_prop_rw("geometries_vertices",
                   nb::overload_cast<>(&TriMeshModel::get_geometries_vertices),
                   &TriMeshModel::set_geometries_vertices)
      .def_prop_rw("geometries_edges",
                   nb::overload_cast<>(&TriMeshModel::get_geometries_edges),
                   &TriMeshModel::set_geometries_edges)
      .def_prop_rw(
          "geometries_perimeter_edges",
          nb::overload_cast<>(&TriMeshModel::get_geometries_perimeter_edges),
          &TriMeshModel::set_geometries_perimeter_edges)
      .def_prop_rw("geometries_id",
                   nb::overload_cast<>(&TriMeshModel::get_geometries_id),
                   &TriMeshModel::set_geometries_id)
      .def_prop_rw("edges", nb::overload_cast<>(&TriMeshModel::get_edges),
                   &TriMeshModel::set_edges)
      .def_prop_rw("perimeter_edges",
                   nb::overload_cast<>(&TriMeshModel::get_perimeter_edges),
                   &TriMeshModel::set_perimeter_edges)
      .def_prop_rw("faces_edges", &TriMeshModel::get_faces_edges,
                   &TriMeshModel::set_faces_edges)
      .def("get_geometry_mesh", &TriMeshModel::get_geometry_mesh)
      .def("add_mesh", &TriMeshModel::add_mesh)
      .def("clear", &TriMeshModel::clear);
}

void primitive_meshers_b(nb::module_ &m) {
  m.def("cdt_trimesher", &cdt_trimesher, "trimesh"_a);
  m.def("create_2d_rectangular_mesh", &create_2d_rectangular_mesh);
  m.def("create_2d_quadrilateral_mesh", &create_2d_quadrilateral_mesh);
  m.def("create_2d_triangular_only_mesh", &create_2d_triangular_only_mesh);
  m.def("create_2d_triangular_mesh", &create_2d_triangular_mesh);
  m.def("create_2d_disc_mesh", &create_2d_disc_mesh);
}
