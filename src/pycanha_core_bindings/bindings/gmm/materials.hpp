#pragma once
#include <nanobind/nanobind.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/string.h>

#include "pycanha-core/gmm/materials/materials.hpp"

namespace nb = nanobind;
using namespace nanobind::literals;
using namespace pycanha::gmm;

// Legacy material types restored in 0.15. BulkMaterial / OpticalMaterial are
// held by shared_ptr inside ThermalMesh, so the same Python object can be
// assigned to several mesh sides and keeps its identity.

inline void Color_b(nb::module_& m) {
  nb::class_<Color>(m, "Color", "A 3-channel (RGB) color, 0-255 per channel.")
      .def(nb::init<std::uint8_t, std::uint8_t, std::uint8_t>(), "red"_a,
           "green"_a, "blue"_a, "Create a color from explicit RGB channels.")
      .def(nb::init<const std::string&>(), "color_name"_a,
           "Create a color from a named palette entry.")
      .def_prop_ro("rgb", &Color::get_rgb, "RGB channels as a (r, g, b) tuple.")
      .def_prop_ro("red", &Color::red, "Red channel [0, 255].")
      .def_prop_ro("green", &Color::green, "Green channel [0, 255].")
      .def_prop_ro("blue", &Color::blue, "Blue channel [0, 255].")
      .def_static("get_rgb_from_color_palette",
                  &Color::get_rgb_from_color_palette, "color_name"_a,
                  "Look up the RGB triple of a named palette color.");
}

inline void BulkMaterial_b(nb::module_& m) {
  nb::class_<BulkMaterial>(
      m, "BulkMaterial",
      "Structural / thermal bulk properties (density, conductivity, specific "
      "heat). All properties must be non-negative.")
      .def(nb::init<>(), "Create an unnamed material with all properties zero.")
      .def(nb::init<std::string, double, double, double>(), "name"_a,
           "density"_a, "conductivity"_a, "specific_heat"_a,
           "Create a bulk material from its properties.")
      .def_prop_rw("name", &BulkMaterial::get_name, &BulkMaterial::set_name,
                   "Material name.")
      .def_prop_rw("density", &BulkMaterial::get_density,
                   &BulkMaterial::set_density, "Density [kg/m^3].")
      .def_prop_rw("conductivity", &BulkMaterial::get_conductivity,
                   &BulkMaterial::set_conductivity,
                   "Thermal conductivity [W/(m.K)].")
      .def_prop_rw("specific_heat", &BulkMaterial::get_specific_heat,
                   &BulkMaterial::set_specific_heat,
                   "Specific heat [J/(kg.K)].");
}

inline void OpticalMaterial_b(nb::module_& m) {
  nb::class_<OpticalMaterial>(
      m, "OpticalMaterial",
      "Thermo-optical surface properties: six DOF [emissivity_ir, "
      "specular_ir, transmissivity_ir, absorptivity_solar, specular_solar, "
      "transmissivity_solar], each in [0, 1].")
      .def(nb::init<>(), "Create a black-body optical material.")
      .def(nb::init<std::string, double, double>(), "name"_a, "emissivity_ir"_a,
           "absorptivity_solar"_a,
           "Two-property convenience constructor (other DOF default to 0).")
      .def(nb::init<std::string, const OpticalMaterial::Properties&>(), "name"_a,
           "properties"_a, "Full six-DOF constructor.")
      .def_prop_rw("name", &OpticalMaterial::get_name,
                   &OpticalMaterial::set_name, "Material name.")
      .def_prop_rw("th_optical_properties",
                   &OpticalMaterial::get_th_optical_properties,
                   &OpticalMaterial::set_th_optical_properties,
                   "All six optical properties as a length-6 array.")
      .def_prop_rw("emissivity_ir", &OpticalMaterial::emissivity_ir,
                   &OpticalMaterial::set_emissivity_ir,
                   "Infrared emissivity (DOF 0).")
      .def_prop_rw("absorptivity_solar", &OpticalMaterial::absorptivity_solar,
                   &OpticalMaterial::set_absorptivity_solar,
                   "Solar absorptivity (DOF 3).");
}
