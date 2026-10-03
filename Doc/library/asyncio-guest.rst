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
* A **background I/O thread** blocks on the selector (I/O polling).
  When I/O events arrive it hands them back to the host thread via a
  thread-safe callback.  The thread is not a daemon thread; it is joined
  when the guest run finishes.
* The host thread then runs
  :meth:`loop.process_events() <asyncio.loop.process_events>` and
  :meth:`loop.process_ready() <asyncio.loop.process_ready>` to advance
  the asyncio event loop by one step, then signals the I/O thread to
  poll again.

Exactly one of the two threads touches the event loop at any moment, so
neither the host loop nor the asyncio loop starves the other.

Typical use cases:

* Incrementally migrating a Tkinter/Qt/GTK application to ``async/await``
  without replacing the native event loop.
* Embedding asyncio I/O (HTTP clients, websockets, …) inside a GUI app.
* Running asyncio alongside a framework that owns the main thread.

.. rubric:: Example

See :source:`Doc/includes/asyncio_guest_tkinter.py` for a complete Tkinter
example that embeds asyncio inside ``tkinter.mainloop()`` using
:func:`start_guest_run`.

.. seealso::

   The `asyncio-guest <https://github.com/congzhangzh/asyncio-guest>`__
   project — the proof of concept this feature is based on — has runnable
   examples for many more hosts: Tkinter, Qt (PyQt5/PySide6), GTK,
   pygame, Win32 and Tornado.

   `Trio's guest mode
   <https://trio.readthedocs.io/en/stable/reference-lowlevel.html#using-guest-mode-to-run-trio-on-top-of-other-event-loops>`__,
   which pioneered this approach.

.. rubric:: API

.. function:: start_guest_run(async_fn, *args, run_sync_soon_threadsafe, done_callback)

   Run *async_fn* as a guest inside another event loop.

   Must be called from the host event loop's thread.  The host loop
   (e.g. ``tkinter.mainloop()``) remains in control of that thread;
   asyncio I/O polling runs in a background non-daemon thread that is
   joined when the run finishes.

   :param async_fn: The async function to run as the top-level coroutine.
   :param args: Positional arguments forwarded to *async_fn*.
   :param run_sync_soon_threadsafe: A callable that schedules a zero-argument
       callable on the host event loop's thread.  It must be thread-safe,
       must not block, and must not raise; it need not preserve ordering.
       For Tkinter use a ``root.call('after', 'idle', ...)`` wrapper; for
       Qt use a ``QMetaObject.invokeMethod`` wrapper; etc.
   :param done_callback: Called on the host thread after the run has fully
       finished and the loop is closed (see :ref:`asyncio-guest-lifecycle`).
       Receives the :class:`Task` as its sole argument.  Inspect the
       outcome with :meth:`Task.result`, :meth:`Task.exception`, or
       :meth:`Task.cancelled`.
   :returns: The :class:`Task` wrapping *async_fn*.

   To cancel the task from the host, use::

       loop.call_soon_threadsafe(task.cancel)

   This wakes the I/O thread from its selector wait so cancellation is
   processed promptly.

   .. versionadded:: 3.16

.. _asyncio-guest-lifecycle:

Lifecycle and Cleanup
=====================

For the whole guest run the guest loop is the host thread's running
loop: :func:`get_running_loop` works inside guest tasks,
:meth:`loop.is_running() <asyncio.loop.is_running>` returns ``True``, and
starting another event loop on that thread — including a nested
:func:`asyncio.run` or :meth:`loop.run_until_complete` — raises
:exc:`RuntimeError`.  Consequently a thread that is already running an
asyncio event loop cannot start a guest run.

When the main task finishes, cleanup equivalent to :func:`asyncio.run`
takes place on the host thread: remaining tasks are cancelled,
asynchronous generators and the default executor are shut down, the I/O
thread is joined, and the loop is closed.  Only then is *done_callback*
invoked.

If the interpreter exits while a guest run is unfinished, the run is
abandoned: the I/O thread is woken and joined so that interpreter
shutdown does not hang, pending tasks are not cancelled, and
*done_callback* is not called.

Signal Handling
===============

In guest mode the *host* owns signal handling:

* The guest loop never touches :func:`signal.set_wakeup_fd`, neither to
  install a file descriptor nor to reset it on close, so the host's
  signal wake-up pipeline stays intact.
* :meth:`loop.add_signal_handler` and :meth:`loop.remove_signal_handler`
  raise :exc:`RuntimeError`.
* To let asyncio code react to a signal, catch it in the host (with
  :func:`signal.signal` or the host framework's facilities) and forward
  it into the loop with :meth:`loop.call_soon_threadsafe`.

Host Requirements
=================

* *run_sync_soon_threadsafe* must be thread-safe, non-blocking, and must
  not raise.  It may run callbacks in any order.
* Host code running *outside* guest callbacks (for example a GUI button
  handler) must interact with the loop exclusively through
  :meth:`loop.call_soon_threadsafe`, even though it runs on the loop's
  own thread: the I/O thread may be inside the selector, and only
  ``call_soon_threadsafe`` wakes it safely.
* :meth:`loop.stop` is not supported in guest mode.

.. rubric:: Low-level Event Loop Methods

:func:`start_guest_run` drives the loop through three low-level methods
-- :meth:`loop.poll_events() <asyncio.loop.poll_events>`,
:meth:`loop.process_events() <asyncio.loop.process_events>`, and
:meth:`loop.process_ready() <asyncio.loop.process_ready>` -- which
decompose a single iteration of the event loop into independently
callable steps.  See :ref:`asyncio-event-loop` for their reference
documentation.
