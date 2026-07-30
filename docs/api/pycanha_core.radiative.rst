:mod:`pycanha_core.radiative` — GPU-Raytraced Radiative Engine
==============================================================

.. currentmodule:: pycanha_core.radiative

The ``radiative`` submodule is a 1:1 exposure of the C++ radiative engine.

.. autodata:: num_virtual_columns
.. autodata:: space_column_offset
.. autodata:: inactive_column_offset
.. autodata:: lost_column_offset

Device discovery
----------------

.. autofunction:: is_available
.. autofunction:: enumerate_devices

.. autoclass:: DeviceInfo
   :members:
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: Device
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

Scene inputs
------------

.. autoclass:: PartKind
   :members:

.. autoclass:: Band
   :members:

.. autoclass:: ScenePart
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: MaterialTable
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: SolarState
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

Settings
--------

.. autoclass:: TraceSettings
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: AccumLayout
   :members:

.. autoclass:: AccumConfig
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

Scene and accumulators
----------------------

.. autoclass:: RadiativeScene
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: VfAccumulator
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: ExchangeAccumulator
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: SolarAccumulator
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

Results
-------

.. autoclass:: SparseF64
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: TraceStats
   :members:
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: VfResult
   :members:
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: ExchangeResult
   :members:
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: SolarResult
   :members:
   :exclude-members: __dict__, __weakref__, __module__

Memory sizing
-------------

.. autoclass:: MemoryEstimate
   :members:
   :exclude-members: __dict__, __weakref__, __module__

.. autofunction:: estimate_memory

Gebhart services (CPU)
----------------------

.. autofunction:: gebhart_factors
.. autofunction:: gebhart_node_factors

Aggregation services (CPU)
--------------------------

.. autofunction:: aggregate_nodes
.. autofunction:: aggregate_matrix
.. autofunction:: aggregate_flux
