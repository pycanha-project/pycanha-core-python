pycanha-core Python bindings
============================

``pycanha-core`` is the Python binding of the `pycanha-core
<https://github.com/pycanha-project/pycanha-core>`_ C++ library. It is imported
as ``pycanha_core`` and exposes the thermal model, geometry, parameter, solver,
radiative and conduction classes of the library.

Most code is written against `pycanha
<https://github.com/pycanha-project/pycanha>`_, the Python package built on
these bindings. This site documents the compiled layer itself and is an API
reference for the wheel published on PyPI.

Documented release: |doc_release|.

.. grid:: 1 1 2 2
   :gutter: 3

   .. grid-item-card:: Installation
      :link: installation
      :link-type: doc

      Install from PyPI, or build the extension from a local checkout of the
      C++ library.

   .. grid-item-card:: API reference
      :link: api/index
      :link-type: doc

      Every class and function of the top-level package and of each submodule.

   .. grid-item-card:: Repository
      :link: https://github.com/pycanha-project/pycanha-core-python

      Source code, issue tracker and releases.

.. toctree::
   :maxdepth: 2
   :hidden:

   installation
   api/index
