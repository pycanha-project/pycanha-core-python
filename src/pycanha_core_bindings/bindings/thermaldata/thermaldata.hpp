#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <nanobind/eigen/dense.h>
#include <nanobind/eigen/sparse.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "pycanha-core/globals.hpp"
#include "pycanha-core/io/esatan.hpp"
#include "pycanha-core/parameters/variable.hpp"
#include "pycanha-core/thermaldata/data_model.hpp"
#include "pycanha-core/thermaldata/data_model_store.hpp"
#include "pycanha-core/thermaldata/data_table_store.hpp"
#include "pycanha-core/thermaldata/dense_matrix_time_series.hpp"
#include "pycanha-core/thermaldata/dense_time_series.hpp"
#include "pycanha-core/thermaldata/lookup_table.hpp"
#include "pycanha-core/thermaldata/sparse_time_series.hpp"
#include "pycanha-core/thermaldata/thermaldata.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::thermaldata {

inline void register_thermaldata_types(nb::module_ &m) {
    using pycanha::DataModel;
    using pycanha::DataModelAttribute;
    using pycanha::DataModelStore;
    using pycanha::DataTableStore;
    using pycanha::DenseMatrixTimeSeries;
  using pycanha::DenseTimeSeries;
  using pycanha::ExtrapolationMethod;
  using pycanha::Index;
  using pycanha::InterpolationMethod;
  using pycanha::LookupTable1D;
  using pycanha::LookupTableVec1D;
  using pycanha::SparseTimeSeries;
  using pycanha::TemperatureVariable;
  using pycanha::TimeVariable;

  // ── Interpolation / Extrapolation enums ───────────────────────────────

  nb::enum_<InterpolationMethod>(
      m, "InterpolationMethod",
      "Method used for interpolating between data points.")
      .value("LINEAR", InterpolationMethod::Linear,
             "Linear interpolation between adjacent points.")
      .value("NEAREST_LOWER", InterpolationMethod::NearestLower,
             "Use the nearest lower data point.")
      .value("NEAREST_UPPER", InterpolationMethod::NearestUpper,
             "Use the nearest upper data point.")
      .value("STEP", InterpolationMethod::Step,
             "Step function (piecewise constant).")
      .export_values();

  nb::enum_<ExtrapolationMethod>(
      m, "ExtrapolationMethod",
      "Method used when querying outside the data range.")
      .value("CONSTANT", ExtrapolationMethod::Constant,
             "Clamp to the nearest boundary value.")
      .value("LINEAR", ExtrapolationMethod::Linear,
             "Extrapolate linearly from the boundary slope.")
      .value("THROW", ExtrapolationMethod::Throw,
             "Raise an error if the query is out of range.")
      .export_values();

  // ── LookupTable1D ─────────────────────────────────────────────────────

  nb::class_<LookupTable1D>(
      m, "LookupTable1D",
      "1-D lookup table with configurable interpolation and\n"
      "extrapolation. Maps scalar x -> scalar y.")
      .def(nb::init<>(), "Create an empty lookup table.")
      .def(nb::init<Eigen::VectorXd, Eigen::VectorXd, InterpolationMethod,
                    ExtrapolationMethod>(),
           "x"_a, "y"_a,
           "interpolation"_a = InterpolationMethod::Linear,
           "extrapolation"_a = ExtrapolationMethod::Constant,
           "Create a lookup table from x and y arrays.")
      .def("__call__", &LookupTable1D::operator(), "x_value"_a,
           "Evaluate the table at a single x value.")
      .def(
          "evaluate",
          static_cast<double (LookupTable1D::*)(double) const>(
              &LookupTable1D::evaluate),
          "x_value"_a, "Evaluate the table at a single x value.")
      .def(
          "evaluate",
          static_cast<Eigen::VectorXd (LookupTable1D::*)(
              const Eigen::VectorXd &) const>(&LookupTable1D::evaluate),
          "x_values"_a, "Evaluate the table at an array of x values.")
      .def_prop_ro("x", &LookupTable1D::x,
                   "Input x data points (read-only view).")
      .def_prop_ro("y", &LookupTable1D::y,
                   "Output y data points (read-only view).")
      .def_prop_ro("size", &LookupTable1D::size,
                   "Number of data points in the table.")
      .def_prop_rw("interpolation_method",
                   &LookupTable1D::interpolation_method,
                   &LookupTable1D::set_interpolation_method,
                   "Interpolation method used between data points.")
      .def_prop_rw("extrapolation_method",
                   &LookupTable1D::extrapolation_method,
                   &LookupTable1D::set_extrapolation_method,
                   "Extrapolation method used outside the data range.")
      .def("set_data", &LookupTable1D::set_data, "x"_a, "y"_a,
           "Replace the x and y data arrays.");

  // ── LookupTableVec1D ──────────────────────────────────────────────────

  using LTVMatrixType = LookupTableVec1D::MatrixType;

  nb::class_<LookupTableVec1D>(
      m, "LookupTableVec1D",
      "1-D lookup table mapping scalar x -> vector y.\n\n"
      "Each x value maps to a row of y values.")
      .def(nb::init<>(), "Create an empty vector lookup table.")
      .def(nb::init<Eigen::VectorXd, LTVMatrixType, InterpolationMethod,
                    ExtrapolationMethod>(),
           "x"_a, "y"_a,
           "interpolation"_a = InterpolationMethod::Linear,
           "extrapolation"_a = ExtrapolationMethod::Constant,
           "Create a vector lookup table from x and y (matrix) data.")
      .def(
          "evaluate",
          static_cast<Eigen::VectorXd (LookupTableVec1D::*)(double) const>(
              &LookupTableVec1D::evaluate),
          "x_value"_a,
          "Evaluate the table at a single x value, returning a 1-D array.")
      .def(
          "evaluate",
          static_cast<LTVMatrixType (LookupTableVec1D::*)(
              const Eigen::VectorXd &) const>(&LookupTableVec1D::evaluate),
          "x_values"_a,
          "Evaluate the table at an array of x values, returning a matrix.")
      .def_prop_ro("x", &LookupTableVec1D::x, "Input x data points.")
      .def_prop_ro("y", &LookupTableVec1D::y, "Output y matrix.")
      .def_prop_ro("size", &LookupTableVec1D::size,
                   "Number of x data points.")
      .def_prop_ro("num_values", &LookupTableVec1D::num_values,
                   "Number of y values per x data point (columns).")
      .def_prop_rw("interpolation_method",
                   &LookupTableVec1D::interpolation_method,
                   &LookupTableVec1D::set_interpolation_method,
                   "Interpolation method used between data points.")
      .def_prop_rw("extrapolation_method",
                   &LookupTableVec1D::extrapolation_method,
                   &LookupTableVec1D::set_extrapolation_method,
                   "Extrapolation method used outside the data range.")
      .def("set_data", &LookupTableVec1D::set_data, "x"_a, "y"_a,
           "Replace the x and y data.");

  // ── DenseTimeSeries ───────────────────────────────────────────────────

  nb::class_<DenseTimeSeries>(
      m, "DenseTimeSeries",
      "Dense time series storing times (vector) and values (matrix).\n\n"
      "Each row corresponds to a timestep; each column to a variable.")
      .def(nb::init<>(), "Create an empty dense time series.")
      .def(nb::init<Index, Index>(), "num_timesteps"_a, "num_columns"_a,
           "Create a dense time series with pre-allocated dimensions.")
      .def_prop_ro(
          "times",
          [](DenseTimeSeries &self) -> Eigen::VectorXd & {
            return self.times();
          },
          nb::rv_policy::reference_internal,
          "Time values for each timestep (mutable numpy view).")
      .def_prop_ro(
          "values",
          [](DenseTimeSeries &self) -> DenseTimeSeries::MatrixType & {
            return self.values();
          },
          nb::rv_policy::reference_internal,
          "Data matrix: rows = timesteps, columns = variables (mutable numpy "
          "view).")
      .def("set_row", &DenseTimeSeries::set_row, "row_idx"_a, "time"_a,
           "row_values"_a,
           "Set a single row of the time series (time + values).")
      .def("interpolate", &DenseTimeSeries::interpolate, "query_time"_a,
           "Interpolate values at an arbitrary time.")
      .def_prop_ro("num_timesteps", &DenseTimeSeries::num_timesteps,
                   "Number of timesteps (rows) in the series.")
      .def_prop_ro("num_columns", &DenseTimeSeries::num_columns,
                   "Number of value columns in the series.")
      .def("resize", &DenseTimeSeries::resize, "num_timesteps"_a,
           "num_columns"_a, "Resize the time series to new dimensions.")
      .def("reset", &DenseTimeSeries::reset,
           "Reset the time series to empty state.");

  // ── DenseMatrixTimeSeries ─────────────────────────────────────────────

  nb::class_<DenseMatrixTimeSeries>(
     m, "DenseMatrixTimeSeries",
     "Dense time series storing a matrix at each timestep.")
     .def(nb::init<>(), "Create an empty matrix time series.")
     .def(nb::init<Index, Index>(), "rows"_a, "cols"_a,
         "Create a matrix time series with fixed matrix dimensions.")
     .def("push_back", &DenseMatrixTimeSeries::push_back, "time"_a,
         "matrix"_a, "Append a matrix sample at the given time.")
     .def("reserve", &DenseMatrixTimeSeries::reserve, "n"_a,
         "Reserve storage for n timesteps.")
     .def_prop_ro(
        "times",
        [](const DenseMatrixTimeSeries &self) { return self.times(); },
        "Time values for each stored matrix sample.")
     .def(
        "at",
        [](DenseMatrixTimeSeries &self, Index i)
            -> DenseMatrixTimeSeries::MatrixType & { return self.at(i); },
        nb::rv_policy::reference_internal, "i"_a,
        "Get a mutable reference to the matrix at timestep index i.")
     .def("time_at", &DenseMatrixTimeSeries::time_at, "i"_a,
         "Get the time value at timestep index i.")
     .def("interpolate", &DenseMatrixTimeSeries::interpolate, "query_time"_a,
         "Interpolate the matrix at an arbitrary time.")
     .def_prop_ro("num_timesteps", &DenseMatrixTimeSeries::num_timesteps,
                "Number of stored timesteps.")
     .def_prop_ro("rows", &DenseMatrixTimeSeries::rows,
                "Number of rows in each stored matrix.")
     .def_prop_ro("cols", &DenseMatrixTimeSeries::cols,
                "Number of columns in each stored matrix.");

  // ── SparseTimeSeries ──────────────────────────────────────────────────

  nb::class_<SparseTimeSeries>(
      m, "SparseTimeSeries",
      "Sparse time series storing a sequence of sparse matrices\n"
      "at discrete time points.")
      .def(nb::init<>(), "Create an empty sparse time series.")
      .def("push_back", &SparseTimeSeries::push_back, "time"_a, "matrix"_a,
           "Append a sparse matrix at the given time.")
      .def("reserve", &SparseTimeSeries::reserve, "n"_a,
           "Reserve storage for n timesteps.")
      .def_prop_ro(
          "times",
          [](const SparseTimeSeries &self) { return self.times(); },
          "Time values for each stored matrix (copy).")
      .def(
          "at",
          [](const SparseTimeSeries &self, Index i) { return self.at(i); },
          "i"_a, "Get the sparse matrix at timestep index i (copy).")
      .def("time_at", &SparseTimeSeries::time_at, "i"_a,
           "Get the time value at timestep index i.")
      .def("interpolate", &SparseTimeSeries::interpolate, "query_time"_a,
           "Interpolate the sparse matrices at an arbitrary time.")
      .def_prop_ro("num_timesteps", &SparseTimeSeries::num_timesteps,
                   "Number of stored timesteps.")
      .def_prop_ro("rows", &SparseTimeSeries::rows,
                   "Number of rows in the sparse matrices.")
      .def_prop_ro("cols", &SparseTimeSeries::cols,
                   "Number of columns in the sparse matrices.")
      .def_prop_ro("nnz", &SparseTimeSeries::nnz,
                   "Number of non-zeros in the reference structure.");

  // ── DataModelAttribute enum ───────────────────────────────────────────

  nb::enum_<DataModelAttribute>(m, "DataModelAttribute",
                                "Attributes available in a transient output model.")
      .value("T", DataModelAttribute::T, "Temperature")
      .value("C", DataModelAttribute::C, "Thermal capacity")
      .value("QS", DataModelAttribute::QS, "Solar heat load")
      .value("QA", DataModelAttribute::QA, "Albedo heat load")
      .value("QE", DataModelAttribute::QE, "Earth IR heat load")
      .value("QI", DataModelAttribute::QI, "Internal heat load")
      .value("QR", DataModelAttribute::QR, "Other heat load")
      .value("A", DataModelAttribute::A, "Area")
      .value("APH", DataModelAttribute::APH, "Solar absorptivity")
      .value("EPS", DataModelAttribute::EPS, "IR emissivity")
      .value("FX", DataModelAttribute::FX, "X coordinate")
      .value("FY", DataModelAttribute::FY, "Y coordinate")
      .value("FZ", DataModelAttribute::FZ, "Z coordinate")
      .value("KL", DataModelAttribute::KL, "Conductive coupling matrix history")
      .value("KR", DataModelAttribute::KR, "Radiative coupling matrix history")
      .value("JAC", DataModelAttribute::JAC, "Jacobian matrix history")
      .export_values();

  // ── DataTableStore ────────────────────────────────────────────────────

  nb::class_<DataTableStore>(
      m, "DataTableStore",
      "Named store of vector lookup tables.")
      .def(nb::init<>(), "Create an empty data table store.")
      .def(
          "add_table",
          [](DataTableStore &self, const std::string &name,
             LookupTableVec1D table) -> LookupTableVec1D & {
            return self.add_table(name, std::move(table));
          },
          nb::rv_policy::reference_internal, "name"_a, "table"_a,
          "Add or replace a named LookupTableVec1D.")
      .def(
          "get_table",
          [](DataTableStore &self, const std::string &name) -> LookupTableVec1D & {
            return self.get_table(name);
          },
          nb::rv_policy::reference_internal, "name"_a,
          "Get a mutable reference to a LookupTableVec1D by name.")
      .def("has_table", &DataTableStore::has_table, "name"_a,
           "Check whether a table with the given name exists.")
      .def("remove_table", &DataTableStore::remove_table, "name"_a,
           "Remove a table by name.")
      .def_prop_ro("size", &DataTableStore::size,
                   "Number of stored lookup tables.")
      .def_prop_ro("table_names", &DataTableStore::table_names,
                   "Names of all stored lookup tables.");

  // ── DataModel ─────────────────────────────────────────────────────────

  nb::class_<DataModel>(
      m, "DataModel",
      "Structured transient output model containing dense, sparse, and matrix time series.")
      .def(nb::init<>(), "Create an empty data model.")
      .def(nb::init<std::vector<Index>>(), "node_numbers"_a,
           "Create a data model for the given node-number ordering.")
      .def_prop_rw(
          "node_numbers",
          [](const DataModel &self) { return self.node_numbers(); },
          [](DataModel &self, std::vector<Index> node_numbers) {
            self.set_node_numbers(std::move(node_numbers));
          },
          "Node-number ordering used by the dense and sparse outputs.")
      .def_prop_ro("populated_attributes", &DataModel::populated_attributes,
                   "List the attributes that currently contain data.")
      .def_prop_ro(
          "T", [](DataModel &self) -> DenseTimeSeries & { return self.T(); },
          nb::rv_policy::reference_internal, "Temperature time series.")
      .def_prop_ro(
          "C", [](DataModel &self) -> DenseTimeSeries & { return self.C(); },
          nb::rv_policy::reference_internal, "Thermal capacity time series.")
      .def_prop_ro(
          "QS", [](DataModel &self) -> DenseTimeSeries & { return self.QS(); },
          nb::rv_policy::reference_internal, "Solar heat-load time series.")
      .def_prop_ro(
          "QA", [](DataModel &self) -> DenseTimeSeries & { return self.QA(); },
          nb::rv_policy::reference_internal, "Albedo heat-load time series.")
      .def_prop_ro(
          "QE", [](DataModel &self) -> DenseTimeSeries & { return self.QE(); },
          nb::rv_policy::reference_internal, "Earth IR heat-load time series.")
      .def_prop_ro(
          "QI", [](DataModel &self) -> DenseTimeSeries & { return self.QI(); },
          nb::rv_policy::reference_internal, "Internal heat-load time series.")
      .def_prop_ro(
          "QR", [](DataModel &self) -> DenseTimeSeries & { return self.QR(); },
          nb::rv_policy::reference_internal, "Other heat-load time series.")
      .def_prop_ro(
          "A", [](DataModel &self) -> DenseTimeSeries & { return self.A(); },
          nb::rv_policy::reference_internal, "Area time series.")
      .def_prop_ro(
          "APH", [](DataModel &self) -> DenseTimeSeries & { return self.APH(); },
          nb::rv_policy::reference_internal, "Solar absorptivity time series.")
      .def_prop_ro(
          "EPS", [](DataModel &self) -> DenseTimeSeries & { return self.EPS(); },
          nb::rv_policy::reference_internal, "IR emissivity time series.")
      .def_prop_ro(
          "FX", [](DataModel &self) -> DenseTimeSeries & { return self.FX(); },
          nb::rv_policy::reference_internal, "X-coordinate time series.")
      .def_prop_ro(
          "FY", [](DataModel &self) -> DenseTimeSeries & { return self.FY(); },
          nb::rv_policy::reference_internal, "Y-coordinate time series.")
      .def_prop_ro(
          "FZ", [](DataModel &self) -> DenseTimeSeries & { return self.FZ(); },
          nb::rv_policy::reference_internal, "Z-coordinate time series.")
      .def_prop_ro(
          "conductive_couplings",
          [](DataModel &self) -> SparseTimeSeries & {
            return self.conductive_couplings();
          },
          nb::rv_policy::reference_internal,
          "Conductive coupling matrix history.")
      .def_prop_ro(
          "radiative_couplings",
          [](DataModel &self) -> SparseTimeSeries & {
            return self.radiative_couplings();
          },
          nb::rv_policy::reference_internal,
          "Radiative coupling matrix history.")
      .def_prop_ro(
          "jacobian",
          [](DataModel &self) -> DenseMatrixTimeSeries & {
            return self.jacobian();
          },
          nb::rv_policy::reference_internal,
          "Jacobian matrix history.")
      .def(
          "get_dense_attribute",
          [](DataModel &self, DataModelAttribute attr) -> DenseTimeSeries & {
            return self.get_dense_attribute(attr);
          },
          nb::rv_policy::reference_internal, "attr"_a,
          "Get a dense attribute time series by enum value.")
      .def(
          "get_sparse_attribute",
          [](DataModel &self, DataModelAttribute attr) -> SparseTimeSeries & {
            return self.get_sparse_attribute(attr);
          },
          nb::rv_policy::reference_internal, "attr"_a,
          "Get a sparse attribute time series by enum value.")
      .def(
          "get_matrix_attribute",
          [](DataModel &self, DataModelAttribute attr) -> DenseMatrixTimeSeries & {
            return self.get_matrix_attribute(attr);
          },
          nb::rv_policy::reference_internal, "attr"_a,
          "Get a matrix attribute time series by enum value.")
      .def(
          "flow_conductive",
          static_cast<Eigen::MatrixXd (DataModel::*)(Index, Index) const>(
              &DataModel::flow_conductive),
          "node_num_1"_a, "node_num_2"_a,
          "Return conductive flow history between two nodes.")
      .def(
          "flow_conductive",
          static_cast<Eigen::MatrixXd (DataModel::*)(const std::vector<Index> &,
                                                     const std::vector<Index> &) const>(
              &DataModel::flow_conductive),
          "node_nums_1"_a, "node_nums_2"_a,
          "Return conductive flow history between two node groups.")
      .def(
          "flow_conductive",
          static_cast<Eigen::MatrixXd (DataModel::*)(Index, Index, double) const>(
              &DataModel::flow_conductive),
          "node_num_1"_a, "node_num_2"_a, "time"_a,
          "Return conductive flow at a single time between two nodes.")
      .def(
          "flow_conductive",
          static_cast<Eigen::MatrixXd (DataModel::*)(const std::vector<Index> &,
                                                     const std::vector<Index> &,
                                                     double) const>(
              &DataModel::flow_conductive),
          "node_nums_1"_a, "node_nums_2"_a, "time"_a,
          "Return conductive flow at a single time between two node groups.")
      .def(
          "flow_conductive",
          static_cast<Eigen::MatrixXd (DataModel::*)(Index, Index, double,
                                                     double) const>(
              &DataModel::flow_conductive),
          "node_num_1"_a, "node_num_2"_a, "start_time"_a, "end_time"_a,
          "Return conductive flow over a time range between two nodes.")
      .def(
          "flow_conductive",
          static_cast<Eigen::MatrixXd (DataModel::*)(const std::vector<Index> &,
                                                     const std::vector<Index> &,
                                                     double, double) const>(
              &DataModel::flow_conductive),
          "node_nums_1"_a, "node_nums_2"_a, "start_time"_a, "end_time"_a,
          "Return conductive flow over a time range between two node groups.")
      .def(
          "flow_radiative",
          static_cast<Eigen::MatrixXd (DataModel::*)(Index, Index) const>(
              &DataModel::flow_radiative),
          "node_num_1"_a, "node_num_2"_a,
          "Return radiative flow history between two nodes.")
      .def(
          "flow_radiative",
          static_cast<Eigen::MatrixXd (DataModel::*)(const std::vector<Index> &,
                                                     const std::vector<Index> &) const>(
              &DataModel::flow_radiative),
          "node_nums_1"_a, "node_nums_2"_a,
          "Return radiative flow history between two node groups.")
      .def(
          "flow_radiative",
          static_cast<Eigen::MatrixXd (DataModel::*)(Index, Index, double) const>(
              &DataModel::flow_radiative),
          "node_num_1"_a, "node_num_2"_a, "time"_a,
          "Return radiative flow at a single time between two nodes.")
      .def(
          "flow_radiative",
          static_cast<Eigen::MatrixXd (DataModel::*)(const std::vector<Index> &,
                                                     const std::vector<Index> &,
                                                     double) const>(
              &DataModel::flow_radiative),
          "node_nums_1"_a, "node_nums_2"_a, "time"_a,
          "Return radiative flow at a single time between two node groups.")
      .def(
          "flow_radiative",
          static_cast<Eigen::MatrixXd (DataModel::*)(Index, Index, double,
                                                     double) const>(
              &DataModel::flow_radiative),
          "node_num_1"_a, "node_num_2"_a, "start_time"_a, "end_time"_a,
          "Return radiative flow over a time range between two nodes.")
      .def(
          "flow_radiative",
          static_cast<Eigen::MatrixXd (DataModel::*)(const std::vector<Index> &,
                                                     const std::vector<Index> &,
                                                     double, double) const>(
              &DataModel::flow_radiative),
          "node_nums_1"_a, "node_nums_2"_a, "start_time"_a, "end_time"_a,
          "Return radiative flow over a time range between two node groups.");

  // ── DataModelStore ────────────────────────────────────────────────────

  nb::class_<DataModelStore>(
      m, "DataModelStore",
      "Named store of transient data models.")
      .def(nb::init<>(), "Create an empty data model store.")
      .def(
          "add_model",
          [](DataModelStore &self, const std::string &name,
             DataModel model) -> DataModel & {
            return self.add_model(name, std::move(model));
          },
          nb::rv_policy::reference_internal, "name"_a, "model"_a,
          "Add or replace a named DataModel.")
      .def(
          "get_model",
          [](DataModelStore &self, const std::string &name) -> DataModel & {
            return self.get_model(name);
          },
          nb::rv_policy::reference_internal, "name"_a,
          "Get a mutable reference to a DataModel by name.")
      .def("has_model", &DataModelStore::has_model, "name"_a,
           "Check whether a model with the given name exists.")
      .def("remove_model", &DataModelStore::remove_model, "name"_a,
           "Remove a model by name.")
      .def_prop_ro("size", &DataModelStore::size,
                   "Number of stored output models.")
      .def_prop_ro("model_names", &DataModelStore::model_names,
                   "Names of all stored output models.");

  // ── TimeVariable ──────────────────────────────────────────────────────

  nb::class_<TimeVariable>(
      m, "TimeVariable",
      "A parameter whose value is driven by a lookup table of time.\n\n"
      "Created via ThermalMathematicalModel.add_time_variable().")
      .def_prop_ro("name", &TimeVariable::name, "Variable name.")
      .def_prop_ro(
          "lookup_table",
          [](const TimeVariable &self) -> const LookupTable1D & {
            return self.lookup_table();
          },
          nb::rv_policy::reference_internal,
          "The underlying LookupTable1D.")
      .def_prop_ro("current_value", &TimeVariable::current_value,
                   "Current interpolated value.");

  // ── TemperatureVariable ───────────────────────────────────────────────

  nb::class_<TemperatureVariable>(
      m, "TemperatureVariable",
      "A variable evaluated as a function of temperature via a\n"
      "lookup table.\n\n"
      "Created via ThermalMathematicalModel.add_temperature_variable().")
      .def(nb::init<std::string, LookupTable1D>(), "name"_a,
           "lookup_table"_a,
           "Create a temperature variable with the given lookup table.")
      .def("evaluate", &TemperatureVariable::evaluate, "temperature"_a,
           "Evaluate the variable at a given temperature.")
      .def_prop_ro("name", &TemperatureVariable::name, "Variable name.")
      .def_prop_ro(
          "lookup_table",
          [](const TemperatureVariable &self) -> const LookupTable1D & {
            return self.lookup_table();
          },
          nb::rv_policy::reference_internal,
          "The underlying LookupTable1D.");

  // ── read_tmd_transient free function ──────────────────────────────────

  using pycanha::ThermalData;

  m.def(
      "read_tmd_transient",
      [](const std::string &filepath, ThermalData &thermal_data,
         const std::string &model_name, bool overwrite,
         const std::vector<DataModelAttribute> &attributes) {
        return pycanha::read_tmd_transient(filepath, thermal_data,
                                            model_name, overwrite,
                                            attributes);
      },
      "filepath"_a, "thermal_data"_a, "model_name"_a,
      "overwrite"_a = false,
      "attributes"_a =
          std::vector<DataModelAttribute>{
              DataModelAttribute::T, DataModelAttribute::C,
              DataModelAttribute::QA, DataModelAttribute::QE,
              DataModelAttribute::QI, DataModelAttribute::QR,
              DataModelAttribute::QS},
      "Read ESATAN TMD transient results into a named DataModel.\n\n"
      "Returns a list of node numbers found in the file.");
}

} // namespace pycanha::bindings::thermaldata
