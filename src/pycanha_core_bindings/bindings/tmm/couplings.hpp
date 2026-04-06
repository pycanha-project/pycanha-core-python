#pragma once

#include <cstdint>
#include <memory>

#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "pycanha-core/tmm/conductivecouplings.hpp"
#include "pycanha-core/tmm/coupling.hpp"
#include "pycanha-core/tmm/couplingmatrices.hpp"
#include "pycanha-core/tmm/couplings.hpp"
#include "pycanha-core/tmm/radiativecouplings.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

using pycanha::ConductiveCouplings;
using pycanha::Coupling;
using pycanha::CouplingMatrices;
using pycanha::Couplings;
using pycanha::Index;
using pycanha::IntAddress;
using pycanha::Nodes;
using pycanha::RadiativeCouplings;

namespace pycanha::bindings::tmm {

inline void Coupling_b(nb::module_ &m) {
  nb::class_<Coupling>(m, "Coupling",
                      "Thermal coupling (conductance) between two nodes.")
      .def(nb::init<Index, Index, double>(), "node_1"_a, "node_2"_a, "value"_a,
           "Create a coupling between two nodes with a conductance value.")
      .def_prop_rw("node_1", &Coupling::get_node_1, &Coupling::set_node_1,
                   "First node number.")
      .def_prop_rw("node_2", &Coupling::get_node_2, &Coupling::set_node_2,
                   "Second node number.")
      .def_prop_rw("value", &Coupling::get_value, &Coupling::set_value,
                   "Conductance value [W/K] or radiative exchange factor [m^2].");
}

inline void CouplingMatrices_b(nb::module_ &m) {
  nb::class_<CouplingMatrices>(m, "CouplingMatrices",
                               "Sparse coupling matrices (dd, db, bb blocks).\n\n"
                               "Stores conductance values in three sparse matrices:\n"
                               "diffusive-diffusive, diffusive-boundary, and\n"
                               "boundary-boundary. Indexed by internal node indices.")
      .def(nb::init<>(), "Create empty coupling matrices.")
      .def_prop_ro("num_diff_nodes",
                   [](CouplingMatrices &self) {
                     return static_cast<Index>(self.sparse_dd.rows());
                   },
                   "Number of diffusive nodes in the matrices.")
      .def_prop_ro("num_bound_nodes",
                   [](CouplingMatrices &self) {
                     return static_cast<Index>(self.sparse_db.cols());
                   },
                   "Number of boundary nodes in the matrices.")
      .def_prop_ro("num_nodes",
                   [](CouplingMatrices &self) {
                     return static_cast<Index>(self.sparse_db.rows() +
                                               self.sparse_db.cols());
                   },
                   "Total number of nodes in the matrices.")
      .def("add_ovw_coupling_from_node_idxs",
           &CouplingMatrices::add_ovw_coupling_from_node_idxs, "idx1"_a,
           "idx2"_a, "value"_a,
           "Add or overwrite a coupling value by internal indices.")
      .def("add_ovw_coupling_from_node_idxs_verbose",
           &CouplingMatrices::add_ovw_coupling_from_node_idxs_verbose, "idx1"_a,
           "idx2"_a, "value"_a,
           "Add or overwrite a coupling value by internal indices (verbose).")
      .def("add_sum_coupling_from_node_idxs",
           &CouplingMatrices::add_sum_coupling_from_node_idxs, "idx1"_a,
           "idx2"_a, "value"_a,
           "Add a coupling value, summing with any existing value.")
      .def("add_sum_coupling_from_node_idxs_verbose",
           &CouplingMatrices::add_sum_coupling_from_node_idxs_verbose, "idx1"_a,
           "idx2"_a, "value"_a,
           "Add a coupling value, summing with any existing value (verbose).")
      .def("add_new_coupling_from_node_idxs",
           &CouplingMatrices::add_new_coupling_from_node_idxs, "idx1"_a,
           "idx2"_a, "value"_a,
           "Add a coupling only if it does not already exist.")
      .def("get_conductor_value_from_idx",
           &CouplingMatrices::get_conductor_value_from_idx, "idx1"_a, "idx2"_a,
           "Get conductance value by internal indices.")
      .def("set_conductor_value_from_idx",
           &CouplingMatrices::set_conductor_value_from_idx, "idx1"_a, "idx2"_a,
           "value"_a, "Set conductance value by internal indices.")
      .def(
          "get_conductor_value_pointer_from_idx",
          [](CouplingMatrices &self, Index idx1, Index idx2) {
            return reinterpret_cast<std::uintptr_t>(
                self.get_conductor_value_ref_from_idx(idx1, idx2));
          },
          "idx1"_a, "idx2"_a,
          "Memory address of the conductance value for formula binding.")
      .def("get_conductor_value_address_from_idx",
           &CouplingMatrices::get_conductor_value_address_from_idx, "idx1"_a,
           "idx2"_a,
           "Memory address (as int) of the conductance value.")
      .def("sparse_dd_copy", &CouplingMatrices::sparse_dd_copy,
           "Return a copy of the diffusive-diffusive coupling matrix.")
      .def("sparse_db_copy", &CouplingMatrices::sparse_db_copy,
           "Return a copy of the diffusive-boundary coupling matrix.")
      .def("sparse_bb_copy", &CouplingMatrices::sparse_bb_copy,
           "Return a copy of the boundary-boundary coupling matrix.")
      .def_prop_ro("num_diff_diff_couplings",
                   &CouplingMatrices::get_num_diff_diff_couplings,
                   "Number of non-zero diffusive-diffusive couplings.")
      .def_prop_ro("num_diff_bound_couplings",
                   &CouplingMatrices::get_num_diff_bound_couplings,
                   "Number of non-zero diffusive-boundary couplings.")
      .def_prop_ro("num_bound_bound_couplings",
                   &CouplingMatrices::get_num_bound_bound_couplings,
                   "Number of non-zero boundary-boundary couplings.")
      .def_prop_ro("num_total_couplings",
                   &CouplingMatrices::get_num_total_couplings,
                   "Total number of non-zero couplings.")
      .def("get_idxs_and_coupling_value_from_coupling_idx",
           &CouplingMatrices::get_idxs_and_coupling_value_from_coupling_idx,
           "coupling_idx"_a,
           "Get (idx1, idx2, value) tuple from a flat coupling index.")
      .def("coupling_exists_from_idxs",
           &CouplingMatrices::coupling_exists_from_idxs, "idx1"_a, "idx2"_a,
           "Check whether a coupling exists between two internal indices.")
      .def("print_sparse", &CouplingMatrices::print_sparse,
           "Print the sparse matrices to the logger (debug).")
      .def("reserve", &CouplingMatrices::reserve, "nnz"_a,
           "Pre-allocate space for the given number of non-zeros.");
}

inline void Couplings_b(nb::module_ &m) {
  nb::class_<Couplings>(m, "Couplings",
                       "Generic coupling manager using user node numbers.\n\n"
                       "Wraps CouplingMatrices and translates between user node\n"
                       "numbers and internal indices. Supports multiple add\n"
                       "strategies: overwrite, sum, new-only.")
      .def(nb::init<std::shared_ptr<Nodes>>(), "nodes"_a,
           "Create a Couplings manager linked to a Nodes container.")
      .def(
          "get_coupling_matrices",
          [](Couplings &self) -> CouplingMatrices & {
            return self.get_coupling_matrices();
          },
          nb::rv_policy::reference_internal,
          "Return a reference to the underlying CouplingMatrices.")
      .def("get_coupling_value", &Couplings::get_coupling_value, "node_num_1"_a,
           "node_num_2"_a,
           "Get the conductance value between two nodes.")
      .def("set_coupling_value", &Couplings::set_coupling_value, "node_num_1"_a,
           "node_num_2"_a, "value"_a,
           "Set the conductance value between two nodes.")
      .def(
          "add_ovw_coupling",
          nb::overload_cast<Index, Index, double>(&Couplings::add_ovw_coupling),
          "node_num_1"_a, "node_num_2"_a, "value"_a,
          "Add or overwrite a coupling between two nodes.")
      .def("add_ovw_coupling",
           nb::overload_cast<const Coupling &>(&Couplings::add_ovw_coupling),
           "coupling"_a,
           "Add or overwrite a coupling from a Coupling object.")
      .def("add_ovw_coupling_verbose",
           nb::overload_cast<Index, Index, double>(
               &Couplings::add_ovw_coupling_verbose),
           "node_num_1"_a, "node_num_2"_a, "value"_a,
           "Add or overwrite a coupling (verbose logging).")
      .def("add_ovw_coupling_verbose",
           nb::overload_cast<const Coupling &>(
               &Couplings::add_ovw_coupling_verbose),
           "coupling"_a,
           "Add or overwrite a coupling from a Coupling object (verbose).")
      .def(
          "add_sum_coupling",
          nb::overload_cast<Index, Index, double>(&Couplings::add_sum_coupling),
          "node_num_1"_a, "node_num_2"_a, "value"_a,
          "Add a coupling, summing with any existing value.")
      .def("add_sum_coupling",
           nb::overload_cast<const Coupling &>(&Couplings::add_sum_coupling),
           "coupling"_a,
           "Add a coupling from a Coupling object, summing with existing.")
      .def("add_sum_coupling_verbose",
           nb::overload_cast<Index, Index, double>(
               &Couplings::add_sum_coupling_verbose),
           "node_num_1"_a, "node_num_2"_a, "value"_a,
           "Add a coupling, summing with existing (verbose logging).")
      .def("add_sum_coupling_verbose",
           nb::overload_cast<const Coupling &>(
               &Couplings::add_sum_coupling_verbose),
           "coupling"_a,
           "Add a coupling from a Coupling object, summing (verbose).")
      .def(
          "add_new_coupling",
          nb::overload_cast<Index, Index, double>(&Couplings::add_new_coupling),
          "node_num_1"_a, "node_num_2"_a, "value"_a,
          "Add a coupling only if it does not already exist.")
      .def("add_new_coupling",
           nb::overload_cast<const Coupling &>(&Couplings::add_new_coupling),
           "coupling"_a,
           "Add a coupling from a Coupling object only if new.")
      .def("add_coupling",
           nb::overload_cast<Index, Index, double>(&Couplings::add_coupling),
           "node_num_1"_a, "node_num_2"_a, "value"_a,
           "Add a coupling between two nodes.")
      .def("add_coupling",
           nb::overload_cast<const Coupling &>(&Couplings::add_coupling),
           "coupling"_a,
           "Add a coupling from a Coupling object.")
      .def(
          "get_coupling_value_pointer",
          [](Couplings &self, Index node_num_1, Index node_num_2) {
            return reinterpret_cast<std::uintptr_t>(
                self.get_coupling_value_ref(node_num_1, node_num_2));
          },
          "node_num_1"_a, "node_num_2"_a,
          "Memory address of the coupling value for formula binding.")
      .def("get_coupling_value_address", &Couplings::get_coupling_value_address,
           "node_num_1"_a, "node_num_2"_a,
           "Memory address (as int) of the coupling value.")
      .def("coupling_exists", &Couplings::coupling_exists, "node_num_1"_a,
           "node_num_2"_a,
           "Check whether a coupling exists between two nodes.")
      .def("get_coupling_from_coupling_idx",
           &Couplings::get_coupling_from_coupling_idx, "idx"_a,
           "Get a Coupling object from a flat coupling index.");
}

inline void ConductiveCouplings_b(nb::module_ &m) {
  nb::class_<ConductiveCouplings>(m, "ConductiveCouplings",
                                  "Container for conductive (linear) couplings GL.\n\n"
                                  "Heat flow: Q = GL * (T1 - T2).")
      .def(nb::init<std::shared_ptr<Nodes>>(), "nodes"_a,
           "Create conductive couplings linked to a Nodes container.")
      .def("add_coupling",
           nb::overload_cast<Index, Index, double>(
               &ConductiveCouplings::add_coupling),
           "node_num_1"_a, "node_num_2"_a, "value"_a,
           "Add a conductive coupling [W/K] between two nodes.")
      .def("add_coupling",
           nb::overload_cast<const Coupling &>(
               &ConductiveCouplings::add_coupling),
           "coupling"_a,
           "Add a conductive coupling from a Coupling object.")
      .def("set_coupling_value", &ConductiveCouplings::set_coupling_value,
           "node_num_1"_a, "node_num_2"_a, "value"_a,
           "Set the conductive coupling value [W/K] between two nodes.")
      .def("get_coupling_value", &ConductiveCouplings::get_coupling_value,
           "node_num_1"_a, "node_num_2"_a,
           "Get the conductive coupling value [W/K] between two nodes.");
}

inline void RadiativeCouplings_b(nb::module_ &m) {
  nb::class_<RadiativeCouplings>(m, "RadiativeCouplings",
                                 "Container for radiative (T^4) couplings GR.\n\n"
                                 "Heat flow: Q = GR * sigma * (T1^4 - T2^4).")
      .def(nb::init<std::shared_ptr<Nodes>>(), "nodes"_a,
           "Create radiative couplings linked to a Nodes container.")
      .def("add_coupling",
           nb::overload_cast<Index, Index, double>(
               &RadiativeCouplings::add_coupling),
           "node_num_1"_a, "node_num_2"_a, "value"_a,
           "Add a radiative coupling [m^2] between two nodes.")
      .def("add_coupling",
           nb::overload_cast<const Coupling &>(
               &RadiativeCouplings::add_coupling),
           "coupling"_a,
           "Add a radiative coupling from a Coupling object.")
      .def("set_coupling_value", &RadiativeCouplings::set_coupling_value,
           "node_num_1"_a, "node_num_2"_a, "value"_a,
           "Set the radiative coupling value [m^2] between two nodes.")
      .def("get_coupling_value", &RadiativeCouplings::get_coupling_value,
           "node_num_1"_a, "node_num_2"_a,
           "Get the radiative coupling value [m^2] between two nodes.");
}

inline void register_couplings(nb::module_ &m) {
  Coupling_b(m);
  CouplingMatrices_b(m);
  Couplings_b(m);
  ConductiveCouplings_b(m);
  RadiativeCouplings_b(m);
}

} // namespace pycanha::bindings::tmm
