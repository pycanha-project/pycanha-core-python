#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <vector>

#include "bindings/gmm/geometry.hpp"
#include "bindings/gmm/geometrymodel.hpp"
#include "bindings/gmm/primitives.hpp"
#include "bindings/gmm/thermalmesh.hpp"
#include "bindings/gmm/transformations.hpp"
#include "bindings/gmm/trimesh.hpp"
#include "bindings/parameters/parameters.hpp"
#include "bindings/solvers/solvers.hpp"
#include "bindings/tmm/couplings.hpp"
#include "bindings/tmm/nodes.hpp"
#include "bindings/tmm/thermalmathematicalmodel.hpp"
#include "bindings/tmm/thermalnetwork.hpp"
#include "pycanha-core/utils/package_info.hpp"

namespace py = pybind11;
using namespace pybind11::literals; // for _a shorthand

using namespace pycanha;

using namespace gmm;
using namespace pycanha::bindings::parameters;
using namespace pycanha::bindings::solvers;
using namespace pycanha::bindings::tmm;

PYBIND11_MODULE(pycanha_core, m) {
  py::module gmm_submodule =
      m.def_submodule("gmm", "Geometrical Mathematical Model");

  Primitive_b(gmm_submodule);

  Triangle_b(gmm_submodule);
  Rectangle_b(gmm_submodule);
  Quadrilateral_b(gmm_submodule);
  Cylinder_b(gmm_submodule);
  Disc_b(gmm_submodule);
  Cone_b(gmm_submodule);
  Sphere_b(gmm_submodule);

  ThermalMesh_b(gmm_submodule);

  CoordinateTransformation_b(gmm_submodule);

  Geometry_b(gmm_submodule);
  GeometryItem_b(gmm_submodule);
  GeometryMeshedItem_b(gmm_submodule);
  GeometryGroup_b(gmm_submodule);
  GeometryGroupCutted_b(gmm_submodule);

  GeometryModel_b(gmm_submodule);

  TriMesh_b(gmm_submodule);
  TriMeshModel_b(gmm_submodule);
  primitive_meshers_b(gmm_submodule);

  py::module tmm_submodule =
      m.def_submodule("tmm", "Thermal Mathematical Model");
  register_nodes(tmm_submodule);
  register_couplings(tmm_submodule);
  register_thermalnetwork(tmm_submodule);

  py::module parameters_submodule =
      m.def_submodule("parameters", "Thermal parameters and formulas");
  register_parameters(parameters_submodule);
  register_entities(parameters_submodule);
  register_formulas(parameters_submodule);

  register_thermal_data(tmm_submodule);
  register_thermal_mathematical_model(tmm_submodule);

  py::module solvers_submodule = m.def_submodule("solvers", "Thermal solvers");
  register_solvers(solvers_submodule);

  // Re-export commonly used enums at the package root for convenience.
  if (py::hasattr(tmm_submodule, "NodeType")) {
    m.attr("NodeType") = tmm_submodule.attr("NodeType");
  }

  m.def("print_package_info", &print_package_info);
}
