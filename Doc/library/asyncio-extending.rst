.. currentmodule:: asyncio


=========
Extending
=========

The main direction for :mod:`asyncio` extending is writing custom *event loop*
classes. Asyncio has helpers that could be used to simplify this task.

.. note::

   Third-parties should reuse existing asyncio code with caution,
   a new Python version is free to break backward compatibility
   in *internal* part of API.


Writing a Custom Event Loop
===========================

:class:`asyncio.AbstractEventLoop` declares very many methods.  Implementing all them
from scratch is a tedious job.

A loop can get many common methods implementation for free by inheriting from
:class:`asyncio.BaseEventLoop`.

In turn, the successor should implement a bunch of *private* methods declared but not
implemented in :class:`asyncio.BaseEventLoop`.

For example, ``loop.create_connection()`` checks arguments, resolves DNS addresses, and
calls ``loop._make_socket_transport()`` that should be implemented by inherited class.
The ``_make_socket_transport()`` method is not documented and is considered as an
*internal* API.



Future and Task private constructors
====================================

:class:`asyncio.Future` and :class:`asyncio.Task` should be never created directly,
please use corresponding :meth:`loop.create_future` and :meth:`loop.create_task`,
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

   .. versionchanged:: 3.11

      *context* argument is added.



Task lifetime support
=====================

A third party task implementation should call the following functions to keep a task
visible by :func:`asyncio.all_tasks` and :func:`asyncio.current_task`:

.. function:: _register_task(task)

   Register a new *task* as managed by *asyncio*.

   Call the function from a task constructor.

.. function:: _unregister_task(task)

   Unregister a *task* from *asyncio* internal structures.

   The function should be called when a task is about to finish.

.. function:: _enter_task(loop, task)

   Switch the current task to the *task* argument.

   Call the function just before executing a portion of embedded *coroutine*
   (:meth:`coroutine.send` or :meth:`coroutine.throw`).

.. function:: _leave_task(loop, task)

   Switch the current task back from *task* to ``None``.

   Call the function just after :meth:`coroutine.send` or :meth:`coroutine.throw`
   execution.
