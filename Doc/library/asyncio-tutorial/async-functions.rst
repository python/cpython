Async Functions, And Other Syntax Features
==========================================

Regular Python functions are created with the keyword ``def``,
and look like this:

.. code-block:: python3

    def f(x, y):
        print(x + y)

    # Evaluate the function
    f(1, 2)

Async functions are different in two respects:

.. code-block:: python3

    import asyncio

    async def f(x, y):
        print(x + y)

    # Execute the *async* function above
    asyncio.run(f(1, 2))

The first difference is that the function declaration is spelled
``async def``. The second difference is that async functions cannot be
executed by simply evaluating them. Instead, we use the ``run()`` function
from the ``asyncio`` module.

Executing Async Functions
-------------------------

The ``run`` function is only good for executing an async function
from "synchronous" code; and this is usually only used to execute
a "main" async function, from which others can be called in a simpler
way. You will not see ``asyncio.run()`` being called from inside an
``async def`` function.

That means the following:

.. code-block:: python3

    import asyncio

    async def f(x, y):
        print(x + y)

    async def main():
        await f(1, 2)

    asyncio.run(main())


The execution of the the async function ``main()`` is performed
with ``run()``, but once you're inside an ``async def`` function, then
calling another async function is done with the ``await`` keyword.

Ordinary (Sync) Functions Cannot Await Async Functions
------------------------------------------------------

If you're starting inside a normal function, you cannot call an
async function using the ``await`` keyword. Doing so will produce
a syntax error:

.. code-block:: python3

    >>> async def f(x, y):
    ...     pass
    ...
    >>> def g():
    ...     await f(1, 2)
    ...
      File "<input>", line 2
    SyntaxError: 'await' outside async function

To fix this, either ``asyncio.run()`` must be used, or the function
``g`` must be changed to be an ``async def`` function.  The ``run()``
function is tricky: you can't call it in a nested way, because
internally it creates an event loop and you cannot have two event
loops running at the same time (in the same thread). So the following
is also illegal:

.. code-block:: python3

    >>> import asyncio
    >>> async def f(x, y):
    ...     pass
    ...
    >>> def g():
    ...     asyncio.run(f(1, 2))
    ...
    >>> async def main():
    ...     g()
    ...
    >>> asyncio.run(main())
    Traceback (most recent call last):

      <snip>

      File "G:\Programs\Python37\lib\asyncio\runners.py", line 34, in run
        "asyncio.run() cannot be called from a running event loop")
    RuntimeError: asyncio.run() cannot be called from a running event loop

So ``asyncio.run()`` is really intended only for launching your *first*
async function; after that, every other async function should be
executed using the ``await`` keyword, and the task-based strategies which
we've not yet discussed.

Async Functions Can Call Sync Functions
---------------------------------------

The inverse works perfectly fine: calling ordinary Python functions
from inside ``async def`` functions. Here's an example:

.. code-block:: python3

    >>> import asyncio
    >>> import time
    >>> async def f():
    ...     print(time.ctime())
    ...
    >>> asyncio.run(f())
    Sun Nov  4 15:04:45 2018

One of the benefits of ``asyncio`` is that you can see at a glance
which code inside a function is subject to a context switch. In the
following code example, we have two kinds of ``sleep()``: a blocking
version from the ``time`` module, and an async version from ``asyncio``:

.. code-block:: python3

    >>> import time, asyncio
    >>> def func1():
    ...    time.sleep(0)
    ...
    >>> async def func2():
    ...    await asyncio.sleep(0)
    ...
    >>> async def main():
    ...     await func2()  # (1)
    ...     func1()
    ...     func1()
    ...     func1()
    ...     func1()
    ...     func1()
    ...     func1()
    ...     await func2()  # (2)
    ...
    >>> asyncio.run(main())

At (1), the underlying event loop is given the opportunity to switch from
``main()`` to any other tasks that are waiting to run, and after line (1)
returns, a series of calls to the sync function ``func1()`` occurs before
the next allowable context switch on the event loop at (2). While the
series of sync calls are running, *no other code* will execute in the
current thread, until you get to the next ``await``. This guarantee applies
a dramatic simplifying effect on your code, because now you can modify
data shared between multiple async tasks without fear of introducing
a race condition.

.. note:: In programs using ``asyncio``, you should never use ``time.sleep()``.
    The correct way to "sleep" is with ``await asyncio.sleep()``. This is
    because ``time.sleep()`` is a *blocking* call that will prevent the
    ``asyncio`` event loop from processing events. The only safe way to
    use ``time.sleep()`` is within a thread, or a subprocess, or with a
    value of zero!

Accurate Terminology For Async Functions
----------------------------------------

So far in this tutorial we've been intentionally sloppy with how
we refer to things like *async functions* or *async def* functions,
and *normal Python functions* and so on. It's time to get more
specific about what to call each of these things. It's important
because we need to be able to understand the difference between
a **coroutine** and a **coroutine function**, and a few other things
still to be introduced.

So let's do that now, using the ``inspect`` module. First let's look
at the two kinds of functions:

.. code-block:: python3

    >>> import inspect
    >>> def f1():
    ...     pass
    ...
    >>> inspect.isfunction(f1)
    True
    >>> inspect.iscoroutinefunction(f1)
    False

This is an ordinary Python function, and the ``inspect`` module
confirms that, but we've included another test to see if the function
is a *coroutine function*, which is ``False`` as expected. Let's do
the same on an ``async def`` function:

.. code-block:: python3

    >>> async def f2():
    ...     pass
    ...
    >>> inspect.isfunction(f2)
    True
    >>> inspect.iscoroutinefunction(f2)
    True

According to Python, ``f2`` is also considered to be a function, but
more specifically, it is a *coroutine function*, and this is the
specific name we will be using for *async def* functions.

Why does it matter? Well, when you evaluate a coroutine function, it'll
return something:

.. code-block:: python3

    >>> async def f2():
    ...     pass
    ...
    >>> result = f2()
    >>> type(result)
    <class 'coroutine'>
    >>> inspect.iscoroutine(result)
    True

The point we're trying to make here is that an *async def* function
is not yet a coroutine, but rather only a *coroutine function*; only
when you *evaluate* the coroutine function, will a coroutine
object be returned. The ``await`` keyword, which we showed in
previous examples, is acting on *coroutine* objects, not
the coroutine functions that create them.

This can be made clear in the following example:

.. code-block:: python3

    >>> async def f3():
    ...     return 123
    ...
    >>> async def main():
    ...     obj = f3()
    ...     result = await obj
    ...     print(result)
    ...
    >>> asyncio.run(main())
    123

In the code above, the value of ``obj`` is *not* ``123`` when
coroutine function ``f3`` is evaluated. Instead, ``obj`` is a
*coroutine* object, and it will only get executed when the
``await`` keyword is used.  Of course, you don't have to write
code like this where you first get the coroutine and then
use ``await`` on the object; simply evaluate the
coroutine function and use ``await`` all in the same line.

An Aside: Similarity To Generator Functions
-------------------------------------------

This has nothing to do with asyncio, but you might be interested
to see how this difference between a function and a
coroutine function is quite similar to the difference between
functions and generator functions:

.. code-block:: python3

    >>> def g():
    ...     yield 123
    ...
    >>> inspect.isfunction(g)
    True
    >>> inspect.isgeneratorfunction(g)
    True

If a function uses the ``yield`` keyword anywhere inside the function
body, that function becomes a *generator function*, very similar to
how a function declared with ``async def`` becomes a
*coroutine function*. And, completing the comparison, if you
evaluate a generator function, a *generator* object is returned, similar
to how a coroutine function, when evaluated, returns a coroutine
object:

.. code-block:: python3

    >>> def g():
    ...     yield 123
    ...
    >>> obj = g()
    >>> type(obj)
    <class 'generator'>
    >>> inspect.isgenerator(obj)
    True

Again, this doesn't have anything to do with asyncio, but
the loose similarity between generator functions and
coroutine functions might give you a useful framework for understanding
the new coroutine functions.

Terminology For Async Generators
--------------------------------

The previous section was useful for giving you a basic framework
for understanding how coroutines and generator have similar
characteristics. Here, we show how we can also make asynchronous
generator functions!  It sounds much more complicated than it
really is, so let's jump directly to some examples:

.. code-block:: python3

    >>> import asyncio
    >>> async def ag():
    ...     yield 1
    ...     yield 2
    ...     yield 3
    ...
    >>> async def main():
    ...     async for value in ag():
    ...         print(value)
    ...
    >>> asyncio.run(main())
    1
    2
    3

If you pretend for a second that the word "async" is temporarily
removed from the code above, the behaviour of the generator
should look very familiar to you (assuming you already know how
Python's generators work). The generator function yields out
values and these values are obtained by iterating over the
generator.

The difference now is of course the presence of those "async"
words. The code sample doesn't show a good reason *why* an async
generator is being used here: that comes a bit further down.
All we want to discuss here is what these kinds of
functions and objects should be called.

Let's have a close look at the function `ag`:

.. code-block:: python3

    >>> async def ag():
    ...     yield 1
    ...
    >>> inspect.isfunction(ag)
    True

    # Ok, so it's a function...

    >>> inspect.iscoroutinefunction(ag)
    False

    # ...but it's not a coroutine function, despite "async def"

    >>> inspect.isasyncgenfunction(ag)
    True

    # Aha, so this is an "async generator function"...

    >>> inspect.isasyncgen(ag())
    True

    # ...and when evaluated, it returns an "async generator"

Hopefully you're comfortable now with how async generators look. Let's
briefly discuss why you might want to use them. In the examples given
above, there was no good reason to make our generator an ``async def``
function; an ordinary generator function would have been fine. Async
generators are useful when you need to ``await`` on another coroutine
either before, or after, each ``yield``.

One example might be receiving network data from a ``StreamReader``
instance:

.. code-block:: python3

    async def new_messages(reader: StreamReader):
        while True:
            data = await reader.read(1024)
            yield data

This pattern makes for a very clean consumer of the received data:

.. code-block:: python3

    async def get_data():
        reader, writer = await asyncio.open_connection(...)
        async for data in new_messages(reader):
            do_something_with(data)

Async generators allow you to improve your abstractions: for
example, you can go one level higher and handle reconnection
while still propagating received data out to a consumer:

.. code-block:: python3

    async def new_messages(reader: StreamReader):
        while True:
            data = await reader.read(1024)
            yield data  # (1)

    async def get_data(host, port):
        while True:
            try:
                reader, writer = await asyncio.open_connection(host, port)
                async for data in new_messages(reader):
                    if not data:
                        continue
                    yield data  # (2)
            except OSError:
                continue
            except asyncio.CancelledError:
                return

    async def main(host, port):
        async for data in get_data(host, port):
            do_something_with(data)  # (3)

    if __name__ == '__main__':
        asyncio.run(main(host, port))

The async generator at ``(1)`` provides results back to an intermediate
async generator at ``(2)``, which does *the same thing* but also handles
reconnection events in its local scope. Finally, at ``(3)``, The async
iterator elegantly produces the received data, and internal reconnection
events (and any other lower level state management) are hidden from the
high-level logic of the application.

Async Context Managers
----------------------

In the previous section we showed how async generators can be driven
with the new ``async for`` syntax. There is also a version of
a *context manager* that can be used with ``asyncio``.

.. note:: There is a common misconception that one **must** use
    async context managers in ``asyncio`` applications.  This is not the
    case. Async context managers are needed only if you need to ``await``
    a coroutine in the *enter* or *exit* parts of the context manager.
    You do *not* required to use an async context manager if there are ``await``
    statements inside only the *body* of the context manager.

Just as the ``contextlib`` library provides the ``@contextmanager``
decorator to let us easily make context managers, so does the
``@asynccontextmanager`` let us do that for async context managers.

Imagine a very simple example where we might want to have a
connection closed during cancellation, and how about adding some
logging around the connection lifecycle events:

.. code-block:: python3

    import asyncio
    import logging
    from contextlib import asynccontextmanager

    @asynccontextmanager
    async def open_conn_logged(*args, **kwargs):
        logging.info('Opening connection...')
        reader, writer = await asyncio.open_connection(*args, **kwargs)
        logging.info('Connection opened.')
        try:
            yield reader, writer  # (1)
        finally:
            logging.info('Cleaning up connection...')
            if not writer.is_closing():
                await writer.close()
            logging.info('Connection closed.')

    async def echo():
        async with open_conn_logged('localhost', 8000) as (reader, writer):
            data = await reader.read(1024)
            await writer.write(data)

    if __name__ == '__main__':
        asyncio.run(echo())

At line marked ``(1)``, data is provided to the context inside the ``echo()``
function. You can see how the ``async with`` keywords are required to
work with the async context manager.

Async context managers are likely to appear in projects using
``asyncio`` because the need to safely close or dispose of resources is
very common in network programming.
