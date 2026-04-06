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
  nb::class_<TriMesh>(m, "TriMesh",
                     "Triangulated surface mesh storing vertices, triangles,\n"
                     "face IDs, edges, and per-surface colors.")
      .def(nb::init<VerticesList, TrianglesList, FaceIdsList, EdgesList,
                    EdgesIdsList, FaceEdges>(),
           "Create a TriMesh from all mesh data arrays.")
      .def("get_vertices",
           [](TriMesh &self)
               -> nb::ndarray<nb::numpy, const double, nb::ndim<2>> {
             auto &vertices = self.get_vertices();
             std::size_t shape[2] = {static_cast<std::size_t>(vertices.rows()),
                                     static_cast<std::size_t>(vertices.cols())};
             return nb::ndarray<nb::numpy, const double, nb::ndim<2>>(
                 vertices.data(), 2, shape, nb::handle());
           },
           "Get vertex coordinates as a numpy array (Nv x 3).")
      .def("get_triangles",
           [](TriMesh &self)
               -> nb::ndarray<nb::numpy, const uint32_t, nb::ndim<2>> {
             auto &triangles = self.get_triangles();
             std::size_t shape[2] = {
                 static_cast<std::size_t>(triangles.rows()),
                 static_cast<std::size_t>(triangles.cols())};
             return nb::ndarray<nb::numpy, const uint32_t, nb::ndim<2>>(
                 triangles.data(), 2, shape, nb::handle());
           },
           "Get triangle vertex indices as a numpy array (Nt x 3).")
      .def("set_face_ids", &TriMesh::set_face_ids,
           "Set the face ID for each triangle.")
      .def("get_face_ids", nb::overload_cast<>(&TriMesh::get_face_ids),
           "Get the face ID array.")
      .def("get_edges", nb::overload_cast<>(&TriMesh::get_edges),
           "Get the edge list (vertex index pairs).")
      .def("set_edges", &TriMesh::set_edges,
           "Set the edge list.")
      .def("get_perimeter_edges",
           nb::overload_cast<>(&TriMesh::get_perimeter_edges),
           "Get the perimeter edge indices.")
      .def("set_perimeter_edges", &TriMesh::set_perimeter_edges,
           "Set the perimeter edge indices.")
      .def("get_faces_edges", &TriMesh::get_faces_edges,
           "Get edges grouped by face.")
      .def("set_faces_edges", &TriMesh::set_faces_edges,
           "Set edges grouped by face.")
      .def("get_face_cumareas", &TriMesh::get_cumareas,
           "Get cumulative area per triangle (requires sorted triangles).")
      .def_prop_rw("surface1_color", &TriMesh::get_surface1_color,
                   &TriMesh::set_surface1_color,
                   "RGB color of surface side 1.")
      .def_prop_rw("surface2_color", &TriMesh::get_surface2_color,
                   &TriMesh::set_surface2_color,
                   "RGB color of surface side 2.")
      .def_prop_rw("vertices", nb::overload_cast<>(&TriMesh::get_vertices),
                   &TriMesh::set_vertices,
                   "Vertex coordinates (Nv x 3).")
      .def_prop_rw("triangles", nb::overload_cast<>(&TriMesh::get_triangles),
                   &TriMesh::set_triangles,
                   "Triangle vertex indices (Nt x 3).")
      .def_prop_rw("face_ids", nb::overload_cast<>(&TriMesh::get_face_ids),
                   &TriMesh::set_face_ids,
                   "Face ID per triangle.")
      .def_prop_rw("edges", nb::overload_cast<>(&TriMesh::get_edges),
                   &TriMesh::set_edges,
                   "Edge list (vertex index pairs).")
      .def_prop_rw("perimeter_edges",
                   nb::overload_cast<>(&TriMesh::get_perimeter_edges),
                   &TriMesh::set_perimeter_edges,
                   "Perimeter edge indices.")
      .def_prop_rw("faces_edges", &TriMesh::get_faces_edges,
                   &TriMesh::set_faces_edges,
                   "Edges grouped by face.");
}

void TriMeshModel_b(nb::module_ &m) {
  nb::class_<TriMeshModel>(m, "TriMeshModel",
                           "Compacted triangle mesh aggregating all geometry\n"
                           "items into a single cache-friendly structure for\n"
                           "rendering and ray-tracing.")
      .def(nb::init<>(), "Create an empty TriMeshModel.")
      .def("get_face_cumareas", &TriMeshModel::get_cumareas,
           "Get cumulative area per triangle.")
      .def_prop_rw("vertices", nb::overload_cast<>(&TriMeshModel::get_vertices),
                   &TriMeshModel::set_vertices,
                   "All vertex coordinates (Nv x 3, float).")
      .def_prop_rw("triangles",
                   nb::overload_cast<>(&TriMeshModel::get_triangles),
                   &TriMeshModel::set_triangles,
                   "All triangle vertex indices (Nt x 3).")
      .def_prop_rw("face_ids", nb::overload_cast<>(&TriMeshModel::get_face_ids),
                   &TriMeshModel::set_face_ids,
                   "Face index per triangle.")
      .def_prop_rw("face_activity",
                   nb::overload_cast<>(&TriMeshModel::get_face_activity),
                   &TriMeshModel::set_face_activity,
                   "Face activity flags (-1=inactive, 0=both, 1/2=one side).")
      .def_prop_rw("opticals", nb::overload_cast<>(&TriMeshModel::get_opticals),
                   &TriMeshModel::set_opticals,
                   "Optical properties per face (Nf x 6).")
      .def_prop_rw("n_faces", &TriMeshModel::get_number_of_faces,
                   &TriMeshModel::set_number_of_faces,
                   "Total number of distinct faces.")
      .def_prop_rw("n_geometries", &TriMeshModel::get_number_of_geometries,
                   &TriMeshModel::set_number_of_geometries,
                   "Number of geometry items in the model.")
      .def_prop_rw("front_colors",
                   nb::overload_cast<>(&TriMeshModel::get_front_colors),
                   &TriMeshModel::set_front_colors,
                   "Front-side RGB colors per geometry.")
      .def_prop_rw("back_colors",
                   nb::overload_cast<>(&TriMeshModel::get_back_colors),
                   &TriMeshModel::set_back_colors,
                   "Back-side RGB colors per geometry.")
      .def_prop_rw("geometries_triangles",
                   nb::overload_cast<>(&TriMeshModel::get_geometries_triangles),
                   &TriMeshModel::set_geometries_triangles,
                   "Triangle index ranges per geometry (Ng+1).")
      .def_prop_rw("geometries_vertices",
                   nb::overload_cast<>(&TriMeshModel::get_geometries_vertices),
                   &TriMeshModel::set_geometries_vertices,
                   "Vertex index ranges per geometry (Ng+1).")
      .def_prop_rw("geometries_edges",
                   nb::overload_cast<>(&TriMeshModel::get_geometries_edges),
                   &TriMeshModel::set_geometries_edges,
                   "Edge index ranges per geometry (Ng+1).")
      .def_prop_rw(
          "geometries_perimeter_edges",
          nb::overload_cast<>(&TriMeshModel::get_geometries_perimeter_edges),
          &TriMeshModel::set_geometries_perimeter_edges,
          "Perimeter edge index ranges per geometry (Ng+1).")
      .def_prop_rw("geometries_id",
                   nb::overload_cast<>(&TriMeshModel::get_geometries_id),
                   &TriMeshModel::set_geometries_id,
                   "Unique ID per geometry.")
      .def_prop_rw("edges", nb::overload_cast<>(&TriMeshModel::get_edges),
                   &TriMeshModel::set_edges,
                   "All edge vertex index pairs.")
      .def_prop_rw("perimeter_edges",
                   nb::overload_cast<>(&TriMeshModel::get_perimeter_edges),
                   &TriMeshModel::set_perimeter_edges,
                   "All perimeter edge vertex index pairs.")
      .def_prop_rw("faces_edges", &TriMeshModel::get_faces_edges,
                   &TriMeshModel::set_faces_edges,
                   "Edges grouped by face.")
      .def("get_geometry_mesh", &TriMeshModel::get_geometry_mesh,
           "Extract a TriMesh for a single geometry by index.")
      .def("add_mesh", &TriMeshModel::add_mesh,
           "Append a TriMesh to the unified model.")
      .def("clear", &TriMeshModel::clear,
           "Remove all mesh data.");
}

void primitive_meshers_b(nb::module_ &m) {
  m.def("cdt_trimesher", &cdt_trimesher, "trimesh"_a,
        "Refine a TriMesh using constrained Delaunay triangulation.");
  m.def("create_2d_rectangular_mesh", &create_2d_rectangular_mesh,
        "Generate a 2D rectangular mesh.");
  m.def("create_2d_quadrilateral_mesh", &create_2d_quadrilateral_mesh,
        "Generate a 2D quadrilateral mesh.");
  m.def("create_2d_triangular_only_mesh", &create_2d_triangular_only_mesh,
        "Generate a 2D mesh with triangles only.");
  m.def("create_2d_triangular_mesh", &create_2d_triangular_mesh,
        "Generate a 2D triangular mesh.");
  m.def("create_2d_disc_mesh", &create_2d_disc_mesh,
        "Generate a 2D disc mesh.");
}
