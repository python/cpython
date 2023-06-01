:mod:`asyncio` --- Asynchronous I/O
===================================

.. module:: asyncio
   :synopsis: Asynchronous I/O.

-------------------------------

.. sidebar:: Hello World!

   ::

       import asyncio

       async def main():
           print('Hello ...')
           await asyncio.sleep(1)
           print('... World!')

       asyncio.run(main())

asyncio is a library to write **concurrent** code using
the **async/await** syntax.

asyncio is used as a foundation for multiple Python asynchronous
frameworks that provide high-performance network and web-servers,
database connection libraries, distributed task queues, etc.

asyncio is often a perfect fit for IO-bound and high-level
**structured** network code.

asyncio provides a set of **high-level** APIs to:

* :ref:`run Python coroutines <coroutine>` concurrently and
  have full control over their execution;

* perform :ref:`network IO and IPC <asyncio-streams>`;

* control :ref:`subprocesses <asyncio-subprocess>`;

* distribute tasks via :ref:`queues <asyncio-queues>`;

* :ref:`synchronize <asyncio-sync>` concurrent code;

Additionally, there are **low-level** APIs for
*library and framework developers* to:

* create and manage :ref:`event loops <asyncio-event-loop>`, which
  provide asynchronous APIs for :meth:`networking <loop.create_server>`,
  running :meth:`subprocesses <loop.subprocess_exec>`,
  handling :meth:`OS signals <loop.add_signal_handler>`, etc;

* implement efficient protocols using
  :ref:`transports <asyncio-transports-protocols>`;

* :ref:`bridge <asyncio-futures>` callback-based libraries and code
  with async/await syntax.


.. We use the "rubric" directive here to avoid creating
   the "Reference" subsection in the TOC.

.. rubric:: Reference

.. toctree::
   :caption: High-level APIs
   :maxdepth: 1

   asyncio-task.rst
   asyncio-stream.rst
   asyncio-sync.rst
   asyncio-subprocess.rst
   asyncio-queue.rst
   asyncio-exceptions.rst

.. toctree::
   :caption: Low-level APIs
   :maxdepth: 1

   asyncio-eventloop.rst
   asyncio-future.rst
   asyncio-protocol.rst
   asyncio-policy.rst
   asyncio-platforms.rst

.. toctree::
   :caption: Guides and Tutorials
   :maxdepth: 1

   asyncio-api-index.rst
   asyncio-llapi-index.rst
   asyncio-dev.rst

.. note::
   The source code for asyncio can be found in :source:`Lib/asyncio/`.
