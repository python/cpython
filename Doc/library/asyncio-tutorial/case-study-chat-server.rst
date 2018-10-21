Asyncio Case Study: Chat Application (Server)
=============================================

We're going to build a chat application. Users will be able to
connect to a central server and send messages to "chat rooms", which
all the other users in those rooms will be able to see. This case study
gives us an opportunity to show how to use the various features of
``asyncio`` in a "real world" application.

This will be a client-server application.  The server application
will run on a central host, and the chat application will run on each
user's computer or device. The server is the easier of the two, for
interesting reasons that we'll get into later.

We're going to start with a basic application layout and build up from
there.

Starting Code Layout
--------------------

The code below shows the basic starting template for an asyncio
service. In these kinds of applications, the service will typically
run for a long time, serving clients that may connect,
disconnect, and then later reconnect multiple times.

.. literalinclude:: server01.py
    :language: python3

As explained earlier, ``main`` itself is a *coroutine function*, and
when evaluated, i.e., ``main()``, it returns a *coroutine* object
which the ``asyncio.run()`` function knows how to execute.

.. note::
    The ``asyncio.run()`` function waits for ``main()`` to complete.
    When ``main()`` returns, the ``run()`` function will then cancel
    all tasks that are still around. This means, precisely, that the
    ``asyncio.CancelledError`` exception will get raised in all such
    pending tasks. This gives you a way to deal with an "application
    shutdown" scenario: you just need to handle the ``CancelledError``
    exception in places where you need a controlled way of terminating
    tasks.

There isn't much more to say about the basic template, so let's add
the actual server.

Server
------

We can use the *Streams API* (ref:TODO) to create a TCP server very
easily:

.. literalinclude:: server02.py
    :language: python3
    :linenos:

We've added the ``start_server()`` call on line 5, and this call takes
not only the ``host`` and ``port`` parameters you'd expect, but also a
*callback* function that will be called for each new connection. This
is coroutine function ``client_connected()``, on line 13.

The callback is provided with ``reader`` and ``writer`` parameters.
These give access to two streams for this new connection. It is here
that data will be received from, and sent to clients.

Printing out "New client connected!" is obviously going to be quite
useless. We're going to want to receive chat messages from a client,
and we also want to send these messages to all the other clients in the
same "chat room". We don't yet have the concept of "rooms" defined
anywhere yet, but that's ok. Let's first focus on what must be sent
and received between server and client.

Let's sketch out a basic design of the communication pattern:

#. Client connects to server
#. Client sends a message to server to announce themselves, and join
   a room
#. Client sends a message to a room
#. Server relays that message to all other clients in the same room
#. Eventually, client disconnects.

These actions suggest a few different kinds of information that need to
be sent between server and client. We need to create a *protocol*
that both server and client can use to communicate.

How about we use JSON messages? Here is an example of the payload a
client needs to provide immediately after connection:

.. code-block:: json
    :caption: Client payload after connection

    {
        "action": "connect",
        "username": "<something>"
    }

Here are example messages for joining and leaving rooms:

.. code-block:: json
    :caption: Client payload to join a room

    {
        "action": "joinroom",
        "room": "<room_name>"
    }

.. code-block:: json
    :caption: Client payload to leave a room

    {
        "action": "leaveroom",
        "room": "<room_name>"
    }

And here's an example of a client payload for sending a chat
message to a room:

.. code-block:: json
    :caption: Client payload to send a message to a room

    {
        "action": "chat",
        "room": "<room_name>",
        "message": "I'm reading the asyncio tutorial!"
    }

All of the JSON examples above are for payloads that will be received
from clients, but remember that the server must also send messages
to all clients in a room. That message might look something like this:

.. code-block:: json
    :caption: Server payload to update all clients in a room
    :linenos:

    {
        "action": "chat",
        "room": "<room_name>",
        "message": "I'm reading the asyncio tutorial!",
        "from": "<username>"
    }

The message is similar to the one received by a client, but on line 5
we now need to indicate from whom the message was sent.

Message-based Protocol
----------------------

Now we have a rough idea about what messages between client and
server will look like. Unfortunately, the Streams API, like the
underlying TCP protocol it wraps, does not give us message-handling
built-in. All we get is a stream of bytes. It is up to us to
know how to break up the stream of bytes into recognizable messages.

The most common raw message structure is based on the idea of a
*size prefix*:

.. code-block:: text
    :caption: Simple message structure, in bytes

    [3 bytes (header)][n bytes (payload)]

The 3-byte header is an encoded 24-bit integer, which must be the size
of the following payload (in bytes). Using 3 bytes is arbitrary: I
chose it here because it allows a message size of up to 16 MB, which
is way, way larger than what we're going to need for this tutorial.

Receiving A Message
^^^^^^^^^^^^^^^^^^^

Imagine we receive a message over a ``StreamReader`` instance.
Remember, this is what we get from the ``client_connected()``
callback function.  This is how we pull the bytes off the stream
to reconstruct the message:

.. code-block:: python3
    :linenos:

    from asyncio import StreamReader
    from typing import AsyncGenerator
    import json

    async def new_messages(reader: StreamReader) -> AsyncGenerator[Dict, None]:
        while True:
            size_prefix = await reader.readexactly(3)
            size = int.from_bytes(size_prefix, byteorder='little')
            message = await reader.readexactly(size)
            yield json.loads(message)

This code shows the following:

- line 5: This is an async function, but it's also a *generator*. All
  that means is that there is a ``yield`` inside, that will produce
  each new message as it comes in.
- line 6: A typical "infinite loop"; this async function will keep
  running, and keep providing newly received messages back to the
  caller code.
- line 7: We read exactly 3 bytes off the stream.
- line 8: Convert those 3 bytes into an integer object. Note that the
  byteorder here, "little", must also be used in client code that
  will be connecting to this server.
- line 9: Using the size calculated above, read exactly that number of
  bytes off the stream
- line 10: decode the bytes from JSON and yield that dict out.

The code sample above is pretty simple, but a little naive. There
are several ways in which it can fail and end the ``while True``
loop. Let's add a bit more error handling:

.. code-block:: python3
    :caption: Receiving JSON Messages over a Stream
    :linenos:

    from asyncio import StreamReader, IncompleteReadError, CancelledError
    from typing import AsyncGenerator
    import json

    async def new_messages(reader: StreamReader) -> AsyncGenerator[Dict, None]:
        try:
            while True:
                size_prefix = await reader.readexactly(3)
                size = int.from_bytes(size_prefix, byteorder='little')
                message = await reader.readexactly(size)
                try:
                    yield json.loads(message)
                except json.decoder.JSONDecodeError:
                    continue
        except (IncompleteReadError, ConnectionAbortedError,
                ConnectionResetError, CancelledError):
            # The connection is dead, leave.
            return

Now, we fail completely on a connection error, but if a particular
payload fails to deserialize via JSON, then we handle that too, but
allow the loop to continue listening for a new message. I included
handling for ``CancelledError``, which is how our application will
signal to this async function that the app is shutting down.

There are many decisions that one can make here about how to deal
with errors: for example, you might perhaps
choose to terminate a connection if a particular payload fails to
deserialise properly. It seems unlikely that a client would send
through only a few invalid JSON messages, but the rest valid. For
simplicity, we'll keep what we have for now, and move onto

Sending A Message
^^^^^^^^^^^^^^^^^

This time, we use a ``StreamWriter`` instance. The code below is
very similar to what we saw in the receiver:

.. code-block:: python3
    :caption: Sending JSON Messages over a Stream
    :linenos:

    from asyncio import StreamWriter
    from typing import Dict
    import json

    async def send_message(writer: StreamWriter, message: Dict):
        payload = json.dumps(message).encode()
        size_prefix = len(payload).to_bytes(3, byteorder='little')
        writer.write(size_prefix)
        writer.write(payload)
        await writer.drain()

Let's step through the lines:

- line 5: This async function must be called for each message that
  must be sent.
- line 6: Serialize the message to bytes.
- line 7: Build the size header; remember, this needs to be sent
  before the payload itself.
- line 8: Write the header to the stream
- line 9: Write the payload to the stream. Note that because there
  is no ``await`` keyword between sending the header and the payload,
  we can be sure that there will be no "context switch" between
  different async function calls trying to write data to this stream.
- line 10: Finally, wait for all the bytes to be sent.

We can place the two async functions above, ``new_messages()``
and ``send_message()``, into their own module called ``utils.py``
(Since we'll be using these function in both our server code and
our client code!).

For completeness, here is the utils module:

.. literalinclude:: utils01.py
    :caption: utils.py
    :language: python3

Let's return to the main server application and see how to
incorporate our new utility functions into the code.

Server: Message Handling
------------------------

Below, we import the new ``utils.py`` module and incorporate
some handling for receiving new messages:

.. literalinclude:: server03.py
    :caption: Server code with basic message handling
    :linenos:
    :language: python3

We've added a few new things inside the ``client_connected``
callback function:

- line 19: This is a handler function that will get called if a "connect"
  message is received. We have similar handler functions for the other
  action types.
- line 31: A simple dictionary that maps an action "name" to the handler
  function for that action.
- line 38: Here you can see how our async generator ``new_messages()``
  gets used. We simply loop over it, as you would with any other generator,
  and it will return a message only when one is received. Note the one
  minor difference as compared to a regular generator: you have to iterate
  over an async generator with ``async for``.
- line 39: Upon receiving a message, check which action must be taken.
- line 43: Look up the *handler function* that corresponds to the
  action. We set up these handlers earlier at line 31.
- line 47: call the handler function.

Our server code still doesn't do much; but at least it'll be testable
with a client sending a few different kinds of actions, and we'll be
able to see print output for each different kind of action received.

The next thing we'll have to do is set up chat rooms. There's no point
receiving messages if there's nowhere to put them!

TODO

Notes:

- then show the server
- spend some time on clean shutdown.
