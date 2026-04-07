#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

#include "pycanha-core/parameters/entity.hpp"
#include "pycanha-core/parameters/formula.hpp"
#include "pycanha-core/parameters/formulas.hpp"
#include "pycanha-core/parameters/parameters.hpp"
#include "pycanha-core/tmm/thermalnetwork.hpp"

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::parameters {

template <typename FormulaType>
nb::object derivative_values_to_object(FormulaType &self) {
  auto *values = self.get_derivative_values();
  if (values == nullptr) {
    return nb::none();
  }
  return nb::cast(*values);
}

inline void register_parameters(nb::module_ &m) {
  using pycanha::Parameters;

  auto to_thermal_value = [](const nb::object &value) {
    if (nb::isinstance<bool>(value)) {
      return Parameters::ThermalValue(nb::cast<bool>(value));
    }
    if (nb::isinstance<nb::int_>(value)) {
      return Parameters::ThermalValue(nb::cast<std::int64_t>(value));
    }
    if (nb::isinstance<nb::float_>(value)) {
      return Parameters::ThermalValue(nb::cast<double>(value));
    }
    if (nb::isinstance<nb::str>(value)) {
      return Parameters::ThermalValue(nb::cast<std::string>(value));
    }

    if (nb::isinstance<nb::ndarray<nb::numpy>>(value)) {
      if (nb::isinstance<nb::ndarray<nb::numpy, bool>>(value)) {
        return Parameters::ThermalValue(nb::cast<Parameters::MatrixRXb>(value));
      }
      if (nb::isinstance<nb::ndarray<nb::numpy, std::int32_t>>(value) ||
          nb::isinstance<nb::ndarray<nb::numpy, std::int64_t>>(value)) {
        return Parameters::ThermalValue(nb::cast<Parameters::MatrixRXi>(value));
      }
      return Parameters::ThermalValue(nb::cast<Parameters::MatrixRXd>(value));
    }

    throw nb::type_error("Unsupported parameter value type");
  };

  auto to_python = [](const Parameters::ThermalValue &value) {
    return std::visit(
        [](const auto &item) -> nb::object { return nb::cast(item); }, value);
  };

  nb::class_<Parameters>(
      m, "Parameters",
      "Named parameter store for parametric studies.\n\n"
      "Stores values of type bool, int, float, str, or numpy\n"
      "array, keyed by string name. Used with Formulas to\n"
      "propagate parameter changes to the thermal network.")
      .def(nb::init<>(), "Create an empty Parameters store.")
      .def(
          "add_parameter",
          [to_thermal_value](Parameters &self, const std::string &name,
                             const nb::object &value) {
            self.add_parameter(name, to_thermal_value(value));
          },
          "name"_a, "value"_a,
          "Add a new named parameter with the given value.")
      .def("remove_parameter", &Parameters::remove_parameter, "name"_a,
           "Remove a parameter by name.")
      .def(
          "get_parameter",
          [to_python](Parameters &self, const std::string &name) {
            return to_python(self.get_parameter(name));
          },
          "name"_a, "Get the current value of a parameter.")
      .def(
          "set_parameter",
          [to_thermal_value](Parameters &self, const std::string &name,
                             const nb::object &value) {
            self.set_parameter(name, to_thermal_value(value));
          },
          "name"_a, "value"_a, "Update the value of an existing parameter.")
      .def("contains", &Parameters::contains, "name"_a,
           "Check whether a parameter with the given name exists.")
      .def("size", &Parameters::size, "Return the number of stored parameters.")
      .def("get_memory_address", &Parameters::get_memory_address, "name"_a,
           "Memory address (as int) of a parameter value for formula binding.")
      .def("get_idx", &Parameters::get_idx, "name"_a,
           "Get the internal index of a parameter by name.")
      .def("get_size_of_parameter", &Parameters::get_size_of_parameter,
           "name"_a, "Get the byte size of a parameter's stored value.")
      .def_prop_ro(
          "data",
          [to_python](Parameters &self) {
            nb::dict result;
            for (const auto &[key, value] : self.data()) {
              result[nb::cast(key)] = to_python(value);
            }
            return result;
          },
          "Dictionary of all parameter name-value pairs.");
}

inline void register_entities(nb::module_ &m) {
  using pycanha::AttributeEntity;
  using pycanha::ConductiveCouplingEntity;
  using pycanha::RadiativeCouplingEntity;
  using pycanha::ThermalEntity;
  using pycanha::ThermalNetwork;

  nb::class_<ThermalEntity>(
      m, "ThermalEntity",
      "Abstract reference to a value in the thermal network.\n\n"
      "Subclasses (AttributeEntity, ConductiveCouplingEntity,\n"
      "RadiativeCouplingEntity) point to specific node\n"
      "attributes or coupling values.")
      .def_prop_ro("type", &ThermalEntity::type,
                   nb::rv_policy::reference_internal,
                   "Entity type string (e.g. 'T', 'C', 'QI', 'GL', 'GR').")
      .def_prop_ro("node_index_1", &ThermalEntity::node_index_1,
                   "First node number.")
      .def_prop_ro("node_index_2", &ThermalEntity::node_index_2,
                   "Second node number (-1 for single-node attributes).")
      .def("string_representation", &ThermalEntity::string_representation,
           "Human-readable string, e.g. 'T1', 'GL(1,2)'.")
      .def("get_value", &ThermalEntity::get_value,
           "Get the current value of the referenced quantity.")
      .def("set_value", &ThermalEntity::set_value, "value"_a,
           "Set the value of the referenced quantity.")
      .def(
          "get_value_pointer",
          [](ThermalEntity &self) {
            return reinterpret_cast<std::uintptr_t>(self.get_value_ref());
          },
          "Memory address of the referenced value for formula binding.")
      .def(
          "clone",
          [](const ThermalEntity &self) {
            auto cloned = self.clone();
            return std::shared_ptr<ThermalEntity>(std::move(cloned));
          },
          "Create an independent copy of this entity.");

  nb::class_<AttributeEntity, ThermalEntity>(
      m, "AttributeEntity",
      "Entity referencing a single-node attribute (T, C, QI, etc.).")
      .def(nb::init<ThermalNetwork &, std::string, int>(), "network"_a,
           "attribute"_a, "node"_a, nb::keep_alive<1, 2>(),
           "Create an entity for the given attribute of a node.")
      .def("get_value", &AttributeEntity::get_value,
           "Get the current attribute value.")
      .def("set_value", &AttributeEntity::set_value, "value"_a,
           "Set the attribute value.");

  nb::class_<ConductiveCouplingEntity, ThermalEntity>(
      m, "ConductiveCouplingEntity",
      "Entity referencing a conductive coupling GL(node_1, node_2).")
      .def(nb::init<ThermalNetwork &, int, int>(), "network"_a, "node_1"_a,
           "node_2"_a, nb::keep_alive<1, 2>(),
           "Create an entity for GL between two nodes.")
      .def("get_value", &ConductiveCouplingEntity::get_value,
           "Get the conductive coupling value [W/K].")
      .def("set_value", &ConductiveCouplingEntity::set_value, "value"_a,
           "Set the conductive coupling value [W/K].");

  nb::class_<RadiativeCouplingEntity, ThermalEntity>(
      m, "RadiativeCouplingEntity",
      "Entity referencing a radiative coupling GR(node_1, node_2).")
      .def(nb::init<ThermalNetwork &, int, int>(), "network"_a, "node_1"_a,
           "node_2"_a, nb::keep_alive<1, 2>(),
           "Create an entity for GR between two nodes.")
      .def("get_value", &RadiativeCouplingEntity::get_value,
           "Get the radiative coupling value [m^2].")
      .def("set_value", &RadiativeCouplingEntity::set_value, "value"_a,
           "Set the radiative coupling value [m^2].");
}

inline void register_formulas(nb::module_ &m) {
  using pycanha::ExpressionFormula;
  using pycanha::Formula;
  using pycanha::Formulas;
  using pycanha::ParameterFormula;
  using pycanha::Parameters;
  using pycanha::ThermalEntity;
  using pycanha::ThermalNetwork;
  using pycanha::ValueFormula;

  nb::class_<Formula>(m, "Formula",
                      "Abstract base class for formulas that bind parameter\n"
                      "values to thermal entities.")
      .def("compile_formula", &Formula::compile_formula,
           "Compile the formula (resolve pointers for fast evaluation).")
      .def("apply_formula", &Formula::apply_formula,
           "Evaluate and write the formula result to the entity.")
      .def("apply_compiled_formula", &Formula::apply_compiled_formula,
           "Evaluate and write using pre-compiled pointers (faster).")
      .def(
          "calculate_derivatives", &Formula::calculate_derivatives,
          "Calculate derivative values with respect to parameter dependencies.")
      .def("get_value", &Formula::get_value,
           "Get the current computed value of the formula.")
      .def("get_derivative_values", &derivative_values_to_object<Formula>,
           "Get derivative values for sensitivity analysis, or None.")
      .def_prop_ro(
          "entity",
          [](Formula &self) -> ThermalEntity & { return self.entity(); },
          nb::rv_policy::reference_internal,
          "Reference to the target ThermalEntity.")
      .def_prop_ro(
          "parameter_dependencies",
          [](const Formula &self) { return self.parameter_dependencies(); },
          "List of parameter names this formula depends on.")
      .def(
          "clone",
          [](const Formula &self) {
            auto cloned = self.clone();
            return std::shared_ptr<Formula>(std::move(cloned));
          },
          "Create an independent copy of this formula.");

  nb::class_<ParameterFormula, Formula>(
      m, "ParameterFormula",
      "Formula that copies a named parameter value to an entity.\n\n"
      "When applied, sets entity value = parameter value.")
      .def(nb::init<ThermalEntity &, Parameters &, std::string>(), "entity"_a,
           "parameters"_a, "parameter"_a, nb::keep_alive<1, 2>(),
           nb::keep_alive<1, 3>(),
           "Create a formula linking entity to a named parameter.")
      .def(
          nb::init<std::shared_ptr<ThermalEntity>, Parameters &, std::string>(),
          "entity"_a, "parameters"_a, "parameter"_a, nb::keep_alive<1, 2>(),
          nb::keep_alive<1, 3>(),
          "Create a formula linking entity (shared_ptr) to a named parameter.")
      .def("compile_formula", &ParameterFormula::compile_formula,
           "Compile the formula (resolve pointers for fast evaluation).")
      .def("apply_formula", &ParameterFormula::apply_formula,
           "Evaluate and write the parameter value to the entity.")
      .def("apply_compiled_formula", &ParameterFormula::apply_compiled_formula,
           "Evaluate and write using pre-compiled pointers (faster).")
      .def("get_value", &ParameterFormula::get_value,
           "Get the current parameter value.")
      .def("get_derivative_values",
           &derivative_values_to_object<ParameterFormula>,
           "Get derivative values for sensitivity analysis, or None.");

  nb::class_<ValueFormula, Formula>(
      m, "ValueFormula",
      "Formula that assigns a fixed constant value to an entity.")
      .def(nb::init<ThermalEntity &>(), "entity"_a, nb::keep_alive<1, 2>(),
           "Create a value formula for the given entity.")
      .def(nb::init<std::shared_ptr<ThermalEntity>>(), "entity"_a,
           nb::keep_alive<1, 2>(),
           "Create a value formula for the given entity (shared_ptr).")
      .def("compile_formula", &ValueFormula::compile_formula,
           "Compile the formula (resolve pointers for fast evaluation).")
      .def("apply_formula", &ValueFormula::apply_formula,
           "Write the constant value to the entity.")
      .def("apply_compiled_formula", &ValueFormula::apply_compiled_formula,
           "Write using pre-compiled pointers (faster).")
      .def("get_value", &ValueFormula::get_value,
           "Get the stored constant value.")
      .def("get_derivative_values", &derivative_values_to_object<ValueFormula>,
           "Get derivative values for sensitivity analysis, or None.")
      .def("set_value", &ValueFormula::set_value, "value"_a,
           "Set the constant value to assign.");

  nb::class_<ExpressionFormula, Formula>(
      m, "ExpressionFormula",
      "Formula that evaluates a scalar expression of named parameters.")
      .def(nb::init<ThermalEntity &, Parameters &, std::string>(), "entity"_a,
           "parameters"_a, "expression"_a, nb::keep_alive<1, 2>(),
           nb::keep_alive<1, 3>(),
           "Create an expression formula for the given entity.")
      .def(
          nb::init<std::shared_ptr<ThermalEntity>, Parameters &, std::string>(),
          "entity"_a, "parameters"_a, "expression"_a, nb::keep_alive<1, 2>(),
          nb::keep_alive<1, 3>(),
          "Create an expression formula for the given entity (shared_ptr).")
      .def("compile_formula", &ExpressionFormula::compile_formula,
           "Compile the expression for fast repeated evaluation.")
      .def("apply_formula", &ExpressionFormula::apply_formula,
           "Evaluate the expression and write the result to the entity.")
      .def("apply_compiled_formula", &ExpressionFormula::apply_compiled_formula,
           "Evaluate the compiled expression and write the result to the "
           "entity.")
      .def("get_value", &ExpressionFormula::get_value,
           "Get the current expression value.")
      .def("get_derivative_values",
           &derivative_values_to_object<ExpressionFormula>,
           "Get derivative values for sensitivity analysis, or None.")
      .def_prop_ro("expression", &ExpressionFormula::expression,
                   "Original expression string.");

  nb::class_<Formulas>(
      m, "Formulas",
      "Collection of formulas linking parameters to thermal\n"
      "entities. Call apply_formulas() to propagate parameter\n"
      "changes to the network.")
      .def(nb::init<>(), "Create an empty Formulas collection.")
      .def(nb::init<std::shared_ptr<ThermalNetwork>,
                    std::shared_ptr<Parameters>>(),
           "network"_a, "parameters"_a,
           "Create a Formulas collection with associated network and "
           "parameters.")
      .def("associate", &Formulas::associate, "network"_a, "parameters"_a,
           "Associate this collection with a network and parameters store.")
      .def("create_parameter_formula", &Formulas::create_parameter_formula,
           "entity"_a, "parameter"_a,
           "Create and add a ParameterFormula for the entity and parameter "
           "name.")
      .def("add_formula",
           static_cast<void (Formulas::*)(const Formula &)>(
               &Formulas::add_formula),
           "formula"_a, "Add a formula (by copy) to the collection.")
      .def("add_formula",
           static_cast<void (Formulas::*)(const std::shared_ptr<Formula> &)>(
               &Formulas::add_formula),
           "formula"_a, "Add a formula (by shared pointer) to the collection.")
      .def("apply_formulas", &Formulas::apply_formulas,
           "Apply all formulas, propagating parameter values to the network.")
      .def("calculate_derivatives", &Formulas::calculate_derivatives,
           "Calculate derivatives for all stored formulas.")
      .def_prop_ro(
          "formulas",
          [](const Formulas &self) {
            nb::list collection;
            for (const auto &formula : self.formulas()) {
              collection.append(formula);
            }
            return collection;
          },
          "List of all stored Formula objects.")
      .def_prop_ro(
          "parameter_dependencies",
          [](const Formulas &self) {
            nb::dict dependencies;
            for (const auto &[name, formulas] : self.parameter_dependencies()) {
              nb::list formula_list;
              for (const auto &formula : formulas) {
                formula_list.append(formula);
              }
              dependencies[nb::cast(name)] = std::move(formula_list);
            }
            return dependencies;
          },
          "Dict mapping parameter names to lists of dependent formulas.");
}

} // namespace pycanha::bindings::parameters
