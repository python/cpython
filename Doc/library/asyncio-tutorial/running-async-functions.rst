Three Ways To Execute Async Functions
=====================================

In a previous section we looked at the difference between sync functions
and async functions, and other async language syntax features.
Here we focus specifically on how to execute async functions.

Imagine we have an async function, called ``my_coro_fn()``, and we want to
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
first (5 seconds is shorter than 10 seconds), and you'll see "g is done"
printed before "f is done". This is because
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

.. note:: The ``create_task()`` API is useful to understand concurrency
    features in modern JavaScript, or *vice-versa* if you're coming to
    Python from the context of JavaScript. JS also has ``async``
    and ``await`` keywords, and they work *almost* exactly the same as
    described in this Python tutorial! There is however one big
    difference: In JavaScript, all async functions, when called, behave
    like ``asyncio.create_task()`` calls. Consider the following
    JavaScript code:

    .. code-block:: javascript

        async func1 () {
            return await http.get('http://example.com/1')
        }
        async func2 () {
            return await http.get('http://example.com/2')
        }
        async main () {
            task1 = func1()  // In Python: `task1 = create_task(func1())`
            task2 = func2()  // In Python: `task2 = create_task(func2())`
            [result1, result2] = [await task1, await task2]
        }

    In Python, when you see two ``await`` keywords in series, it usually
    reads as "first the one, then the other". This is because the ``await``
    keyword suspends the calling context until the coroutine returns.
    In the JavaScript shown above, that is not the case, both ``task1``
    *and* ``task2`` will run concurrently, although ``result1`` and
    ``result2`` will only be set when both tasks have completed.

    A naive translation of the JavaScript code to Python might look
    like this:

    .. code-block:: python3

        async def func1():
            return await http.get('http://example.com/1')

        async func2():
            return await http.get('http://example.com/2')

        async def main():
            coro1 = func1()
            coro2 = func2()
            [result1, result2] = [await coro1, await coro2]
        }

    However, this will *not* behave the same: ``coro2`` will begin
    running only after ``coro1`` has completed! Instead, one can use
    Python's ``create_task()`` to more closely mimic the JavaScript
    behaviour:

    .. code-block:: python3

        async def func1():
            return await http.get('http://example.com/1')

        async func2():
            return await http.get('http://example.com/2')

        async def main():
            task1 = asyncio.create_task(func1())
            task2 = asyncio.create_task(func2())
            [result1, result2] = [await task1, await task2]
        }

    Now ``task1`` and ``task2`` will run concurrently, and the results
    will be assigned only after both tasks are complete. Of course, this is
    not idiomatic in Python: the more common pattern for waiting on
    several coroutines concurrently is with the ``gather`` API, which
    includes a highly-recommended error-handling feature:

    .. code-block:: python3

        async def main():
            [result1, result2] = await asyncio.gather(
                func1(), func2(), return_exceptions=True
            )

    Setting ``return_exceptions=True`` makes raised exceptions from
    any of the given coroutines become "returned" values instead, and
    then it is up to you to check whether either of ``result1`` or
    ``result2`` is an ``Exception`` type.

    The documentation for ``asyncio.gather()`` has an important warning:
    if ``return_exceptions=False``, any exception raised from one of the
    coroutines will bubble up into your calling code. This will cause
    the ``gather`` call to terminate, but the *other* coroutines supplied
    to the ``gather()`` call will **not** be affected, and will continue
    to run.
