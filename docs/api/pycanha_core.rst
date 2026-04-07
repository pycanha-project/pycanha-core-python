:mod:`pycanha_core` — Top-Level Package
=======================================

.. currentmodule:: pycanha_core

The ``pycanha_core`` package contains the compiled Python bindings for the
``pycanha-core`` C++ library. The top-level package exposes logging helpers,
package metadata utilities, and the main submodules.

Logging
-------

.. autoclass:: Logger
   :members:
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: LogLevel
   :members:
   :undoc-members:

Utilities
---------

.. autofunction:: print_package_info

.. autofunction:: get_logger

.. autofunction:: get_python_logger

.. autofunction:: set_logger_level

.. autofunction:: set_python_logger_level

Convenience exports
-------------------

.. autodata:: NodeType