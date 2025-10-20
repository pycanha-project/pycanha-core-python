#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "pycanha-core/tmm/node.hpp"
#include "pycanha-core/tmm/nodes.hpp"

namespace py = pybind11;
using namespace pybind11::literals;  // NOLINT(build/namespaces)

using pycanha::Node;
using pycanha::NodeType;
using pycanha::Nodes;

namespace pycanha::bindings::tmm {

inline void Node_b(py::module_ &m) {
  py::enum_<NodeType>(m, "NodeType")
      .value("DIFFUSIVE", NodeType::DIFFUSIVE_NODE)
      .value("BOUNDARY", NodeType::BOUNDARY_NODE)
      .export_values();

     py::class_<Node>(m, "Node")
               .def(py::init<int>(), "node_num"_a, "Create an unassociated node")
               .def(py::init<const Node &>(), "Copy constructor")
               .def_property("node_num", &Node::get_node_num, &Node::set_node_num)
               .def_property(
                         "type",
                         [](Node &self) {
                              return static_cast<NodeType>(self.get_type());
                         },
                         [](Node &self, NodeType node_type) {
                              self.set_type(static_cast<char>(node_type));
                         })
      .def_property("T", &Node::get_T, &Node::set_T)
      .def_property("C", &Node::get_C, &Node::set_C)
      .def_property("qs", &Node::get_qs, &Node::set_qs)
      .def_property("qa", &Node::get_qa, &Node::set_qa)
      .def_property("qe", &Node::get_qe, &Node::set_qe)
      .def_property("qi", &Node::get_qi, &Node::set_qi)
      .def_property("qr", &Node::get_qr, &Node::set_qr)
      .def_property("a", &Node::get_a, &Node::set_a)
      .def_property("fx", &Node::get_fx, &Node::set_fx)
      .def_property("fy", &Node::get_fy, &Node::set_fy)
      .def_property("fz", &Node::get_fz, &Node::set_fz)
      .def_property("eps", &Node::get_eps, &Node::set_eps)
      .def_property("aph", &Node::get_aph, &Node::set_aph)
      .def_property("literal_C", &Node::get_literal_C, &Node::set_literal_C)
      .def("int_node_num", &Node::get_int_node_num)
      .def(
          "parent_pointer",
          [](Node &self) { return self.get_parent_pointer().lock(); },
          py::return_value_policy::reference)
      .def("parent_pointer_address", &Node::get_int_parent_pointer,
           "Unsigned integer with the parent Nodes memory address");
}

inline void Nodes_b(py::module_ &m) {
  py::class_<Nodes, std::shared_ptr<Nodes>>(m, "Nodes")
      .def(py::init<>())
      .def_property("estimated_number_of_nodes",
                    [](Nodes &self) { return self.estimated_number_of_nodes; },
                    [](Nodes &self, int value) {
                      self.estimated_number_of_nodes = value;
                    })
      .def("add_node", &Nodes::add_node, "node"_a)
      .def("remove_node", &Nodes::remove_node, "node_num"_a)
      .def("is_node", &Nodes::is_node, "node_num"_a)
      .def("get_type",
           [](Nodes &self, int node_num) {
             return static_cast<NodeType>(self.get_type(node_num));
           },
           "node_num"_a)
      .def("set_type",
           [](Nodes &self, int node_num, NodeType node_type) {
             return self.set_type(node_num, static_cast<char>(node_type));
           },
           "node_num"_a, "node_type"_a)
      .def("get_T", &Nodes::get_T, "node_num"_a)
      .def("set_T", &Nodes::set_T, "node_num"_a, "value"_a)
      .def("get_C", &Nodes::get_C, "node_num"_a)
      .def("set_C", &Nodes::set_C, "node_num"_a, "value"_a)
      .def("get_qs", &Nodes::get_qs, "node_num"_a)
      .def("set_qs", &Nodes::set_qs, "node_num"_a, "value"_a)
      .def("get_qa", &Nodes::get_qa, "node_num"_a)
      .def("set_qa", &Nodes::set_qa, "node_num"_a, "value"_a)
      .def("get_qe", &Nodes::get_qe, "node_num"_a)
      .def("set_qe", &Nodes::set_qe, "node_num"_a, "value"_a)
      .def("get_qi", &Nodes::get_qi, "node_num"_a)
      .def("set_qi", &Nodes::set_qi, "node_num"_a, "value"_a)
      .def("get_qr", &Nodes::get_qr, "node_num"_a)
      .def("set_qr", &Nodes::set_qr, "node_num"_a, "value"_a)
      .def("get_a", &Nodes::get_a, "node_num"_a)
      .def("set_a", &Nodes::set_a, "node_num"_a, "value"_a)
      .def("get_fx", &Nodes::get_fx, "node_num"_a)
      .def("set_fx", &Nodes::set_fx, "node_num"_a, "value"_a)
      .def("get_fy", &Nodes::get_fy, "node_num"_a)
      .def("set_fy", &Nodes::set_fy, "node_num"_a, "value"_a)
      .def("get_fz", &Nodes::get_fz, "node_num"_a)
      .def("set_fz", &Nodes::set_fz, "node_num"_a, "value"_a)
      .def("get_eps", &Nodes::get_eps, "node_num"_a)
      .def("set_eps", &Nodes::set_eps, "node_num"_a, "value"_a)
      .def("get_aph", &Nodes::get_aph, "node_num"_a)
      .def("set_aph", &Nodes::set_aph, "node_num"_a, "value"_a)
      .def("get_literal_C", &Nodes::get_literal_C, "node_num"_a)
      .def("set_literal_C", &Nodes::set_literal_C, "node_num"_a,
           "literal"_a)
      .def("get_idx_from_node_num", &Nodes::get_idx_from_node_num,
           "node_num"_a)
      .def("get_node_num_from_idx", &Nodes::get_node_num_from_idx, "idx"_a)
      .def("get_node_from_node_num", &Nodes::get_node_from_node_num,
           py::return_value_policy::move, "node_num"_a)
      .def("get_node_from_idx", &Nodes::get_node_from_idx,
           py::return_value_policy::move, "idx"_a)
      .def_property_readonly("num_nodes", &Nodes::get_num_nodes)
      .def_property_readonly("num_diff_nodes", &Nodes::get_num_diff_nodes)
      .def_property_readonly("num_bound_nodes", &Nodes::get_num_bound_nodes)
      .def("is_mapped", &Nodes::is_mapped)
      .def("get_T_value_pointer",
           [](Nodes &self, int node_num) {
             return reinterpret_cast<std::uintptr_t>(
                 self.get_T_value_ref(node_num));
           },
           "node_num"_a)
      .def("get_C_value_pointer",
           [](Nodes &self, int node_num) {
             return reinterpret_cast<std::uintptr_t>(
                 self.get_C_value_ref(node_num));
           },
           "node_num"_a)
      .def("get_qs_value_pointer",
           [](Nodes &self, int node_num) {
             return reinterpret_cast<std::uintptr_t>(
                 self.get_qs_value_ref(node_num));
           },
           "node_num"_a)
      .def("get_qa_value_pointer",
           [](Nodes &self, int node_num) {
             return reinterpret_cast<std::uintptr_t>(
                 self.get_qa_value_ref(node_num));
           },
           "node_num"_a)
      .def("get_qe_value_pointer",
           [](Nodes &self, int node_num) {
             return reinterpret_cast<std::uintptr_t>(
                 self.get_qe_value_ref(node_num));
           },
           "node_num"_a)
      .def("get_qi_value_pointer",
           [](Nodes &self, int node_num) {
             return reinterpret_cast<std::uintptr_t>(
                 self.get_qi_value_ref(node_num));
           },
           "node_num"_a)
      .def("get_qr_value_pointer",
           [](Nodes &self, int node_num) {
             return reinterpret_cast<std::uintptr_t>(
                 self.get_qr_value_ref(node_num));
           },
           "node_num"_a)
      .def("get_a_value_pointer",
           [](Nodes &self, int node_num) {
             return reinterpret_cast<std::uintptr_t>(
                 self.get_a_value_ref(node_num));
           },
           "node_num"_a)
      .def("get_fx_value_pointer",
           [](Nodes &self, int node_num) {
             return reinterpret_cast<std::uintptr_t>(
                 self.get_fx_value_ref(node_num));
           },
           "node_num"_a)
      .def("get_fy_value_pointer",
           [](Nodes &self, int node_num) {
             return reinterpret_cast<std::uintptr_t>(
                 self.get_fy_value_ref(node_num));
           },
           "node_num"_a)
      .def("get_fz_value_pointer",
           [](Nodes &self, int node_num) {
             return reinterpret_cast<std::uintptr_t>(
                 self.get_fz_value_ref(node_num));
           },
           "node_num"_a)
      .def("get_eps_value_pointer",
           [](Nodes &self, int node_num) {
             return reinterpret_cast<std::uintptr_t>(
                 self.get_eps_value_ref(node_num));
           },
           "node_num"_a)
      .def("get_aph_value_pointer",
           [](Nodes &self, int node_num) {
             return reinterpret_cast<std::uintptr_t>(
                 self.get_aph_value_ref(node_num));
           },
           "node_num"_a);
}

inline void register_nodes(py::module_ &m) {
  Node_b(m);
  Nodes_b(m);
}

}  // namespace pycanha::bindings::tmm
