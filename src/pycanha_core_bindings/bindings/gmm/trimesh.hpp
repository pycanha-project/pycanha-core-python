#pragma once
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>

#include <cstdint>

#include "pycanha-core/gmm/mesh/trimesh.hpp"

namespace nb = nanobind;
using namespace nanobind::literals;
using namespace pycanha::gmm;

// TriMesh<Scalar> is bound for both precisions: TriMeshD (float64, per-item /
// per-cut caches) and TriMeshF (float32, the model root mesh for the raytracer).
// The bulk arrays are exposed as ZERO-COPY, read-only numpy views: returning the
// Eigen member by const reference with rv_policy::reference_internal makes
// nanobind's Eigen caster hand out a non-owning, non-writeable view (correct for
// Eigen's column-major storage). `primitives` is small metadata, returned as a
// list of (geometry_id, first_face_id, last_face_id) tuples.

namespace pycanha::bindings::gmm::detail {

template <class Scalar>
void bind_trimesh(nb::module_& m, const char* name, const char* doc) {
  using TM = pycanha::gmm::TriMesh<Scalar>;
  nb::class_<TM>(m, name, doc)
      .def(nb::init<>(), "Create an empty mesh.")
      // Returning Eigen::Ref<const ...> makes nanobind hand out a non-owning,
      // non-writeable (read-only) numpy view; rv_policy::reference_internal ties
      // its lifetime to the mesh. Zero data copy.
      .def_prop_ro(
          "vertices",
          [](const TM& self) -> Eigen::Ref<const typename TM::VertexMatrix> {
            return self.vertices;
          },
          nb::rv_policy::reference_internal,
          "Vertex coordinates (Np x 3), read-only zero-copy view.")
      .def_prop_ro(
          "triangles",
          [](const TM& self) -> Eigen::Ref<const typename TM::TriangleMatrix> {
            return self.triangles;
          },
          nb::rv_policy::reference_internal,
          "Triangle vertex indices (Nt x 3), read-only zero-copy view.")
      .def_prop_ro(
          "face_ids",
          [](const TM& self) -> Eigen::Ref<const typename TM::FaceIdVector> {
            return self.face_ids;
          },
          nb::rv_policy::reference_internal,
          "Per-triangle face id (Nt); even = side 1, odd = side 2.")
      .def_prop_ro(
          "node_numbers",
          [](const TM& self) -> Eigen::Ref<const typename TM::NodeNumberVector> {
            return self.node_numbers;
          },
          nb::rv_policy::reference_internal,
          "Dense node numbers indexed by face_id (Nf); -1 = no node.")
      .def_prop_ro(
          "primitives",
          [](const TM& self) {
            nb::list out;
            for (const auto& range : self.primitives) {
              out.append(nb::make_tuple(
                  static_cast<std::uint64_t>(range.geometry_id),
                  range.first_face_id, range.last_face_id));
            }
            return out;
          },
          "List of (geometry_id, first_face_id, last_face_id) provenance "
          "ranges, sorted by first_face_id.")
      .def("np", &TM::np, "Number of vertices (== len(vertices)).")
      .def("nt", &TM::nt, "Number of triangles (== len(triangles)).")
      .def("nf", &TM::nf,
           "Number of faces the mesh owns, both sides counted. A cut removes "
           "triangles but never faces, so a pair whose triangles all went "
           "away keeps its ids and reports a zero area.")
      .def(
          "astype_float32",
          [](const TM& self) { return self.template cast<float>(); },
          "Return a float32 (TriMeshF) copy of this mesh.")
      .def(
          "astype_float64",
          [](const TM& self) { return self.template cast<double>(); },
          "Return a float64 (TriMeshD) copy of this mesh.");
}

}  // namespace pycanha::bindings::gmm::detail

inline void TriMeshD_b(nb::module_& m) {
  pycanha::bindings::gmm::detail::bind_trimesh<double>(
      m, "TriMeshD", "Triangular surface mesh with float64 vertices.");
}

inline void TriMeshF_b(nb::module_& m) {
  pycanha::bindings::gmm::detail::bind_trimesh<float>(
      m, "TriMeshF", "Triangular surface mesh with float32 vertices.");
}
