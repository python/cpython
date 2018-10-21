Functions: Sync vs Async
========================

Regular Python functions are created with the keyword ``def``,
and look like this:

.. code-block:: python

    def f(x, y):
        print(x + y)

    # Evaluate the function
    f(1, 2)

Async functions are different in two respects:

.. code-block:: python

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

.. code-block:: python

    import asyncio

    async def f(x, y):
        print(x + y)

    async def main():
        await f(1, 2)

    asyncio.run(main())


The execution of the first async function, ``main()``, is performed
with ``run()``, but once you're inside an ``async def`` function, then
all you need to execute another async function is the ``await`` keyword.

TODO:
- which kind of functions can be called from which other kind
- use the "inspect" module to verify the formal names of functions,
coroutine functions, coroutines, etc.


