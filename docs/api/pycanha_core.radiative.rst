:mod:`pycanha_core.radiative` — GPU raytraced radiative engine
==============================================================

.. currentmodule:: pycanha_core.radiative

Device discovery, scene setup, and the view factor, exchange and solar
accumulators, together with the CPU-side Gebhart and aggregation services. The
binding is a one to one exposure of the C++ engine.

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

.. autoclass:: TriangulationMode
   :members:

.. autoclass:: TriangulationConfig
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

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

Sparse results are returned as ``scipy.sparse.csr_matrix``, and sparse
arguments accept the same type. A matrix result holds the upper triangle of the
symmetric extensive quantity. The virtual bucket columns follow the real face
columns.

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

.. autoclass:: AggregateResult
   :members:
   :exclude-members: __dict__, __weakref__, __module__

.. autofunction:: aggregate_nodes
.. autofunction:: aggregate_matrix
.. autofunction:: aggregate_flux
