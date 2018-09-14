:mod:`asyncio` --- Asynchronous I/O
===================================

.. module:: asyncio
   :synopsis: Asynchronous I/O.

--------------

asyncio is a library to write **concurrent** code using
**async/await** syntax.

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

as well as **low-level** APIs for *library and framework developers* to:

* create and manage :ref:`event loops <asyncio-event-loop>`, which
  provide asynchronous APIs for networking, subprocesses, OS signals,
  etc;

* implement efficient protocols using
  :ref:`transports <asyncio-transports-protocols>`;

* :ref:`bridge <asyncio-futures>` callback-based libraries and code
  with async/await syntax.


Contents
--------

.. rubric:: High-level APIs

.. toctree::
   :maxdepth: 1

   asyncio-task.rst
   asyncio-stream.rst
   asyncio-sync.rst
   asyncio-subprocess.rst
   asyncio-queue.rst
   asyncio-exceptions.rst

.. rubric:: Low-level APIs

.. toctree::
   :maxdepth: 1

   asyncio-eventloop.rst
   asyncio-future.rst
   asyncio-protocol.rst
   asyncio-policy.rst
   asyncio-platforms.rst

.. rubric:: Guides and Tutorials

.. toctree::
   :maxdepth: 1

   asyncio-dev.rst


.. seealso::

   The :mod:`asyncio` module was proposed in :PEP:`3156`.
   Since the acceptance of the PEP many new APIs were added and many
   original APIs were altered.  The PEP should be treated as a
   historical document.
