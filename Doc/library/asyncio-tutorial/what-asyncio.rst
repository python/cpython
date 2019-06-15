What Does "Async" Mean?
=======================

Let's make a function that communicates over the network:

.. code-block:: python3

    import socket

    def greet(host, port):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((host, host))
            s.sendall(b'Hello, world')
            reply = s.recv(1024)

        print('Reply:', repr(reply))

This function will:

#. make a socket connection to a host,
#. send ``b'Hello, world'``, and
#. **wait** for a reply.

The key point here is about the word *wait*: in the code, execution proceeds line-by-line
until the line ``reply = s.recv(1024)``. While these lines
of code are executing, no other code (in the same thread) can run. We call this behaviour
*synchronous*. At the point where we wait, execution pauses
until we get a reply from the host.

Now the question comes up: what if you need to send a greeting to
*multiple* hosts? You could just call it twice, right?

.. code-block:: python3

    import socket

    def greet(host, port):
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((host, host))
            s.sendall(b'Hello, world')
            reply = s.recv(1024)

        print('Reply:', repr(reply))

    greet(host1, port1)
    greet(host2, port2)

This works fine, but see that the first call to ``greet`` will completely
finish before the second call (to ``host2``) can begin.

Computers process things sequentially, which is why programming languages
like Python also work sequentially; but what we see here is that our
code is also going to **wait** sequentially. This is, quite literally,
a waste of time. What we would really like to do here is wait for
all the replies *concurrently*, i.e., at the same time.

Preemptive Concurrency
----------------------

Operating systems like Windows, Mac and Linux, and others, understand
this problem deeply. If you're reading this on a computer or even on your
mobile, there will be tens or hundreds of processes running at the same
time on your device. At least, they will appear to be running
concurrently.  What is really happening is that the operating system
is sharing little slices of processor (CPU) time among all the
processes.  If we started two completely separate instances of our ``greet`` program at the same time, they *would* run (and therefore wait)
concurrently which is exactly what we want.

However, there is a price for that: each new process consumes extra resources
from the operating system; it's not just that we wait in parallel, but
*everything* is now in parallel, a full copy of our program.
But more than that, there is another tricky
problem about *how* the operating system knows when to allocate
execution time between each copy of our process. The answer: it doesn't!
This means that the operating system can decide when to give processor
time to each process. Your code, and therefore you, will not know when
these switches occur. This is called "preemption". From
`Wikipedia <https://en.wikipedia.org/wiki/Preemption_(computing)>`_:
*In computing, preemption is the act of temporarily interrupting a
task being carried out by a computer system, without requiring
its cooperation, and with the intention of resuming the task
at a later time*.

Operating Systems do this kind of preemptive switching for both
processes and threads. A simplistic but useful description of the
difference is that one process can have multiple threads, and those
threads share all the memory in their parent process.

Because of this preemptive switching, you will never be sure of
when each of your processes and threads is *actually* executing on
a CPU *relative to each other*. For processes, this is quite safe because
their memory spaces are isolated from each other; however,
**threads** are not isolated from each other (within the same process).
In fact, the primary feature of threads over processes is that
multiple threads within a single process can access the same memory.
And this is where all the problems begin.

Jumping back to our code sample further up: we may also choose to run the
``greet()`` function in multiple threads; and then
they will also wait for replies concurrently. However, now you have
two threads that are allowed to access the same objects in memory,
with little control over
how execution will switch between the two threads (unless you
use the synchronization primitives in the ``threading`` module) . This
situation can result in *race conditions* in how objects are modified,
and these bugs can be very difficult to fix.

Cooperative Concurrency
-----------------------

This is where "async" programming comes in. It provides a way to manage
multiple socket connections all in a single thread; and the best part
is that you get to control *when* execution is allowed to switch between
these different contexts.

We will explain more of the details throughout this tutorial,
but briefly, our earlier example becomes something like the following
pseudocode:

.. code-block:: python3

    import asyncio

    async def greet(host, port):
        reader, writer = await asyncio.open_connection(host, port)
        writer.write(b'Hello, world')
        reply = await reader.recv(1024)
        writer.close()

        print('Reply:', repr(reply))

    async def main():
        # Both calls run at the same time
        await asyncio.gather(
            greet(host1, port1),
            greet(host2, port2)
        )

    asyncio.run(main())

In this code, the two instances of the ``greet()`` function will
run concurrently.

There are a couple of new things here, but I want you to focus
on the new keyword ``await``. Unlike threads, execution is allowed to
switch between the two ``greet()`` invocations **only** where the
``await`` keyword appears. On all other lines, execution is exactly the
same as normal Python, and will not be preempt by thread switching (there's
typically only a single thread in most ``asyncio`` programs).
These ``async def`` functions are called
"asynchronous" because execution does not pass through the function
top-down, but instead can suspend in the middle of a function at the
``await`` keyword, and allow another function to execute while
*this function* is waiting for network data.

An additional advantage of the *async* style above is that it lets us
manage several thousand concurrent long-lived socket connections in a simple way.
One can also use threads to manage concurrent long-lived socket connections,
but it gets difficult to go past a few thousand because the creation
of operating system threads, just like processes, consumes additional
resources from the operating system.
