#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/ndarray.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/string_view.h>
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
      .def("rename_parameter", &Parameters::rename_parameter, "current_name"_a,
           "new_name"_a, "Rename a parameter.")
      .def("contains", &Parameters::contains, "name"_a,
           "Check whether a parameter with the given name exists.")
      .def("size", &Parameters::size, "Return the number of stored parameters.")
      .def("get_memory_address", &Parameters::get_memory_address, "name"_a,
           "Memory address (as int) of a parameter value for formula binding.")
      .def("get_idx", &Parameters::get_idx, "name"_a,
           "Get the internal index of a parameter by name, or None.")
      .def("get_size_of_parameter", &Parameters::get_size_of_parameter,
           "name"_a, "Get the byte size of a parameter's stored value.")
      .def("is_internal_parameter", &Parameters::is_internal_parameter,
           "name"_a, "Check whether a parameter is marked as internal.")
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
  using pycanha::Entity;
  using pycanha::EntityType;
  using pycanha::NodeNum;
  using pycanha::ThermalNetwork;

  nb::enum_<EntityType>(m, "EntityType",
                        "Type of thermal entity in the network.")
      .value("T", EntityType::t, "Temperature")
      .value("C", EntityType::c, "Thermal capacity")
      .value("QS", EntityType::qs, "Solar heat load")
      .value("QA", EntityType::qa, "Albedo heat load")
      .value("QE", EntityType::qe, "Earth IR heat load")
      .value("QI", EntityType::qi, "Internal heat load")
      .value("QR", EntityType::qr, "Other heat load")
      .value("GL", EntityType::gl, "Conductive coupling")
      .value("GR", EntityType::gr, "Radiative coupling")
      .export_values();

  nb::class_<Entity>(
      m, "Entity",
      "Reference to a value in the thermal network.\n\n"
      "An Entity points to a specific node attribute (T, C, QI, etc.)\n"
      "or coupling value (GL, GR) in a ThermalNetwork. Use the static\n"
      "factory methods (Entity.make, Entity.t, Entity.gl, etc.) or\n"
      "Entity.from_string to create instances.")
      // ── Static factory methods ──────────────────────────────────────
      .def_static("make", &Entity::make, "network"_a, "type"_a,
                  "node_1"_a = Entity::invalid_node,
                  "node_2"_a = Entity::invalid_node, nb::keep_alive<0, 1>(),
                  "Create an entity from type and node numbers.")
      .def_static(
          "from_string",
          [](ThermalNetwork &network, const std::string &text) {
            return Entity::from_string(network, text);
          },
          "network"_a, "text"_a, nb::keep_alive<0, 1>(),
          "Parse an entity from string (e.g. 'T1', 'GL(1,2)').\n\n"
          "Returns None if the string cannot be parsed.")
      .def_static("from_internal_symbol", &Entity::from_internal_symbol,
                  "network"_a, "symbol"_a, nb::keep_alive<0, 1>(),
                  "Parse an entity from internal symbol format.")
      // Short-name convenience factories
      .def_static("t", &Entity::t, "network"_a, "node"_a,
                  nb::keep_alive<0, 1>(),
                  "Create a temperature entity for a node.")
      .def_static("c", &Entity::c, "network"_a, "node"_a,
                  nb::keep_alive<0, 1>(),
                  "Create a thermal capacity entity for a node.")
      .def_static("qs", &Entity::qs, "network"_a, "node"_a,
                  nb::keep_alive<0, 1>(),
                  "Create a solar heat load entity for a node.")
      .def_static("qa", &Entity::qa, "network"_a, "node"_a,
                  nb::keep_alive<0, 1>(),
                  "Create an albedo heat load entity for a node.")
      .def_static("qe", &Entity::qe, "network"_a, "node"_a,
                  nb::keep_alive<0, 1>(),
                  "Create an Earth IR heat load entity for a node.")
      .def_static("qi", &Entity::qi, "network"_a, "node"_a,
                  nb::keep_alive<0, 1>(),
                  "Create an internal heat load entity for a node.")
      .def_static("qr", &Entity::qr, "network"_a, "node"_a,
                  nb::keep_alive<0, 1>(),
                  "Create an other heat load entity for a node.")
      .def_static("gl", &Entity::gl, "network"_a, "node_1"_a, "node_2"_a,
                  nb::keep_alive<0, 1>(),
                  "Create a conductive coupling entity.")
      .def_static("gr", &Entity::gr, "network"_a, "node_1"_a, "node_2"_a,
                  nb::keep_alive<0, 1>(), "Create a radiative coupling entity.")
      // Long-name convenience factories
      .def_static("temperature", &Entity::temperature, "network"_a, "node"_a,
                  nb::keep_alive<0, 1>(), "Alias for Entity.t().")
      .def_static("capacity", &Entity::capacity, "network"_a, "node"_a,
                  nb::keep_alive<0, 1>(), "Alias for Entity.c().")
      .def_static("solar_heat", &Entity::solar_heat, "network"_a, "node"_a,
                  nb::keep_alive<0, 1>(), "Alias for Entity.qs().")
      .def_static("albedo_heat", &Entity::albedo_heat, "network"_a, "node"_a,
                  nb::keep_alive<0, 1>(), "Alias for Entity.qa().")
      .def_static("earth_ir", &Entity::earth_ir, "network"_a, "node"_a,
                  nb::keep_alive<0, 1>(), "Alias for Entity.qe().")
      .def_static("internal_heat", &Entity::internal_heat, "network"_a,
                  "node"_a, nb::keep_alive<0, 1>(), "Alias for Entity.qi().")
      .def_static("other_heat", &Entity::other_heat, "network"_a, "node"_a,
                  nb::keep_alive<0, 1>(), "Alias for Entity.qr().")
      .def_static("conductive", &Entity::conductive, "network"_a, "node_1"_a,
                  "node_2"_a, nb::keep_alive<0, 1>(), "Alias for Entity.gl().")
      .def_static("radiative", &Entity::radiative, "network"_a, "node_1"_a,
                  "node_2"_a, nb::keep_alive<0, 1>(), "Alias for Entity.gr().")
      // ── Instance methods ────────────────────────────────────────────
      .def_prop_ro("type", &Entity::type,
                   "Entity type (EntityType enum value).")
      .def_prop_ro(
          "token", [](const Entity &self) { return std::string(self.token()); },
          "Entity type token string (e.g. 'T', 'GL').")
      .def_prop_ro("node_1", &Entity::node_1, "First node number.")
      .def_prop_ro("node_2", &Entity::node_2,
                   "Second node number (-1 for single-node entities).")
      .def_prop_ro("node_count", &Entity::node_count,
                   "Number of nodes this entity type references (0, 1, or 2).")
      .def_prop_ro("writable", &Entity::writable,
                   "Whether the entity's value can be set.")
      .def("exists", &Entity::exists,
           "Check if the referenced node/coupling exists in the network.")
      .def("get_value", &Entity::get_value,
           "Get the current value (NaN if invalid).")
      .def("set_value", &Entity::set_value, "value"_a,
           "Set the value. Returns True on success, False on failure.")
      .def("string_representation", &Entity::string_representation,
           "Human-readable string, e.g. 'T1', 'GL(1,2)'.")
      .def("internal_symbol_name", &Entity::internal_symbol_name,
           "Internal symbol name for formula preprocessing.")
      .def("is_same_as", &Entity::is_same_as, "other"_a,
           "Check if this entity references the same value as another.")
      .def(
          "get_value_pointer",
          [](Entity &self) {
            return reinterpret_cast<std::uintptr_t>(self.get_value_ref());
          },
          "Memory address of the referenced value for formula binding.")
      .def("__eq__", &Entity::operator==, "other"_a)
      .def("__repr__", [](const Entity &self) {
        return "<Entity " + self.string_representation() + ">";
      });
}

inline void register_formulas(nb::module_ &m) {
  using pycanha::DerivativeParameterRegistry;
  using pycanha::Entity;
  using pycanha::ExpressionFormula;
  using pycanha::Formula;
  using pycanha::Formulas;
  using pycanha::ParameterFormula;
  using pycanha::Parameters;
  using pycanha::ThermalNetwork;
  using pycanha::ValueFormula;

  nb::class_<DerivativeParameterRegistry>(
      m, "DerivativeParameterRegistry",
      "Ordered registry of parameters used for derivative and Jacobian output.")
      .def(nb::init<>(), "Create an empty derivative-parameter registry.")
      .def(nb::init<std::shared_ptr<Parameters>>(), "parameters"_a,
           "Create a derivative-parameter registry associated with a "
           "Parameters store.")
      .def("associate", &DerivativeParameterRegistry::associate, "parameters"_a,
           "Associate this registry with a Parameters store.")
      .def("add_parameter", &DerivativeParameterRegistry::add_parameter,
           "parameter_name"_a,
           "Append a parameter to the ordered derivative subset.")
      .def("remove_parameter", &DerivativeParameterRegistry::remove_parameter,
           "parameter_name"_a, "Remove a parameter from the derivative subset.")
      .def("contains", &DerivativeParameterRegistry::contains,
           "parameter_name"_a,
           "Return whether the parameter is in the derivative subset.")
      .def_prop_ro("parameter_names",
                   &DerivativeParameterRegistry::parameter_names,
                   "Ordered derivative-parameter names.");

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
          "entity", [](Formula &self) -> Entity & { return self.entity(); },
          nb::rv_policy::reference_internal, "Reference to the target Entity.")
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
      .def(nb::init<Entity, Parameters &, std::string>(), "entity"_a,
           "parameters"_a, "parameter"_a, nb::keep_alive<1, 3>(),
           "Create a formula linking entity to a named parameter.")
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
           "Get derivative values for sensitivity analysis, or None.")
      .def_prop_ro("expression", &ParameterFormula::expression,
                   "Original parameter expression string.");

  nb::class_<ValueFormula, Formula>(
      m, "ValueFormula",
      "Formula that assigns a fixed constant value to an entity.")
      .def(nb::init<Entity>(), "entity"_a,
           "Create a value formula for the given entity.")
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
      "Formula that evaluates a scalar expression of named parameters\n"
      "and/or entity values.")
      .def(nb::init<Entity, Parameters &, std::string>(), "entity"_a,
           "parameters"_a, "expression"_a, nb::keep_alive<1, 3>(),
           "Create an expression formula for the given entity.")
      .def(
          "__init__",
          [](ExpressionFormula *self, Entity entity, Parameters &parameters,
             std::string expression, ThermalNetwork &network) {
            new (self) ExpressionFormula(std::move(entity), parameters,
                                         std::move(expression), &network);
          },
          "entity"_a, "parameters"_a, "expression"_a, "network"_a,
          nb::keep_alive<1, 3>(), nb::keep_alive<1, 5>(),
          "Create an expression formula with entity references via network.")
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
      .def("create_formula", &Formulas::create_formula, "entity"_a,
           "formula_string"_a,
           "Create a formula by auto-detecting its type from the string.\n\n"
           "Returns a shared pointer to the created Formula.")
      .def("add_value_formula",
           static_cast<ValueFormula &(Formulas::*)(Entity, double)>(
               &Formulas::add_value_formula),
           nb::rv_policy::reference_internal, "entity"_a, "value"_a,
           "Add a ValueFormula bound to an Entity target.")
      .def("add_value_formula",
           static_cast<ValueFormula &(Formulas::*)(std::string_view, double)>(
               &Formulas::add_value_formula),
           nb::rv_policy::reference_internal, "entity"_a, "value"_a,
           "Add a ValueFormula bound to an entity string target.")
      .def("add_parameter_formula",
           static_cast<ParameterFormula &(Formulas::*)(Entity,
                                                       const std::string &)>(
               &Formulas::add_parameter_formula),
           nb::rv_policy::reference_internal, "entity"_a, "expression"_a,
           "Add a ParameterFormula bound to an Entity target.")
      .def("add_parameter_formula",
           static_cast<ParameterFormula &(Formulas::*)(std::string_view,
                                                       const std::string &)>(
               &Formulas::add_parameter_formula),
           nb::rv_policy::reference_internal, "entity"_a, "expression"_a,
           "Add a ParameterFormula bound to an entity string target.")
      .def("add_expression_formula",
           static_cast<ExpressionFormula &(Formulas::*)(Entity,
                                                        const std::string &)>(
               &Formulas::add_expression_formula),
           nb::rv_policy::reference_internal, "entity"_a, "expression"_a,
           "Add an ExpressionFormula bound to an Entity target.")
      .def("add_expression_formula",
           static_cast<ExpressionFormula &(Formulas::*)(std::string_view,
                                                        const std::string &)>(
               &Formulas::add_expression_formula),
           nb::rv_policy::reference_internal, "entity"_a, "expression"_a,
           "Add an ExpressionFormula bound to an entity string target.")
      .def("add_formula",
           static_cast<Formula &(Formulas::*)(Entity, double)>(
               &Formulas::add_formula),
           nb::rv_policy::reference_internal, "entity"_a, "value"_a,
           "Add a formula for an Entity target from a numeric value.")
      .def("add_formula",
           static_cast<Formula &(Formulas::*)(std::string_view, double)>(
               &Formulas::add_formula),
           nb::rv_policy::reference_internal, "entity"_a, "value"_a,
           "Add a formula for an entity string target from a numeric value.")
      .def("add_formula",
           static_cast<Formula &(Formulas::*)(Entity, const std::string &)>(
               &Formulas::add_formula),
           nb::rv_policy::reference_internal, "entity"_a, "formula_string"_a,
           "Add a formula for an Entity target from a string expression.")
      .def(
          "add_formula",
          static_cast<Formula &(Formulas::*)(std::string_view,
                                             const std::string &)>(
              &Formulas::add_formula),
          nb::rv_policy::reference_internal, "entity"_a, "formula_string"_a,
          "Add a formula for an entity string target from a string expression.")
      .def("add_formula",
           static_cast<void (Formulas::*)(const Formula &)>(
               &Formulas::add_formula),
           "formula"_a, "Add a formula (by copy) to the collection.")
      .def("add_formula",
           static_cast<void (Formulas::*)(const std::shared_ptr<Formula> &)>(
               &Formulas::add_formula),
           "formula"_a, "Add a formula (by shared pointer) to the collection.")
      .def("remove_formula",
           static_cast<bool (Formulas::*)(const Entity &) noexcept>(
               &Formulas::remove_formula),
           "entity"_a, "Remove a formula by Entity target.")
      .def("remove_formula",
           static_cast<bool (Formulas::*)(std::string_view) noexcept>(
               &Formulas::remove_formula),
           "entity"_a, "Remove a formula by entity string target.")
      .def("validate_for_execution", &Formulas::validate_for_execution,
           "Validate all formulas can be evaluated.")
      .def("compile_formulas", &Formulas::compile_formulas,
           "Compile all formulas for fast application.")
      .def("apply_formulas", &Formulas::apply_formulas,
           "Apply all formulas, propagating parameter values to the network.")
      .def("apply_compiled_formulas", &Formulas::apply_compiled_formulas,
           "Apply using pre-compiled pointers (faster, requires "
           "compile_formulas()).")
      .def("calculate_derivatives", &Formulas::calculate_derivatives,
           "Calculate derivatives for all stored formulas.")
      .def("lock_parameters_for_execution",
           &Formulas::lock_parameters_for_execution,
           "Lock parameters to prevent structural changes during solve.")
      .def("unlock_parameters", &Formulas::unlock_parameters,
           "Unlock parameters after solve completes.")
      .def("is_validation_current", &Formulas::is_validation_current,
           "Check whether the current validation state is up to date.")
      .def("is_compiled_current", &Formulas::is_compiled_current,
           "Check whether the compiled state is up to date.")
      .def_rw("debug_formulas", &Formulas::debug_formulas,
              "Enable debug logging of formula application.")
      .def_prop_ro(
          "parameters_with_derivatives",
          [](Formulas &self) -> DerivativeParameterRegistry & {
            return self.parameters_with_derivatives();
          },
          nb::rv_policy::reference_internal,
          "Ordered registry of derivative-parameter names.")
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
