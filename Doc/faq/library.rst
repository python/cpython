:tocdepth: 2

=========================
Library and Extension FAQ
=========================

.. only:: html

   .. contents::

General Library Questions
=========================

How do I find a module or application to perform task X?
--------------------------------------------------------

Check :ref:`the Library Reference <library-index>` to see if there's a relevant
standard library module.  (Eventually you'll learn what's in the standard
library and will be able to skip this step.)

For third-party packages, search the `Python Package Index
<https://pypi.org>`_ or try `Google <https://www.google.com>`_ or
another web search engine.  Searching for "Python" plus a keyword or two for
your topic of interest will usually find something helpful.


Where is the math.py (socket.py, regex.py, etc.) source file?
-------------------------------------------------------------

If you can't find a source file for a module it may be a built-in or
dynamically loaded module implemented in C, C++ or other compiled language.
In this case you may not have the source file or it may be something like
:file:`mathmodule.c`, somewhere in a C source directory (not on the Python Path).

There are (at least) three kinds of modules in Python:

1) modules written in Python (.py);
2) modules written in C and dynamically loaded (.dll, .pyd, .so, .sl, etc);
3) modules written in C and linked with the interpreter; to get a list of these,
   type::

      import sys
      print(sys.builtin_module_names)


How do I make a Python script executable on Unix?
-------------------------------------------------

You need to do two things: the script file's mode must be executable and the
first line must begin with ``#!`` followed by the path of the Python
interpreter.

The first is done by executing ``chmod +x scriptfile`` or perhaps ``chmod 755
scriptfile``.

The second can be done in a number of ways.  The most straightforward way is to
write ::

  #!/usr/local/bin/python

as the very first line of your file, using the pathname for where the Python
interpreter is installed on your platform.

If you would like the script to be independent of where the Python interpreter
lives, you can use the :program:`env` program.  Almost all Unix variants support
the following, assuming the Python interpreter is in a directory on the user's
:envvar:`PATH`::

  #!/usr/bin/env python

*Don't* do this for CGI scripts.  The :envvar:`PATH` variable for CGI scripts is
often very minimal, so you need to use the actual absolute pathname of the
interpreter.

Occasionally, a user's environment is so full that the :program:`/usr/bin/env`
program fails; or there's no env program at all.  In that case, you can try the
following hack (due to Alex Rezinsky):

.. code-block:: sh

   #! /bin/sh
   """:"
   exec python $0 ${1+"$@"}
   """

The minor disadvantage is that this defines the script's __doc__ string.
However, you can fix that by adding ::

   __doc__ = """...Whatever..."""



Is there a curses/termcap package for Python?
---------------------------------------------

.. XXX curses *is* built by default, isn't it?

For Unix variants: The standard Python source distribution comes with a curses
module in the :source:`Modules` subdirectory, though it's not compiled by default.
(Note that this is not available in the Windows distribution -- there is no
curses module for Windows.)

The :mod:`curses` module supports basic curses features as well as many additional
functions from ncurses and SYSV curses such as colour, alternative character set
support, pads, and mouse support. This means the module isn't compatible with
operating systems that only have BSD curses, but there don't seem to be any
currently maintained OSes that fall into this category.


Is there an equivalent to C's onexit() in Python?
-------------------------------------------------

The :mod:`atexit` module provides a register function that is similar to C's
:c:func:`!onexit`.


Why don't my signal handlers work?
----------------------------------

The most common problem is that the signal handler is declared with the wrong
argument list.  It is called as ::

   handler(signum, frame)

so it should be declared with two parameters::

   def handler(signum, frame):
       ...


Common tasks
============

How do I test a Python program or component?
--------------------------------------------

Python comes with two testing frameworks.  The :mod:`doctest` module finds
examples in the docstrings for a module and runs them, comparing the output with
the expected output given in the docstring.

The :mod:`unittest` module is a fancier testing framework modelled on Java and
Smalltalk testing frameworks.

To make testing easier, you should use good modular design in your program.
Your program should have almost all functionality
encapsulated in either functions or class methods -- and this sometimes has the
surprising and delightful effect of making the program run faster (because local
variable accesses are faster than global accesses).  Furthermore the program
should avoid depending on mutating global variables, since this makes testing
much more difficult to do.

The "global main logic" of your program may be as simple as ::

   if __name__ == "__main__":
       main_logic()

at the bottom of the main module of your program.

Once your program is organized as a tractable collection of function and class
behaviours, you should write test functions that exercise the behaviours.  A
test suite that automates a sequence of tests can be associated with each module.
This sounds like a lot of work, but since Python is so terse and flexible it's
surprisingly easy.  You can make coding much more pleasant and fun by writing
your test functions in parallel with the "production code", since this makes it
easy to find bugs and even design flaws earlier.

"Support modules" that are not intended to be the main module of a program may
include a self-test of the module. ::

   if __name__ == "__main__":
       self_test()

Even programs that interact with complex external interfaces may be tested when
the external interfaces are unavailable by using "fake" interfaces implemented
in Python.


How do I create documentation from doc strings?
-----------------------------------------------

The :mod:`pydoc` module can create HTML from the doc strings in your Python
source code.  An alternative for creating API documentation purely from
docstrings is `epydoc <https://epydoc.sourceforge.net/>`_.  `Sphinx
<https://www.sphinx-doc.org>`_ can also include docstring content.


How do I get a single keypress at a time?
-----------------------------------------

For Unix variants there are several solutions.  It's straightforward to do this
using curses, but curses is a fairly large module to learn.

.. XXX this doesn't work out of the box, some IO expert needs to check why

   Here's a solution without curses::

   import termios, fcntl, sys, os
   fd = sys.stdin.fileno()

   oldterm = termios.tcgetattr(fd)
   newattr = termios.tcgetattr(fd)
   newattr[3] = newattr[3] & ~termios.ICANON & ~termios.ECHO
   termios.tcsetattr(fd, termios.TCSANOW, newattr)

   oldflags = fcntl.fcntl(fd, fcntl.F_GETFL)
   fcntl.fcntl(fd, fcntl.F_SETFL, oldflags | os.O_NONBLOCK)

   try:
       while True:
           try:
               c = sys.stdin.read(1)
               print("Got character", repr(c))
           except OSError:
               pass
   finally:
       termios.tcsetattr(fd, termios.TCSAFLUSH, oldterm)
       fcntl.fcntl(fd, fcntl.F_SETFL, oldflags)

   You need the :mod:`termios` and the :mod:`fcntl` module for any of this to
   work, and I've only tried it on Linux, though it should work elsewhere.  In
   this code, characters are read and printed one at a time.

   :func:`termios.tcsetattr` turns off stdin's echoing and disables canonical
   mode.  :func:`fcntl.fnctl` is used to obtain stdin's file descriptor flags
   and modify them for non-blocking mode.  Since reading stdin when it is empty
   results in an :exc:`OSError`, this error is caught and ignored.

   .. versionchanged:: 3.3
      *sys.stdin.read* used to raise :exc:`IOError`. Starting from Python 3.3
      :exc:`IOError` is alias for :exc:`OSError`.


Threads
=======

How do I program using threads?
-------------------------------

Be sure to use the :mod:`threading` module and not the :mod:`_thread` module.
The :mod:`threading` module builds convenient abstractions on top of the
low-level primitives provided by the :mod:`_thread` module.


None of my threads seem to run: why?
------------------------------------

As soon as the main thread exits, all threads are killed.  Your main thread is
running too quickly, giving the threads no time to do any work.

A simple fix is to add a sleep to the end of the program that's long enough for
all the threads to finish::

   import threading, time

   def thread_task(name, n):
       for i in range(n):
           print(name, i)

   for i in range(10):
       T = threading.Thread(target=thread_task, args=(str(i), i))
       T.start()

   time.sleep(10)  # <---------------------------!

But now (on many platforms) the threads don't run in parallel, but appear to run
sequentially, one at a time!  The reason is that the OS thread scheduler doesn't
start a new thread until the previous thread is blocked.

A simple fix is to add a tiny sleep to the start of the run function::

   def thread_task(name, n):
       time.sleep(0.001)  # <--------------------!
       for i in range(n):
           print(name, i)

   for i in range(10):
       T = threading.Thread(target=thread_task, args=(str(i), i))
       T.start()

   time.sleep(10)

Instead of trying to guess a good delay value for :func:`time.sleep`,
it's better to use some kind of semaphore mechanism.  One idea is to use the
:mod:`queue` module to create a queue object, let each thread append a token to
the queue when it finishes, and let the main thread read as many tokens from the
queue as there are threads.


How do I parcel out work among a bunch of worker threads?
---------------------------------------------------------

The easiest way is to use the :mod:`concurrent.futures` module,
especially the :mod:`~concurrent.futures.ThreadPoolExecutor` class.

Or, if you want fine control over the dispatching algorithm, you can write
your own logic manually.  Use the :mod:`queue` module to create a queue
containing a list of jobs.  The :class:`~queue.Queue` class maintains a
list of objects and has a ``.put(obj)`` method that adds items to the queue and
a ``.get()`` method to return them.  The class will take care of the locking
necessary to ensure that each job is handed out exactly once.

Here's a trivial example::

   import threading, queue, time

   # The worker thread gets jobs off the queue.  When the queue is empty, it
   # assumes there will be no more work and exits.
   # (Realistically workers will run until terminated.)
   def worker():
       print('Running worker')
       time.sleep(0.1)
       while True:
           try:
               arg = q.get(block=False)
           except queue.Empty:
               print('Worker', threading.current_thread(), end=' ')
               print('queue empty')
               break
           else:
               print('Worker', threading.current_thread(), end=' ')
               print('running with argument', arg)
               time.sleep(0.5)

   # Create queue
   q = queue.Queue()

   # Start a pool of 5 workers
   for i in range(5):
       t = threading.Thread(target=worker, name='worker %i' % (i+1))
       t.start()

   # Begin adding work to the queue
   for i in range(50):
       q.put(i)

   # Give threads time to run
   print('Main thread sleeping')
   time.sleep(5)

When run, this will produce the following output:

.. code-block:: none

   Running worker
   Running worker
   Running worker
   Running worker
   Running worker
   Main thread sleeping
   Worker <Thread(worker 1, started 130283832797456)> running with argument 0
   Worker <Thread(worker 2, started 130283824404752)> running with argument 1
   Worker <Thread(worker 3, started 130283816012048)> running with argument 2
   Worker <Thread(worker 4, started 130283807619344)> running with argument 3
   Worker <Thread(worker 5, started 130283799226640)> running with argument 4
   Worker <Thread(worker 1, started 130283832797456)> running with argument 5
   ...

Consult the module's documentation for more details; the :class:`~queue.Queue`
class provides a featureful interface.


What kinds of global value mutation are thread-safe?
----------------------------------------------------

A :term:`global interpreter lock` (GIL) is used internally to ensure that only one
thread runs in the Python VM at a time.  In general, Python offers to switch
among threads only between bytecode instructions; how frequently it switches can
be set via :func:`sys.setswitchinterval`.  Each bytecode instruction and
therefore all the C implementation code reached from each instruction is
therefore atomic from the point of view of a Python program.

In theory, this means an exact accounting requires an exact understanding of the
PVM bytecode implementation.  In practice, it means that operations on shared
variables of built-in data types (ints, lists, dicts, etc) that "look atomic"
really are.

For example, the following operations are all atomic (L, L1, L2 are lists, D,
D1, D2 are dicts, x, y are objects, i, j are ints)::

   L.append(x)
   L1.extend(L2)
   x = L[i]
   x = L.pop()
   L1[i:j] = L2
   L.sort()
   x = y
   x.field = y
   D[x] = y
   D1.update(D2)
   D.keys()

These aren't::

   i = i+1
   L.append(L[-1])
   L[i] = L[j]
   D[x] = D[x] + 1

Operations that replace other objects may invoke those other objects'
:meth:`~object.__del__` method when their reference count reaches zero, and that can
affect things.  This is especially true for the mass updates to dictionaries and
lists.  When in doubt, use a mutex!


Can't we get rid of the Global Interpreter Lock?
------------------------------------------------

The :term:`global interpreter lock` (GIL) is often seen as a hindrance to Python's
deployment on high-end multiprocessor server machines, because a multi-threaded
Python program effectively only uses one CPU, due to the insistence that
(almost) all Python code can only run while the GIL is held.

With the approval of :pep:`703` work is now underway to remove the GIL from the
CPython implementation of Python.  Initially it will be implemented as an
optional compiler flag when building the interpreter, and so separate
builds will be available with and without the GIL.  Long-term, the hope is
to settle on a single build, once the performance implications of removing the
GIL are fully understood.  Python 3.13 is likely to be the first release
containing this work, although it may not be completely functional in this
release.

The current work to remove the GIL is based on a
`fork of Python 3.9 with the GIL removed <https://github.com/colesbury/nogil>`_
by Sam Gross.
Prior to that,
in the days of Python 1.5, Greg Stein actually implemented a comprehensive
patch set (the "free threading" patches) that removed the GIL and replaced it
with fine-grained locking.  Adam Olsen did a similar experiment
in his `python-safethread <https://code.google.com/archive/p/python-safethread>`_
project.  Unfortunately, both of these earlier experiments exhibited a sharp
drop in single-thread
performance (at least 30% slower), due to the amount of fine-grained locking
necessary to compensate for the removal of the GIL.  The Python 3.9 fork
is the first attempt at removing the GIL with an acceptable performance
impact.

The presence of the GIL in current Python releases
doesn't mean that you can't make good use of Python on multi-CPU machines!
You just have to be creative with dividing the work up between multiple
*processes* rather than multiple *threads*.  The
:class:`~concurrent.futures.ProcessPoolExecutor` class in the new
:mod:`concurrent.futures` module provides an easy way of doing so; the
:mod:`multiprocessing` module provides a lower-level API in case you want
more control over dispatching of tasks.

Judicious use of C extensions will also help; if you use a C extension to
perform a time-consuming task, the extension can release the GIL while the
thread of execution is in the C code and allow other threads to get some work
done.  Some standard library modules such as :mod:`zlib` and :mod:`hashlib`
already do this.

An alternative approach to reducing the impact of the GIL is
to make the GIL a per-interpreter-state lock rather than truly global.
This was :ref:`first implemented in Python 3.12 <whatsnew312-pep684>` and is
available in the C API. A Python interface to it is expected in Python 3.13.
The main limitation to it at the moment is likely to be 3rd party extension
modules, since these must be written with multiple interpreters in mind in
order to be usable, so many older extension modules will not be usable.


Input and Output
================

How do I delete a file? (And other file questions...)
-----------------------------------------------------

Use ``os.remove(filename)`` or ``os.unlink(filename)``; for documentation, see
the :mod:`os` module.  The two functions are identical; :func:`~os.unlink` is simply
the name of the Unix system call for this function.

To remove a directory, use :func:`os.rmdir`; use :func:`os.mkdir` to create one.
``os.makedirs(path)`` will create any intermediate directories in ``path`` that
don't exist. ``os.removedirs(path)`` will remove intermediate directories as
long as they're empty; if you want to delete an entire directory tree and its
contents, use :func:`shutil.rmtree`.

To rename a file, use ``os.rename(old_path, new_path)``.

To truncate a file, open it using ``f = open(filename, "rb+")``, and use
``f.truncate(offset)``; offset defaults to the current seek position.  There's
also ``os.ftruncate(fd, offset)`` for files opened with :func:`os.open`, where
*fd* is the file descriptor (a small integer).

The :mod:`shutil` module also contains a number of functions to work on files
including :func:`~shutil.copyfile`, :func:`~shutil.copytree`, and
:func:`~shutil.rmtree`.


How do I copy a file?
---------------------

The :mod:`shutil` module contains a :func:`~shutil.copyfile` function.
Note that on Windows NTFS volumes, it does not copy
`alternate data streams
<https://en.wikipedia.org/wiki/NTFS#Alternate_data_stream_(ADS)>`_
nor `resource forks <https://en.wikipedia.org/wiki/Resource_fork>`__
on macOS HFS+ volumes, though both are now rarely used.
It also doesn't copy file permissions and metadata, though using
:func:`shutil.copy2` instead will preserve most (though not all) of it.


How do I read (or write) binary data?
-------------------------------------

To read or write complex binary data formats, it's best to use the :mod:`struct`
module.  It allows you to take a string containing binary data (usually numbers)
and convert it to Python objects; and vice versa.

For example, the following code reads two 2-byte integers and one 4-byte integer
in big-endian format from a file::

   import struct

   with open(filename, "rb") as f:
       s = f.read(8)
       x, y, z = struct.unpack(">hhl", s)

The '>' in the format string forces big-endian data; the letter 'h' reads one
"short integer" (2 bytes), and 'l' reads one "long integer" (4 bytes) from the
string.

For data that is more regular (e.g. a homogeneous list of ints or floats),
you can also use the :mod:`array` module.

.. note::

   To read and write binary data, it is mandatory to open the file in
   binary mode (here, passing ``"rb"`` to :func:`open`).  If you use
   ``"r"`` instead (the default), the file will be open in text mode
   and ``f.read()`` will return :class:`str` objects rather than
   :class:`bytes` objects.


I can't seem to use os.read() on a pipe created with os.popen(); why?
---------------------------------------------------------------------

:func:`os.read` is a low-level function which takes a file descriptor, a small
integer representing the opened file.  :func:`os.popen` creates a high-level
file object, the same type returned by the built-in :func:`open` function.
Thus, to read *n* bytes from a pipe *p* created with :func:`os.popen`, you need to
use ``p.read(n)``.


How do I access the serial (RS232) port?
----------------------------------------

For Win32, OSX, Linux, BSD, Jython, IronPython:

   :pypi:`pyserial`

For Unix, see a Usenet post by Mitch Chapman:

   https://groups.google.com/groups?selm=34A04430.CF9@ohioee.com


Why doesn't closing sys.stdout (stdin, stderr) really close it?
---------------------------------------------------------------

Python :term:`file objects <file object>` are a high-level layer of
abstraction on low-level C file descriptors.

For most file objects you create in Python via the built-in :func:`open`
function, ``f.close()`` marks the Python file object as being closed from
Python's point of view, and also arranges to close the underlying C file
descriptor.  This also happens automatically in ``f``'s destructor, when
``f`` becomes garbage.

But stdin, stdout and stderr are treated specially by Python, because of the
special status also given to them by C.  Running ``sys.stdout.close()`` marks
the Python-level file object as being closed, but does *not* close the
associated C file descriptor.

To close the underlying C file descriptor for one of these three, you should
first be sure that's what you really want to do (e.g., you may confuse
extension modules trying to do I/O).  If it is, use :func:`os.close`::

   os.close(stdin.fileno())
   os.close(stdout.fileno())
   os.close(stderr.fileno())

Or you can use the numeric constants 0, 1 and 2, respectively.


Network/Internet Programming
============================

What WWW tools are there for Python?
------------------------------------

See the chapters titled :ref:`internet` and :ref:`netdata` in the Library
Reference Manual.  Python has many modules that will help you build server-side
and client-side web systems.

.. XXX check if wiki page is still up to date

A summary of available frameworks is maintained by Paul Boddie at
https://wiki.python.org/moin/WebProgramming\ .


What module should I use to help with generating HTML?
------------------------------------------------------

.. XXX add modern template languages

You can find a collection of useful links on the `Web Programming wiki page
<https://wiki.python.org/moin/WebProgramming>`_.


How do I send mail from a Python script?
----------------------------------------

Use the standard library module :mod:`smtplib`.

Here's a very simple interactive mail sender that uses it.  This method will
work on any host that supports an SMTP listener. ::

   import sys, smtplib

   fromaddr = input("From: ")
   toaddrs  = input("To: ").split(',')
   print("Enter message, end with ^D:")
   msg = ''
   while True:
       line = sys.stdin.readline()
       if not line:
           break
       msg += line

   # The actual mail send
   server = smtplib.SMTP('localhost')
   server.sendmail(fromaddr, toaddrs, msg)
   server.quit()

A Unix-only alternative uses sendmail.  The location of the sendmail program
varies between systems; sometimes it is ``/usr/lib/sendmail``, sometimes
``/usr/sbin/sendmail``.  The sendmail manual page will help you out.  Here's
some sample code::

   import os

   SENDMAIL = "/usr/sbin/sendmail"  # sendmail location
   p = os.popen("%s -t -i" % SENDMAIL, "w")
   p.write("To: receiver@example.com\n")
   p.write("Subject: test\n")
   p.write("\n")  # blank line separating headers from body
   p.write("Some text\n")
   p.write("some more text\n")
   sts = p.close()
   if sts != 0:
       print("Sendmail exit status", sts)


How do I avoid blocking in the connect() method of a socket?
------------------------------------------------------------

The :mod:`select` module is commonly used to help with asynchronous I/O on
sockets.

To prevent the TCP connect from blocking, you can set the socket to non-blocking
mode.  Then when you do the :meth:`~socket.socket.connect`,
you will either connect immediately
(unlikely) or get an exception that contains the error number as ``.errno``.
``errno.EINPROGRESS`` indicates that the connection is in progress, but hasn't
finished yet.  Different OSes will return different values, so you're going to
have to check what's returned on your system.

You can use the :meth:`~socket.socket.connect_ex` method
to avoid creating an exception.
It will just return the errno value.
To poll, you can call :meth:`~socket.socket.connect_ex` again later
-- ``0`` or ``errno.EISCONN`` indicate that you're connected -- or you can pass this
socket to :meth:`select.select` to check if it's writable.

.. note::
   The :mod:`asyncio` module provides a general purpose single-threaded and
   concurrent asynchronous library, which can be used for writing non-blocking
   network code.
   The third-party `Twisted <https://twisted.org/>`_ library is
   a popular and feature-rich alternative.


Databases
=========

Are there any interfaces to database packages in Python?
--------------------------------------------------------

Yes.

Interfaces to disk-based hashes such as :mod:`DBM <dbm.ndbm>` and :mod:`GDBM
<dbm.gnu>` are also included with standard Python.  There is also the
:mod:`sqlite3` module, which provides a lightweight disk-based relational
database.

Support for most relational databases is available.  See the
`DatabaseProgramming wiki page
<https://wiki.python.org/moin/DatabaseProgramming>`_ for details.


How do you implement persistent objects in Python?
--------------------------------------------------

The :mod:`pickle` library module solves this in a very general way (though you
still can't store things like open files, sockets or windows), and the
:mod:`shelve` library module uses pickle and (g)dbm to create persistent
mappings containing arbitrary Python objects.


Mathematics and Numerics
========================

How do I generate random numbers in Python?
-------------------------------------------

The standard module :mod:`random` implements a random number generator.  Usage
is simple::

   import random
   random.random()

This returns a random floating-point number in the range [0, 1).

There are also many other specialized generators in this module, such as:

* ``randrange(a, b)`` chooses an integer in the range [a, b).
* ``uniform(a, b)`` chooses a floating-point number in the range [a, b).
* ``normalvariate(mean, sdev)`` samples the normal (Gaussian) distribution.

Some higher-level functions operate on sequences directly, such as:

* ``choice(S)`` chooses a random element from a given sequence.
* ``shuffle(L)`` shuffles a list in-place, i.e. permutes it randomly.

There's also a ``Random`` class you can instantiate to create independent
multiple random number generators.
