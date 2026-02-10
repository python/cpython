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
      An introduction to the fundamentals of asyncio.

   :mod:`asyncio` reference documentation
      The complete API reference.


.. _asyncio-chat-server-echo:

Starting with an echo server
============================

Before building the chat server, let's start with something simpler: an echo
server that sends back whatever a client sends.

The core of any asyncio network server is :func:`asyncio.start_server`.  You
give it a callback function, a host, and a port.  When a client connects,
asyncio calls your callback with two arguments: a
:class:`~asyncio.StreamReader` for receiving data and a
:class:`~asyncio.StreamWriter` for sending data back.  Each connection runs
as its own coroutine, so multiple clients are handled concurrently.

Here is a complete echo server::

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

The :meth:`~asyncio.StreamWriter.write` method buffers data without sending
it immediately.  Awaiting :meth:`~asyncio.StreamWriter.drain` flushes the
buffer and applies back-pressure if the client is slow to read.  Similarly,
:meth:`~asyncio.StreamWriter.close` initiates shutdown, and awaiting
:meth:`~asyncio.StreamWriter.wait_closed` waits until the connection is
fully closed.

To test, run the server in one terminal and connect from another using ``nc``
(or ``telnet``):

.. code-block:: none

   $ nc 127.0.0.1 8888


.. _asyncio-chat-server-building:

Building the chat server
========================

The chat server extends the echo server with two additions: tracking connected
clients and broadcasting messages to everyone.

We store each client's name and :class:`~asyncio.StreamWriter` in a dictionary.
When a message arrives, we broadcast it to all other connected clients.
:class:`asyncio.TaskGroup` sends to all recipients concurrently, and
:func:`contextlib.suppress` silently handles any :exc:`ConnectionError` from
clients that have already disconnected.

::

   import asyncio
   import contextlib

   connected_clients: dict[str, asyncio.StreamWriter] = {}

   async def broadcast(message, *, sender=None):
       """Send a message to all connected clients except the sender."""
       async def send(writer):
           with contextlib.suppress(ConnectionError):
               writer.write(message.encode())
               await writer.drain()

       async with asyncio.TaskGroup() as tg:
           # Iterate over a copy: clients may leave during the broadcast.
           for name, writer in list(connected_clients.items()):
               if name != sender:
                   tg.create_task(send(writer))

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
           # Ensure cleanup even if the client disconnects unexpectedly.
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

To test, start the server and connect from two or more terminals using ``nc``
(or ``telnet``):

.. code-block:: none

   $ nc 127.0.0.1 8888
   Enter your name: Alice
   *** Bob has joined the chat ***
   Bob: Hi Alice!
   Hello Bob!

Each message you type is broadcast to all other connected users.


.. _asyncio-chat-server-timeout:

Adding an idle timeout
======================

To disconnect clients who have been idle for too long, wrap the read call in
:func:`asyncio.timeout`.  This async context manager takes a duration in
seconds.  If the enclosed ``await`` does not complete within that time, the
operation is cancelled and :exc:`TimeoutError` is raised.  This frees server
resources when clients connect but stop sending data.

Replace the message loop in ``handle_client`` with::

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
