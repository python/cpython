.. currentmodule:: asyncio

Event loops
===========

**Source code:** :source:`Lib/asyncio/events.py`

Event loop functions
--------------------

The following functions are convenient shortcuts to accessing the methods of the
global policy. Note that this provides access to the default policy, unless an
alternative policy was set by calling :func:`set_event_loop_policy` earlier in
the execution of the process.

.. function:: get_event_loop()

   Equivalent to calling ``get_event_loop_policy().get_event_loop()``.

.. function:: set_event_loop(loop)

   Equivalent to calling ``get_event_loop_policy().set_event_loop(loop)``.

.. function:: new_event_loop()

   Equivalent to calling ``get_event_loop_policy().new_event_loop()``.


.. _asyncio-event-loops:

Available event loops
---------------------

asyncio currently provides two implementations of event loops:
:class:`SelectorEventLoop` and :class:`ProactorEventLoop`.

.. class:: SelectorEventLoop

   Event loop based on the :mod:`selectors` module. Subclass of
   :class:`AbstractEventLoop`.

   Use the most efficient selector available on the platform.

   On Windows, only sockets are supported (ex: pipes are not supported):
   see the `MSDN documentation of select
   <https://msdn.microsoft.com/en-us/library/windows/desktop/ms740141%28v=vs.85%29.aspx>`_.

.. class:: ProactorEventLoop

   Proactor event loop for Windows using "I/O Completion Ports" aka IOCP.
   Subclass of :class:`AbstractEventLoop`.

   Availability: Windows.

   .. seealso::

      `MSDN documentation on I/O Completion Ports
      <https://msdn.microsoft.com/en-us/library/windows/desktop/aa365198%28v=vs.85%29.aspx>`_.

Example to use a :class:`ProactorEventLoop` on Windows::

    import asyncio, sys

    if sys.platform == 'win32':
        loop = asyncio.ProactorEventLoop()
        asyncio.set_event_loop(loop)

.. _asyncio-platform-support:

Platform support
----------------

The :mod:`asyncio` module has been designed to be portable, but each platform
still has subtle differences and may not support all :mod:`asyncio` features.

Windows
^^^^^^^

Common limits of Windows event loops:

- :meth:`~AbstractEventLoop.create_unix_connection` and
  :meth:`~AbstractEventLoop.create_unix_server` are not supported: the socket
  family :data:`socket.AF_UNIX` is specific to UNIX
- :meth:`~AbstractEventLoop.add_signal_handler` and
  :meth:`~AbstractEventLoop.remove_signal_handler` are not supported
- :meth:`EventLoopPolicy.set_child_watcher` is not supported.
  :class:`ProactorEventLoop` supports subprocesses. It has only one
  implementation to watch child processes, there is no need to configure it.

:class:`SelectorEventLoop` specific limits:

- :class:`~selectors.SelectSelector` is used which only supports sockets
  and is limited to 512 sockets.
- :meth:`~AbstractEventLoop.add_reader` and :meth:`~AbstractEventLoop.add_writer` only
  accept file descriptors of sockets
- Pipes are not supported
  (ex: :meth:`~AbstractEventLoop.connect_read_pipe`,
  :meth:`~AbstractEventLoop.connect_write_pipe`)
- :ref:`Subprocesses <asyncio-subprocess>` are not supported
  (ex: :meth:`~AbstractEventLoop.subprocess_exec`,
  :meth:`~AbstractEventLoop.subprocess_shell`)

:class:`ProactorEventLoop` specific limits:

- :meth:`~AbstractEventLoop.create_datagram_endpoint` (UDP) is not supported
- :meth:`~AbstractEventLoop.add_reader` and :meth:`~AbstractEventLoop.add_writer` are
  not supported

The resolution of the monotonic clock on Windows is usually around 15.6 msec.
The best resolution is 0.5 msec. The resolution depends on the hardware
(availability of `HPET
<https://en.wikipedia.org/wiki/High_Precision_Event_Timer>`_) and on the Windows
configuration. See :ref:`asyncio delayed calls <asyncio-delayed-calls>`.

.. versionchanged:: 3.5

   :class:`ProactorEventLoop` now supports SSL.


Mac OS X
^^^^^^^^

Character devices like PTY are only well supported since Mavericks (Mac OS
10.9). They are not supported at all on Mac OS 10.5 and older.

On Mac OS 10.6, 10.7 and 10.8, the default event loop is
:class:`SelectorEventLoop` which uses :class:`selectors.KqueueSelector`.
:class:`selectors.KqueueSelector` does not support character devices on these
versions.  The :class:`SelectorEventLoop` can be used with
:class:`~selectors.SelectSelector` or :class:`~selectors.PollSelector` to
support character devices on these versions of Mac OS X. Example::

    import asyncio
    import selectors

    selector = selectors.SelectSelector()
    loop = asyncio.SelectorEventLoop(selector)
    asyncio.set_event_loop(loop)


Event loop policies and the default policy
------------------------------------------

Event loop management is abstracted with a *policy* pattern, to provide maximal
flexibility for custom platforms and frameworks. Throughout the execution of a
process, a single global policy object manages the event loops available to the
process based on the calling context. A policy is an object implementing the
:class:`AbstractEventLoopPolicy` interface.

For most users of :mod:`asyncio`, policies never have to be dealt with
explicitly, since the default global policy is sufficient.

The default policy defines context as the current thread, and manages an event
loop per thread that interacts with :mod:`asyncio`. The module-level functions
:func:`get_event_loop` and :func:`set_event_loop` provide convenient access to
event loops managed by the default policy.


Event loop policy interface
---------------------------

An event loop policy must implement the following interface:

.. class:: AbstractEventLoopPolicy

   Event loop policy.

   .. method:: get_event_loop()

      Get the event loop for the current context.

      Returns an event loop object implementing the :class:`AbstractEventLoop`
      interface.

      Raises an exception in case no event loop has been set for the current
      context and the current policy does not specify to create one. It must
      never return ``None``.

   .. method:: set_event_loop(loop)

      Set the event loop for the current context to *loop*.

   .. method:: new_event_loop()

      Create and return a new event loop object according to this policy's
      rules.

      If there's need to set this loop as the event loop for the current
      context, :meth:`set_event_loop` must be called explicitly.


Access to the global loop policy
--------------------------------

.. function:: get_event_loop_policy()

   Get the current event loop policy.

.. function:: set_event_loop_policy(policy)

   Set the current event loop policy. If *policy* is ``None``, the default
   policy is restored.
