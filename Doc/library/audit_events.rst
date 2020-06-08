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
