#pragma once
#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

#include <string>

#include "bindings/utils/logger.hpp"
#include "pycanha-core/conduction/conduction.hpp"
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
      .value("CutGeometrySkipped", cond::DiagnosticCode::CutGeometrySkipped,
             "Geometry inside a boolean-cut group: its face grid no longer "
             "exists, so the parametric integrals do not apply.")
      .value("UnmeshedPrimitive", cond::DiagnosticCode::UnmeshedPrimitive,
             "The primitive produces no faces at all (Cube is cutter-only).")
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
      .value("TriangleApproximated", cond::DiagnosticCode::TriangleApproximated,
             "A triangle's fan parametrisation is not orthogonal, so its "
             "conductors come from the discrete shared-edge fallback rather "
             "than a closed form.")
      .value("NoNodeNumbers", cond::DiagnosticCode::NoNodeNumbers,
             "The item has no node numbers assigned on any active side.")
      .value("DegenerateCell", cond::DiagnosticCode::DegenerateCell,
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
      .def_ro("cell_links_computed", &cond::TmmBuildReport::cell_links_computed,
              "In-plane links computed at face level, before aggregation.")
      .def_ro("diagnostics", &cond::TmmBuildReport::diagnostics,
              "List of BuildDiagnostic entries.")
      .def("__repr__", [](const cond::TmmBuildReport& r) {
        return "<TmmBuildReport nodes=" + std::to_string(r.nodes_created) +
               " conductors=" + std::to_string(r.conductors_created) +
               " diagnostics=" + std::to_string(r.diagnostics.size()) + ">";
      });

  m.def("build_tmm_from_gmm", &cond::build_tmm_from_gmm, "model"_a,
        "options"_a = cond::TmmBuildOptions{}, nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
        "Populate the model's tmm from its gmm: one node per ACTIVE face slot "
        "that carries a node number — active meaning it takes part in "
        "conduction, radiation or both — plus the in-plane and "
        "through-thickness conductors the conductively active ones imply. A "
        "node fed only by radiative-only slots therefore exists, with "
        "capacitance and area, but with no conductor attached. Radiative "
        "couplings, "
        "parameters, formulas and thermal data are left untouched. Raises "
        "ValueError when the tmm already holds nodes or conductive couplings "
        "(there is no merge semantics). Same as ThermalModel."
        "build_tmm_from_gmm.");

  // ---- link-level services ------------------------------------------------
  nb::class_<cond::CellLink>(
      m, "CellLink",
      "An in-plane conductor between two adjacent faces of one item. cell_a "
      "and cell_b are linear indices of the ThermalMesh face grid, "
      "k = i + j * (n1 - 1) with direction 1 varying fastest.")
      .def_ro("cell_a", &cond::CellLink::cell_a, "First face index.")
      .def_ro("cell_b", &cond::CellLink::cell_b, "Second face index.")
      .def_ro("side", &cond::CellLink::side,
              "1 or 2: which of the two sheets this conductor belongs to. The "
              "link always joins the SAME side of both faces.")
      .def_ro("conductance", &cond::CellLink::conductance, "W/K.")
      .def("__repr__", [](const cond::CellLink& link) {
        return "<CellLink " + std::to_string(link.cell_a) + "-" +
               std::to_string(link.cell_b) +
               " side=" + std::to_string(link.side) + ">";
      });

  m.def("intra_primitive_links", &cond::intra_primitive_links, "primitive"_a,
        "thermal_mesh"_a, "options"_a = cond::TmmBuildOptions{},
        "In-plane conductors of one item's face grid: pure geometry and "
        "material, no model and no node numbers. A side contributes only when "
        "it is conductively active and carries both a bulk material with "
        "non-zero conductivity and a non-zero thickness.");

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
        "form: a Triangle (handled by the discrete fallback) or a Cube "
        "(cutter-only, it never meshes).");
}

}  // namespace pycanha::bindings::conduction
