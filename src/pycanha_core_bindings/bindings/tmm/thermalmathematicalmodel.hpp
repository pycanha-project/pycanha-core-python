#pragma once

#include <memory>
#include <string>

#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/function.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "pycanha-core/globals.hpp"
#include "pycanha-core/io/esatan.hpp"
#include "pycanha-core/parameters/formulas.hpp"
#include "pycanha-core/parameters/parameters.hpp"
#include "pycanha-core/thermaldata/thermaldata.hpp"
#include "pycanha-core/tmm/coupling.hpp"
#include "pycanha-core/tmm/node.hpp"
#include "pycanha-core/tmm/thermalmathematicalmodel.hpp"
#include "pycanha-core/tmm/thermalnetwork.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::tmm {

inline void register_thermal_data(nb::module_ &m) {
  using pycanha::ThermalData;
  using pycanha::ThermalNetwork;

  nb::class_<ThermalData>(m, "ThermalData", "Class for storing thermal data.")
      .def(nb::init<>())
      .def(nb::init<std::shared_ptr<ThermalNetwork>>(), "network"_a)
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
          nb::rv_policy::reference_internal, "name"_a)
      .def(
          "get_table_const",
          [](const ThermalData &self,
             const std::string &name) -> const ThermalData::MatrixDataType & {
            return self.get_table(name);
          },
          nb::rv_policy::reference_internal, "name"_a)
      .def("has_table", &ThermalData::has_table, "name"_a)
      .def_prop_ro("network_ptr", &ThermalData::network_ptr)
      .def_prop_ro(
          "network",
          [](ThermalData &self) -> ThermalNetwork & { return *self.network(); },
          nb::rv_policy::reference_internal)
      .def_prop_ro("size", &ThermalData::size);
}

inline void register_thermal_mathematical_model(nb::module_ &m) {
  using pycanha::ConductiveCouplings;
  using pycanha::Coupling;
  using pycanha::ESATANReader;
  using pycanha::Formulas;
  using pycanha::Index;
  using pycanha::Node;
  using pycanha::Nodes;
  using pycanha::Parameters;
  using pycanha::RadiativeCouplings;
  using pycanha::ThermalData;
  using pycanha::ThermalMathematicalModel;
  using pycanha::ThermalNetwork;

  nb::class_<ESATANReader>(m, "ESATANReader", "ESATAN TMD file reader.")
      .def(nb::init<ThermalMathematicalModel &>(), "model"_a,
           nb::keep_alive<1, 2>())
      .def("read_tmd", &ESATANReader::read_tmd, "filepath"_a,
           "Read an ESATAN TMD file into the associated model.")
      .def_rw("verbose", &ESATANReader::verbose,
              "Enable verbose reader output.");

  nb::class_<ThermalMathematicalModel>(m, "ThermalMathematicalModel",
                                       "Thermal Mathematical Model (TMM).")
      .def(nb::init<std::string>(), "model_name"_a)
      .def(nb::init<std::string, std::shared_ptr<Nodes>,
                    std::shared_ptr<ConductiveCouplings>,
                    std::shared_ptr<RadiativeCouplings>>(),
           "model_name"_a, "nodes"_a, "conductive"_a, "radiative"_a)
      .def(nb::init<std::string, std::shared_ptr<Nodes>,
                    std::shared_ptr<ConductiveCouplings>,
                    std::shared_ptr<RadiativeCouplings>,
                    std::shared_ptr<Parameters>, std::shared_ptr<Formulas>,
                    std::shared_ptr<ThermalData>>(),
           "model_name"_a, "nodes"_a, "conductive"_a, "radiative"_a,
           "parameters"_a, "formulas"_a, "thermal_data"_a)
      .def_prop_rw(
          "name",
          [](ThermalMathematicalModel &self) -> std::string & {
            return self.name;
          },
          [](ThermalMathematicalModel &self, const std::string &v) {
            self.name = v;
          },
          nb::rv_policy::reference_internal)
      .def_prop_rw(
          "time",
          [](ThermalMathematicalModel &self) -> double & { return self.time; },
          [](ThermalMathematicalModel &self, double value) {
            self.time = value;
          },
          nb::rv_policy::reference_internal)
      .def_prop_ro(
          "network",
          [](ThermalMathematicalModel &self) -> ThermalNetwork & {
            return self.network();
          },
          nb::rv_policy::reference_internal)
      .def_prop_ro(
          "network_ptr",
          [](ThermalMathematicalModel &self) { return self.network_ptr(); })
      .def_prop_ro(
          "nodes",
          [](ThermalMathematicalModel &self) -> Nodes & {
            return self.nodes();
          },
          nb::rv_policy::reference_internal)
      .def_prop_ro(
          "nodes_ptr",
          [](ThermalMathematicalModel &self) { return self.nodes_ptr(); })
      .def_prop_ro(
          "conductive_couplings",
          [](ThermalMathematicalModel &self) -> ConductiveCouplings & {
            return self.conductive_couplings();
          },
          nb::rv_policy::reference_internal)
      .def_prop_ro(
          "radiative_couplings",
          [](ThermalMathematicalModel &self) -> RadiativeCouplings & {
            return self.radiative_couplings();
          },
          nb::rv_policy::reference_internal)
      .def_prop_ro(
          "parameters",
          [](ThermalMathematicalModel &self) -> Parameters & {
            return self.parameters;
          },
          nb::rv_policy::reference_internal)
      .def_prop_ro(
          "formulas",
          [](ThermalMathematicalModel &self) -> Formulas & {
            return self.formulas;
          },
          nb::rv_policy::reference_internal)
      .def_prop_ro(
          "thermal_data",
          [](ThermalMathematicalModel &self) -> ThermalData & {
            return self.thermal_data;
          },
          nb::rv_policy::reference_internal)
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
      .def(
          "read_tmd",
          [](ThermalMathematicalModel &self, const std::string &filepath,
             bool verbose) {
            ESATANReader reader(self);
            reader.verbose = verbose;
            reader.read_tmd(filepath);
          },
          "filepath"_a, "verbose"_a = false,
          "Read an ESATAN TMD file into this model.")
      .def_rw("callbacks_active", &ThermalMathematicalModel::callbacks_active)
      .def_rw("internal_callbacks_active",
              &ThermalMathematicalModel::internal_callbacks_active)
      .def_rw("c_callbacks_active",
              &ThermalMathematicalModel::c_callbacks_active)
      .def_rw("python_callbacks_active",
              &ThermalMathematicalModel::python_callbacks_active)
      .def_rw("python_formulas_active",
              &ThermalMathematicalModel::python_formulas_active)
      .def_rw("python_apply_formulas",
              &ThermalMathematicalModel::python_apply_formulas)
      .def_rw("python_extern_callback_solver_loop",
              &ThermalMathematicalModel::python_extern_callback_solver_loop)
      .def_rw("python_extern_callback_transient_time_change",
              &ThermalMathematicalModel::
                  python_extern_callback_transient_time_change)
      .def_rw("python_extern_callback_transient_after_timestep",
              &ThermalMathematicalModel::
                  python_extern_callback_transient_after_timestep);
}

} // namespace pycanha::bindings::tmm
