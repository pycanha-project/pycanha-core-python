#pragma once

#include <memory>

#include <nanobind/nanobind.h>

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
  nb::class_<ThermalNetwork>(m, "ThermalNetwork",
                             "Thermal network (nodes + couplings).")
      .def(nb::init<>())
      .def(
          nb::init<std::shared_ptr<Nodes>, std::shared_ptr<ConductiveCouplings>,
                   std::shared_ptr<RadiativeCouplings>>(),
          "nodes"_a, "conductive"_a, "radiative"_a)
      .def("add_node", &ThermalNetwork::add_node, "node"_a)
      .def("remove_node", &ThermalNetwork::remove_node, "node_num"_a)
      .def_prop_ro(
          "nodes", [](ThermalNetwork &self) -> Nodes & { return self.nodes(); },
          nb::rv_policy::reference_internal)
      .def_prop_ro(
          "conductive_couplings",
          [](ThermalNetwork &self) -> ConductiveCouplings & {
            return self.conductive_couplings();
          },
          nb::rv_policy::reference_internal)
      .def_prop_ro(
          "radiative_couplings",
          [](ThermalNetwork &self) -> RadiativeCouplings & {
            return self.radiative_couplings();
          },
          nb::rv_policy::reference_internal)
      .def_prop_ro(
          "nodes_ptr",
          static_cast<std::shared_ptr<Nodes> (ThermalNetwork::*)() noexcept>(
              &ThermalNetwork::nodes_ptr));
}

inline void register_thermalnetwork(nb::module_ &m) { ThermalNetwork_b(m); }

} // namespace pycanha::bindings::tmm
