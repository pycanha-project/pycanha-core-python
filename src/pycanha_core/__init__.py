from .pycanha_core import Primitive, Triangle, Rectangle, Quadrilateral, Cylinder, Disc, Cone, Sphere
from .pycanha_core import ThermalMesh
from .pycanha_core import TriMesh, TriMeshModel
from .pycanha_core import CoordinateTransformation
from .pycanha_core import (
    cdt_trimesher,
    create_2d_rectangular_mesh,
    create_2d_quadrilateral_mesh,
    create_2d_triangular_only_mesh,
    create_2d_triangular_mesh,
    create_2d_disc_mesh,
)

# Geometry
from .pycanha_core import (
    Geometry,
    GeometryItem,
    GeometryMeshedItem,
    GeometryGroup,
    GeometryGroupCutted,
)


from .pycanha_core import GeometryModel

from .pycanha_core import print_package_info

print("DEBUG PRINT - REMOVE WHEN RELEASING 1.0")
print_package_info()

# __all__ = ["__doc__", "__version__", "add", "subtract"]
