"""Type-checking probe for the installed package (not a pytest module).

The CI runs mypy on this file with no configuration, after the wheel is
installed: it fails if mypy cannot find the package, treats it as untyped, or
cannot resolve a name from the stubs, including those of nested submodules.
"""

import pycanha_core
import pycanha_core.solvers.transient
from pycanha_core.solvers import Backend, Settings, SolverReport
from pycanha_core.solvers.sensitivity import JacobianPropagation
from pycanha_core.solvers.steady import SteadyState
from pycanha_core.solvers.transient import CrankNicolson
from pycanha_core.tmm import ThermalModel


def probe() -> None:
    model = ThermalModel("probe")
    transient: CrankNicolson = model.solvers.transient.crank_nicolson
    time_step: float | None = transient.time_step
    transient.set_settings(Settings(time_step=10.0, backend=Backend.PARDISO))
    steady: SteadyState = pycanha_core.solvers.steady.SteadyState(model.tmm)
    report: SolverReport = steady.report()
    jacobian: type[JacobianPropagation] = pycanha_core.solvers.sensitivity.JacobianPropagation
    mkl: bool = pycanha_core.MKL_ENABLED
    nested: type[CrankNicolson] = pycanha_core.solvers.transient.CrankNicolson
    print(time_step, report.converged, jacobian, mkl, nested)
