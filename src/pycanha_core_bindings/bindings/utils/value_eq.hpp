#pragma once

#include <nanobind/nanobind.h>

namespace nb = nanobind;
using namespace nanobind::literals; // NOLINT(build/namespaces)

namespace pycanha::bindings::utils {

// __eq__ of a value type. Python may compare it with any object, so the
// argument is declared as object (as builtins.object.__eq__ has it, which the
// stubs must match); anything that is not a T compares unequal.
template <typename T, typename Cls> void def_value_eq(Cls &cls) {
  cls.def(
      "__eq__",
      [](const T &self, nb::handle other) {
        return nb::isinstance<T>(other) && self == nb::cast<const T &>(other);
      },
      "other"_a);
}

} // namespace pycanha::bindings::utils
