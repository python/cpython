:mod:`asyncio` --- Asynchronous I/O
===================================

.. module:: asyncio
   :synopsis: Asynchronous I/O.

--------------

.. TODO: rewrite the introduction section

This module provides infrastructure for writing single-threaded concurrent
code using coroutines, multiplexing I/O access over sockets and other
resources, running network clients and servers, and other related primitives.
Here is a more detailed list of the package contents:

* a pluggable :ref:`event loop <asyncio-event-loop>` with various system-specific
  implementations;

* :ref:`transport <asyncio-transport>` and :ref:`protocol <asyncio-protocol>` abstractions
  (similar to those in `Twisted <https://twistedmatrix.com/trac/>`_);

* concrete support for TCP, UDP, SSL, subprocess pipes, delayed calls, and
  others (some may be system-dependent);

* a :class:`Future` class that mimics the one in the :mod:`concurrent.futures`
  module, but adapted for use with the event loop;

* coroutines and tasks based on ``yield from`` (:PEP:`380`), to help write
  concurrent code in a sequential fashion;

* cancellation support for :class:`Future`\s and coroutines;

* :ref:`synchronization primitives <asyncio-sync>` for use between coroutines in
  a single thread, mimicking those in the :mod:`threading` module;

* an interface for passing work off to a threadpool, for times when
  you absolutely, positively have to use a library that makes blocking
  I/O calls.

Asynchronous programming is more complex than classical "sequential"
programming: see the :ref:`Develop with asyncio <asyncio-dev>` page which lists
common traps and explains how to avoid them. :ref:`Enable the debug mode
<asyncio-debug-mode>` during development to detect common issues.

High-level APIs:

.. toctree::
   :maxdepth: 1

   asyncio-task.rst
   asyncio-stream.rst
   asyncio-sync.rst
   asyncio-subprocess.rst
   asyncio-queue.rst
   asyncio-exceptions.rst

Low-level APIs:

.. toctree::
   :maxdepth: 1

   asyncio-eventloop.rst
   asyncio-future.rst
   asyncio-protocol.rst
   asyncio-policy.rst
   asyncio-platforms.rst

Guides and Tutorials:

.. toctree::
   :maxdepth: 1

   asyncio-dev.rst


.. seealso::

   The :mod:`asyncio` module was proposed in :PEP:`3156`.
   Since the acceptance of the PEP many new APIs were added and many
   original APIs were altered.  The PEP should be treated as a
   historical document.
