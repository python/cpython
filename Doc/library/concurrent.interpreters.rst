:mod:`!concurrent.interpreters` --- Multiple interpreters in the same process
=============================================================================

.. module:: concurrent.interpreters
   :synopsis: Multiple interpreters in the same process

.. versionadded:: 3.14

**Source code:** :source:`Lib/concurrent/interpreters`

--------------

The :mod:`!concurrent.interpreters` module constructs higher-level
interfaces on top of the lower level :mod:`!_interpreters` module.

The module is primarily meant to provide a basic API for managing
interpreters (AKA "subinterpreters") and running things in them.
Running mostly involves switching to an interpreter (in the current
thread) and calling a function in that execution context.

For concurrency, interpreters themselves (and this module) don't
provide much more than isolation, which on its own isn't useful.
Actual concurrency is available separately through
:mod:`threads <threading>`  See `below <interp-concurrency_>`_

.. seealso::

   :class:`~concurrent.futures.InterpreterPoolExecutor`
      Combines threads with interpreters in a familiar interface.

   .. XXX Add references to the upcoming HOWTO docs in the seealso block.

   :ref:`isolating-extensions-howto`
      How to update an extension module to support multiple interpreters.

   :pep:`554`

   :pep:`734`

   :pep:`684`

.. XXX Why do we disallow multiple interpreters on WASM?

.. include:: ../includes/wasm-notavail.rst


Key details
-----------

Before we dive in further, there are a small number of details
to keep in mind about using multiple interpreters:

* `isolated <interp-isolation_>`_, by default
* no implicit threads
* not all PyPI packages support use in multiple interpreters yet

.. XXX Are there other relevant details to list?


.. _interpreters-intro:

Introduction
------------

An "interpreter" is effectively the execution context of the Python
runtime.  It contains all of the state the runtime needs to execute
a program.  This includes things like the import state and builtins.
(Each thread, even if there's only the main thread, has some extra
runtime state, in addition to the current interpreter, related to
the current exception and the bytecode eval loop.)

The concept and functionality of the interpreter have been a part of
Python since version 2.2, but the feature was only available through
the C-API and not well known, and the `isolation <interp-isolation_>`_
was relatively incomplete until version 3.12.

.. _interp-isolation:

Multiple Interpreters and Isolation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A Python implementation may support using multiple interpreters in the
same process.  CPython has this support.  Each interpreter is
effectively isolated from the others (with a limited number of
carefully managed process-global exceptions to the rule).

That isolation is primarily useful as a strong separation between
distinct logical components of a program, where you want to have
careful control of how those components interact.

.. note::

   Interpreters in the same process can technically never be strictly
   isolated from one another since there are few restrictions on memory
   access within the same process.  The Python runtime makes a best
   effort at isolation but extension modules may easily violate that.
   Therefore, do not use multiple interpreters in security-sensitive
   situations, where they shouldn't have access to each other's data.

Running in an Interpreter
^^^^^^^^^^^^^^^^^^^^^^^^^

Running in a different interpreter involves switching to it in the
current thread and then calling some function.  The runtime will
execute the function using the current interpreter's state.  The
:mod:`!concurrent.interpreters` module provides a basic API for
creating and managing interpreters, as well as the switch-and-call
operation.

No other threads are automatically started for the operation.
There is `a helper <interp-call-in-thread_>`_ for that though.
There is another dedicated helper for calling the builtin
:func:`exec` in an interpreter.

When :func:`exec` (or :func:`eval`) are called in an interpreter,
they run using the interpreter's :mod:`!__main__` module as the
"globals" namespace.  The same is true for functions that aren't
associated with any module.  This is the same as how scripts invoked
from the command-line run in the :mod:`!__main__` module.


.. _interp-concurrency:

Concurrency and Parallelism
^^^^^^^^^^^^^^^^^^^^^^^^^^^

As noted earlier, interpreters do not provide any concurrency
on their own.  They strictly represent the isolated execution
context the runtime will use *in the current thread*.  That isolation
makes them similar to processes, but they still enjoy in-process
efficiency, like threads.

All that said, interpreters do naturally support certain flavors of
concurrency.
There's a powerful side effect of that isolation.  It enables a
different approach to concurrency than you can take with async or
threads.  It's a similar concurrency model to CSP or the actor model,
a model which is relatively easy to reason about.

You can take advantage of that concurrency model in a single thread,
switching back and forth between interpreters, Stackless-style.
However, this model is more useful when you combine interpreters
with multiple threads.  This mostly involves starting a new thread,
where you switch to another interpreter and run what you want there.

Each actual thread in Python, even if you're only running in the main
thread, has its own *current* execution context.  Multiple threads can
use the same interpreter or different ones.

At a high level, you can think of the combination of threads and
interpreters as threads with opt-in sharing.

As a significant bonus, interpreters are sufficiently isolated that
they do not share the :term:`GIL`, which means combining threads with
multiple interpreters enables full multi-core parallelism.
(This has been the case since Python 3.12.)

Communication Between Interpreters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

In practice, multiple interpreters are useful only if we have a way
to communicate between them.  This usually involves some form of
message passing, but can even mean sharing data in some carefully
managed way.

With this in mind, the :mod:`!concurrent.interpreters` module provides
a :class:`queue.Queue` implementation, available through
:func:`create_queue`.

.. _interp-object-sharing:

"Sharing" Objects
^^^^^^^^^^^^^^^^^

Any data actually shared between interpreters loses the thread-safety
provided by the :term:`GIL`.  There are various options for dealing with
this in extension modules.  However, from Python code the lack of
thread-safety means objects can't actually be shared, with a few
exceptions.  Instead, a copy must be created, which means mutable
objects won't stay in sync.

By default, most objects are copied with :mod:`pickle` when they are
passed to another interpreter.  Nearly all of the immutable builtin
objects are either directly shared or copied efficiently.  For example:

* :const:`None`
* :class:`bool` (:const:`True` and :const:`False`)
* :class:`bytes`
* :class:`str`
* :class:`int`
* :class:`float`
* :class:`tuple` (of similarly supported objects)

There is a small number of Python types that actually share mutable
data between interpreters:

* :class:`memoryview`
* :class:`Queue`


Reference
---------

This module defines the following functions:

.. function:: list_all()

   Return a :class:`list` of :class:`Interpreter` objects,
   one for each existing interpreter.

.. function:: get_current()

   Return an :class:`Interpreter` object for the currently running
   interpreter.

.. function:: get_main()

   Return an :class:`Interpreter` object for the main interpreter.
   This is the interpreter the runtime created to run the :term:`REPL`
   or the script given at the command-line.  It is usually the only one.

.. function:: create()

   Initialize a new (idle) Python interpreter
   and return a :class:`Interpreter` object for it.

.. function:: create_queue(maxsize=0, *, unbounditems=UNBOUND)

   Initialize a new cross-interpreter queue and return a :class:`Queue`
   object for it.

   *maxsize* sets the upper bound on the number of items that can be placed
   in the queue.  If *maxsize* is less than or equal to zero, the queue
   size is infinite.

   *unbounditems* sets the default behavior when getting an item from the
   queue whose original interpreter has been destroyed.
   See :meth:`Queue.put` for supported values.

.. function:: is_shareable(obj)

   Return ``True`` if the object can be sent to another interpreter
   without using :mod:`pickle`, and ``False`` otherwise.
   See :ref:`interp-object-sharing`.


Interpreter objects
^^^^^^^^^^^^^^^^^^^

.. class:: Interpreter(id)

   A single interpreter in the current process.

   Generally, :class:`Interpreter` shouldn't be called directly.
   Instead, use :func:`create` or one of the other module functions.

   .. attribute:: id

      (read-only)

      The underlying interpreter's ID.

   .. attribute:: whence

      (read-only)

      A string describing where the interpreter came from.

   .. method:: is_running()

      Return ``True`` if the interpreter is currently executing code
      in its :mod:`!__main__` module and ``False`` otherwise.

   .. method:: close()

      Finalize and destroy the interpreter.

   .. method:: prepare_main(ns=None, /, **kwargs)

      Bind the given objects into the interpreter's :mod:`!__main__`
      module namespace.  This is the primary way to pass data to code
      running in another interpreter.

      *ns* is an optional :class:`dict` mapping names to values.
      Any additional keyword arguments are also bound as names.

      The values must be shareable between interpreters.  Some objects
      are actually shared, some are copied efficiently, and most are
      copied via :mod:`pickle`.  See :ref:`interp-object-sharing`.

      For example::

         interp = interpreters.create()
         interp.prepare_main(name='world')
         interp.exec('print(f"Hello, {name}!")')

      This is equivalent to setting variables in the interpreter's
      :mod:`!__main__` module before calling :meth:`exec` or :meth:`call`.
      The names are available as global variables in the executed code.

   .. method:: exec(code, /)

      Run the given source code in the interpreter (in the current thread).

      *code* is a :class:`str` of Python source code.  It is executed as
      though it were the body of a script, using the interpreter's
      :mod:`!__main__` module as the globals namespace.

      There is no return value.  To get a result back, use :meth:`call`
      instead, or communicate through a :class:`Queue`.

      If the code raises an unhandled exception, an :exc:`ExecutionFailed`
      exception is raised in the calling interpreter.  The actual
      exception object is not preserved because objects cannot be shared
      between interpreters directly.

      This blocks the current thread until the code finishes.

   .. method:: call(callable, /, *args, **kwargs)

      Call *callable* in the interpreter (in the current thread) and
      return the result.

      Nearly all callables, args, kwargs, and return values are supported.
      All "shareable" objects are supported, as are "stateless" functions
      (meaning non-closures that do not use any globals).  For other
      objects, this method falls back to :mod:`pickle`.

      If the callable raises an exception, an :exc:`ExecutionFailed`
      exception is raised in the calling interpreter.

   .. _interp-call-in-thread:

   .. method:: call_in_thread(callable, /, *args, **kwargs)

      Start a new :class:`~threading.Thread` that calls *callable* in
      the interpreter and return the thread object.

      This is a convenience wrapper that combines
      :mod:`threading` with :meth:`call`.  The thread is started
      immediately.  Call :meth:`~threading.Thread.join` on the returned
      thread to wait for it to finish.

Exceptions
^^^^^^^^^^

.. exception:: InterpreterError

   This exception, a subclass of :exc:`Exception`, is raised when
   an interpreter-related error happens.

.. exception:: InterpreterNotFoundError

   This exception, a subclass of :exc:`InterpreterError`, is raised when
   the targeted interpreter no longer exists.

.. exception:: ExecutionFailed

   This exception, a subclass of :exc:`InterpreterError`, is raised when
   the running code raised an uncaught exception.

   .. attribute:: excinfo

      A basic snapshot of the exception raised in the other interpreter.

.. XXX Document the excinfoattrs?

.. exception:: NotShareableError

   This exception, a subclass of :exc:`TypeError`, is raised when
   an object cannot be sent to another interpreter.


Communicating Between Interpreters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. class:: Queue(id)

   A cross-interpreter queue that can be used to pass data safely
   between interpreters.  It provides the same interface as
   :class:`queue.Queue`.  The underlying queue can only be created
   through :func:`create_queue`.

   When an object is placed in the queue, it is prepared for use in
   another interpreter.  Some objects are actually shared and some are
   copied efficiently, but most are copied via :mod:`pickle`.
   See :ref:`interp-object-sharing`.

   :class:`Queue` objects themselves are shareable between interpreters
   (they reference the same underlying queue), making them suitable for
   use with :meth:`Interpreter.prepare_main`.

   .. attribute:: id

      (read-only)

      The queue's ID.

   .. attribute:: maxsize

      (read-only)

      The maximum number of items allowed in the queue.  A value of zero
      means the queue size is infinite.

   .. method:: empty()

      Return ``True`` if the queue is empty, ``False`` otherwise.

   .. method:: full()

      Return ``True`` if the queue is full, ``False`` otherwise.

   .. method:: qsize()

      Return the number of items in the queue.

   .. method:: put(obj, block=True, timeout=None, *, unbounditems=None)

      Put *obj* into the queue.  If *block* is true (the default),
      block if necessary until a free slot is available.  If *timeout*
      is a positive number, block at most *timeout* seconds and raise
      :exc:`QueueFullError` if no free slot is available within that time.

      If *block* is false, put *obj* in the queue if a free slot is
      immediately available, otherwise raise :exc:`QueueFullError`.

      *unbounditems* controls what happens when the item is retrieved
      via :meth:`get` after the interpreter that called :meth:`put` has
      been destroyed.  If ``None`` (the default), the queue's default
      (set via :func:`create_queue`) is used.  Supported values:

      * :data:`UNBOUND` -- :meth:`get` returns the :data:`UNBOUND`
        sentinel in place of the original object.
      * :data:`UNBOUND_ERROR` -- :meth:`get` raises
        :exc:`ItemInterpreterDestroyed`.
      * :data:`UNBOUND_REMOVE` -- the item is silently removed from
        the queue when the original interpreter is destroyed.

   .. method:: put_nowait(obj, *, unbounditems=None)

      Equivalent to ``put(obj, block=False)``.

   .. method:: get(block=True, timeout=None)

      Remove and return an item from the queue.  If *block* is true
      (the default), block if necessary until an item is available.
      If *timeout* is a positive number, block at most *timeout* seconds
      and raise :exc:`QueueEmptyError` if no item is available within
      that time.

      If *block* is false, return an item if one is immediately
      available, otherwise raise :exc:`QueueEmptyError`.

   .. method:: get_nowait()

      Equivalent to ``get(block=False)``.


.. exception:: QueueEmptyError

   This exception, a subclass of :exc:`queue.Empty`, is raised from
   :meth:`!Queue.get` and :meth:`!Queue.get_nowait` when the queue
   is empty.

.. exception:: QueueFullError

   This exception, a subclass of :exc:`queue.Full`, is raised from
   :meth:`!Queue.put` and :meth:`!Queue.put_nowait` when the queue
   is full.


Basic usage
-----------

Creating an interpreter and running code in it::

    from concurrent import interpreters

    interp = interpreters.create()

    # Run source code directly.
    interp.exec('print("Hello from a subinterpreter!")')

    # Call a function and get the result.
    def add(x, y):
        return x + y

    result = interp.call(add, 3, 4)
    print(result)  # 7

    # Run a function in a new thread.
    def worker():
        print('Running in a thread!')

    t = interp.call_in_thread(worker)
    t.join()

Passing data with :meth:`~Interpreter.prepare_main`::

    interp = interpreters.create()

    # Bind variables into the interpreter's __main__ namespace.
    interp.prepare_main(greeting='Hello', name='world')
    interp.exec('print(f"{greeting}, {name}!")')

    # Can also use a dict.
    config = {'host': 'localhost', 'port': 8080}
    interp.prepare_main(config)
    interp.exec('print(f"Connecting to {host}:{port}")')

Using queues to communicate between interpreters::

    interp = interpreters.create()

    # Create a queue and share it with the subinterpreter.
    queue = interpreters.create_queue()
    interp.prepare_main(queue=queue)

    # The subinterpreter puts results into the queue.
    interp.exec("""
    import math
    queue.put(math.factorial(10))
    """)

    # The main interpreter reads from the same queue.
    result = queue.get()
    print(result)  # 3628800

Running CPU-bound work in parallel using threads and interpreters::

    import time
    from concurrent import interpreters

    def compute(n):
        total = sum(range(n))
        return total

    interp1 = interpreters.create()
    interp2 = interpreters.create()

    # Each interpreter runs in its own thread and does not share
    # the GIL, enabling true parallel execution.
    t1 = interp1.call_in_thread(compute, 50_000_000)
    t2 = interp2.call_in_thread(compute, 50_000_000)
    t1.join()
    t2.join()

.. tip::

   For many use cases, :class:`~concurrent.futures.InterpreterPoolExecutor`
   provides a higher-level interface that combines threads with
   interpreters automatically.
