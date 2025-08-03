.. _a-conceptual-overview-of-asyncio:

****************************************
A Conceptual Overview of :mod:`!asyncio`
****************************************

This article seeks to help you build a sturdy mental model of how :mod:`asyncio`
fundamentally works, helping you understand the how and why behind the
recommended patterns.

You might be curious about some key :mod:`!asyncio` concepts.
You'll be comfortably able to answer these questions by the end of this
article.

- What's happening behind the scenes when an object is ``await``\ ed?
- How does :mod:`!asyncio` differentiate between a task which doesn't need
  CPU-time (such as a network request or file read) as opposed to a task that
  does (such as computing n-factorial)?
- How would I go about writing my own asynchronous variant of some operation?
  Something like an async sleep, database request, and so on.

.. seealso::

   The `guide <https://github.com/anordin95/a-conceptual-overview-of-asyncio/
   tree/main>`_ which inspired this HOWTO article.

--------------------------------------------
A conceptual overview part 1: the high-level
--------------------------------------------

In part 1, we'll cover the main, high-level building blocks of :mod:`!asyncio`:
the event loop, coroutine functions, coroutine objects, tasks and ``await``.

==========
Event Loop
==========

Everything in :mod:`!asyncio` happens relative to the event loop.
It's the star of the show.
It's like an orchestra conductor.
It's behind the scenes managing resources.
Some power is explicitly granted to it, but a lot of its ability to get things
done comes from the respect and cooperation of its teammates.

In more technical terms, the event loop contains a queue of jobs (or "chunks
of work") to be run.
Some jobs are added directly by you, and some indirectly by :mod:`!asyncio`.
The event loop pops a job from the queue and invokes it (or "gives it control"),
similar to calling a function, then that job runs.
Once it pauses or completes, it returns control to the event loop.
The event loop will then move on to the next job in its queue and invoke it.
This process repeats indefinitely.
Even if the queue is empty, the event loop continues to cycle (somewhat
aimlessly).

Effective execution relies on tasks sharing well: a greedy job could hog
control and leave the other tasks to starve, rendering the overall event loop
approach rather useless.

::

   import asyncio

   # This creates an event loop and indefinitely cycles through
   # its queue of tasks.
   event_loop = asyncio.new_event_loop()
   event_loop.run_forever()

=====================================
Asynchronous Functions and Coroutines
=====================================

This is a regular 'ol Python function::

   def hello_printer():
       print(
           "Hi, I am a lowly, simple printer, though I have all I "
           "need in life -- \nfresh paper and my dearly beloved octopus "
           "partner in crime."
       )

Calling a regular function invokes its logic or body::

   >>> hello_printer()
   Hi, I am a lowly, simple printer, though I have all I need in life --
   fresh paper and my dearly beloved octopus partner in crime.

The :ref:`async def <async def>`, as opposed to just a plain ``def``, makes
this an asynchronous function (or "coroutine function").
Calling it creates and returns a :ref:`coroutine <coroutine>` object.

::

   async def loudmouth_penguin(magic_number: int):
       print(
        "I am a super special talking penguin. Far cooler than that printer. "
        f"By the way, my lucky number is: {magic_number}."
       )

Note that calling it does not execute the function::

   >>> loudmouth_penguin(magic_number=3)
   <coroutine object loudmouth_penguin at 0x104ed2740>

The terms "asynchronous function" and "coroutine object" are often conflated
as coroutine.
That can be confusing!
In this article, coroutine specifically refers to a coroutine object, or more
precisely, an instance of :data:`types.CoroutineType` (native coroutine).
Note that coroutines can also exist as instances of
:class:`collections.abc.Coroutine` -- a distinction that matters for type
checking.

A coroutine represents the function's body or logic.
A coroutine has to be explicitly started; again, merely creating the coroutine
does not start it.
Notably, the coroutine can be paused and resumed at various points within the
function's body.
That pausing and resuming ability is what allows for asynchronous behavior!

Coroutines and coroutine functions were built by leveraging the functionality
of :term:`generators <generator iterator>` and
:term:`generator functions <generator>`.
Recall, a generator function is a function that :keyword:`yield`\s, like this
one::

   def get_random_number():
       # This would be a bad random number generator!
       print("Hi")
       yield 1
       print("Hello")
       yield 7
       print("Howdy")
       yield 4
       ...

Similar to a coroutine function, calling a generator function does not run it.
Instead, it provides a generator object::

   >>> get_random_number()
   <generator object get_random_number at 0x1048671c0>

You can proceed to the next ``yield`` of a generator by using the
built-in function :func:`next`.
In other words, the generator runs, then pauses.
For example::

   >>> generator = get_random_number()
   >>> next(generator)
   Hi
   1
   >>> next(generator)
   Hello
   7

=====
Tasks
=====

Roughly speaking, :ref:`tasks <asyncio-task-obj>` are coroutines (not coroutine
functions) tied to an event loop.
A task also maintains a list of callback functions whose importance will become
clear in a moment when we discuss ``await``.
The recommended way to create tasks is via :func:`asyncio.create_task`.
Creating a task automatically adds it to the event loop's queue of tasks.

Since there's only one event loop (in each thread), :mod:`!asyncio` takes care of
associating the task with the event loop for you. As such, there's no need
to specify the event loop.

::

   # This creates a Task object and puts it on the event loop's queue.
   task = asyncio.create_task(coro=loudmouth_penguin(magic_number=5))

=====
await
=====

:keyword:`await` is a Python keyword that's commonly used in one of two
different ways::

   await task
   await coroutine

Unfortunately, it does matter which type of object is awaited.

``await``\ ing a task will cede control from the current task or coroutine to
the event loop.
And while doing so, adds a callback to the awaited task's list of callbacks
indicating it should resume the current task/coroutine when it (the
``await``\ ed one) finishes.
In other words, when that awaited task finishes, the original task is added
back to the event loops queue.

This is a basic, yet reliable mental model.
In practice, it's slightly more complex, but not by much.
In part 2, we'll walk through the details that make this possible.

**Unlike tasks, awaiting a coroutine does not cede control!**
Wrapping a coroutine in a task first, then ``await``\ ing that would cede
control.
The behavior of ``await coroutine`` is effectively the same as invoking a
regular, synchronous Python function.
Consider this program::

   import asyncio

   async def coro_a():
      print("I am coro_a(). Hi!")

   async def coro_b():
      print("I am coro_b(). I sure hope no one hogs the event loop...")

   async def main():
      task_b = asyncio.create_task(coro_b())
      num_repeats = 3
      for _ in range(num_repeats):
         await coro_a()
      await task_b

   asyncio.run(main())

The first statement in the coroutine ``main()`` creates ``task_b`` and places
it on the event loop's queue.
Then, ``coro_a()`` is repeatedly ``await``\ ed. Control never cedes to the
event loop which is why we see the output of all three ``coro_a()``
invocations before ``coro_b()``'s output:

.. code-block:: none

   I am coro_a(). Hi!
   I am coro_a(). Hi!
   I am coro_a(). Hi!
   I am coro_b(). I sure hope no one hogs the event loop...

If we change ``await coro_a()`` to ``await asyncio.create_task(coro_a())``, the
behavior changes.
The coroutine ``main()`` cedes control to the event loop with that statement.
The event loop then works through its queue, calling ``coro_b()`` and then
``coro_a()`` before resuming the coroutine ``main()``.

.. code-block:: none

   I am coro_b(). I sure hope no one hogs the event loop...
   I am coro_a(). Hi!
   I am coro_a(). Hi!
   I am coro_a(). Hi!

------------------------------------------------
A conceptual overview part 2: the nuts and bolts
------------------------------------------------

Part 2 goes into detail on the mechanisms :mod:`!asyncio` uses to manage
control flow.
This is where the magic happens.
You'll come away from this section knowing what await does behind the scenes
and how to make your own asynchronous operators.

================================================
coroutine.send(), await, yield and StopIteration
================================================

:mod:`!asyncio` leverages 4 components to pass around control.

:meth:`coroutine.send(arg) <generator.send>` is the method used to start or
resume a coroutine.
If the coroutine was paused and is now being resumed, the argument ``arg``
will be sent in as the return value of the ``yield`` statement which originally
paused it.
If the coroutine is being started, as opposed to resumed, ``arg`` must be
``None``.

:ref:`yield <yieldexpr>`, like usual, pauses execution and returns control
to the caller.
In the example below, the ``yield``, on line 3, is called by
``... = await rock`` on line 11.
Generally, ``await`` calls the :meth:`~object.__await__` method of the given
object.
``await`` also does one more very special thing: it propagates (or "passes
along") any ``yield``\ s it receives up the call-chain.
In this case, that's back to ``... = coroutine.send(None)`` on line 16.

The coroutine is resumed via the ``coroutine.send(42)`` call on line 21.
The coroutine picks back up from where it ``yield``\ ed (or paused) on line 3
and executes the remaining statements in its body.
When a coroutine finishes, it raises a :exc:`StopIteration` exception with the
return value attached in the :attr:`~StopIteration.value` attribute.

.. code-block::
   :linenos:

   class Rock:
       def __await__(self):
           value_sent_in = yield 7
           print(f"Rock.__await__ resuming with value: {value_sent_in}.")
           return value_sent_in

   async def main():
       print("Beginning coroutine main().")
       rock = Rock()
       print("Awaiting rock...")
       value_from_rock = await rock
       print(f"Coroutine received value: {value_from_rock} from rock.")
       return 23

   coroutine = main()
   intermediate_result = coroutine.send(None)
   print(f"Coroutine paused and returned intermediate value: {intermediate_result}.")

   print(f"Resuming coroutine and sending in value: 42.")
   try:
       coroutine.send(42)
   except StopIteration as e:
       returned_value = e.value
   print(f"Coroutine main() finished and provided value: {returned_value}.")

That snippet produces this output:

.. code-block:: none

   Beginning coroutine main().
   Awaiting rock...
   Coroutine paused and returned intermediate value: 7.
   Resuming coroutine and sending in value: 42.
   Rock.__await__ resuming with value: 42.
   Coroutine received value: 42 from rock.
   Coroutine main() finished and provided value: 23.

It's worth pausing for a moment here and making sure you followed the various
ways that control flow and values were passed.

The only way to yield (or effectively cede control) from a coroutine is to
``await`` an object that ``yield``\ s in its ``__await__`` method.
That might sound odd to you. You might be thinking:

   1. What about a ``yield`` directly within the coroutine? The coroutine becomes
   an async generator, a different beast entirely.

   2. What about a ``yield from`` within the coroutine to a function that yields
   (that is, a plain generator)?
   ``SyntaxError: yield from not allowed in a coroutine.``
   This was intentionally designed for the sake of simplicity -- mandating only
   one way of using coroutines. Originally ``yield`` was actually barred as well,
   but was re-accepted to allow for async generators.
   Despite that, ``yield from`` and ``await`` effectively do the same thing.

=======
Futures
=======

A :ref:`future <asyncio-future-obj>` is an object meant to represent a
computation's status and result.
The term is a nod to the idea of something still to come or not yet happened,
and the object is a way to keep an eye on that something.

A future has a few important attributes. One is its state which can be either
"pending", "cancelled" or "done".
Another is its result which is set when the state transitions to done.
Unlike a coroutine, a future does not represent the actual computation to be
done; instead, it represents the status and result of that computation, kind of
like a status light (red, yellow or green) or indicator.

:class:`asyncio.Task` subclasses :class:`asyncio.Future` in order to gain
these various capabilities.
The prior section said tasks store a list of callbacks, which wasn't entirely
true.
It's actually the ``Future`` class that implements this logic which ``Task``
inherits.

Futures may also be used directly (not via tasks).
Tasks mark themselves as done when their coroutine is complete.
Futures are much more versatile and will be marked as done when you say so.
In this way, they're the flexible interface for you to make your own conditions
for waiting and resuming.

========================
A homemade asyncio.sleep
========================

We'll go through an example of how you could leverage a future to create your
own variant of asynchronous sleep (``async_sleep``) which mimics
:func:`asyncio.sleep`.

This snippet puts a few tasks on the event loop's queue and then ``await``\ s a
coroutine wrapped in a task: ``async_sleep(3)``.
We want that task to finish only after three seconds have elapsed, but without
preventing other tasks from running.

::

   async def other_work():
       print("I am worker. Work work.")

   async def main():
       # Add a few other tasks to the event loop, so there's something
       # to do while asynchronously sleeping.
       work_tasks = [
           asyncio.create_task(other_work()),
           asyncio.create_task(other_work()),
           asyncio.create_task(other_work())
       ]
       print(
           "Beginning asynchronous sleep at time: "
           f"{datetime.datetime.now().strftime("%H:%M:%S")}."
       )
       await asyncio.create_task(async_sleep(3))
       print(
           "Done asynchronous sleep at time: "
           f"{datetime.datetime.now().strftime("%H:%M:%S")}."
       )
       # asyncio.gather effectively awaits each task in the collection.
       await asyncio.gather(*work_tasks)


Below, we use a future to enable custom control over when that task will be
marked as done.
If :meth:`future.set_result() <asyncio.Future.set_result>` (the method
responsible for marking that future as done) is never called, then this task
will never finish.
We've also enlisted the help of another task, which we'll see in a moment, that
will monitor how much time has elapsed and accordingly call
``future.set_result()``.

::

   async def async_sleep(seconds: float):
       future = asyncio.Future()
       time_to_wake = time.time() + seconds
       # Add the watcher-task to the event loop.
       watcher_task = asyncio.create_task(_sleep_watcher(future, time_to_wake))
       # Block until the future is marked as done.
       await future

We'll use a rather bare object, ``YieldToEventLoop()``, to ``yield`` from
``__await__`` in order to cede control to the event loop.
This is effectively the same as calling ``asyncio.sleep(0)``, but this approach
offers more clarity, not to mention it's somewhat cheating to use
``asyncio.sleep`` when showcasing how to implement it!

As usual, the event loop cycles through its queue of tasks, giving them control
and receiving control back when they pause or finish.
The ``watcher_task``, which runs the coroutine ``_sleep_watcher(...)``, will
be invoked once per full cycle of the event loop's queue.
On each resumption, it'll check the time and if not enough has elapsed, then
it'll pause once again and hand control back to the event loop.
Eventually, enough time will have elapsed, and ``_sleep_watcher(...)`` will
mark the future as done, and then itself finish too by breaking out of the
infinite ``while`` loop.
Given this helper task is only invoked once per cycle of the event loop's queue,
you'd be correct to note that this asynchronous sleep will sleep *at least*
three seconds, rather than exactly three seconds.
Note this is also of true of ``asyncio.sleep``.

::

   class YieldToEventLoop:
       def __await__(self):
           yield

   async def _sleep_watcher(future, time_to_wake):
       while True:
           if time.time() >= time_to_wake:
               # This marks the future as done.
               future.set_result(None)
               break
           else:
               await YieldToEventLoop()

Here is the full program's output:

.. code-block:: none

   $ python custom-async-sleep.py
   Beginning asynchronous sleep at time: 14:52:22.
   I am worker. Work work.
   I am worker. Work work.
   I am worker. Work work.
   Done asynchronous sleep at time: 14:52:25.

You might feel this implementation of asynchronous sleep was unnecessarily
convoluted.
And, well, it was.
The example was meant to showcase the versatility of futures with a simple
example that could be mimicked for more complex needs.
For reference, you could implement it without futures, like so::

   async def simpler_async_sleep(seconds):
       time_to_wake = time.time() + seconds
       while True:
           if time.time() >= time_to_wake:
               return
           else:
               await YieldToEventLoop()

But, that's all for now. Hopefully you're ready to more confidently dive into
some async programming or check out advanced topics in the
:mod:`docs <asyncio>`.
