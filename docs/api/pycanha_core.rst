:mod:`pycanha_core` — Top-level package
=======================================

.. currentmodule:: pycanha_core

The compiled bindings of the ``pycanha-core`` C++ library. The top-level
package holds the package metadata utilities and the submodules. Logging
control lives in :doc:`pycanha_core.log`.

Utilities
---------

.. autofunction:: print_package_info

.. autodata:: MKL_ENABLED

   Whether the extension is built with Intel MKL (the PARDISO and LAPACK
   solver backends).

.. autodata:: ACCELERATE_ENABLED

   Whether the extension is built with Apple Accelerate (the ACCELERATE solver
   backend, which also needs macOS 15.5 or newer: see
   :func:`pycanha_core.solvers.accelerate_available`).

Convenience exports
-------------------

:class:`~pycanha_core.tmm.NodeType` and :class:`~pycanha_core.log.LogLevel` are
re-exported at package level. They are documented on the
:doc:`pycanha_core.tmm` and :doc:`pycanha_core.log` pages.

.. autodata:: NodeType

.. autodata:: LogLevel
