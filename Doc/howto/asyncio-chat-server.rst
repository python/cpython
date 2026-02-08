.. _asyncio-chat-server-howto:

***********************************************
Building a TCP chat server with :mod:`!asyncio`
***********************************************

This guide walks you through building a TCP chat server where multiple users
can connect and exchange messages in real time.  Along the way, you will learn
how to use :ref:`asyncio streams <asyncio-streams>` for network programming.

The guide assumes basic Python knowledge --- functions, classes, and context
managers --- and a general understanding of async/await.

.. seealso::

   :ref:`a-conceptual-overview-of-asyncio`
      An explanation of how asyncio works under the hood.

   :mod:`asyncio` reference documentation
      The complete API reference.


.. _asyncio-chat-server-echo:

Starting with an echo server
============================

Before building the chat server, let's start with something simpler: an echo
server that sends back whatever a client sends.  This introduces the
:ref:`streams <asyncio-streams>` API that the chat server builds on.

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


Testing with a client
---------------------

To test the echo server, run it in one terminal and this client in another::

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

You can also test using ``telnet`` or ``nc``:

.. code-block:: none

   $ nc 127.0.0.1 8888


.. _asyncio-chat-server-building:

Building the chat server
========================

The chat server extends the echo server with two key additions: tracking
connected clients and broadcasting messages to all of them.

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
  ``await`` points.

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


.. _asyncio-chat-server-extending:

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

These exercises build on the chat server:

- **Add a** ``/quit`` **command** that lets a user disconnect gracefully by
  typing ``/quit``.

- **Add private messaging.**  If a user types ``/msg Alice hello``, only
  Alice should receive the message.

- **Log messages to a file** using :func:`asyncio.to_thread` to avoid
  blocking the event loop during file writes.

- **Limit concurrent connections** using :class:`asyncio.Semaphore` to
  restrict the server to a maximum number of users.


.. _asyncio-chat-server-pitfalls:

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
