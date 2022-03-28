.. currentmodule:: asyncio


=========================
Custom Event Loop Helpers
=========================

Asyncio can be extended by a custom event loop (and event loop policy) implemented by
third-party libraries.


Writing a Custom Event Loop
===========================

A custom loop can inherit :class:`asyncio.BaseEventLoop` and get for free many loop
methods. In turn, the base event loop requires some private methods defined by a
successor.

For example, ``loop.create_connection()`` checks arguments, resolves DNS addresses, and
calls ``loop._make_socket_transport()`` that should be implemented by inherited class.
The ``_make_socket_transport()`` method is not documented and is considered as an
*internal* API.


.. note::

   Third-parties should reuse existing asyncio code
   (e.g. ``asyncio.BaseEventLoop``) with caution,
   a new Python version is free to break backward compatibility
   in *non-public* API.


Future and Task private constructors
====================================

:class:`asyncio.Future` and :class:`asyncio.Task` should be never created directly,
plase use corresponding :meth:`loop.create_future` and :meth:`loop.create_task`,
or :func:`asyncio.create_task` factories instead.

However, third-party *event loops* may *reuse* built-in future and task implementations
for the sake of getting a complex and highly optimized code for free.

For this purpose the following, *private* constructors are listed:

.. method:: Future.__init__(*, loop=None)

Create a built-in future instance.

*loop* is an optional event loop instance.

.. method:: Task.__init__(coro, *, loop=None, name=None, context=None)

Create a built-in task instance.

*loop* is an optional event loop instance. The rest of arguments are described in
:meth:`loop.create_task` description.


Task lifetime support
=====================

A third party task implementation should call the following functions to keep a task
visible by :func:`asyncio.get_tasks` and :func:`asyncio.current_task`:

.. function:: _register_task(task)

   Register a new *task* as managed by *asyncio*.

   Call the function from a task constructor.


.. function:: _unregister_task(task)

   Unregister a *task* from *asyncio* internal structures.

   Could be called from a task ``__del__`` method or when a task is about to finish (or
   get cancelled).


.. function:: _enter_task(loop, task)

   Switch the current task to the *task* argument.

   Call the function just before executing a portion of embedded *coroutine*
   (``coro.send()`` or ``coro.throw()``).


.. function:: _leave_task(loop, task)

   Switch the current task back from *task* to ``None``.

   Call the function just after ``coro.send()`` or ``coro.throw()`` execution.
