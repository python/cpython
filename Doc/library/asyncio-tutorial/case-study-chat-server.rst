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
    :language: python

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
    :language: python
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



TODO

Notes:

- using the streams API
- first show the client.
- will have to explain a message protocol. (But that's easy)
- then show the server
- spend some time on clean shutdown.
