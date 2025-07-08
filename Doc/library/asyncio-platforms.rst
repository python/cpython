.. currentmodule:: asyncio


.. _asyncio-platform-support:


================
Platform Support
================

The :mod:`asyncio` module is designed to be portable,
but some platforms have subtle differences and limitations
due to the platforms' underlying architecture and capabilities.


All Platforms
=============

* :meth:`loop.add_reader` and :meth:`loop.add_writer`
  cannot be used to monitor file I/O.


Windows
=======

**Source code:** :source:`Lib/asyncio/proactor_events.py`,
:source:`Lib/asyncio/windows_events.py`,
:source:`Lib/asyncio/windows_utils.py`

--------------------------------------

.. versionchanged:: 3.8

   On Windows, :class:`ProactorEventLoop` is now the default event loop.

All event loops on Windows do not support the following methods:

* :meth:`loop.create_unix_connection` and
  :meth:`loop.create_unix_server` are not supported.
  The :const:`socket.AF_UNIX` socket family is specific to Unix.

* :meth:`loop.add_signal_handler` and
  :meth:`loop.remove_signal_handler` are not supported.

:class:`SelectorEventLoop` has the following limitations:

* :class:`~selectors.SelectSelector` is used to wait on socket events:
  it supports sockets and is limited to 512 sockets.

* :meth:`loop.add_reader` and :meth:`loop.add_writer` only accept
  socket handles (e.g. pipe file descriptors are not supported).

* Pipes are not supported, so the :meth:`loop.connect_read_pipe`
  and :meth:`loop.connect_write_pipe` methods are not implemented.

* :ref:`Subprocesses <asyncio-subprocess>` are not supported, i.e.
  :meth:`loop.subprocess_exec` and :meth:`loop.subprocess_shell`
  methods are not implemented.

:class:`ProactorEventLoop` has the following limitations:

* The :meth:`loop.add_reader` and :meth:`loop.add_writer`
  methods are not supported.

The resolution of the monotonic clock on Windows is usually around 15.6
milliseconds.  The best resolution is 0.5 milliseconds. The resolution depends on the
hardware (availability of `HPET
<https://en.wikipedia.org/wiki/High_Precision_Event_Timer>`_) and on the
Windows configuration.


.. _asyncio-windows-subprocess:

Subprocess Support on Windows
-----------------------------

On Windows, the default event loop :class:`ProactorEventLoop` supports
subprocesses, whereas :class:`SelectorEventLoop` does not.


macOS
=====

Modern macOS versions are fully supported.

.. rubric:: macOS <= 10.8

On macOS 10.6, 10.7 and 10.8, the default event loop
uses :class:`selectors.KqueueSelector`, which does not support
character devices on these versions.  The :class:`SelectorEventLoop`
can be manually configured to use :class:`~selectors.SelectSelector`
or :class:`~selectors.PollSelector` to support character devices on
these older versions of macOS.  Example::

    import asyncio
    import selectors

    selector = selectors.SelectSelector()
    loop = asyncio.SelectorEventLoop(selector)
    asyncio.set_event_loop(loop)
