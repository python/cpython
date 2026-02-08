.. _asyncio-tutorial:

****************
Asyncio Tutorial
****************

This tutorial teaches you how to write concurrent Python programs using
:mod:`asyncio`.  You will start with simple examples and progressively build up
to a working TCP chat server.

You should be comfortable with basic Python --- functions, classes, and context
managers --- but no prior asyncio experience is required.

.. seealso::

   :ref:`a-conceptual-overview-of-asyncio`
      An explanation of how asyncio works under the hood.

   :mod:`asyncio` reference documentation
      The complete API reference.


.. _asyncio-tutorial-why:

Why asyncio?
============

Many programs spend most of their time *waiting*: waiting for a network
response, waiting for data from a file, waiting for a user to type something.
While one operation waits, the program could be doing other useful work.
This is the problem asyncio solves.

You might be thinking: "threads can do that too."  They can, but threads come
with a cost.  When multiple threads share data, you need locks to prevent
race conditions, and those locks are easy to get wrong.  Bugs caused by
incorrect locking are notoriously hard to reproduce and debug.

Asyncio takes a different approach.  Your code runs in a single thread, and
you explicitly mark every point where execution can switch to another task
using the ``await`` keyword.  Between any two ``await``\s, your code has
exclusive access to all shared data --- no locks required.  This makes
concurrent programs easier to write and easier to reason about.

Asyncio is a great fit when your program is **I/O-bound**: network servers,
web clients, database applications, and similar workloads.  It is not a good
fit for CPU-bound computation (number crunching, image processing), though it
can integrate with threads and processes for those cases, as you will see
later in this tutorial.


.. _asyncio-tutorial-first-program:

Your first async program
========================

An *asynchronous function* (also called a *coroutine function*) is defined
with :keyword:`async def` instead of plain :keyword:`def`::

   import asyncio

   async def main():
       print('Hello')
       await asyncio.sleep(1)
       print('World')

   asyncio.run(main())

.. code-block:: none

   Hello
   World

There are three things to notice:

1. ``async def main()`` defines a coroutine function.  Calling it returns a
   *coroutine object* --- it does not execute the function body.

2. :func:`asyncio.run` is the entry point.  It creates an event loop, runs the
   coroutine until it completes, and then shuts down the loop.  Call it once,
   at the top level of your program.

3. ``await`` suspends the current coroutine until the awaited operation
   finishes.  While ``main()`` is suspended on :func:`asyncio.sleep`,
   the event loop is free to run other tasks.

Let's add a helper to see how ``await`` affects timing::

   import asyncio
   import time

   async def say_after(delay, message):
       await asyncio.sleep(delay)
       print(message)

   async def main():
       start = time.time()

       await say_after(1, 'hello')
       await say_after(2, 'world')

       print(f'Finished in {time.time() - start:.1f} seconds')

   asyncio.run(main())

.. code-block:: none

   hello
   world
   Finished in 3.0 seconds

The two calls run *sequentially* --- just like regular function calls.  The
total time is 1 + 2 = 3 seconds.  To do better, we need to run them
concurrently.


.. _asyncio-tutorial-concurrent-tasks:

Running tasks concurrently
==========================

A :class:`~asyncio.Task` wraps a coroutine and schedules it to run
concurrently with other tasks.  Use :func:`asyncio.create_task` to create
one::

   async def main():
       start = time.time()

       task1 = asyncio.create_task(say_after(1, 'hello'))
       task2 = asyncio.create_task(say_after(2, 'world'))

       await task1
       await task2

       print(f'Finished in {time.time() - start:.1f} seconds')

   asyncio.run(main())

.. code-block:: none

   hello
   world
   Finished in 2.0 seconds

Both tasks start immediately when created.  The ``await`` statements just wait
for each task to finish.  Because the tasks run concurrently, the total time is
2 seconds instead of 3.

Using TaskGroup
---------------

:class:`asyncio.TaskGroup` is the recommended way to manage concurrent tasks.
It ensures that all tasks are properly awaited and cleaned up, even if one of
them fails::

   async def main():
       start = time.time()

       async with asyncio.TaskGroup() as tg:
           tg.create_task(say_after(1, 'hello'))
           tg.create_task(say_after(2, 'world'))

       # Both tasks are guaranteed to be done here.
       print(f'Finished in {time.time() - start:.1f} seconds')

   asyncio.run(main())

The ``async with`` block waits for all tasks in the group to complete.  If any
task raises an exception, the remaining tasks are cancelled and the exceptions
are collected into an :exc:`ExceptionGroup`::

   async def fail():
       raise ValueError('something went wrong')

   async def main():
       try:
           async with asyncio.TaskGroup() as tg:
               tg.create_task(say_after(1, 'hello'))
               tg.create_task(fail())
       except* ValueError as eg:
           for exc in eg.exceptions:
               print(f'Caught: {exc}')

   asyncio.run(main())

.. code-block:: none

   Caught: something went wrong

The ``say_after`` task is automatically cancelled when ``fail()`` raises.
This structured approach prevents tasks from being accidentally lost or
silently failing.

.. note::

   The ``except*`` syntax handles :exc:`ExceptionGroup`\s.  See
   :ref:`tut-exception-groups` for details.


.. _asyncio-tutorial-timeouts:

Handling timeouts
=================

Use :func:`asyncio.timeout` to set a time limit on an async operation::

   import asyncio

   async def slow_operation():
       await asyncio.sleep(10)
       return 'done'

   async def main():
       try:
           async with asyncio.timeout(2):
               result = await slow_operation()
               print(f'Result: {result}')
       except TimeoutError:
           print('Operation timed out!')

   asyncio.run(main())

.. code-block:: none

   Operation timed out!

When the timeout expires, the inner coroutine is cancelled and a
:exc:`TimeoutError` is raised.  The ``except`` block must be *outside* the
``async with`` block.

.. seealso::

   :func:`asyncio.timeout` for rescheduling deadlines, and
   :func:`asyncio.timeout_at` for absolute deadlines.


.. _asyncio-tutorial-queues:

Producer-consumer with queues
=============================

A common concurrency pattern is the *producer-consumer* pattern: one or more
tasks produce work items, and one or more tasks consume them.
:class:`asyncio.Queue` provides a safe way to pass items between tasks::

   import asyncio
   import random

   async def producer(queue, urls):
       """Add URLs to the queue for processing."""
       for url in urls:
           await queue.put(url)
           print(f'  Queued {url}')

   async def consumer(name, queue):
       """Process URLs from the queue until it shuts down."""
       try:
           while True:
               url = await queue.get()
               # Simulate a network request with random latency.
               delay = random.uniform(0.5, 1.5)
               await asyncio.sleep(delay)
               print(f'  {name} processed {url} ({delay:.1f}s)')
               queue.task_done()
       except asyncio.QueueShutDown:
           return

   async def main():
       queue = asyncio.Queue(maxsize=10)
       urls = [f'https://example.com/page/{i}' for i in range(6)]

       async with asyncio.TaskGroup() as tg:
           # Start three consumers.
           for i in range(3):
               tg.create_task(consumer(f'worker-{i}', queue))

           # Produce all URLs.
           for url in urls:
               await queue.put(url)

           # Wait until every item has been processed.
           await queue.join()

           # Signal consumers to stop.
           queue.shutdown()

   asyncio.run(main())

.. code-block:: none

   worker-0 processed https://example.com/page/0 (0.7s)
   worker-1 processed https://example.com/page/1 (1.2s)
   worker-2 processed https://example.com/page/2 (0.5s)
   worker-0 processed https://example.com/page/3 (0.9s)
   worker-2 processed https://example.com/page/4 (1.1s)
   worker-1 processed https://example.com/page/5 (0.8s)

Key points:

- :meth:`~asyncio.Queue.put` blocks if the queue is full, providing
  back-pressure.
- :meth:`~asyncio.Queue.task_done` tells the queue that an item has been
  fully processed.
- :meth:`~asyncio.Queue.join` blocks until every item put into the queue has
  had :meth:`~asyncio.Queue.task_done` called for it.
- :meth:`~asyncio.Queue.shutdown` causes pending and future
  :meth:`~asyncio.Queue.get` calls to raise :exc:`asyncio.QueueShutDown`,
  which is how the consumers exit their loops cleanly.


.. _asyncio-tutorial-to-thread:

Running blocking code with to_thread
=====================================

Sometimes you need to call code that blocks --- for example, a library that
performs synchronous file I/O or a CPU-intensive function.  Running such code
directly in a coroutine would freeze the event loop and prevent other tasks
from making progress.

:func:`asyncio.to_thread` runs a function in a separate thread, so the event
loop stays responsive::

   import asyncio
   import time

   def blocking_task(seconds):
       """A slow, blocking function (simulating file I/O)."""
       time.sleep(seconds)
       return f'Slept for {seconds}s'

   async def main():
       start = time.time()

       async with asyncio.TaskGroup() as tg:
           task1 = tg.create_task(asyncio.to_thread(blocking_task, 2))
           task2 = tg.create_task(asyncio.to_thread(blocking_task, 3))
           task3 = tg.create_task(asyncio.sleep(1))

       print(task1.result())
       print(task2.result())
       print(f'Total time: {time.time() - start:.1f}s')

   asyncio.run(main())

.. code-block:: none

   Slept for 2s
   Slept for 3s
   Total time: 3.0s

All three operations ran concurrently --- the total time is 3 seconds, not 6.

.. note::

   :func:`asyncio.to_thread` is for I/O-bound blocking code.  For
   CPU-bound work, consider :class:`~concurrent.futures.ProcessPoolExecutor`
   with :meth:`loop.run_in_executor() <asyncio.loop.run_in_executor>`.


.. _asyncio-tutorial-streams:

Network programming with streams
=================================

Asyncio provides a high-level API for TCP networking through
:ref:`streams <asyncio-streams>`.  Let's build an echo server that sends
back whatever a client sends.

Echo server
-----------

::

   import asyncio

   async def handle_client(reader, writer):
       addr = writer.get_extra_info('peername')
       print(f'New connection from {addr}')

       while True:
           data = await reader.readline()
           if not data:
               break
           writer.write(data)
           await writer.drain()

       print(f'Connection from {addr} closed')
       writer.close()
       await writer.wait_closed()

   async def main():
       server = await asyncio.start_server(
           handle_client, '127.0.0.1', 8888)
       addr = server.sockets[0].getsockname()
       print(f'Serving on {addr}')

       async with server:
           await server.serve_forever()

   asyncio.run(main())

:func:`asyncio.start_server` listens for incoming connections.  Each time a
client connects, it calls ``handle_client`` with a
:class:`~asyncio.StreamReader` and a :class:`~asyncio.StreamWriter`.  Multiple
clients are handled concurrently --- each connection runs as its own coroutine.

Two patterns are essential when working with streams:

- **Write then drain:** :meth:`~asyncio.StreamWriter.write` buffers data.
  ``await`` :meth:`~asyncio.StreamWriter.drain` ensures it is actually sent
  (and applies back-pressure if the client is slow to read).

- **Close then wait_closed:** :meth:`~asyncio.StreamWriter.close` initiates
  shutdown. ``await`` :meth:`~asyncio.StreamWriter.wait_closed` waits until
  the connection is fully closed.

Echo client
-----------

To test the server, run it in one terminal and this client in another::

   import asyncio

   async def main():
       reader, writer = await asyncio.open_connection(
           '127.0.0.1', 8888)

       for message in ['Hello!\n', 'How are you?\n', 'Goodbye!\n']:
           writer.write(message.encode())
           await writer.drain()

           data = await reader.readline()
           print(f'Received: {data.decode().strip()!r}')

       writer.close()
       await writer.wait_closed()

   asyncio.run(main())

.. code-block:: none

   Received: 'Hello!'
   Received: 'How are you?'
   Received: 'Goodbye!'


.. _asyncio-tutorial-chat-server:

Case study: a chat server
=========================

Let's combine everything into a real application: a TCP chat server where
multiple users can connect and exchange messages.  This example brings together
streams, shared state, and proper cleanup.

::

   import asyncio

   connected_clients: dict[str, asyncio.StreamWriter] = {}

   async def broadcast(message, *, sender=None):
       """Send a message to all connected clients except the sender."""
       for name, writer in list(connected_clients.items()):
           if name != sender:
               try:
                   writer.write(message.encode())
                   await writer.drain()
               except ConnectionError:
                   pass  # Client disconnected; cleaned up elsewhere.

   async def handle_client(reader, writer):
       addr = writer.get_extra_info('peername')

       writer.write(b'Enter your name: ')
       await writer.drain()
       data = await reader.readline()
       if not data:
           writer.close()
           await writer.wait_closed()
           return

       name = data.decode().strip()
       connected_clients[name] = writer
       print(f'{name} ({addr}) has joined')
       await broadcast(f'*** {name} has joined the chat ***\n', sender=name)

       try:
           while True:
               data = await reader.readline()
               if not data:
                   break
               message = data.decode().strip()
               if message:
                   print(f'{name}: {message}')
                   await broadcast(f'{name}: {message}\n', sender=name)
       except ConnectionError:
           pass
       finally:
           del connected_clients[name]
           print(f'{name} ({addr}) has left')
           await broadcast(f'*** {name} has left the chat ***\n')
           writer.close()
           await writer.wait_closed()

   async def main():
       server = await asyncio.start_server(
           handle_client, '127.0.0.1', 8888)
       addr = server.sockets[0].getsockname()
       print(f'Chat server running on {addr}')

       async with server:
           await server.serve_forever()

   asyncio.run(main())

Some things to note about this design:

- **No locks needed.**  ``connected_clients`` is a plain :class:`dict`.
  Because asyncio runs in a single thread, no other task can modify it between
  ``await`` points.  This is the thread-safety advantage from the
  :ref:`opening section <asyncio-tutorial-why>`.

- **Iterating a copy.**  ``broadcast()`` iterates over ``list(...)`` because a
  client might disconnect (and be removed from the dict) while we are
  broadcasting.

- **Cleanup in** ``finally``.  The ``try``/``finally`` block ensures the
  client is removed from ``connected_clients`` and the connection is closed
  even if the client disconnects unexpectedly.

To test, start the server in one terminal and connect from two or more others
using ``telnet`` or ``nc``:

.. code-block:: none

   $ nc 127.0.0.1 8888
   Enter your name: Alice
   *** Bob has joined the chat ***
   Bob: Hi Alice!
   Hello Bob!

Each message you type is broadcast to all other connected users.


.. _asyncio-tutorial-extending-chat:

Extending the chat server
=========================

The chat server is a good foundation to build on.  Here are some ideas to
try.

Adding an idle timeout
----------------------

Disconnect users who have been idle for too long using
:func:`asyncio.timeout`::

   async def handle_client(reader, writer):
       # ... (name registration as before) ...
       try:
           while True:
               try:
                   async with asyncio.timeout(300):  # 5-minute timeout
                       data = await reader.readline()
               except TimeoutError:
                   writer.write(b'Disconnected: idle timeout.\n')
                   await writer.drain()
                   break
               if not data:
                   break
               message = data.decode().strip()
               if message:
                   await broadcast(f'{name}: {message}\n', sender=name)
       except ConnectionError:
           pass
       finally:
           # ... (cleanup as before) ...

Exercises
---------

These exercises build on the concepts covered in this tutorial:

- **Add a** ``/quit`` **command** that lets a user disconnect gracefully by
  typing ``/quit``.

- **Add private messaging.**  If a user types ``/msg Alice hello``, only
  Alice should receive the message.

- **Log messages to a file** using :func:`asyncio.to_thread` to avoid
  blocking the event loop during file writes.

- **Limit concurrent connections** using :class:`asyncio.Semaphore` to
  restrict the server to a maximum number of users.


.. _asyncio-tutorial-pitfalls:

Common pitfalls
===============

Forgetting to await
-------------------

Calling a coroutine function without ``await`` creates a coroutine object but
does not run it::

   async def main():
       asyncio.sleep(1)  # Wrong: creates a coroutine but never runs it.
       await asyncio.sleep(1)  # Correct.

Python will emit a :exc:`RuntimeWarning` if a coroutine is never awaited.
If you see ``RuntimeWarning: coroutine '...' was never awaited``, check for a
missing ``await``.

Blocking the event loop
-----------------------

Calling blocking functions like :func:`time.sleep` or performing synchronous
I/O inside a coroutine freezes the entire event loop::

   async def bad():
       time.sleep(5)  # Wrong: blocks the event loop for 5 seconds.

   async def good():
       await asyncio.sleep(5)        # Correct: suspends without blocking.
       await asyncio.to_thread(time.sleep, 5)  # Also correct: runs in a thread.

You can use :ref:`debug mode <asyncio-debug-mode>` to detect blocking calls:
pass ``debug=True`` to :func:`asyncio.run`.

Fire-and-forget tasks disappearing
-----------------------------------

If you create a task without keeping a reference to it, the task may be
garbage collected before it finishes::

   async def main():
       asyncio.create_task(some_coroutine())  # No reference kept!
       await asyncio.sleep(10)

Use :class:`asyncio.TaskGroup` to manage task lifetimes, or store task
references in a collection.

Calling asyncio.run() inside an async function
-----------------------------------------------

:func:`asyncio.run` creates a new event loop.  You cannot call it from inside
a running event loop --- doing so raises :exc:`RuntimeError`::

   async def main():
       asyncio.run(other())  # RuntimeError: already running!

Instead, use ``await`` to call other coroutines, or :func:`asyncio.create_task`
to run them concurrently.


.. _asyncio-tutorial-next-steps:

Next steps
==========

You have learned the core concepts of asyncio: coroutines, tasks, timeouts,
queues, streams, and how to combine them into a real application.  Here are
some resources for further learning:

.. seealso::

   :ref:`a-conceptual-overview-of-asyncio`
      Understand how asyncio works under the hood --- event loops, futures,
      and the mechanics of ``await``.

   :ref:`asyncio-streams`
      The complete Streams API reference, including Unix sockets and TLS.

   :ref:`asyncio-sync`
      Synchronization primitives: :class:`~asyncio.Lock`,
      :class:`~asyncio.Event`, :class:`~asyncio.Semaphore`,
      :class:`~asyncio.Barrier`, and more.

   :ref:`asyncio-dev`
      Development tips, debug mode, and logging.
