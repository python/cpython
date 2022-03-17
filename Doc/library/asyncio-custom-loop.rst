.. currentmodule:: asyncio


.. _asyncio-custom-loop:

=================
Custom Event Loop
=================

Asyncio can be extended by a custom event loop (and event loop policy) implemented by
third-party libraries.


.. note::
   That third-parties should reuse existing asyncio code
   (e.g. ``asyncio.BaseEventLoop``) with caution,
   a new Python version can make a change that breaks the backward
   compatibility accidentally.


Future and Task private constructors
====================================

:class:`asyncio.Future` and :class:`asyncio.Task` should be never created directly,
plase use corresponding :meth:`loop.create_future` and :meth:`loop.create_task`,
or `asyncio.create_task` factories instead.

However, during a customloop implementation the third-party library may *reuse* defaul
highly optimized asyncio future and task implementation.  For this purpose, *private*
constructor signatures are listed:

* ``Future.__init__(*, loop=None)``, where  *loop* is an optional event loop instance.


* ``Task.__init__(coro, *, loop=None, name=None, context=None)``, where *loop* is an
  optional event loop instance. The rest of arguments are described in
  :meth:`loop.create_task` description.


Task lifetime support
=====================

I
