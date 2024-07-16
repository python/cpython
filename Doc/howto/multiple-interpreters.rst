.. _multiple-interpreters-howto:

***************************
Multiple Interpreters HOWTO
***************************

When it comes to concurrency and parallelism,
Python provides a number of options, each with various tradeoffs.
This includes threads, async, and multiprocessing.
There's one more option: multiple interpreters (AKA "subinterpreters").

.. (PyPI packages expand that with "greenlets", and distributed computing.)

Multiple interpreters in the same process provide conceptual isolation
like processes but more efficiently, like threads.  In fact, you can
think of them like threads with opt-in sharing.

That provides a concurrency model which is easier to reason about,
similar to CSP or the actor model.
Furthermore, each interpreter has its own GIL, so they provide
true multi-core parallelism.

This HOWTO document has 2 parts: first a tutorial and then a set
of recipes and practical examples.

The tuturial covers the basics of using multiple interpreters
(:ref:`interp-tutorial-basics`), as well as how to communicate between
interpreters (:ref:`interp-tutorial-communicating`).
The examples section (:ref:`interp-recipes`) is focused on providing
effective solutions for real concurrency use cases.  This includes
comparisons with Python's various existing concurrency models.

.. currentmodule:: interpreters

.. seealso::

   :ref:`execcomponents`
   :mod:`interpreters`
   :mod:`interpreters.queues`
   :mod:`interpreters.channels`

.. note::

   This page is focused on *using* multiple interpreters.
   For information about changing an extension module to work
   with multiple interpreters, see :ref:`isolating-extensions-howto`.
   





Why Use Multiple Interpreters?
------------------------------

This provides a similar

We can take advantage of the isolation and independence of multiple
processes, by using :mod:`multiprocessing` or :mod:`subprocess`
(with sockets and pipes)
In the same process, We have threads, 


For concurrent code, imagine combining the efficiency of threads
with the isolation 
You can run multiple isolated Python execution environments
in the same process




Python can actually run multiple interpreters at the same time,
which lets you have independent, isolated execution environments
in the same process.  This has been true since Python 2.2, but
the feature was only available through the C-API and not well known;
and the isolation was incomplete.  However...

As of Python 3.12, isolation has been fixed,
to the point that interpreters in the same process no longer
even share the GIL (global interpreter lock).  That means you can

has had the ability to run multiple copies 

Benefits:

Downsides:

...


When to Use Multiple Interpreters
---------------------------------

...

+-------------------------------------+--------------------------------------+
| Task you want to perform            | The best tool for the task           |
+=====================================+======================================+
| ...                                 | ...                                  |
+-------------------------------------+--------------------------------------+
| ...                                 | ...                                  |
+-------------------------------------+--------------------------------------+


.. _interp-tutorial-basics:

Tutorial: Basics
================

First of all, keep in mind that using multiple interpreters is like
using multiple processes.  They are isolated and independent from each
other.  The main difference is that multiple interpreters live in the
same process, which makes it all more efficient.

Each interpreter has its own ``__main__`` module, its own
:func:`sys.modules`, and, in fact, its own set of *all* runtime state.
Interpreters pretty much never share objects and very little data.

Running Code in an Interpreter
------------------------------

Running code in an interpreter is basically equivalent to running the
buitlin :func:`exec` using that interpreter::

    import interpreters

    interp = interpreters.create()

    interp.exec('print("spam!")')
    interp.exec("""
        print('spam!')
        """)

Calling a function in an interpreter works the same way::

    import interpreters

    interp = interpreters.create()

    def script():
        print('spam!')
    interp.call(script)

When it runs, the code is executed using the interpreter's ``__main__``
module, just like a Python process normally does::

    import interpreters

    print(__name__)
    # __main__

    interp = interpreters.create()
    interp.exec("""
        print(__name__)
        """)
    # __main__

In fact, a comparison with ``python -c`` is quite direct::

    import interpreters

    ############################
    # python -c 'print("spam!")'
    ############################

    interp = interpreters.create()
    interp.exec('print("spam!")')

It's also fairly easy to simulate the other forms of the Python CLI::

    import interpreters

    with open('script.py', 'w') as outfile:
        outfile.write("""
    print('spam!')
    """)

    ##################
    # python script.py
    ##################

    interp = interpreters.create()
    with open('script.py') as infile:
        script = infile.read()
    interp.exec(script)

    ##################
    # python -m script
    ##################

    interp = interpreters.create()
    interp.exec("""
        import runpy
        runpy.run_module('script')
        """)

That's more or less what the ``python`` executable is doing for each
of those cases.

Preparing and Reusing an Interpreter
------------------------------------

When you use the Python REPL, it doesn't reset after each command.
That would make the REPL much less useful.  Likewise, when you use
the builtin :func:`exec`, it doesn't reset the namespace it uses
to run the code.

In the same way, running code in an interpreter does not reset it.
The next time you run code in that interpreter, the ``__main__`` module 
will be in exactly the state in which you left it::

    import interpreters

    interp = interpreters.create()
    interp.exec("""
        answer = 42
        """)
    interp.exec("""
        assert answer == 42
        """)

You can take advantage of this to prepare an interpreter ahead of time
or use it incrementally::

    import interpreters

    interp = interpreters.create()

    # Prepare the interpreter.
    interp.exec("""
        # We will need this later.
        import math

        # Initialize the value.
        value = 1

        def double(val):
            return val + val
        """)

    # Do the work.
    for _ in range(10):
        interp.exec("""
            assert math.factorial(value + 1) >= double(value)
            value = double(value)
            """)

    # Show the result.
    interp.exec("""
        print(value)
        """)

In case you're curious, in a little while we'll look at how to pass
data in and out of an interpreter (instead of just printing things).

Sometimes you might also have values that you want to use in another
interpreter.  We'll look more closely at that case in a little while:
:ref:`interp-script-args`.

Running in a Thread
-------------------

A key point is that, in a new Python process, the runtime doesn't
create a new thread just to run the target script.  It uses the main
thread the process started with.

In the same way, running code in another interpreter doesn't create
(or need) a new thread.  It uses the current OS thread, switching
to the interpreter long enough to run the code.

To actually run in a new thread, you do it explicitly::

    import interpreters
    import threading

    interp = interpreters.create()

    def run():
        interp.exec("print('spam!')")
    t = threading.Thread(target=run)
    t.start()
    t.join()

    def run():
        interp.call(script)
    t = threading.Thread(target=run)
    t.start()
    t.join()

There's also a helper method for that::

    import interpreters

    interp = interpreters.create()

    def script():
        print('spam!')
    t = interp.call_in_thread(script)
    t.join()

Handling Uncaught Exceptions
----------------------------

Consider what happens when you run a script at the commandline
and there's an unhandled exception.  In that case, Python will print
the traceback and the process will exit with a failure code.

The behavior is very similar when code is run in an interpreter.
The traceback get printed and, rather than a failure code,
an :class:`ExecutionFailed` exception is raised::

    import interpreters

    interp = interpreters.create()

    try:
        interp.exec('1/0')
    except interpreters.ExecutionFailed:
        print('oops!')

The exception also has some information, which you can use to handle
it with more specificity::

    import interpreters

    interp = interpreters.create()

    try:
        interp.exec('1/0')
    except interpreters.ExecutionFailed as exc:
        if exc.excinfo.type.__name__ == 'ZeroDivisionError':
            ...
        else:
            raise  # re-raise

You can handle the exception in the interpreter as well, like you might
do with threads or a script::

    import interpreters

    interp = interpreters.create()

    interp.exec("""
        try:
            1/0
        except ZeroDivisionError:
            ...
        """)

At the moment there isn't an easy way to "re-raise" an unhandled
exception from another interpreter.  Here's one approach that works for
exceptions that can be pickled::

    import interpreters

    interp = interpreters.create()

    try:
        interp.exec("""
            try:
                1/0
            except Exception as exc:
                import pickle
                data = pickle.dumps(exc)
                class PickledException(Exception):
                    pass
                raise PickledException(data)
            """)
    except interpreters.ExecutionFailed as exc:
        if exc.excinfo.type.__name__ == 'PickledException':
            import pickle
            raise pickle.loads(exc.excinfo.msg)
        else:
            raise  # re-raise

Managing Interpreter Lifetime
-----------------------------

Every interpreter uses resources, particularly memory, that should be
cleaned up as soon as you're done with the interpreter.  While you can
wait until the interpreter is cleaned up when the process exits, you
can explicitly clean it up sooner::

    import interpreters

    interp = interpreters.create()
    try:
        interp.exec("""
            print('spam!')
            """)
    finally:
        interp.close()

concurrent.futures.InterpreterPoolExecutor
------------------------------------------

The :mod:`concurrent.futures` module is a simple, popular concurrency
framework that wraps both threading and multiprocessing.  You can also
use it with multiple interpreters::

    from concurrent.futures import InterpreterPoolExecutor

    with InterpreterPoolExecutor(max_workers=5) as executor:
        executor.submit('print("spam!")')

Gotchas
-------

...

Keep in mind that there is a limit to the kinds of functions that may
be called by an interpreter.  Specifically, ...


* limited callables
* limited shareable objects
* not all PyPI packages support use in multiple interpreters yet
* ...


.. _interp-tutorial-communicating:

Tutorial: Communicating Between Interpreters
============================================

Multiple interpreters are useful in the convenience and efficiency
they provide for isolated computing.  However, their main strength is
in running concurrent code.  Much like with any concurrency tool,
there has to be a simple and efficient way to communicate
between interpreters.

In this half of the tutorial, we explore various ways you can do so.

.. _interp-script-args:

Passing Values to an Interpreter
--------------------------------

When you call a function in Python, sometimes that function requires
arguments and sometimes it doesn't.  In the same way, sometimes a
script you want to run in another interpreter requires some values.
Providing such values to the interpreter, for the script to use,
is the simplest kind of communication between interpreters.

Perhaps the simplest approach: you can use an f-string or format string
for the script and interpolate the required values::

    import interpreters

    interp = interpreters.create()

    def run_interp(interp, name, value):
        interp.exec(f"""
            {name} = {value!r}
            ...
            """)
    run_interp(interp, 'spam', 42)

    try:
        infile = open('spam.txt')
    except FileNotFoundError:
        pass
    else:
        with infile:
            interp.exec(f"""
                import os
                for line in os.fdopen({infile.fileno())):
                    print(line)
                """))

This works especially well for intrinsic values.  For complex values
you'll usually want to reach for other solutions.

There's one convenient alternative that works for some objects:
:func:`Interpreter.prepare_main`.  It binds values to names in the
interpreter's ``__main__`` module, which makes them available to any
scripts that run in the interpreter after that::

    import interpreters

    interp = interpreters.create()

    def run_interp(interp, name, value):
        interp.prepare_main(**{name: value})
        interp.exec(f"""
            ...
            """)
    run_interp(interp, 'spam', 42)

    try:
        infile = open('spam.txt')
    except FileNotFoundError:
        pass
    else:
        with infile:
            interp.prepare_main(fd=infile.fileno())
            interp.exec(f"""
                import os
                for line in os.fdopen(fd):
                    print(line)
                """))

Note that :func:`Interpreter.prepare_main` only works with "shareable"
objects.  (See :ref:`interp-shareable-objects`.)  In a little while
also see how shareable objects make queues and channels especially
convenient and efficient.

For anything more complex that isn't shareable, you'll probably need
something more than f-strings or :func:`Interpreter.prepare_main`.

That brings us the next part: 2-way communication, passing data back
and forth between interpreters.

Pipes, Sockets, etc.
--------------------

We'll start off with an approach to duplex communication between
interpreters that you can implement using OS-provided utilities.
The examples will use :func:`os.pipe`, but the approach applies equally
well to regular files and sockets.

Keep in mind that pipes, sockets, and files work with :class:`bytes`,
not other objects, so this may involve at least some serialization.
Aside from that inefficiency, sending data through the OS will
also slow things down.

After this we'll look at using queues and channels for simpler and more
efficient 2-way communication.  The contrast should be clear then.

First, let's use a pipe to pass a message from one interpreter
to another::

    import interpreters
    import os

    interp1 = interpreters.create()
    interp2 = interpreters.create()

    rpipe, spipe = os.pipe()
    try:
        def task():
            interp.exec("""
                import os
                msg = os.read({rpipe}, 20)
                print(msg)
                """)
        t = threading.thread(target=task)
        t.start()

        # Sending the message:
        os.write(spipe, b'spam!')
        t.join()
    finally:
        os.close(rpipe)
        os.close(spipe)

One interesting part of that is how the subthread blocked until
we wrote to the pipe.  In addition to delivering the message, the
read end of the pipe acted like a lock.

We can actually make use of that to synchronize
execution between the interpreters::

    import interpreters

    interp = interpreters.create()

    r1, s1 = os.pipe()
    r2, s2 = os.pipe()
    try:
        def task():
            interp.exec("""
                # Do stuff...

                # Synchronize!
                msg = os.read(r1, 1)
                os.write(s2, msg)

                # Do other stuff...
                """)
        t = threading.thread(target=task)
        t.start()

        # Synchronize!
        os.write(s1, '')
        os.read(r2, 1)
    finally:
        os.close(r1)
        os.close(s1)
        os.close(r2)
        os.close(s2)

You can also close the pipe ends and join the thread to synchronize.

Again, using :func:`os.pipe`, etc. to communicate between interpreters
is a little awkward, as well as inefficient.  We'll look at some of
the alternatives next.

Using Queues
------------

:class:`interpreters.queues.Queue` is an implementation of the
:class:`queue.Queue` interface that supports safely and efficiently
passing data between interpreters.

::

    import interpreters.queues

    queue = interpreters.queues.create()
    interp = interpreters.create()
    interp.prepare_main(queue=queue)

    queue.put('spam!')
    interp.exec("""
        msg = queue.get()
        print(msg)
        """)

::

    queue = interpreters.queues.create()
    interp = interpreters.create()
    interp.prepare_main(queue=queue)

    queue.put('spam!')
    interp.exec("""
        obj = queue.get()
        print(msg)
        """)

::

    queue1 = interpreters.queues.create()
    queue2 = interpreters.queues.create(syncobj=True)
    interp = interpreters.create()
    interp.prepare_main(queue1=queue1, queue2=queue2)

    queue1.put('spam!')
    queue1.put('spam!', syncobj=True)
    queue2.put('spam!')
    queue2.put('spam!', syncobj=False)
    interp.exec("""
        msg1 = queue1.get()
        msg2 = queue1.get()
        msg3 = queue2.get()
        msg4 = queue2.get()
        print(msg)
        """)


Using Channels
--------------

...

::

    import interpreters.channels

    rch, sch = interpreters.channels.create()
    interp = interpreters.create()
    interp.prepare_main(rch=rch)

    sch.send_nowait('spam!')
    interp.exec("""
        msg = rch.recv()
        print(msg)
        """)

::

    rch, sch = interpreters.channels.create()
    interp = interpreters.create()
    interp.prepare_main(rch=rch)

    data = bytearray(100)

    sch.send_buffer_nowait(data)
    interp.exec("""
        data = rch.recv()
        for i in range(len(data)):
            data[i] = i
        """)
    assert len(data) == 100
    for i in range(len(data)):
        assert data[i] == i

Capturing an interpreter's stdout
---------------------------------

::

   interp = interpreters.create()
   stdout = io.StringIO()
   with contextlib.redirect_stdout(stdout):
       interp.exec(tw.dedent("""
           print('spam!')
           """))
   assert(stdout.getvalue() == 'spam!')

   # alternately:
   interp.exec(tw.dedent("""
       import contextlib, io
       stdout = io.StringIO()
       with contextlib.redirect_stdout(stdout):
           print('spam!')
       captured = stdout.getvalue()
       """))
   captured = interp.get_main_attr('captured')
   assert(captured == 'spam!')

:func:`os.pipe()` could be used similarly.

Capturing Unhandled Exceptions
----------------------------

...

Falling Back to Pickle
----------------------

For other objects you can use pickle::

    import interpreters
    import pickle

    SCRIPT = """
        import pickle
        data = pickle.loads(_pickled)
        setattr(data['name'], data['value'])
        """
    data = dict(name='spam', value=42)

    interp = interpreters.create()
    interp.prepare_main(_pickled=pickle.dumps(data))
    interp.exec(SCRIPT)

Passing objects via pickle::

   interp = interpreters.create()
   r, s = os.pipe()
   interp.exec(tw.dedent(f"""
       import os
       import pickle
       reader = {r}
       """))
   interp.exec(tw.dedent("""
           data = b''
           c = os.read(reader, 1)
           while c != b'\x00':
               while c != b'\x00':
                   data += c
                   c = os.read(reader, 1)
               obj = pickle.loads(data)
               do_something(obj)
               c = os.read(reader, 1)
           """))
   for obj in input:
       data = pickle.dumps(obj)
       os.write(s, data)
       os.write(s, b'\x00')
   os.write(s, b'\x00')

Gotchas
-------

...


.. _interp-recipes:

Recipes (Practical Examples)
============================

Concurrency-Related Python Workloads
------------------------------------

...

Comparisons With Other Concurrency Models
-----------------------------------------

...

Concurrency
-----------

(incl. minimal comparisons with multiprocessing, threads, and async)

...

Isolation
---------

...
