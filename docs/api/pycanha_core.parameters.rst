:mod:`pycanha_core.parameters` — Parameters and Formulas
========================================================

.. currentmodule:: pycanha_core.parameters

The ``parameters`` submodule provides the core parameter storage and formula
binding types used by the thermal model.

Parameters
----------

.. autoclass:: Parameters
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

Entities
--------

.. autoclass:: ThermalEntity
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: AttributeEntity
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: ConductiveCouplingEntity
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: RadiativeCouplingEntity
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

Formulas
--------

.. autoclass:: Formula
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: ParameterFormula
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: ValueFormula
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: ExpressionFormula
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: Formulas
   :members:
   :special-members: __init__
   :exclude-members: __dict__, __weakref__, __module__