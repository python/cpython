:mod:`sys.monitoring` --- Execution event monitoring
====================================================

..  module:: sys.monitoring
    :synopsis: Access and control event monitoring

-----------------

.. note::

    ``sys.monitoring`` is a namespace within the ``sys`` module,
    not an independent module, so there is no need to
    ``import sys.monitoring``, simply ``import sys`` and then use
    ``sys.monitoring``.


This namespace provides access to the functions and constants necessary to
activate and control event monitoring.

As programs execute, events occur that might be of interest to tools that
monitor execution. The :mod:`!sys.monitoring` namespace provides means to
receive callbacks when events of interest occur.

The monitoring API consists of three components:

* Tool identifiers
* Events
* Callbacks

Tool identifiers
----------------

A tool identifier is an integer and associated name.
Tool identifiers are used to discourage tools from interfering with each
other and to allow multiple tools to operate at the same time.
Currently tools are completely independent and cannot be used to
monitor each other. This restriction may be lifted in the future.

Before registering or activating events, a tool should choose an identifier.
Identifiers are integers in the range 0 to 5.

Registering and using tools
'''''''''''''''''''''''''''

.. function:: use_tool_id(id: int, name: str) -> None

   Must be called before ``id`` can be used.
   ``id`` must be in the range 0 to 5 inclusive.
   Raises a ``ValueError`` if ``id`` is in use.

.. function:: free_tool_id(id: int) -> None

   Should be called once a tool no longer requires ``id``.

.. function:: get_tool(id: int) -> str | None

   Returns the name of the tool if ``id`` is in use,
   otherwise it returns ``None``.
   ``id`` must be in the range 0 to 5 inclusive.

All IDs are treated the same by the VM with regard to events, but the
following IDs are pre-defined to make co-operation of tools easier::

  sys.monitoring.DEBUGGER_ID = 0
  sys.monitoring.COVERAGE_ID = 1
  sys.monitoring.PROFILER_ID = 2
  sys.monitoring.OPTIMIZER_ID = 5

There is no obligation to set an ID, nor is there anything preventing a tool
from using an ID even it is already in use.
However, tools are encouraged to use a unique ID and respect other tools.

Events
------

The following events are supported:

BRANCH
  A conditional branch is taken (or not).
CALL
  A call in Python code (event occurs before the call).
C_RAISE
  Exception raised from any callable, except Python functions (event occurs after the exit).
C_RETURN
  Return from any callable, except Python functions (event occurs after the return).
EXCEPTION_HANDLED
  An exception is handled.
INSTRUCTION
  A VM instruction is about to be executed.
JUMP
  An unconditional jump in the control flow graph is made.
LINE
  An instruction is about to be executed that has a different line number from the preceding instruction.
PY_RESUME
  Resumption of a Python function (for generator and coroutine functions), except for throw() calls.
PY_RETURN
  Return from a Python function (occurs immediately before the return, the callee's frame will be on the stack).
PY_START
  Start of a Python function (occurs immediately after the call, the callee's frame will be on the stack)
PY_THROW
  A Python function is resumed by a throw() call.
PY_UNWIND
  Exit from a Python function during exception unwinding.
PY_YIELD
  Yield from a Python function (occurs immediately before the yield, the callee's frame will be on the stack).
RAISE
  An exception is raised, except those that cause a ``STOP_ITERATION`` event.
RERAISE
  An exception is re-raised, for example at the end of a ``finally`` block.
STOP_ITERATION
  An artificial ``StopIteration`` is raised; see `the STOP_ITERATION event`_.

More events may be added in the future.

These events are attributes of the :mod:`!sys.monitoring.events` namespace.
Each event is represented as a power-of-2 integer constant.
To define a set of events, simply bitwise or the individual events together.
For example, to specify both ``PY_RETURN`` and ``PY_START`` events, use the
expression ``PY_RETURN | PY_START``.

Events are divided into three groups:

Local events
''''''''''''

Local events are associated with normal execution of the program and happen
at clearly defined locations. All local events can be disabled.
The local events are:

* PY_START
* PY_RESUME
* PY_RETURN
* PY_YIELD
* CALL
* LINE
* INSTRUCTION
* JUMP
* BRANCH
* STOP_ITERATION

Ancillary events
''''''''''''''''

Ancillary events can be monitored like other events, but are controlled
by another event:

* C_RAISE
* C_RETURN

The ``C_RETURN`` and ``C_RAISE`` events are controlled by the ``CALL``
event. ``C_RETURN`` and ``C_RAISE`` events will only be seen if the
corresponding ``CALL`` event is being monitored.

Other events
''''''''''''

Other events are not necessarily tied to a specific location in the
program and cannot be individually disabled.

The other events that can be monitored are:

* PY_THROW
* PY_UNWIND
* RAISE
* EXCEPTION_HANDLED


The STOP_ITERATION event
''''''''''''''''''''''''

:pep:`PEP 380 <380#use-of-stopiteration-to-return-values>`
specifies that a ``StopIteration`` exception is raised when returning a value
from a generator or coroutine. However, this is a very inefficient way to
return a value, so some Python implementations, notably CPython 3.12+, do not
raise an exception unless it would be visible to other code.

To allow tools to monitor for real exceptions without slowing down generators
and coroutines, the ``STOP_ITERATION`` event is provided.
``STOP_ITERATION`` can be locally disabled, unlike ``RAISE``.


Turning events on and off
-------------------------

In order to monitor an event, it must be turned on and a callback registered.
Events can be turned on or off by setting the events either globally or
for a particular code object.


Setting events globally
'''''''''''''''''''''''

Events can be controlled globally by modifying the set of events being monitored.

.. function:: get_events(tool_id: int) -> int

   Returns the ``int`` representing all the active events.

.. function:: set_events(tool_id: int, event_set: int)

   Activates all events which are set in ``event_set``.
   Raises a ``ValueError`` if ``tool_id`` is not in use.

No events are active by default.

Per code object events
''''''''''''''''''''''

Events can also be controlled on a per code object basis.

.. function:: get_local_events(tool_id: int, code: CodeType) -> int

   Returns all the local events for ``code``

.. function:: set_local_events(tool_id: int, code: CodeType, event_set: int)

   Activates all the local events for ``code`` which are set in ``event_set``.
   Raises a ``ValueError`` if ``tool_id`` is not in use.

Local events add to global events, but do not mask them.
In other words, all global events will trigger for a code object,
regardless of the local events.


Disabling events
''''''''''''''''

Local events can be disabled for a specific code location by returning
``sys.monitoring.DISABLE`` from a callback function. This does not change
which events are set, or any other code locations for the same event.

Disabling events for specific locations is very important for high
performance monitoring. For example, a program can be run under a
debugger with no overhead if the debugger disables all monitoring
except for a few breakpoints.


Registering callback functions
------------------------------

To register a callable for events call

.. function:: register_callback(tool_id: int, event: int, func: Callable | None) -> Callable | None

   Registers the callable ``func`` for the ``event`` with the given ``tool_id``

   If another callback was registered for the given ``tool_id`` and ``event``,
   it is unregistered and returned.
   Otherwise ``register_callback`` returns ``None``.


Functions can be unregistered by calling
``sys.monitoring.register_callback(tool_id, event, None)``.

Callback functions can be registered and unregistered at any time.

Registering or unregistering a callback function will generate a ``sys.audit`` event.


Callback function arguments
'''''''''''''''''''''''''''

When an active event occurs, the registered callback function is called.
Different events will provide the callback function with different arguments, as follows:

* ``PY_START`` and ``PY_RESUME``::

    func(code: CodeType, instruction_offset: int) -> DISABLE | Any

* ``PY_RETURN`` and ``PY_YIELD``:

    ``func(code: CodeType, instruction_offset: int, retval: object) -> DISABLE | Any``

* ``CALL``, ``C_RAISE`` and ``C_RETURN``:

    ``func(code: CodeType, instruction_offset: int, callable: object, arg0: object | MISSING) -> DISABLE | Any``

    If there are no arguments, ``arg0`` is set to ``MISSING``.

* ``RAISE``, ``RERAISE``, ``EXCEPTION_HANDLED``, ``PY_UNWIND``, ``PY_THROW`` and ``STOP_ITERATION``:

    ``func(code: CodeType, instruction_offset: int, exception: BaseException) -> DISABLE | Any``

* ``LINE``:

    ``func(code: CodeType, line_number: int) -> DISABLE | Any``

* ``BRANCH`` and ``JUMP``:

    ``func(code: CodeType, instruction_offset: int, destination_offset: int) -> DISABLE | Any``

  Note that the ``destination_offset`` is where the code will next execute.
  For an untaken branch this will be the offset of the instruction following
  the branch.

* ``INSTRUCTION``:

    ``func(code: CodeType, instruction_offset: int) -> DISABLE | Any``


