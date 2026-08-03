:mod:`pycanha_core.conduction` — GMM to TMM Conduction Builder
==============================================================

.. currentmodule:: pycanha_core.conduction

The ``conduction`` submodule turns a ``GeometryModel`` into the nodes and
conductive couplings of a ``ThermalMathematicalModel``. It sits between ``gmm``
and ``tmm`` the same way ``radiative`` does, so ``gmm`` stays free of ``tmm``
knowledge and ``tmm`` free of ``gmm`` knowledge.

The usual entry point is :meth:`pycanha_core.tmm.ThermalModel.build_tmm_from_gmm`,
which delegates to :func:`build_tmm_from_gmm` below. Every conductively active
face slot that carries a node number contributes its capacitance, its area and
its centroid to that node; every pair of adjacent cells contributes an in-plane
conductor; and face pairs whose two sides carry different node numbers also get
a through-thickness conductor. Which sides count as conductively active comes
from :class:`pycanha_core.gmm.ThermalMesh`'s ``conductive_active_side``.

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

These compute the conductors of a single primitive without building a model —
useful to check one shape's discretization in isolation.

.. autoclass:: CellLink
   :members:
   :exclude-members: __dict__, __weakref__, __module__

.. autofunction:: intra_primitive_links

.. autofunction:: through_thickness_conductance

Meridian profile
----------------

Every non-planar pycanha primitive is a surface of revolution. The profile
carries the two scalar maps one-dimensional conduction needs from it: the
coordinate heat flows along in direction 1, and the potential
``Phi = integral of dl2 / rho`` along the meridian.

.. autofunction:: profile_of

.. autoclass:: MeridianProfile
   :members:
   :exclude-members: __dict__, __weakref__, __module__
