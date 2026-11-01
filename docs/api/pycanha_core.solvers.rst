:mod:`pycanha_core.solvers` — Solvers
=====================================

.. currentmodule:: pycanha_core.solvers

The solver types live in one submodule per question:
:mod:`pycanha_core.solvers.steady` (:class:`~steady.SteadyState`),
:mod:`pycanha_core.solvers.transient` (:class:`~transient.CrankNicolson`) and
:mod:`pycanha_core.solvers.sensitivity`
(:class:`~sensitivity.JacobianPropagation`). Everything they share is in
:mod:`pycanha_core.solvers`: the settings and their enums, the report, the
:class:`Solver` base and the registry.

A solver works on one model with one :class:`Settings`. Each solver class has
an attribute for every setting that applies to its type, and only for those:
``crank_nicolson.time_step`` exists, ``steady_state.time_step`` is an
``AttributeError``. Unset settings (``None``) take the solver's default.

.. code-block:: python

   import pycanha_core as pcc

   solvers = pcc.solvers
   cn = tm.solvers.transient.crank_nicolson
   cn.set_simulation_time(0.0, 5400.0, 10.0, 60.0)  # start, end, time step, output [s]
   cn.tolerance = 1e-3  # [K]
   cn.reuse_factors = True
   cn.pardiso.iparm = {3: 61}  # assigned as a whole dict
   cn.initialize()
   cn.solve()
   print(cn.report())

   shared = solvers.Settings(tolerance=1e-4, backend=solvers.Backend.EIGEN)
   tm.solvers.steady.steady_state.set_settings(shared)

``settings()`` returns a copy; attributes, :meth:`Solver.set` and
:meth:`Solver.set_settings` write through to the solver. The INIT settings
(see :class:`Mutability`) are read by ``initialize()`` and frozen while the
solver is initialised: changing one then raises ``RuntimeError``. A set field
that does not apply to a solver is ignored there with a warning at
``initialize()``, so one :class:`Settings` can be shared by every type.

Solver types
------------

.. autoclass:: pycanha_core.solvers.steady.SteadyState
   :members:
   :special-members: __init__
   :show-inheritance:
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: pycanha_core.solvers.transient.CrankNicolson
   :members:
   :special-members: __init__
   :show-inheritance:
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: pycanha_core.solvers.sensitivity.JacobianPropagation
   :members:
   :special-members: __init__
   :show-inheritance:
   :exclude-members: __dict__, __weakref__, __module__

Solver base
-----------

.. autoclass:: Solver
   :members:
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: PardisoView
   :members:

.. autoclass:: EigenView
   :members:

.. autoclass:: AccelerateView
   :members:

.. autoexception:: ModelChangedError

Settings
--------

.. autoclass:: Settings
   :members:
   :special-members: __init__, __str__

.. autoclass:: PardisoSettings
   :members:
   :special-members: __init__

.. autoclass:: EigenSettings
   :members:
   :special-members: __init__

.. autoclass:: AccelerateSettings
   :members:
   :special-members: __init__

.. autofunction:: setting_fields

.. autofunction:: setting_field

.. autoclass:: SettingField
   :members:

Enumerations
------------

.. autoclass:: SolverType
   :members:
   :undoc-members:

.. autoclass:: Storage
   :members:
   :undoc-members:

.. autoclass:: Backend
   :members:
   :undoc-members:

.. autoclass:: Factorization
   :members:
   :undoc-members:

.. autoclass:: PardisoOrdering
   :members:
   :undoc-members:

.. autoclass:: PardisoLevel
   :members:
   :undoc-members:

.. autoclass:: EigenOrdering
   :members:
   :undoc-members:

.. autoclass:: AccelerateOrdering
   :members:
   :undoc-members:

.. autoclass:: Mutability
   :members:
   :undoc-members:

.. autoclass:: Origin
   :members:
   :undoc-members:

Resolution
----------

What ``initialize()`` does with the settings, available without a solver.

.. autoclass:: ResolveContext
   :members:
   :special-members: __init__

.. autofunction:: resolve_context

.. autofunction:: resolve

.. autoclass:: ResolvedSettings
   :members:

.. autoclass:: ResolvedPardiso
   :members:

.. autoclass:: ResolvedEigen
   :members:

.. autoclass:: ResolvedAccelerate
   :members:

.. autoclass:: NotApplied
   :members:

.. autofunction:: explain_settings

.. autoclass:: SettingsExplanation
   :members:

.. autoclass:: FieldExplanation
   :members:

.. autofunction:: changed_init_fields

.. autofunction:: check_settings_change

.. autofunction:: accelerate_available

.. autofunction:: all_dense_output_attributes

.. autofunction:: all_output_attributes

Report and memory
-----------------

.. autoclass:: SolverReport
   :members:
   :special-members: __str__

.. autoclass:: SolveStatus
   :members:
   :undoc-members:

.. autoclass:: NonConvergedStep
   :members:

.. autoclass:: SolverMemory
   :members:
   :special-members: __str__

.. autofunction:: format_bytes

.. autofunction:: parse_memory_size

Registry and callbacks
----------------------

.. autoclass:: SolverRegistry
   :members:

.. autoclass:: SteadySolvers
   :members:

.. autoclass:: TransientSolvers
   :members:

.. autoclass:: SensitivitySolvers
   :members:

.. autoclass:: CallbackRegistry
   :members:

.. autoclass:: CallbackContext
   :members:
