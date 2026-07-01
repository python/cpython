.. currentmodule:: asyncio

.. _asyncio-guest:

==========
Guest Mode
==========

**Source code:** :source:`Lib/asyncio/guest.py`

----

Running asyncio as a Guest in Another Event Loop
=================================================

*Guest mode* allows asyncio to run cooperatively inside a *host* event loop
such as a GUI toolkit's main loop (Tkinter, Qt, GTK, etc.).  Instead of
replacing the host loop, asyncio piggybacks on it:

* The **host thread** keeps running its own main loop as usual.
* A **background daemon thread** blocks on the selector (I/O polling).
  When I/O events arrive it hands them back to the host thread via a
  thread-safe callback.
* The host thread then runs :meth:`~asyncio.BaseEventLoop.process_events`
  and :meth:`~asyncio.BaseEventLoop.process_ready` to advance the asyncio
  event loop by one step, then signals the background thread to poll again.

This dual-thread architecture means neither the host loop nor the asyncio
loop starves the other.

Typical use cases:

* Incrementally migrating a Tkinter/Qt/GTK application to ``async/await``
  without replacing the native event loop.
* Embedding asyncio I/O (HTTP clients, websockets, …) inside a GUI app.
* Running asyncio alongside a framework that owns the main thread.

.. rubric:: Example

See :source:`Doc/includes/asyncio_guest_tkinter.py` for a complete Tkinter
example that embeds asyncio inside ``tkinter.mainloop()`` using
:func:`start_guest_run`.

.. rubric:: API

.. function:: start_guest_run(async_fn, *args, run_sync_soon_threadsafe, done_callback)

   Run *async_fn* as a guest inside another event loop.

   The host event loop (e.g. ``tkinter.mainloop()``) remains in control of the
   main thread.  asyncio I/O polling runs in a daemon background thread and
   dispatches work back to the host thread via *run_sync_soon_threadsafe*.

   :param async_fn: The async function to run as the top-level coroutine.
   :param args: Positional arguments forwarded to *async_fn*.
   :param run_sync_soon_threadsafe: A callable that schedules a zero-argument
       callable on the host event loop's thread in a thread-safe manner.
       For Tkinter use ``widget.after(0, fn)``; for Qt use a
       ``QMetaObject.invokeMethod`` wrapper; etc.
   :param done_callback: Called on the host thread when *async_fn* finishes.
       Receives the completed :class:`Task` as its sole argument.  Inspect
       the outcome with :meth:`Task.result`, :meth:`Task.exception`, or
       :meth:`Task.cancelled`.
   :returns: The :class:`Task` wrapping *async_fn*.

   To cancel the task from the host thread, use::

       loop.call_soon_threadsafe(task.cancel)

   This wakes the I/O thread from its selector wait so cancellation is
   processed promptly.

   .. versionadded:: 3.15

.. rubric:: Low-level Event Loop Methods

The following three methods on :class:`BaseEventLoop` are used internally by
:func:`start_guest_run`.  They decompose :meth:`~BaseEventLoop._run_once`
into independently callable steps and are documented here for completeness.

.. method:: loop.poll_events()

   Poll for I/O events without processing them.

   Cleans up cancelled scheduled handles, computes an appropriate timeout
   from the scheduled callbacks, and calls the underlying selector.  Returns
   the raw event list.

   Together with :meth:`~BaseEventLoop.process_events` and
   :meth:`~BaseEventLoop.process_ready`, this method decomposes
   :meth:`~BaseEventLoop._run_once` into independently callable steps so that
   an external event loop can drive asyncio (see :func:`start_guest_run`).

   .. versionadded:: 3.15

.. method:: loop.process_events(event_list)

   Process I/O events returned by :meth:`~BaseEventLoop.poll_events`.

   Delegates to the selector-specific ``_process_events`` implementation
   which turns raw selector events into ready callbacks.

   .. versionadded:: 3.15

.. method:: loop.process_ready()

   Process expired timers and execute ready callbacks.

   Moves scheduled callbacks whose deadline has passed into the ready queue,
   then runs all callbacks that were ready at call time.  Callbacks enqueued
   *by* running callbacks are left for the next iteration.

   .. versionadded:: 3.15
