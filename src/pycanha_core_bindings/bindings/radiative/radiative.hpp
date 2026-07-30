#pragma once
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "pycanha-core/globals.hpp"
#include "pycanha-core/radiative/radiative.hpp"

namespace pycanha::bindings::radiative {

namespace nb = nanobind;
using namespace nanobind::literals;  // NOLINT(build/namespaces)

namespace rad = pycanha::radiative;
namespace gmm = pycanha::gmm;

// 1:1 nanobind exposure of pycanha::radiative. Zero policy: orbits, environments
// and the apply_to machinery live in the pure-Python pycanha.radiative layer.
// Core 0.17 completes the kernel set, so the whole public C++ surface is exposed
// here: the geometric view-factor, multi-bounce exchange and solar paths, the
// CPU Gebhart and face->node aggregation services, and the memory-sizing
// mechanism the Python layer's accumulator policy is built on.
inline void register_radiative(nb::module_& m) {
  // ---- virtual bucket columns ---------------------------------------------
  // Every matrix result appends these columns after the num_face_slots real
  // ones, so each parcel of emitted energy has an explicit destination and full
  // rows close exactly (a vf row sums to 1; an exchange row conserves energy).
  m.attr("num_virtual_columns") = rad::num_virtual_columns;
  m.attr("space_column_offset") = rad::space_column_offset;
  m.attr("inactive_column_offset") = rad::inactive_column_offset;
  m.attr("lost_column_offset") = rad::lost_column_offset;

  // ---- enums --------------------------------------------------------------
  nb::enum_<rad::PartKind>(m, "PartKind",
                           "Role of a part inside the raytraced scene.")
      .value("Spacecraft", rad::PartKind::Spacecraft)
      .value("Articulated", rad::PartKind::Articulated)
      .value("CelestialBody", rad::PartKind::CelestialBody);

  nb::enum_<rad::Band>(m, "Band", "Named radiation band (data-model dimension).")
      .value("IR", rad::Band::IR)
      .value("Solar", rad::Band::Solar);

  nb::enum_<rad::AccumLayout>(
      m, "AccumLayout",
      "Accumulator buffer layout. Both layouts yield bit-identical results for "
      "the same seed (cells accumulate as integers).")
      .value("Dense", rad::AccumLayout::Dense)
      .value("Tiled", rad::AccumLayout::Tiled);

  // ---- device -------------------------------------------------------------
  nb::class_<rad::DeviceInfo>(m, "DeviceInfo",
                              "One physical Vulkan device as seen by the "
                              "radiative engine.")
      .def_ro("name", &rad::DeviceInfo::name, "Device name reported by the driver.")
      .def_ro("ray_tracing", &rad::DeviceInfo::ray_tracing,
              "True when the device meets the full ray-tracing requirement set.")
      .def_ro("software", &rad::DeviceInfo::software,
              "True for a software implementation (e.g. lavapipe).")
      .def_ro("max_dispatch_rays", &rad::DeviceInfo::max_dispatch_rays,
              "Device-limit upper bound of rays per emitter in one dispatch.")
      .def_ro("index", &rad::DeviceInfo::index,
              "Position in enumerate_devices(); the argument for Device.create().")
      .def("__repr__", [](const rad::DeviceInfo& d) {
        return "<DeviceInfo index=" + std::to_string(d.index) + " name='" +
               d.name + "' ray_tracing=" + (d.ray_tracing ? "True" : "False") +
               " software=" + (d.software ? "True" : "False") + ">";
      });

  m.def("is_available", &rad::is_available,
        "Whether a ray-tracing-capable Vulkan device is present. Cheap and "
        "safe on machines with no Vulkan driver (returns False, never raises).");
  m.def("enumerate_devices", &rad::enumerate_devices,
        "List every Vulkan device visible to the engine (empty when none).");

  nb::class_<rad::Device>(
      m, "Device",
      "Owns the Vulkan instance/device/queues/allocator; one Device can serve "
      "many scenes.")
      .def_static("create", &rad::Device::create, "index"_a = -1,
                  "Create the device at `index` (from enumerate_devices()), or "
                  "the best ray-tracing-capable device when index < 0. Raises "
                  "RuntimeError when no capable device exists.")
      .def_prop_ro("info", &rad::Device::info,
                   nb::rv_policy::reference_internal,
                   "DeviceInfo for the selected device.")
      .def("memory_budget", &rad::Device::memory_budget,
           "DEVICE_LOCAL bytes currently available for new allocations on the "
           "largest heap.");

  // ---- scene inputs -------------------------------------------------------
  nb::class_<rad::ScenePart>(
      m, "ScenePart",
      "A rigid piece of the scene: one BLAS / one TLAS instance. Face ids stay "
      "global across parts.")
      .def(nb::init<>(), "Create a default (empty) scene part.")
      .def(
          "__init__",
          [](rad::ScenePart* self, gmm::TriMeshF mesh,
             gmm::CoordinateTransformation transform, rad::PartKind kind,
             std::uint32_t part_id) {
            new (self) rad::ScenePart{std::move(mesh), std::move(transform), kind,
                                      part_id};
          },
          "mesh"_a, "transform"_a, "kind"_a = rad::PartKind::Spacecraft,
          "part_id"_a = 0,
          "Build a part from its local-frame mesh, part->world transform, kind "
          "and caller-chosen part id.")
      .def_rw("mesh", &rad::ScenePart::mesh, "Part geometry (float32 TriMeshF).")
      .def_rw("transform", &rad::ScenePart::transform,
              "Part->world placement (CoordinateTransformation).")
      .def_rw("kind", &rad::ScenePart::kind, "Part role (PartKind).")
      .def_rw("part_id", &rad::ScenePart::part_id,
              "Caller-chosen identifier echoed in results/instancing.");

  nb::class_<rad::MaterialTable>(
      m, "MaterialTable",
      "Per-face optical material / activity tables consumed by the raytracer.")
      .def(nb::init<>(), "Create an empty material table.")
      .def(
          "__init__",
          [](rad::MaterialTable* self,
             Eigen::Matrix<float, Eigen::Dynamic, 6> properties,
             Eigen::VectorXi face_material,
             Eigen::Matrix<bool, Eigen::Dynamic, 1> face_active) {
            new (self) rad::MaterialTable{std::move(properties),
                                          std::move(face_material),
                                          std::move(face_active)};
          },
          "properties"_a, "face_material"_a, "face_active"_a,
          "Build from the (M, 6) material properties, the (Nf,) per-slot "
          "material index (-1 = none) and the (Nf,) per-slot activity mask.")
      .def_rw("properties", &rad::MaterialTable::properties,
              "(M, 6) rows [eps_ir, spec_ir, tau_ir, alpha_sol, spec_sol, "
              "tau_sol].")
      .def_rw("face_material", &rad::MaterialTable::face_material,
              "(Nf,) per-slot row index into `properties`, or -1.")
      .def_rw("face_active", &rad::MaterialTable::face_active,
              "(Nf,) per-slot emission/reception activity.")
      .def("num_materials",
           [](const rad::MaterialTable& t) { return t.num_materials(); },
           "Number of distinct materials (rows of `properties`).")
      .def("num_face_slots",
           [](const rad::MaterialTable& t) { return t.num_face_slots(); },
           "Number of face slots (rows of `face_material`).");

  // One solar snapshot. Everything above a single snapshot (orbits, dates,
  // eclipse sequencing) belongs to the pure-Python layer.
  nb::class_<rad::SolarState>(m, "SolarState",
                              "One solar-illumination snapshot: a parallel "
                              "sun (direction + irradiance).")
      .def(nb::init<>(), "Create a default (zero) solar state.")
      .def(
          "__init__",
          [](rad::SolarState* self, Vector3D direction, double irradiance) {
            new (self) rad::SolarState{std::move(direction), irradiance};
          },
          "direction"_a, "irradiance"_a = 0.0,
          "Build a sun snapshot from the world-frame sun->spacecraft "
          "direction (normalized internally) and the irradiance in W/m^2.")
      .def_rw("direction", &rad::SolarState::direction,
              "World-frame direction pointing sun -> spacecraft.")
      .def_rw("irradiance", &rad::SolarState::irradiance,
              "W/m^2 at the spacecraft (the solar constant for this "
              "snapshot).");

  // ---- settings -----------------------------------------------------------
  nb::class_<rad::TraceSettings>(
      m, "TraceSettings",
      "Monte-Carlo trace control. Accuracy is caller-driven: re-invoke the "
      "batch-additive accumulate calls until the reported statistics satisfy.")
      .def(nb::init<>(), "Create settings with the default values.")
      .def(
          "__init__",
          [](rad::TraceSettings* self, std::uint64_t rays_per_face,
             std::uint32_t seed, float energy_threshold,
             std::uint32_t max_bounces, bool normal_emission) {
            new (self) rad::TraceSettings{rays_per_face, seed, energy_threshold,
                                          max_bounces, normal_emission};
          },
          "rays_per_face"_a = 10'000, "seed"_a = 0,
          "energy_threshold"_a = 1e-4F, "max_bounces"_a = 64,
          "normal_emission"_a = false, "Build trace settings.")
      .def_rw("rays_per_face", &rad::TraceSettings::rays_per_face,
              "Rays per emitting face, per accumulate() call.")
      .def_rw("seed", &rad::TraceSettings::seed,
              "Deterministic seed; batches use seed + batch_index.")
      .def_rw("energy_threshold", &rad::TraceSettings::energy_threshold,
              "MCRT ray-kill energy cutoff (exchange kernels).")
      .def_rw("max_bounces", &rad::TraceSettings::max_bounces,
              "Hard safety bound on the bounce loop.")
      .def_rw("normal_emission", &rad::TraceSettings::normal_emission,
              "Emit along the face normal instead of the cosine-weighted "
              "hemisphere (the legacy `_Nodes` mode; vf/exchange only).");

  nb::class_<rad::AccumConfig>(
      m, "AccumConfig",
      "Accumulator configuration: layout, tile height and readback sparsity "
      "threshold.")
      .def(nb::init<>(), "Create the default (Dense) configuration.")
      .def(
          "__init__",
          [](rad::AccumConfig* self, rad::AccumLayout layout,
             std::uint32_t tile_rows, double sparse_threshold) {
            new (self) rad::AccumConfig{layout, tile_rows, sparse_threshold};
          },
          "layout"_a = rad::AccumLayout::Dense, "tile_rows"_a = 0,
          "sparse_threshold"_a = 0.0, "Build an accumulator configuration.")
      .def_rw("layout", &rad::AccumConfig::layout, "Dense or Tiled.")
      .def_rw("tile_rows", &rad::AccumConfig::tile_rows,
              "Tiled only; row-block height (0 is invalid, must be set).")
      .def_rw("sparse_threshold", &rad::AccumConfig::sparse_threshold,
              "Drop result entries with |value| <= threshold (0 keeps any "
              "nonzero). Row sums and statistics are computed BEFORE "
              "thresholding, so closure/conservation accounting stays exact.");

  // ---- results ------------------------------------------------------------
  nb::class_<rad::TraceStats>(
      m, "TraceStats",
      "Monte-Carlo convergence statistics carried by every result.")
      .def_ro("total_rays", &rad::TraceStats::total_rays, "Total rays traced.")
      .def_ro("rays_per_face", &rad::TraceStats::rays_per_face,
              "Cumulative rays per face across accumulate() calls.")
      .def_ro("mean_stderr", &rad::TraceStats::mean_stderr,
              "Mean per-entry standard-error estimate.")
      .def_ro("max_stderr", &rad::TraceStats::max_stderr,
              "Max per-entry standard-error estimate.")
      .def_ro("reciprocity_residual", &rad::TraceStats::reciprocity_residual,
              "VF only: max |Ai*Fij - Aj*Fji| (normalized).")
      .def_ro("lost_energy_fraction", &rad::TraceStats::lost_energy_fraction,
              "Exchange only: the mathematical residue (Russian-roulette "
              "balance, max_bounces cutoff). Absorption at inactive faces is "
              "its own bucket column, not part of this.")
      .def_prop_ro(
          "gpu_time_ns",
          [](const rad::TraceStats& s) {
            return static_cast<std::int64_t>(s.gpu_time.count());
          },
          "GPU trace time in nanoseconds.");

  // Minimal CSR container; the three arrays are exposed zero-copy so a consumer
  // can wrap them in scipy.sparse.csr_matrix((values, indices, indptr), shape)
  // without a copy.
  nb::class_<rad::SparseF64>(m, "SparseF64",
                             "Minimal CSR matrix (rows/cols are face slots).")
      .def(nb::init<>(), "Create an empty (0x0) sparse matrix.")
      .def(
          "__init__",
          [](rad::SparseF64* self, Eigen::VectorX<std::int64_t> indptr,
             Eigen::VectorX<std::int32_t> indices, Eigen::VectorXd values,
             std::int64_t rows, std::int64_t cols) {
            new (self) rad::SparseF64{std::move(indptr), std::move(indices),
                                      std::move(values), rows, cols};
          },
          "indptr"_a, "indices"_a, "values"_a, "rows"_a, "cols"_a,
          "Build a CSR matrix from its arrays and shape (scipy csr layout).")
      .def_prop_ro(
          "indptr",
          [](const rad::SparseF64& s)
              -> Eigen::Ref<const Eigen::VectorX<std::int64_t>> {
            return s.indptr;
          },
          nb::rv_policy::reference_internal,
          "CSR row-pointer array (rows + 1), zero-copy view.")
      .def_prop_ro(
          "indices",
          [](const rad::SparseF64& s)
              -> Eigen::Ref<const Eigen::VectorX<std::int32_t>> {
            return s.indices;
          },
          nb::rv_policy::reference_internal,
          "CSR column-index array, zero-copy view.")
      .def_prop_ro(
          "values",
          [](const rad::SparseF64& s) -> Eigen::Ref<const Eigen::VectorXd> {
            return s.values;
          },
          nb::rv_policy::reference_internal, "CSR stored values, zero-copy view.")
      .def_ro("rows", &rad::SparseF64::rows, "Number of rows.")
      .def_ro("cols", &rad::SparseF64::cols, "Number of columns.")
      .def("nnz", &rad::SparseF64::nnz, "Number of stored entries.")
      .def_prop_ro(
          "shape",
          [](const rad::SparseF64& s) {
            return nb::make_tuple(s.rows, s.cols);
          },
          "(rows, cols) tuple.");

  nb::class_<rad::VfResult>(
      m, "VfResult",
      "Geometric view-factor result: the VF matrix, row sums and statistics.")
      .def_prop_ro(
          "vf",
          [](const rad::VfResult& r) -> const rad::SparseF64& { return r.vf; },
          nb::rv_policy::reference_internal,
          "View-factor matrix (SparseF64), Nf x (Nf + num_virtual_columns); "
          "the view to space is the space bucket column, not a row deficit.")
      .def_prop_ro(
          "row_sums",
          [](const rad::VfResult& r) -> Eigen::Ref<const Eigen::VectorXd> {
            return r.row_sums;
          },
          nb::rv_policy::reference_internal,
          "Per-row sums over ALL columns (Nf,) — exactly 1 for rows that "
          "emitted, i.e. the closure check.")
      .def_prop_ro(
          "stats",
          [](const rad::VfResult& r) -> const rad::TraceStats& {
            return r.stats;
          },
          nb::rv_policy::reference_internal, "TraceStats for this result.");

  nb::class_<rad::ExchangeResult>(
      m, "ExchangeResult",
      "Multi-bounce MCRT exchange-factor result for one radiation band.")
      .def_ro("band", &rad::ExchangeResult::band,
              "The band the factors were traced for.")
      .def_prop_ro(
          "factors",
          [](const rad::ExchangeResult& r) -> const rad::SparseF64& {
            return r.factors;
          },
          nb::rv_policy::reference_internal,
          "Exchange-factor matrix (SparseF64), Nf x (Nf + "
          "num_virtual_columns); the full row conserves energy exactly.")
      .def_prop_ro(
          "stats",
          [](const rad::ExchangeResult& r) -> const rad::TraceStats& {
            return r.stats;
          },
          nb::rv_policy::reference_internal, "TraceStats for this result.");

  nb::class_<rad::SolarResult>(
      m, "SolarResult",
      "Per-face-slot absorbed solar power in WATTS (extensive): node mapping "
      "is a plain per-node sum, flux is watts / face area.")
      .def_prop_ro(
          "direct",
          [](const rad::SolarResult& r) -> Eigen::Ref<const Eigen::VectorXd> {
            return r.direct;
          },
          nb::rv_policy::reference_internal,
          "(Nf,) W absorbed from direct illumination.")
      .def_prop_ro(
          "total",
          [](const rad::SolarResult& r) -> Eigen::Ref<const Eigen::VectorXd> {
            return r.total;
          },
          nb::rv_policy::reference_internal,
          "(Nf,) W absorbed including reflections.")
      .def_prop_ro(
          "stats",
          [](const rad::SolarResult& r) -> const rad::TraceStats& {
            return r.stats;
          },
          nb::rv_policy::reference_internal, "TraceStats for this result.");

  // ---- memory sizing ------------------------------------------------------
  nb::class_<rad::MemoryEstimate>(
      m, "MemoryEstimate",
      "Exact byte requirements of an accumulator configuration; the Python "
      "layer weighs these against Device.memory_budget() to pick a layout.")
      .def_ro("gpu_bytes_dense", &rad::MemoryEstimate::gpu_bytes_dense,
              "Full Nf x (Nf + virtual columns) accumulator, sized for the u64 "
              "exchange cells (vf counting cells take half).")
      .def_ro("gpu_bytes_per_tile_row",
              &rad::MemoryEstimate::gpu_bytes_per_tile_row,
              "One row of the tiled block scratch.")
      .def_ro("gpu_bytes_scene", &rad::MemoryEstimate::gpu_bytes_scene,
              "Resident scene cost: geometry, acceleration structures, tables.")
      .def_ro("host_bytes_block", &rad::MemoryEstimate::host_bytes_block,
              "One readback block for the configured layout.");

  // ---- scene + accumulator ------------------------------------------------
  nb::class_<rad::RadiativeScene>(
      m, "RadiativeScene",
      "The stateful engine ('build once, compute many'): uploads geometry + "
      "materials, builds acceleration structures, traces batch-additively.")
      .def(
          "__init__",
          [](rad::RadiativeScene* self, rad::Device& device,
             std::vector<rad::ScenePart> parts, rad::MaterialTable materials) {
            new (self) rad::RadiativeScene(device, std::move(parts),
                                           std::move(materials));
          },
          "device"_a, "parts"_a, "materials"_a, nb::keep_alive<1, 2>(),
          "Build a scene on `device` (which must outlive the scene) from the "
          "parts and material table.")
      .def("set_part_transform", &rad::RadiativeScene::set_part_transform,
           "part_id"_a, "world_transform"_a,
           "Replace one part's world placement; call commit() afterwards.")
      .def("commit", &rad::RadiativeScene::commit,
           "Rebuild the instance structure after transform changes (cheap).")
      .def(
          "accumulate_vf",
          [](rad::RadiativeScene& self, rad::VfAccumulator& acc,
             const rad::TraceSettings& settings,
             const std::vector<std::uint32_t>& emitters) {
            self.accumulate_vf(acc, settings,
                               std::span<const std::uint32_t>(emitters));
          },
          "acc"_a, "settings"_a, "emitters"_a = std::vector<std::uint32_t>{},
          nb::call_guard<nb::gil_scoped_release>(),
          "Trace settings.rays_per_face rays per emitting face and ADD the "
          "first-hit counts into `acc`. Empty `emitters` => all active faces "
          "emit. Releases the GIL while the GPU works.")
      .def(
          "accumulate_exchange",
          [](rad::RadiativeScene& self, rad::ExchangeAccumulator& acc,
             const rad::TraceSettings& settings,
             const std::vector<std::uint32_t>& emitters) {
            self.accumulate_exchange(acc, settings,
                                     std::span<const std::uint32_t>(emitters));
          },
          "acc"_a, "settings"_a, "emitters"_a = std::vector<std::uint32_t>{},
          nb::call_guard<nb::gil_scoped_release>(),
          "Multi-bounce MCRT exchange factors for the accumulator's band "
          "(fixed at accumulator construction), ADDED into `acc`. Same "
          "emitter semantics as accumulate_vf. Releases the GIL.")
      .def(
          "accumulate_solar",
          [](rad::RadiativeScene& self, const rad::SolarState& sun,
             rad::SolarAccumulator& acc, const rad::TraceSettings& settings) {
            self.accumulate_solar(sun, acc, settings);
          },
          "sun"_a, "acc"_a, "settings"_a,
          nb::call_guard<nb::gil_scoped_release>(),
          "Direct + reflected solar absorption for one sun snapshot, ADDED "
          "into `acc`. Every batch in one accumulator must use the same "
          "SolarState (only the seed varies); a different sun needs a fresh "
          "accumulator. Releases the GIL.")
      .def("update_materials", &rad::RadiativeScene::update_materials,
           "materials"_a,
           "Replace the optical properties WITHOUT rebuilding geometry "
           "(BOL/EOL swaps, sensitivity overrides): the new table must keep "
           "the same face_material mapping and activity.")
      .def("num_face_slots", &rad::RadiativeScene::num_face_slots,
           "Total face slots (rows/cols of every result matrix).")
      .def("materials", &rad::RadiativeScene::materials,
           nb::rv_policy::reference_internal, "The scene's MaterialTable.")
      .def(
          "face_areas",
          [](const rad::RadiativeScene& self) {
            const std::span<const double> areas = self.face_areas();
            return Eigen::VectorXd(
                Eigen::Map<const Eigen::VectorXd>(areas.data(),
                                                  static_cast<Eigen::Index>(
                                                      areas.size())));
          },
          "Per-slot mesh areas (Nf,); sides share the pair area.");

  nb::class_<rad::VfAccumulator>(
      m, "VfAccumulator",
      "Owns the GPU counting buffer for view factors; batch-additive across "
      "accumulate_vf calls.")
      .def(
          "__init__",
          [](rad::VfAccumulator* self, const rad::RadiativeScene& scene,
             rad::AccumConfig config) {
            new (self) rad::VfAccumulator(scene, config);
          },
          "scene"_a, "config"_a = rad::AccumConfig{}, nb::keep_alive<1, 2>(),
          "Allocate an accumulator for `scene` (Dense by default).")
      .def("reset", &rad::VfAccumulator::reset,
           "Clear all accumulated counts.")
      .def("result", &rad::VfAccumulator::result,
           "Read back + normalize into a VfResult (callable repeatedly).");

  nb::class_<rad::ExchangeAccumulator>(
      m, "ExchangeAccumulator",
      "Owns the GPU fixed-point energy cells for exchange factors; the band "
      "is fixed at construction (mixing bands in one matrix is meaningless).")
      .def(
          "__init__",
          [](rad::ExchangeAccumulator* self, const rad::RadiativeScene& scene,
             rad::Band band, rad::AccumConfig config) {
            new (self) rad::ExchangeAccumulator(scene, band, config);
          },
          "scene"_a, "band"_a, "config"_a = rad::AccumConfig{},
          nb::keep_alive<1, 2>(),
          "Allocate an accumulator for `scene` in `band` (Dense by default).")
      .def("reset", &rad::ExchangeAccumulator::reset,
           "Clear all accumulated energy.")
      .def("result", &rad::ExchangeAccumulator::result,
           "Read back + normalize into an ExchangeResult (callable "
           "repeatedly).")
      .def("conservation_error", &rad::ExchangeAccumulator::conservation_error,
           "Max over rows of |full row sum - rays * scale| in raw fixed-point "
           "units. Zero by construction, so a nonzero value means a broken "
           "kernel, not Monte-Carlo noise.");

  nb::class_<rad::SolarAccumulator>(
      m, "SolarAccumulator",
      "Owns the per-face-slot direct/total solar energy vectors (the solar "
      "kernel is O(Nf): no matrix, no layout choice).")
      .def(
          "__init__",
          [](rad::SolarAccumulator* self, const rad::RadiativeScene& scene) {
            new (self) rad::SolarAccumulator(scene);
          },
          "scene"_a, nb::keep_alive<1, 2>(),
          "Allocate a solar accumulator for `scene`.")
      .def("reset", &rad::SolarAccumulator::reset,
           "Clear all accumulated energy.")
      .def("result", &rad::SolarAccumulator::result,
           "Read back into a SolarResult in watts (callable repeatedly).");

  m.def("estimate_memory", &rad::estimate_memory, "scene"_a,
        "config"_a = rad::AccumConfig{},
        "Byte requirements of `config` on `scene`, so a run can fail fast "
        "with a clear message instead of exhausting device memory mid-trace.");

  // ---- CPU Gebhart services ----------------------------------------------
  m.def(
      "gebhart_factors", &rad::gebhart_factors, "vf"_a, "emissivity"_a,
      "space_fraction_policy"_a = 1.0,
      "Face-level Gebhart factors B = (I - F R)^-1 F E from a geometric VF "
      "matrix — the diffuse-gray fast path, no re-tracing and no GPU. The "
      "dense solve is limited to ~20k face slots; use gebhart_node_factors "
      "above that. `space_fraction_policy` decides what a row deficit means: "
      "1.0 a real view to space, 0.0 renormalize the row (closed enclosure).");

  m.def(
      "gebhart_node_factors",
      [](const rad::SparseF64& vf, const Eigen::VectorXd& emissivity,
         const Eigen::VectorXi& node_numbers, const Eigen::VectorXd& face_areas,
         double space_fraction_policy) {
        return rad::gebhart_node_factors(
            vf, emissivity,
            std::span<const NodeNum>(node_numbers.data(),
                                     static_cast<std::size_t>(
                                         node_numbers.size())),
            std::span<const double>(face_areas.data(),
                                    static_cast<std::size_t>(
                                        face_areas.size())),
            space_fraction_policy);
      },
      "vf"_a, "emissivity"_a, "node_numbers"_a, "face_areas"_a,
      "space_fraction_policy"_a = 1.0,
      "Node-level Gebhart GR matrix (m^2) for ANY model size: one sparse "
      "factorization plus one solve per node instead of a dense inverse. "
      "Rows/cols are indexed by position in aggregate_nodes(node_numbers).");

  // ---- CPU aggregation services ------------------------------------------
  m.def(
      "aggregate_nodes",
      [](const Eigen::VectorXi& node_numbers) {
        const std::vector<NodeNum> nodes = rad::aggregate_nodes(
            std::span<const NodeNum>(node_numbers.data(),
                                     static_cast<std::size_t>(
                                         node_numbers.size())));
        return Eigen::VectorXi(
            Eigen::Map<const Eigen::VectorXi>(nodes.data(),
                                              static_cast<Eigen::Index>(
                                                  nodes.size())));
      },
      "node_numbers"_a,
      "Sorted unique node numbers with NO_NODE (-1) removed — the row/col "
      "labels of the aggregated outputs.");

  m.def(
      "aggregate_matrix",
      [](const rad::SparseF64& face_matrix, const Eigen::VectorXi& node_numbers,
         const Eigen::VectorXd& face_areas) {
        return rad::aggregate_matrix(
            face_matrix,
            std::span<const NodeNum>(node_numbers.data(),
                                     static_cast<std::size_t>(
                                         node_numbers.size())),
            std::span<const double>(face_areas.data(),
                                    static_cast<std::size_t>(
                                        face_areas.size())));
      },
      "face_matrix"_a, "node_numbers"_a, "face_areas"_a,
      "Area-weighted face->node reduction of a face matrix (extensive, m^2). "
      "The virtual bucket columns are DROPPED (they have no column label); "
      "use the row/column overload to map a bucket to a real node.");

  m.def(
      "aggregate_matrix",
      [](const rad::SparseF64& face_matrix,
         const Eigen::VectorXi& row_node_numbers,
         const Eigen::VectorXi& col_node_numbers,
         const Eigen::VectorXd& face_areas) {
        return rad::aggregate_matrix(
            face_matrix,
            std::span<const NodeNum>(row_node_numbers.data(),
                                     static_cast<std::size_t>(
                                         row_node_numbers.size())),
            std::span<const NodeNum>(col_node_numbers.data(),
                                     static_cast<std::size_t>(
                                         col_node_numbers.size())),
            std::span<const double>(face_areas.data(),
                                    static_cast<std::size_t>(
                                        face_areas.size())));
      },
      "face_matrix"_a, "row_node_numbers"_a, "col_node_numbers"_a,
      "face_areas"_a,
      "Rows and columns labeled independently: `col_node_numbers` has one "
      "entry per matrix COLUMN including the virtual buckets, so the space "
      "bucket can be assigned the space node (or NO_NODE to drop it).");

  m.def(
      "aggregate_flux",
      [](const Eigen::VectorXd& face_flux_w_m2,
         const Eigen::VectorXi& node_numbers, const Eigen::VectorXd& face_areas) {
        return rad::aggregate_flux(
            face_flux_w_m2,
            std::span<const NodeNum>(node_numbers.data(),
                                     static_cast<std::size_t>(
                                         node_numbers.size())),
            std::span<const double>(face_areas.data(),
                                    static_cast<std::size_t>(
                                        face_areas.size())));
      },
      "face_flux_w_m2"_a, "node_numbers"_a, "face_areas"_a,
      "Face->node flux reduction: W per node from W/m^2 per face.");
}

}  // namespace pycanha::bindings::radiative
