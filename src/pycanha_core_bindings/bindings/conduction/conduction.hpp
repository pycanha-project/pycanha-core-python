#pragma once
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "bindings/tmm/bulk.hpp"
#include "bindings/utils/logger.hpp"
#include "pycanha-core/conduction/conduction.hpp"
#include "pycanha-core/conduction/network_part.hpp"
#include "pycanha-core/gmm/scene/coordinate_transformation.hpp"
#include "pycanha-core/gmm/scene/geometry_item.hpp"
#include "pycanha-core/tmm/thermalmathematicalmodel.hpp"
#include "pycanha-core/gmm/mesh/thermal_mesh.hpp"
#include "pycanha-core/gmm/primitives/primitive.hpp"
#include "pycanha-core/tmm/thermalmodel.hpp"

namespace pycanha::bindings::conduction {

namespace nb = nanobind;
using namespace nanobind::literals;  // NOLINT(build/namespaces)

// The enclosing namespace shadows pycanha::conduction, so spell the core
// namespace out once and use the alias everywhere below.
namespace cond = ::pycanha::conduction;

// 1:1 nanobind exposure of pycanha::conduction, the gmm -> tmm conduction
// builder. The entry point is ThermalModel.build_tmm_from_gmm (bound with the
// rest of ThermalModel in bindings/tmm); this module carries its options, its
// report, and the two link-level services a caller can use to check one
// primitive without building a whole model.
//
// MeridianProfile is exposed read-only, through profile_of(): its make_*
// factories take primitive-specific parameter tuples that only profile_of
// knows how to fill correctly.
inline void register_conduction(nb::module_& m) {
  // ---- options ------------------------------------------------------------
  nb::class_<cond::TmmBuildOptions>(m, "TmmBuildOptions",
                                    "Knobs of the gmm -> tmm conduction build.")
      .def(nb::init<>(), "Create options with the default values.")
      .def(
          "__init__",
          [](cond::TmmBuildOptions* self, double initial_temperature,
             bool intra_primitive_conductors, bool through_thickness_conductors,
             bool close_full_revolution, double min_conductance) {
            new (self) cond::TmmBuildOptions{
                initial_temperature, intra_primitive_conductors,
                through_thickness_conductors, close_full_revolution,
                min_conductance};
          },
          "initial_temperature"_a = 0.0, "intra_primitive_conductors"_a = true,
          "through_thickness_conductors"_a = true,
          "close_full_revolution"_a = true, "min_conductance"_a = 0.0,
          "Build the option set.")
      .def_rw("initial_temperature", &cond::TmmBuildOptions::initial_temperature,
              "Temperature written to every generated node.")
      .def_rw("intra_primitive_conductors",
              &cond::TmmBuildOptions::intra_primitive_conductors,
              "Generate the in-plane conductors of each primitive's own face "
              "grid.")
      .def_rw("through_thickness_conductors",
              &cond::TmmBuildOptions::through_thickness_conductors,
              "Generate the side-1 <-> side-2 conductors of each face pair.")
      .def_rw("close_full_revolution",
              &cond::TmmBuildOptions::close_full_revolution,
              "Close the conductor ring between the last and the first angular "
              "face of a primitive that spans a full revolution.")
      .def_rw("min_conductance", &cond::TmmBuildOptions::min_conductance,
              "Generated conductors at or below this value are dropped. The "
              "default keeps everything except exact zeros.");

  // ---- diagnostics --------------------------------------------------------
  nb::enum_<cond::DiagnosticCode>(
      m, "DiagnosticCode",
      "Why the builder skipped something or had to approximate it. A "
      "diagnostic never fails the build.")
      .value("CutFacePairs", cond::DiagnosticCode::CutFacePairs,
             "Face pairs of an item that other geometry cuts: their capacity "
             "and through-thickness conductance are scaled by the area that "
             "survives, and the in-plane conductors touching them are "
             "removed. Face pairs cut away completely contribute nothing.")
      .value("UncoupledNodes", cond::DiagnosticCode::UncoupledNodes,
             "Diffusive nodes left with no conductive coupling once the "
             "in-plane conductors of cut face pairs were removed.")
      .value("UnmeshedPrimitive", cond::DiagnosticCode::UnmeshedPrimitive,
             "The primitive produces no faces at all (Cube and "
             "TriangularPrism are cutter-only).")
      .value("InactiveSideSkipped", cond::DiagnosticCode::InactiveSideSkipped,
             "A side carrying node numbers that one of the active-side "
             "selectors excludes: either it takes part in neither physics, and "
             "so contributes nothing at all, or it is radiative only, and so "
             "defines nodes with capacitance but no conductors.")
      .value("MissingBulk", cond::DiagnosticCode::MissingBulk,
             "No bulk material on an active side: the side still defines "
             "nodes, but with no capacitance and no conductors.")
      .value("ZeroThickness", cond::DiagnosticCode::ZeroThickness,
             "Zero thickness on an active side.")
      .value("ZeroConductivity", cond::DiagnosticCode::ZeroConductivity,
             "Zero conductivity on a conductively active side.")
      .value("MixedBulkOnNode", cond::DiagnosticCode::MixedBulkOnNode,
             "The two sides mapped to one node carry different bulk "
             "materials; the contributions are summed anyway.")
      .value("DiscreteLinkFallback", cond::DiagnosticCode::DiscreteLinkFallback,
             "The primitive has no closed-form conduction profile -- a "
             "triangle's fan parametrisation is not orthogonal, a "
             "quadrilateral's bilinear face pairs vary in width -- so its "
             "conductors come from the discrete shared-edge path instead.")
      .value("NoNodeNumbers", cond::DiagnosticCode::NoNodeNumbers,
             "The item has no node numbers assigned on any active side.")
      .value("DegenerateFacePair", cond::DiagnosticCode::DegenerateFacePair,
             "A face with zero parametric extent, which carries no "
             "conductance.")
      .value("AxisSingularity", cond::DiagnosticCode::AxisSingularity,
             "A face band reaching the axis of revolution, whose "
             "around-the-axis conductance uses the near-axis form.");

  m.def(
      "diagnostic_code_name",
      [](cond::DiagnosticCode code) {
        return std::string(cond::to_string(code));
      },
      "code"_a, "The C++ spelling of a DiagnosticCode, for log messages.");

  nb::class_<cond::BuildDiagnostic>(
      m, "BuildDiagnostic",
      "One thing the builder skipped or approximated, and where.")
      .def_ro("code", &cond::BuildDiagnostic::code, "The DiagnosticCode.")
      .def_ro("geometry_name", &cond::BuildDiagnostic::geometry_name,
              "Name of the geometry the diagnostic refers to.")
      .def_ro("message", &cond::BuildDiagnostic::message,
              "Human-readable explanation.")
      .def("__repr__", [](const cond::BuildDiagnostic& d) {
        return "<BuildDiagnostic " + std::string(cond::to_string(d.code)) +
               " '" + d.geometry_name + "'>";
      });

  nb::class_<cond::TmmBuildReport>(
      m, "TmmBuildReport",
      "What one gmm -> tmm build produced, plus its diagnostics.")
      .def_ro("items_processed", &cond::TmmBuildReport::items_processed,
              "Geometry items the builder walked.")
      .def_ro("items_skipped", &cond::TmmBuildReport::items_skipped,
              "Geometry items the builder could not use.")
      .def_ro("nodes_created", &cond::TmmBuildReport::nodes_created,
              "Nodes added to the tmm.")
      .def_ro("conductors_created", &cond::TmmBuildReport::conductors_created,
              "Conductive couplings added to the tmm, at node-pair level "
              "(after aggregation).")
      .def_ro("face_pair_links_computed", &cond::TmmBuildReport::face_pair_links_computed,
              "In-plane links computed at face level, before aggregation.")
      .def_ro("face_pairs_cut", &cond::TmmBuildReport::face_pairs_cut,
              "Face pairs partly cut away (0 < surviving fraction < 1).")
      .def_ro("face_pairs_removed", &cond::TmmBuildReport::face_pairs_removed,
              "Face pairs cut away completely.")
      .def_ro("links_removed", &cond::TmmBuildReport::links_removed,
              "In-plane links dropped because they touch a cut face pair.")
      .def_ro("diagnostics", &cond::TmmBuildReport::diagnostics,
              "List of BuildDiagnostic entries.")
      .def("__repr__", [](const cond::TmmBuildReport& r) {
        return "<TmmBuildReport nodes=" + std::to_string(r.nodes_created) +
               " conductors=" + std::to_string(r.conductors_created) +
               " diagnostics=" + std::to_string(r.diagnostics.size()) + ">";
      });

  m.def("build_tmm_from_gmm", &cond::build_tmm_from_gmm, "model"_a,
        "options"_a = cond::TmmBuildOptions{}, nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
        "Populate the model's tmm from its gmm: one node per ACTIVE face "
        "that carries a node number — active meaning it takes part in "
        "conduction, radiation or both — plus the in-plane and "
        "through-thickness conductors the conductively active ones imply. A "
        "node fed only by radiative-only faces therefore exists, with "
        "capacitance, but with no conductor attached. Every item builds its "
        "own network part from its definition (exact capacities, no "
        "triangulation unless it is cut); the parts are merged with the bulk "
        "calls. The node area is not set: see assign_node_areas. Radiative "
        "couplings, "
        "parameters, formulas and thermal data are left untouched. Raises "
        "ValueError when the tmm already holds nodes or conductive couplings "
        "(there is no merge semantics). Same as ThermalModel."
        "build_tmm_from_gmm.");

  // ---- network parts ------------------------------------------------------
  using pycanha::bindings::tmm::view_of;
  nb::class_<cond::NetworkPart>(
      m, "NetworkPart",
      "One geometry item's contribution to the conduction network, built from "
      "its definition alone. Nodes are sorted by number; couplings join two "
      "nodes of the part by their position in node_numbers "
      "(coupling_index_1 < coupling_index_2), sorted. The arrays are "
      "read-only views into the part.")
      .def_prop_ro("node_numbers", [](cond::NetworkPart& self) {
        return view_of(self.node_numbers, nb::find(self)); }, "Node numbers, sorted.")
      .def_prop_ro("thermal_capacity", [](cond::NetworkPart& self) {
        return view_of(self.thermal_capacity, nb::find(self)); }, "Thermal capacity per node [J/K].")
      .def_prop_ro("position_x", [](cond::NetworkPart& self) {
        return view_of(self.position_x, nb::find(self)); }, "Node X coordinate, root frame [m].")
      .def_prop_ro("position_y", [](cond::NetworkPart& self) {
        return view_of(self.position_y, nb::find(self)); }, "Node Y coordinate, root frame [m].")
      .def_prop_ro("position_z", [](cond::NetworkPart& self) {
        return view_of(self.position_z, nb::find(self)); }, "Node Z coordinate, root frame [m].")
      .def_prop_ro("position_weight", [](cond::NetworkPart& self) {
        return view_of(self.position_weight, nb::find(self)); },
        "Weight behind each position (the node's surviving face area over its "
        "active sides).")
      .def_prop_ro("coupling_index_1", [](cond::NetworkPart& self) {
        return view_of(self.coupling_index_1, nb::find(self)); },
        "First node of each coupling, as a position in node_numbers.")
      .def_prop_ro("coupling_index_2", [](cond::NetworkPart& self) {
        return view_of(self.coupling_index_2, nb::find(self)); },
        "Second node of each coupling, as a position in node_numbers.")
      .def_prop_ro("conductance", [](cond::NetworkPart& self) {
        return view_of(self.conductance, nb::find(self)); }, "Conductance of each coupling [W/K].")
      .def_prop_ro("node_sides", [](cond::NetworkPart& self) {
        return view_of(self.node_sides, nb::find(self)); },
        "Which sides fed each node: bit 0 side 1, bit 1 side 2. With "
        "side_bulk, what tells a node that gathers two different bulk "
        "materials when parts are merged.")
      .def_prop_ro("side_bulk", [](const cond::NetworkPart& self) {
        using Material = std::optional<pycanha::gmm::BulkMaterial>;
        const auto copy_of = [](const pycanha::gmm::BulkMaterial* material) -> Material {
          return material == nullptr ? Material{} : Material{*material};
        };
        return std::pair<Material, Material>{copy_of(self.side_bulk[0]),
                                             copy_of(self.side_bulk[1])}; },
        "Bulk material of side 1 and side 2 (None when the side has none), as "
        "copies. The part refers to the item's materials: replacing a material "
        "on the item after building the part invalidates the part.")
      .def_prop_ro("coupling_node_1", [](const cond::NetworkPart& self) {
        std::vector<pycanha::NodeNum> nodes;
        nodes.reserve(self.coupling_index_1.size());
        for (const std::int32_t local : self.coupling_index_1) {
          nodes.push_back(self.node_numbers[static_cast<std::size_t>(local)]);
        }
        return pycanha::bindings::tmm::to_numpy(std::move(nodes)); },
        nb::rv_policy::move,
        "First node number of each coupling (a new array).")
      .def_prop_ro("coupling_node_2", [](const cond::NetworkPart& self) {
        std::vector<pycanha::NodeNum> nodes;
        nodes.reserve(self.coupling_index_2.size());
        for (const std::int32_t local : self.coupling_index_2) {
          nodes.push_back(self.node_numbers[static_cast<std::size_t>(local)]);
        }
        return pycanha::bindings::tmm::to_numpy(std::move(nodes)); },
        nb::rv_policy::move,
        "Second node number of each coupling (a new array).")
      .def_ro("report", &cond::NetworkPart::report,
              "This item's share of the build report.")
      .def("__repr__", [](const cond::NetworkPart& part) {
        return "<NetworkPart nodes=" + std::to_string(part.node_numbers.size()) +
               " couplings=" + std::to_string(part.conductance.size()) + ">";
      });

  m.def(
      "build_network_part",
      [](const pycanha::gmm::GeometryItem& item,
         const pycanha::gmm::CoordinateTransformation& to_root,
         const cond::TmmBuildOptions& options,
         const std::optional<pycanha::bindings::tmm::DoubleArray>& surviving_fraction,
         const std::optional<nb::ndarray<const double, nb::shape<-1, 3>, nb::c_contig,
                                         nb::device::cpu>>& surviving_centroid) {
        std::vector<pycanha::Vector3D> centroids;
        if (surviving_centroid.has_value()) {
          const std::size_t rows = surviving_centroid->shape(0);
          const double* data = surviving_centroid->data();
          centroids.reserve(rows);
          for (std::size_t row = 0; row < rows; ++row) {
            centroids.emplace_back(data[3 * row], data[(3 * row) + 1], data[(3 * row) + 2]);
          }
        }
        return cond::build_network_part(
            item, to_root, options, pycanha::bindings::tmm::span_of(surviving_fraction),
            centroids);
      },
      "item"_a, "to_root"_a, "options"_a = cond::TmmBuildOptions{},
      "surviving_fraction"_a = nb::none(), "surviving_centroid"_a = nb::none(),
      nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(), nb::keep_alive<0, 1>(),
      "The network part of one item: exact capacities, the parametric "
      "in-plane conductances and the through-thickness ones. to_root places "
      "the item (it moves the positions only). A cut item passes, per face "
      "pair, the surviving fraction of its area and the (n, 3) root-frame "
      "centroid of what survives.");

  m.def(
      "commit_network_parts",
      [](pycanha::ThermalMathematicalModel& tmm,
         const std::vector<const cond::NetworkPart*>& parts,
         const cond::TmmBuildOptions& options) {
        return cond::commit_network_parts(tmm, parts, options);
      },
      "tmm"_a, "parts"_a, "options"_a = cond::TmmBuildOptions{},
      nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
      "Merge network parts and write them into an empty tmm with the bulk "
      "calls (parts are not copied). Parts whose node numbers do not "
      "interleave are appended as they are; others are merged first. Raises "
      "ValueError when the tmm already holds nodes or conductive couplings.");

  m.def("assign_node_areas", &cond::assign_node_areas, "model"_a,
        nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
        "Set the node area `a` from the model's triangulation: the area of "
        "every triangulated face on an active side, summed per node. This "
        "triangulates the gmm; build_tmm_from_gmm never sets `a`. Returns a "
        "BulkReport.");

  // ---- link-level services ------------------------------------------------
  nb::class_<cond::FacePairLink>(
      m, "FacePairLink",
      "An in-plane conductor between two adjacent face pairs of one item. "
      "face_pair_a and face_pair_b are linear indices of the ThermalMesh "
      "face-pair grid, "
      "k = i + j * (n1 - 1) with direction 1 varying fastest.")
      .def_ro("face_pair_a", &cond::FacePairLink::face_pair_a,
              "First face-pair index.")
      .def_ro("face_pair_b", &cond::FacePairLink::face_pair_b,
              "Second face-pair index.")
      .def_ro("side", &cond::FacePairLink::side,
              "1 or 2: which of the two sheets this conductor belongs to. The "
              "link always joins the SAME side of both faces.")
      .def_ro("conductance", &cond::FacePairLink::conductance, "W/K.")
      .def("__repr__", [](const cond::FacePairLink& link) {
        return "<FacePairLink " + std::to_string(link.face_pair_a) + "-" +
               std::to_string(link.face_pair_b) +
               " side=" + std::to_string(link.side) + ">";
      });

  m.def("intra_primitive_links", &cond::intra_primitive_links, "primitive"_a,
        "thermal_mesh"_a, "options"_a = cond::TmmBuildOptions{},
        "In-plane conductors of one item's face-pair grid: pure geometry and "
        "material, no model and no node numbers. A side contributes only when "
        "it is conductively active and carries both a bulk material with "
        "non-zero conductivity and a non-zero thickness.");

  m.def(
      "for_each_intra_primitive_link",
      [](const pycanha::gmm::Primitive& primitive, const pycanha::gmm::ThermalMesh& thermal_mesh,
         const cond::TmmBuildOptions& options,
         const nb::typed<nb::callable, void(const cond::FacePairLink&)>& links) {
        // Each link is handed over as a copy: the C++ one is a temporary.
        cond::for_each_intra_primitive_link(
            primitive, thermal_mesh, options, [&links](const cond::FacePairLink& link) {
              links(nb::cast(link, nb::rv_policy::copy));
            });
      },
      "primitive"_a, "thermal_mesh"_a, "options"_a, "links"_a,
      "The same links as intra_primitive_links, passed one by one to the "
      "callable `links(link)` as they are computed instead of collected in a "
      "list. An exception raised by the callable stops the walk and "
      "propagates.");

  m.def("through_thickness_conductance", &cond::through_thickness_conductance,
        "thermal_mesh"_a, "pair_area"_a,
        "Conductance through the thickness of one face pair: the two "
        "half-slabs in series, A / (t1/k1 + t2/k2). Zero when a side is "
        "conductively inactive, has no bulk material, or has zero thickness "
        "or conductivity.");

  // ---- meridian profile ---------------------------------------------------
  nb::class_<cond::MeridianProfile> profile(
      m, "MeridianProfile",
      "The two scalar maps one-dimensional conduction needs from a primitive: "
      "the coordinate heat flows along in direction 1, and the potential "
      "Phi = integral of dl2 / rho along the meridian. Obtained from "
      "profile_of().");

  nb::enum_<cond::MeridianProfile::Kind>(
      profile, "Kind", "Which surface of revolution the profile describes.")
      .value("Planar", cond::MeridianProfile::Kind::Planar)
      .value("Disc", cond::MeridianProfile::Kind::Disc)
      .value("Cylinder", cond::MeridianProfile::Kind::Cylinder)
      .value("Cone", cond::MeridianProfile::Kind::Cone)
      .value("Sphere", cond::MeridianProfile::Kind::Sphere)
      .value("Paraboloid", cond::MeridianProfile::Kind::Paraboloid);

  profile
      .def_prop_ro("kind", &cond::MeridianProfile::kind,
                   "The profile Kind.")
      .def_prop_ro("closes_ring", &cond::MeridianProfile::closes_ring,
                   "True when direction 1 spans a full revolution, so the last "
                   "angular face is adjacent to the first one.")
      .def_prop_ro("dir1_extent", &cond::MeridianProfile::dir1_extent,
                   "Total direction-1 extent, dir1_coordinate(1) - "
                   "dir1_coordinate(0).")
      .def("dir1_coordinate", &cond::MeridianProfile::dir1_coordinate,
           "fraction"_a,
           "Coordinate along direction 1 at a cut fraction in [0, 1]: radians "
           "for a surface of revolution, metres for a planar primitive.")
      .def("rho", &cond::MeridianProfile::rho, "fraction"_a,
           "Distance from the axis of revolution at a cut fraction. "
           "Identically one (and dimensionless) for a planar primitive.")
      .def("on_axis", &cond::MeridianProfile::on_axis, "fraction"_a,
           "True where rho vanishes: the potential is unbounded there and the "
           "direction-1 band must be integrated from its reference line.")
      .def("potential", &cond::MeridianProfile::potential, "fraction"_a,
           "Phi at a cut fraction, up to an additive constant that cancels in "
           "every difference the conduction model takes.")
      .def("meridian_length", &cond::MeridianProfile::meridian_length, "low"_a,
           "high"_a, "Meridian arc length between two cut fractions.");

  m.def("profile_of", &cond::profile_of, "primitive"_a,
        "The conduction profile of a primitive, or None when it has no closed "
        "form: a Triangle (its fan parametrisation is not orthogonal) or a "
        "Quadrilateral (a bilinear patch, so its face pairs vary in width) -- "
        "both handled by the discrete shared-edge path -- or a Cube or a "
        "TriangularPrism, which are cutter-only and never mesh.");
}

}  // namespace pycanha::bindings::conduction
