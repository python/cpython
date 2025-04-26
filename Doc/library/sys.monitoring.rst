:mod:`!sys.monitoring` --- Execution event monitoring
=====================================================

.. module:: sys.monitoring
   :synopsis: Access and control event monitoring

.. versionadded:: 3.12

-----------------

.. note::

    :mod:`sys.monitoring` is a namespace within the :mod:`sys` module,
    not an independent module, so there is no need to
    ``import sys.monitoring``, simply ``import sys`` and then use
    ``sys.monitoring``.


This namespace provides access to the functions and constants necessary to
activate and control event monitoring.

As programs execute, events occur that might be of interest to tools that
monitor execution. The :mod:`sys.monitoring` namespace provides means to
receive callbacks when events of interest occur.

The monitoring API consists of three components:

* `Tool identifiers`_
* `Events`_
* :ref:`Callbacks <callbacks>`

Tool identifiers
----------------

A tool identifier is an integer and the associated name.
Tool identifiers are used to discourage tools from interfering with each
other and to allow multiple tools to operate at the same time.
Currently tools are completely independent and cannot be used to
monitor each other. This restriction may be lifted in the future.

Before registering or activating events, a tool should choose an identifier.
Identifiers are integers in the range 0 to 5 inclusive.

Registering and using tools
'''''''''''''''''''''''''''

.. function:: use_tool_id(tool_id: int, name: str, /) -> None

   Must be called before *tool_id* can be used.
   *tool_id* must be in the range 0 to 5 inclusive.
   Raises a :exc:`ValueError` if *tool_id* is in use.

.. function:: clear_tool_id(tool_id: int, /) -> None

   Unregister all events and callback functions associated with *tool_id*.

.. function:: free_tool_id(tool_id: int, /) -> None

   Should be called once a tool no longer requires *tool_id*.
   Will call :func:`clear_tool_id` before releasing *tool_id*.

.. function:: get_tool(tool_id: int, /) -> str | None

   Returns the name of the tool if *tool_id* is in use,
   otherwise it returns ``None``.
   *tool_id* must be in the range 0 to 5 inclusive.

All IDs are treated the same by the VM with regard to events, but the
following IDs are pre-defined to make co-operation of tools easier::

  sys.monitoring.DEBUGGER_ID = 0
  sys.monitoring.COVERAGE_ID = 1
  sys.monitoring.PROFILER_ID = 2
  sys.monitoring.OPTIMIZER_ID = 5


Events
------

The following events are supported:

.. monitoring-event:: BRANCH_LEFT

   A conditional branch goes left.

   It is up to the tool to determine how to present "left" and "right" branches.
   There is no guarantee which branch is "left" and which is "right", except
   that it will be consistent for the duration of the program.

.. monitoring-event:: BRANCH_RIGHT

   A conditional branch goes right.

.. monitoring-event:: CALL

   A call in Python code (event occurs before the call).

.. monitoring-event:: C_RAISE

   An exception raised from any callable, except for Python functions (event occurs after the exit).

.. monitoring-event:: C_RETURN

   Return from any callable, except for Python functions (event occurs after the return).

.. monitoring-event:: EXCEPTION_HANDLED

   An exception is handled.

.. monitoring-event:: INSTRUCTION

   A VM instruction is about to be executed.

.. monitoring-event:: JUMP

   An unconditional jump in the control flow graph is made.

.. monitoring-event:: LINE

   An instruction is about to be executed that has a different line number from the preceding instruction.

.. monitoring-event:: PY_RESUME

   Resumption of a Python function (for generator and coroutine functions), except for ``throw()`` calls.

.. monitoring-event:: PY_RETURN

   Return from a Python function (occurs immediately before the return, the callee's frame will be on the stack).

.. monitoring-event:: PY_START

   Start of a Python function (occurs immediately after the call, the callee's frame will be on the stack)

.. monitoring-event:: PY_THROW

   A Python function is resumed by a ``throw()`` call.

.. monitoring-event:: PY_UNWIND

   Exit from a Python function during exception unwinding.

.. monitoring-event:: PY_YIELD

   Yield from a Python function (occurs immediately before the yield, the callee's frame will be on the stack).

.. monitoring-event:: RAISE

   An exception is raised, except those that cause a :monitoring-event:`STOP_ITERATION` event.

.. monitoring-event:: RERAISE

   An exception is re-raised, for example at the end of a :keyword:`finally` block.

.. monitoring-event:: STOP_ITERATION

   An artificial :exc:`StopIteration` is raised; see `the STOP_ITERATION event`_.


More events may be added in the future.

These events are attributes of the :mod:`!sys.monitoring.events` namespace.
Each event is represented as a power-of-2 integer constant.
To define a set of events, simply bitwise OR the individual events together.
For example, to specify both :monitoring-event:`PY_RETURN` and :monitoring-event:`PY_START`
events, use the expression ``PY_RETURN | PY_START``.

.. monitoring-event:: NO_EVENTS

    An alias for ``0`` so users can do explicit comparisons like::

      if get_events(DEBUGGER_ID) == NO_EVENTS:
          ...

Events are divided into three groups:

.. _monitoring-event-local:

Local events
''''''''''''

Local events are associated with normal execution of the program and happen
at clearly defined locations. All local events can be disabled.
The local events are:

* :monitoring-event:`PY_START`
* :monitoring-event:`PY_RESUME`
* :monitoring-event:`PY_RETURN`
* :monitoring-event:`PY_YIELD`
* :monitoring-event:`CALL`
* :monitoring-event:`LINE`
* :monitoring-event:`INSTRUCTION`
* :monitoring-event:`JUMP`
* :monitoring-event:`BRANCH_LEFT`
* :monitoring-event:`BRANCH_RIGHT`
* :monitoring-event:`STOP_ITERATION`

Deprecated event
''''''''''''''''

* ``BRANCH``

The ``BRANCH`` event is deprecated in 3.14.
Using :monitoring-event:`BRANCH_LEFT` and :monitoring-event:`BRANCH_RIGHT`
events will give much better performance as they can be disabled
independently.

Ancillary events
''''''''''''''''

Ancillary events can be monitored like other events, but are controlled
by another event:

* :monitoring-event:`C_RAISE`
* :monitoring-event:`C_RETURN`

The :monitoring-event:`C_RETURN` and :monitoring-event:`C_RAISE` events
are controlled by the :monitoring-event:`CALL` event.
:monitoring-event:`C_RETURN` and :monitoring-event:`C_RAISE` events will only be seen if the
corresponding :monitoring-event:`CALL` event is being monitored.

Other events
''''''''''''

Other events are not necessarily tied to a specific location in the
program and cannot be individually disabled.

The other events that can be monitored are:

* :monitoring-event:`PY_THROW`
* :monitoring-event:`PY_UNWIND`
* :monitoring-event:`RAISE`
* :monitoring-event:`EXCEPTION_HANDLED`


The STOP_ITERATION event
''''''''''''''''''''''''

:pep:`PEP 380 <380#use-of-stopiteration-to-return-values>`
specifies that a :exc:`StopIteration` exception is raised when returning a value
from a generator or coroutine. However, this is a very inefficient way to
return a value, so some Python implementations, notably CPython 3.12+, do not
raise an exception unless it would be visible to other code.

To allow tools to monitor for real exceptions without slowing down generators
and coroutines, the :monitoring-event:`STOP_ITERATION` event is provided.
:monitoring-event:`STOP_ITERATION` can be locally disabled, unlike :monitoring-event:`RAISE`.

Note that the :monitoring-event:`STOP_ITERATION` event and the :monitoring-event:`RAISE`
event for a :exc:`StopIteration` exception are equivalent, and are treated as interchangeable
when generating events. Implementations will favor :monitoring-event:`STOP_ITERATION` for
performance reasons, but may generate a :monitoring-event:`RAISE` event with a :exc:`StopIteration`.

Turning events on and off
-------------------------

In order to monitor an event, it must be turned on and a corresponding callback
must be registered.
Events can be turned on or off by setting the events either globally or
for a particular code object.


Setting events globally
'''''''''''''''''''''''

Events can be controlled globally by modifying the set of events being monitored.

.. function:: get_events(tool_id: int, /) -> int

   Returns the ``int`` representing all the active events.

.. function:: set_events(tool_id: int, event_set: int, /) -> None

   Activates all events which are set in *event_set*.
   Raises a :exc:`ValueError` if *tool_id* is not in use.

No events are active by default.

Per code object events
''''''''''''''''''''''

Events can also be controlled on a per code object basis. The functions
defined below which accept a :class:`types.CodeType` should be prepared
to accept a look-alike object from functions which are not defined
in Python (see :ref:`c-api-monitoring`).

.. function:: get_local_events(tool_id: int, code: CodeType, /) -> int

   Returns all the local events for *code*

.. function:: set_local_events(tool_id: int, code: CodeType, event_set: int, /) -> None

   Activates all the local events for *code* which are set in *event_set*.
   Raises a :exc:`ValueError` if *tool_id* is not in use.

Local events add to global events, but do not mask them.
In other words, all global events will trigger for a code object,
regardless of the local events.


Disabling events
''''''''''''''''

.. data:: DISABLE

   A special value that can be returned from a callback function to disable
   events for the current code location.

Local events can be disabled for a specific code location by returning
:data:`sys.monitoring.DISABLE` from a callback function. This does not change
which events are set, or any other code locations for the same event.

Disabling events for specific locations is very important for high
performance monitoring. For example, a program can be run under a
debugger with no overhead if the debugger disables all monitoring
except for a few breakpoints.

.. function:: restart_events() -> None

   Enable all the events that were disabled by :data:`sys.monitoring.DISABLE`
   for all tools.


.. _callbacks:

Registering callback functions
------------------------------

To register a callable for events call

.. function:: register_callback(tool_id: int, event: int, func: Callable | None, /) -> Callable | None

   Registers the callable *func* for the *event* with the given *tool_id*

   If another callback was registered for the given *tool_id* and *event*,
   it is unregistered and returned.
   Otherwise :func:`register_callback` returns ``None``.


Functions can be unregistered by calling
``sys.monitoring.register_callback(tool_id, event, None)``.

Callback functions can be registered and unregistered at any time.

Registering or unregistering a callback function will generate a :func:`sys.audit` event.


Callback function arguments
'''''''''''''''''''''''''''

.. data:: MISSING

   A special value that is passed to a callback function to indicate
   that there are no arguments to the call.

When an active event occurs, the registered callback function is called.
Different events will provide the callback function with different arguments, as follows:

* :monitoring-event:`PY_START` and :monitoring-event:`PY_RESUME`::

    func(code: CodeType, instruction_offset: int) -> DISABLE | Any

* :monitoring-event:`PY_RETURN` and :monitoring-event:`PY_YIELD`::

    func(code: CodeType, instruction_offset: int, retval: object) -> DISABLE | Any

* :monitoring-event:`CALL`, :monitoring-event:`C_RAISE` and :monitoring-event:`C_RETURN`::

    func(code: CodeType, instruction_offset: int, callable: object, arg0: object | MISSING) -> DISABLE | Any

  If there are no arguments, *arg0* is set to :data:`sys.monitoring.MISSING`.

* :monitoring-event:`RAISE`, :monitoring-event:`RERAISE`, :monitoring-event:`EXCEPTION_HANDLED`,
  :monitoring-event:`PY_UNWIND`, :monitoring-event:`PY_THROW` and :monitoring-event:`STOP_ITERATION`::

    func(code: CodeType, instruction_offset: int, exception: BaseException) -> DISABLE | Any

* :monitoring-event:`LINE`::

    func(code: CodeType, line_number: int) -> DISABLE | Any

* :monitoring-event:`BRANCH_LEFT`, :monitoring-event:`BRANCH_RIGHT` and :monitoring-event:`JUMP`::

    func(code: CodeType, instruction_offset: int, destination_offset: int) -> DISABLE | Any

  Note that the *destination_offset* is where the code will next execute.

* :monitoring-event:`INSTRUCTION`::

    func(code: CodeType, instruction_offset: int) -> DISABLE | Any
