.. highlightlang:: c


.. _initialization:

*****************************************
Initialization, Finalization, and Threads
*****************************************


Initializing and finalizing the interpreter
===========================================


.. c:function:: void Py_Initialize()

   .. index::
      single: Py_SetProgramName()
      single: PyEval_InitThreads()
      single: PyEval_ReleaseLock()
      single: PyEval_AcquireLock()
      single: modules (in module sys)
      single: path (in module sys)
      module: __builtin__
      module: __main__
      module: sys
      triple: module; search; path
      single: PySys_SetArgv()
      single: PySys_SetArgvEx()
      single: Py_Finalize()

   Initialize the Python interpreter.  In an application embedding  Python, this
   should be called before using any other Python/C API functions; with the
   exception of :c:func:`Py_SetProgramName`, :c:func:`Py_SetPythonHome`, :c:func:`PyEval_InitThreads`,
   :c:func:`PyEval_ReleaseLock`, and :c:func:`PyEval_AcquireLock`. This initializes
   the table of loaded modules (``sys.modules``), and creates the fundamental
   modules :mod:`__builtin__`, :mod:`__main__` and :mod:`sys`.  It also initializes
   the module search path (``sys.path``). It does not set ``sys.argv``; use
   :c:func:`PySys_SetArgvEx` for that.  This is a no-op when called for a second time
   (without calling :c:func:`Py_Finalize` first).  There is no return value; it is a
   fatal error if the initialization fails.


.. c:function:: void Py_InitializeEx(int initsigs)

   This function works like :c:func:`Py_Initialize` if *initsigs* is 1. If
   *initsigs* is 0, it skips initialization registration of signal handlers, which
   might be useful when Python is embedded.

   .. versionadded:: 2.4


.. c:function:: int Py_IsInitialized()

   Return true (nonzero) when the Python interpreter has been initialized, false
   (zero) if not.  After :c:func:`Py_Finalize` is called, this returns false until
   :c:func:`Py_Initialize` is called again.


.. c:function:: void Py_Finalize()

   Undo all initializations made by :c:func:`Py_Initialize` and subsequent use of
   Python/C API functions, and destroy all sub-interpreters (see
   :c:func:`Py_NewInterpreter` below) that were created and not yet destroyed since
   the last call to :c:func:`Py_Initialize`.  Ideally, this frees all memory
   allocated by the Python interpreter.  This is a no-op when called for a second
   time (without calling :c:func:`Py_Initialize` again first).  There is no return
   value; errors during finalization are ignored.

   This function is provided for a number of reasons.  An embedding application
   might want to restart Python without having to restart the application itself.
   An application that has loaded the Python interpreter from a dynamically
   loadable library (or DLL) might want to free all memory allocated by Python
   before unloading the DLL. During a hunt for memory leaks in an application a
   developer might want to free all memory allocated by Python before exiting from
   the application.

   **Bugs and caveats:** The destruction of modules and objects in modules is done
   in random order; this may cause destructors (:meth:`__del__` methods) to fail
   when they depend on other objects (even functions) or modules.  Dynamically
   loaded extension modules loaded by Python are not unloaded.  Small amounts of
   memory allocated by the Python interpreter may not be freed (if you find a leak,
   please report it).  Memory tied up in circular references between objects is not
   freed.  Some memory allocated by extension modules may not be freed.  Some
   extensions may not work properly if their initialization routine is called more
   than once; this can happen if an application calls :c:func:`Py_Initialize` and
   :c:func:`Py_Finalize` more than once.


Process-wide parameters
=======================


.. c:function:: void Py_SetProgramName(char *name)

   .. index::
      single: Py_Initialize()
      single: main()
      single: Py_GetPath()

   This function should be called before :c:func:`Py_Initialize` is called for
   the first time, if it is called at all.  It tells the interpreter the value
   of the ``argv[0]`` argument to the :c:func:`main` function of the program.
   This is used by :c:func:`Py_GetPath` and some other functions below to find
   the Python run-time libraries relative to the interpreter executable.  The
   default value is ``'python'``.  The argument should point to a
   zero-terminated character string in static storage whose contents will not
   change for the duration of the program's execution.  No code in the Python
   interpreter will change the contents of this storage.


.. c:function:: char* Py_GetProgramName()

   .. index:: single: Py_SetProgramName()

   Return the program name set with :c:func:`Py_SetProgramName`, or the default.
   The returned string points into static storage; the caller should not modify its
   value.


.. c:function:: char* Py_GetPrefix()

   Return the *prefix* for installed platform-independent files. This is derived
   through a number of complicated rules from the program name set with
   :c:func:`Py_SetProgramName` and some environment variables; for example, if the
   program name is ``'/usr/local/bin/python'``, the prefix is ``'/usr/local'``. The
   returned string points into static storage; the caller should not modify its
   value.  This corresponds to the :makevar:`prefix` variable in the top-level
   :file:`Makefile` and the ``--prefix`` argument to the :program:`configure`
   script at build time.  The value is available to Python code as ``sys.prefix``.
   It is only useful on Unix.  See also the next function.


.. c:function:: char* Py_GetExecPrefix()

   Return the *exec-prefix* for installed platform-*dependent* files.  This is
   derived through a number of complicated rules from the program name set with
   :c:func:`Py_SetProgramName` and some environment variables; for example, if the
   program name is ``'/usr/local/bin/python'``, the exec-prefix is
   ``'/usr/local'``.  The returned string points into static storage; the caller
   should not modify its value.  This corresponds to the :makevar:`exec_prefix`
   variable in the top-level :file:`Makefile` and the ``--exec-prefix``
   argument to the :program:`configure` script at build  time.  The value is
   available to Python code as ``sys.exec_prefix``.  It is only useful on Unix.

   Background: The exec-prefix differs from the prefix when platform dependent
   files (such as executables and shared libraries) are installed in a different
   directory tree.  In a typical installation, platform dependent files may be
   installed in the :file:`/usr/local/plat` subtree while platform independent may
   be installed in :file:`/usr/local`.

   Generally speaking, a platform is a combination of hardware and software
   families, e.g.  Sparc machines running the Solaris 2.x operating system are
   considered the same platform, but Intel machines running Solaris 2.x are another
   platform, and Intel machines running Linux are yet another platform.  Different
   major revisions of the same operating system generally also form different
   platforms.  Non-Unix operating systems are a different story; the installation
   strategies on those systems are so different that the prefix and exec-prefix are
   meaningless, and set to the empty string. Note that compiled Python bytecode
   files are platform independent (but not independent from the Python version by
   which they were compiled!).

   System administrators will know how to configure the :program:`mount` or
   :program:`automount` programs to share :file:`/usr/local` between platforms
   while having :file:`/usr/local/plat` be a different filesystem for each
   platform.


.. c:function:: char* Py_GetProgramFullPath()

   .. index::
      single: Py_SetProgramName()
      single: executable (in module sys)

   Return the full program name of the Python executable; this is  computed as a
   side-effect of deriving the default module search path  from the program name
   (set by :c:func:`Py_SetProgramName` above). The returned string points into
   static storage; the caller should not modify its value.  The value is available
   to Python code as ``sys.executable``.


.. c:function:: char* Py_GetPath()

   .. index::
      triple: module; search; path
      single: path (in module sys)

   Return the default module search path; this is computed from the program name
   (set by :c:func:`Py_SetProgramName` above) and some environment variables.
   The returned string consists of a series of directory names separated by a
   platform dependent delimiter character.  The delimiter character is ``':'``
   on Unix and Mac OS X, ``';'`` on Windows.  The returned string points into
   static storage; the caller should not modify its value.  The list
   :data:`sys.path` is initialized with this value on interpreter startup; it
   can be (and usually is) modified later to change the search path for loading
   modules.

   .. XXX should give the exact rules


.. c:function:: const char* Py_GetVersion()

   Return the version of this Python interpreter.  This is a string that looks
   something like ::

      "1.5 (#67, Dec 31 1997, 22:34:28) [GCC 2.7.2.2]"

   .. index:: single: version (in module sys)

   The first word (up to the first space character) is the current Python version;
   the first three characters are the major and minor version separated by a
   period.  The returned string points into static storage; the caller should not
   modify its value.  The value is available to Python code as ``sys.version``.


.. c:function:: const char* Py_GetPlatform()

   .. index:: single: platform (in module sys)

   Return the platform identifier for the current platform.  On Unix, this is
   formed from the "official" name of the operating system, converted to lower
   case, followed by the major revision number; e.g., for Solaris 2.x, which is
   also known as SunOS 5.x, the value is ``'sunos5'``.  On Mac OS X, it is
   ``'darwin'``.  On Windows, it is ``'win'``.  The returned string points into
   static storage; the caller should not modify its value.  The value is available
   to Python code as ``sys.platform``.


.. c:function:: const char* Py_GetCopyright()

   Return the official copyright string for the current Python version, for example

   ``'Copyright 1991-1995 Stichting Mathematisch Centrum, Amsterdam'``

   .. index:: single: copyright (in module sys)

   The returned string points into static storage; the caller should not modify its
   value.  The value is available to Python code as ``sys.copyright``.


.. c:function:: const char* Py_GetCompiler()

   Return an indication of the compiler used to build the current Python version,
   in square brackets, for example::

      "[GCC 2.7.2.2]"

   .. index:: single: version (in module sys)

   The returned string points into static storage; the caller should not modify its
   value.  The value is available to Python code as part of the variable
   ``sys.version``.


.. c:function:: const char* Py_GetBuildInfo()

   Return information about the sequence number and build date and time  of the
   current Python interpreter instance, for example ::

      "#67, Aug  1 1997, 22:34:28"

   .. index:: single: version (in module sys)

   The returned string points into static storage; the caller should not modify its
   value.  The value is available to Python code as part of the variable
   ``sys.version``.


.. c:function:: void PySys_SetArgvEx(int argc, char **argv, int updatepath)

   .. index::
      single: main()
      single: Py_FatalError()
      single: argv (in module sys)

   Set :data:`sys.argv` based on *argc* and *argv*.  These parameters are
   similar to those passed to the program's :c:func:`main` function with the
   difference that the first entry should refer to the script file to be
   executed rather than the executable hosting the Python interpreter.  If there
   isn't a script that will be run, the first entry in *argv* can be an empty
   string.  If this function fails to initialize :data:`sys.argv`, a fatal
   condition is signalled using :c:func:`Py_FatalError`.

   If *updatepath* is zero, this is all the function does.  If *updatepath*
   is non-zero, the function also modifies :data:`sys.path` according to the
   following algorithm:

   - If the name of an existing script is passed in ``argv[0]``, the absolute
     path of the directory where the script is located is prepended to
     :data:`sys.path`.
   - Otherwise (that is, if *argc* is 0 or ``argv[0]`` doesn't point
     to an existing file name), an empty string is prepended to
     :data:`sys.path`, which is the same as prepending the current working
     directory (``"."``).

   .. note::
      It is recommended that applications embedding the Python interpreter
      for purposes other than executing a single script pass 0 as *updatepath*,
      and update :data:`sys.path` themselves if desired.
      See `CVE-2008-5983 <http://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2008-5983>`_.

      On versions before 2.6.6, you can achieve the same effect by manually
      popping the first :data:`sys.path` element after having called
      :c:func:`PySys_SetArgv`, for example using::

         PyRun_SimpleString("import sys; sys.path.pop(0)\n");

   .. versionadded:: 2.6.6

   .. XXX impl. doesn't seem consistent in allowing 0/NULL for the params;
      check w/ Guido.


.. c:function:: void PySys_SetArgv(int argc, char **argv)

   This function works like :c:func:`PySys_SetArgvEx` with *updatepath* set to 1.


.. c:function:: void Py_SetPythonHome(char *home)

   Set the default "home" directory, that is, the location of the standard
   Python libraries.  See :envvar:`PYTHONHOME` for the meaning of the
   argument string.

   The argument should point to a zero-terminated character string in static
   storage whose contents will not change for the duration of the program's
   execution.  No code in the Python interpreter will change the contents of
   this storage.


.. c:function:: char* Py_GetPythonHome()

   Return the default "home", that is, the value set by a previous call to
   :c:func:`Py_SetPythonHome`, or the value of the :envvar:`PYTHONHOME`
   environment variable if it is set.


.. _threads:

Thread State and the Global Interpreter Lock
============================================

.. index::
   single: GIL
   single: global interpreter lock
   single: interpreter lock
   single: lock, interpreter

The Python interpreter is not fully thread-safe.  In order to support
multi-threaded Python programs, there's a global lock, called the :term:`global
interpreter lock` or :term:`GIL`, that must be held by the current thread before
it can safely access Python objects. Without the lock, even the simplest
operations could cause problems in a multi-threaded program: for example, when
two threads simultaneously increment the reference count of the same object, the
reference count could end up being incremented only once instead of twice.

.. index:: single: setcheckinterval() (in module sys)

Therefore, the rule exists that only the thread that has acquired the
:term:`GIL` may operate on Python objects or call Python/C API functions.
In order to emulate concurrency of execution, the interpreter regularly
tries to switch threads (see :func:`sys.setcheckinterval`).  The lock is also
released around potentially blocking I/O operations like reading or writing
a file, so that other Python threads can run in the meantime.

.. index::
   single: PyThreadState
   single: PyThreadState

The Python interpreter keeps some thread-specific bookkeeping information
inside a data structure called :c:type:`PyThreadState`.  There's also one
global variable pointing to the current :c:type:`PyThreadState`: it can
be retrieved using :c:func:`PyThreadState_Get`.

Releasing the GIL from extension code
-------------------------------------

Most extension code manipulating the :term:`GIL` has the following simple
structure::

   Save the thread state in a local variable.
   Release the global interpreter lock.
   ... Do some blocking I/O operation ...
   Reacquire the global interpreter lock.
   Restore the thread state from the local variable.

This is so common that a pair of macros exists to simplify it::

   Py_BEGIN_ALLOW_THREADS
   ... Do some blocking I/O operation ...
   Py_END_ALLOW_THREADS

.. index::
   single: Py_BEGIN_ALLOW_THREADS
   single: Py_END_ALLOW_THREADS

The :c:macro:`Py_BEGIN_ALLOW_THREADS` macro opens a new block and declares a
hidden local variable; the :c:macro:`Py_END_ALLOW_THREADS` macro closes the
block.  These two macros are still available when Python is compiled without
thread support (they simply have an empty expansion).

When thread support is enabled, the block above expands to the following code::

   PyThreadState *_save;

   _save = PyEval_SaveThread();
   ...Do some blocking I/O operation...
   PyEval_RestoreThread(_save);

.. index::
   single: PyEval_RestoreThread()
   single: PyEval_SaveThread()

Here is how these functions work: the global interpreter lock is used to protect the pointer to the
current thread state.  When releasing the lock and saving the thread state,
the current thread state pointer must be retrieved before the lock is released
(since another thread could immediately acquire the lock and store its own thread
state in the global variable). Conversely, when acquiring the lock and restoring
the thread state, the lock must be acquired before storing the thread state
pointer.

.. note::
   Calling system I/O functions is the most common use case for releasing
   the GIL, but it can also be useful before calling long-running computations
   which don't need access to Python objects, such as compression or
   cryptographic functions operating over memory buffers.  For example, the
   standard :mod:`zlib` and :mod:`hashlib` modules release the GIL when
   compressing or hashing data.


.. _gilstate:

Non-Python created threads
--------------------------

When threads are created using the dedicated Python APIs (such as the
:mod:`threading` module), a thread state is automatically associated to them
and the code showed above is therefore correct.  However, when threads are
created from C (for example by a third-party library with its own thread
management), they don't hold the GIL, nor is there a thread state structure
for them.

If you need to call Python code from these threads (often this will be part
of a callback API provided by the aforementioned third-party library),
you must first register these threads with the interpreter by
creating a thread state data structure, then acquiring the GIL, and finally
storing their thread state pointer, before you can start using the Python/C
API.  When you are done, you should reset the thread state pointer, release
the GIL, and finally free the thread state data structure.

The :c:func:`PyGILState_Ensure` and :c:func:`PyGILState_Release` functions do
all of the above automatically.  The typical idiom for calling into Python
from a C thread is::

   PyGILState_STATE gstate;
   gstate = PyGILState_Ensure();

   /* Perform Python actions here. */
   result = CallSomeFunction();
   /* evaluate result or handle exception */

   /* Release the thread. No Python API allowed beyond this point. */
   PyGILState_Release(gstate);

Note that the :c:func:`PyGILState_\*` functions assume there is only one global
interpreter (created automatically by :c:func:`Py_Initialize`).  Python
supports the creation of additional interpreters (using
:c:func:`Py_NewInterpreter`), but mixing multiple interpreters and the
:c:func:`PyGILState_\*` API is unsupported.

Another important thing to note about threads is their behaviour in the face
of the C :c:func:`fork` call. On most systems with :c:func:`fork`, after a
process forks only the thread that issued the fork will exist. That also
means any locks held by other threads will never be released. Python solves
this for :func:`os.fork` by acquiring the locks it uses internally before
the fork, and releasing them afterwards. In addition, it resets any
:ref:`lock-objects` in the child. When extending or embedding Python, there
is no way to inform Python of additional (non-Python) locks that need to be
acquired before or reset after a fork. OS facilities such as
:c:func:`pthread_atfork` would need to be used to accomplish the same thing.
Additionally, when extending or embedding Python, calling :c:func:`fork`
directly rather than through :func:`os.fork` (and returning to or calling
into Python) may result in a deadlock by one of Python's internal locks
being held by a thread that is defunct after the fork.
:c:func:`PyOS_AfterFork` tries to reset the necessary locks, but is not
always able to.


High-level API
--------------

These are the most commonly used types and functions when writing C extension
code, or when embedding the Python interpreter:

.. c:type:: PyInterpreterState

   This data structure represents the state shared by a number of cooperating
   threads.  Threads belonging to the same interpreter share their module
   administration and a few other internal items. There are no public members in
   this structure.

   Threads belonging to different interpreters initially share nothing, except
   process state like available memory, open file descriptors and such.  The global
   interpreter lock is also shared by all threads, regardless of to which
   interpreter they belong.


.. c:type:: PyThreadState

   This data structure represents the state of a single thread.  The only public
   data member is :c:type:`PyInterpreterState \*`:attr:`interp`, which points to
   this thread's interpreter state.


.. c:function:: void PyEval_InitThreads()

   .. index::
      single: PyEval_ReleaseLock()
      single: PyEval_ReleaseThread()
      single: PyEval_SaveThread()
      single: PyEval_RestoreThread()

   Initialize and acquire the global interpreter lock.  It should be called in the
   main thread before creating a second thread or engaging in any other thread
   operations such as :c:func:`PyEval_ReleaseLock` or
   ``PyEval_ReleaseThread(tstate)``. It is not needed before calling
   :c:func:`PyEval_SaveThread` or :c:func:`PyEval_RestoreThread`.

   .. index:: single: Py_Initialize()

   This is a no-op when called for a second time.  It is safe to call this function
   before calling :c:func:`Py_Initialize`.

   .. index:: module: thread

   .. note::

      When only the main thread exists, no GIL operations are needed. This is a
      common situation (most Python programs do not use threads), and the lock
      operations slow the interpreter down a bit. Therefore, the lock is not
      created initially.  This situation is equivalent to having acquired the lock:
      when there is only a single thread, all object accesses are safe.  Therefore,
      when this function initializes the global interpreter lock, it also acquires
      it.  Before the Python :mod:`_thread` module creates a new thread, knowing
      that either it has the lock or the lock hasn't been created yet, it calls
      :c:func:`PyEval_InitThreads`.  When this call returns, it is guaranteed that
      the lock has been created and that the calling thread has acquired it.

      It is **not** safe to call this function when it is unknown which thread (if
      any) currently has the global interpreter lock.

      This function is not available when thread support is disabled at compile time.


.. c:function:: int PyEval_ThreadsInitialized()

   Returns a non-zero value if :c:func:`PyEval_InitThreads` has been called.  This
   function can be called without holding the GIL, and therefore can be used to
   avoid calls to the locking API when running single-threaded.  This function is
   not available when thread support is disabled at compile time.

   .. versionadded:: 2.4


.. c:function:: PyThreadState* PyEval_SaveThread()

   Release the global interpreter lock (if it has been created and thread
   support is enabled) and reset the thread state to *NULL*, returning the
   previous thread state (which is not *NULL*).  If the lock has been created,
   the current thread must have acquired it.  (This function is available even
   when thread support is disabled at compile time.)


.. c:function:: void PyEval_RestoreThread(PyThreadState *tstate)

   Acquire the global interpreter lock (if it has been created and thread
   support is enabled) and set the thread state to *tstate*, which must not be
   *NULL*.  If the lock has been created, the current thread must not have
   acquired it, otherwise deadlock ensues.  (This function is available even
   when thread support is disabled at compile time.)


.. c:function:: PyThreadState* PyThreadState_Get()

   Return the current thread state.  The global interpreter lock must be held.
   When the current thread state is *NULL*, this issues a fatal error (so that
   the caller needn't check for *NULL*).


.. c:function:: PyThreadState* PyThreadState_Swap(PyThreadState *tstate)

   Swap the current thread state with the thread state given by the argument
   *tstate*, which may be *NULL*.  The global interpreter lock must be held
   and is not released.


.. c:function:: void PyEval_ReInitThreads()

   This function is called from :c:func:`PyOS_AfterFork` to ensure that newly
   created child processes don't hold locks referring to threads which
   are not running in the child process.


The following functions use thread-local storage, and are not compatible
with sub-interpreters:

.. c:function:: PyGILState_STATE PyGILState_Ensure()

   Ensure that the current thread is ready to call the Python C API regardless
   of the current state of Python, or of the global interpreter lock. This may
   be called as many times as desired by a thread as long as each call is
   matched with a call to :c:func:`PyGILState_Release`. In general, other
   thread-related APIs may be used between :c:func:`PyGILState_Ensure` and
   :c:func:`PyGILState_Release` calls as long as the thread state is restored to
   its previous state before the Release().  For example, normal usage of the
   :c:macro:`Py_BEGIN_ALLOW_THREADS` and :c:macro:`Py_END_ALLOW_THREADS` macros is
   acceptable.

   The return value is an opaque "handle" to the thread state when
   :c:func:`PyGILState_Ensure` was called, and must be passed to
   :c:func:`PyGILState_Release` to ensure Python is left in the same state. Even
   though recursive calls are allowed, these handles *cannot* be shared - each
   unique call to :c:func:`PyGILState_Ensure` must save the handle for its call
   to :c:func:`PyGILState_Release`.

   When the function returns, the current thread will hold the GIL and be able
   to call arbitrary Python code.  Failure is a fatal error.

   .. versionadded:: 2.3


.. c:function:: void PyGILState_Release(PyGILState_STATE)

   Release any resources previously acquired.  After this call, Python's state will
   be the same as it was prior to the corresponding :c:func:`PyGILState_Ensure` call
   (but generally this state will be unknown to the caller, hence the use of the
   GILState API).

   Every call to :c:func:`PyGILState_Ensure` must be matched by a call to
   :c:func:`PyGILState_Release` on the same thread.

   .. versionadded:: 2.3


.. c:function:: PyThreadState* PyGILState_GetThisThreadState()

   Get the current thread state for this thread.  May return ``NULL`` if no
   GILState API has been used on the current thread.  Note that the main thread
   always has such a thread-state, even if no auto-thread-state call has been
   made on the main thread.  This is mainly a helper/diagnostic function.

   .. versionadded:: 2.3


The following macros are normally used without a trailing semicolon; look for
example usage in the Python source distribution.


.. c:macro:: Py_BEGIN_ALLOW_THREADS

   This macro expands to ``{ PyThreadState *_save; _save = PyEval_SaveThread();``.
   Note that it contains an opening brace; it must be matched with a following
   :c:macro:`Py_END_ALLOW_THREADS` macro.  See above for further discussion of this
   macro.  It is a no-op when thread support is disabled at compile time.


.. c:macro:: Py_END_ALLOW_THREADS

   This macro expands to ``PyEval_RestoreThread(_save); }``. Note that it contains
   a closing brace; it must be matched with an earlier
   :c:macro:`Py_BEGIN_ALLOW_THREADS` macro.  See above for further discussion of
   this macro.  It is a no-op when thread support is disabled at compile time.


.. c:macro:: Py_BLOCK_THREADS

   This macro expands to ``PyEval_RestoreThread(_save);``: it is equivalent to
   :c:macro:`Py_END_ALLOW_THREADS` without the closing brace.  It is a no-op when
   thread support is disabled at compile time.


.. c:macro:: Py_UNBLOCK_THREADS

   This macro expands to ``_save = PyEval_SaveThread();``: it is equivalent to
   :c:macro:`Py_BEGIN_ALLOW_THREADS` without the opening brace and variable
   declaration.  It is a no-op when thread support is disabled at compile time.


Low-level API
-------------

All of the following functions are only available when thread support is enabled
at compile time, and must be called only when the global interpreter lock has
been created.


.. c:function:: PyInterpreterState* PyInterpreterState_New()

   Create a new interpreter state object.  The global interpreter lock need not
   be held, but may be held if it is necessary to serialize calls to this
   function.


.. c:function:: void PyInterpreterState_Clear(PyInterpreterState *interp)

   Reset all information in an interpreter state object.  The global interpreter
   lock must be held.


.. c:function:: void PyInterpreterState_Delete(PyInterpreterState *interp)

   Destroy an interpreter state object.  The global interpreter lock need not be
   held.  The interpreter state must have been reset with a previous call to
   :c:func:`PyInterpreterState_Clear`.


.. c:function:: PyThreadState* PyThreadState_New(PyInterpreterState *interp)

   Create a new thread state object belonging to the given interpreter object.
   The global interpreter lock need not be held, but may be held if it is
   necessary to serialize calls to this function.


.. c:function:: void PyThreadState_Clear(PyThreadState *tstate)

   Reset all information in a thread state object.  The global interpreter lock
   must be held.


.. c:function:: void PyThreadState_Delete(PyThreadState *tstate)

   Destroy a thread state object.  The global interpreter lock need not be held.
   The thread state must have been reset with a previous call to
   :c:func:`PyThreadState_Clear`.


.. c:function:: PyObject* PyThreadState_GetDict()

   Return a dictionary in which extensions can store thread-specific state
   information.  Each extension should use a unique key to use to store state in
   the dictionary.  It is okay to call this function when no current thread state
   is available. If this function returns *NULL*, no exception has been raised and
   the caller should assume no current thread state is available.

   .. versionchanged:: 2.3
      Previously this could only be called when a current thread is active, and *NULL*
      meant that an exception was raised.


.. c:function:: int PyThreadState_SetAsyncExc(long id, PyObject *exc)

   Asynchronously raise an exception in a thread. The *id* argument is the thread
   id of the target thread; *exc* is the exception object to be raised. This
   function does not steal any references to *exc*. To prevent naive misuse, you
   must write your own C extension to call this.  Must be called with the GIL held.
   Returns the number of thread states modified; this is normally one, but will be
   zero if the thread id isn't found.  If *exc* is :const:`NULL`, the pending
   exception (if any) for the thread is cleared. This raises no exceptions.

   .. versionadded:: 2.3


.. c:function:: void PyEval_AcquireThread(PyThreadState *tstate)

   Acquire the global interpreter lock and set the current thread state to
   *tstate*, which should not be *NULL*.  The lock must have been created earlier.
   If this thread already has the lock, deadlock ensues.

   :c:func:`PyEval_RestoreThread` is a higher-level function which is always
   available (even when thread support isn't enabled or when threads have
   not been initialized).


.. c:function:: void PyEval_ReleaseThread(PyThreadState *tstate)

   Reset the current thread state to *NULL* and release the global interpreter
   lock.  The lock must have been created earlier and must be held by the current
   thread.  The *tstate* argument, which must not be *NULL*, is only used to check
   that it represents the current thread state --- if it isn't, a fatal error is
   reported.

   :c:func:`PyEval_SaveThread` is a higher-level function which is always
   available (even when thread support isn't enabled or when threads have
   not been initialized).


.. c:function:: void PyEval_AcquireLock()

   Acquire the global interpreter lock.  The lock must have been created earlier.
   If this thread already has the lock, a deadlock ensues.

   .. warning::
      This function does not change the current thread state.  Please use
      :c:func:`PyEval_RestoreThread` or :c:func:`PyEval_AcquireThread`
      instead.


.. c:function:: void PyEval_ReleaseLock()

   Release the global interpreter lock.  The lock must have been created earlier.

   .. warning::
      This function does not change the current thread state.  Please use
      :c:func:`PyEval_SaveThread` or :c:func:`PyEval_ReleaseThread`
      instead.


Sub-interpreter support
=======================

While in most uses, you will only embed a single Python interpreter, there
are cases where you need to create several independent interpreters in the
same process and perhaps even in the same thread.  Sub-interpreters allow
you to do that.  You can switch between sub-interpreters using the
:c:func:`PyThreadState_Swap` function.  You can create and destroy them
using the following functions:


.. c:function:: PyThreadState* Py_NewInterpreter()

   .. index::
      module: builtins
      module: __main__
      module: sys
      single: stdout (in module sys)
      single: stderr (in module sys)
      single: stdin (in module sys)

   Create a new sub-interpreter.  This is an (almost) totally separate environment
   for the execution of Python code.  In particular, the new interpreter has
   separate, independent versions of all imported modules, including the
   fundamental modules :mod:`builtins`, :mod:`__main__` and :mod:`sys`.  The
   table of loaded modules (``sys.modules``) and the module search path
   (``sys.path``) are also separate.  The new environment has no ``sys.argv``
   variable.  It has new standard I/O stream file objects ``sys.stdin``,
   ``sys.stdout`` and ``sys.stderr`` (however these refer to the same underlying
   file descriptors).

   The return value points to the first thread state created in the new
   sub-interpreter.  This thread state is made in the current thread state.
   Note that no actual thread is created; see the discussion of thread states
   below.  If creation of the new interpreter is unsuccessful, *NULL* is
   returned; no exception is set since the exception state is stored in the
   current thread state and there may not be a current thread state.  (Like all
   other Python/C API functions, the global interpreter lock must be held before
   calling this function and is still held when it returns; however, unlike most
   other Python/C API functions, there needn't be a current thread state on
   entry.)

   .. index::
      single: Py_Finalize()
      single: Py_Initialize()

   Extension modules are shared between (sub-)interpreters as follows: the first
   time a particular extension is imported, it is initialized normally, and a
   (shallow) copy of its module's dictionary is squirreled away.  When the same
   extension is imported by another (sub-)interpreter, a new module is initialized
   and filled with the contents of this copy; the extension's ``init`` function is
   not called.  Note that this is different from what happens when an extension is
   imported after the interpreter has been completely re-initialized by calling
   :c:func:`Py_Finalize` and :c:func:`Py_Initialize`; in that case, the extension's
   ``initmodule`` function *is* called again.

   .. index:: single: close() (in module os)


.. c:function:: void Py_EndInterpreter(PyThreadState *tstate)

   .. index:: single: Py_Finalize()

   Destroy the (sub-)interpreter represented by the given thread state. The given
   thread state must be the current thread state.  See the discussion of thread
   states below.  When the call returns, the current thread state is *NULL*.  All
   thread states associated with this interpreter are destroyed.  (The global
   interpreter lock must be held before calling this function and is still held
   when it returns.)  :c:func:`Py_Finalize` will destroy all sub-interpreters that
   haven't been explicitly destroyed at that point.


Bugs and caveats
----------------

Because sub-interpreters (and the main interpreter) are part of the same
process, the insulation between them isn't perfect --- for example, using
low-level file operations like  :func:`os.close` they can
(accidentally or maliciously) affect each other's open files.  Because of the
way extensions are shared between (sub-)interpreters, some extensions may not
work properly; this is especially likely when the extension makes use of
(static) global variables, or when the extension manipulates its module's
dictionary after its initialization.  It is possible to insert objects created
in one sub-interpreter into a namespace of another sub-interpreter; this should
be done with great care to avoid sharing user-defined functions, methods,
instances or classes between sub-interpreters, since import operations executed
by such objects may affect the wrong (sub-)interpreter's dictionary of loaded
modules.

Also note that combining this functionality with :c:func:`PyGILState_\*` APIs
is delicate, because these APIs assume a bijection between Python thread states
and OS-level threads, an assumption broken by the presence of sub-interpreters.
It is highly recommended that you don't switch sub-interpreters between a pair
of matching :c:func:`PyGILState_Ensure` and :c:func:`PyGILState_Release` calls.
Furthermore, extensions (such as :mod:`ctypes`) using these APIs to allow calling
of Python code from non-Python created threads will probably be broken when using
sub-interpreters.


Asynchronous Notifications
==========================

A mechanism is provided to make asynchronous notifications to the main
interpreter thread.  These notifications take the form of a function
pointer and a void pointer argument.


.. c:function:: int Py_AddPendingCall(int (*func)(void *), void *arg)

   .. index:: single: Py_AddPendingCall()

   Schedule a function to be called from the main interpreter thread.  On
   success, 0 is returned and *func* is queued for being called in the
   main thread.  On failure, -1 is returned without setting any exception.

   When successfully queued, *func* will be *eventually* called from the
   main interpreter thread with the argument *arg*.  It will be called
   asynchronously with respect to normally running Python code, but with
   both these conditions met:

   * on a :term:`bytecode` boundary;
   * with the main thread holding the :term:`global interpreter lock`
     (*func* can therefore use the full C API).

   *func* must return 0 on success, or -1 on failure with an exception
   set.  *func* won't be interrupted to perform another asynchronous
   notification recursively, but it can still be interrupted to switch
   threads if the global interpreter lock is released.

   This function doesn't need a current thread state to run, and it doesn't
   need the global interpreter lock.

   .. warning::
      This is a low-level function, only useful for very special cases.
      There is no guarantee that *func* will be called as quick as
      possible.  If the main thread is busy executing a system call,
      *func* won't be called before the system call returns.  This
      function is generally **not** suitable for calling Python code from
      arbitrary C threads.  Instead, use the :ref:`PyGILState API<gilstate>`.

   .. versionadded:: 2.7


.. _profiling:

Profiling and Tracing
=====================

.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>


The Python interpreter provides some low-level support for attaching profiling
and execution tracing facilities.  These are used for profiling, debugging, and
coverage analysis tools.

Starting with Python 2.2, the implementation of this facility was substantially
revised, and an interface from C was added.  This C interface allows the
profiling or tracing code to avoid the overhead of calling through Python-level
callable objects, making a direct C function call instead.  The essential
attributes of the facility have not changed; the interface allows trace
functions to be installed per-thread, and the basic events reported to the trace
function are the same as had been reported to the Python-level trace functions
in previous versions.


.. c:type:: int (*Py_tracefunc)(PyObject *obj, PyFrameObject *frame, int what, PyObject *arg)

   The type of the trace function registered using :c:func:`PyEval_SetProfile` and
   :c:func:`PyEval_SetTrace`. The first parameter is the object passed to the
   registration function as *obj*, *frame* is the frame object to which the event
   pertains, *what* is one of the constants :const:`PyTrace_CALL`,
   :const:`PyTrace_EXCEPTION`, :const:`PyTrace_LINE`, :const:`PyTrace_RETURN`,
   :const:`PyTrace_C_CALL`, :const:`PyTrace_C_EXCEPTION`, or
   :const:`PyTrace_C_RETURN`, and *arg* depends on the value of *what*:

   +------------------------------+--------------------------------------+
   | Value of *what*              | Meaning of *arg*                     |
   +==============================+======================================+
   | :const:`PyTrace_CALL`        | Always *NULL*.                       |
   +------------------------------+--------------------------------------+
   | :const:`PyTrace_EXCEPTION`   | Exception information as returned by |
   |                              | :func:`sys.exc_info`.                |
   +------------------------------+--------------------------------------+
   | :const:`PyTrace_LINE`        | Always *NULL*.                       |
   +------------------------------+--------------------------------------+
   | :const:`PyTrace_RETURN`      | Value being returned to the caller,  |
   |                              | or *NULL* if caused by an exception. |
   +------------------------------+--------------------------------------+
   | :const:`PyTrace_C_CALL`      | Function object being called.        |
   +------------------------------+--------------------------------------+
   | :const:`PyTrace_C_EXCEPTION` | Function object being called.        |
   +------------------------------+--------------------------------------+
   | :const:`PyTrace_C_RETURN`    | Function object being called.        |
   +------------------------------+--------------------------------------+


.. c:var:: int PyTrace_CALL

   The value of the *what* parameter to a :c:type:`Py_tracefunc` function when a new
   call to a function or method is being reported, or a new entry into a generator.
   Note that the creation of the iterator for a generator function is not reported
   as there is no control transfer to the Python bytecode in the corresponding
   frame.


.. c:var:: int PyTrace_EXCEPTION

   The value of the *what* parameter to a :c:type:`Py_tracefunc` function when an
   exception has been raised.  The callback function is called with this value for
   *what* when after any bytecode is processed after which the exception becomes
   set within the frame being executed.  The effect of this is that as exception
   propagation causes the Python stack to unwind, the callback is called upon
   return to each frame as the exception propagates.  Only trace functions receives
   these events; they are not needed by the profiler.


.. c:var:: int PyTrace_LINE

   The value passed as the *what* parameter to a trace function (but not a
   profiling function) when a line-number event is being reported.


.. c:var:: int PyTrace_RETURN

   The value for the *what* parameter to :c:type:`Py_tracefunc` functions when a
   call is returning without propagating an exception.


.. c:var:: int PyTrace_C_CALL

   The value for the *what* parameter to :c:type:`Py_tracefunc` functions when a C
   function is about to be called.


.. c:var:: int PyTrace_C_EXCEPTION

   The value for the *what* parameter to :c:type:`Py_tracefunc` functions when a C
   function has raised an exception.


.. c:var:: int PyTrace_C_RETURN

   The value for the *what* parameter to :c:type:`Py_tracefunc` functions when a C
   function has returned.


.. c:function:: void PyEval_SetProfile(Py_tracefunc func, PyObject *obj)

   Set the profiler function to *func*.  The *obj* parameter is passed to the
   function as its first parameter, and may be any Python object, or *NULL*.  If
   the profile function needs to maintain state, using a different value for *obj*
   for each thread provides a convenient and thread-safe place to store it.  The
   profile function is called for all monitored events except the line-number
   events.


.. c:function:: void PyEval_SetTrace(Py_tracefunc func, PyObject *obj)

   Set the tracing function to *func*.  This is similar to
   :c:func:`PyEval_SetProfile`, except the tracing function does receive line-number
   events.

.. c:function:: PyObject* PyEval_GetCallStats(PyObject *self)

   Return a tuple of function call counts.  There are constants defined for the
   positions within the tuple:

   +-------------------------------+-------+
   | Name                          | Value |
   +===============================+=======+
   | :const:`PCALL_ALL`            | 0     |
   +-------------------------------+-------+
   | :const:`PCALL_FUNCTION`       | 1     |
   +-------------------------------+-------+
   | :const:`PCALL_FAST_FUNCTION`  | 2     |
   +-------------------------------+-------+
   | :const:`PCALL_FASTER_FUNCTION`| 3     |
   +-------------------------------+-------+
   | :const:`PCALL_METHOD`         | 4     |
   +-------------------------------+-------+
   | :const:`PCALL_BOUND_METHOD`   | 5     |
   +-------------------------------+-------+
   | :const:`PCALL_CFUNCTION`      | 6     |
   +-------------------------------+-------+
   | :const:`PCALL_TYPE`           | 7     |
   +-------------------------------+-------+
   | :const:`PCALL_GENERATOR`      | 8     |
   +-------------------------------+-------+
   | :const:`PCALL_OTHER`          | 9     |
   +-------------------------------+-------+
   | :const:`PCALL_POP`            | 10    |
   +-------------------------------+-------+

   :const:`PCALL_FAST_FUNCTION` means no argument tuple needs to be created.
   :const:`PCALL_FASTER_FUNCTION` means that the fast-path frame setup code is used.

   If there is a method call where the call can be optimized by changing
   the argument tuple and calling the function directly, it gets recorded
   twice.

   This function is only present if Python is compiled with :const:`CALL_PROFILE`
   defined.

.. _advanced-debugging:

Advanced Debugger Support
=========================

.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>


These functions are only intended to be used by advanced debugging tools.


.. c:function:: PyInterpreterState* PyInterpreterState_Head()

   Return the interpreter state object at the head of the list of all such objects.

   .. versionadded:: 2.2


.. c:function:: PyInterpreterState* PyInterpreterState_Next(PyInterpreterState *interp)

   Return the next interpreter state object after *interp* from the list of all
   such objects.

   .. versionadded:: 2.2


.. c:function:: PyThreadState * PyInterpreterState_ThreadHead(PyInterpreterState *interp)

   Return the pointer to the first :c:type:`PyThreadState` object in the list of
   threads associated with the interpreter *interp*.

   .. versionadded:: 2.2


.. c:function:: PyThreadState* PyThreadState_Next(PyThreadState *tstate)

   Return the next thread state object after *tstate* from the list of all such
   objects belonging to the same :c:type:`PyInterpreterState` object.

   .. versionadded:: 2.2

