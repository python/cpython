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

Handling Typical Socket Errors
------------------------------

- Maybe describe the situations in which they can occur? Not sure.

- ``ConnectionError``
- ``ConnectionResetError``
- ``ConnectionAbortedError``
- ``ConnectionRefusedError``

Might also want to show some examples of ``asyncio.IncompleteReadError``.

Graceful Shutdown on Windows
----------------------------

TODO


Run A Blocking Call In An Executor
----------------------------------

- show example with default executor
- show example with a custom executor (thread-based)
- show example with a custom executor (process-based)




Notes:

- My thinking here was a Q&A style, and then each section has
  a code snippet demonstrating the answer.

