#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <nanobind/nanobind.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "pycanha-core/tmm/node.hpp"
#include "pycanha-core/tmm/nodes.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

using pycanha::Node;
using pycanha::NodeNum;
using pycanha::Nodes;
using pycanha::NodeType;

namespace pycanha::bindings::tmm {

inline void Node_b(nb::module_ &m) {
  nb::enum_<NodeType>(m, "NodeType",
                      "Thermal node type: DIFFUSIVE or BOUNDARY.")
      .value("DIFFUSIVE", NodeType::DIFFUSIVE_NODE,
             "Diffusive node with thermal capacity.")
      .value("BOUNDARY", NodeType::BOUNDARY_NODE,
             "Boundary node with fixed temperature.")
      .export_values();

  nb::class_<Node>(
      m, "Node",
      "Thermal node representing a lumped thermal element.\n\n"
      "A node stores temperature, thermal capacity, heat loads,\n"
      "optical properties, area, and position. It can be standalone\n"
      "or associated with a Nodes container.")
      .def(nb::init<NodeNum>(), "node_num"_a,
           "Create a standalone node with the given user node number.")
      .def(nb::init<const Node &>(), "other"_a, "Copy constructor.")
      .def_prop_rw("node_num", &Node::get_node_num, &Node::set_node_num,
                   "User-assigned node number.")
      .def_prop_rw(
          "type",
          [](Node &self) { return static_cast<NodeType>(self.get_type()); },
          [](Node &self, NodeType node_type) {
            self.set_type(static_cast<char>(node_type));
          },
          "Node type (DIFFUSIVE or BOUNDARY).")
      .def_prop_rw("T", &Node::get_T, &Node::set_T, "Node temperature [K].")
      .def_prop_rw("C", &Node::get_C, &Node::set_C, "Thermal capacity [J/K].")
      .def_prop_rw("capacity", &Node::get_C, &Node::set_C,
                   "Alias for C (thermal capacity) [J/K].")
      .def_prop_rw("qs", &Node::get_qs, &Node::set_qs, "Solar heat load [W].")
      .def_prop_rw("qa", &Node::get_qa, &Node::set_qa, "Albedo heat load [W].")
      .def_prop_rw("qe", &Node::get_qe, &Node::set_qe,
                   "Earth IR heat load [W].")
      .def_prop_rw("qi", &Node::get_qi, &Node::set_qi,
                   "Internal dissipation heat load [W].")
      .def_prop_rw("qr", &Node::get_qr, &Node::set_qr,
                   "Other (residual) heat load [W].")
      .def_prop_rw("a", &Node::get_a, &Node::set_a, "Node area [m^2].")
      .def_prop_rw("fx", &Node::get_fx, &Node::set_fx, "Node X coordinate [m].")
      .def_prop_rw("fy", &Node::get_fy, &Node::set_fy, "Node Y coordinate [m].")
      .def_prop_rw("fz", &Node::get_fz, &Node::set_fz, "Node Z coordinate [m].")
      .def_prop_rw("eps", &Node::get_eps, &Node::set_eps, "IR emissivity [-].")
      .def_prop_rw("aph", &Node::get_aph, &Node::set_aph,
                   "Solar absorptivity [-].")
      .def_prop_rw("literal_C", &Node::get_literal_C, &Node::set_literal_C,
                   "Literal (formula) representation of thermal capacity.")
      .def(
          "int_node_num",
          [](Node &self) {
            const auto internal_node_num = self.get_int_node_num();
            return internal_node_num.has_value() ? *internal_node_num : -1;
          },
          "Return the internal (solver) node index, or -1 if standalone.")
      .def(
          "parent_pointer",
          [](Node &self) { return self.get_parent_pointer().lock(); },
          nb::rv_policy::reference,
          "Return the parent Nodes container, or None if standalone.")
      .def("parent_pointer_address", &Node::get_int_parent_pointer,
           "Memory address of the parent Nodes container.");
}

inline void Nodes_b(nb::module_ &m) {
  nb::class_<Nodes>(
      m, "Nodes",
      "Collection of thermal nodes.\n\n"
      "Stores nodes efficiently using dense vectors for temperature\n"
      "and capacity, and sparse vectors for heat loads and other\n"
      "attributes. Nodes are auto-sorted: diffusive nodes first,\n"
      "then boundary nodes.")
      .def(nb::init<>(), "Create an empty Nodes container.")
      .def_prop_rw(
          "estimated_number_of_nodes",
          [](Nodes &self) { return self.estimated_number_of_nodes; },
          [](Nodes &self, int value) {
            self.estimated_number_of_nodes = value;
          },
          "Hint for pre-allocating internal storage.")
      .def("add_node", &Nodes::add_node, "node"_a,
           "Add a node to the container.")
      .def("remove_node", &Nodes::remove_node, "node_num"_a,
           "Remove a node by its user node number.")
      .def("is_node", &Nodes::is_node, "node_num"_a,
           "Check whether a node with the given number exists.")
      .def(
          "get_type",
          [](Nodes &self, int node_num) {
            return static_cast<NodeType>(self.get_type(node_num));
          },
          "node_num"_a, "Get the type of a node (DIFFUSIVE or BOUNDARY).")
      .def(
          "set_type",
          [](Nodes &self, int node_num, NodeType node_type) {
            return self.set_type(node_num, static_cast<char>(node_type));
          },
          "node_num"_a, "node_type"_a,
          "Set the type of a node (DIFFUSIVE or BOUNDARY).")
      .def("get_T", &Nodes::get_T, "node_num"_a,
           "Get temperature [K] of a node.")
      .def("set_T", &Nodes::set_T, "node_num"_a, "value"_a,
           "Set temperature [K] of a node.")
      .def("get_C", &Nodes::get_C, "node_num"_a,
           "Get thermal capacity [J/K] of a node.")
      .def("set_C", &Nodes::set_C, "node_num"_a, "value"_a,
           "Set thermal capacity [J/K] of a node.")
      .def("get_qs", &Nodes::get_qs, "node_num"_a,
           "Get solar heat load [W] of a node.")
      .def("set_qs", &Nodes::set_qs, "node_num"_a, "value"_a,
           "Set solar heat load [W] of a node.")
      .def("get_qa", &Nodes::get_qa, "node_num"_a,
           "Get albedo heat load [W] of a node.")
      .def("set_qa", &Nodes::set_qa, "node_num"_a, "value"_a,
           "Set albedo heat load [W] of a node.")
      .def("get_qe", &Nodes::get_qe, "node_num"_a,
           "Get Earth IR heat load [W] of a node.")
      .def("set_qe", &Nodes::set_qe, "node_num"_a, "value"_a,
           "Set Earth IR heat load [W] of a node.")
      .def("get_qi", &Nodes::get_qi, "node_num"_a,
           "Get internal dissipation heat load [W] of a node.")
      .def("set_qi", &Nodes::set_qi, "node_num"_a, "value"_a,
           "Set internal dissipation heat load [W] of a node.")
      .def("get_qr", &Nodes::get_qr, "node_num"_a,
           "Get other (residual) heat load [W] of a node.")
      .def("set_qr", &Nodes::set_qr, "node_num"_a, "value"_a,
           "Set other (residual) heat load [W] of a node.")
      .def("get_a", &Nodes::get_a, "node_num"_a, "Get area [m^2] of a node.")
      .def("set_a", &Nodes::set_a, "node_num"_a, "value"_a,
           "Set area [m^2] of a node.")
      .def("get_fx", &Nodes::get_fx, "node_num"_a,
           "Get X coordinate [m] of a node.")
      .def("set_fx", &Nodes::set_fx, "node_num"_a, "value"_a,
           "Set X coordinate [m] of a node.")
      .def("get_fy", &Nodes::get_fy, "node_num"_a,
           "Get Y coordinate [m] of a node.")
      .def("set_fy", &Nodes::set_fy, "node_num"_a, "value"_a,
           "Set Y coordinate [m] of a node.")
      .def("get_fz", &Nodes::get_fz, "node_num"_a,
           "Get Z coordinate [m] of a node.")
      .def("set_fz", &Nodes::set_fz, "node_num"_a, "value"_a,
           "Set Z coordinate [m] of a node.")
      .def("get_eps", &Nodes::get_eps, "node_num"_a,
           "Get IR emissivity [-] of a node.")
      .def("set_eps", &Nodes::set_eps, "node_num"_a, "value"_a,
           "Set IR emissivity [-] of a node.")
      .def("get_aph", &Nodes::get_aph, "node_num"_a,
           "Get solar absorptivity [-] of a node.")
      .def("set_aph", &Nodes::set_aph, "node_num"_a, "value"_a,
           "Set solar absorptivity [-] of a node.")
      .def("get_literal_C", &Nodes::get_literal_C, "node_num"_a,
           "Get literal (formula) representation of thermal capacity.")
      .def("set_literal_C", &Nodes::set_literal_C, "node_num"_a, "literal"_a,
           "Set literal (formula) representation of thermal capacity.")
      .def("get_idx_from_node_num", &Nodes::get_idx_from_node_num, "node_num"_a,
           "Get internal index from user node number, or None if not found.")
      .def("get_node_num_from_idx", &Nodes::get_node_num_from_idx, "idx"_a,
           "Get user node number from internal index, or None if invalid.")
      .def("get_node_from_node_num", &Nodes::get_node_from_node_num,
           nb::rv_policy::move, "node_num"_a,
           "Get a Node object by user node number.")
      .def("get_node_from_idx", &Nodes::get_node_from_idx, nb::rv_policy::move,
           "idx"_a, "Get a Node object by internal index.")
      .def_prop_ro("num_nodes", &Nodes::get_num_nodes, "Total number of nodes.")
      .def_prop_ro("num_diff_nodes", &Nodes::get_num_diff_nodes,
                   "Number of diffusive nodes.")
      .def_prop_ro("num_bound_nodes", &Nodes::get_num_bound_nodes,
                   "Number of boundary nodes.")
      .def("is_mapped", &Nodes::is_mapped,
           "Check whether the internal node-number map is up to date.")
      .def(
          "get_T_value_pointer",
          [](Nodes &self, int node_num) {
            return reinterpret_cast<std::uintptr_t>(
                self.get_T_value_ref(node_num));
          },
          "node_num"_a,
          "Memory address of the temperature value for formula binding.")
      .def(
          "get_C_value_pointer",
          [](Nodes &self, int node_num) {
            return reinterpret_cast<std::uintptr_t>(
                self.get_C_value_ref(node_num));
          },
          "node_num"_a,
          "Memory address of the thermal capacity value for formula binding.")
      .def(
          "get_qs_value_pointer",
          [](Nodes &self, int node_num) {
            return reinterpret_cast<std::uintptr_t>(
                self.get_qs_value_ref(node_num));
          },
          "node_num"_a,
          "Memory address of the solar heat load value for formula binding.")
      .def(
          "get_qa_value_pointer",
          [](Nodes &self, int node_num) {
            return reinterpret_cast<std::uintptr_t>(
                self.get_qa_value_ref(node_num));
          },
          "node_num"_a,
          "Memory address of the albedo heat load value for formula binding.")
      .def(
          "get_qe_value_pointer",
          [](Nodes &self, int node_num) {
            return reinterpret_cast<std::uintptr_t>(
                self.get_qe_value_ref(node_num));
          },
          "node_num"_a,
          "Memory address of the Earth IR heat load value for formula binding.")
      .def(
          "get_qi_value_pointer",
          [](Nodes &self, int node_num) {
            return reinterpret_cast<std::uintptr_t>(
                self.get_qi_value_ref(node_num));
          },
          "node_num"_a,
          "Memory address of the internal heat load value for formula binding.")
      .def(
          "get_qr_value_pointer",
          [](Nodes &self, int node_num) {
            return reinterpret_cast<std::uintptr_t>(
                self.get_qr_value_ref(node_num));
          },
          "node_num"_a,
          "Memory address of the residual heat load value for formula binding.")
      .def(
          "get_a_value_pointer",
          [](Nodes &self, int node_num) {
            return reinterpret_cast<std::uintptr_t>(
                self.get_a_value_ref(node_num));
          },
          "node_num"_a, "Memory address of the area value for formula binding.")
      .def(
          "get_fx_value_pointer",
          [](Nodes &self, int node_num) {
            return reinterpret_cast<std::uintptr_t>(
                self.get_fx_value_ref(node_num));
          },
          "node_num"_a,
          "Memory address of the X coordinate value for formula binding.")
      .def(
          "get_fy_value_pointer",
          [](Nodes &self, int node_num) {
            return reinterpret_cast<std::uintptr_t>(
                self.get_fy_value_ref(node_num));
          },
          "node_num"_a,
          "Memory address of the Y coordinate value for formula binding.")
      .def(
          "get_fz_value_pointer",
          [](Nodes &self, int node_num) {
            return reinterpret_cast<std::uintptr_t>(
                self.get_fz_value_ref(node_num));
          },
          "node_num"_a,
          "Memory address of the Z coordinate value for formula binding.")
      .def(
          "get_eps_value_pointer",
          [](Nodes &self, int node_num) {
            return reinterpret_cast<std::uintptr_t>(
                self.get_eps_value_ref(node_num));
          },
          "node_num"_a,
          "Memory address of the IR emissivity value for formula binding.")
      .def(
          "get_aph_value_pointer",
          [](Nodes &self, int node_num) {
            return reinterpret_cast<std::uintptr_t>(
                self.get_aph_value_ref(node_num));
          },
          "node_num"_a,
          "Memory address of the solar absorptivity value for formula "
          "binding.");
}

inline void register_nodes(nb::module_ &m) {
  Node_b(m);
  Nodes_b(m);
}

} // namespace pycanha::bindings::tmm
