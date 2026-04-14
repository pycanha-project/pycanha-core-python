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
#include "pycanha-core/thermaldata/dense_time_series.hpp"
#include "pycanha-core/thermaldata/lookup_table.hpp"
#include "pycanha-core/thermaldata/sparse_time_series.hpp"
#include "pycanha-core/thermaldata/thermaldata.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::thermaldata {

inline void register_thermaldata_types(nb::module_ &m) {
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

  // ── TMDNodeAttribute enum ────────────────────────────────────────────

  using pycanha::TMDNodeAttribute;

  nb::enum_<TMDNodeAttribute>(m, "TMDNodeAttribute",
                              "ESATAN TMD node attributes for transient "
                              "result import.")
      .value("T", TMDNodeAttribute::T, "Temperature")
      .value("C", TMDNodeAttribute::C, "Thermal capacity")
      .value("QA", TMDNodeAttribute::QA, "Albedo heat load")
      .value("QE", TMDNodeAttribute::QE, "Earth IR heat load")
      .value("QI", TMDNodeAttribute::QI, "Internal heat load")
      .value("QR", TMDNodeAttribute::QR, "Other heat load")
      .value("QS", TMDNodeAttribute::QS, "Solar heat load")
      .value("A", TMDNodeAttribute::A, "Area")
      .value("APH", TMDNodeAttribute::APH, "Solar absorptivity")
      .value("EPS", TMDNodeAttribute::EPS, "IR emissivity")
      .value("FX", TMDNodeAttribute::FX, "X coordinate")
      .value("FY", TMDNodeAttribute::FY, "Y coordinate")
      .value("FZ", TMDNodeAttribute::FZ, "Z coordinate")
      .export_values();

  // ── read_tmd_transient free function ──────────────────────────────────

  using pycanha::ThermalData;

  m.def(
      "read_tmd_transient",
      [](const std::string &filepath, ThermalData &thermal_data,
         const std::string &table_prefix, bool overwrite,
         const std::vector<TMDNodeAttribute> &attributes) {
        return pycanha::read_tmd_transient(filepath, thermal_data,
                                            table_prefix, overwrite,
                                            attributes);
      },
      "filepath"_a, "thermal_data"_a, "table_prefix"_a,
      "overwrite"_a = false,
      "attributes"_a =
          std::vector<TMDNodeAttribute>{
              TMDNodeAttribute::T, TMDNodeAttribute::C, TMDNodeAttribute::QA,
              TMDNodeAttribute::QE, TMDNodeAttribute::QI, TMDNodeAttribute::QR,
              TMDNodeAttribute::QS},
      "Read ESATAN TMD transient results into DenseTimeSeries tables.\n\n"
      "Returns a list of node numbers found in the file.");
}

} // namespace pycanha::bindings::thermaldata
