:mod:`pycanha_core` — Top-level package
=======================================

.. currentmodule:: pycanha_core

The compiled bindings of the ``pycanha-core`` C++ library. The top-level
package holds the package metadata utilities and the submodules. Logging
control lives in :doc:`pycanha_core.log`.

Utilities
---------

.. autofunction:: print_package_info

Convenience exports
-------------------

:class:`~pycanha_core.tmm.NodeType` and :class:`~pycanha_core.log.LogLevel` are
re-exported at package level. They are documented on the
:doc:`pycanha_core.tmm` and :doc:`pycanha_core.log` pages.

.. autodata:: NodeType

.. autodata:: LogLevel
