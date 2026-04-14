:mod:`pycanha_core.tmm` — Thermal Model Classes
================================================

.. currentmodule:: pycanha_core.tmm

The ``tmm`` submodule provides the core classes used to build and inspect
thermal mathematical models.

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

.. autoclass:: ESATANReader
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

Helpers
-------

.. autofunction:: read_tmd_transient