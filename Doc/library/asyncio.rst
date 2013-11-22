:mod:`asyncio` -- Asynchronous I/O, event loop, coroutines and tasks
====================================================================

.. module:: asyncio
   :synopsis: Asynchronous I/O, event loop, coroutines and tasks.

.. versionadded:: 3.4


This module provides infrastructure for writing single-threaded concurrent
code using coroutines, multiplexing I/O access over sockets and other
resources, running network clients and servers, and other related primitives.

Here is a more detailed list of the package contents:

* a pluggable :ref:`event loop <event-loop>` with various system-specific
  implementations;

* :ref:`transport <transport>` and :ref:`protocol <protocol>` abstractions
  (similar to those in `Twisted <http://twistedmatrix.com/>`_);

* concrete support for TCP, UDP, SSL, subprocess pipes, delayed calls, and
  others (some may be system-dependent);

* a Future class that mimicks the one in the :mod:`concurrent.futures` module,
  but adapted for use with the event loop;

* coroutines and tasks based on ``yield from`` (:PEP:`380`), to help write
  concurrent code in a sequential fashion;

* cancellation support for Futures and coroutines;

* :ref:`synchronization primitives <sync>` for use between coroutines in
  a single thread, mimicking those in the :mod:`threading` module;

* an interface for passing work off to a threadpool, for times when
  you absolutely, positively have to use a library that makes blocking
  I/O calls.


Disclaimer
----------

Full documentation is not yet ready; we hope to have it written
before Python 3.4 leaves beta.  Until then, the best reference is
:PEP:`3156`.  For a motivational primer on transports and protocols,
see :PEP:`3153`.


.. XXX should the asyncio documentation come in several pages, as for logging?


.. _event-loop:

Event loops
-----------


.. _protocol:

Protocols
---------


.. _transport:

Transports
----------


.. _sync:

Synchronization primitives
--------------------------


Examples
--------

A :class:`Protocol` implementing an echo server::

   class EchoServer(asyncio.Protocol):

       TIMEOUT = 5.0

       def timeout(self):
           print('connection timeout, closing.')
           self.transport.close()

       def connection_made(self, transport):
           print('connection made')
           self.transport = transport

           # start 5 seconds timeout timer
           self.h_timeout = asyncio.get_event_loop().call_later(
               self.TIMEOUT, self.timeout)

       def data_received(self, data):
           print('data received: ', data.decode())
           self.transport.write(b'Re: ' + data)

           # restart timeout timer
           self.h_timeout.cancel()
           self.h_timeout = asyncio.get_event_loop().call_later(
               self.TIMEOUT, self.timeout)

       def eof_received(self):
           pass

       def connection_lost(self, exc):
           print('connection lost:', exc)
           self.h_timeout.cancel()

