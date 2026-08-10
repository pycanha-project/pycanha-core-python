:mod:`pycanha_core.conduction` — GMM to TMM conduction builder
==============================================================

.. currentmodule:: pycanha_core.conduction

Turns a ``GeometryModel`` into the nodes and conductive couplings of a
``ThermalMathematicalModel``. The submodule sits between ``gmm`` and ``tmm``,
like ``radiative``, so neither of those two depends on the other.

The usual entry point is
:meth:`pycanha_core.tmm.ThermalModel.build_tmm_from_gmm`, which calls
:func:`build_tmm_from_gmm` below. Each conductively active face slot that
carries a node number adds its thermal capacity, its area and its centroid to
that node. Each pair of adjacent faces gives an in-plane conductive coupling.
Face pairs whose two sides carry different node numbers also give a
through-thickness coupling. The conductively active sides are read from
``conductive_active_side`` of :class:`pycanha_core.gmm.ThermalMesh`.

Building
--------

.. autofunction:: build_tmm_from_gmm

.. autoclass:: TmmBuildOptions
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

Reports and diagnostics
-----------------------

.. autoclass:: TmmBuildReport
   :members:
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: BuildDiagnostic
   :members:
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: DiagnosticCode
   :members:

.. autofunction:: diagnostic_code_name

Link-level services
-------------------

These compute the conductive couplings of a single primitive without building a
model. Use them to check the discretization of one shape in isolation.

.. autoclass:: CellLink
   :members:
   :exclude-members: __dict__, __weakref__, __module__

.. autofunction:: intra_primitive_links

.. autofunction:: through_thickness_conductance

Meridian profile
----------------

Every non-planar primitive is a surface of revolution. The profile carries the
two scalar maps that one-dimensional conduction needs from it: the coordinate
the heat flows along in direction 1, and the potential
``Phi = integral of dl2 / rho`` along the meridian.

.. autofunction:: profile_of

.. autoclass:: MeridianProfile
   :members:
   :exclude-members: __dict__, __weakref__, __module__
