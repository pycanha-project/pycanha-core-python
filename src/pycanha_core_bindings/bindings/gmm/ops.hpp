#pragma once
#include <nanobind/eigen/dense.h>
#include <nanobind/nanobind.h>
#include <nanobind/stl/variant.h>

#include "pycanha-core/gmm/ops/distance.hpp"
#include "pycanha-core/gmm/ops/transform.hpp"

namespace nb = nanobind;
using namespace nanobind::literals;
using namespace pycanha::gmm;

// Free primitive operations. Both take/return the Primitive variant, handled by
// <nanobind/stl/variant.h>, so a concrete Python primitive is accepted and the
// result surfaces as its concrete Python type.

inline void ops_b(nb::module_& m) {
  m.def("distance", &pycanha::gmm::ops::distance, "primitive"_a, "point"_a,
        "Signed distance from a point to a primitive's surface.");
  m.def("transform", &pycanha::gmm::ops::transform, "primitive"_a,
        "transformation"_a,
        "Return a copy of the primitive with the transformation applied.");
}
