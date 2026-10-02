#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/vector.h>
#include <spdlog/spdlog.h>

#include "bindings/utils/logger.hpp"
#include "pycanha-core/tmm/bulk.hpp"
#include "pycanha-core/tmm/node.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::tmm {

// Arrays read in place: a C-contiguous 1-D array of the exact dtype passes
// without a copy; anything else is converted once by nanobind.
template <typename T>
using InArray = nb::ndarray<const T, nb::ndim<1>, nb::c_contig, nb::device::cpu>;
using DoubleArray = InArray<double>;
using OptionalDoubleArray = std::optional<DoubleArray>;

template <typename T> [[nodiscard]] std::span<const T> span_of(const InArray<T> &array) {
  return {array.data(), array.shape(0)};
}

[[nodiscard]] inline std::span<const double> span_of(const OptionalDoubleArray &array) {
  return array.has_value() ? span_of(*array) : std::span<const double>{};
}

// A numpy array that owns `values`.
template <typename T>
[[nodiscard]] nb::ndarray<nb::numpy, T, nb::ndim<1>> to_numpy(std::vector<T> values) {
  auto *owned = new std::vector<T>(std::move(values));
  nb::capsule owner(owned, [](void *pointer) noexcept {
    delete static_cast<std::vector<T> *>(pointer);
  });
  const std::size_t shape[1] = {owned->size()};
  return nb::ndarray<nb::numpy, T, nb::ndim<1>>(owned->data(), 1, shape, owner);
}

// A read-only numpy view of a vector owned by the Python object `owner`.
template <typename T>
[[nodiscard]] nb::ndarray<nb::numpy, const T, nb::ndim<1>> view_of(const std::vector<T> &values,
                                                                    nb::handle owner) {
  const std::size_t shape[1] = {values.size()};
  return nb::ndarray<nb::numpy, const T, nb::ndim<1>>(values.data(), 1, shape, owner);
}

// Every 64-bit array argument of the bulk calls comes through one of the two
// helpers below. nanobind converts an array of another integer dtype to the
// parameter's dtype, and numpy wraps a value that does not fit: a node number
// above 2^31 would silently become another node. So the int64 overloads are
// registered first (nanobind tries exact dtypes before conversions, so int64
// and every other dtype except int32 lands there) and narrow here, with a
// range check.

[[nodiscard]] inline bool fits_int32(std::int64_t number) {
  return number >= std::numeric_limits<std::int32_t>::min() &&
         number <= std::numeric_limits<std::int32_t>::max();
}

// 64-bit numbers narrowed to int32, or nothing when one of them is outside the
// int32 range; the whole call is then rejected and reported. For the calls
// that reject a whole batch on a bad entry (add_nodes, set_types,
// append_couplings).
[[nodiscard]] inline std::optional<std::vector<std::int32_t>>
narrow_node_numbers(std::span<const std::int64_t> numbers, BulkReport &report,
                    const char *what = "node number") {
  std::vector<std::int32_t> narrowed;
  narrowed.reserve(numbers.size());
  for (const std::int64_t number : numbers) {
    if (!fits_int32(number)) {
      report.rejected = numbers.size();
      report.first_rejections.push_back(std::string(what) + " " + std::to_string(number) +
                                        " is outside the int32 range: batch rejected");
      return std::nullopt;
    }
    narrowed.push_back(static_cast<std::int32_t>(number));
  }
  return narrowed;
}

// The entries of one or two 64-bit node-number arrays that fit NodeNum, with
// their positions in the caller's arrays. An entry with a number outside the
// int32 range cannot name a node, so it is treated as an unknown node: the
// getters read NaN for it and the setters skip it, as the core does for any
// unknown node.
struct NarrowedEntries {
  std::vector<NodeNum> first;
  std::vector<NodeNum> second;
  std::vector<std::size_t> positions;
  BulkReport out_of_range;

  [[nodiscard]] bool complete() const { return out_of_range.rejected == 0; }
};

[[nodiscard]] inline NarrowedEntries narrow_entries(std::span<const std::int64_t> first,
                                                    std::span<const std::int64_t> second = {}) {
  const bool pairs = !second.empty();
  NarrowedEntries entries;
  entries.first.reserve(first.size());
  entries.positions.reserve(first.size());
  if (pairs) {
    entries.second.reserve(first.size());
  }
  for (std::size_t entry = 0; entry < first.size(); ++entry) {
    const std::int64_t number_2 = pairs ? second[entry] : 0;
    if (!fits_int32(first[entry]) || !fits_int32(number_2)) {
      entries.out_of_range.reject(
          "node number " +
          std::to_string(fits_int32(first[entry]) ? number_2 : first[entry]) +
          " is outside the int32 range");
      continue;
    }
    entries.first.push_back(static_cast<NodeNum>(first[entry]));
    if (pairs) {
      entries.second.push_back(static_cast<NodeNum>(number_2));
    }
    entries.positions.push_back(entry);
  }
  return entries;
}

// The one WARN for the entries narrow_entries() set aside.
inline void warn_out_of_range(const char *call, const BulkReport &out_of_range) {
  SPDLOG_LOGGER_WARN(pycanha::get_logger(), "{}: {} entries skipped, first: {}", call,
                     out_of_range.rejected, out_of_range.first_rejections.front());
}

// Adds the entries narrow_entries() set aside to the report of the call made
// with the others.
[[nodiscard]] inline BulkReport merge_reports(BulkReport report, const BulkReport &out_of_range) {
  report.rejected += out_of_range.rejected;
  for (const std::string &rejection : out_of_range.first_rejections) {
    if (report.first_rejections.size() >= BulkReport::max_reported_rejections) {
      break;
    }
    report.first_rejections.push_back(rejection);
  }
  return report;
}

// The report of a call whose arrays do not have the same length, made before
// the arrays are narrowed (the per-entry split needs equal lengths).
[[nodiscard]] inline BulkReport length_mismatch(const char *call, std::size_t entries) {
  BulkReport report;
  report.rejected = entries;
  report.first_rejections.emplace_back("arrays differ in length");
  SPDLOG_LOGGER_WARN(pycanha::get_logger(), "{}: {} entries rejected: arrays differ in length",
                     call, entries);
  return report;
}

// `read` fills its output for the narrowed entries; the rest stay NaN.
template <typename Read>
[[nodiscard]] std::vector<double> read_narrowed(const char *call, const NarrowedEntries &entries,
                                                std::size_t size, const Read &read) {
  std::vector<double> values(size, std::numeric_limits<double>::quiet_NaN());
  if (entries.complete()) {
    read(std::span<double>(values));
    return values;
  }
  std::vector<double> found(entries.positions.size());
  read(std::span<double>(found));
  for (std::size_t entry = 0; entry < found.size(); ++entry) {
    values[entries.positions[entry]] = found[entry];
  }
  warn_out_of_range(call, entries.out_of_range);
  return values;
}

// `write` stores the values of the narrowed entries; its report gains the rest.
template <typename Write>
[[nodiscard]] BulkReport write_narrowed(const char *call, const NarrowedEntries &entries,
                                        std::span<const double> values, const Write &write) {
  if (entries.complete()) {
    return write(values);
  }
  std::vector<double> kept;
  kept.reserve(entries.positions.size());
  for (const std::size_t position : entries.positions) {
    kept.push_back(values[position]);
  }
  warn_out_of_range(call, entries.out_of_range);
  return merge_reports(write(std::span<const double>(kept)), entries.out_of_range);
}

inline void register_bulk_types(nb::module_ &m) {
  nb::class_<BulkReport>(m, "BulkReport",
                         "Outcome of one bulk call on the nodes or the couplings.\n\n"
                         "A bulk call never fails half-way: every entry is either "
                         "taken into the model or rejected, and rejected entries "
                         "leave the model untouched.")
      .def_ro("accepted", &BulkReport::accepted, "Entries written into the model.")
      .def_ro("merged", &BulkReport::merged,
              "Couplings only: accepted entries that met an existing coupling, "
              "or an earlier entry of the same call, resolved by the merge mode.")
      .def_ro("rejected", &BulkReport::rejected,
              "Entries dropped: unknown or duplicated node, bad value, ...")
      .def_ro("first_rejections", &BulkReport::first_rejections,
              "The first few rejections, described.")
      .def("__repr__", [](const BulkReport &report) {
        return "<BulkReport accepted=" + std::to_string(report.accepted) +
               " merged=" + std::to_string(report.merged) +
               " rejected=" + std::to_string(report.rejected) + ">";
      });

  nb::enum_<CouplingMerge>(m, "CouplingMerge",
                           "What a bulk coupling insertion does with a coupling that "
                           "already exists, or that the call gives more than once.")
      .value("OVERWRITE", CouplingMerge::OVERWRITE,
             "The later value replaces the earlier one (as add_coupling).")
      .value("SUM", CouplingMerge::SUM, "The values are added, in call order.")
      .value("NEW", CouplingMerge::NEW, "The first value is kept, later ones are dropped.");

  nb::enum_<NodeAttribute>(m, "NodeAttribute",
                           "Node attribute selector for the bulk getters and setters.")
      .value("T", NodeAttribute::T, "Temperature [K].")
      .value("C", NodeAttribute::C, "Thermal capacity [J/K].")
      .value("QS", NodeAttribute::QS, "Solar heat load [W].")
      .value("QA", NodeAttribute::QA, "Albedo heat load [W].")
      .value("QE", NodeAttribute::QE, "Earth IR heat load [W].")
      .value("QI", NodeAttribute::QI, "Internal heat load [W].")
      .value("QR", NodeAttribute::QR, "Other heat load [W].")
      .value("A", NodeAttribute::A, "Area [m^2].")
      .value("FX", NodeAttribute::FX, "X coordinate [m].")
      .value("FY", NodeAttribute::FY, "Y coordinate [m].")
      .value("FZ", NodeAttribute::FZ, "Z coordinate [m].")
      .value("EPS", NodeAttribute::EPS, "IR emissivity.")
      .value("APH", NodeAttribute::APH, "Solar absorptivity.");
}

constexpr const char *add_nodes_doc =
    "Add a batch of nodes of one type from arrays.\n\n"
    "Every attribute array given must have the length of `numbers`; one left "
    "as None takes its default (zero). A batch sorted by increasing number, "
    "all above the numbers already stored in its block, is appended: O(k). "
    "Any other batch is sorted and merged in. Rejected, and reported: every "
    "copy of a number repeated in the batch, a number that is already a node, "
    "a number outside the int32 range, and the whole batch when an attribute "
    "array has the wrong length. Pointers handed out for formula binding are "
    "invalidated.";

// add_nodes(numbers, type, T, C, qs, ...) on any class forwarding to
// Nodes::add_nodes. The int64 overload comes first: nanobind tries exact
// dtypes before conversions, and a converted array then goes through the
// range-checked 64-bit path rather than being wrapped into int32.
template <typename Class, typename Forward>
void bind_add_nodes(nb::class_<Class> &cls, const Forward &forward) {
  const auto batch_of = [](char type, std::span<const NodeNum> numbers,
                           const OptionalDoubleArray &temperature, const OptionalDoubleArray &capacity,
                           const OptionalDoubleArray &qs, const OptionalDoubleArray &qa,
                           const OptionalDoubleArray &qe, const OptionalDoubleArray &qi,
                           const OptionalDoubleArray &qr, const OptionalDoubleArray &a,
                           const OptionalDoubleArray &fx, const OptionalDoubleArray &fy,
                           const OptionalDoubleArray &fz, const OptionalDoubleArray &eps,
                           const OptionalDoubleArray &aph) {
    return NodeBatch{.type = type,
                     .numbers = numbers,
                     .temperature = span_of(temperature),
                     .capacity = span_of(capacity),
                     .qs = span_of(qs),
                     .qa = span_of(qa),
                     .qe = span_of(qe),
                     .qi = span_of(qi),
                     .qr = span_of(qr),
                     .a = span_of(a),
                     .fx = span_of(fx),
                     .fy = span_of(fy),
                     .fz = span_of(fz),
                     .eps = span_of(eps),
                     .aph = span_of(aph)};
  };
  cls.def(
         "add_nodes",
         [forward, batch_of](Class &self, const InArray<std::int64_t> &numbers, NodeType type,
                             const OptionalDoubleArray &T, const OptionalDoubleArray &C,
                             const OptionalDoubleArray &qs, const OptionalDoubleArray &qa,
                             const OptionalDoubleArray &qe, const OptionalDoubleArray &qi,
                             const OptionalDoubleArray &qr, const OptionalDoubleArray &a,
                             const OptionalDoubleArray &fx, const OptionalDoubleArray &fy,
                             const OptionalDoubleArray &fz, const OptionalDoubleArray &eps,
                             const OptionalDoubleArray &aph) {
           BulkReport out_of_range;
           const auto narrowed = narrow_node_numbers(span_of(numbers), out_of_range);
           if (!narrowed.has_value()) {
             return out_of_range;
           }
           return forward(self, batch_of(static_cast<char>(type), *narrowed, T, C, qs, qa, qe, qi,
                                         qr, a, fx, fy, fz, eps, aph));
         },
         "numbers"_a, "type"_a = NodeType::DIFFUSIVE_NODE, "T"_a = nb::none(),
         "C"_a = nb::none(), "qs"_a = nb::none(), "qa"_a = nb::none(), "qe"_a = nb::none(),
         "qi"_a = nb::none(), "qr"_a = nb::none(), "a"_a = nb::none(), "fx"_a = nb::none(),
         "fy"_a = nb::none(), "fz"_a = nb::none(), "eps"_a = nb::none(), "aph"_a = nb::none(),
         nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(), add_nodes_doc)
      .def(
          "add_nodes",
          [forward, batch_of](Class &self, const InArray<NodeNum> &numbers, NodeType type,
                              const OptionalDoubleArray &T, const OptionalDoubleArray &C,
                              const OptionalDoubleArray &qs, const OptionalDoubleArray &qa,
                              const OptionalDoubleArray &qe, const OptionalDoubleArray &qi,
                              const OptionalDoubleArray &qr, const OptionalDoubleArray &a,
                              const OptionalDoubleArray &fx, const OptionalDoubleArray &fy,
                              const OptionalDoubleArray &fz, const OptionalDoubleArray &eps,
                              const OptionalDoubleArray &aph) {
            return forward(self, batch_of(static_cast<char>(type), span_of(numbers), T, C, qs, qa,
                                          qe, qi, qr, a, fx, fy, fz, eps, aph));
          },
          "numbers"_a, "type"_a = NodeType::DIFFUSIVE_NODE, "T"_a = nb::none(),
          "C"_a = nb::none(), "qs"_a = nb::none(), "qa"_a = nb::none(), "qe"_a = nb::none(),
          "qi"_a = nb::none(), "qr"_a = nb::none(), "a"_a = nb::none(), "fx"_a = nb::none(),
          "fy"_a = nb::none(), "fz"_a = nb::none(), "eps"_a = nb::none(), "aph"_a = nb::none(),
          nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(), add_nodes_doc);
}

constexpr const char *add_couplings_doc =
    "Add many couplings from arrays of node numbers and values.\n\n"
    "Entry i couples node_1[i] and node_2[i] (either order) with values[i]. "
    "The arrays are read in place and every value is written once, straight "
    "into the coupling storage. Fastest when the entries are sorted by "
    "(smaller node, larger node) -- for an all-diffusive model -- and come "
    "after every coupling already stored: they are then appended. Otherwise "
    "they are sorted and merged in place. A coupling that already exists, or "
    "that the call gives twice, is resolved by `merge` (OVERWRITE by default, "
    "as add_coupling). Dropped, and reported: an unknown node, the same node "
    "on both ends, a negative or non-finite value, a number outside the int32 "
    "range.";

constexpr const char *append_couplings_doc =
    "Append couplings given by INTERNAL node index (offset + idx), with no "
    "node-number lookup, no sorting and no merging: the fastest path, for "
    "callers that already know the internal order. Within each block "
    "(diffusive-diffusive, diffusive-boundary, boundary-boundary) the "
    "entries must be strictly increasing in (lower index, higher index) "
    "and come after every coupling already stored; the order is checked "
    "and a call that breaks it, or holds an index outside the int32 range, "
    "is rejected whole.";

constexpr const char *coupling_get_values_doc =
    "Values of many couplings; a missing coupling or node reads as NaN.";

constexpr const char *coupling_set_values_doc =
    "Change the values of existing couplings; a missing coupling or node, "
    "or a negative value, is skipped and reported.";

// The bulk coupling methods of Couplings, ConductiveCouplings and
// RadiativeCouplings, which share their names.
template <typename Class> void bind_bulk_couplings(nb::class_<Class> &cls) {
  cls.def(
         "add_couplings",
         [](Class &self, const InArray<std::int64_t> &node_1, const InArray<std::int64_t> &node_2,
            const DoubleArray &values, CouplingMerge merge) {
           return self.add_couplings(span_of(node_1), span_of(node_2), span_of(values), merge);
         },
         "node_1"_a, "node_2"_a, "values"_a, "merge"_a = CouplingMerge::OVERWRITE,
         nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(), add_couplings_doc)
      .def(
          "add_couplings",
          [](Class &self, const InArray<NodeNum> &node_1, const InArray<NodeNum> &node_2,
             const DoubleArray &values, CouplingMerge merge) {
            return self.add_couplings(span_of(node_1), span_of(node_2), span_of(values), merge);
          },
          "node_1"_a, "node_2"_a, "values"_a, "merge"_a = CouplingMerge::OVERWRITE,
          nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(), add_couplings_doc)
      .def(
          "append_couplings",
          [](Class &self, const InArray<std::int64_t> &idx_1, const InArray<std::int64_t> &idx_2,
             const DoubleArray &values, std::int32_t offset) {
            BulkReport out_of_range;
            const auto narrowed_1 = narrow_node_numbers(span_of(idx_1), out_of_range, "index");
            if (!narrowed_1.has_value()) {
              return out_of_range;
            }
            const auto narrowed_2 = narrow_node_numbers(span_of(idx_2), out_of_range, "index");
            if (!narrowed_2.has_value()) {
              return out_of_range;
            }
            const std::vector<CouplingChunk> chunks{CouplingChunk{.idx_1 = *narrowed_1,
                                                                  .idx_2 = *narrowed_2,
                                                                  .values = span_of(values),
                                                                  .offset = offset}};
            return self.append_couplings(chunks);
          },
          "idx_1"_a, "idx_2"_a, "values"_a, "offset"_a = 0,
          nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(), append_couplings_doc)
      .def(
          "append_couplings",
          [](Class &self, const InArray<std::int32_t> &idx_1, const InArray<std::int32_t> &idx_2,
             const DoubleArray &values, std::int32_t offset) {
            const std::vector<CouplingChunk> chunks{CouplingChunk{.idx_1 = span_of(idx_1),
                                                                  .idx_2 = span_of(idx_2),
                                                                  .values = span_of(values),
                                                                  .offset = offset}};
            return self.append_couplings(chunks);
          },
          "idx_1"_a, "idx_2"_a, "values"_a, "offset"_a = 0,
          nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(), append_couplings_doc)
      .def(
          "get_values",
          [](Class &self, const InArray<std::int64_t> &node_1, const InArray<std::int64_t> &node_2) {
            if (node_1.shape(0) != node_2.shape(0)) {
              static_cast<void>(length_mismatch("get_values", node_1.shape(0)));
              return to_numpy(std::vector<double>(node_1.shape(0),
                                                  std::numeric_limits<double>::quiet_NaN()));
            }
            const NarrowedEntries entries = narrow_entries(span_of(node_1), span_of(node_2));
            return to_numpy(read_narrowed(
                "get_values", entries, node_1.shape(0), [&](std::span<double> out) {
                  static_cast<void>(self.get_values(entries.first, entries.second, out));
                }));
          },
          "node_1"_a, "node_2"_a, nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
          coupling_get_values_doc)
      .def(
          "get_values",
          [](Class &self, const InArray<NodeNum> &node_1, const InArray<NodeNum> &node_2) {
            std::vector<double> values(node_1.shape(0));
            static_cast<void>(self.get_values(span_of(node_1), span_of(node_2), values));
            return to_numpy(std::move(values));
          },
          "node_1"_a, "node_2"_a, nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
          coupling_get_values_doc)
      .def(
          "set_values",
          [](Class &self, const InArray<std::int64_t> &node_1, const InArray<std::int64_t> &node_2,
             const DoubleArray &values) {
            if (node_1.shape(0) != node_2.shape(0) || node_1.shape(0) != values.shape(0)) {
              return length_mismatch("set_values", node_1.shape(0));
            }
            const NarrowedEntries entries = narrow_entries(span_of(node_1), span_of(node_2));
            return write_narrowed("set_values", entries, span_of(values),
                                  [&](std::span<const double> kept) {
                                    return self.set_values(entries.first, entries.second, kept);
                                  });
          },
          "node_1"_a, "node_2"_a, "values"_a,
          nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(), coupling_set_values_doc)
      .def(
          "set_values",
          [](Class &self, const InArray<NodeNum> &node_1, const InArray<NodeNum> &node_2,
             const DoubleArray &values) {
            return self.set_values(span_of(node_1), span_of(node_2), span_of(values));
          },
          "node_1"_a, "node_2"_a, "values"_a,
          nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(), coupling_set_values_doc)
      .def(
          "to_arrays",
          [](Class &self) {
            auto arrays = self.to_arrays();
            return std::make_tuple(to_numpy(std::move(arrays.node_1)),
                                   to_numpy(std::move(arrays.node_2)),
                                   to_numpy(std::move(arrays.values)));
          },
          nb::rv_policy::move,
          "Every stored coupling as (node_1, node_2, values) arrays, in internal "
          "order.");
}

constexpr const char *set_types_doc =
    "Change the type of many nodes at once (DIFFUSIVE or BOUNDARY), from a "
    "list or an array of node numbers.\n\n"
    "Each node keeps its attributes and couplings; the nodes and the coupling "
    "matrices are reordered once for all of them. Unknown node numbers are "
    "rejected and reported; a node that already has the type is accepted "
    "unchanged. A solver initialised before the call must be initialised "
    "again.";

constexpr const char *node_set_values_doc =
    "Set one attribute of many nodes. Unknown nodes (including numbers "
    "outside the int32 range) are skipped and reported; for a node given "
    "twice the last value wins.";

constexpr const char *node_get_values_doc =
    "One attribute of the given nodes (NaN for an unknown one, including a "
    "number outside the int32 range), or of every node in internal order "
    "when node_nums is None.";

// The bulk node getters and setters of Nodes.
template <typename Class> void bind_bulk_node_values(nb::class_<Class> &cls) {
  cls.def("reserve", &Class::reserve, "num_nodes"_a,
          "Reserve storage for num_nodes nodes in total.")
      .def_prop_ro("structure_version", &Class::structure_version,
                   "Changes whenever a node is added, removed or moved.")
      .def(
          "node_numbers", [](const Class &self) { return to_numpy(self.node_numbers()); },
          "User numbers of every node, in internal order.")
      .def(
          "set_values",
          [](Class &self, NodeAttribute attribute, const InArray<std::int64_t> &node_nums,
             const DoubleArray &values) {
            if (node_nums.shape(0) != values.shape(0)) {
              return length_mismatch("set_values", node_nums.shape(0));
            }
            const NarrowedEntries entries = narrow_entries(span_of(node_nums));
            return write_narrowed("set_values", entries, span_of(values),
                                  [&](std::span<const double> kept) {
                                    return self.set_values(attribute, entries.first, kept);
                                  });
          },
          "attribute"_a, "node_nums"_a, "values"_a,
          nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(), node_set_values_doc)
      .def(
          "set_values",
          [](Class &self, NodeAttribute attribute, const InArray<NodeNum> &node_nums,
             const DoubleArray &values) {
            return self.set_values(attribute, span_of(node_nums), span_of(values));
          },
          "attribute"_a, "node_nums"_a, "values"_a,
          nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(), node_set_values_doc)
      .def(
          "set_types",
          [](Class &self, const InArray<std::int64_t> &node_nums, NodeType node_type) {
            BulkReport out_of_range;
            const auto narrowed = narrow_node_numbers(span_of(node_nums), out_of_range);
            if (!narrowed.has_value()) {
              return out_of_range;
            }
            return self.set_types(*narrowed, static_cast<char>(node_type));
          },
          "node_nums"_a, "node_type"_a, nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
          set_types_doc)
      .def(
          "set_types",
          [](Class &self, const InArray<NodeNum> &node_nums, NodeType node_type) {
            return self.set_types(span_of(node_nums), static_cast<char>(node_type));
          },
          "node_nums"_a, "node_type"_a, nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
          set_types_doc)
      .def(
          "set_types",
          [](Class &self, const std::vector<NodeNum> &node_nums, NodeType node_type) {
            return self.set_types(node_nums, static_cast<char>(node_type));
          },
          "node_nums"_a, "node_type"_a, nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
          set_types_doc)
      .def(
          "get_values",
          [](const Class &self, NodeAttribute attribute, const InArray<std::int64_t> &node_nums) {
            const NarrowedEntries entries = narrow_entries(span_of(node_nums));
            return to_numpy(read_narrowed(
                "get_values", entries, node_nums.shape(0), [&](std::span<double> out) {
                  static_cast<void>(self.get_values(attribute, entries.first, out));
                }));
          },
          "attribute"_a, "node_nums"_a, nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(),
          node_get_values_doc)
      .def(
          "get_values",
          [](const Class &self, NodeAttribute attribute,
             const std::optional<InArray<NodeNum>> &node_nums) {
            if (!node_nums.has_value()) {
              return to_numpy(self.get_values(attribute));
            }
            std::vector<double> values(node_nums->shape(0));
            static_cast<void>(self.get_values(attribute, span_of(*node_nums), values));
            return to_numpy(std::move(values));
          },
          "attribute"_a, "node_nums"_a = nb::none(),
          nb::call_guard<pycanha::bindings::utils::LogDrainGuard>(), node_get_values_doc);
}

} // namespace pycanha::bindings::tmm
