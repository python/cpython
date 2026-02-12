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

Accepting connections
---------------------

The core of any asyncio network server is :func:`asyncio.start_server`.  You
give it a callback function, a host, and a port.  When a client connects,
asyncio calls your callback with a :class:`~asyncio.StreamReader` for receiving
data and a :class:`~asyncio.StreamWriter` for sending it back.

Here is a minimal callback that accepts a connection, prints the client's
address, and immediately closes it.  The :meth:`~asyncio.StreamWriter.close`
method initiates the connection shutdown, and awaiting
:meth:`~asyncio.StreamWriter.wait_closed` waits until it is fully closed::

   async def handle_client(reader, writer):
       addr = writer.get_extra_info('peername')
       print(f'New connection from {addr}')
       writer.close()
       await writer.wait_closed()

Running the server
------------------

To keep the server running and accepting connections, use
:meth:`~asyncio.Server.serve_forever` inside an ``async with`` block.
Using the server as an async context manager ensures it is properly cleaned up
when done, and :meth:`~asyncio.Server.serve_forever` keeps it running until the
program is interrupted.  Finally, :func:`asyncio.run` starts the event loop and
runs the top-level coroutine::

   async def main():
       server = await asyncio.start_server(
           handle_client, '127.0.0.1', 8888)
       async with server:
           await server.serve_forever()

   asyncio.run(main())

Reading and writing data
------------------------

To turn this into an echo server, the callback needs to read data from the
client and send it back.  :meth:`~asyncio.StreamReader.readline` reads one line
at a time, returning an empty :class:`bytes` object when the client
disconnects.

:meth:`~asyncio.StreamWriter.write` buffers outgoing data without sending it
immediately.  Awaiting :meth:`~asyncio.StreamWriter.drain` flushes the buffer
and applies back-pressure if the client is slow to read.

With these pieces, the echo callback becomes::

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

To test, run the server in one terminal and connect from another using ``nc``
or ``telnet``:

.. code-block:: none

   $ nc 127.0.0.1 8888


.. _asyncio-chat-server-building:

Building the chat server
========================

The echo server handles each client independently --- it reads from one client
and writes back to the same client.  A chat server, on the other hand, needs to
deliver each message to *every* connected client.  This means the server must
keep track of who is connected so it can send messages to all of them.

Tracking connected clients
--------------------------

We store each client's name and :class:`~asyncio.StreamWriter` in a
module-level dictionary.  When a client connects, ``handle_client`` prompts for
a name and adds the writer to the dictionary.  A ``finally`` block ensures
the client is always removed on disconnect, even if the connection drops
unexpectedly::

   connected_clients: dict[str, asyncio.StreamWriter] = {}

   async def handle_client(reader, writer):
       writer.write(b'Enter your name: ')
       await writer.drain()
       name = (await reader.readline()).decode().strip()
       connected_clients[name] = writer
       try:
           ...   # message loop (shown below)
       finally:
           del connected_clients[name]
           writer.close()
           await writer.wait_closed()

Broadcasting messages
---------------------

To send a message to all clients, we define a ``broadcast`` function.
:class:`asyncio.TaskGroup` sends to all recipients concurrently rather than
one at a time::

   async def broadcast(message, *, sender=None):
       """Send a message to all connected clients except the sender."""
       async def send(writer):
           # Ignore clients that have already disconnected.
           with contextlib.suppress(ConnectionError):
               writer.write(message.encode())
               await writer.drain()

       async with asyncio.TaskGroup() as tg:
           for name, writer in list(connected_clients.items()):
               if name != sender:
                   tg.create_task(send(writer))


.. _asyncio-chat-server-timeout:

Adding an idle timeout
----------------------

To disconnect clients who have been idle for too long, wrap the read call in
:func:`asyncio.timeout`.  This async context manager takes a delay in seconds.
If the enclosed ``await`` does not complete within that time, the operation is
cancelled and :exc:`TimeoutError` is raised, freeing server resources when
clients connect but stop sending data::

   async with asyncio.timeout(300):  # 5-minute timeout
       data = await reader.readline()

The complete chat server
------------------------

Putting it all together, here is the complete chat server with client tracking,
broadcasting, and an idle timeout::

   import asyncio
   import contextlib

   connected_clients: dict[str, asyncio.StreamWriter] = {}

   async def broadcast(message, *, sender=None):
       """Send a message to all connected clients except the sender."""
       async def send(writer):
           # Ignore clients that have already disconnected.
           with contextlib.suppress(ConnectionError):
               writer.write(message.encode())
               await writer.drain()

       async with asyncio.TaskGroup() as tg:
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

       try:
           await broadcast(f'*** {name} has joined the chat ***\n', sender=name)
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

.. note::

   This server does not handle two clients choosing the same name.  Adding
   support for unique names is left as an exercise for the reader.

To test, start the server and connect from two or more terminals using ``nc``
or ``telnet``:

.. code-block:: none

   $ nc 127.0.0.1 8888
   Enter your name: Alice
   *** Bob has joined the chat ***
   Bob: Hi Alice!
   Hello Bob!

Each message you type is broadcasted to all other connected users.
