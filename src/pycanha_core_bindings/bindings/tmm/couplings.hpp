#pragma once

#include <cstdint>
#include <memory>

#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "pycanha-core/tmm/conductivecouplings.hpp"
#include "pycanha-core/tmm/coupling.hpp"
#include "pycanha-core/tmm/couplingmatrices.hpp"
#include "pycanha-core/tmm/couplings.hpp"
#include "pycanha-core/tmm/radiativecouplings.hpp"

namespace py = pybind11;
using namespace pybind11::literals;  // NOLINT(build/namespaces)

using pycanha::ConductiveCouplings;
using pycanha::Coupling;
using pycanha::CouplingMatrices;
using pycanha::Couplings;
using pycanha::Index;
using pycanha::IntAddress;
using pycanha::RadiativeCouplings;
using pycanha::Nodes;

namespace pycanha::bindings::tmm {

inline void Coupling_b(py::module_ &m) {
  py::class_<Coupling>(m, "Coupling")
      .def(py::init<Index, Index, double>(), "node_1"_a, "node_2"_a,
           "value"_a)
      .def_property("node_1", &Coupling::get_node_1, &Coupling::set_node_1)
      .def_property("node_2", &Coupling::get_node_2, &Coupling::set_node_2)
      .def_property("value", &Coupling::get_value, &Coupling::set_value);
}

inline void CouplingMatrices_b(py::module_ &m) {
  py::class_<CouplingMatrices, std::shared_ptr<CouplingMatrices>>(
      m, "CouplingMatrices")
      .def(py::init<>())
               .def_property_readonly("num_diff_nodes",
                                                                       [](CouplingMatrices &self) {
                                                                            return static_cast<Index>(
                                                                                      self.sparse_dd.rows());
                                                                       })
               .def_property_readonly("num_bound_nodes",
                                                                       [](CouplingMatrices &self) {
                                                                            return static_cast<Index>(
                                                                                      self.sparse_db.cols());
                                                                       })
               .def_property_readonly("num_nodes",
                                                                       [](CouplingMatrices &self) {
                                                                            return static_cast<Index>(
                                                                                      self.sparse_db.rows() +
                                                                                      self.sparse_db.cols());
                                                                       })
      .def("add_ovw_coupling_from_node_idxs",
           &CouplingMatrices::add_ovw_coupling_from_node_idxs, "idx1"_a,
           "idx2"_a, "value"_a)
      .def("add_ovw_coupling_from_node_idxs_verbose",
           &CouplingMatrices::add_ovw_coupling_from_node_idxs_verbose,
           "idx1"_a, "idx2"_a, "value"_a)
      .def("add_sum_coupling_from_node_idxs",
           &CouplingMatrices::add_sum_coupling_from_node_idxs, "idx1"_a,
           "idx2"_a, "value"_a)
      .def("add_sum_coupling_from_node_idxs_verbose",
           &CouplingMatrices::add_sum_coupling_from_node_idxs_verbose,
           "idx1"_a, "idx2"_a, "value"_a)
      .def("add_new_coupling_from_node_idxs",
           &CouplingMatrices::add_new_coupling_from_node_idxs, "idx1"_a,
           "idx2"_a, "value"_a)
      .def("get_conductor_value_from_idx",
           &CouplingMatrices::get_conductor_value_from_idx, "idx1"_a,
           "idx2"_a)
      .def("set_conductor_value_from_idx",
           &CouplingMatrices::set_conductor_value_from_idx, "idx1"_a,
           "idx2"_a, "value"_a)
      .def("get_conductor_value_pointer_from_idx",
           [](CouplingMatrices &self, Index idx1, Index idx2) {
             return reinterpret_cast<std::uintptr_t>(
                 self.get_conductor_value_ref_from_idx(idx1, idx2));
           },
           "idx1"_a, "idx2"_a)
      .def("get_conductor_value_address_from_idx",
           &CouplingMatrices::get_conductor_value_address_from_idx, "idx1"_a,
           "idx2"_a)
      .def("sparse_dd_copy", &CouplingMatrices::sparse_dd_copy)
      .def("sparse_db_copy", &CouplingMatrices::sparse_db_copy)
      .def("sparse_bb_copy", &CouplingMatrices::sparse_bb_copy)
      .def_property_readonly("num_diff_diff_couplings",
                             &CouplingMatrices::get_num_diff_diff_couplings)
      .def_property_readonly("num_diff_bound_couplings",
                             &CouplingMatrices::get_num_diff_bound_couplings)
      .def_property_readonly("num_bound_bound_couplings",
                             &CouplingMatrices::get_num_bound_bound_couplings)
      .def_property_readonly("num_total_couplings",
                             &CouplingMatrices::get_num_total_couplings)
      .def("get_idxs_and_coupling_value_from_coupling_idx",
           &CouplingMatrices::get_idxs_and_coupling_value_from_coupling_idx,
           "coupling_idx"_a)
      .def("coupling_exists_from_idxs",
           &CouplingMatrices::coupling_exists_from_idxs, "idx1"_a,
           "idx2"_a)
      .def("print_sparse", &CouplingMatrices::print_sparse)
      .def("reserve", &CouplingMatrices::reserve, "nnz"_a);
}

inline void Couplings_b(py::module_ &m) {
  py::class_<Couplings, std::shared_ptr<Couplings>>(m, "Couplings")
      .def(py::init<std::shared_ptr<Nodes>>(), "nodes"_a)
      .def(
          "get_coupling_matrices",
          [](Couplings &self) -> CouplingMatrices & {
            return self.get_coupling_matrices();
          },
          py::return_value_policy::reference_internal)
      .def(
          "get_coupling_value", &Couplings::get_coupling_value, "node_num_1"_a,
          "node_num_2"_a)
      .def("set_coupling_value", &Couplings::set_coupling_value,
           "node_num_1"_a, "node_num_2"_a, "value"_a)
      .def("add_ovw_coupling",
           py::overload_cast<Index, Index, double>(&Couplings::add_ovw_coupling),
           "node_num_1"_a, "node_num_2"_a, "value"_a)
      .def("add_ovw_coupling",
           py::overload_cast<const Coupling &>(&Couplings::add_ovw_coupling),
           "coupling"_a)
      .def("add_ovw_coupling_verbose",
           py::overload_cast<Index, Index, double>(
               &Couplings::add_ovw_coupling_verbose),
           "node_num_1"_a, "node_num_2"_a, "value"_a)
      .def("add_ovw_coupling_verbose",
           py::overload_cast<const Coupling &>(
               &Couplings::add_ovw_coupling_verbose),
           "coupling"_a)
      .def("add_sum_coupling",
           py::overload_cast<Index, Index, double>(&Couplings::add_sum_coupling),
           "node_num_1"_a, "node_num_2"_a, "value"_a)
      .def("add_sum_coupling",
           py::overload_cast<const Coupling &>(&Couplings::add_sum_coupling),
           "coupling"_a)
      .def("add_sum_coupling_verbose",
           py::overload_cast<Index, Index, double>(
               &Couplings::add_sum_coupling_verbose),
           "node_num_1"_a, "node_num_2"_a, "value"_a)
      .def("add_sum_coupling_verbose",
           py::overload_cast<const Coupling &>(
               &Couplings::add_sum_coupling_verbose),
           "coupling"_a)
      .def("add_new_coupling",
           py::overload_cast<Index, Index, double>(&Couplings::add_new_coupling),
           "node_num_1"_a, "node_num_2"_a, "value"_a)
      .def("add_new_coupling",
           py::overload_cast<const Coupling &>(&Couplings::add_new_coupling),
           "coupling"_a)
      .def("add_coupling",
           py::overload_cast<Index, Index, double>(&Couplings::add_coupling),
           "node_num_1"_a, "node_num_2"_a, "value"_a)
      .def("add_coupling",
           py::overload_cast<const Coupling &>(&Couplings::add_coupling),
           "coupling"_a)
      .def("get_coupling_value_pointer",
           [](Couplings &self, Index node_num_1, Index node_num_2) {
             return reinterpret_cast<std::uintptr_t>(
                 self.get_coupling_value_ref(node_num_1, node_num_2));
           },
           "node_num_1"_a, "node_num_2"_a)
      .def("get_coupling_value_address",
           &Couplings::get_coupling_value_address, "node_num_1"_a,
           "node_num_2"_a)
      .def("coupling_exists", &Couplings::coupling_exists, "node_num_1"_a,
           "node_num_2"_a)
      .def("get_coupling_from_coupling_idx",
           &Couplings::get_coupling_from_coupling_idx, "idx"_a);
}

inline void ConductiveCouplings_b(py::module_ &m) {
  py::class_<ConductiveCouplings, std::shared_ptr<ConductiveCouplings>>(
      m, "ConductiveCouplings")
      .def(py::init<std::shared_ptr<Nodes>>(), "nodes"_a)
      .def("add_coupling",
           py::overload_cast<Index, Index, double>(&ConductiveCouplings::add_coupling),
           "node_num_1"_a, "node_num_2"_a, "value"_a)
      .def("add_coupling",
           py::overload_cast<const Coupling &>(&ConductiveCouplings::add_coupling),
           "coupling"_a)
      .def("set_coupling_value", &ConductiveCouplings::set_coupling_value,
           "node_num_1"_a, "node_num_2"_a, "value"_a)
      .def("get_coupling_value", &ConductiveCouplings::get_coupling_value,
           "node_num_1"_a, "node_num_2"_a);
}

inline void RadiativeCouplings_b(py::module_ &m) {
  py::class_<RadiativeCouplings, std::shared_ptr<RadiativeCouplings>>(
      m, "RadiativeCouplings")
      .def(py::init<std::shared_ptr<Nodes>>(), "nodes"_a)
      .def("add_coupling",
           py::overload_cast<Index, Index, double>(&RadiativeCouplings::add_coupling),
           "node_num_1"_a, "node_num_2"_a, "value"_a)
      .def("add_coupling",
           py::overload_cast<const Coupling &>(&RadiativeCouplings::add_coupling),
           "coupling"_a)
      .def("set_coupling_value", &RadiativeCouplings::set_coupling_value,
           "node_num_1"_a, "node_num_2"_a, "value"_a)
      .def("get_coupling_value", &RadiativeCouplings::get_coupling_value,
           "node_num_1"_a, "node_num_2"_a);
}

inline void register_couplings(py::module_ &m) {
  Coupling_b(m);
  CouplingMatrices_b(m);
  Couplings_b(m);
  ConductiveCouplings_b(m);
  RadiativeCouplings_b(m);
}

}  // namespace pycanha::bindings::tmm
