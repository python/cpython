Functions: Sync vs Async
========================

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
way.

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
all you need to execute another async function is the ``await`` keyword.

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
executed using the ``await`` keyword, and the task-based methods which
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
    ...     yield 123
    ...
    >>> async def main():
    ...     async for value in ag():
    ...         print(value)
    ...
    >>> asyncio.run(main())
    123

If you pretend for a second that the word "async" is temporarily
removed from the code above, the behaviour of the generator
should look very familiar to you (assuming you already know how
Python's generators work). The generator function yields out
values and these values are obtained by iterating over the
generator.

The difference now is of course the presence of those "async"
words. The code sample doesn't a good reason *why* an async
generator is being used here. That will come later in the
cookbook. All we want to discuss here is what these kinds of
functions and objects should be called.

Let's have a close look at the function `ag`:

.. code-block:: python3

    >>> async def ag():
    ...     yield 123
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





TODO:
- which kind of functions can be called from which other kind
- use the "inspect" module to verify the formal names of functions,
coroutine functions, coroutines, etc.


