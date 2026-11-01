"""Models, build-dependent marks and backend configurations shared by the solver tests."""

import numpy as np
import pycanha_core as pcc
import pytest

solvers = pcc.solvers
tmm = pcc.tmm

requires_mkl = pytest.mark.skipif(not pcc.MKL_ENABLED, reason="needs a build with MKL")
requires_accelerate = pytest.mark.skipif(
    not solvers.accelerate_available(), reason="needs Accelerate (macOS 15.5 or newer)"
)

# Chain 1-2-3-4 to the boundary node 100, heat into node 1. Each link carries
# all of it, so node k sits (5 - k) links above the boundary.
CHAIN_G = 0.5
CHAIN_Q = 10.0
CHAIN_T_BOUNDARY = 300.0
CHAIN_NODES = [1, 2, 3, 4]
CHAIN_EXPECTED = [CHAIN_T_BOUNDARY + (5 - k) * CHAIN_Q / CHAIN_G for k in CHAIN_NODES]

BASIC_DIFFUSIVE = [10, 15, 20, 25]


def make_chain_tmm():
    """A conduction-only model with a known steady state, built with the bulk calls."""
    model = tmm.ThermalMathematicalModel("chain")
    model.add_nodes(
        np.array(CHAIN_NODES, dtype=np.int32),
        T=np.full(4, 273.15),
        C=np.full(4, 10.0),
        qi=np.array([CHAIN_Q, 0.0, 0.0, 0.0]),
    )
    model.add_nodes(
        np.array([100], dtype=np.int32), type=tmm.NodeType.BOUNDARY, T=np.array([CHAIN_T_BOUNDARY])
    )
    model.add_conductive_couplings(
        np.array([1, 2, 3, 4], dtype=np.int32),
        np.array([2, 3, 4, 100], dtype=np.int32),
        np.full(4, CHAIN_G),
    )
    return model


def make_jacobian_tmm():
    """One diffusive node coupled to a boundary node, with derivatives for k and C."""
    model = tmm.ThermalMathematicalModel("jacobian_example")

    diffusive_node = tmm.Node(1)
    diffusive_node.T = 0.0
    diffusive_node.capacity = 1.0
    diffusive_node.qi = 1.0

    boundary_node = tmm.Node(2)
    boundary_node.type = pcc.NodeType.BOUNDARY
    boundary_node.T = 1.0

    model.add_node(diffusive_node)
    model.add_node(boundary_node)
    model.add_conductive_coupling(1, 2, 1.0)

    model.parameters.add_parameter("k", 1.0)
    model.parameters.add_parameter("C", 1.0)

    model.formulas.add_parameter_formula(model.entities.conductive_coupling(1, 2), "k")
    model.formulas.add_parameter_formula(model.entities.capacity(1), "C")
    model.formulas.parameters_with_derivatives.add_parameter("k")
    model.formulas.parameters_with_derivatives.add_parameter("C")
    model.formulas.apply_formulas()

    return model


def temperatures(model, numbers):
    return np.array([model.nodes.get_T(number) for number in numbers])


def _config(storage, backend, **groups):
    return {"storage": storage, "backend": backend, **groups}


S = solvers.Storage
B = solvers.Backend

# Every storage x backend (x ordering) the solver types can run, each skipped
# where the build or the platform lacks its library.
CONFIGURATIONS = [
    pytest.param(
        _config(S.SPARSE, B.EIGEN, eigen=solvers.EigenSettings(ordering=solvers.EigenOrdering.COLAMD)),
        id="sparse-eigen-colamd",
    ),
    pytest.param(
        _config(S.SPARSE, B.EIGEN, eigen=solvers.EigenSettings(ordering=solvers.EigenOrdering.AMD)),
        id="sparse-eigen-amd",
    ),
    pytest.param(_config(S.DENSE, B.EIGEN), id="dense-eigen"),
    pytest.param(
        _config(
            S.SPARSE,
            B.PARDISO,
            pardiso=solvers.PardisoSettings(
                level=solvers.PardisoLevel.TWO_LEVEL,
                ordering=solvers.PardisoOrdering.NESTED_DISSECTION,
            ),
        ),
        id="sparse-pardiso-two-level",
        marks=requires_mkl,
    ),
    pytest.param(
        _config(
            S.SPARSE,
            B.PARDISO,
            pardiso=solvers.PardisoSettings(
                level=solvers.PardisoLevel.ONE_LEVEL,
                ordering=solvers.PardisoOrdering.NESTED_DISSECTION,
            ),
        ),
        id="sparse-pardiso-one-level",
        marks=requires_mkl,
    ),
    pytest.param(
        _config(
            S.SPARSE,
            B.PARDISO,
            pardiso=solvers.PardisoSettings(
                level=solvers.PardisoLevel.ONE_LEVEL,
                ordering=solvers.PardisoOrdering.MIN_DEGREE,
            ),
        ),
        id="sparse-pardiso-min-degree",
        marks=requires_mkl,
    ),
    pytest.param(_config(S.DENSE, B.LAPACK), id="dense-lapack", marks=requires_mkl),
    pytest.param(
        _config(
            S.SPARSE,
            B.ACCELERATE,
            accelerate=solvers.AccelerateSettings(ordering=solvers.AccelerateOrdering.METIS),
        ),
        id="sparse-accelerate-metis",
        marks=requires_accelerate,
    ),
    pytest.param(
        _config(
            S.SPARSE,
            B.ACCELERATE,
            accelerate=solvers.AccelerateSettings(ordering=solvers.AccelerateOrdering.AMD),
        ),
        id="sparse-accelerate-amd",
        marks=requires_accelerate,
    ),
]

# Every factorization a model with radiative couplings accepts.
RADIATIVE_FACTORIZATIONS = [solvers.Factorization.AUTO, solvers.Factorization.LU]
# Every factorization a conduction-only model accepts.
CONDUCTION_FACTORIZATIONS = [*RADIATIVE_FACTORIZATIONS, solvers.Factorization.CHOLESKY]

# Configurations that iterate on old factors, for reuse_factors = True.
REUSE_CONFIGURATIONS = [
    pytest.param(
        _config(
            S.SPARSE,
            B.PARDISO,
            pardiso=solvers.PardisoSettings(
                level=solvers.PardisoLevel.ONE_LEVEL,
                ordering=solvers.PardisoOrdering.MIN_DEGREE,
            ),
        ),
        id="sparse-pardiso-min-degree",
        marks=requires_mkl,
    ),
    pytest.param(_config(S.DENSE, B.LAPACK), id="dense-lapack", marks=requires_mkl),
]
