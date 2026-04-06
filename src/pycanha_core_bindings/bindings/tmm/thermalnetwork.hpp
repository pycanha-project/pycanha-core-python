#pragma once

#include <memory>
#include <vector>

#include <nanobind/nanobind.h>
#include <nanobind/stl/vector.h>

#include "pycanha-core/tmm/node.hpp"
#include "pycanha-core/tmm/thermalnetwork.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

using pycanha::ConductiveCouplings;
using pycanha::Node;
using pycanha::Nodes;
using pycanha::RadiativeCouplings;
using pycanha::ThermalNetwork;

namespace pycanha::bindings::tmm {

inline void ThermalNetwork_b(nb::module_ &m) {
  using PairFlowMethod =
      double (ThermalNetwork::*)(pycanha::Index, pycanha::Index);
  using GroupFlowMethod = double (ThermalNetwork::*)(
      const std::vector<pycanha::Index> &, const std::vector<pycanha::Index> &);

  nb::class_<ThermalNetwork>(m, "ThermalNetwork",
                             "Thermal network combining nodes, conductive\n"
                             "couplings, and radiative couplings.")
      .def(nb::init<>(), "Create an empty thermal network.")
      .def(
          nb::init<std::shared_ptr<Nodes>, std::shared_ptr<ConductiveCouplings>,
                   std::shared_ptr<RadiativeCouplings>>(),
          "nodes"_a, "conductive"_a, "radiative"_a,
          "Create a thermal network from existing components.")
      .def("add_node", &ThermalNetwork::add_node, "node"_a,
           "Add a node to the network.")
      .def("remove_node", &ThermalNetwork::remove_node, "node_num"_a,
           "Remove a node from the network by user node number.")
      .def_prop_ro(
          "nodes", [](ThermalNetwork &self) -> Nodes & { return self.nodes(); },
          nb::rv_policy::reference_internal,
          "Reference to the Nodes container.")
      .def_prop_ro(
          "conductive_couplings",
          [](ThermalNetwork &self) -> ConductiveCouplings & {
            return self.conductive_couplings();
          },
          nb::rv_policy::reference_internal,
          "Reference to the ConductiveCouplings container.")
      .def_prop_ro(
          "radiative_couplings",
          [](ThermalNetwork &self) -> RadiativeCouplings & {
            return self.radiative_couplings();
          },
          nb::rv_policy::reference_internal,
          "Reference to the RadiativeCouplings container.")
      .def("flow_conductive",
           static_cast<PairFlowMethod>(&ThermalNetwork::flow_conductive),
           "node_num_1"_a, "node_num_2"_a,
           "Compute conductive heat flow [W] between two nodes.")
      .def("flow_conductive",
           static_cast<GroupFlowMethod>(&ThermalNetwork::flow_conductive),
           "node_nums_1"_a, "node_nums_2"_a,
           "Compute total conductive heat flow [W] between two groups of nodes.")
      .def("flow_radiative",
           static_cast<PairFlowMethod>(&ThermalNetwork::flow_radiative),
           "node_num_1"_a, "node_num_2"_a,
           "Compute radiative heat flow [W] between two nodes.")
      .def("flow_radiative",
           static_cast<GroupFlowMethod>(&ThermalNetwork::flow_radiative),
           "node_nums_1"_a, "node_nums_2"_a,
           "Compute total radiative heat flow [W] between two groups of nodes.")
      .def_prop_ro(
          "nodes_ptr",
          static_cast<std::shared_ptr<Nodes> (ThermalNetwork::*)() noexcept>(
              &ThermalNetwork::nodes_ptr),
          "Shared pointer to the Nodes container.");
}

inline void register_thermalnetwork(nb::module_ &m) { ThermalNetwork_b(m); }

} // namespace pycanha::bindings::tmm
