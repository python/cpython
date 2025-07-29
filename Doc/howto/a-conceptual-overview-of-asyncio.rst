.. _a-conceputal-overview-of-asyncio:

*********************************
A Conceptual Overview of asyncio
*********************************

:Author: Alexander Nordin

This article seeks to help you build a sturdy mental model of how asyncio
fundamentally works. Something that will help you understand the how and why
behind the recommended patterns. The final section, :ref:`which_concurrency_do_I_want`, zooms out a bit and compares the
common approaches to concurrency -- multiprocessing, multithreading & asyncio -- and describes where
each is most useful.

During my own asyncio learning process, a few aspects particually drove my curiosity (read: drove me nuts). You
should be able to comfortably answer all these questions by the end of this article.

- What's roughly happening behind the scenes when an object is ``await``-ed?
- How does asyncio differentiate between a task which doesn't need CPU-time to make progress towards completion, for example, a network request or file read as opposed to a task that does need cpu-time to make progress, like computing n-factorial?
- How would I go about writing my own asynchronous variant of some operation? Something like an async sleep, database request, etc.

The first two sections feature some examples but are generally focused on theory and explaining concepts.
The next two sections are centered around examples, focused on further illustrating and reinforcing ideas
practically.

.. contents:: Sections
    :depth: 1
    :local:

---------------------------------------------
A conceptual overview part 1: the high-level
---------------------------------------------

In part 1, we'll cover the main, high-level building blocks of asyncio: the event-loop,
coroutine functions, coroutine objects, tasks & ``await``.


==========================
Event Loop
==========================

Everything in asyncio happens relative to the event-loop. It's the star of the show and there's only one.
It's kind of like an orchestra conductor or military general. She's behind the scenes managing resources.
Some power is explicitly granted to her, but a lot of her ability to get things done comes from the respect
& cooperation of her subordinates.

In more technical terms, the event-loop contains a queue of tasks to be run. Some tasks are added directly
by you, and some indirectly by asyncio. The event-loop pops a task from the queue and invokes it (or gives
it control), similar to calling a function. That task then runs. Once it pauses or completes, it returns
control to the event-loop. The event-loop will then move on to the next task in its queue and invoke it.
This process repeats indefinitely. Even if the queue is empty, the event-loop continues to cycle
(somewhat aimlessly).

Effective overall execution relies on tasks sharing well. A greedy task could hog control and leave the
other tasks to starve rendering the overall event-loop approach rather useless.

::

    import asyncio

    # This creates an event-loop and indefinitely cycles through
    # its queue of tasks.
    event_loop = asyncio.new_event_loop()
    event_loop.run_forever()

====================================
Asynchronous Functions & Coroutines
====================================

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

Calling an asynchronous function creates and returns a coroutine object. It does not execute the function::

    >>> special_fella(magic_number=3)
    <coroutine object special_fella at 0x104ed2740>
    >>>

The terms "asynchronous function" (or "coroutine function") and "coroutine object" are often conflated
as coroutine. I find that a tad confusing. In this article, coroutine will exclusively mean "coroutine object"
-- the thing produced by executing a coroutine function.

That coroutine represents the function's body or logic. A coroutine has to be explicitly started;
again, merely creating the coroutine does not start it. Notably, the coroutine can be paused &
resumed at various points within the function's body. That pausing & resuming ability is what allows
for asynchronous behavior!

===========
Tasks
===========

Roughly speaking, tasks are coroutines (not coroutine functions) tied to an event-loop. A task also maintains a list of callback
functions whose importance will become clear in a moment when we discuss ``await``. When tasks are created
they are automatically added to the event-loop's queue of tasks::

    # This creates a Task object and puts it on the event-loop's queue.
    special_task = asyncio.Task(coro=special_fella(magic_number=5), loop=event_loop)

It's common to see a task instantiated without explicitly specifying the event-loop it belongs to. Since
there's only one event-loop (a global singleton), asyncio made the loop argument optional and will add it
for you if it's left unspecified::

    # This creates another Task object and puts it on the event-loop's queue.
    # The task is implicitly tied to the event-loop by asyncio since the
    # loop argument was left unspecified.
    another_special_task = asyncio.Task(coro=special_fella(magic_number=12))

===========
await
===========

``await`` is a Python keyword that's commonly used in one of two different ways::

    await task
    await coroutine

Unfortunately, it actually does matter which type of object await is applied to.

``await``-ing a task will cede control from the current task or coroutine to the event-loop. And while doing so,
add a callback to the awaited task's list of callbacks indicating it should resume the current task/coroutine
when it (the ``await``-ed one) finishes. Said another way, when that awaited task finishes, it adds the original task
back to the event-loops queue.

In practice, it's slightly more convoluted, but not by much. In part 2, we'll walk through the
details that make this possible. And in the control flow analysis example we'll walk through, in precise detail,
the various control handoffs in an example async program.

**Unlike tasks, await-ing a coroutine does not cede control!** Wrapping a coroutine in a task first, then ``await``-ing
that would cede control. The behavior of ``await coroutine`` is effectively the same as invoking a regular,
synchronous Python function. Consider this program::

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

The first statement in the coroutine ``main()`` creates ``task_b`` and places it on the event-loops queue. Then,
``coro_a()`` is repeatedly ``await``-ed. Control never cedes to the event-loop which is why we see the output
of all three ``coro_a()`` invocations before ``coro_b()``'s output:

.. code-block:: none

    I am coro_a(). Hi!
    I am coro_a(). Hi!
    I am coro_a(). Hi!
    I am coro_b(). I sure hope no one hogs the event-loop...

If we change ``await coro_a()`` to ``await asyncio.Task(coro_a())``, the behavior changes. The coroutine
``main()`` cedes control to the event-loop with that statement. The event-loop then works through its queue,
calling ``coro_b()`` and then ``coro_a()`` before resuming the coroutine ``main()``.

.. code-block:: none

    I am coro_b(). I sure hope no one hogs the event-loop...
    I am coro_a(). Hi!
    I am coro_a(). Hi!
    I am coro_a(). Hi!


----------------------------------------------
A conceptual overview part 2: the nuts & bolts
----------------------------------------------

Part 2 goes into detail on the mechanisms asyncio uses to manage control flow. This is where the magic
happens. You'll come away from this section knowing what await does behind the scenes and how to make
your own asynchronous operators.

===============================================
coroutine.send(), await, yield & StopIteration
===============================================

asyncio leverages those 4 components to pass around control.

``coroutine.send(arg)`` is the method used to start or resume a coroutine. If the coroutine was paused and is now being
resumed, the argument ``arg`` will be sent in as the return value of the ``yield`` statement which originally paused it. If
the coroutine is being started, as opposed to resumed, ``arg`` must be None.

``yield``, like usual, pauses execution and returns control to the caller. In the example below, the ``yield`` is on line 3
and the caller is ``... = await rock`` on line 11. Generally, ``await`` calls the ``__await__`` method of the given object.
``await`` also does one more very special thing: it percolates (or passes along) any yields it receives up the call-chain.
In this case, that's back to ``... = coroutine.send(None)`` on line 16.

The coroutine is resumed via the ``coroutine.send(42)`` call on line 21. The coroutine picks back up from where it
``yield``-ed (i.e. paused) on line 3 and executes the remaining statements in its body. When a coroutine finishes
it raises a ``StopIteration`` exception with the return value attached to the exception.

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

It's worth pausing for a moment here and making sure you followed the various ways control flow and values were passed.

The only way to yield (or effectively cede control) from a coroutine is to ``await`` an object that ``yield``\ s in its ``__await__`` method.
That might sound odd to you. Frankly, it was to me too. You might be thinking:

    1. What about a ``yield`` directly within the coroutine? The coroutine becomes
    a generator-coroutine, a different beast entirely.

    2. What about a ``yield from`` within the coroutine to a function that yields
    (i.e. plain generator)? ``SyntaxError: yield from not allowed in a coroutine.``
    I imagine Python made this a ``SyntaxError`` to mandate only one way of using coroutines
    for the sake of simplicity. Ideologically, ``yield from`` and ``await`` are quite similar.

===========
Futures
===========

A future is an object meant to represent a computation or process's status and result. The term is a nod
to the idea of something still to come or not yet happened, and the object is a way to keep an eye
on that something.

A future has a few important attributes. One is its state which can be either pending, cancelled
or done. Another is its result which is set when the state transitions to done. To be clear, a
future does not represent the actual computation to be done, like a coroutine does, instead it
represents the status and result of that computation, kind of like a status-light
(red, yellow or green) or indicator.

``Task`` subclasses ``Future`` in order to gain these various capabilities. I said in the prior section tasks store a list
of callbacks and I lied a bit. It's actually the ``Future`` class that implements this logic which ``Task`` inherits.

Futures may be also used directly i.e. not via tasks. Tasks mark themselves as done when their coroutine's
complete. Futures are much more versatile and will be marked as done when you say so. In this way, they're
the flexible interface for you to make your own conditions for waiting and resuming.

==========================
await-ing Tasks & futures
==========================

``Future`` defines an important method: ``__await__``. Below is the actual implementation (well, one line was
removed for simplicity's sake) found in ``asyncio.futures.Future``. It's okay if it doesn't make complete sense
now, we'll go through it in detail in the control-flow example.

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

The ``Task`` class does not override ``Future``'s ``__await__`` implementation. ``await``-ing a task or future
invokes that above ``__await__`` method and percolates the ``yield`` on line 6 to relinquish
control to its caller, which is generally the event-loop.

========================
A homemade asyncio.sleep
========================

We'll go through an example of how you could leverage a future to create your own variant of asynchronous
sleep (i.e. asyncio.sleep).

This snippet puts a few tasks on the event-loops queue and then ``await``\ s a
yet unknown coroutine wrapped in a task: ``async_sleep(3)``. We want that task to finish only after
3 seconds have elapsed, but without hogging control while waiting.

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


Below, we use a future to enable custom control over when that task will be marked as done. If ``future.set_result()``,
the method responsible for marking that future as done, is never called, this task will never finish. We've also enlisted
the help of another task, which we'll see in a moment, that will monitor how much time has elapsed and
accordingly call ``future.set_result()``.

::

    async def async_sleep(seconds: float):
        future = asyncio.Future()
        time_to_wake = time.time() + seconds
        # Add the watcher-task to the event-loop.
        watcher_task = asyncio.Task(_sleep_watcher(future, time_to_wake))
        # Block until the future is marked as done.
        await future


We'll use a rather bare object ``YieldToEventLoop()`` to ``yield`` from its ``__await__`` in order
to cede control to the event-loop. This is effectively the same as calling ``asyncio.sleep(0)``,
but I prefer the clarity this approach offers, not to mention it's somewhat cheating to use
``asyncio.sleep`` when showcasing how to implement it!

The event-loop, as usual, cycles through its queue of tasks, giving them control, and receiving
control back when each task pauses or finishes. The ``watcher_task``, which runs the coroutine: ``_sleep_watcher(...)``
will be invoked once per full cycle of the event-loop's queue. On each resumption, it'll check the time and
if not enough has elapsed, it'll pause once again and return control to the event-loop. Eventually, enough time
will have elapsed, and ``_sleep_watcher(...)`` will mark the future as done, and then itself finish too by
breaking out of the infinite while loop. Given this helper task is only invoked once per cycle of the event-loop's
queue, you'd be correct to note that this asynchronous sleep will sleep **at least** three seconds,
rather than exactly three seconds. Note, this is also of true of the library-provided asynchronous
function: ``asyncio.sleep``.

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

You might feel this implementation of asynchronous sleep was unnecessarily convoluted.
And, well, it was. I wanted to showcase the versatility of futures with a
simple example that could be mimicked for more complex needs. For reference,
you could implement it without futures, like so::

    async def simpler_async_sleep(seconds):
        time_to_wake = time.time() + seconds
        while True:
            if time.time() >= time_to_wake:
                return
            else:
                await YieldToEventLoop()


.. _anaylzing-control-flow-example:

----------------------------------------------
Analyzing an example programs control flow
----------------------------------------------

We'll walkthrough, step by step, a simple asynchronous program following along in the key methods of Task & Future that are leveraged when asyncio is orchestrating the show.


===============
Task.step
===============

The actual method that invokes a Tasks coroutine: ``asyncio.tasks.Task.__step_run_and_handle_result``
is about 80 lines long. For the sake of clarity, I've removed all of the edge-case error handling,
simplified some aspects and renamed it, but the core logic remains unchanged.

::

    1  class Task(Future):
    2      ...
    3      def step(self):
    4          try:
    5              awaited_task = self.coro.send(None)
    6          except StopIteration as e:
    7              super().set_result(e.value)
    8          else:
    9             awaited_task.add_done_callback(self.__step)
    10         ...


======================
Example program
======================

::

    # Filename: program.py
    1  async def triple(val: int):
    2      return val * 3
    3
    4  async def main():
    5      triple_task = asyncio.Task(coro=triple(val=5))
    6      tripled_val = await triple_task
    7      return tripled_val + 2
    8
    9  loop = asyncio.new_event_loop()
    10 main_task = asyncio.Task(main(), loop=loop)
    11 loop.run_forever()

=====================
Control flow
=====================

At a high-level, this is how control flows:

.. code-block:: none

    1  program
    2      event-loop
    3          main_task.step
    4              main()
    5                  triple_task.__await__
    6              main()
    7          main_task.step
    8      event-loop
    9          triple_task.step
    10             triple()
    11         triple_task.step
    12     event-loop
    13         main_task.step
    14             triple_task.__await__
    15                 main()
    16         main_task.step
    17     event-loop

And, in much more detail:

1. Control begins in ``program.py``
        Line 9 creates an event-loop, line 10 creates ``main_task`` and adds it to
        the event-loop, line 11 indefinitely passes control to the event-loop.
2. Control is now in the event-loop
        The event-loop pops ``main_task`` off its queue, then invokes it by calling ``main_task.step()``.
3. Control is now in ``main_task.step``
        We enter the try-block on line 4 then begin the coroutine ``main()`` on line 5.
4. Control is now in the coroutine: ``main()``
        The Task ``triple_task`` is created on line 5 which adds it to the event-loops queue. Line 6
        ``await``\ s triple_task. Remember, that calls ``Task.__await__`` then percolates any ``yield``\ s.
5. Control is now in ``triple_task.__await__``
        ``triple_task`` is not done given it was just created, so we enter the first if-block on line 5 and ``yield`` the thing
        we'll be waiting for -- ``triple_task``.
6. Control is now in the coroutine: ``main()``
        ``await`` percolates the ``yield`` and the yielded value -- ``triple_task``.
7. Control is now in ``main_task.step``
        The variable ``awaited_task`` is ``triple_task``. No ``StopIteration`` was raised so the else in the try-block
        on line 8 executes. A done-callback: ``main_task.step`` is added to the ``triple_task``. The step method ends and
        returns to the event-loop.
8. Control is now in the event-loop
        The event-loop cycles to the next task in its queue. The event-loop pops ``triple_task`` from its queue and invokes
        it by calling ``triple_task.step()``.
9. Control is now in ``triple_task.step``
        We enter the try-block on line 4 then begin the coroutine ``triple()`` via line 5.
10. Control is now in the coroutine: ``triple()``
        It computes 3 times 5, then finishes and raises a ``StopIteration`` exception.
11. Control is now in ``triple_task.step``
        The ``StopIteration`` exception is caught so we go to line 7. The return value of the coroutine ``triple()`` is embedded in the
        value attribute of that exception. ``Future.set_result()`` saves the result, marks the task as done and adds
        the done-callbacks of ``triple_task`` to the event-loops queue. The step method ends and returns control to the
        event-loop.
12. Control is now in the event-loop
        The event-loop cycles to the next task in its queue. The event-loop pops ``main_task`` and resumes it by
        calling ``main_task.step()``.
13. Control is now in ``main_task.step``
        We enter the try-block on line 4 then resume the coroutine ``main`` which will pick up again from where it
        ``yield``-ed. Recall, it ``yield``-ed not in the coroutine, but in ``triple_task.__await__`` on line 6.
14. Control is now in ``triple_task.__await__``
        We evaluate the if-statement on line 8 which ensures that ``triple_task`` was completed. Then, it returns the
        result of ``triple_task`` which was saved earlier. Finally that result is returned to the
        caller (i.e. ``... = await triple_task``).
15. Control is now in the coroutine: ``main()``
        ``tripled_val`` is 15. The coroutine finishes and raises a ``StopIteration`` exception with the return value of 17 attached.
16. Control is now in ``main_task.step``
        The ``StopIteration`` exception is caught and ``main_task`` is marked as done and its result is saved. The step method ends
        and returns control to the event-loop.
17. Control is now in the event-loop
        There's nothing in the queue. The event-loop cycles aimlessly onwards.

----------------------------------------------
Barebones network I/O example
----------------------------------------------

Here we'll see a simple but thorough example showing how asyncio can offer an advantage over serial programs. The example doesn't rely on any asyncio
operators (besides the event-loop). It's all non-blocking sockets & custom awaitables that help you see what's actually happening under the
hood and how you could do something similar.

Performing a database request across a network might take half a second or so, but that's ages in computer-time. Your processor could have
done millions or even billions of things. The same is true for, say, requesting a website, downloading a car, loading a file from disk
into memory, etc. The general theme is those are all input/output (I/O) actions.

Consider performing two tasks: requesting some information from a server and doing some computation locally. A serial approach would
look like: ping the server, idle while waiting for a response, receive the response, perform the local computation. An asynchronous
approach would look like: ping the server, do some of the local computation while waiting for a response, check if the server is
ready yet, do a bit more of the local computation, check again, etc. Basically we're freeing up the CPU to do other activities
instead of scratching its belly button.

This example has a server (a separate, local process) compute the sum of many samples from a Gaussian (i.e. normal) distribution.
And the local computation finds the sum of many samples from a uniform distribution. As you'll see, the asynchronous approach
runs notably faster, since progress can be made on computing the sum of many uniform samples, while waiting for the server to
calculate and respond.

=====================
Serial output
=====================

.. code-block:: none

    $ python serial_approach.py
    Beginning server_request.
    ====== Done server_request. total: -2869.04. Ran for: 2.77s. ======
    Beginning uniform_sum.
    ====== Done uniform_sum. total: 60001676.02. Ran for: 4.77s. ======
    Total time elapsed: 7.54s.

=====================
Asynchronous output
=====================

.. code-block:: none

    $ python async_approach.py
    Beginning uniform_sum.
    Pausing uniform_sum at sample_num: 26,999,999. time_elapsed: 1.01s.

    Beginning server_request.
    Pausing server_request. time_elapsed: 0.00s.

    Resuming uniform_sum.
    Pausing uniform_sum at sample_num: 53,999,999. time_elapsed: 1.05s.

    Resuming server_request.
    Pausing server_request. time_elapsed: 0.00s.

    Resuming uniform_sum.
    Pausing uniform_sum at sample_num: 80,999,999. time_elapsed: 1.05s.

    Resuming server_request.
    Pausing server_request. time_elapsed: 0.00s.

    Resuming uniform_sum.
    Pausing uniform_sum at sample_num: 107,999,999. time_elapsed: 1.04s.

    Resuming server_request.
    ====== Done server_request. total: -2722.46. ======

    Resuming uniform_sum.
    ====== Done uniform_sum. total: 59999087.62 ======

    Total time elapsed: 4.60s.

======================
Code
======================

Now, we'll explore some of the most important snippets.

Below is the portion of the asynchronous approach responsible for checking if the server's done yet. And,
if not, yielding control back to the event-loop instead of idly waiting. I'd like to draw your attention
to a specific part of this snippet. Setting a socket to non-blocking mode means the ``recv()`` call
won't idle while waiting for a response. Instead, if there's no data to be read, it'll immediately
raise a ``BlockingIOError``. If there is data available, the ``recv()`` will proceed as normal.

.. code-block:: python

    class YieldToEventLoop:
        def __await__(self):
            yield
    ...

    async def get_server_data():
        client = socket.socket()
        client.connect(server.SERVER_ADDRESS)
        client.setblocking(False)

        while True:
            try:
                # For reference, the first argument to recv() is the maximum number
                # of bytes to attempt to read. Setting it to 4096 means we could get 2
                # bytes or 4 bytes, or even 4091 bytes, but not 4097 bytes back.
                # However, if there are no bytes available to be read, this recv()
                # will raise a BlockingIOError since the socket was set to
                # non-blocking mode.
                response = client.recv(4096)
                break
            except BlockingIOError:
                await YieldToEventLoop()
        return response


And this is the portion of code responsible for asynchronously computing
the uniform sums. It's designed to allow for working through the sum a portion at a time.
The ``time_allotment`` argument to the coroutine function decides how long the sum
function will iterate, in other words, synchronously hog control, before ceding
back to the event-loop.

.. code-block:: python

    async def uniform_sum(n_samples: int, time_allotment: float) -> float:

        start_time = time.time()

        total = 0.0
        for _ in range(n_samples):
            total += random.random()

            time_elapsed = time.time() - start_time
            if time_elapsed > time_allotment:
                await YieldToEventLoop()
                start_time = time.time()

        return total

The above snippet was simplified a bit. Reading ``time.time()`` and evaluating an if-condition
on every iteration for many, many iterations (in this case roughly a hundred million) more than
eats up the runtime savings associated with the asynchronous approach. The actual
implementation involves chunking the iteration, so you only perform the check
every few million iterations. With that change, the asynchronous approach wins in a landslide.
This is important to keep in mind. Too much checking or constantly jumping between tasks can
ultimately cause more harm than good!

The server, async & serial programs are available in full here:
https://github.com/anordin95/a-conceptual-overview-of-asyncio/tree/main/barebones-network-io-example.

.. _which_concurrency_do_I_want:

------------------------------
Which concurrency do I want
------------------------------

===========================
multiprocessing
===========================

For any computationally bound work in Python, you likely want to use multiprocessing.
Otherwise, the Global Interpreter Lock (GIL) will generally get in your way! For those who don't
know, the GIL is a lock which ensures only one Python instruction is executed at a time.
Of course, since processes are generally entirely independent from one another, the GIL in one
process won't be impeded by the GIL in another process. Granted, I believe there are ways to
also get around the GIL in a single process by leveraging C extensions.

===========================
multithreading & asyncio
===========================

Multithreading and asyncio are much more similar in where they're useful with Python: not at all for
computationally-bound work, and crucially for I/O bound work. For applications that need to
manage absolutely tons of distinct I/O connections or chunks-of-work, asyncio is a must. For example,
a web server handling thousands of requests "simultaneously" (in quotes, because, as we saw, the frequent
handoffs of control only create the illusion of simultaneous execution). Otherwise, I think the choice
between which to use is somewhat down to taste.

Multithreading maintains an OS managed thread for each chunk of work. Whereas asyncio uses Tasks for
each work-chunk and manages them via the event-loop's queue. I believe the marginal overhead of one more
chunk of work is a fair bit lower for asyncio than threads, which matters a lot for applications that
need to manage many, many chunks of work.

There are some other benefits associated with using asyncio. One is clearer visibility into when and where
interleaving occurs. The code chunk between two awaits is certainly synchronous. Another is simpler debugging,
since it's easier to attach and follow a trace and reason about code execution. With threading, the interleaving
is more of a black-box. One benefit of multithreading is not really having to worry about greedy threads
hogging execution, something that could happen with asyncio where a greedy coroutine never awaits and
effectively stalls the event-loop.
