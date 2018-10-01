.. currentmodule:: asyncio


====================
High-level API Index
====================

This page lists all high-level async/await enabled asyncio APIs.


Tasks
=====

Utilities to run asyncio programs, create Tasks, and
await on multiple things with timeouts.

.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :func:`run`
      - Create event loop, run a coroutine, close the loop.

    * - :func:`create_task`
      - Start an asyncio Task.

    * - ``await`` :func:`sleep`
      - Sleep for a number of seconds.

    * - ``await`` :func:`gather`
      - Schedule and wait for things concurrently.

    * - ``await`` :func:`wait_for`
      - Run with a timeout.

    * - ``await`` :func:`shield`
      - Shield from cancellation.

    * - ``await`` :func:`wait`
      - Monitor for completion.

    * - :func:`current_task`
      - Return the current Task.

    * - :func:`all_tasks`
      - Return all tasks for an event loop.

    * - :class:`Task`
      - Task object.

    * - :func:`run_coroutine_threadsafe`
      - Schedule a coroutine from another OS thread.

    * - ``for in`` :func:`as_completed`
      - Monitor for completion with a ``for`` loop.


.. rubric:: Examples

* :ref:`Using asyncio.gather() to run things in parallel
  <asyncio_example_gather>`.

* :ref:`Using asyncio.wait_for() to enforce a timeout
  <asyncio_example_waitfor>`.

* :ref:`Cancellation <asyncio_example_task_cancel>`.

* :ref:`Using asyncio.sleep() <asyncio_example_sleep>`.

* See also the main :ref:`Tasks documentation page <coroutine>`.


Queues
======

Queues should be used to distribute work amongst multiple asyncio Tasks,
implement connection pools, and pub/sub patterns.


.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :class:`Queue`
      - A FIFO queue.

    * - :class:`PriorityQueue`
      - A priority queue.

    * - :class:`LifoQueue`
      - A LIFO queue.


.. rubric:: Examples

* :ref:`Using asyncio.Queue to distribute workload between several
  Tasks <asyncio_example_queue_dist>`.

* See also the :ref:`Queues documentation page <asyncio-queues>`.


Subprocesses
============

Utilities to spawn subprocesses and run shell commands.

.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - ``await`` :func:`create_subprocess_exec`
      - Create a subprocess.

    * - ``await`` :func:`create_subprocess_shell`
      - Run a shell command.


.. rubric:: Examples

* :ref:`Executing a shell command <asyncio_example_subprocess_shell>`.

* See also the :ref:`subprocess APIs <asyncio-subprocess>`
  documentation.


Streams
=======

High-level APIs to work with network IO.

.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - ``await`` :func:`open_connection`
      -  Establish a TCP connection.

    * - ``await`` :func:`open_unix_connection`
      -  Establish a Unix socket connection.

    * - ``await`` :func:`start_server`
      - Start a TCP server.

    * - ``await`` :func:`start_unix_server`
      - Start a Unix socket server.

    * - :class:`StreamReader`
      - High-level async/await object to receive network data.

    * - :class:`StreamWriter`
      - High-level async/await object to send network data.


.. rubric:: Examples

* :ref:`Example TCP client <asyncio_example_stream>`.

* See also the :ref:`streams APIs <asyncio-streams>`
  documentation.


Synchronization
===============

Threading-like synchronization primitives that can be used in Tasks.

.. list-table::
    :widths: 50 50
    :class: full-width-table

    * - :class:`Lock`
      - A mutex lock.

    * - :class:`Event`
      - An event object.

    * - :class:`Condition`
      - A condition object.

    * - :class:`Semaphore`
      - A semaphore.

    * - :class:`BoundedSemaphore`
      - A bounded semaphore.


.. rubric:: Examples

* :ref:`Using asyncio.Event <asyncio_example_sync_event>`.

* See also the documentation of asyncio
  :ref:`synchronization primitives <asyncio-sync>`.


Exceptions
==========

.. list-table::
    :widths: 50 50
    :class: full-width-table


    * - :exc:`asyncio.TimeoutError`
      - Raised on timeout by functions like :func:`wait_for`.
        Keep in mind that ``asyncio.TimeoutError`` is **unrelated**
        to the built-in :exc:`TimeoutError` exception.

    * - :exc:`asyncio.CancelledError`
      - Raised when a Task is cancelled. See also :meth:`Task.cancel`.


.. rubric:: Examples

* :ref:`Handling CancelledError to run code on cancellation request
  <asyncio_example_task_cancel>`.

* See also the full list of
  :ref:`asyncio-specific exceptions <asyncio-exceptions>`.
