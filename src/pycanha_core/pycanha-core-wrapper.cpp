#include <pybind11/eigen.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <vector>

#include "pycanha-core/utils/package_info.hpp"
#include "bindings/gmm/geometry.hpp"
#include "bindings/gmm/geometrymodel.hpp"
#include "bindings/gmm/primitives.hpp"
#include "bindings/gmm/thermalmesh.hpp"
#include "bindings/gmm/transformations.hpp"
#include "bindings/gmm/trimesh.hpp"

namespace py = pybind11;
using namespace pybind11::literals;  // for _a shorthand

using namespace pycanha;

using namespace gmm;

PYBIND11_MODULE(pycanha_core, m) {
  Primitive_b(m);

  Triangle_b(m);
  Rectangle_b(m);
  Quadrilateral_b(m);
  Cylinder_b(m);
  Disc_b(m);
  Cone_b(m);
  Sphere_b(m);

  ThermalMesh_b(m);

  CoordinateTransformation_b(m);

  Geometry_b(m);
  GeometryItem_b(m);
  GeometryMeshedItem_b(m);
  GeometryGroup_b(m);
  GeometryGroupCutted_b(m);

  GeometryModel_b(m);

  TriMesh_b(m);
  TriMeshModel_b(m);
  primitive_meshers_b(m);

  m.def("print_package_info", &print_package_info);
}
