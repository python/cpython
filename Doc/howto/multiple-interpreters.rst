.. _multiple-interpreters-howto:

***************************
Multiple Interpreters HOWTO
***************************

In this HOWTO document we'll look at how to take advantage of
multiple interpreters in a Python program.  We will focus on doing
so in Python code, through the stdlib :mod:`concurrent.interpreters`
module.

This document has 2 parts: first a brief introduction and then a tutorial.

The tutorial covers :ref:`the basics <interp-tutorial-basics>` of using
multiple interpreters, as well as how :ref:`to communicate
<interp-tutorial-communicating>` between interpreters.  It finishes with
some :ref:`extra information <interp-tutorial-misc>`.

.. currentmodule:: concurrent.interpreters

.. seealso::

   :mod:`concurrent.interpreters`

   .. XXX Add a ref to the upcoming lang ref runtime components section.

   .. XXX Add a reference to the upcoming concurrency HOWTO doc.

.. note::

   This page is focused on *using* multiple interpreters.
   For information about changing an extension module to work
   with multiple interpreters, see :ref:`isolating-extensions-howto`.

.. TODO:

   * tutorial: explain how to use interpreters C-API?
   * add a top-level recipes/examples section
   * recipes: add some!
   * recipes: add short examples of how to solve specific small problems
   * recipes: add a section specifically for workflow-oriented examples
   * recipes: add comparisons with other concurrency models
   * recipes: add examples focused just on taking advantage of isolation?
   * recipes: add examples of using C-API in extensions?  embedded?


Introduction
============

You can find a thorough explanation about what interpreters are and
what they're for in the :ref:`concurrent.interpreters <interpreters-intro>`
docs.

In summary, think of an interpreter as the Python runtime's execution
context.  You can have multiple interpreters in a single process,
switching between them at any time.  Each interpreter is almost
completely isolated from the others.

.. note::

   Interpreters in the same process can technically never be strictly
   isolated from one another since there are few restrictions on memory
   access within the same process.  The Python runtime makes a best
   effort at isolation but extension modules may easily violate that.
   Therefore, do not use multiple interpreters in security-sensitive
   situations, where they shouldn't have access to each other's data.

That isolation facilitates a concurrency model based on independent
logical threads of execution, like CSP or the actor model.

Each actual thread in Python, even if you're only running in the main
thread, has its own *current* execution context.  Multiple threads can
use the same interpreter or different ones.

Why Use Multiple Interpreters?
------------------------------

These are the main benefits:

* isolation supports a human-friendly concurrency model
* isolation supports full multi-core parallelism
* avoids the data races that make threads so challenging normally

There are some downsides and temporary limitations:

* the concurrency model requires extra rigor; it enforces higher
  discipline about how the isolated components in your program interact
* not all PyPI extension modules support multiple interpreters yet
* the existing tools for passing data between interpreters safely
  are still relatively inefficient and limited
* actually *sharing* data safely is tricky (true for free-threading too)
* all necessary modules must be imported separately in each interpreter
* relatively slow startup time per interpreter
* non-trivial extra memory usage per interpreter

Nearly all of these should improve in subsequent Python releases.


.. _interp-tutorial-basics:

Tutorial: Basics
================

First of all, keep in mind that using multiple interpreters is like
using multiple processes.  They are isolated and independent from each
other.  The main difference is that multiple interpreters live in the
same process, which makes it all more efficient and uses fewer
system resources.

Each interpreter has its own :mod:`!__main__` module, its own
:data:`sys.modules`, and, in fact, its own set of *all* runtime state.
Interpreters pretty much never share objects and very little data.

Running Code in an Interpreter
------------------------------

Running code in an interpreter is basically equivalent to running the
builtin :func:`exec` using that interpreter::

    from concurrent import interpreters

    interp = interpreters.create()

    interp.exec('print("spam!")')
    # prints: spam!

    interp.exec("""if True:
        ...
        print('spam!')
        ...
        """)
    # prints: spam!

(See :meth:`Interpreter.exec`.)

Calls are also supported.
See :ref:`the next section <interp-tutorial-calls>`.

When :meth:`Interpreter.exec` runs, the code is executed using the
interpreter's :mod:`!__main__` module, just like a Python process
normally does when invoked from the command-line::

    from concurrent import interpreters

    print(__name__)
    # prints: __main__

    interp = interpreters.create()
    interp.exec('print(__name__)')
    # prints: __main__

In fact, a comparison with ``python -c`` is quite direct::

    from concurrent import interpreters

    ############################
    # python -c 'print("spam!")'
    ############################

    interp = interpreters.create()
    interp.exec('print("spam!")')

It's also fairly easy to simulate the other forms of the Python CLI::

    from concurrent import interpreters
    from textwrap import dedent

    SCRIPT = """
    print('spam!')
    """

    with open('script.py', 'w') as outfile:
        outfile.write(SCRIPT)

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
    interp.exec('import os, sys; sys.path.insert(0, os.getcwd())')
    interp.exec(dedent("""
        import runpy
        runpy.run_module('script')
        """))

That's more or less what the ``python`` executable is doing for each
of those cases.

You can also exec any function that doesn't take any arguments, nor
returns anything.  Closures are not allowed but other nested functions
are.  It works the same as if you had passed the script corresponding
to the function's body::

    from concurrent import interpreters

    def script():
        print('spam!')

    def get_script():
        def nested():
            print('eggs!')
        return nested

    if __name__ == '__main__':
        interp = interpreters.create()

        interp.exec(script)
        # prints: spam!

        script2 = get_script()
        interp.exec(script2)
        # prints: eggs!

Any referenced globals are resolved relative to the interpreter's
:mod:`!__main__` module, just like happens for scripts, rather than
the original function's module::

    from concurrent import interpreters

    def script():
        print(__name__)

    if __name__ == '__main__':
        interp = interpreters.create()
        interp.exec(script)
        # prints: __main__

One practical difference is that with a script function you get syntax
highlighting in your editor.  With script text you probably don't.

.. _interp-tutorial-calls:

Calling a Function in an Interpreter
------------------------------------

You can just as easily *call* a function in another interpreter::

    from concurrent import interpreters

    def spam():
        print('spam!')

    if __name__ == '__main__':
        interp = interpreters.create()
        interp.call(spam)
        # prints: spam!

(See :meth:`Interpreter.call`.)

In fact, nearly all Python functions and callables are supported, with
the notable exclusion of closures.  We'll focus on plain functions
for the moment.

Relative to :meth:`Interpreter.call`, there are five kinds of functions:

1. builtin functions
2. extension module functions
3. "stateless" user functions
4. stateful user functions, defined in :mod:`!__main__`
5. stateful user functions, not defined in :mod:`!__main__`

A "stateless" function is one that doesn't use any globals.  It also
can't be a closure.  It can have parameters but not argument defaults.
It can have return values.  We'll cover
:ref:`arguments and returning <interp-tutorial-return-values>` soon.

Some builtin functions, like :func:`exec` and :func:`eval`, use the
current globals as a default (or :func:`implicit <globals>`) argument
value.  When such functions are called directly with
:meth:`Interpreter.call`, they use the :mod:`!__main__` module's
:data:`!__dict__` as the default/implicit "globals"::

    from concurrent import interpreters

    interp = interpreters.create()

    interp.call(exec, 'print(__name__)')
    # prints: __main__

    interp.call(eval, 'print(__name__)')
    # prints: __main__

Note that, for some of those builtin functions, like :func:`globals`
and even :func:`eval` (for some expressions), the return value may
be :ref:`"unshareable" <interp-tutorial-shareable>` and the call will
fail.

In each of the above cases, the function is more-or-less copied, when
sent to the other interpreter.  For most of the cases, the function's
module is imported in the target interpreter and the function is pulled
from there.  The exceptions are cases (3) and (4).

In case (3), the function is also copied, but efficiently and only
temporarily (long enough to be called) and is not actually bound to
any module.  This temporary function object's :data:`!__name__` will
be set to match the code object, but the :data:`!__module__`,
:data:`!__qualname__`, and other attributes are not guaranteed to
be set.  This shouldn't be a problem in practice, though, since
introspecting the currently running function is fairly rare.

We'll cover case (4) in
:ref:`the next section <interp-tutorial-funcs-in-main>`.

In all the cases, the function will get any global variables from the
module (in the target interpreter), like normal::

    from concurrent import interpreters
    from mymod import spam

    if __name__ == '__main__':
        interp = interpreters.create()
        interp.call(spam)
        # prints: mymod

    ##########
    # mymod.py
    ##########

    def spam():
        print(__name__)

Note, however, that in case (3) functions rarely look up any globals.

.. _interp-tutorial-funcs-in-main:

Functions Defined in __main__
-----------------------------

Functions defined in :mod:`!__main__` are treated specially by
:meth:`Interpreter.call`.  This is because the script or module that
ran in the main interpreter will have not run in other interpreters,
so any functions defined in the script would only be accessible by
running it in the target interpreter first.  That is essentially
how :meth:`Interpreter.call` handles it.

If the function isn't found in the target interpreter's :mod:`!__main__`
module then the script that ran in the main interpreter gets run in the
target interpreter, though under a different name than "__main__".
The function is then looked up on that resulting fake __main__ module.
For example::

    from concurrent import interpreters

    def spam():
        print(__name__)

    if __name__ == '__main__':
        interp = interpreters.create()
        interp.call(spam)
        # prints: '<fake __main__>'

The dummy module is never added to :data:`sys.modules`, though it may
be cached away internally.

This approach with a fake :mod:`!__main__` module means we can avoid
polluting the :mod:`!__main__` module of the target interpreter.  It
also means global state used in such a function won't be reflected
in the interpreter's :mod:`!__main__` module::

    from textwrap import dedent
    from concurrent import interpreters

    total = 0

    def inc():
        global total
        total += 1

    def show_total():
        print(total)

    if __name__ == '__main__':
        interp = interpreters.create()

        interp.call(show_total)
        # prints: 0

        interp.call(inc)
        interp.call(show_total)
        # prints: 1

        interp.exec(dedent("""
            try:
                print(total)
            except NameError:
                pass
            else:
                raise AssertionError('expected NameError')
            """))
        interp.exec('total = -1')
        interp.exec('print(total)')
        # prints: -1

        interp.call(show_total)
        # prints: 1

        interp.call(inc)
        interp.call(show_total)
        # prints: 2

        interp.exec('print(total)')
        # prints: -1

        print(total)
        # prints: 0

Note that the recommended ``if __name__ == '__main__`:`` idiom is
especially important for such functions, since the script will be
executed with :data:`!__name__` set to something other than
:mod:`!__main__`.  Thus, for any code you don't want run repeatedly,
put it in the if block.  You will probably only want functions or
classes outside the if block.

Another thing to keep in mind is that, when you run the REPL or run
``python -c ...``, the code that runs is essentially unrecoverable.  The
contents of the :mod:`__main__` module cannot be reproduced by executing
a script (or module) file.  Consequently, calling a function defined
in :mod:`__main__` in these cases will probably fail.

Here's one last tip before we move on.  You can avoid the extra
complexity involved with functions (and classes) defined in
:mod:`__main__` by simply not defining them in :mod:`__main__`.
Instead, put them in another module and import them in your script.

Calling Methods and Other Objects in an Interpreter
---------------------------------------------------

Pretty much any callable object may be run in an interpreter, following
the same rules as functions::

    from concurrent import interpreters
    from mymod import Spam

    class Eggs:
        @classmethod
        def modname(cls):
            print(__name__)
        def __call__(self):
            print(__name__)
        def eggs(self):
            print(__name__)

    if __name__ == '__main__':
        interp = interpreters.create()

        spam = Spam()
        interp.call(Spam.modname)
        # prints: mymod

        interp.call(spam)
        # prints: mymod

        interp.call(spam.spam)
        # prints: mymod

        eggs = Eggs()
        res = interp.call(Eggs.modname)
        # prints: <fake __main__>

        res = interp.call(eggs)
        # prints: <fake __main__>

        res = interp.call(eggs.eggs)
        # prints: <fake __main__>

    ##########
    # mymod.py
    ##########

    class Spam:
        @classmethod
        def modname(cls):
            print(__name__)
        def __call__(self):
            print(__name__)
        def spam(self):
            print(__name__)

Mutable State is not Shared
---------------------------

Just to be clear, the underlying data of very few mutable objects is
actually shared between interpreters.  The notable exceptions are
:class:`Queue` and :class:`memoryview`, which we will explore in a
little while.  In nearly every case, the raw data is only copied in
the other interpreter and thus never automatically synchronized::

    from concurrent import interpreters

    class Counter:
        def __init__(self, initial=0):
            self.value = initial
        def inc(self):
            self.value += 1
        def dec(self):
            self.value -= 1
        def show(self):
            print(self.value)

    if __name__ == '__main__':
        counter = Counter(17)
        interp = interpreters.create()

        interp.call(counter.show)
        # prints: 17
        counter.show()
        # prints: 17

        interp.call(counter.inc)
        interp.call(counter.show)
        # prints: 18
        counter.show()
        # prints: 17

        interp.call(counter.inc)
        interp.call(counter.show)
        # prints: 18
        counter.show()
        # prints: 17

        interp.call(counter.inc)
        interp.call(counter.show)
        # prints: 18
        counter.show()
        # prints: 17

Preparing and Reusing an Interpreter
------------------------------------

When you use the Python REPL, it doesn't reset after each command.
That would make the REPL much less useful.  Likewise, when you use
the builtin :func:`exec`, it doesn't reset the namespace it uses
to run the code.

In the same way, running code in an interpreter does not reset that
interpreter.  The next time you run code in that interpreter, the
:mod:`!__main__` module will be in exactly the state in which you
left it::

    from concurrent import interpreters

    interp = interpreters.create()

    interp.exec("""if True:
        answer = 42
        """)
    interp.exec("""if True:
        assert answer == 42
        """)

Similarly::

    from concurrent import interpreters

    def set(value):
        assert __name__ == '<fake __main__>', __name__
        global answer
        answer = value

    def check(expected):
        assert answer == expected

    if __name__ == '__main__':
        interp = interpreters.create()
        interp.call(set, 100)
        interp.call(check, 100)


You can take advantage of this to prepare an interpreter ahead of time
or use it incrementally::

    from textwrap import dedent
    from concurrent import interpreters

    interp = interpreters.create()

    # Prepare the interpreter.
    interp.exec(dedent("""
        # We will need this later.
        import math

        # Initialize the value.
        value = 1

        def double(val):
            return val + val
        """))

    # Do the work.
    for _ in range(9):
        interp.exec(dedent("""
            assert math.factorial(value + 1) >= double(value)
            value = double(value)
            """))

    # Show the result.
    interp.exec('print(value)')
    # prints: 1024

In case you're curious, in a little while we'll look at how to pass
data in and out of an interpreter (instead of just printing things).

Sometimes you might also have values that you want to use in a later
script in another interpreter.  We'll look more closely at that
case in a little while: :ref:`interp-script-args`.

Running in a Thread
-------------------

A key point is that, in a new Python process, the runtime doesn't
create a new thread just to run the target script.  It uses the main
thread the process started with.

In the same way, running code in another interpreter doesn't create
(or need) a new thread.  It uses the current OS thread, switching
to the interpreter long enough to run the code.

To actually run in a new thread, you can do it explicitly::

    from concurrent import interpreters
    import threading

    interp = interpreters.create()

    def run():
        interp.exec("print('spam!')")
    t = threading.Thread(target=run)
    t.start()
    t.join()

    def run():
        def script():
            print('spam!')
        interp.call(script)
    t = threading.Thread(target=run)
    t.start()
    t.join()

There's also a helper method for that::

    from concurrent import interpreters

    def script():
        print('spam!')

    if __name__ == '__main__':
        interp = interpreters.create()
        t = interp.call_in_thread(script)
        t.join()

Handling Uncaught Exceptions
----------------------------

Consider what happens when you run a script at the commandline
and there's an unhandled exception.  In that case, Python will print
the traceback and the process will exit with a failure code.

The behavior is very similar when code is run in an interpreter.
The traceback gets printed and, rather than a failure code,
an :class:`ExecutionFailed` exception is raised::

    from concurrent import interpreters

    interp = interpreters.create()

    try:
        interp.exec('1/0')
    except interpreters.ExecutionFailed:
        print('oops!')

The exception also has some information, which you can use to handle
it with more specificity::

    from concurrent import interpreters

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

    from textwrap import dedent
    from concurrent import interpreters

    interp = interpreters.create()

    interp.exec(dedent("""
        try:
            1/0
        except ZeroDivisionError:
            ...
        """))

At the moment there isn't an easy way to "re-raise" an unhandled
exception from another interpreter.  Here's one approach that works for
exceptions that can be pickled::

    from textwrap import dedent
    from concurrent import interpreters

    interp = interpreters.create()

    try:
        try:
            interp.exec(dedent("""
                try:
                    1/0
                except Exception as exc:
                    import pickle
                    data = pickle.dumps(exc)
                    class PickledException(Exception):
                        pass
                    raise PickledException(data)
                """))
        except interpreters.ExecutionFailed as exc:
            if exc.excinfo.type.__name__ == 'PickledException':
                import pickle
                raise pickle.loads(eval(exc.excinfo.msg))
            else:
                raise  # re-raise
    except ZeroDivisionError:
        # Handle it!
        ...

Managing Interpreter Lifetime
-----------------------------

Every interpreter uses resources, particularly memory, that should be
cleaned up as soon as you're done with the interpreter.  While you can
wait until the interpreter is cleaned up when the process exits, you
can explicitly clean it up sooner::

    from concurrent import interpreters

    interp = interpreters.create()
    try:
        interp.exec('print("spam!")')
    finally:
        interp.close()


.. _interp-tutorial-communicating:

Tutorial: Communicating Between Interpreters
============================================

Multiple interpreters are useful in the convenience and efficiency
they provide for isolated computing.  However, their main strength is
in running concurrent code.  Much like with any concurrency tool,
there has to be a simple and efficient way to communicate
between interpreters.

In this half of the tutorial, we explore various ways you can do so.

.. _interp-tutorial-return-values:

Call Args and Return Values
---------------------------

As already noted, :meth:`Interpreter.call` supports most callables,
as well as arguments and return values.

Arguments provide a way to send information to an interpreter::

    from concurrent import interpreters

    def show(arg):
        print(arg)

    def ident_full(a, /, b, c=42, *args, d, e='eggs', **kwargs):
        print([a, b, c, d, e], args, kwargs)

    def handle_request(req):
        # do the work
        ...

    if __name__ == '__main__':
        interp = interpreters.create()

        interp.call(show, 'spam!')
        # prints: spam!

        interp.call(ident_full, 1, 2, 3, 4, 5, d=6, e=7, f=8, g=9)
        # prints: [1, 2, 3, 6, 7] (4, 5) {'f': 8, 'g': 9}

        req = ...
        interp.call(handle_request, req)

Return values are a way an interpreter can send information back::

    from concurrent import interpreters

    data = {}

    def put(key, value):
        data[key] = value

    def get(key, default=None):
        return data.get(key, default)

    if __name__ == '__main__':
        interp = interpreters.create()

        res = interp.call(get, 'spam')
        # res: None

        res = interp.call(get, 'spam', -1)
        # res: -1

        interp.call(put, 'spam', True)
        res = interp.call(get, 'spam')
        # res: True

        interp.call(put, 'spam', 42)
        res = interp.call(get, 'spam')
        # res: 42

Don't forget that the underlying data of few objects is actually shared
between interpreters.  That means that, nearly always, arguments are
copied on the way in and return values on the way out.  Furthermore,
this will happen with each call, so it's a new copy every time
and no state persists between calls.

For example::

    from concurrent import interpreters

    data = {
        'a': 1,
        'b': 2,
        'c': 3,
    }

    if __name__ == '__main__':
        interp = interpreters.create()

        interp.call(data.clear)
        assert data == dict(a=1, b=2, c=3)

        def update_and_copy(data, **updates):
            data.update(updates)
            return dict(data)

        res = interp.call(update_and_copy, data)
        assert res == dict(a=1, b=2, c=3)
        assert res is not data
        assert data == dict(a=1, b=2, c=3)

        res = interp.call(update_and_copy, data, d=4, e=5)
        assert res == dict(a=1, b=2, c=3, d=4, e=5)
        assert data == dict(a=1, b=2, c=3)

        res = interp.call(update_and_copy, data)
        assert res == dict(a=1, b=2, c=3)
        assert res is not data
        assert data == dict(a=1, b=2, c=3)

.. _interp-tutorial-shareable:

Supported and Unsupported Objects
---------------------------------

There are other ways of communicating between interpreters, like
:meth:`Interpreter.prepare_main` and :class:`Queue`, which we'll
cover shortly.  Before that, we should talk about which objects are
supported by :meth:`Interpreter.call` and the others.

Most objects are supported.  This includes anything that can be
:mod:`pickled <pickle>`, though for some pickleable objects we copy the
object in a more efficient way.  Aside from pickleable objects, there
is a small set of other objects that are supported, such as
non-closure inner functions.

If you try to pass an unsupported object as an argument to
:meth:`Interpreter.call` then you will get a :exc:`NotShareableError`
with ``__cause__`` set appropriately.  :meth:`Interpreter.call` will
also raise that way if the return value is not supported.  The same
goes for :meth:`~Interpreter.prepare_main` and :class:`Queue.* <Queue>`.

Relatedly, there's an interesting side-effect to how we use a fake
module for objects with a class defined in :mod:`!__main__`.  If you
pass the class to :meth:`Interpreter.call`, which will create an
instance in the other interpreter, then that instance will fail to
unpickle in the original interpreter, due to the fake module name::

    from concurrent import interpreters

    class Spam:
        def __init__(self, x):
            self.x = x

    if __name__ == '__main__':
        interp = interpreters.create()
        try:
            spam = interp.call(Spam, 10)
        except interpreters.NotShareableError:
            pass
        else:
            raise AssertionError('unexpected success')

Sharing Data
------------

There are actually a small number of objects that aren't just copied and
for which the underlying data is actually shared between interpreters.
We'll talk about :class:`Queue` in the next section.

Another example is :class:`memoryview`, where the underlying
:ref:`buffer <bufferobjects>` can be read and written by multiple
interpreters at once.  Users are responsible to manage thread-safety
for that data in such cases.

Support for actually sharing data for other objects is possible through
extension modules, but we won't get into that here.

Using Queues
------------

:class:`concurrent.interpreters.Queue` is an implementation of the
:class:`queue.Queue` interface that supports safely and efficiently
passing data between interpreters::

    import time
    from concurrent import interpreters

    def push(queue, value, *extra, delay=0.1):
        queue.put(value)
        for val in extra:
            if delay > 0:
                time.sleep(delay)
            queue.put(val)

    def pop(queue, count=1, timeout=None):
        return tuple(queue.get(timeout=timeout)
                     for _ in range(count))

    if __name__ == '__main__':
        interp1 = interpreters.create()
        interp2 = interpreters.create()
        queue = interpreters.create_queue()

        t = interp1.call_in_thread(push, queue,
                                   'spam!', 42, 'eggs')
        res = interp2.call(pop, queue)
        # res: ('spam!', 42, 'eggs')
        t.join()

.. _interp-script-args:

Initializing Globals for a Script
---------------------------------

When you call a function in Python, sometimes that function depends on
arguments, globals, and non-locals, and sometimes it doesn't.  In the
same way, sometimes a script you want to run in another interpreter
depends on some global variables.  Setting them ahead of time on the
interpreter's :mod:`!__main__` module, where the script will run,
is the simplest kind of communication between interpreters.

There's a method that supports this: :meth:`Interpreter.prepare_main`.
It binds values to names in the interpreter's :mod:`!__main__` module,
which makes them available to any scripts that run in the interpreter
after that::

    from concurrent import interpreters
    from textwrap import dedent

    interp = interpreters.create()

    def run_interp(interp, name, value):
        interp.prepare_main(**{name: value})
        interp.exec(dedent(f"""
            ...
            """))
    run_interp(interp, 'spam', 42)

    try:
        infile = open('spam.txt')
    except FileNotFoundError:
        pass
    else:
        with infile:
            interp.prepare_main(fd=infile.fileno())
            interp.exec(dedent(f"""
                import os
                for line in os.fdopen(fd):
                    print(line)
                """))

This is particularly useful when you want to use a queue in a script::

    from textwrap import dedent
    from concurrent import interpreters

    interp = interpreters.create()
    queue = interpreters.create_queue()

    interp.prepare_main(queue=queue)
    queue.put('spam!')

    interp.exec(dedent("""
        msg = queue.get()
        print(msg)
        """))
    # prints: spam!

For basic, intrinsic data it can also make sense to use an f-string or
format string for the script and interpolate the required values::

    from textwrap import dedent
    from concurrent import interpreters

    interp = interpreters.create()

    def run_interp(interp, name, value):
        interp.exec(dedent(f"""
            {name} = {value!r}
            ...
            """))
    run_interp(interp, 'spam', 42)

    try:
        infile = open('spam.txt')
    except FileNotFoundError:
        pass
    else:
        with infile:
            interp.exec(dedent(f"""
                import os
                for line in os.fdopen({infile.fileno()}):
                    print(line)
                """))

Pipes, Sockets, etc.
--------------------

For the sake of contrast, let's take a look at a different approach
to duplex communication between interpreters, which you can implement
using OS-provided utilities.  You could do something similar with
subprocesses.  The examples will use :func:`os.pipe`, but the approach
applies equally well to regular files and sockets.

Keep in mind that pipes, sockets, and files work with :class:`bytes`,
not other objects, so this may involve at least some serialization.
Aside from that inefficiency, sending data through the OS will
also slow things down.

First, let's use a pipe to pass a message from one interpreter
to another::

    from concurrent import interpreters
    import os

    READY = b'\0'

    def send(fd_tokens, fd_data, msg):
        # Wait until ready.
        token = os.read(fd_tokens, 1)
        assert token == READY, token
        # Ready!
        os.write(fd_data, msg)

    if __name__ == '__main__':
        interp = interpreters.create()
        r_tokens, s_tokens = os.pipe()
        r_data, s_data = os.pipe()
        try:
            t = interp.call_in_thread(
                    send, r_tokens, s_data, b'spam!')
            os.write(s_tokens, READY)
            msg = os.read(r_data, 20)
            # msg: b'spam!'
            t.join()
        finally:
            os.close(r_tokens)
            os.close(s_tokens)
            os.close(r_data)
            os.close(s_data)

One interesting part of that is how the subthread blocked until
we sent the "ready" token.  In addition to delivering the message,
the read end of the pipes acted like locks.

We can actually make use of that to synchronize execution between the
interpreters (and use :class:`Queue` the same way)::

    from concurrent import interpreters
    import os

    STOP = b'\0'
    READY = b'\1'

    def task(tokens):
        r, s = tokens
        while True:
            # Do stuff.
            ...

            # Synchronize!
            token = os.read(r, 1)
            os.write(s, token)
            if token == STOP:
                break

            # Do other stuff.
            ...

    steps = [...]

    if __name__ == '__main__':
        interp = interpreters.create()
        r1, s1 = os.pipe()
        r2, s2 = os.pipe()
        try:
            t = interp.call_in_thread(task, (r1, s2))
            for step in steps:
                # Do the step.
                ...

                # Synchronize!
                os.write(s1, READY)
                os.read(r2, 1)
            os.write(s1, STOP)
            os.read(r2, 1)
            t.join()
        finally:
            os.close(r1)
            os.close(s1)
            os.close(r2)
            os.close(s2)

You can also close the pipe ends and join the thread to synchronize.

Using :func:`os.pipe` (or similar) to communicate between interpreters
is a little awkward, as well as inefficient.


.. _interp-tutorial-misc:

Tutorial: Miscellaneous
=======================

concurrent.futures.InterpreterPoolExecutor
------------------------------------------

The :mod:`concurrent.futures` module is a simple, popular concurrency
framework that wraps both threading and multiprocessing.  You can also
use it with multiple interpreters, via
:class:`~concurrent.futures.InterpreterPoolExecutor`::

    from concurrent.futures import InterpreterPoolExecutor

    def script():
        return 'spam!'

    with InterpreterPoolExecutor(max_workers=5) as executor:
        fut = executor.submit(script)
    res = fut.result()
    # res: 'spam!'

Capturing an interpreter's stdout
---------------------------------

While interpreters share the same default file descriptors for stdout,
stderr, and stdin, each interpreter still has its own :mod:`sys` module,
with its own :data:`~sys.stdout` and so forth.  That means it isn't
obvious how to capture stdout for multiple interpreters.

The solution is a bit anticlimactic; you have to capture it for each
interpreter manually.  Here's a basic example::

    from concurrent import interpreters
    import contextlib
    import io

    def task():
        stdout = io.StringIO()
        with contextlib.redirect_stdout(stdout):
            # Do stuff in worker!
            ...
        return stdout.getvalue()

    if __name__ == '__main__':
        interp = interpreters.create()
        output = interp.call(task)
        ...

Here's a more elaborate example::

    from concurrent import interpreters
    import contextlib
    import errno
    import os
    import sys
    import threading

    def run_and_capture(fd, task, args, kwargs):
        stdout = open(fd, 'w', closefd=False)
        with contextlib.redirect_stdout(stdout):
            return task(*args, **kwargs)

    @contextlib.contextmanager
    def running_captured(interp, task, *args, **kwargs):
        def background(fd, stdout=sys.stdout):
            # Pass through captured output to local stdout.
            # Normally we would not ignore the encoding.
            infile = open(fd, 'r', closefd=False)
            while True:
                try:
                    line = infile.readline()
                except OSError as exc:
                    if exc.errno == errno.EBADF:
                        break
                    raise  # re-raise
                stdout.write(line)

        bg = None
        r, s = os.pipe()
        try:
            bg = threading.Thread(target=background, args=(r,))
            bg.start()
            t = interp.call_in_thread(run_and_capture, s, task, args, kwargs)
            try:
                yield
            finally:
                t.join()
        finally:
            os.close(r)
            os.close(s)
            if bg is not None:
                bg.join()

    def task():
        # Do stuff in worker!
        ...

    if __name__ == '__main__':
        interp = interpreters.create()
        with running_captured(interp, task):
            # Do stuff in main!
            ...

Using a :mod:`logger <logging>` can also help.
