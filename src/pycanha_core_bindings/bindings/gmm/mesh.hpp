#pragma once
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/variant.h>

#include "pycanha-core/gmm/mesh/mesh_options.hpp"
#include "pycanha-core/gmm/mesh/ops/compute_areas.hpp"
#include "pycanha-core/gmm/mesh/ops/validate.hpp"
#include "pycanha-core/gmm/mesh/trimesh.hpp"
#include "pycanha-core/gmm/mesh/uv_mesher.hpp"

namespace nb = nanobind;
using namespace nanobind::literals;
using namespace pycanha::gmm;

inline void MeshOptions_b(nb::module_& m) {
  nb::class_<MeshOptions>(m, "MeshOptions", "Meshing tolerances.")
      .def(
          "__init__",
          [](MeshOptions* self, double deviation_tolerance) {
            new (self) MeshOptions{deviation_tolerance};
          },
          "deviation_tolerance"_a = 1e-3, "Create mesh options.")
      .def_rw("deviation_tolerance", &MeshOptions::deviation_tolerance,
              "Maximum chordal deviation tolerance [m].");
}

inline void UvMesher_b(nb::module_& m) {
  nb::class_<UvMesher>(m, "UvMesher",
                       "Meshes a single primitive against a ThermalMesh.")
      .def(nb::init<>(), "Create a UvMesher.")
      .def("mesh", &UvMesher::mesh, "primitive"_a, "thermal_mesh"_a, "options"_a,
           "Mesh a primitive into a TriMeshD.");
}

// mesh::ops free-function templates, bound as overloads over both precisions.
template <class Scalar>
inline void bind_mesh_ops_for(nb::module_& m) {
  namespace ops = pycanha::gmm::mesh::ops;
  m.def("compute_areas", &ops::compute_areas<Scalar>, "mesh"_a,
        "Per-triangle areas (M,).");
  m.def("compute_centroids", &ops::compute_centroids<Scalar>, "mesh"_a,
        "Per-triangle centroids (M, 3).");
  m.def("compute_face_normals", &ops::compute_face_normals<Scalar>, "mesh"_a,
        "Per-triangle unit normals (M, 3).");
  m.def(
      "bounding_box",
      [](const TriMesh<Scalar>& mesh) {
        const auto box = ops::bounding_box(mesh);
        return nb::make_tuple(box.min(), box.max());
      },
      "mesh"_a, "Axis-aligned bounding box as (min_xyz, max_xyz).");
  m.def("is_watertight", &ops::is_watertight<Scalar>, "mesh"_a,
        "Whether every edge is shared by exactly two triangles.");
  m.def("has_consistent_face_ids", &ops::has_consistent_face_ids<Scalar>,
        "mesh"_a, "Whether there is one face_id per triangle.");
}

inline void mesh_ops_b(nb::module_& m) {
  bind_mesh_ops_for<double>(m);
  bind_mesh_ops_for<float>(m);
}
