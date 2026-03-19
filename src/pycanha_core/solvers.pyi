from __future__ import annotations

from typing import Optional

from .tmm import ThermalMathematicalModel

class Solver:
    MAX_ITERS: int
    MAX_ITER: int
    abstol_temp: float
    abstol_enrgy: float
    eps_capacity: float
    eps_time: float
    eps_coupling: float
    pardiso_iparm_3: int

    @property
    def solver_iter(self) -> int: ...
    @property
    def solver_name(self) -> str: ...
    @property
    def solver_initialized(self) -> bool: ...
    @property
    def solver_converged(self) -> bool: ...

class SteadyStateSolver(Solver): ...

class TransientSolver(Solver):
    def set_simulation_time(
        self,
        start_time: float,
        end_time: float,
        dtime: float,
        output_stride: float,
    ) -> None: ...
    @property
    def time(self) -> float: ...
    @property
    def time_iter(self) -> int: ...
    @property
    def output_table_name(self) -> str: ...

class TSCN(TransientSolver): ...
class TSCNRL(TSCN): ...

class SSLU(SteadyStateSolver):
    def __init__(self, tmm: ThermalMathematicalModel) -> None: ...
    def initialize(self) -> None: ...
    def solve(self) -> None: ...
    def deinitialize(self) -> None: ...

class TSCNRLDS(TSCNRL):
    def __init__(self, tmm: ThermalMathematicalModel) -> None: ...
    def initialize(self) -> None: ...
    def solve(self) -> None: ...
    def deinitialize(self) -> None: ...

__all__ = [
    "Solver",
    "SteadyStateSolver",
    "TransientSolver",
    "TSCN",
    "TSCNRL",
    "SSLU",
    "TSCNRLDS",
]
