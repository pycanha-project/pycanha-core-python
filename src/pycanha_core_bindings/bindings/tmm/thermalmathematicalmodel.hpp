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
#include "pycanha-core/parameters/variable.hpp"
#include "pycanha-core/thermaldata/dense_time_series.hpp"
#include "pycanha-core/thermaldata/lookup_table.hpp"
#include "pycanha-core/thermaldata/sparse_time_series.hpp"
#include "pycanha-core/thermaldata/thermaldata.hpp"
#include "pycanha-core/tmm/coupling.hpp"
#include "pycanha-core/tmm/node.hpp"
#include "pycanha-core/tmm/thermalmathematicalmodel.hpp"
#include "pycanha-core/tmm/thermalnetwork.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::tmm {

inline void register_thermal_data(nb::module_ &m) {
  using pycanha::DenseTimeSeries;
  using pycanha::Index;
  using pycanha::LookupTable1D;
  using pycanha::LookupTableVec1D;
  using pycanha::SparseTimeSeries;
  using pycanha::ThermalData;
  using pycanha::ThermalNetwork;

  nb::class_<ThermalData>(
      m, "ThermalData",
      "Storage for simulation data: dense/sparse time series and\n"
      "lookup tables, each identified by a string name.")
      .def(nb::init<>(), "Create an empty ThermalData store.")
      .def(nb::init<std::shared_ptr<ThermalNetwork>>(), "network"_a,
           "Create a ThermalData store associated with a network.")
      .def("associate", &ThermalData::associate, "network"_a,
           "Associate this ThermalData with a ThermalNetwork.")
      .def_prop_ro("network_ptr", &ThermalData::network_ptr,
                   "Shared pointer to the associated ThermalNetwork.")
      .def_prop_ro(
          "network",
          [](ThermalData &self) -> ThermalNetwork & { return *self.network(); },
          nb::rv_policy::reference_internal,
          "Reference to the associated ThermalNetwork.")
      .def_prop_ro("size", &ThermalData::size,
                   "Total number of stored data objects.")

      // ── Dense Time Series ───────────────────────────────────────────
      .def(
          "add_dense_time_series",
          static_cast<DenseTimeSeries &(ThermalData::*)(
              const std::string &, DenseTimeSeries)>(
              &ThermalData::add_dense_time_series),
          "name"_a, "series"_a, nb::rv_policy::reference_internal,
          "Add or replace a named DenseTimeSeries.")
      .def(
          "add_dense_time_series",
          static_cast<DenseTimeSeries &(ThermalData::*)(const std::string &,
                                                         Index, Index)>(
              &ThermalData::add_dense_time_series),
          "name"_a, "num_timesteps"_a, "num_columns"_a,
          nb::rv_policy::reference_internal,
          "Create and add a DenseTimeSeries with the given dimensions.")
      .def(
          "get_dense_time_series",
          [](ThermalData &self, const std::string &name)
              -> DenseTimeSeries & {
            return self.get_dense_time_series(name);
          },
          nb::rv_policy::reference_internal, "name"_a,
          "Get a mutable reference to a DenseTimeSeries by name.")
      .def("has_dense_time_series", &ThermalData::has_dense_time_series,
           "name"_a,
           "Check whether a DenseTimeSeries with the given name exists.")
      .def("remove_dense_time_series", &ThermalData::remove_dense_time_series,
           "name"_a, "Remove a DenseTimeSeries by name.")

      // ── Sparse Time Series ──────────────────────────────────────────
      .def(
          "add_sparse_time_series",
          static_cast<SparseTimeSeries &(ThermalData::*)(
              const std::string &, SparseTimeSeries)>(
              &ThermalData::add_sparse_time_series),
          "name"_a, "series"_a, nb::rv_policy::reference_internal,
          "Add or replace a named SparseTimeSeries.")
      .def(
          "add_sparse_time_series",
          static_cast<SparseTimeSeries &(ThermalData::*)(const std::string &)>(
              &ThermalData::add_sparse_time_series),
          "name"_a, nb::rv_policy::reference_internal,
          "Create and add an empty SparseTimeSeries.")
      .def(
          "get_sparse_time_series",
          [](ThermalData &self, const std::string &name)
              -> SparseTimeSeries & {
            return self.get_sparse_time_series(name);
          },
          nb::rv_policy::reference_internal, "name"_a,
          "Get a mutable reference to a SparseTimeSeries by name.")
      .def("has_sparse_time_series", &ThermalData::has_sparse_time_series,
           "name"_a,
           "Check whether a SparseTimeSeries with the given name exists.")
      .def("remove_sparse_time_series",
           &ThermalData::remove_sparse_time_series, "name"_a,
           "Remove a SparseTimeSeries by name.")

      // ── Lookup Tables ───────────────────────────────────────────────
      .def("add_lookup_table", &ThermalData::add_lookup_table, "name"_a,
           "table"_a, nb::rv_policy::reference_internal,
           "Add or replace a named LookupTable1D.")
      .def(
          "get_lookup_table",
          [](ThermalData &self, const std::string &name) -> LookupTable1D & {
            return self.get_lookup_table(name);
          },
          nb::rv_policy::reference_internal, "name"_a,
          "Get a mutable reference to a LookupTable1D by name.")
      .def("has_lookup_table", &ThermalData::has_lookup_table, "name"_a,
           "Check whether a LookupTable1D with the given name exists.")
      .def("remove_lookup_table", &ThermalData::remove_lookup_table, "name"_a,
           "Remove a LookupTable1D by name.")

      // ── Vector Lookup Tables ────────────────────────────────────────
      .def("add_lookup_table_vec", &ThermalData::add_lookup_table_vec,
           "name"_a, "table"_a, nb::rv_policy::reference_internal,
           "Add or replace a named LookupTableVec1D.")
      .def(
          "get_lookup_table_vec",
          [](ThermalData &self,
             const std::string &name) -> LookupTableVec1D & {
            return self.get_lookup_table_vec(name);
          },
          nb::rv_policy::reference_internal, "name"_a,
          "Get a mutable reference to a LookupTableVec1D by name.")
      .def("has_lookup_table_vec", &ThermalData::has_lookup_table_vec,
           "name"_a,
           "Check whether a LookupTableVec1D with the given name exists.")
      .def("remove_lookup_table_vec", &ThermalData::remove_lookup_table_vec,
           "name"_a, "Remove a LookupTableVec1D by name.");
}

inline void register_thermal_mathematical_model(nb::module_ &m) {
  using pycanha::ConductiveCouplings;
  using pycanha::Coupling;
  using pycanha::ESATANReader;
  using pycanha::ExtrapolationMethod;
  using pycanha::Formulas;
  using pycanha::Index;
  using pycanha::InterpolationMethod;
  using pycanha::Node;
  using pycanha::Nodes;
  using pycanha::Parameters;
  using pycanha::RadiativeCouplings;
  using pycanha::TemperatureVariable;
  using pycanha::ThermalData;
  using pycanha::ThermalMathematicalModel;
  using pycanha::ThermalNetwork;
  using pycanha::TimeVariable;

  nb::class_<ESATANReader>(m, "ESATANReader",
                         "Reader for ESATAN TMD thermal model files.")
      .def(nb::init<ThermalMathematicalModel &>(), "model"_a,
           nb::keep_alive<1, 2>(),
           "Create a reader bound to a ThermalMathematicalModel.")
      .def("read_tmd", &ESATANReader::read_tmd, "filepath"_a,
           "Read an ESATAN TMD file into the associated model.")
      .def_rw("verbose", &ESATANReader::verbose,
              "Enable verbose logging during file reading.");

  nb::class_<ThermalMathematicalModel>(m, "ThermalMathematicalModel",
                                       "Top-level thermal mathematical model.\n\n"
                                       "Aggregates a thermal network (nodes and couplings),\n"
                                       "parameters, formulas, thermal data tables, and\n"
                                       "solver callbacks. Non-copyable.")
      .def(nb::init<std::string>(), "model_name"_a,
           "Create a model with an empty network.")
      .def(nb::init<std::string, std::shared_ptr<Nodes>,
                    std::shared_ptr<ConductiveCouplings>,
                    std::shared_ptr<RadiativeCouplings>>(),
           "model_name"_a, "nodes"_a, "conductive"_a, "radiative"_a,
           "Create a model from existing network components.")
      .def(nb::init<std::string, std::shared_ptr<Nodes>,
                    std::shared_ptr<ConductiveCouplings>,
                    std::shared_ptr<RadiativeCouplings>,
                    std::shared_ptr<Parameters>, std::shared_ptr<Formulas>,
                    std::shared_ptr<ThermalData>>(),
           "model_name"_a, "nodes"_a, "conductive"_a, "radiative"_a,
           "parameters"_a, "formulas"_a, "thermal_data"_a,
           "Create a model from all existing components.")
      .def_prop_rw(
          "name",
          [](ThermalMathematicalModel &self) -> std::string & {
            return self.name;
          },
          [](ThermalMathematicalModel &self, const std::string &v) {
            self.name = v;
          },
          nb::rv_policy::reference_internal,
          "Model name.")
      .def_prop_rw(
          "time",
          [](ThermalMathematicalModel &self) -> double & { return self.time; },
          [](ThermalMathematicalModel &self, double value) {
            self.time = value;
          },
          nb::rv_policy::reference_internal,
          "Current simulation time [s].")
      .def_prop_ro(
          "network",
          [](ThermalMathematicalModel &self) -> ThermalNetwork & {
            return self.network();
          },
          nb::rv_policy::reference_internal,
          "Reference to the ThermalNetwork.")
      .def_prop_ro(
          "network_ptr",
          [](ThermalMathematicalModel &self) { return self.network_ptr(); },
          "Shared pointer to the ThermalNetwork.")
      .def_prop_ro(
          "nodes",
          [](ThermalMathematicalModel &self) -> Nodes & {
            return self.nodes();
          },
          nb::rv_policy::reference_internal,
          "Reference to the Nodes container.")
      .def_prop_ro(
          "nodes_ptr",
          [](ThermalMathematicalModel &self) { return self.nodes_ptr(); },
          "Shared pointer to the Nodes container.")
      .def_prop_ro(
          "conductive_couplings",
          [](ThermalMathematicalModel &self) -> ConductiveCouplings & {
            return self.conductive_couplings();
          },
          nb::rv_policy::reference_internal,
          "Reference to the ConductiveCouplings.")
      .def_prop_ro(
          "radiative_couplings",
          [](ThermalMathematicalModel &self) -> RadiativeCouplings & {
            return self.radiative_couplings();
          },
          nb::rv_policy::reference_internal,
          "Reference to the RadiativeCouplings.")
      .def_prop_ro(
          "parameters",
          [](ThermalMathematicalModel &self) -> Parameters & {
            return self.parameters;
          },
          nb::rv_policy::reference_internal,
          "Reference to the Parameters store.")
      .def_prop_ro(
          "formulas",
          [](ThermalMathematicalModel &self) -> Formulas & {
            return self.formulas;
          },
          nb::rv_policy::reference_internal,
          "Reference to the Formulas collection.")
      .def_prop_ro(
          "thermal_data",
          [](ThermalMathematicalModel &self) -> ThermalData & {
            return self.thermal_data;
          },
          nb::rv_policy::reference_internal,
          "Reference to the ThermalData store.")
      .def(
          "add_node",
          [](ThermalMathematicalModel &self, const Node &node) {
            self.add_node(node);
          },
          "node"_a,
          "Add a Node object to the model.")
      .def(
          "add_node",
          [](ThermalMathematicalModel &self, int node_num) {
            self.add_node(static_cast<Index>(node_num));
          },
          "node_num"_a,
          "Add a default node by user node number.")
      .def(
          "add_conductive_coupling",
          [](ThermalMathematicalModel &self, int node_1, int node_2,
             double value) {
            self.add_conductive_coupling(static_cast<Index>(node_1),
                                         static_cast<Index>(node_2), value);
          },
          "node_1"_a, "node_2"_a, "value"_a,
          "Add a conductive coupling [W/K] between two nodes.")
      .def(
          "add_conductive_coupling",
          [](ThermalMathematicalModel &self, const Coupling &coupling) {
            self.add_conductive_coupling(coupling);
          },
          "coupling"_a,
          "Add a conductive coupling from a Coupling object.")
      .def(
          "add_radiative_coupling",
          [](ThermalMathematicalModel &self, int node_1, int node_2,
             double value) {
            self.add_radiative_coupling(static_cast<Index>(node_1),
                                        static_cast<Index>(node_2), value);
          },
          "node_1"_a, "node_2"_a, "value"_a,
          "Add a radiative coupling [m^2] between two nodes.")
      .def(
          "add_radiative_coupling",
          [](ThermalMathematicalModel &self, const Coupling &coupling) {
            self.add_radiative_coupling(coupling);
          },
          "coupling"_a,
          "Add a radiative coupling from a Coupling object.")
      // ── Time variables ──────────────────────────────────────────────
      .def(
          "add_time_variable",
          [](ThermalMathematicalModel &self, const std::string &name,
             Eigen::VectorXd x_data, Eigen::VectorXd y_data,
             InterpolationMethod interp, ExtrapolationMethod extrap) {
            self.add_time_variable(name, std::move(x_data), std::move(y_data),
                                   interp, extrap);
          },
          "name"_a, "x_data"_a, "y_data"_a,
          "interpolation"_a = InterpolationMethod::Linear,
          "extrapolation"_a = ExtrapolationMethod::Constant,
          "Add a time-driven variable (parameter updated from a lookup table "
          "of time).")
      .def("remove_time_variable",
           &ThermalMathematicalModel::remove_time_variable, "name"_a,
           "Remove a time variable by name.")
      .def("has_time_variable",
           &ThermalMathematicalModel::has_time_variable, "name"_a,
           "Check whether a time variable with the given name exists.")
      .def(
          "get_time_variable",
          [](const ThermalMathematicalModel &self,
             const std::string &name) -> const TimeVariable & {
            return self.get_time_variable(name);
          },
          nb::rv_policy::reference_internal, "name"_a,
          "Get a read-only reference to a time variable by name.")
      // ── Temperature variables ───────────────────────────────────────
      .def(
          "add_temperature_variable",
          [](ThermalMathematicalModel &self, const std::string &name,
             Eigen::VectorXd x_data, Eigen::VectorXd y_data,
             InterpolationMethod interp, ExtrapolationMethod extrap) {
            self.add_temperature_variable(name, std::move(x_data),
                                          std::move(y_data), interp, extrap);
          },
          "name"_a, "x_data"_a, "y_data"_a,
          "interpolation"_a = InterpolationMethod::Linear,
          "extrapolation"_a = ExtrapolationMethod::Constant,
          "Add a temperature-driven variable (evaluated from a lookup table "
          "of temperature).")
      .def("remove_temperature_variable",
           &ThermalMathematicalModel::remove_temperature_variable, "name"_a,
           "Remove a temperature variable by name.")
      .def("has_temperature_variable",
           &ThermalMathematicalModel::has_temperature_variable, "name"_a,
           "Check whether a temperature variable with the given name exists.")
      .def(
          "get_temperature_variable",
          [](const ThermalMathematicalModel &self,
             const std::string &name) -> const TemperatureVariable & {
            return self.get_temperature_variable(name);
          },
          nb::rv_policy::reference_internal, "name"_a,
          "Get a read-only reference to a temperature variable by name.")
      // ── Callbacks ───────────────────────────────────────────────────
      .def("callback_solver_loop",
           &ThermalMathematicalModel::callback_solver_loop,
           "Execute all registered solver-loop callbacks.")
      .def("callback_transient_time_change",
           &ThermalMathematicalModel::callback_transient_time_change,
           "Execute all registered time-change callbacks.")
      .def("callback_transient_after_timestep",
           &ThermalMathematicalModel::callback_transient_after_timestep,
           "Execute all registered after-timestep callbacks.")
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
      .def_rw("callbacks_active", &ThermalMathematicalModel::callbacks_active,
              "Master switch to enable/disable all callbacks.")
      .def_rw("internal_callbacks_active",
              &ThermalMathematicalModel::internal_callbacks_active,
              "Enable/disable internal (C++) callbacks.")
      .def_rw("c_callbacks_active",
              &ThermalMathematicalModel::c_callbacks_active,
              "Enable/disable C function-pointer callbacks.")
      .def_rw("python_callbacks_active",
              &ThermalMathematicalModel::python_callbacks_active,
              "Enable/disable Python callbacks.")
      .def_rw("python_formulas_active",
              &ThermalMathematicalModel::python_formulas_active,
              "Enable/disable Python formula evaluation during callbacks.")
      .def_rw("python_apply_formulas",
              &ThermalMathematicalModel::python_apply_formulas,
              "Python callable invoked to apply formulas during callbacks.")
      .def_rw("python_extern_callback_solver_loop",
              &ThermalMathematicalModel::python_extern_callback_solver_loop,
              "Python callable invoked each solver iteration.")
      .def_rw("python_extern_callback_transient_time_change",
              &ThermalMathematicalModel::
                  python_extern_callback_transient_time_change,
              "Python callable invoked when simulation time changes.")
      .def_rw("python_extern_callback_transient_after_timestep",
              &ThermalMathematicalModel::
                  python_extern_callback_transient_after_timestep,
              "Python callable invoked after each transient timestep.");
}

} // namespace pycanha::bindings::tmm
