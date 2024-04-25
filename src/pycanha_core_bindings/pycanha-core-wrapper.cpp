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
#include "pycanha-core/utils/package_info.hpp"

namespace py = pybind11;
using namespace pybind11::literals;  // for _a shorthand

using namespace pycanha;

using namespace gmm;

PYBIND11_MODULE(pycanha_core_cpp, m) {
  py::module gmm_submodule = m.def_submodule("gmm", "Geometry submodule");

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

  m.def("print_package_info", &print_package_info);
}
