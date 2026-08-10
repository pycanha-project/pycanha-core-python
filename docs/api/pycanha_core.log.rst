:mod:`pycanha_core.log` — Logging
=================================

.. currentmodule:: pycanha_core.log

Every record is produced by the C++ core, which also owns the two destinations
that have to stay live during a call: a console sink on stderr and a daily file
under ``logs/``. No Python code runs on the calling thread while a C++ call is
in progress, so only a C++ sink can report on a long operation as it goes.

Every record also lands in a bounded in-memory buffer. Records are pulled from
that buffer, never pushed. The bridge to the standard library :mod:`logging`
does the pulling, and it is installed on import of ``pycanha_core``.

Integration with :mod:`logging`
-------------------------------

Records are attached to the ``pycanha`` logger, which carries a
:class:`logging.NullHandler`. The Python side prints nothing of its own, so no
record appears twice. Adding a handler to that logger adds a second
destination. To keep only that one, silence the C++ console with
``set_display_level(LogLevel.OFF)``.

A record keeps the clock, process and thread of the C++ event, not those of the
drain that delivered it. The extra ``origin`` attribute is ``pycanha-core`` for
a record made inside the C++ library and ``pycanha`` for one made by a layer
above it. ``TRACE`` maps to level 5 and is registered under that name, so it
does not render as ``Level 5``.

Delivery is deferred. It happens on the way out of the coarse entry points
(solve, read, build, mesh, accumulate), on an explicit :func:`flush`, and at
interpreter exit. Reporting during the call is the job of the console sink.

Record and display thresholds
-----------------------------

The record threshold sets what is produced at all, and therefore what the log
file keeps. The display threshold sets what additionally reaches the console.
The defaults record at ``INFO`` and display at ``WARN``, so a clean run is
silent and the file keeps the full trail.

Both thresholds are bounded below by what the build compiled in. ``DEBUG`` and
``TRACE`` are stripped from a released wheel: :func:`compiled_level_floor`
reports ``INFO`` there, and a request for a more verbose level raises
``ValueError``.

Log file location
-----------------

``logs/`` is always relative to the current working directory. The location is
never inferred from the install path or from a platform user-data directory.
The daily file is created on the first record and appended to. Concurrent
processes share it: every record carries its pid, and every process writes one
banner. If the directory cannot be written to, the file sink is dropped with a
single warning and the run continues.

Levels
------

.. autoclass:: LogLevel
   :members:
   :undoc-members:

.. autofunction:: compiled_level_floor

Thresholds
----------

.. autofunction:: set_record_level

.. autofunction:: record_level

.. autofunction:: set_display_level

.. autofunction:: display_level

The log file
------------

.. autofunction:: set_file_output

.. autofunction:: file_output

.. autofunction:: set_log_directory

.. autofunction:: log_directory

.. autofunction:: current_log_file

The record buffer
-----------------

.. autoclass:: LogRecord
   :members:
   :exclude-members: __dict__, __weakref__, __module__

.. autoclass:: LogDrain
   :members:
   :exclude-members: __dict__, __weakref__, __module__

.. autofunction:: records

.. autofunction:: drain

.. autofunction:: set_buffer_capacity

.. autofunction:: buffer_capacity

.. autofunction:: clear_records

.. autofunction:: flush

Emitting
--------

.. autofunction:: write

.. autofunction:: should_log

Development builds
------------------

.. autofunction:: dev_mode
