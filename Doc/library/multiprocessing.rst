:mod:`multiprocessing` --- Process-based parallelism
====================================================

.. module:: multiprocessing
   :synopsis: Process-based parallelism.

**Source code:** :source:`Lib/multiprocessing/`

--------------

.. include:: ../includes/wasm-notavail.rst

Introduction
------------

:mod:`multiprocessing` is a package that supports spawning processes using an
API similar to the :mod:`threading` module.  The :mod:`multiprocessing` package
offers both local and remote concurrency, effectively side-stepping the
:term:`Global Interpreter Lock <global interpreter lock>` by using
subprocesses instead of threads.  Due
to this, the :mod:`multiprocessing` module allows the programmer to fully
leverage multiple processors on a given machine.  It runs on both POSIX and
Windows.

The :mod:`multiprocessing` module also introduces APIs which do not have
analogs in the :mod:`threading` module.  A prime example of this is the
:class:`~multiprocessing.pool.Pool` object which offers a convenient means of
parallelizing the execution of a function across multiple input values,
distributing the input data across processes (data parallelism).  The following
example demonstrates the common practice of defining such functions in a module
so that child processes can successfully import that module.  This basic example
of data parallelism using :class:`~multiprocessing.pool.Pool`, ::

   from multiprocessing import Pool

   def f(x):
       return x*x

   if __name__ == '__main__':
       with Pool(5) as p:
           print(p.map(f, [1, 2, 3]))

will print to standard output ::

   [1, 4, 9]


.. seealso::

   :class:`concurrent.futures.ProcessPoolExecutor` offers a higher level interface
   to push tasks to a background process without blocking execution of the
   calling process. Compared to using the :class:`~multiprocessing.pool.Pool`
   interface directly, the :mod:`concurrent.futures` API more readily allows
   the submission of work to the underlying process pool to be separated from
   waiting for the results.


The :class:`Process` class
~~~~~~~~~~~~~~~~~~~~~~~~~~

In :mod:`multiprocessing`, processes are spawned by creating a :class:`Process`
object and then calling its :meth:`~Process.start` method.  :class:`Process`
follows the API of :class:`threading.Thread`.  A trivial example of a
multiprocess program is ::

   from multiprocessing import Process

   def f(name):
       print('hello', name)

   if __name__ == '__main__':
       p = Process(target=f, args=('bob',))
       p.start()
       p.join()

To show the individual process IDs involved, here is an expanded example::

    from multiprocessing import Process
    import os

    def info(title):
        print(title)
        print('module name:', __name__)
        print('parent process:', os.getppid())
        print('process id:', os.getpid())

    def f(name):
        info('function f')
        print('hello', name)

    if __name__ == '__main__':
        info('main line')
        p = Process(target=f, args=('bob',))
        p.start()
        p.join()

For an explanation of why the ``if __name__ == '__main__'`` part is
necessary, see :ref:`multiprocessing-programming`.



.. _multiprocessing-start-methods:

Contexts and start methods
~~~~~~~~~~~~~~~~~~~~~~~~~~

Depending on the platform, :mod:`multiprocessing` supports three ways
to start a process.  These *start methods* are

  *spawn*
    The parent process starts a fresh Python interpreter process.  The
    child process will only inherit those resources necessary to run
    the process object's :meth:`~Process.run` method.  In particular,
    unnecessary file descriptors and handles from the parent process
    will not be inherited.  Starting a process using this method is
    rather slow compared to using *fork* or *forkserver*.

    Available on POSIX and Windows platforms.  The default on Windows and macOS.

  *fork*
    The parent process uses :func:`os.fork` to fork the Python
    interpreter.  The child process, when it begins, is effectively
    identical to the parent process.  All resources of the parent are
    inherited by the child process.  Note that safely forking a
    multithreaded process is problematic.

    Available on POSIX systems.  Currently the default on POSIX except macOS.

    .. note::
       The default start method will change away from *fork* in Python 3.14.
       Code that requires *fork* should explicitly specify that via
       :func:`get_context` or :func:`set_start_method`.

  *forkserver*
    When the program starts and selects the *forkserver* start method,
    a server process is spawned.  From then on, whenever a new process
    is needed, the parent process connects to the server and requests
    that it fork a new process.  The fork server process is single threaded
    unless system libraries or preloaded imports spawn threads as a
    side-effect so it is generally safe for it to use :func:`os.fork`.
    No unnecessary resources are inherited.

    Available on POSIX platforms which support passing file descriptors
    over Unix pipes such as Linux.


.. versionchanged:: 3.8

   On macOS, the *spawn* start method is now the default.  The *fork* start
   method should be considered unsafe as it can lead to crashes of the
   subprocess as macOS system libraries may start threads. See :issue:`33725`.

.. versionchanged:: 3.4
   *spawn* added on all POSIX platforms, and *forkserver* added for
   some POSIX platforms.
   Child processes no longer inherit all of the parents inheritable
   handles on Windows.

On POSIX using the *spawn* or *forkserver* start methods will also
start a *resource tracker* process which tracks the unlinked named
system resources (such as named semaphores or
:class:`~multiprocessing.shared_memory.SharedMemory` objects) created
by processes of the program.  When all processes
have exited the resource tracker unlinks any remaining tracked object.
Usually there should be none, but if a process was killed by a signal
there may be some "leaked" resources.  (Neither leaked semaphores nor shared
memory segments will be automatically unlinked until the next reboot. This is
problematic for both objects because the system allows only a limited number of
named semaphores, and shared memory segments occupy some space in the main
memory.)

To select a start method you use the :func:`set_start_method` in
the ``if __name__ == '__main__'`` clause of the main module.  For
example::

       import multiprocessing as mp

       def foo(q):
           q.put('hello')

       if __name__ == '__main__':
           mp.set_start_method('spawn')
           q = mp.Queue()
           p = mp.Process(target=foo, args=(q,))
           p.start()
           print(q.get())
           p.join()

:func:`set_start_method` should not be used more than once in the
program.

Alternatively, you can use :func:`get_context` to obtain a context
object.  Context objects have the same API as the multiprocessing
module, and allow one to use multiple start methods in the same
program. ::

       import multiprocessing as mp

       def foo(q):
           q.put('hello')

       if __name__ == '__main__':
           ctx = mp.get_context('spawn')
           q = ctx.Queue()
           p = ctx.Process(target=foo, args=(q,))
           p.start()
           print(q.get())
           p.join()

Note that objects related to one context may not be compatible with
processes for a different context.  In particular, locks created using
the *fork* context cannot be passed to processes started using the
*spawn* or *forkserver* start methods.

A library which wants to use a particular start method should probably
use :func:`get_context` to avoid interfering with the choice of the
library user.

.. warning::

   The ``'spawn'`` and ``'forkserver'`` start methods generally cannot
   be used with "frozen" executables (i.e., binaries produced by
   packages like **PyInstaller** and **cx_Freeze**) on POSIX systems.
   The ``'fork'`` start method may work if code does not use threads.


Exchanging objects between processes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:mod:`multiprocessing` supports two types of communication channel between
processes:

**Queues**

   The :class:`Queue` class is a near clone of :class:`queue.Queue`.  For
   example::

      from multiprocessing import Process, Queue

      def f(q):
          q.put([42, None, 'hello'])

      if __name__ == '__main__':
          q = Queue()
          p = Process(target=f, args=(q,))
          p.start()
          print(q.get())    # prints "[42, None, 'hello']"
          p.join()

   Queues are thread and process safe.

**Pipes**

   The :func:`Pipe` function returns a pair of connection objects connected by a
   pipe which by default is duplex (two-way).  For example::

      from multiprocessing import Process, Pipe

      def f(conn):
          conn.send([42, None, 'hello'])
          conn.close()

      if __name__ == '__main__':
          parent_conn, child_conn = Pipe()
          p = Process(target=f, args=(child_conn,))
          p.start()
          print(parent_conn.recv())   # prints "[42, None, 'hello']"
          p.join()

   The two connection objects returned by :func:`Pipe` represent the two ends of
   the pipe.  Each connection object has :meth:`~Connection.send` and
   :meth:`~Connection.recv` methods (among others).  Note that data in a pipe
   may become corrupted if two processes (or threads) try to read from or write
   to the *same* end of the pipe at the same time.  Of course there is no risk
   of corruption from processes using different ends of the pipe at the same
   time.


Synchronization between processes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

:mod:`multiprocessing` contains equivalents of all the synchronization
primitives from :mod:`threading`.  For instance one can use a lock to ensure
that only one process prints to standard output at a time::

   from multiprocessing import Process, Lock

   def f(l, i):
       l.acquire()
       try:
           print('hello world', i)
       finally:
           l.release()

   if __name__ == '__main__':
       lock = Lock()

       for num in range(10):
           Process(target=f, args=(lock, num)).start()

Without using the lock output from the different processes is liable to get all
mixed up.


Sharing state between processes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

As mentioned above, when doing concurrent programming it is usually best to
avoid using shared state as far as possible.  This is particularly true when
using multiple processes.

However, if you really do need to use some shared data then
:mod:`multiprocessing` provides a couple of ways of doing so.

**Shared memory**

   Data can be stored in a shared memory map using :class:`Value` or
   :class:`Array`.  For example, the following code ::

      from multiprocessing import Process, Value, Array

      def f(n, a):
          n.value = 3.1415927
          for i in range(len(a)):
              a[i] = -a[i]

      if __name__ == '__main__':
          num = Value('d', 0.0)
          arr = Array('i', range(10))

          p = Process(target=f, args=(num, arr))
          p.start()
          p.join()

          print(num.value)
          print(arr[:])

   will print ::

      3.1415927
      [0, -1, -2, -3, -4, -5, -6, -7, -8, -9]

   The ``'d'`` and ``'i'`` arguments used when creating ``num`` and ``arr`` are
   typecodes of the kind used by the :mod:`array` module: ``'d'`` indicates a
   double precision float and ``'i'`` indicates a signed integer.  These shared
   objects will be process and thread-safe.

   For more flexibility in using shared memory one can use the
   :mod:`multiprocessing.sharedctypes` module which supports the creation of
   arbitrary ctypes objects allocated from shared memory.

**Server process**

   A manager object returned by :func:`Manager` controls a server process which
   holds Python objects and allows other processes to manipulate them using
   proxies.

   A manager returned by :func:`Manager` will support types
   :class:`list`, :class:`dict`, :class:`~managers.Namespace`, :class:`Lock`,
   :class:`RLock`, :class:`Semaphore`, :class:`BoundedSemaphore`,
   :class:`Condition`, :class:`Event`, :class:`Barrier`,
   :class:`Queue`, :class:`Value` and :class:`Array`.  For example, ::

      from multiprocessing import Process, Manager

      def f(d, l):
          d[1] = '1'
          d['2'] = 2
          d[0.25] = None
          l.reverse()

      if __name__ == '__main__':
          with Manager() as manager:
              d = manager.dict()
              l = manager.list(range(10))

              p = Process(target=f, args=(d, l))
              p.start()
              p.join()

              print(d)
              print(l)

   will print ::

       {0.25: None, 1: '1', '2': 2}
       [9, 8, 7, 6, 5, 4, 3, 2, 1, 0]

   Server process managers are more flexible than using shared memory objects
   because they can be made to support arbitrary object types.  Also, a single
   manager can be shared by processes on different computers over a network.
   They are, however, slower than using shared memory.


Using a pool of workers
~~~~~~~~~~~~~~~~~~~~~~~

The :class:`~multiprocessing.pool.Pool` class represents a pool of worker
processes.  It has methods which allows tasks to be offloaded to the worker
processes in a few different ways.

For example::

   from multiprocessing import Pool, TimeoutError
   import time
   import os

   def f(x):
       return x*x

   if __name__ == '__main__':
       # start 4 worker processes
       with Pool(processes=4) as pool:

           # print "[0, 1, 4,..., 81]"
           print(pool.map(f, range(10)))

           # print same numbers in arbitrary order
           for i in pool.imap_unordered(f, range(10)):
               print(i)

           # evaluate "f(20)" asynchronously
           res = pool.apply_async(f, (20,))      # runs in *only* one process
           print(res.get(timeout=1))             # prints "400"

           # evaluate "os.getpid()" asynchronously
           res = pool.apply_async(os.getpid, ()) # runs in *only* one process
           print(res.get(timeout=1))             # prints the PID of that process

           # launching multiple evaluations asynchronously *may* use more processes
           multiple_results = [pool.apply_async(os.getpid, ()) for i in range(4)]
           print([res.get(timeout=1) for res in multiple_results])

           # make a single worker sleep for 10 seconds
           res = pool.apply_async(time.sleep, (10,))
           try:
               print(res.get(timeout=1))
           except TimeoutError:
               print("We lacked patience and got a multiprocessing.TimeoutError")

           print("For the moment, the pool remains available for more work")

       # exiting the 'with'-block has stopped the pool
       print("Now the pool is closed and no longer available")

Note that the methods of a pool should only ever be used by the
process which created it.

.. note::

   Functionality within this package requires that the ``__main__`` module be
   importable by the children. This is covered in :ref:`multiprocessing-programming`
   however it is worth pointing out here. This means that some examples, such
   as the :class:`multiprocessing.pool.Pool` examples will not work in the
   interactive interpreter. For example:

   .. code-block:: text

      >>> from multiprocessing import Pool
      >>> p = Pool(5)
      >>> def f(x):
      ...     return x*x
      ...
      >>> with p:
      ...     p.map(f, [1,2,3])
      Process PoolWorker-1:
      Process PoolWorker-2:
      Process PoolWorker-3:
      Traceback (most recent call last):
      Traceback (most recent call last):
      Traceback (most recent call last):
      AttributeError: Can't get attribute 'f' on <module '__main__' (<class '_frozen_importlib.BuiltinImporter'>)>
      AttributeError: Can't get attribute 'f' on <module '__main__' (<class '_frozen_importlib.BuiltinImporter'>)>
      AttributeError: Can't get attribute 'f' on <module '__main__' (<class '_frozen_importlib.BuiltinImporter'>)>

   (If you try this it will actually output three full tracebacks
   interleaved in a semi-random fashion, and then you may have to
   stop the parent process somehow.

.. We use the "rubric" directive here to avoid creating
   the "Reference" subsection in the TOC.

.. rubric:: Reference

.. toctree::
   :caption: Reference
   :maxdepth: 1

   multiprocessing-reference.rst

.. toctree::
   :caption: Programming Guidelines
   :maxdepth: 1

   multiprocessing-guidelines.rst

