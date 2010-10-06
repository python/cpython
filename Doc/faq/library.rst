:tocdepth: 2

=========================
Library and Extension FAQ
=========================

.. contents::

General Library Questions
=========================

How do I find a module or application to perform task X?
--------------------------------------------------------

Check :ref:`the Library Reference <library-index>` to see if there's a relevant
standard library module.  (Eventually you'll learn what's in the standard
library and will able to skip this step.)

For third-party packages, search the `Python Package Index
<http://pypi.python.org/pypi>`_ or try `Google <http://www.google.com>`_ or
another Web search engine.  Searching for "Python" plus a keyword or two for
your topic of interest will usually find something helpful.


Where is the math.py (socket.py, regex.py, etc.) source file?
-------------------------------------------------------------

If you can't find a source file for a module it may be a built-in or
dynamically loaded module implemented in C, C++ or other compiled language.
In this case you may not have the source file or it may be something like
mathmodule.c, somewhere in a C source directory (not on the Python Path).

There are (at least) three kinds of modules in Python:

1) modules written in Python (.py);
2) modules written in C and dynamically loaded (.dll, .pyd, .so, .sl, etc);
3) modules written in C and linked with the interpreter; to get a list of these,
   type::

      import sys
      print sys.builtin_module_names


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
lives, you can use the "env" program.  Almost all Unix variants support the
following, assuming the Python interpreter is in a directory on the user's
$PATH::

  #!/usr/bin/env python

*Don't* do this for CGI scripts.  The $PATH variable for CGI scripts is often
very minimal, so you need to use the actual absolute pathname of the
interpreter.

Occasionally, a user's environment is so full that the /usr/bin/env program
fails; or there's no env program at all.  In that case, you can try the
following hack (due to Alex Rezinsky)::

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
module in the ``Modules/`` subdirectory, though it's not compiled by default
(note that this is not available in the Windows distribution -- there is no
curses module for Windows).

The curses module supports basic curses features as well as many additional
functions from ncurses and SYSV curses such as colour, alternative character set
support, pads, and mouse support. This means the module isn't compatible with
operating systems that only have BSD curses, but there don't seem to be any
currently maintained OSes that fall into this category.

For Windows: use `the consolelib module
<http://effbot.org/zone/console-index.htm>`_.


Is there an equivalent to C's onexit() in Python?
-------------------------------------------------

The :mod:`atexit` module provides a register function that is similar to C's
onexit.


Why don't my signal handlers work?
----------------------------------

The most common problem is that the signal handler is declared with the wrong
argument list.  It is called as ::

   handler(signum, frame)

so it should be declared with two arguments::

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

For testing, it helps to write the program so that it may be easily tested by
using good modular design.  Your program should have almost all functionality
encapsulated in either functions or class methods -- and this sometimes has the
surprising and delightful effect of making the program run faster (because local
variable accesses are faster than global accesses).  Furthermore the program
should avoid depending on mutating global variables, since this makes testing
much more difficult to do.

The "global main logic" of your program may be as simple as ::

   if __name__ == "__main__":
       main_logic()

at the bottom of the main module of your program.

Once your program is organized as a tractable collection of functions and class
behaviours you should write test functions that exercise the behaviours.  A test
suite can be associated with each module which automates a sequence of tests.
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
docstrings is `epydoc <http://epydoc.sf.net/>`_.  `Sphinx
<http://sphinx.pocoo.org>`_ can also include docstring content.


How do I get a single keypress at a time?
-----------------------------------------

For Unix variants: There are several solutions.  It's straightforward to do this
using curses, but curses is a fairly large module to learn.  Here's a solution
without curses::

   import termios, fcntl, sys, os
   fd = sys.stdin.fileno()

   oldterm = termios.tcgetattr(fd)
   newattr = termios.tcgetattr(fd)
   newattr[3] = newattr[3] & ~termios.ICANON & ~termios.ECHO
   termios.tcsetattr(fd, termios.TCSANOW, newattr)

   oldflags = fcntl.fcntl(fd, fcntl.F_GETFL)
   fcntl.fcntl(fd, fcntl.F_SETFL, oldflags | os.O_NONBLOCK)

   try:
       while 1:
           try:
               c = sys.stdin.read(1)
               print "Got character", repr(c)
           except IOError: pass
   finally:
       termios.tcsetattr(fd, termios.TCSAFLUSH, oldterm)
       fcntl.fcntl(fd, fcntl.F_SETFL, oldflags)

You need the :mod:`termios` and the :mod:`fcntl` module for any of this to work,
and I've only tried it on Linux, though it should work elsewhere.  In this code,
characters are read and printed one at a time.

:func:`termios.tcsetattr` turns off stdin's echoing and disables canonical mode.
:func:`fcntl.fnctl` is used to obtain stdin's file descriptor flags and modify
them for non-blocking mode.  Since reading stdin when it is empty results in an
:exc:`IOError`, this error is caught and ignored.


Threads
=======

How do I program using threads?
-------------------------------

.. XXX it's _thread in py3k

Be sure to use the :mod:`threading` module and not the :mod:`thread` module.
The :mod:`threading` module builds convenient abstractions on top of the
low-level primitives provided by the :mod:`thread` module.

Aahz has a set of slides from his threading tutorial that are helpful; see
http://www.pythoncraft.com/OSCON2001/.


None of my threads seem to run: why?
------------------------------------

As soon as the main thread exits, all threads are killed.  Your main thread is
running too quickly, giving the threads no time to do any work.

A simple fix is to add a sleep to the end of the program that's long enough for
all the threads to finish::

   import threading, time

   def thread_task(name, n):
       for i in range(n): print name, i

   for i in range(10):
       T = threading.Thread(target=thread_task, args=(str(i), i))
       T.start()

   time.sleep(10) # <----------------------------!

But now (on many platforms) the threads don't run in parallel, but appear to run
sequentially, one at a time!  The reason is that the OS thread scheduler doesn't
start a new thread until the previous thread is blocked.

A simple fix is to add a tiny sleep to the start of the run function::

   def thread_task(name, n):
       time.sleep(0.001) # <---------------------!
       for i in range(n): print name, i

   for i in range(10):
       T = threading.Thread(target=thread_task, args=(str(i), i))
       T.start()

   time.sleep(10)

Instead of trying to guess how long a :func:`time.sleep` delay will be enough,
it's better to use some kind of semaphore mechanism.  One idea is to use the
:mod:`Queue` module to create a queue object, let each thread append a token to
the queue when it finishes, and let the main thread read as many tokens from the
queue as there are threads.


How do I parcel out work among a bunch of worker threads?
---------------------------------------------------------

Use the :mod:`Queue` module to create a queue containing a list of jobs.  The
:class:`~Queue.Queue` class maintains a list of objects with ``.put(obj)`` to
add an item to the queue and ``.get()`` to return an item.  The class will take
care of the locking necessary to ensure that each job is handed out exactly
once.

Here's a trivial example::

   import threading, Queue, time

   # The worker thread gets jobs off the queue.  When the queue is empty, it
   # assumes there will be no more work and exits.
   # (Realistically workers will run until terminated.)
   def worker ():
       print 'Running worker'
       time.sleep(0.1)
       while True:
           try:
               arg = q.get(block=False)
           except Queue.Empty:
               print 'Worker', threading.currentThread(),
               print 'queue empty'
               break
           else:
               print 'Worker', threading.currentThread(),
               print 'running with argument', arg
               time.sleep(0.5)

   # Create queue
   q = Queue.Queue()

   # Start a pool of 5 workers
   for i in range(5):
       t = threading.Thread(target=worker, name='worker %i' % (i+1))
       t.start()

   # Begin adding work to the queue
   for i in range(50):
       q.put(i)

   # Give threads time to run
   print 'Main thread sleeping'
   time.sleep(5)

When run, this will produce the following output:

   Running worker
   Running worker
   Running worker
   Running worker
   Running worker
   Main thread sleeping
   Worker <Thread(worker 1, started)> running with argument 0
   Worker <Thread(worker 2, started)> running with argument 1
   Worker <Thread(worker 3, started)> running with argument 2
   Worker <Thread(worker 4, started)> running with argument 3
   Worker <Thread(worker 5, started)> running with argument 4
   Worker <Thread(worker 1, started)> running with argument 5
   ...

Consult the module's documentation for more details; the ``Queue`` class
provides a featureful interface.


What kinds of global value mutation are thread-safe?
----------------------------------------------------

A global interpreter lock (GIL) is used internally to ensure that only one
thread runs in the Python VM at a time.  In general, Python offers to switch
among threads only between bytecode instructions; how frequently it switches can
be set via :func:`sys.setcheckinterval`.  Each bytecode instruction and
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
:meth:`__del__` method when their reference count reaches zero, and that can
affect things.  This is especially true for the mass updates to dictionaries and
lists.  When in doubt, use a mutex!


Can't we get rid of the Global Interpreter Lock?
------------------------------------------------

.. XXX mention multiprocessing
.. XXX link to dbeazley's talk about GIL?

The Global Interpreter Lock (GIL) is often seen as a hindrance to Python's
deployment on high-end multiprocessor server machines, because a multi-threaded
Python program effectively only uses one CPU, due to the insistence that
(almost) all Python code can only run while the GIL is held.

Back in the days of Python 1.5, Greg Stein actually implemented a comprehensive
patch set (the "free threading" patches) that removed the GIL and replaced it
with fine-grained locking.  Unfortunately, even on Windows (where locks are very
efficient) this ran ordinary Python code about twice as slow as the interpreter
using the GIL.  On Linux the performance loss was even worse because pthread
locks aren't as efficient.

Since then, the idea of getting rid of the GIL has occasionally come up but
nobody has found a way to deal with the expected slowdown, and users who don't
use threads would not be happy if their code ran at half at the speed.  Greg's
free threading patch set has not been kept up-to-date for later Python versions.

This doesn't mean that you can't make good use of Python on multi-CPU machines!
You just have to be creative with dividing the work up between multiple
*processes* rather than multiple *threads*.  Judicious use of C extensions will
also help; if you use a C extension to perform a time-consuming task, the
extension can release the GIL while the thread of execution is in the C code and
allow other threads to get some work done.

It has been suggested that the GIL should be a per-interpreter-state lock rather
than truly global; interpreters then wouldn't be able to share objects.
Unfortunately, this isn't likely to happen either.  It would be a tremendous
amount of work, because many object implementations currently have global state.
For example, small integers and short strings are cached; these caches would
have to be moved to the interpreter state.  Other object types have their own
free list; these free lists would have to be moved to the interpreter state.
And so on.

And I doubt that it can even be done in finite time, because the same problem
exists for 3rd party extensions.  It is likely that 3rd party extensions are
being written at a faster rate than you can convert them to store all their
global state in the interpreter state.

And finally, once you have multiple interpreters not sharing any state, what
have you gained over running each interpreter in a separate process?


Input and Output
================

How do I delete a file? (And other file questions...)
-----------------------------------------------------

Use ``os.remove(filename)`` or ``os.unlink(filename)``; for documentation, see
the :mod:`os` module.  The two functions are identical; :func:`unlink` is simply
the name of the Unix system call for this function.

To remove a directory, use :func:`os.rmdir`; use :func:`os.mkdir` to create one.
``os.makedirs(path)`` will create any intermediate directories in ``path`` that
don't exist. ``os.removedirs(path)`` will remove intermediate directories as
long as they're empty; if you want to delete an entire directory tree and its
contents, use :func:`shutil.rmtree`.

To rename a file, use ``os.rename(old_path, new_path)``.

To truncate a file, open it using ``f = open(filename, "r+")``, and use
``f.truncate(offset)``; offset defaults to the current seek position.  There's
also ``os.ftruncate(fd, offset)`` for files opened with :func:`os.open`, where
``fd`` is the file descriptor (a small integer).

The :mod:`shutil` module also contains a number of functions to work on files
including :func:`~shutil.copyfile`, :func:`~shutil.copytree`, and
:func:`~shutil.rmtree`.


How do I copy a file?
---------------------

The :mod:`shutil` module contains a :func:`~shutil.copyfile` function.  Note
that on MacOS 9 it doesn't copy the resource fork and Finder info.


How do I read (or write) binary data?
-------------------------------------

To read or write complex binary data formats, it's best to use the :mod:`struct`
module.  It allows you to take a string containing binary data (usually numbers)
and convert it to Python objects; and vice versa.

For example, the following code reads two 2-byte integers and one 4-byte integer
in big-endian format from a file::

   import struct

   f = open(filename, "rb")  # Open in binary mode for portability
   s = f.read(8)
   x, y, z = struct.unpack(">hhl", s)

The '>' in the format string forces big-endian data; the letter 'h' reads one
"short integer" (2 bytes), and 'l' reads one "long integer" (4 bytes) from the
string.

For data that is more regular (e.g. a homogeneous list of ints or thefloats),
you can also use the :mod:`array` module.


I can't seem to use os.read() on a pipe created with os.popen(); why?
---------------------------------------------------------------------

:func:`os.read` is a low-level function which takes a file descriptor, a small
integer representing the opened file.  :func:`os.popen` creates a high-level
file object, the same type returned by the built-in :func:`open` function.
Thus, to read n bytes from a pipe p created with :func:`os.popen`, you need to
use ``p.read(n)``.


How do I run a subprocess with pipes connected to both input and output?
------------------------------------------------------------------------

.. XXX update to use subprocess

Use the :mod:`popen2` module.  For example::

   import popen2
   fromchild, tochild = popen2.popen2("command")
   tochild.write("input\n")
   tochild.flush()
   output = fromchild.readline()

Warning: in general it is unwise to do this because you can easily cause a
deadlock where your process is blocked waiting for output from the child while
the child is blocked waiting for input from you.  This can be caused because the
parent expects the child to output more text than it does, or it can be caused
by data being stuck in stdio buffers due to lack of flushing.  The Python parent
can of course explicitly flush the data it sends to the child before it reads
any output, but if the child is a naive C program it may have been written to
never explicitly flush its output, even if it is interactive, since flushing is
normally automatic.

Note that a deadlock is also possible if you use :func:`popen3` to read stdout
and stderr. If one of the two is too large for the internal buffer (increasing
the buffer size does not help) and you ``read()`` the other one first, there is
a deadlock, too.

Note on a bug in popen2: unless your program calls ``wait()`` or ``waitpid()``,
finished child processes are never removed, and eventually calls to popen2 will
fail because of a limit on the number of child processes.  Calling
:func:`os.waitpid` with the :data:`os.WNOHANG` option can prevent this; a good
place to insert such a call would be before calling ``popen2`` again.

In many cases, all you really need is to run some data through a command and get
the result back.  Unless the amount of data is very large, the easiest way to do
this is to write it to a temporary file and run the command with that temporary
file as input.  The standard module :mod:`tempfile` exports a ``mktemp()``
function to generate unique temporary file names. ::

   import tempfile
   import os

   class Popen3:
       """
       This is a deadlock-safe version of popen that returns
       an object with errorlevel, out (a string) and err (a string).
       (capturestderr may not work under windows.)
       Example: print Popen3('grep spam','\n\nhere spam\n\n').out
       """
       def __init__(self,command,input=None,capturestderr=None):
           outfile=tempfile.mktemp()
           command="( %s ) > %s" % (command,outfile)
           if input:
               infile=tempfile.mktemp()
               open(infile,"w").write(input)
               command=command+" <"+infile
           if capturestderr:
               errfile=tempfile.mktemp()
               command=command+" 2>"+errfile
           self.errorlevel=os.system(command) >> 8
           self.out=open(outfile,"r").read()
           os.remove(outfile)
           if input:
               os.remove(infile)
           if capturestderr:
               self.err=open(errfile,"r").read()
               os.remove(errfile)

Note that many interactive programs (e.g. vi) don't work well with pipes
substituted for standard input and output.  You will have to use pseudo ttys
("ptys") instead of pipes. Or you can use a Python interface to Don Libes'
"expect" library.  A Python extension that interfaces to expect is called "expy"
and available from http://expectpy.sourceforge.net.  A pure Python solution that
works like expect is `pexpect <http://pypi.python.org/pypi/pexpect/>`_.


How do I access the serial (RS232) port?
----------------------------------------

For Win32, POSIX (Linux, BSD, etc.), Jython:

   http://pyserial.sourceforge.net

For Unix, see a Usenet post by Mitch Chapman:

   http://groups.google.com/groups?selm=34A04430.CF9@ohioee.com


Why doesn't closing sys.stdout (stdin, stderr) really close it?
---------------------------------------------------------------

Python file objects are a high-level layer of abstraction on top of C streams,
which in turn are a medium-level layer of abstraction on top of (among other
things) low-level C file descriptors.

For most file objects you create in Python via the built-in ``file``
constructor, ``f.close()`` marks the Python file object as being closed from
Python's point of view, and also arranges to close the underlying C stream.
This also happens automatically in ``f``'s destructor, when ``f`` becomes
garbage.

But stdin, stdout and stderr are treated specially by Python, because of the
special status also given to them by C.  Running ``sys.stdout.close()`` marks
the Python-level file object as being closed, but does *not* close the
associated C stream.

To close the underlying C stream for one of these three, you should first be
sure that's what you really want to do (e.g., you may confuse extension modules
trying to do I/O).  If it is, use os.close::

    os.close(0)   # close C's stdin stream
    os.close(1)   # close C's stdout stream
    os.close(2)   # close C's stderr stream


Network/Internet Programming
============================

What WWW tools are there for Python?
------------------------------------

See the chapters titled :ref:`internet` and :ref:`netdata` in the Library
Reference Manual.  Python has many modules that will help you build server-side
and client-side web systems.

.. XXX check if wiki page is still up to date

A summary of available frameworks is maintained by Paul Boddie at
http://wiki.python.org/moin/WebProgramming .

Cameron Laird maintains a useful set of pages about Python web technologies at
http://phaseit.net/claird/comp.lang.python/web_python.


How can I mimic CGI form submission (METHOD=POST)?
--------------------------------------------------

I would like to retrieve web pages that are the result of POSTing a form. Is
there existing code that would let me do this easily?

Yes. Here's a simple example that uses httplib::

   #!/usr/local/bin/python

   import httplib, sys, time

   ### build the query string
   qs = "First=Josephine&MI=Q&Last=Public"

   ### connect and send the server a path
   httpobj = httplib.HTTP('www.some-server.out-there', 80)
   httpobj.putrequest('POST', '/cgi-bin/some-cgi-script')
   ### now generate the rest of the HTTP headers...
   httpobj.putheader('Accept', '*/*')
   httpobj.putheader('Connection', 'Keep-Alive')
   httpobj.putheader('Content-type', 'application/x-www-form-urlencoded')
   httpobj.putheader('Content-length', '%d' % len(qs))
   httpobj.endheaders()
   httpobj.send(qs)
   ### find out what the server said in response...
   reply, msg, hdrs = httpobj.getreply()
   if reply != 200:
       sys.stdout.write(httpobj.getfile().read())

Note that in general for percent-encoded POST operations, query strings must be
quoted using :func:`urllib.quote`.  For example to send name="Guy Steele, Jr."::

   >>> from urllib import quote
   >>> x = quote("Guy Steele, Jr.")
   >>> x
   'Guy%20Steele,%20Jr.'
   >>> query_string = "name="+x
   >>> query_string
   'name=Guy%20Steele,%20Jr.'


What module should I use to help with generating HTML?
------------------------------------------------------

.. XXX add modern template languages

There are many different modules available:

* HTMLgen is a class library of objects corresponding to all the HTML 3.2 markup
  tags. It's used when you are writing in Python and wish to synthesize HTML
  pages for generating a web or for CGI forms, etc.

* DocumentTemplate and Zope Page Templates are two different systems that are
  part of Zope.

* Quixote's PTL uses Python syntax to assemble strings of text.

Consult the `Web Programming wiki pages
<http://wiki.python.org/moin/WebProgramming>`_ for more links.


How do I send mail from a Python script?
----------------------------------------

Use the standard library module :mod:`smtplib`.

Here's a very simple interactive mail sender that uses it.  This method will
work on any host that supports an SMTP listener. ::

   import sys, smtplib

   fromaddr = raw_input("From: ")
   toaddrs  = raw_input("To: ").split(',')
   print "Enter message, end with ^D:"
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
varies between systems; sometimes it is ``/usr/lib/sendmail``, sometime
``/usr/sbin/sendmail``.  The sendmail manual page will help you out.  Here's
some sample code::

   SENDMAIL = "/usr/sbin/sendmail" # sendmail location
   import os
   p = os.popen("%s -t -i" % SENDMAIL, "w")
   p.write("To: receiver@example.com\n")
   p.write("Subject: test\n")
   p.write("\n") # blank line separating headers from body
   p.write("Some text\n")
   p.write("some more text\n")
   sts = p.close()
   if sts != 0:
       print "Sendmail exit status", sts


How do I avoid blocking in the connect() method of a socket?
------------------------------------------------------------

The select module is commonly used to help with asynchronous I/O on sockets.

To prevent the TCP connect from blocking, you can set the socket to non-blocking
mode.  Then when you do the ``connect()``, you will either connect immediately
(unlikely) or get an exception that contains the error number as ``.errno``.
``errno.EINPROGRESS`` indicates that the connection is in progress, but hasn't
finished yet.  Different OSes will return different values, so you're going to
have to check what's returned on your system.

You can use the ``connect_ex()`` method to avoid creating an exception.  It will
just return the errno value.  To poll, you can call ``connect_ex()`` again later
-- 0 or ``errno.EISCONN`` indicate that you're connected -- or you can pass this
socket to select to check if it's writable.


Databases
=========

Are there any interfaces to database packages in Python?
--------------------------------------------------------

Yes.

.. XXX remove bsddb in py3k, fix other module names

Python 2.3 includes the :mod:`bsddb` package which provides an interface to the
BerkeleyDB library.  Interfaces to disk-based hashes such as :mod:`DBM <dbm>`
and :mod:`GDBM <gdbm>` are also included with standard Python.

Support for most relational databases is available.  See the
`DatabaseProgramming wiki page
<http://wiki.python.org/moin/DatabaseProgramming>`_ for details.


How do you implement persistent objects in Python?
--------------------------------------------------

The :mod:`pickle` library module solves this in a very general way (though you
still can't store things like open files, sockets or windows), and the
:mod:`shelve` library module uses pickle and (g)dbm to create persistent
mappings containing arbitrary Python objects.  For better performance, you can
use the :mod:`cPickle` module.

A more awkward way of doing things is to use pickle's little sister, marshal.
The :mod:`marshal` module provides very fast ways to store noncircular basic
Python types to files and strings, and back again.  Although marshal does not do
fancy things like store instances or handle shared references properly, it does
run extremely fast.  For example loading a half megabyte of data may take less
than a third of a second.  This often beats doing something more complex and
general such as using gdbm with pickle/shelve.


Why is cPickle so slow?
-----------------------

.. XXX update this, default protocol is 2/3

The default format used by the pickle module is a slow one that results in
readable pickles.  Making it the default, but it would break backward
compatibility::

    largeString = 'z' * (100 * 1024)
    myPickle = cPickle.dumps(largeString, protocol=1)


If my program crashes with a bsddb (or anydbm) database open, it gets corrupted. How come?
------------------------------------------------------------------------------------------

Databases opened for write access with the bsddb module (and often by the anydbm
module, since it will preferentially use bsddb) must explicitly be closed using
the ``.close()`` method of the database.  The underlying library caches database
contents which need to be converted to on-disk form and written.

If you have initialized a new bsddb database but not written anything to it
before the program crashes, you will often wind up with a zero-length file and
encounter an exception the next time the file is opened.


I tried to open Berkeley DB file, but bsddb produces bsddb.error: (22, 'Invalid argument'). Help! How can I restore my data?
----------------------------------------------------------------------------------------------------------------------------

Don't panic! Your data is probably intact. The most frequent cause for the error
is that you tried to open an earlier Berkeley DB file with a later version of
the Berkeley DB library.

Many Linux systems now have all three versions of Berkeley DB available.  If you
are migrating from version 1 to a newer version use db_dump185 to dump a plain
text version of the database.  If you are migrating from version 2 to version 3
use db2_dump to create a plain text version of the database.  In either case,
use db_load to create a new native database for the latest version installed on
your computer.  If you have version 3 of Berkeley DB installed, you should be
able to use db2_load to create a native version 2 database.

You should move away from Berkeley DB version 1 files because the hash file code
contains known bugs that can corrupt your data.


Mathematics and Numerics
========================

How do I generate random numbers in Python?
-------------------------------------------

The standard module :mod:`random` implements a random number generator.  Usage
is simple::

   import random
   random.random()

This returns a random floating point number in the range [0, 1).

There are also many other specialized generators in this module, such as:

* ``randrange(a, b)`` chooses an integer in the range [a, b).
* ``uniform(a, b)`` chooses a floating point number in the range [a, b).
* ``normalvariate(mean, sdev)`` samples the normal (Gaussian) distribution.

Some higher-level functions operate on sequences directly, such as:

* ``choice(S)`` chooses random element from a given sequence
* ``shuffle(L)`` shuffles a list in-place, i.e. permutes it randomly

There's also a ``Random`` class you can instantiate to create independent
multiple random number generators.
