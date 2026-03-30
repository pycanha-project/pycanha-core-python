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

  nb::class_<Parameters>(m, "Parameters")
      .def(nb::init<>())
      .def(
          "add_parameter",
          [to_thermal_value](Parameters &self, const std::string &name,
                             const nb::object &value) {
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
                             const nb::object &value) {
            self.set_parameter(name, to_thermal_value(value));
          },
          "name"_a, "value"_a)
      .def("contains", &Parameters::contains, "name"_a)
      .def("size", &Parameters::size)
      .def("get_memory_address", &Parameters::get_memory_address, "name"_a)
      .def("get_idx", &Parameters::get_idx, "name"_a)
      .def("get_size_of_parameter", &Parameters::get_size_of_parameter,
           "name"_a)
      .def_prop_ro("data", [to_python](Parameters &self) {
        nb::dict result;
        for (const auto &[key, value] : self.data()) {
          result[nb::cast(key)] = to_python(value);
        }
        return result;
      });
}

inline void register_entities(nb::module_ &m) {
  using pycanha::AttributeEntity;
  using pycanha::ConductiveCouplingEntity;
  using pycanha::RadiativeCouplingEntity;
  using pycanha::ThermalEntity;
  using pycanha::ThermalNetwork;

  nb::class_<ThermalEntity>(m, "ThermalEntity")
      .def_prop_ro("type", &ThermalEntity::type,
                   nb::rv_policy::reference_internal)
      .def_prop_ro("node_index_1", &ThermalEntity::node_index_1)
      .def_prop_ro("node_index_2", &ThermalEntity::node_index_2)
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

  nb::class_<AttributeEntity, ThermalEntity>(m, "AttributeEntity")
      .def(nb::init<ThermalNetwork &, std::string, int>(), "network"_a,
           "attribute"_a, "node"_a, nb::keep_alive<1, 2>())
      .def("get_value", &AttributeEntity::get_value)
      .def("set_value", &AttributeEntity::set_value, "value"_a);

  nb::class_<ConductiveCouplingEntity, ThermalEntity>(
      m, "ConductiveCouplingEntity")
      .def(nb::init<ThermalNetwork &, int, int>(), "network"_a, "node_1"_a,
           "node_2"_a, nb::keep_alive<1, 2>())
      .def("get_value", &ConductiveCouplingEntity::get_value)
      .def("set_value", &ConductiveCouplingEntity::set_value, "value"_a);

  nb::class_<RadiativeCouplingEntity, ThermalEntity>(m,
                                                     "RadiativeCouplingEntity")
      .def(nb::init<ThermalNetwork &, int, int>(), "network"_a, "node_1"_a,
           "node_2"_a, nb::keep_alive<1, 2>())
      .def("get_value", &RadiativeCouplingEntity::get_value)
      .def("set_value", &RadiativeCouplingEntity::set_value, "value"_a);
}

inline void register_formulas(nb::module_ &m) {
  using pycanha::Formula;
  using pycanha::Formulas;
  using pycanha::ParameterFormula;
  using pycanha::Parameters;
  using pycanha::ThermalEntity;
  using pycanha::ThermalNetwork;
  using pycanha::ValueFormula;

  nb::class_<Formula>(m, "Formula")
      .def("compile_formula", &Formula::compile_formula)
      .def("apply_formula", &Formula::apply_formula)
      .def("apply_compiled_formula", &Formula::apply_compiled_formula)
      .def("get_value", &Formula::get_value)
      .def("get_derivative_values",
           [](Formula &self) -> nb::object {
             auto *values = self.get_derivative_values();
             if (values == nullptr) {
               return nb::none();
             }
             return nb::cast(*values);
           })
      .def_prop_ro(
          "entity",
          [](Formula &self) -> ThermalEntity & { return self.entity(); },
          nb::rv_policy::reference_internal)
      .def_prop_ro(
          "parameter_dependencies",
          [](const Formula &self) { return self.parameter_dependencies(); })
      .def("clone", [](const Formula &self) {
        auto cloned = self.clone();
        return std::shared_ptr<Formula>(std::move(cloned));
      });

  nb::class_<ParameterFormula, Formula>(m, "ParameterFormula")
      .def(nb::init<ThermalEntity &, Parameters &, std::string>(), "entity"_a,
           "parameters"_a, "parameter"_a, nb::keep_alive<1, 2>(),
           nb::keep_alive<1, 3>())
      .def(
          nb::init<std::shared_ptr<ThermalEntity>, Parameters &, std::string>(),
          "entity"_a, "parameters"_a, "parameter"_a, nb::keep_alive<1, 2>(),
          nb::keep_alive<1, 3>())
      .def("compile_formula", &ParameterFormula::compile_formula)
      .def("apply_formula", &ParameterFormula::apply_formula)
      .def("apply_compiled_formula", &ParameterFormula::apply_compiled_formula)
      .def("get_value", &ParameterFormula::get_value)
      .def("get_derivative_values", [](ParameterFormula &self) -> nb::object {
        auto *values = self.get_derivative_values();
        if (values == nullptr) {
          return nb::none();
        }
        return nb::cast(*values);
      });

  nb::class_<ValueFormula, Formula>(m, "ValueFormula")
      .def(nb::init<ThermalEntity &>(), "entity"_a, nb::keep_alive<1, 2>())
      .def(nb::init<std::shared_ptr<ThermalEntity>>(), "entity"_a,
           nb::keep_alive<1, 2>())
      .def("compile_formula", &ValueFormula::compile_formula)
      .def("apply_formula", &ValueFormula::apply_formula)
      .def("apply_compiled_formula", &ValueFormula::apply_compiled_formula)
      .def("get_value", &ValueFormula::get_value)
      .def("get_derivative_values",
           [](ValueFormula &self) -> nb::object {
             auto *values = self.get_derivative_values();
             if (values == nullptr) {
               return nb::none();
             }
             return nb::cast(*values);
           })
      .def("set_value", &ValueFormula::set_value, "value"_a);

  nb::class_<Formulas>(m, "Formulas")
      .def(nb::init<>())
      .def(nb::init<std::shared_ptr<ThermalNetwork>,
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
      .def_prop_ro("formulas",
                   [](const Formulas &self) {
                     nb::list collection;
                     for (const auto &formula : self.formulas()) {
                       collection.append(formula);
                     }
                     return collection;
                   })
      .def_prop_ro("parameter_dependencies", [](const Formulas &self) {
        nb::dict dependencies;
        for (const auto &[name, formulas] : self.parameter_dependencies()) {
          nb::list formula_list;
          for (const auto &formula : formulas) {
            formula_list.append(formula);
          }
          dependencies[nb::cast(name)] = std::move(formula_list);
        }
        return dependencies;
      });
}

} // namespace pycanha::bindings::parameters
