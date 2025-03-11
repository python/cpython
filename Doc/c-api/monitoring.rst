.. highlight:: c

.. _c-api-monitoring:

Monitoring C API
================

Added in version 3.13.

An extension may need to interact with the event monitoring system. Subscribing
to events and registering callbacks can be done via the Python API exposed in
:mod:`sys.monitoring`.

Generating Execution Events
===========================

The functions below make it possible for an extension to fire monitoring
events as it emulates the execution of Python code. Each of these functions
accepts a ``PyMonitoringState`` struct which contains concise information
about the activation state of events, as well as the event arguments, which
include a ``PyObject*`` representing the code object, the instruction offset
and sometimes additional, event-specific arguments (see :mod:`sys.monitoring`
for details about the signatures of the different event callbacks).
The ``codelike`` argument should be an instance of :class:`types.CodeType`
or of a type that emulates it.

The VM disables tracing when firing an event, so there is no need for user
code to do that.

Monitoring functions should not be called with an exception set,
except those listed below as working with the current exception.

.. c:type:: PyMonitoringState

  Representation of the state of an event type. It is allocated by the user
  while its contents are maintained by the monitoring API functions described below.


All of the functions below return 0 on success and -1 (with an exception set) on error.

See :mod:`sys.monitoring` for descriptions of the events.

.. c:function:: int PyMonitoring_FirePyStartEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)

   Fire a ``PY_START`` event.


.. c:function:: int PyMonitoring_FirePyResumeEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)

   Fire a ``PY_RESUME`` event.


.. c:function:: int PyMonitoring_FirePyReturnEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject* retval)

   Fire a ``PY_RETURN`` event.


.. c:function:: int PyMonitoring_FirePyYieldEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject* retval)

   Fire a ``PY_YIELD`` event.


.. c:function:: int PyMonitoring_FireCallEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject* callable, PyObject *arg0)

   Fire a ``CALL`` event.


.. c:function:: int PyMonitoring_FireLineEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, int lineno)

   Fire a ``LINE`` event.


.. c:function:: int PyMonitoring_FireJumpEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *target_offset)

   Fire a ``JUMP`` event.


.. c:function:: int PyMonitoring_FireBranchLeftEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *target_offset)

   Fire a ``BRANCH_LEFT`` event.


.. c:function:: int PyMonitoring_FireBranchRightEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *target_offset)

   Fire a ``BRANCH_RIGHT`` event.


.. c:function:: int PyMonitoring_FireCReturnEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *retval)

   Fire a ``C_RETURN`` event.


.. c:function:: int PyMonitoring_FirePyThrowEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)

   Fire a ``PY_THROW`` event with the current exception (as returned by
   :c:func:`PyErr_GetRaisedException`).


.. c:function:: int PyMonitoring_FireRaiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)

   Fire a ``RAISE`` event with the current exception (as returned by
   :c:func:`PyErr_GetRaisedException`).


.. c:function:: int PyMonitoring_FireCRaiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)

   Fire a ``C_RAISE`` event with the current exception (as returned by
   :c:func:`PyErr_GetRaisedException`).


.. c:function:: int PyMonitoring_FireReraiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)

   Fire a ``RERAISE`` event with the current exception (as returned by
   :c:func:`PyErr_GetRaisedException`).


.. c:function:: int PyMonitoring_FireExceptionHandledEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)

   Fire an ``EXCEPTION_HANDLED`` event with the current exception (as returned by
   :c:func:`PyErr_GetRaisedException`).


.. c:function:: int PyMonitoring_FirePyUnwindEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset)

   Fire a ``PY_UNWIND`` event with the current exception (as returned by
   :c:func:`PyErr_GetRaisedException`).


.. c:function:: int PyMonitoring_FireStopIterationEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *value)

   Fire a ``STOP_ITERATION`` event. If ``value`` is an instance of :exc:`StopIteration`, it is used. Otherwise,
   a new :exc:`StopIteration` instance is created with ``value`` as its argument.


Managing the Monitoring State
-----------------------------

Monitoring states can be managed with the help of monitoring scopes. A scope
would typically correspond to a python function.

.. c:function:: int PyMonitoring_EnterScope(PyMonitoringState *state_array, uint64_t *version, const uint8_t *event_types, Py_ssize_t length)

   Enter a monitored scope. ``event_types`` is an array of the event IDs for
   events that may be fired from the scope. For example, the ID of a ``PY_START``
   event is the value ``PY_MONITORING_EVENT_PY_START``, which is numerically equal
   to the base-2 logarithm of ``sys.monitoring.events.PY_START``.
   ``state_array`` is an array with a monitoring state entry for each event in
   ``event_types``, it is allocated by the user but populated by
   :c:func:`!PyMonitoring_EnterScope` with information about the activation state of
   the event. The size of ``event_types`` (and hence also of ``state_array``)
   is given in ``length``.

   The ``version`` argument is a pointer to a value which should be allocated
   by the user together with ``state_array`` and initialized to 0,
   and then set only by :c:func:`!PyMonitoring_EnterScope` itself. It allows this
   function to determine whether event states have changed since the previous call,
   and to return quickly if they have not.

   The scopes referred to here are lexical scopes: a function, class or method.
   :c:func:`!PyMonitoring_EnterScope` should be called whenever the lexical scope is
   entered. Scopes can be reentered, reusing the same *state_array* and *version*,
   in situations like when emulating a recursive Python function. When a code-like's
   execution is paused, such as when emulating a generator, the scope needs to
   be exited and re-entered.

   The macros for *event_types* are:

   .. c:namespace:: NULL

   .. The table is here to make the docs searchable, and to allow automatic
      links to the identifiers.

   ================================================== =====================================
   Macro                                              Event
   ================================================== =====================================
   .. c:macro:: PY_MONITORING_EVENT_BRANCH_LEFT       :monitoring-event:`BRANCH_LEFT`
   .. c:macro:: PY_MONITORING_EVENT_BRANCH_RIGHT      :monitoring-event:`BRANCH_RIGHT`
   .. c:macro:: PY_MONITORING_EVENT_CALL              :monitoring-event:`CALL`
   .. c:macro:: PY_MONITORING_EVENT_C_RAISE           :monitoring-event:`C_RAISE`
   .. c:macro:: PY_MONITORING_EVENT_C_RETURN          :monitoring-event:`C_RETURN`
   .. c:macro:: PY_MONITORING_EVENT_EXCEPTION_HANDLED :monitoring-event:`EXCEPTION_HANDLED`
   .. c:macro:: PY_MONITORING_EVENT_INSTRUCTION       :monitoring-event:`INSTRUCTION`
   .. c:macro:: PY_MONITORING_EVENT_JUMP              :monitoring-event:`JUMP`
   .. c:macro:: PY_MONITORING_EVENT_LINE              :monitoring-event:`LINE`
   .. c:macro:: PY_MONITORING_EVENT_PY_RESUME         :monitoring-event:`PY_RESUME`
   .. c:macro:: PY_MONITORING_EVENT_PY_RETURN         :monitoring-event:`PY_RETURN`
   .. c:macro:: PY_MONITORING_EVENT_PY_START          :monitoring-event:`PY_START`
   .. c:macro:: PY_MONITORING_EVENT_PY_THROW          :monitoring-event:`PY_THROW`
   .. c:macro:: PY_MONITORING_EVENT_PY_UNWIND         :monitoring-event:`PY_UNWIND`
   .. c:macro:: PY_MONITORING_EVENT_PY_YIELD          :monitoring-event:`PY_YIELD`
   .. c:macro:: PY_MONITORING_EVENT_RAISE             :monitoring-event:`RAISE`
   .. c:macro:: PY_MONITORING_EVENT_RERAISE           :monitoring-event:`RERAISE`
   .. c:macro:: PY_MONITORING_EVENT_STOP_ITERATION    :monitoring-event:`STOP_ITERATION`
   ================================================== =====================================

.. c:function:: int PyMonitoring_ExitScope(void)

   Exit the last scope that was entered with :c:func:`!PyMonitoring_EnterScope`.


.. c:function:: int PY_MONITORING_IS_INSTRUMENTED_EVENT(uint8_t ev)

   Return true if the event corresponding to the event ID *ev* is
   a :ref:`local event <monitoring-event-local>`.

   .. versionadded:: 3.13

   .. deprecated:: next

      This function is :term:`soft deprecated`.
