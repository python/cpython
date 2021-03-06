.. _audit-events:

.. index:: single: audit events

Audit events table
==================

This table contains all events raised by :func:`sys.audit` or
:c:func:`PySys_Audit` calls throughout the CPython runtime and the
standard library.  These calls were added in 3.8.0 or later.

See :func:`sys.addaudithook` and :c:func:`PySys_AddAuditHook` for
information on handling these events.

.. impl-detail::

   This table is generated from the CPython documentation, and may not
   represent events raised by other implementations. See your runtime
   specific documentation for actual events raised.

.. audit-event-table::

The following events are raised internally and do not correspond to any
public API of CPython:

+--------------------------+-------------------------------------------+
| Audit event              | Arguments                                 |
+==========================+===========================================+
| _winapi.CreateFile       | ``file_name``, ``desired_access``,        |
|                          | ``share_mode``, ``creation_disposition``, |
|                          | ``flags_and_attributes``                  |
+--------------------------+-------------------------------------------+
| _winapi.CreateJunction   | ``src_path``, ``dst_path``                |
+--------------------------+-------------------------------------------+
| _winapi.CreateNamedPipe  | ``name``, ``open_mode``, ``pipe_mode``    |
+--------------------------+-------------------------------------------+
| _winapi.CreatePipe       |                                           |
+--------------------------+-------------------------------------------+
| _winapi.CreateProcess    | ``application_name``, ``command_line``,   |
|                          | ``current_directory``                     |
+--------------------------+-------------------------------------------+
| _winapi.OpenProcess      | ``process_id``, ``desired_access``        |
+--------------------------+-------------------------------------------+
| _winapi.TerminateProcess | ``handle``, ``exit_code``                 |
+--------------------------+-------------------------------------------+
| ctypes.PyObj_FromPtr     | ``obj``                                   |
+--------------------------+-------------------------------------------+
