.. highlight:: c

.. _monitoring:

Monitorong C API
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


.. c:function:: int PyMonitoring_FireBranchEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *target_offset)

   Fire a ``BRANCH`` event.


.. c:function:: int PyMonitoring_FireCReturnEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *retval)

   Fire a ``C_RETURN`` event.


.. c:function:: int PyMonitoring_FirePyThrowEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *exception)

   Fire a ``PY_THROW`` event.


.. c:function:: int PyMonitoring_FireRaiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *exception)

   Fire a ``RAISE`` event.


.. c:function:: int PyMonitoring_FireCRaiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *exception)

   Fire a ``C_RAISE`` event.


.. c:function:: int PyMonitoring_FireReraiseEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *exception)

   Fire a ``RERAISE`` event.


.. c:function:: int PyMonitoring_FireExceptionHandledEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *exception)

   Fire an ``EXCEPTION_HANDLED`` event.


.. c:function:: int PyMonitoring_FirePyUnwindEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *exception)

   Fire a ``PY_UNWIND`` event.


.. c:function:: int PyMonitoring_FireStopIterationEvent(PyMonitoringState *state, PyObject *codelike, int32_t offset, PyObject *exception)

   Fire a ``STOP_ITERATION`` event.


Managing the Monitoring State
-----------------------------

Monitoring states can be managed with the help of monitoring scopes. A scope
would typically correspond to a python function.

.. :c:function:: int PyMonitoring_EnterScope(PyMonitoringState *state_array, uint64_t *version, const uint8_t *event_types, Py_ssize_t length)

   Enter a monitored scope. ``event_types`` is an array of the event IDs for
   events that may be fired from the scope. For example, the ID of a ``PY_START``
   event is the value ``PY_MONITORING_EVENT_PY_START``, which is numerically equal
   to the base-2 logarithm of ``sys.monitoring.events.PY_START``.
   ``state_array`` is an array with a monitoring state entry for each event in
   ``event_types``, it is allocated by the user but populated by
   ``PyMonitoring_EnterScope`` with information about the activation state of
   the event. The size of ``event_types`` (and hence also of ``state_array``)
   is given in ``length``.

   The ``version`` argument is a pointer to a value which should be initialized
   to 0 and then set only by ``PyMonitoring_EnterScope`` itelf. It allows this
   function to determine whether event states have changed since the previous call,
   and to return quickly if they have not.

   The scopes referred to here are lexical scopes: a function, class or method.
   ``PyMonitoring_EnterScope`` should be called whenever the lexical scope
   entered. Scopes can be reentered, reusing the same *state_array* and *version*,
   in situations like when emulating a recursive Python function. When a code-like's
   execution is paused, such as when emulating a generator, the scope needs to
   be exited and re-entered.


.. :c:function:: int PyMonitoring_ExitScope(void)

   Exit the last scope that was entered with ``PyMonitoring_EnterScope``.
