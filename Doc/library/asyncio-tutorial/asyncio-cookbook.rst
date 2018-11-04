Asyncio Cookbook
================

Let's look at a few common situations that will come up in your
``asyncio`` programs, and how best to tackle them.

[There's a lot more we can do if we're able to refer to
3rd party packages here. We could show a websockets example,
and other things.]

Using A Queue To Move Data Between Long-Lived Tasks
---------------------------------------------------

TODO

Using A Queue To Control A Pool of Resources
--------------------------------------------

- show example with a pool of workers
- show example with a connection pool

Best Practices For Timeouts
---------------------------

- start with ``asyncio.wait_for()``
- also look at ``asyncio.wait()``, and what to do if not all tasks
  are finished when the timeout happens. Also look at the different
  termination conditions of ``asyncio.wait()``

How To Handle Cancellation
--------------------------

- app shutdown
- how to handle CancelledError and then close sockets
- also, when waiting in a loop on ``await queue.get()`` is it better to
  handle CancelledError, or use the idiom of putting ``None`` on the
  queue? (``None`` would be better because it ensures the contents of the
  queue get processed first, but I don't think we can prevent
  CancelledError from getting raised so it must be handled anyway. I
  can make an example to explain better.)

Keeping Track Of Many Connections
---------------------------------

- example using a global dict
- show how a weakref container can simplify cleanup
- show how to access connection info e.g. ``get_extra_info()``
- this kind of thing:

.. code-block:: python3

    import asyncio
    from weakref import WeakValueDictionary

    CONNECTIONS = WeakValueDictionary()

    async def client_connected_cb(reader, writer):

        addr = writer.get_extra_info('peername')
        print(f'New connection from {addr}')

        # Every new connection gets added to the global dict.
        # Actually, *writer* objects get added. This makes
        # it easy to look up a connection and immediately
        # send data to it from other async functions.
        CONNECTIONS[addr] = writer
        ...

    async def main():
        server = await asyncio.start_server(
            client_connected_cb=client_connected_db,
            host='localhost',
            port='9011',
        )
        async with server:
            await server.serve_forever()

    if __name__ == '__main__':
        asyncio.run(main())

Handling Reconnection
---------------------

- Example is a client app that needs to reconnect to a server
  if the server goes down, restarts, or there is a network partition
  or other general kind of error

Async File I/O
--------------

- mention that disk I/O is still IO
- Python file operations like ``open()``, etc. are blocking
- I think all we can do here is refer to the 3rd party *aiofiles*
  package?
- I suppose we could show how to do file IO in thread, driven
  by ``run_in_executor()``...

Wait For Async Results In Parallel
----------------------------------

TODO

- show an example with gather
- show another example with wait
- maybe throw in an example with gather that also uses
  "wait_for" for timeout
- either include "return_exceptions" here or in a different question

.. code-block:: python3

    import asyncio

    async def slow_sum(x, y):
        result = x + y
        await asyncio.sleep(result)
        return result

    async def main():
        results = await asyncio.gather(
            slow_sum(1, 1),
            slow_sum(2, 2),
        )
        print(results)  # "[2, 4]"

    if __name__ == '__main__':
        asyncio.run(main())

- we should also include a brief discussion of "when to use asyncio.gather and
  when to use asyncio.wait"

Secure Client-Server Networking
-------------------------------

- built-in support for secure sockets
- you have to make your own secret key, and server certificate

.. code-block:: bash
    :caption: Create a new private key and certificate

    $ openssl req -newkey rsa:2048 -nodes -keyout chat.key \
        -x509 -days 365 -out chat.crt

This creates ``chat.key`` and ``chat.crt`` in the current dir.

.. code-block:: python3
    :caption: Secure server

    import asyncio
    import ssl

    async def main():
        ctx = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
        ctx.check_hostname = False

        # These must have been created earlier with openssl
        ctx.load_cert_chain('chat.crt', 'chat.key')

        server = await asyncio.start_server(
            client_connected_cb=client_connected_cb,
            host='localhost',
            port=9011,
            ssl=ctx,
        )
        async with server:
            await server.serve_forever()

    async def client_connected_cb(reader, writer):
        print('Client connected')
        received = await reader.read(1024)
        while received:
            print(f'received: {received}')
            received = await reader.read(1024)

    if __name__ == '__main__':
        asyncio.run(main())


.. code-block:: python3
    :caption: Secure client

    import asyncio
    import ssl

    async def main():
        print('Connecting...')
        ctx = ssl.create_default_context(ssl.Purpose.SERVER_AUTH)
        ctx.check_hostname = False

        # The client must only have access to the cert *not* the key
        ctx.load_verify_locations('chat.crt')
        reader, writer = await asyncio.open_connection(
            host='localhost',
            port=9011,
            ssl=ctx
        )

        writer.write(b'blah blah blah')
        await writer.drain()
        writer.close()
        await writer.wait_closed()

    if __name__ == '__main__':
        asyncio.run(main())

Correctly Closing Connections
-----------------------------

- from the client side
- from the server side
- Yury I need your help here. What is the actual "correct" way
  to do this? Streams API preferable, if possible.

Handling Typical Socket Errors
------------------------------

- Maybe describe the situations in which they can occur? Not sure.

- ``ConnectionError``
- ``ConnectionResetError``
- ``ConnectionAbortedError``
- ``ConnectionRefusedError``

Might also want to show some examples of ``asyncio.IncompleteReadError``.

Also link/refer to the socket programming HOWTO in the docs.

Graceful Shutdown on Windows
----------------------------

TODO


Run A Blocking Call In An Executor
----------------------------------

- show example with default executor
- show example with a custom executor (thread-based)
- show example with a custom executor (process-based)


Adding Asyncio To An Existing Sync (Threaded) Application
---------------------------------------------------------

- Imagine an existing app that uses threading for concurrency,
  but we want to make use of asyncio only for, say, a large
  number of concurrent GET requests, but leave the rest of the
  app unchanged.
- Plan would be to run the asyncio loop in another thread
- Can show how to safely communicate between that thread and
  the main thread (or others).



Notes:

- My thinking here was a Q&A style, and then each section has
  a code snippet demonstrating the answer.

