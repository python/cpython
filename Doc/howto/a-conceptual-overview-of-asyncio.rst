.. _a-conceputal-overview-of-asyncio:

*********************************
A Conceptual Overview of asyncio
*********************************

:Author: Alexander Nordin

This article seeks to help you build a sturdy mental model of how asyncio
fundamentally works.
Something that will help you understand the how and why behind the recommended
patterns.

During my own asyncio learning process, a few aspects particually drove my
curiosity (read: drove me nuts).
You should be able to comfortably answer all these questions by the end
of this article.

- What's roughly happening behind the scenes when an object is ``await``-ed?
- How does asyncio differentiate between a task which doesn't need CPU-time
  to make progress towards completion, for example, a network request or file
  read as opposed to a task that does need cpu-time to make progress, like
  computing n-factorial?
- How would I go about writing my own asynchronous variant of some operation?
  Something like an async sleep, database request, etc.

---------------------------------------------
A conceptual overview part 1: the high-level
---------------------------------------------

In part 1, we'll cover the main, high-level building blocks of asyncio: the
event-loop, coroutine functions, coroutine objects, tasks & ``await``.


==========
Event Loop
==========

Everything in asyncio happens relative to the event-loop.
It's the star of the show and there's only one.
It's kind of like an orchestra conductor or military general.
She's behind the scenes managing resources.
Some power is explicitly granted to her, but a lot of her ability to get things
done comes from the respect & cooperation of her subordinates.

In more technical terms, the event-loop contains a queue of tasks to be run.
Some tasks are added directly by you, and some indirectly by asyncio.
The event-loop pops a task from the queue and invokes it (or gives it control),
similar to calling a function.
That task then runs.
Once it pauses or completes, it returns control to the event-loop.
The event-loop will then move on to the next task in its queue and invoke it.
This process repeats indefinitely.
Even if the queue is empty, the event-loop continues to cycle (somewhat aimlessly).

Effective overall execution relies on tasks sharing well.
A greedy task could hog control and leave the other tasks to starve rendering
the overall event-loop approach rather useless.

::

    import asyncio

    # This creates an event-loop and indefinitely cycles through
    # its queue of tasks.
    event_loop = asyncio.new_event_loop()
    event_loop.run_forever()

===================================
Asynchronous Functions & Coroutines
===================================

This is a regular 'ol Python function::

    def hello_printer():
        print(
            "Hi, I am a lowly, simple printer, though I have all I "
            "need in life -- \nfresh paper & a loving octopus-wife."
        )

Calling a regular function invokes its logic or body::

    >>> hello_printer()
    Hi, I am a lowly, simple printer, though I have all I need in life --
    fresh paper & a loving octopus-wife.
    >>>

This is an asynchronous-function or coroutine-function::

    async def special_fella(magic_number: int):
        print(
            "I am a super special function. Far cooler than that printer. "
            f"By the way, my lucky number is: {magic_number}."
        )

Calling an asynchronous function creates and returns a coroutine object. It
does not execute the function::

    >>> special_fella(magic_number=3)
    <coroutine object special_fella at 0x104ed2740>
    >>>

The terms "asynchronous function" (or "coroutine function") and "coroutine object"
are often conflated as coroutine.
I find that a tad confusing.
In this article, coroutine will exclusively mean "coroutine object" -- the
thing produced by executing a coroutine function.

That coroutine represents the function's body or logic.
A coroutine has to be explicitly started; again, merely creating the coroutine
does not start it.
Notably, the coroutine can be paused & resumed at various points within the
function's body.
That pausing & resuming ability is what allows for asynchronous behavior!

=====
Tasks
=====

Roughly speaking, tasks are coroutines (not coroutine functions) tied to an
event-loop.
A task also maintains a list of callback functions whose importance will become
clear in a moment when we discuss ``await``.
When tasks are created they are automatically added to the event-loop's queue
of tasks::

    # This creates a Task object and puts it on the event-loop's queue.
    special_task = asyncio.Task(coro=special_fella(magic_number=5), loop=event_loop)

It's common to see a task instantiated without explicitly specifying the event-loop
it belongs to.
Since there's only one event-loop (a global singleton), asyncio made the loop
argument optional and will add it for you if it's left unspecified::

    # This creates another Task object and puts it on the event-loop's queue.
    # The task is implicitly tied to the event-loop by asyncio since the
    # loop argument was left unspecified.
    another_special_task = asyncio.Task(coro=special_fella(magic_number=12))

=====
await
=====

``await`` is a Python keyword that's commonly used in one of two different ways::

    await task
    await coroutine

Unfortunately, it actually does matter which type of object await is applied to.

``await``-ing a task will cede control from the current task or coroutine to
the event-loop.
And while doing so, add a callback to the awaited task's list of callbacks
indicating it should resume the current task/coroutine when it (the
``await``-ed one) finishes.
Said another way, when that awaited task finishes, it adds the original task
back to the event-loops queue.

In practice, it's slightly more convoluted, but not by much.
In part 2, we'll walk through the details that make this possible.
And in the control flow analysis example we'll walk through, in precise detail,
the various control handoffs in an example async program.

**Unlike tasks, await-ing a coroutine does not cede control!**
Wrapping a coroutine in a task first, then ``await``-ing that would cede control.
The behavior of ``await coroutine`` is effectively the same as invoking a regular,
synchronous Python function.
Consider this program::

    import asyncio

    async def coro_a():
        print("I am coro_a(). Hi!")

    async def coro_b():
        print("I am coro_b(). I sure hope no one hogs the event-loop...")

    async def main():
        task_b = asyncio.Task(coro_b())
        num_repeats = 3
        for _ in range(num_repeats):
            await coro_a()
        await task_b

    asyncio.run(main())

The first statement in the coroutine ``main()`` creates ``task_b`` and places
it on the event-loops queue.
Then, ``coro_a()`` is repeatedly ``await``-ed. Control never cedes to the
event-loop which is why we see the output of all three ``coro_a()``
invocations before ``coro_b()``'s output:

.. code-block:: none

    I am coro_a(). Hi!
    I am coro_a(). Hi!
    I am coro_a(). Hi!
    I am coro_b(). I sure hope no one hogs the event-loop...

If we change ``await coro_a()`` to ``await asyncio.Task(coro_a())``, the
behavior changes.
The coroutine ``main()`` cedes control to the event-loop with that statement.
The event-loop then works through its queue, calling ``coro_b()`` and then
``coro_a()`` before resuming the coroutine ``main()``.

.. code-block:: none

    I am coro_b(). I sure hope no one hogs the event-loop...
    I am coro_a(). Hi!
    I am coro_a(). Hi!
    I am coro_a(). Hi!


----------------------------------------------
A conceptual overview part 2: the nuts & bolts
----------------------------------------------

Part 2 goes into detail on the mechanisms asyncio uses to manage control flow.
This is where the magic happens.
You'll come away from this section knowing what await does behind the scenes
and how to make your own asynchronous operators.

==============================================
coroutine.send(), await, yield & StopIteration
==============================================

asyncio leverages those 4 components to pass around control.

``coroutine.send(arg)`` is the method used to start or resume a coroutine.
If the coroutine was paused and is now being resumed, the argument ``arg``
will be sent in as the return value of the ``yield`` statement which originally
paused it.
If the coroutine is being started, as opposed to resumed, ``arg`` must be None.

``yield``, like usual, pauses execution and returns control to the caller.
In the example below, the ``yield`` is on line 3 and the caller is
``... = await rock`` on line 11.
Generally, ``await`` calls the ``__await__`` method of the given object.
``await`` also does one more very special thing: it percolates (or passes along)
any yields it receives up the call-chain.
In this case, that's back to ``... = coroutine.send(None)`` on line 16.

The coroutine is resumed via the ``coroutine.send(42)`` call on line 21.
The coroutine picks back up from where it ``yield``-ed (i.e. paused) on line 3
and executes the remaining statements in its body.
When a coroutine finishes it raises a ``StopIteration`` exception with the
return value attached to the exception.

::

    1   class Rock:
    2       def __await__(self):
    3           value_sent_in = yield 7
    4           print(f"Rock.__await__ resuming with value: {value_sent_in}.")
    5           return value_sent_in
    6
    7   async def main():
    8       print("Beginning coroutine main().")
    9       rock = Rock()
    10      print("Awaiting rock...")
    11      value_from_rock = await rock
    12      print(f"Coroutine received value: {value_from_rock} from rock.")
    13      return 23
    14
    15  coroutine = main()
    16  intermediate_result = coroutine.send(None)
    17  print(f"Coroutine paused and returned intermediate value: {intermediate_result}.")
    18
    19  print(f"Resuming coroutine and sending in value: 42.")
    20  try:
    21      coroutine.send(42)
    22  except StopIteration as e:
    23      returned_value = e.value
    24  print(f"Coroutine main() finished and provided value: {returned_value}.")

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
ways control flow and values were passed.

The only way to yield (or effectively cede control) from a coroutine is to
``await`` an object that ``yield``\ s in its ``__await__`` method.
That might sound odd to you. Frankly, it was to me too. You might be thinking:

    1. What about a ``yield`` directly within the coroutine? The coroutine becomes
    a generator-coroutine, a different beast entirely.

    2. What about a ``yield from`` within the coroutine to a function that yields
    (i.e. plain generator)?
    ``SyntaxError: yield from not allowed in a coroutine.``
    I imagine Python made this a ``SyntaxError`` to mandate only one way of using
    coroutines for the sake of simplicity.
    Ideologically, ``yield from`` and ``await`` are quite similar.

=======
Futures
=======

A future is an object meant to represent a computation or process's status and
result.
The term is a nod to the idea of something still to come or not yet happened,
and the object is a way to keep an eye on that something.

A future has a few important attributes. One is its state which can be either
pending, cancelled or done.
Another is its result which is set when the state transitions to done.
To be clear, a future does not represent the actual computation to be done, like
a coroutine does, instead it represents the status and result of that computation,
kind of like a status-light (red, yellow or green) or indicator.

``Task`` subclasses ``Future`` in order to gain these various capabilities.
I said in the prior section tasks store a list of callbacks and I lied a bit.
It's actually the ``Future`` class that implements this logic which ``Task``
inherits.

Futures may be also used directly i.e. not via tasks.
Tasks mark themselves as done when their coroutine's complete.
Futures are much more versatile and will be marked as done when you say so.
In this way, they're the flexible interface for you to make your own conditions
for waiting and resuming.

=========================
await-ing Tasks & futures
=========================

``Future`` defines an important method: ``__await__``. Below is the actual
implementation (well, one line was removed for simplicity's sake) found
in ``asyncio.futures.Future``.
It's okay if it doesn't make complete sense now, we'll go through it in detail
in the control-flow example.

::

    1  class Future:
    2      ...
    3      def __await__(self):
    4
    5          if not self.done():
    6              yield self
    7
    8          if not self.done():
    9              raise RuntimeError("await wasn't used with future")
    10
    11         return self.result()

The ``Task`` class does not override ``Future``'s ``__await__`` implementation.
``await``-ing a task or future invokes that above ``__await__`` method and
percolates the ``yield`` on line 6 to relinquish control to its caller, which
is generally the event-loop.

========================
A homemade asyncio.sleep
========================

We'll go through an example of how you could leverage a future to create your
own variant of asynchronous sleep (i.e. asyncio.sleep).

This snippet puts a few tasks on the event-loops queue and then ``await``\ s a
yet unknown coroutine wrapped in a task: ``async_sleep(3)``.
We want that task to finish only after 3 seconds have elapsed, but without
hogging control while waiting.

::

    async def other_work():
        print(f"I am worker. Work work.")

    async def main():
        # Add a few other tasks to the event-loop, so there's something
        # to do while asynchronously sleeping.
        work_tasks = [
            asyncio.Task(other_work()),
            asyncio.Task(other_work()),
            asyncio.Task(other_work())
        ]
        print(
            "Beginning asynchronous sleep at time: "
            f"{datetime.datetime.now().strftime("%H:%M:%S")}."
        )
        await asyncio.Task(async_sleep(3))
        print(
            "Done asynchronous sleep at time: "
            f"{datetime.datetime.now().strftime("%H:%M:%S")}."
        )
        # asyncio.gather effectively awaits each task in the collection.
        await asyncio.gather(*work_tasks)


Below, we use a future to enable custom control over when that task will be marked
as done.
If ``future.set_result()``, the method responsible for marking that future as
done, is never called, this task will never finish.
We've also enlisted the help of another task, which we'll see in a moment, that
will monitor how much time has elapsed and accordingly call
``future.set_result()``.

::

    async def async_sleep(seconds: float):
        future = asyncio.Future()
        time_to_wake = time.time() + seconds
        # Add the watcher-task to the event-loop.
        watcher_task = asyncio.Task(_sleep_watcher(future, time_to_wake))
        # Block until the future is marked as done.
        await future


We'll use a rather bare object ``YieldToEventLoop()`` to ``yield`` from its
``__await__`` in order to cede control to the event-loop.
This is effectively the same as calling ``asyncio.sleep(0)``, but I prefer the
clarity this approach offers, not to mention it's somewhat cheating to use
``asyncio.sleep`` when showcasing how to implement it!

The event-loop, as usual, cycles through its queue of tasks, giving them control,
and receiving control back when each task pauses or finishes.
The ``watcher_task``, which runs the coroutine: ``_sleep_watcher(...)`` will be
invoked once per full cycle of the event-loop's queue.
On each resumption, it'll check the time and if not enough has elapsed, it'll
pause once again and return control to the event-loop.
Eventually, enough time will have elapsed, and ``_sleep_watcher(...)`` will
mark the future as done, and then itself finish too by breaking out of the
infinite while loop.
Given this helper task is only invoked once per cycle of the event-loop's queue,
you'd be correct to note that this asynchronous sleep will sleep **at least**
three seconds, rather than exactly three seconds.
Note, this is also of true of the library-provided asynchronous function:
``asyncio.sleep``.

::

    class YieldToEventLoop:
        def __await__(self):
            yield

    async def _sleep_watcher(future: asyncio.Future, time_to_wake: float):
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
I wanted to showcase the versatility of futures with a simple example that
could be mimicked for more complex needs.
For reference, you could implement it without futures, like so::

    async def simpler_async_sleep(seconds):
        time_to_wake = time.time() + seconds
        while True:
            if time.time() >= time_to_wake:
                return
            else:
                await YieldToEventLoop()

