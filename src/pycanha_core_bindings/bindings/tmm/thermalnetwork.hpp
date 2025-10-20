#pragma once

#include <memory>

#include <pybind11/pybind11.h>

#include "pycanha-core/tmm/node.hpp"
#include "pycanha-core/tmm/thermalnetwork.hpp"

namespace py = pybind11;
using namespace pybind11::literals;  // NOLINT(build/namespaces)

using pycanha::ConductiveCouplings;
using pycanha::Node;
using pycanha::Nodes;
using pycanha::RadiativeCouplings;
using pycanha::ThermalNetwork;

namespace pycanha::bindings::tmm {

inline void ThermalNetwork_b(py::module_ &m) {
  py::class_<ThermalNetwork, std::shared_ptr<ThermalNetwork>>(m,
                                                             "ThermalNetwork")
      .def(py::init<>())
      .def(py::init<std::shared_ptr<Nodes>, std::shared_ptr<ConductiveCouplings>,
                    std::shared_ptr<RadiativeCouplings>>(),
           "nodes"_a, "conductive"_a, "radiative"_a)
      .def("add_node", &ThermalNetwork::add_node, "node"_a)
      .def("remove_node", &ThermalNetwork::remove_node, "node_num"_a)
      .def_property_readonly(
          "nodes",
          [](ThermalNetwork &self) -> Nodes & { return self.nodes(); },
          py::return_value_policy::reference_internal)
      .def_property_readonly(
          "conductive_couplings",
          [](ThermalNetwork &self) -> ConductiveCouplings & {
            return self.conductive_couplings();
          },
          py::return_value_policy::reference_internal)
      .def_property_readonly(
          "radiative_couplings",
          [](ThermalNetwork &self) -> RadiativeCouplings & {
            return self.radiative_couplings();
          },
          py::return_value_policy::reference_internal)
      .def_property_readonly(
          "nodes_ptr",
          static_cast<std::shared_ptr<Nodes> (ThermalNetwork::*)() noexcept>(
              &ThermalNetwork::nodes_ptr));
}

inline void register_thermalnetwork(py::module_ &m) { ThermalNetwork_b(m); }

}  // namespace pycanha::bindings::tmm
