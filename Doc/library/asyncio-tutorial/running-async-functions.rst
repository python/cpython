Executing Async Functions
=========================

In a previous section we looked at the difference between sync functions
and async functions. Here we focus specifically on async functions, and
how to call them.

Imagine we have an async function, called ``my_coro_fn``, and we want to
run it. There are three ways:

1. ``asyncio.run(my_coro_fn())``
2. ``await my_coro_fn()``
3. ``asyncio.create_task(my_coro_fn())``

The first, ``asyncio.run()`` will create a new event loop and is intended
to be called only from sync code. It is typically used to start off the
whole program.

The second, ``await my_coro_fn()``, has already been covered in a previous
section and is used to both execute the async function, and wait for the
function to complete. The ``await`` keyword can only be used inside an
``async def`` function.  It is expected that *most* of your async functions
will be executed with the ``await`` keyword.

The third is something we haven't covered before: the ``asyncio.create_task()``
function. This function will call your async function, and create an
``asyncio.Task`` object to wrap it. This means that your async function will
begin executing but the code in the calling context will *not* wait for your
async function before continuing with the next code.

Let's have a look at that with an example:

.. code-block:: python3

    import asyncio

    async def f():
        await asyncio.sleep(10)
        print('f is done')

    async def g():
        await asyncio.sleep(5)
        print('g is done')

    async main():
        asyncio.create_task(f())  # (1)
        await g()                 # (2)

    asyncio.run(main())

Looking at line (1), we see that async function ``f()`` is called and
passed to ``create_task()``, and immediately after, async function ``g()``
is called with ``await g()``.

Even though ``f()`` is called first, async function ``g()`` will finish
first, and you'll see "g is done" printed before "f is done". This is because
although ``create_task()`` does schedule the given async function to be
executed, it does not wait for the call to complete, unlike when the
``await`` keyword is used.

However, note that the task returned by ``create_task()`` can indeed be
awaited, and this will make the order of calls sequential once again:

.. code-block:: python3

    import asyncio

    async def f():
        await asyncio.sleep(10)
        print('f is done')

    async def g():
        await asyncio.sleep(5)
        print('g is done')

    async main():
        task = asyncio.create_task(f())  # (1)
        await task
        await g()                 # (2)

    asyncio.run(main())

In the sample above, we specifically use the ``await`` keyword on the task
object returned by the ``create_task()`` function, and this means that
the execution of that task must complete before the next ``await g()`` call
can be started.

There are a few other ways that async functions can be started, but they
are just decoration over the three ways discussed above. For example, the
``asyncio.gather()`` function can also receive async functions:

.. code-block:: python3

    import asyncio

    async def f():
        await asyncio.sleep(10)
        print('f is done')

    async def g():
        await asyncio.sleep(5)
        print('g is done')

    async main():
        await asyncio.gather(
            asyncio.create_task(f()),
            g()
        )

    asyncio.run(main())

In this example above, we didn't explicitly use the ``await`` keyword on
the async function ``g()``, but nevertheless it will still be executed.
Inside the ``gather()`` function, the coroutine object returned by ``g()``
will be wrapped in a ``Task`` object, similar to what we're doing with
``f()``. The ``await gather()`` line above will only return once *both*
``f()`` and ``g()`` have completed (and in fact, it wasn't necessary to
wrap ``f()`` in a task at all here, but it was included just to show that
it works).
