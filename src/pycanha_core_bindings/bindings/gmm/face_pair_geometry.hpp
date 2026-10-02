#pragma once
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/variant.h>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "pycanha-core/globals.hpp"
#include "pycanha-core/gmm/mesh/thermal_mesh.hpp"
#include "pycanha-core/gmm/primitives/face_pair_geometry.hpp"
#include "pycanha-core/gmm/primitives/primitive.hpp"

namespace nb = nanobind;
using namespace nanobind::literals;

namespace pycanha::bindings::gmm {

// FacePairGeometryEvaluator does not check its indices: an (i, j) outside the
// thermal mesh reads past its per-interval arrays. Python gets the evaluator
// together with the grid size it was built for, and every call is checked
// against it.
class FacePairGeometryGrid {
  public:
    FacePairGeometryGrid(const pycanha::gmm::Primitive& primitive,
                         const pycanha::gmm::ThermalMesh& thermal_mesh)
        : _evaluator(primitive, thermal_mesh),
          _dir1_count(intervals(thermal_mesh.get_dir1_mesh().size())),
          _dir2_count(intervals(thermal_mesh.get_dir2_mesh().size())) {}

    [[nodiscard]] bool is_supported() const noexcept { return _evaluator.is_supported(); }
    [[nodiscard]] pycanha::MeshIndex dir1_count() const noexcept { return _dir1_count; }
    [[nodiscard]] pycanha::MeshIndex dir2_count() const noexcept { return _dir2_count; }

    [[nodiscard]] pycanha::gmm::FacePairGeometry at(pycanha::MeshIndex i,
                                                    pycanha::MeshIndex j) const {
        if (i >= _dir1_count || j >= _dir2_count) {
            throw nb::index_error(("face pair (" + std::to_string(i) + ", " + std::to_string(j) +
                                   ") is outside the " + std::to_string(_dir1_count) + " x " +
                                   std::to_string(_dir2_count) + " grid")
                                      .c_str());
        }
        return _evaluator(i, j);
    }

    // Every face pair, direction 1 fastest: area (n,) and centroid (n, 3).
    // Empty for a primitive with no face pairs.
    [[nodiscard]] std::pair<std::vector<double>, std::vector<double>> all() const {
        std::vector<double> areas;
        std::vector<double> centroids;
        if (!_evaluator.is_supported()) {
            return {std::move(areas), std::move(centroids)};
        }
        const std::size_t count = static_cast<std::size_t>(_dir1_count) * _dir2_count;
        areas.reserve(count);
        centroids.reserve(3 * count);
        for (pycanha::MeshIndex j = 0; j < _dir2_count; ++j) {
            for (pycanha::MeshIndex i = 0; i < _dir1_count; ++i) {
                const pycanha::gmm::FacePairGeometry geometry = _evaluator(i, j);
                areas.push_back(geometry.area);
                centroids.push_back(geometry.centroid.x());
                centroids.push_back(geometry.centroid.y());
                centroids.push_back(geometry.centroid.z());
            }
        }
        return {std::move(areas), std::move(centroids)};
    }

  private:
    [[nodiscard]] static pycanha::MeshIndex intervals(std::size_t cuts) noexcept {
        return cuts == 0 ? 0 : static_cast<pycanha::MeshIndex>(cuts - 1);
    }

    pycanha::gmm::FacePairGeometryEvaluator _evaluator;
    pycanha::MeshIndex _dir1_count;
    pycanha::MeshIndex _dir2_count;
};

using AreaArray = nb::ndarray<nb::numpy, double, nb::ndim<1>>;
using CentroidArray = nb::ndarray<nb::numpy, double, nb::ndim<2>>;

// numpy arrays owning the two vectors of FacePairGeometryGrid::all().
[[nodiscard]] inline std::pair<AreaArray, CentroidArray> to_numpy_arrays(
    std::pair<std::vector<double>, std::vector<double>> arrays) {
    auto* areas = new std::vector<double>(std::move(arrays.first));
    auto* centroids = new std::vector<double>(std::move(arrays.second));
    nb::capsule areas_owner(areas, [](void* pointer) noexcept {
        delete static_cast<std::vector<double>*>(pointer);
    });
    nb::capsule centroids_owner(centroids, [](void* pointer) noexcept {
        delete static_cast<std::vector<double>*>(pointer);
    });
    const std::size_t count = areas->size();
    const std::size_t area_shape[1] = {count};
    const std::size_t centroid_shape[2] = {count, 3};
    return {AreaArray(areas->data(), 1, area_shape, areas_owner),
            CentroidArray(centroids->data(), 2, centroid_shape, centroids_owner)};
}

constexpr const char* face_pair_evaluator_doc =
    "The exact geometry of every face pair of one primitive under its thermal "
    "mesh.\n\n"
    "Exact for the geometry definition, not for its triangulation: no mesh is "
    "built. Face pair (i, j) is the part of the surface between cuts i and "
    "i + 1 of direction 1 and j and j + 1 of direction 2, in the "
    "parametrisation the mesher samples, and both faces of the pair share it. "
    "Results are in the primitive's own frame and ignore any cut. Rectangle, "
    "Triangle and Quadrilateral are planar bilinear patches (a face pair is "
    "two triangles with straight edges); Disc, Cylinder, Cone, Sphere and "
    "Paraboloid integrate in closed form. Cube and TriangularPrism have no "
    "face pairs.\n\n"
    "Construction is O(n1 + n2); each face pair then costs O(1). Calling it "
    "with (i, j) gives one face pair; all() gives every face pair as arrays.";

constexpr const char* face_pair_geometry_doc =
    "Exact area and centroid of face pair (i, j) of a primitive under its "
    "thermal mesh, in the primitive's own frame, ignoring any cut. Builds the "
    "evaluator on every call: for many face pairs use "
    "FacePairGeometryEvaluator, which documents the method. Raises IndexError "
    "outside the grid.";

inline void FacePairGeometry_b(nb::module_& m) {
    using pycanha::gmm::FacePairGeometry;

    nb::class_<FacePairGeometry>(m, "FacePairGeometry",
                                 "Exact area and area-weighted centroid of one face pair.")
        .def_ro("area", &FacePairGeometry::area, "Area of the face pair [m^2].")
        .def_ro("centroid", &FacePairGeometry::centroid,
                "Area-weighted centroid, in the primitive's own frame [m].")
        .def("__repr__", [](const FacePairGeometry& geometry) {
            return "<FacePairGeometry area=" + std::to_string(geometry.area) + ">";
        });

    nb::class_<FacePairGeometryGrid>(
        m, "FacePairGeometryEvaluator", face_pair_evaluator_doc)
        .def(nb::init<const pycanha::gmm::Primitive&, const pycanha::gmm::ThermalMesh&>(),
             "primitive"_a, "thermal_mesh"_a,
             "Precompute the per-interval integrals of the primitive under the mesh.")
        .def("is_supported", &FacePairGeometryGrid::is_supported,
             "False for the cutter-only primitives (Cube, TriangularPrism), "
             "which have no face pairs.")
        .def_prop_ro("dir1_count", &FacePairGeometryGrid::dir1_count,
                     "Face pairs along direction 1.")
        .def_prop_ro("dir2_count", &FacePairGeometryGrid::dir2_count,
                     "Face pairs along direction 2.")
        .def("__call__", &FacePairGeometryGrid::at, "i"_a, "j"_a,
             "Face pair (i, j), direction 1 first. Raises IndexError outside "
             "the grid. A primitive with no face pairs gives zero area.")
        .def(
            "all",
            [](const FacePairGeometryGrid& self) { return to_numpy_arrays(self.all()); },
            nb::rv_policy::move,
            "Every face pair, direction 1 fastest (entry i + j * dir1_count): a "
            "tuple (areas (n,), centroids (n, 3)). Empty for a primitive with no "
            "face pairs.");

    m.def(
        "face_pair_geometry",
        [](const pycanha::gmm::Primitive& primitive,
           const pycanha::gmm::ThermalMesh& thermal_mesh, pycanha::MeshIndex i,
           pycanha::MeshIndex j) { return FacePairGeometryGrid(primitive, thermal_mesh).at(i, j); },
        "primitive"_a, "thermal_mesh"_a, "i"_a, "j"_a, face_pair_geometry_doc);
}

}  // namespace pycanha::bindings::gmm
