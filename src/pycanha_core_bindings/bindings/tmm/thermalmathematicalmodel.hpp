#pragma once

#include <memory>
#include <string>

#include <pybind11/eigen.h>
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "pycanha-core/globals.hpp"
#include "pycanha-core/parameters/formulas.hpp"
#include "pycanha-core/parameters/parameters.hpp"
#include "pycanha-core/thermaldata/thermaldata.hpp"
#include "pycanha-core/tmm/coupling.hpp"
#include "pycanha-core/tmm/node.hpp"
#include "pycanha-core/tmm/thermalmathematicalmodel.hpp"
#include "pycanha-core/tmm/thermalnetwork.hpp"

namespace py = pybind11;
using namespace pybind11::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::tmm {

inline void register_thermal_data(py::module_ &m) {
  using pycanha::ThermalData;
  using pycanha::ThermalNetwork;

  py::class_<ThermalData, std::shared_ptr<ThermalData>>(m, "ThermalData")
      .def(py::init<>())
      .def(py::init<std::shared_ptr<ThermalNetwork>>(), "network"_a)
      .def("associate", &ThermalData::associate, "network"_a)
      .def("create_new_table", &ThermalData::create_new_table, "name"_a,
           "rows"_a, "cols"_a)
      .def("create_reset_table", &ThermalData::create_reset_table, "name"_a,
           "rows"_a, "cols"_a)
      .def("remove_table", &ThermalData::remove_table, "name"_a)
      .def(
          "get_table",
          [](ThermalData &self, const std::string &name)
              -> ThermalData::MatrixDataType & { return self.get_table(name); },
          py::return_value_policy::reference_internal, "name"_a)
      .def(
          "get_table_const",
          [](const ThermalData &self,
             const std::string &name) -> const ThermalData::MatrixDataType & {
            return self.get_table(name);
          },
          py::return_value_policy::reference_internal, "name"_a)
      .def("has_table", &ThermalData::has_table, "name"_a)
      .def_property_readonly("network_ptr", &ThermalData::network_ptr)
      .def_property_readonly(
          "network",
          [](ThermalData &self) -> ThermalNetwork & { return *self.network(); },
          py::return_value_policy::reference_internal)
      .def_property_readonly("size", &ThermalData::size);
}

inline void register_thermal_mathematical_model(py::module_ &m) {
  using pycanha::ConductiveCouplings;
  using pycanha::Coupling;
  using pycanha::Formulas;
  using pycanha::Index;
  using pycanha::Node;
  using pycanha::Nodes;
  using pycanha::Parameters;
  using pycanha::RadiativeCouplings;
  using pycanha::ThermalData;
  using pycanha::ThermalMathematicalModel;
  using pycanha::ThermalNetwork;

  py::class_<ThermalMathematicalModel,
             std::shared_ptr<ThermalMathematicalModel>>(
      m, "ThermalMathematicalModel")
      .def(py::init<std::string>(), "model_name"_a)
      .def(py::init<std::string, std::shared_ptr<Nodes>,
                    std::shared_ptr<ConductiveCouplings>,
                    std::shared_ptr<RadiativeCouplings>>(),
           "model_name"_a, "nodes"_a, "conductive"_a, "radiative"_a)
      .def(py::init<std::string, std::shared_ptr<Nodes>,
                    std::shared_ptr<ConductiveCouplings>,
                    std::shared_ptr<RadiativeCouplings>,
                    std::shared_ptr<Parameters>, std::shared_ptr<Formulas>,
                    std::shared_ptr<ThermalData>>(),
           "model_name"_a, "nodes"_a, "conductive"_a, "radiative"_a,
           "parameters"_a, "formulas"_a, "thermal_data"_a)
      .def_property(
          "name",
          [](ThermalMathematicalModel &self) -> std::string & {
            return self.name;
          },
          [](ThermalMathematicalModel &self, const std::string &v) {
            self.name = v;
          },
          py::return_value_policy::reference_internal)
      .def_property(
          "time",
          [](ThermalMathematicalModel &self) -> double & { return self.time; },
          [](ThermalMathematicalModel &self, double value) {
            self.time = value;
          },
          py::return_value_policy::reference_internal)
      .def_property_readonly(
          "network",
          [](ThermalMathematicalModel &self) -> ThermalNetwork & {
            return self.network();
          },
          py::return_value_policy::reference_internal)
      .def_property_readonly(
          "network_ptr",
          [](ThermalMathematicalModel &self) { return self.network_ptr(); })
      .def_property_readonly(
          "nodes",
          [](ThermalMathematicalModel &self) -> Nodes & {
            return self.nodes();
          },
          py::return_value_policy::reference_internal)
      .def_property_readonly(
          "nodes_ptr",
          [](ThermalMathematicalModel &self) { return self.nodes_ptr(); })
      .def_property_readonly(
          "conductive_couplings",
          [](ThermalMathematicalModel &self) -> ConductiveCouplings & {
            return self.conductive_couplings();
          },
          py::return_value_policy::reference_internal)
      .def_property_readonly(
          "radiative_couplings",
          [](ThermalMathematicalModel &self) -> RadiativeCouplings & {
            return self.radiative_couplings();
          },
          py::return_value_policy::reference_internal)
      .def_property_readonly(
          "parameters",
          [](ThermalMathematicalModel &self) -> Parameters & {
            return self.parameters;
          },
          py::return_value_policy::reference_internal)
      .def_property_readonly(
          "formulas",
          [](ThermalMathematicalModel &self) -> Formulas & {
            return self.formulas;
          },
          py::return_value_policy::reference_internal)
      .def_property_readonly(
          "thermal_data",
          [](ThermalMathematicalModel &self) -> ThermalData & {
            return self.thermal_data;
          },
          py::return_value_policy::reference_internal)
      .def(
          "add_node",
          [](ThermalMathematicalModel &self, const Node &node) {
            self.add_node(node);
          },
          "node"_a)
      .def(
          "add_node",
          [](ThermalMathematicalModel &self, int node_num) {
            self.add_node(static_cast<Index>(node_num));
          },
          "node_num"_a)
      .def(
          "add_conductive_coupling",
          [](ThermalMathematicalModel &self, int node_1, int node_2,
             double value) {
            self.add_conductive_coupling(static_cast<Index>(node_1),
                                         static_cast<Index>(node_2), value);
          },
          "node_1"_a, "node_2"_a, "value"_a)
      .def(
          "add_conductive_coupling",
          [](ThermalMathematicalModel &self, const Coupling &coupling) {
            self.add_conductive_coupling(coupling);
          },
          "coupling"_a)
      .def(
          "add_radiative_coupling",
          [](ThermalMathematicalModel &self, int node_1, int node_2,
             double value) {
            self.add_radiative_coupling(static_cast<Index>(node_1),
                                        static_cast<Index>(node_2), value);
          },
          "node_1"_a, "node_2"_a, "value"_a)
      .def(
          "add_radiative_coupling",
          [](ThermalMathematicalModel &self, const Coupling &coupling) {
            self.add_radiative_coupling(coupling);
          },
          "coupling"_a)
      .def("callback_solver_loop",
           &ThermalMathematicalModel::callback_solver_loop)
      .def("callback_transient_time_change",
           &ThermalMathematicalModel::callback_transient_time_change)
      .def("callback_transient_after_timestep",
           &ThermalMathematicalModel::callback_transient_after_timestep)
      .def_readwrite("callbacks_active",
                     &ThermalMathematicalModel::callbacks_active)
      .def_readwrite("internal_callbacks_active",
                     &ThermalMathematicalModel::internal_callbacks_active)
      .def_readwrite("c_callbacks_active",
                     &ThermalMathematicalModel::c_callbacks_active)
      .def_readwrite("python_callbacks_active",
                     &ThermalMathematicalModel::python_callbacks_active)
      .def_readwrite("python_formulas_active",
                     &ThermalMathematicalModel::python_formulas_active)
      .def_readwrite("python_apply_formulas",
                     &ThermalMathematicalModel::python_apply_formulas)
      .def_readwrite(
          "python_extern_callback_solver_loop",
          &ThermalMathematicalModel::python_extern_callback_solver_loop)
      .def_readwrite("python_extern_callback_transient_time_change",
                     &ThermalMathematicalModel::
                         python_extern_callback_transient_time_change)
      .def_readwrite("python_extern_callback_transient_after_timestep",
                     &ThermalMathematicalModel::
                         python_extern_callback_transient_after_timestep);
}

} // namespace pycanha::bindings::tmm
