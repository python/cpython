Why Asyncio?
============

There are two very specific reasons for using ``async def`` functions:

#. Safety: easier reasoning about concurrency, and virtually
   eliminate `memory races <https://en.wikipedia.org/wiki/Race_condition#Software>`_
   in concurrent network code
#. High concurrency: huge number of open socket connections

Safety
------

- async/await makes all context switches visible; that makes it easy
  to spot race conditions and
  `reason about your code <https://glyph.twistedmatrix.com/2014/02/unyielding.html>`_

- in general, all datastructures are safe for async (we cannot say same
  for threads)

- an async/await library means that it's safe to use it in concurrent
  async/await code (you can never be sure if some library is thread-safe,
  even if it claims that)

- language constructs like 'async for' and 'async with' enable structured
  concurrency

High Concurrency
----------------

- high-throughput IO or 1000s of long-living connections are only
  doable with asyncio

- if you don't need to scale your code right now but might need
  in near future investing in async/await is wise
