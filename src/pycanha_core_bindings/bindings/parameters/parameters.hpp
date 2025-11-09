#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include <pybind11/eigen.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "pycanha-core/parameters/entity.hpp"
#include "pycanha-core/parameters/formula.hpp"
#include "pycanha-core/parameters/formulas.hpp"
#include "pycanha-core/parameters/parameters.hpp"
#include "pycanha-core/tmm/thermalnetwork.hpp"

namespace py = pybind11;
using namespace pybind11::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::parameters {

inline void register_parameters(py::module_ &m) {
  using pycanha::Parameters;

  auto to_thermal_value = [](const py::object &value) {
    if (py::isinstance<py::bool_>(value)) {
      return Parameters::ThermalValue(value.cast<bool>());
    }
    if (py::isinstance<py::int_>(value)) {
      return Parameters::ThermalValue(value.cast<std::int64_t>());
    }
    if (py::isinstance<py::float_>(value)) {
      return Parameters::ThermalValue(value.cast<double>());
    }
    if (py::isinstance<py::str>(value)) {
      return Parameters::ThermalValue(value.cast<std::string>());
    }

    if (py::isinstance<py::array>(value)) {
      py::array array = py::reinterpret_borrow<py::array>(value);
      if (py::isinstance<py::array_t<bool>>(array)) {
        return Parameters::ThermalValue(array.cast<Parameters::MatrixRXb>());
      }
      if (py::isinstance<py::array_t<std::int32_t>>(array) ||
          py::isinstance<py::array_t<std::int64_t>>(array)) {
        return Parameters::ThermalValue(array.cast<Parameters::MatrixRXi>());
      }
      return Parameters::ThermalValue(array.cast<Parameters::MatrixRXd>());
    }

    throw py::type_error("Unsupported parameter value type");
  };

  auto to_python = [](const Parameters::ThermalValue &value) {
    return std::visit(
        [](const auto &item) -> py::object { return py::cast(item); }, value);
  };

  py::class_<Parameters, std::shared_ptr<Parameters>>(m, "Parameters")
      .def(py::init<>())
      .def(
          "add_parameter",
          [to_thermal_value](Parameters &self, const std::string &name,
                             const py::object &value) {
            self.add_parameter(name, to_thermal_value(value));
          },
          "name"_a, "value"_a)
      .def("remove_parameter", &Parameters::remove_parameter, "name"_a)
      .def(
          "get_parameter",
          [to_python](Parameters &self, const std::string &name) {
            return to_python(self.get_parameter(name));
          },
          "name"_a)
      .def(
          "set_parameter",
          [to_thermal_value](Parameters &self, const std::string &name,
                             const py::object &value) {
            self.set_parameter(name, to_thermal_value(value));
          },
          "name"_a, "value"_a)
      .def("contains", &Parameters::contains, "name"_a)
      .def("size", &Parameters::size)
      .def("get_memory_address", &Parameters::get_memory_address, "name"_a)
      .def("get_idx", &Parameters::get_idx, "name"_a)
      .def("get_size_of_parameter", &Parameters::get_size_of_parameter,
           "name"_a)
      .def_property_readonly("data", [to_python](Parameters &self) {
        py::dict result;
        for (const auto &[key, value] : self.data()) {
          result[py::cast(key)] = to_python(value);
        }
        return result;
      });
}

inline void register_entities(py::module_ &m) {
  using pycanha::AttributeEntity;
  using pycanha::ConductiveCouplingEntity;
  using pycanha::RadiativeCouplingEntity;
  using pycanha::ThermalEntity;
  using pycanha::ThermalNetwork;

  py::class_<ThermalEntity, std::shared_ptr<ThermalEntity>>(m, "ThermalEntity")
      .def_property_readonly("type", &ThermalEntity::type,
                             py::return_value_policy::reference_internal)
      .def_property_readonly("node_index_1", &ThermalEntity::node_index_1)
      .def_property_readonly("node_index_2", &ThermalEntity::node_index_2)
      .def("string_representation", &ThermalEntity::string_representation)
      .def("get_value", &ThermalEntity::get_value)
      .def("set_value", &ThermalEntity::set_value, "value"_a)
      .def("get_value_pointer",
           [](ThermalEntity &self) {
             return reinterpret_cast<std::uintptr_t>(self.get_value_ref());
           })
      .def("clone", [](const ThermalEntity &self) {
        auto cloned = self.clone();
        return std::shared_ptr<ThermalEntity>(std::move(cloned));
      });

  py::class_<AttributeEntity, ThermalEntity, std::shared_ptr<AttributeEntity>>(
      m, "AttributeEntity")
      .def(py::init<ThermalNetwork &, std::string, int>(), "network"_a,
           "attribute"_a, "node"_a, py::keep_alive<1, 2>())
      .def("get_value", &AttributeEntity::get_value)
      .def("set_value", &AttributeEntity::set_value, "value"_a);

  py::class_<ConductiveCouplingEntity, ThermalEntity,
             std::shared_ptr<ConductiveCouplingEntity>>(
      m, "ConductiveCouplingEntity")
      .def(py::init<ThermalNetwork &, int, int>(), "network"_a, "node_1"_a,
           "node_2"_a, py::keep_alive<1, 2>())
      .def("get_value", &ConductiveCouplingEntity::get_value)
      .def("set_value", &ConductiveCouplingEntity::set_value, "value"_a);

  py::class_<RadiativeCouplingEntity, ThermalEntity,
             std::shared_ptr<RadiativeCouplingEntity>>(
      m, "RadiativeCouplingEntity")
      .def(py::init<ThermalNetwork &, int, int>(), "network"_a, "node_1"_a,
           "node_2"_a, py::keep_alive<1, 2>())
      .def("get_value", &RadiativeCouplingEntity::get_value)
      .def("set_value", &RadiativeCouplingEntity::set_value, "value"_a);
}

inline void register_formulas(py::module_ &m) {
  using pycanha::Formula;
  using pycanha::Formulas;
  using pycanha::ParameterFormula;
  using pycanha::Parameters;
  using pycanha::ThermalEntity;
  using pycanha::ThermalNetwork;
  using pycanha::ValueFormula;

  py::class_<Formula, std::shared_ptr<Formula>>(m, "Formula")
      .def("compile_formula", &Formula::compile_formula)
      .def("apply_formula", &Formula::apply_formula)
      .def("apply_compiled_formula", &Formula::apply_compiled_formula)
      .def("get_value", &Formula::get_value)
      .def("get_derivative_values",
           [](Formula &self) -> py::object {
             auto *values = self.get_derivative_values();
             if (values == nullptr) {
               return py::none();
             }
             return py::cast(*values);
           })
      .def_property_readonly(
          "entity",
          [](Formula &self) -> ThermalEntity & { return self.entity(); },
          py::return_value_policy::reference_internal)
      .def_property_readonly(
          "parameter_dependencies",
          [](const Formula &self) { return self.parameter_dependencies(); })
      .def("clone", [](const Formula &self) {
        auto cloned = self.clone();
        return std::shared_ptr<Formula>(std::move(cloned));
      });

  py::class_<ParameterFormula, Formula, std::shared_ptr<ParameterFormula>>(
      m, "ParameterFormula")
      .def(py::init<ThermalEntity &, Parameters &, std::string>(), "entity"_a,
           "parameters"_a, "parameter"_a, py::keep_alive<1, 2>(),
           py::keep_alive<1, 3>())
      .def(
          py::init<std::shared_ptr<ThermalEntity>, Parameters &, std::string>(),
          "entity"_a, "parameters"_a, "parameter"_a, py::keep_alive<1, 2>(),
          py::keep_alive<1, 3>())
      .def("compile_formula", &ParameterFormula::compile_formula)
      .def("apply_formula", &ParameterFormula::apply_formula)
      .def("apply_compiled_formula", &ParameterFormula::apply_compiled_formula)
      .def("get_value", &ParameterFormula::get_value)
      .def("get_derivative_values", [](ParameterFormula &self) -> py::object {
        auto *values = self.get_derivative_values();
        if (values == nullptr) {
          return py::none();
        }
        return py::cast(*values);
      });

  py::class_<ValueFormula, Formula, std::shared_ptr<ValueFormula>>(
      m, "ValueFormula")
      .def(py::init<ThermalEntity &>(), "entity"_a, py::keep_alive<1, 2>())
      .def(py::init<std::shared_ptr<ThermalEntity>>(), "entity"_a,
           py::keep_alive<1, 2>())
      .def("compile_formula", &ValueFormula::compile_formula)
      .def("apply_formula", &ValueFormula::apply_formula)
      .def("apply_compiled_formula", &ValueFormula::apply_compiled_formula)
      .def("get_value", &ValueFormula::get_value)
      .def("get_derivative_values",
           [](ValueFormula &self) -> py::object {
             auto *values = self.get_derivative_values();
             if (values == nullptr) {
               return py::none();
             }
             return py::cast(*values);
           })
      .def("set_value", &ValueFormula::set_value, "value"_a);

  py::class_<Formulas, std::shared_ptr<Formulas>>(m, "Formulas")
      .def(py::init<>())
      .def(py::init<std::shared_ptr<ThermalNetwork>,
                    std::shared_ptr<Parameters>>(),
           "network"_a, "parameters"_a)
      .def("associate", &Formulas::associate, "network"_a, "parameters"_a)
      .def("create_parameter_formula", &Formulas::create_parameter_formula,
           "entity"_a, "parameter"_a)
      .def("add_formula",
           static_cast<void (Formulas::*)(const Formula &)>(
               &Formulas::add_formula),
           "formula"_a)
      .def("add_formula",
           static_cast<void (Formulas::*)(const std::shared_ptr<Formula> &)>(
               &Formulas::add_formula),
           "formula"_a)
      .def("apply_formulas", &Formulas::apply_formulas)
      .def_property_readonly("formulas",
                             [](const Formulas &self) {
                               py::list collection;
                               for (const auto &formula : self.formulas()) {
                                 collection.append(formula);
                               }
                               return collection;
                             })
      .def_property_readonly(
          "parameter_dependencies", [](const Formulas &self) {
            py::dict dependencies;
            for (const auto &[name, formulas] : self.parameter_dependencies()) {
              py::list formula_list;
              for (const auto &formula : formulas) {
                formula_list.append(formula);
              }
              dependencies[py::cast(name)] = std::move(formula_list);
            }
            return dependencies;
          });
}

} // namespace pycanha::bindings::parameters
