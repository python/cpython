.. module:: asyncio

Tasks and coroutines
====================

.. _coroutine:

Coroutines
----------

A coroutine is a generator that follows certain conventions.  For
documentation purposes, all coroutines should be decorated with
``@asyncio.coroutine``, but this cannot be strictly enforced.

Coroutines use the ``yield from`` syntax introduced in :pep:`380`,
instead of the original ``yield`` syntax.

The word "coroutine", like the word "generator", is used for two
different (though related) concepts:

- The function that defines a coroutine (a function definition
  decorated with ``asyncio.coroutine``).  If disambiguation is needed
  we will call this a *coroutine function*.

- The object obtained by calling a coroutine function.  This object
  represents a computation or an I/O operation (usually a combination)
  that will complete eventually.  If disambiguation is needed we will
  call it a *coroutine object*.

Things a coroutine can do:

- ``result = yield from future`` -- suspends the coroutine until the
  future is done, then returns the future's result, or raises an
  exception, which will be propagated.  (If the future is cancelled,
  it will raise a ``CancelledError`` exception.)  Note that tasks are
  futures, and everything said about futures also applies to tasks.

- ``result = yield from coroutine`` -- wait for another coroutine to
  produce a result (or raise an exception, which will be propagated).
  The ``coroutine`` expression must be a *call* to another coroutine.

- ``return expression`` -- produce a result to the coroutine that is
  waiting for this one using ``yield from``.

- ``raise exception`` -- raise an exception in the coroutine that is
  waiting for this one using ``yield from``.

Calling a coroutine does not start its code running -- it is just a
generator, and the coroutine object returned by the call is really a
generator object, which doesn't do anything until you iterate over it.
In the case of a coroutine object, there are two basic ways to start
it running: call ``yield from coroutine`` from another coroutine
(assuming the other coroutine is already running!), or convert it to a
:class:`Task`.

Coroutines (and tasks) can only run when the event loop is running.


Task
----

.. class:: Task(coro, \*, loop=None)

   A coroutine wrapped in a :class:`~concurrent.futures.Future`.

   .. classmethod:: all_tasks(loop=None)

      Return a set of all tasks for an event loop.

      By default all tasks for the current event loop are returned.

   .. method:: cancel()

      Cancel the task.

   .. method:: get_stack(self, \*, limit=None)

      Return the list of stack frames for this task's coroutine.

      If the coroutine is active, this returns the stack where it is suspended.
      If the coroutine has completed successfully or was cancelled, this
      returns an empty list.  If the coroutine was terminated by an exception,
      this returns the list of traceback frames.

      The frames are always ordered from oldest to newest.

      The optional limit gives the maximum nummber of frames to return; by
      default all available frames are returned.  Its meaning differs depending
      on whether a stack or a traceback is returned: the newest frames of a
      stack are returned, but the oldest frames of a traceback are returned.
      (This matches the behavior of the traceback module.)

      For reasons beyond our control, only one stack frame is returned for a
      suspended coroutine.

   .. method:: print_stack(\*, limit=None, file=None)

      Print the stack or traceback for this task's coroutine.

      This produces output similar to that of the traceback module, for the
      frames retrieved by get_stack().  The limit argument is passed to
      get_stack().  The file argument is an I/O stream to which the output
      goes; by default it goes to sys.stderr.


Task functions
--------------

.. function:: as_completed(fs, *, loop=None, timeout=None)

   Return an iterator whose values, when waited for, are
   :class:`~concurrent.futures.Future` instances.

   Raises :exc:`TimeoutError` if the timeout occurs before all Futures are done.

   Example::

       for f in as_completed(fs):
           result = yield from f  # The 'yield from' may raise
           # Use result

   .. note::

      The futures ``f`` are not necessarily members of fs.

.. function:: async(coro_or_future, *, loop=None)

   Wrap a :ref:`coroutine <coroutine>` in a future.

   If the argument is a :class:`~concurrent.futures.Future`, it is returned
   directly.

.. function:: gather(*coros_or_futures, loop=None, return_exceptions=False)

   Return a future aggregating results from the given coroutines or futures.

   All futures must share the same event loop.  If all the tasks are done
   successfully, the returned future's result is the list of results (in the
   order of the original sequence, not necessarily the order of results
   arrival).  If *result_exception* is True, exceptions in the tasks are
   treated the same as successful results, and gathered in the result list;
   otherwise, the first raised exception will be immediately propagated to the
   returned future.

   Cancellation: if the outer Future is cancelled, all children (that have not
   completed yet) are also cancelled.  If any child is cancelled, this is
   treated as if it raised :exc:`~concurrent.futures.CancelledError` -- the
   outer Future is *not* cancelled in this case.  (This is to prevent the
   cancellation of one child to cause other children to be cancelled.)

.. function:: tasks.iscoroutinefunction(func)

   Return ``True`` if *func* is a decorated coroutine function.

.. function:: tasks.iscoroutine(obj)

   Return ``True`` if *obj* is a coroutine object.

.. function:: sleep(delay, result=None, \*, loop=None)

   Create a :ref:`coroutine <coroutine>` that completes after a given time
   (in seconds).

.. function:: shield(arg, \*, loop=None)

   Wait for a future, shielding it from cancellation.

   The statement::

       res = yield from shield(something())

   is exactly equivalent to the statement::

       res = yield from something()

   *except* that if the coroutine containing it is cancelled, the task running
   in ``something()`` is not cancelled.  From the point of view of
   ``something()``, the cancellation did not happen.  But its caller is still
   cancelled, so the yield-from expression still raises
   :exc:`~concurrent.futures.CancelledError`.  Note: If ``something()`` is
   cancelled by other means this will still cancel ``shield()``.

   If you want to completely ignore cancellation (not recommended) you can
   combine ``shield()`` with a try/except clause, as follows::

       try:
           res = yield from shield(something())
       except CancelledError:
           res = None

.. function:: wait(fs, \*, loop=None, timeout=None, return_when=ALL_COMPLETED)

   Wait for the Futures and coroutines given by fs to complete. Coroutines will
   be wrapped in Tasks.  Returns two sets of
   :class:`~concurrent.futures.Future`: (done, pending).

   *timeout* can be used to control the maximum number of seconds to wait before
   returning.  *timeout* can be an int or float.  If *timeout* is not specified
   or ``None``, there is no limit to the wait time.

   *return_when* indicates when this function should return.  It must be one of
   the following constants of the :mod`concurrent.futures` module:

   .. tabularcolumns:: |l|L|

   +-----------------------------+----------------------------------------+
   | Constant                    | Description                            |
   +=============================+========================================+
   | :const:`FIRST_COMPLETED`    | The function will return when any      |
   |                             | future finishes or is cancelled.       |
   +-----------------------------+----------------------------------------+
   | :const:`FIRST_EXCEPTION`    | The function will return when any      |
   |                             | future finishes by raising an          |
   |                             | exception.  If no future raises an     |
   |                             | exception then it is equivalent to     |
   |                             | :const:`ALL_COMPLETED`.                |
   +-----------------------------+----------------------------------------+
   | :const:`ALL_COMPLETED`      | The function will return when all      |
   |                             | futures finish or are cancelled.       |
   +-----------------------------+----------------------------------------+

   This function returns a :ref:`coroutine <coroutine>`.

   Usage::

        done, pending = yield from asyncio.wait(fs)

   .. note::

      This does not raise :exc:`TimeoutError`! Futures that aren't done when
      the timeout occurs are returned in the second set.


.. _asyncio-hello-world-coroutine:

Example: Hello World (coroutine)
--------------------------------

Print ``Hello World`` every two seconds, using a coroutine::

    import asyncio

    @asyncio.coroutine
    def greet_every_two_seconds():
        while True:
            print('Hello World')
            yield from asyncio.sleep(2)

    loop = asyncio.get_event_loop()
    loop.run_until_complete(greet_every_two_seconds())


.. seealso::

   :ref:`Hello World example using a callback <asyncio-hello-world-callback>`.
