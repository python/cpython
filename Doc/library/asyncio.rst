:mod:`asyncio` --- Asynchronous I/O, event loop, coroutines and tasks
=====================================================================

.. module:: asyncio
   :synopsis: Asynchronous I/O, event loop, coroutines and tasks.

.. versionadded:: 3.4

**Source code:** :source:`Lib/asyncio/`

--------------

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

Table of contents:

.. toctree::
   :maxdepth: 3

   asyncio-eventloop.rst
   asyncio-eventloops.rst
   asyncio-task.rst
   asyncio-protocol.rst
   asyncio-stream.rst
   asyncio-subprocess.rst
   asyncio-sync.rst
   asyncio-queue.rst
   asyncio-dev.rst

.. seealso::

   The :mod:`asyncio` module was designed in :PEP:`3156`. For a
   motivational primer on transports and protocols, see :PEP:`3153`.

