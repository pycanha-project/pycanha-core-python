:mod:`pycanha_core.tmm` — Thermal model classes
===============================================

.. currentmodule:: pycanha_core.tmm

Nodes, couplings, time-dependent data and the Thermal Mathematical Model (TMM)
that holds them.

Enumerations
------------

.. autoclass:: NodeType
   :members:
   :undoc-members:

.. autoclass:: InterpolationMethod
   :members:
   :undoc-members:

.. autoclass:: ExtrapolationMethod
   :members:
   :undoc-members:

.. autoclass:: TMDNodeAttribute
   :members:
   :undoc-members:

.. autoclass:: NodeAttribute
   :members:
   :undoc-members:

.. autoclass:: CouplingMerge
   :members:
   :undoc-members:

Bulk calls
----------

``add_nodes``, ``add_couplings`` and the bulk ``get_values`` / ``set_values``
take numpy arrays and return a :class:`BulkReport`. Node numbers may be any
integer dtype; a number outside the int32 range is never wrapped onto another
node.

.. autoclass:: BulkReport
   :members:
   :exclude-members: __dict__, __weakref__, __module__

Nodes
-----

.. autoclass:: Node
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: Nodes
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

Couplings
---------

.. autoclass:: Coupling
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: CouplingMatrices
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: Couplings
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: ConductiveCouplings
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: RadiativeCouplings
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

Thermal data
------------

.. autoclass:: LookupTable1D
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: LookupTableVec1D
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: DenseTimeSeries
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: SparseTimeSeries
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: TimeVariable
   :members:
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: TemperatureVariable
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

Network and model
-----------------

.. autoclass:: ThermalNetwork
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: ThermalData
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: ThermalMathematicalModel
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: ThermalModel
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: ESATANReader
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

Helpers
-------

.. autofunction:: read_tmd_transient