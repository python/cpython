.. highlight:: c

.. _threads:

Thread States and the Global Interpreter Lock
=============================================

.. index::
   single: global interpreter lock
   single: interpreter lock
   single: lock, interpreter

Unless on a :term:`free-threaded <free threading>` build of :term:`CPython`,
the Python interpreter is not fully thread-safe.  In order to support
multi-threaded Python programs, there's a global lock, called the :term:`global
interpreter lock` or :term:`GIL`, that must be held by the current thread before
it can safely access Python objects. Without the lock, even the simplest
operations could cause problems in a multi-threaded program: for example, when
two threads simultaneously increment the reference count of the same object, the
reference count could end up being incremented only once instead of twice.

.. index:: single: setswitchinterval (in module sys)

Therefore, the rule exists that only the thread that has acquired the
:term:`GIL` may operate on Python objects or call Python/C API functions.
In order to emulate concurrency of execution, the interpreter regularly
tries to switch threads (see :func:`sys.setswitchinterval`).  The lock is also
released around potentially blocking I/O operations like reading or writing
a file, so that other Python threads can run in the meantime.

.. index::
   single: PyThreadState (C type)

The Python interpreter keeps some thread-specific bookkeeping information
inside a data structure called :c:type:`PyThreadState`, known as a :term:`thread state`.
Each OS thread has a thread-local pointer to a :c:type:`PyThreadState`; a thread state
referenced by this pointer is considered to be :term:`attached <attached thread state>`.

A thread can only have one :term:`attached thread state` at a time. An attached
thread state is typically analogous with holding the :term:`GIL`, except on
:term:`free-threaded <free threading>` builds.  On builds with the :term:`GIL` enabled,
:term:`attaching <attached thread state>` a thread state will block until the :term:`GIL`
can be acquired. However,  even on builds with the :term:`GIL` disabled, it is still required
to have an attached thread state to call most of the C API.

In general, there will always be an :term:`attached thread state` when using Python's C API.
Only in some specific cases (such as in a :c:macro:`Py_BEGIN_ALLOW_THREADS` block) will the
thread not have an attached thread state. If uncertain, check if :c:func:`PyThreadState_GetUnchecked` returns
``NULL``.

Detaching the thread state from extension code
----------------------------------------------

Most extension code manipulating the :term:`thread state` has the following simple
structure::

   Save the thread state in a local variable.
   ... Do some blocking I/O operation ...
   Restore the thread state from the local variable.

This is so common that a pair of macros exists to simplify it::

   Py_BEGIN_ALLOW_THREADS
   ... Do some blocking I/O operation ...
   Py_END_ALLOW_THREADS

.. index::
   single: Py_BEGIN_ALLOW_THREADS (C macro)
   single: Py_END_ALLOW_THREADS (C macro)

The :c:macro:`Py_BEGIN_ALLOW_THREADS` macro opens a new block and declares a
hidden local variable; the :c:macro:`Py_END_ALLOW_THREADS` macro closes the
block.

The block above expands to the following code::

   PyThreadState *_save;

   _save = PyEval_SaveThread();
   ... Do some blocking I/O operation ...
   PyEval_RestoreThread(_save);

.. index::
   single: PyEval_RestoreThread (C function)
   single: PyEval_SaveThread (C function)

Here is how these functions work:

The :term:`attached thread state` holds the :term:`GIL` for the entire interpreter. When detaching
the :term:`attached thread state`, the :term:`GIL` is released, allowing other threads to attach
a thread state to their own thread, thus getting the :term:`GIL` and can start executing.
The pointer to the prior :term:`attached thread state` is stored as a local variable.
Upon reaching :c:macro:`Py_END_ALLOW_THREADS`, the thread state that was
previously :term:`attached <attached thread state>` is passed to :c:func:`PyEval_RestoreThread`.
This function will block until another releases its :term:`thread state <attached thread state>`,
thus allowing the old :term:`thread state <attached thread state>` to get re-attached and the
C API can be called again.

For :term:`free-threaded <free threading>` builds, the :term:`GIL` is normally
out of the question, but detaching the :term:`thread state <attached thread state>` is still required
for blocking I/O and long operations. The difference is that threads don't have to wait for the :term:`GIL`
to be released to attach their thread state, allowing true multi-core parallelism.

.. note::
   Calling system I/O functions is the most common use case for detaching
   the :term:`thread state <attached thread state>`, but it can also be useful before calling
   long-running computations which don't need access to Python objects, such
   as compression or cryptographic functions operating over memory buffers.
   For example, the standard :mod:`zlib` and :mod:`hashlib` modules detach the
   :term:`thread state <attached thread state>` when compressing or hashing data.


.. _gilstate:

Non-Python created threads
--------------------------

When threads are created using the dedicated Python APIs (such as the
:mod:`threading` module), a thread state is automatically associated to them
and the code showed above is therefore correct.  However, when threads are
created from C (for example by a third-party library with its own thread
management), they don't hold the :term:`GIL`, because they don't have an
:term:`attached thread state`.

If you need to call Python code from these threads (often this will be part
of a callback API provided by the aforementioned third-party library),
you must first register these threads with the interpreter by
creating an :term:`attached thread state` before you can start using the Python/C
API.  When you are done, you should detach the :term:`thread state <attached thread state>`, and
finally free it.

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

Note that the ``PyGILState_*`` functions assume there is only one global
interpreter (created automatically by :c:func:`Py_Initialize`).  Python
supports the creation of additional interpreters (using
:c:func:`Py_NewInterpreter`), but mixing multiple interpreters and the
``PyGILState_*`` API is unsupported. This is because :c:func:`PyGILState_Ensure`
and similar functions default to :term:`attaching <attached thread state>` a
:term:`thread state` for the main interpreter, meaning that the thread can't safely
interact with the calling subinterpreter.

Supporting subinterpreters in non-Python threads
------------------------------------------------

If you would like to support subinterpreters with non-Python created threads, you
must use the ``PyThreadState_*`` API instead of the traditional ``PyGILState_*``
API.

In particular, you must store the interpreter state from the calling
function and pass it to :c:func:`PyThreadState_New`, which will ensure that
the :term:`thread state` is targeting the correct interpreter::

   /* The return value of PyInterpreterState_Get() from the
      function that created this thread. */
   PyInterpreterState *interp = ThreadData->interp;
   PyThreadState *tstate = PyThreadState_New(interp);
   PyThreadState_Swap(tstate);

   /* GIL of the subinterpreter is now held.
      Perform Python actions here. */
   result = CallSomeFunction();
   /* evaluate result or handle exception */

   /* Destroy the thread state. No Python API allowed beyond this point. */
   PyThreadState_Clear(tstate);
   PyThreadState_DeleteCurrent();

.. _fork-and-threads:

Cautions about fork()
---------------------

Another important thing to note about threads is their behaviour in the face
of the C :c:func:`fork` call. On most systems with :c:func:`fork`, after a
process forks only the thread that issued the fork will exist.  This has a
concrete impact both on how locks must be handled and on all stored state
in CPython's runtime.

The fact that only the "current" thread remains
means any locks held by other threads will never be released. Python solves
this for :func:`os.fork` by acquiring the locks it uses internally before
the fork, and releasing them afterwards. In addition, it resets any
:ref:`lock-objects` in the child. When extending or embedding Python, there
is no way to inform Python of additional (non-Python) locks that need to be
acquired before or reset after a fork. OS facilities such as
:c:func:`!pthread_atfork` would need to be used to accomplish the same thing.
Additionally, when extending or embedding Python, calling :c:func:`fork`
directly rather than through :func:`os.fork` (and returning to or calling
into Python) may result in a deadlock by one of Python's internal locks
being held by a thread that is defunct after the fork.
:c:func:`PyOS_AfterFork_Child` tries to reset the necessary locks, but is not
always able to.

The fact that all other threads go away also means that CPython's
runtime state there must be cleaned up properly, which :func:`os.fork`
does.  This means finalizing all other :c:type:`PyThreadState` objects
belonging to the current interpreter and all other
:c:type:`PyInterpreterState` objects.  Due to this and the special
nature of the :ref:`"main" interpreter <sub-interpreter-support>`,
:c:func:`fork` should only be called in that interpreter's "main"
thread, where the CPython global runtime was originally initialized.
The only exception is if :c:func:`exec` will be called immediately
after.


High-level APIs
---------------

These are the most commonly used types and functions when writing multi-threaded
C extensions.


.. c:type:: PyThreadState

   This data structure represents the state of a single thread.  The only public
   data member is:

   .. c:member:: PyInterpreterState *interp

      This thread's interpreter state.


.. c:function:: void PyEval_InitThreads()

   .. index::
      single: PyEval_AcquireThread()
      single: PyEval_ReleaseThread()
      single: PyEval_SaveThread()
      single: PyEval_RestoreThread()

   Deprecated function which does nothing.

   In Python 3.6 and older, this function created the GIL if it didn't exist.

   .. versionchanged:: 3.9
      The function now does nothing.

   .. versionchanged:: 3.7
      This function is now called by :c:func:`Py_Initialize()`, so you don't
      have to call it yourself anymore.

   .. versionchanged:: 3.2
      This function cannot be called before :c:func:`Py_Initialize()` anymore.

   .. deprecated:: 3.9

   .. index:: pair: module; _thread


.. c:function:: PyThreadState* PyEval_SaveThread()

   Detach the :term:`attached thread state` and return it.
   The thread will have no :term:`thread state` upon returning.


.. c:function:: void PyEval_RestoreThread(PyThreadState *tstate)

   Set the :term:`attached thread state` to *tstate*.
   The passed :term:`thread state` **should not** be :term:`attached <attached thread state>`,
   otherwise deadlock ensues. *tstate* will be attached upon returning.

   .. note::
      Calling this function from a thread when the runtime is finalizing will
      hang the thread until the program exits, even if the thread was not
      created by Python.  Refer to
      :ref:`cautions-regarding-runtime-finalization` for more details.

   .. versionchanged:: 3.14
      Hangs the current thread, rather than terminating it, if called while the
      interpreter is finalizing.

.. c:function:: PyThreadState* PyThreadState_Get()

   Return the :term:`attached thread state`. If the thread has no attached
   thread state, (such as when inside of :c:macro:`Py_BEGIN_ALLOW_THREADS`
   block), then this issues a fatal error (so that the caller needn't check
   for ``NULL``).

   See also :c:func:`PyThreadState_GetUnchecked`.

.. c:function:: PyThreadState* PyThreadState_GetUnchecked()

   Similar to :c:func:`PyThreadState_Get`, but don't kill the process with a
   fatal error if it is NULL. The caller is responsible to check if the result
   is NULL.

   .. versionadded:: 3.13
      In Python 3.5 to 3.12, the function was private and known as
      ``_PyThreadState_UncheckedGet()``.


.. c:function:: PyThreadState* PyThreadState_Swap(PyThreadState *tstate)

   Set the :term:`attached thread state` to *tstate*, and return the
   :term:`thread state` that was attached prior to calling.

   This function is safe to call without an :term:`attached thread state`; it
   will simply return ``NULL`` indicating that there was no prior thread state.

   .. seealso::
      :c:func:`PyEval_ReleaseThread`

   .. note::
      Similar to :c:func:`PyGILState_Ensure`, this function will hang the
      thread if the runtime is finalizing.


Releasing the GIL
-----------------

The following macros are normally used without a trailing semicolon; look for
example usage in the Python source distribution.

.. note::

    These macros are still necessary on the :term:`free-threaded build` to prevent
    deadlocks.

.. c:macro:: Py_BEGIN_ALLOW_THREADS

   This macro expands to ``{ PyThreadState *_save; _save = PyEval_SaveThread();``.
   Note that it contains an opening brace; it must be matched with a following
   :c:macro:`Py_END_ALLOW_THREADS` macro.  See above for further discussion of this
   macro.


.. c:macro:: Py_END_ALLOW_THREADS

   This macro expands to ``PyEval_RestoreThread(_save); }``. Note that it contains
   a closing brace; it must be matched with an earlier
   :c:macro:`Py_BEGIN_ALLOW_THREADS` macro.  See above for further discussion of
   this macro.


.. c:macro:: Py_BLOCK_THREADS

   This macro expands to ``PyEval_RestoreThread(_save);``: it is equivalent to
   :c:macro:`Py_END_ALLOW_THREADS` without the closing brace.


.. c:macro:: Py_UNBLOCK_THREADS

   This macro expands to ``_save = PyEval_SaveThread();``: it is equivalent to
   :c:macro:`Py_BEGIN_ALLOW_THREADS` without the opening brace and variable
   declaration.

GIL-state APIs
--------------

The following functions use thread-local storage, and are not compatible
with sub-interpreters:

.. c:type:: PyGILState_STATE

   The type of the value returned by :c:func:`PyGILState_Ensure` and passed to
   :c:func:`PyGILState_Release`.

   .. c:enumerator:: PyGILState_LOCKED

      The GIL was already held when :c:func:`PyGILState_Ensure` was called.

   .. c:enumerator:: PyGILState_UNLOCKED

      The GIL was not held when :c:func:`PyGILState_Ensure` was called.

.. c:function:: PyGILState_STATE PyGILState_Ensure()

   Ensure that the current thread is ready to call the Python C API regardless
   of the current state of Python, or of the :term:`attached thread state`. This may
   be called as many times as desired by a thread as long as each call is
   matched with a call to :c:func:`PyGILState_Release`. In general, other
   thread-related APIs may be used between :c:func:`PyGILState_Ensure` and
   :c:func:`PyGILState_Release` calls as long as the thread state is restored to
   its previous state before the Release().  For example, normal usage of the
   :c:macro:`Py_BEGIN_ALLOW_THREADS` and :c:macro:`Py_END_ALLOW_THREADS` macros is
   acceptable.

   The return value is an opaque "handle" to the :term:`attached thread state` when
   :c:func:`PyGILState_Ensure` was called, and must be passed to
   :c:func:`PyGILState_Release` to ensure Python is left in the same state. Even
   though recursive calls are allowed, these handles *cannot* be shared - each
   unique call to :c:func:`PyGILState_Ensure` must save the handle for its call
   to :c:func:`PyGILState_Release`.

   When the function returns, there will be an :term:`attached thread state`
   and the thread will be able to call arbitrary Python code.  Failure is a fatal error.

   .. warning::
      Calling this function when the runtime is finalizing is unsafe. Doing
      so will either hang the thread until the program ends, or fully crash
      the interpreter in rare cases. Refer to
      :ref:`cautions-regarding-runtime-finalization` for more details.

   .. versionchanged:: 3.14
      Hangs the current thread, rather than terminating it, if called while the
      interpreter is finalizing.

.. c:function:: void PyGILState_Release(PyGILState_STATE)

   Release any resources previously acquired.  After this call, Python's state will
   be the same as it was prior to the corresponding :c:func:`PyGILState_Ensure` call
   (but generally this state will be unknown to the caller, hence the use of the
   GILState API).

   Every call to :c:func:`PyGILState_Ensure` must be matched by a call to
   :c:func:`PyGILState_Release` on the same thread.

.. c:function:: PyThreadState* PyGILState_GetThisThreadState()

   Get the :term:`attached thread state` for this thread.  May return ``NULL`` if no
   GILState API has been used on the current thread.  Note that the main thread
   always has such a thread-state, even if no auto-thread-state call has been
   made on the main thread.  This is mainly a helper/diagnostic function.

   .. note::
      This function may return non-``NULL`` even when the :term:`thread state`
      is detached.
      Prefer :c:func:`PyThreadState_Get` or :c:func:`PyThreadState_GetUnchecked`
      for most cases.

   .. seealso:: :c:func:`PyThreadState_Get`

.. c:function:: int PyGILState_Check()

   Return ``1`` if the current thread is holding the :term:`GIL` and ``0`` otherwise.
   This function can be called from any thread at any time.
   Only if it has had its :term:`thread state <attached thread state>` initialized
   via :c:func:`PyGILState_Ensure` will it return ``1``.
   This is mainly a helper/diagnostic function.  It can be useful
   for example in callback contexts or memory allocation functions when
   knowing that the :term:`GIL` is locked can allow the caller to perform sensitive
   actions or otherwise behave differently.

   .. note::
      If the current Python process has ever created a subinterpreter, this
      function will *always* return ``1``. Prefer :c:func:`PyThreadState_GetUnchecked`
      for most cases.

   .. versionadded:: 3.4


Low-level APIs
--------------

.. c:function:: PyThreadState* PyThreadState_New(PyInterpreterState *interp)

   Create a new thread state object belonging to the given interpreter object.
   An :term:`attached thread state` is not needed.

.. c:function:: void PyThreadState_Clear(PyThreadState *tstate)

   Reset all information in a :term:`thread state` object.  *tstate*
   must be :term:`attached <attached thread state>`

   .. versionchanged:: 3.9
      This function now calls the :c:member:`!PyThreadState.on_delete` callback.
      Previously, that happened in :c:func:`PyThreadState_Delete`.

   .. versionchanged:: 3.13
      The :c:member:`!PyThreadState.on_delete` callback was removed.


.. c:function:: void PyThreadState_Delete(PyThreadState *tstate)

   Destroy a :term:`thread state` object.  *tstate* should not
   be :term:`attached <attached thread state>` to any thread.
   *tstate* must have been reset with a previous call to
   :c:func:`PyThreadState_Clear`.


.. c:function:: void PyThreadState_DeleteCurrent(void)

   Detach the :term:`attached thread state` (which must have been reset
   with a previous call to :c:func:`PyThreadState_Clear`) and then destroy it.

   No :term:`thread state` will be :term:`attached <attached thread state>` upon
   returning.

.. c:function:: PyFrameObject* PyThreadState_GetFrame(PyThreadState *tstate)

   Get the current frame of the Python thread state *tstate*.

   Return a :term:`strong reference`. Return ``NULL`` if no frame is currently
   executing.

   See also :c:func:`PyEval_GetFrame`.

   *tstate* must not be ``NULL``, and must be :term:`attached <attached thread state>`.

   .. versionadded:: 3.9


.. c:function:: uint64_t PyThreadState_GetID(PyThreadState *tstate)

   Get the unique :term:`thread state` identifier of the Python thread state *tstate*.

   *tstate* must not be ``NULL``, and must be :term:`attached <attached thread state>`.

   .. versionadded:: 3.9


.. c:function:: PyInterpreterState* PyThreadState_GetInterpreter(PyThreadState *tstate)

   Get the interpreter of the Python thread state *tstate*.

   *tstate* must not be ``NULL``, and must be :term:`attached <attached thread state>`.

   .. versionadded:: 3.9


.. c:function:: void PyThreadState_EnterTracing(PyThreadState *tstate)

   Suspend tracing and profiling in the Python thread state *tstate*.

   Resume them using the :c:func:`PyThreadState_LeaveTracing` function.

   .. versionadded:: 3.11


.. c:function:: void PyThreadState_LeaveTracing(PyThreadState *tstate)

   Resume tracing and profiling in the Python thread state *tstate* suspended
   by the :c:func:`PyThreadState_EnterTracing` function.

   See also :c:func:`PyEval_SetTrace` and :c:func:`PyEval_SetProfile`
   functions.

   .. versionadded:: 3.11


.. c:function:: int PyUnstable_ThreadState_SetStackProtection(PyThreadState *tstate, void *stack_start_addr, size_t stack_size)

   Set the stack protection start address and stack protection size
   of a Python thread state.

   On success, return ``0``.
   On failure, set an exception and return ``-1``.

   CPython implements :ref:`recursion control <recursion>` for C code by raising
   :py:exc:`RecursionError` when it notices that the machine execution stack is close
   to overflow. See for example the :c:func:`Py_EnterRecursiveCall` function.
   For this, it needs to know the location of the current thread's stack, which it
   normally gets from the operating system.
   When the stack is changed, for example using context switching techniques like the
   Boost library's ``boost::context``, you must call
   :c:func:`~PyUnstable_ThreadState_SetStackProtection` to inform CPython of the change.

   Call :c:func:`~PyUnstable_ThreadState_SetStackProtection` either before
   or after changing the stack.
   Do not call any other Python C API between the call and the stack
   change.

   See :c:func:`PyUnstable_ThreadState_ResetStackProtection` for undoing this operation.

   .. versionadded:: 3.15


.. c:function:: void PyUnstable_ThreadState_ResetStackProtection(PyThreadState *tstate)

   Reset the stack protection start address and stack protection size
   of a Python thread state to the operating system defaults.

   See :c:func:`PyUnstable_ThreadState_SetStackProtection` for an explanation.

   .. versionadded:: 3.15


.. c:function:: PyObject* PyThreadState_GetDict()

   Return a dictionary in which extensions can store thread-specific state
   information.  Each extension should use a unique key to use to store state in
   the dictionary.  It is okay to call this function when no :term:`thread state`
   is :term:`attached <attached thread state>`. If this function returns
   ``NULL``, no exception has been raised and the caller should assume no
   thread state is attached.


.. c:function:: void PyEval_AcquireThread(PyThreadState *tstate)

   :term:`Attach <attached thread state>` *tstate* to the current thread,
   which must not be ``NULL`` or already :term:`attached <attached thread state>`.

   The calling thread must not already have an :term:`attached thread state`.

   .. note::
      Calling this function from a thread when the runtime is finalizing will
      hang the thread until the program exits, even if the thread was not
      created by Python.  Refer to
      :ref:`cautions-regarding-runtime-finalization` for more details.

   .. versionchanged:: 3.8
      Updated to be consistent with :c:func:`PyEval_RestoreThread`,
      :c:func:`Py_END_ALLOW_THREADS`, and :c:func:`PyGILState_Ensure`,
      and terminate the current thread if called while the interpreter is finalizing.

   .. versionchanged:: 3.14
      Hangs the current thread, rather than terminating it, if called while the
      interpreter is finalizing.

   :c:func:`PyEval_RestoreThread` is a higher-level function which is always
   available (even when threads have not been initialized).


.. c:function:: void PyEval_ReleaseThread(PyThreadState *tstate)

   Detach the :term:`attached thread state`.
   The *tstate* argument, which must not be ``NULL``, is only used to check
   that it represents the :term:`attached thread state` --- if it isn't, a fatal error is
   reported.

   :c:func:`PyEval_SaveThread` is a higher-level function which is always
   available (even when threads have not been initialized).


Asynchronous Notifications
--------------------------

A mechanism is provided to make asynchronous notifications to the main
interpreter thread.  These notifications take the form of a function
pointer and a void pointer argument.


.. c:function:: int Py_AddPendingCall(int (*func)(void *), void *arg)

   Schedule a function to be called from the main interpreter thread.  On
   success, ``0`` is returned and *func* is queued for being called in the
   main thread.  On failure, ``-1`` is returned without setting any exception.

   When successfully queued, *func* will be *eventually* called from the
   main interpreter thread with the argument *arg*.  It will be called
   asynchronously with respect to normally running Python code, but with
   both these conditions met:

   * on a :term:`bytecode` boundary;
   * with the main thread holding an :term:`attached thread state`
     (*func* can therefore use the full C API).

   *func* must return ``0`` on success, or ``-1`` on failure with an exception
   set.  *func* won't be interrupted to perform another asynchronous
   notification recursively, but it can still be interrupted to switch
   threads if the :term:`thread state <attached thread state>` is detached.

   This function doesn't need an :term:`attached thread state`. However, to call this
   function in a subinterpreter, the caller must have an :term:`attached thread state`.
   Otherwise, the function *func* can be scheduled to be called from the wrong interpreter.

   .. warning::
      This is a low-level function, only useful for very special cases.
      There is no guarantee that *func* will be called as quick as
      possible.  If the main thread is busy executing a system call,
      *func* won't be called before the system call returns.  This
      function is generally **not** suitable for calling Python code from
      arbitrary C threads.  Instead, use the :ref:`PyGILState API<gilstate>`.

   .. versionadded:: 3.1

   .. versionchanged:: 3.9
      If this function is called in a subinterpreter, the function *func* is
      now scheduled to be called from the subinterpreter, rather than being
      called from the main interpreter. Each subinterpreter now has its own
      list of scheduled calls.

   .. versionchanged:: 3.12
      This function now always schedules *func* to be run in the main
      interpreter.


.. c:function:: int Py_MakePendingCalls(void)

   Execute all pending calls. This is usually executed automatically by the
   interpreter.

   This function returns ``0`` on success, and returns ``-1`` with an exception
   set on failure.

   If this is not called in the main thread of the main
   interpreter, this function does nothing and returns ``0``.
   The caller must hold an :term:`attached thread state`.

   .. versionadded:: 3.1

   .. versionchanged:: 3.12
      This function only runs pending calls in the main interpreter.


.. c:function:: int PyThreadState_SetAsyncExc(unsigned long id, PyObject *exc)

   Asynchronously raise an exception in a thread. The *id* argument is the thread
   id of the target thread; *exc* is the exception object to be raised. This
   function does not steal any references to *exc*. To prevent naive misuse, you
   must write your own C extension to call this.  Must be called with an :term:`attached thread state`.
   Returns the number of thread states modified; this is normally one, but will be
   zero if the thread id isn't found.  If *exc* is ``NULL``, the pending
   exception (if any) for the thread is cleared. This raises no exceptions.

   .. versionchanged:: 3.7
      The type of the *id* parameter changed from :c:expr:`long` to
      :c:expr:`unsigned long`.

.. _thread-local-storage:

Thread Local Storage Support
============================

The Python interpreter provides low-level support for thread-local storage
(TLS) which wraps the underlying native TLS implementation to support the
Python-level thread local storage API (:class:`threading.local`).  The
CPython C level APIs are similar to those offered by pthreads and Windows:
use a thread key and functions to associate a :c:expr:`void*` value per
thread.

A :term:`thread state` does *not* need to be :term:`attached <attached thread state>`
when calling these functions; they supply their own locking.

Note that :file:`Python.h` does not include the declaration of the TLS APIs,
you need to include :file:`pythread.h` to use thread-local storage.

.. note::
   None of these API functions handle memory management on behalf of the
   :c:expr:`void*` values.  You need to allocate and deallocate them yourself.
   If the :c:expr:`void*` values happen to be :c:expr:`PyObject*`, these
   functions don't do refcount operations on them either.

.. _thread-specific-storage-api:

Thread Specific Storage (TSS) API
---------------------------------

TSS API is introduced to supersede the use of the existing TLS API within the
CPython interpreter.  This API uses a new type :c:type:`Py_tss_t` instead of
:c:expr:`int` to represent thread keys.

.. versionadded:: 3.7

.. seealso:: "A New C-API for Thread-Local Storage in CPython" (:pep:`539`)


.. c:type:: Py_tss_t

   This data structure represents the state of a thread key, the definition of
   which may depend on the underlying TLS implementation, and it has an
   internal field representing the key's initialization state.  There are no
   public members in this structure.

   When :ref:`Py_LIMITED_API <stable>` is not defined, static allocation of
   this type by :c:macro:`Py_tss_NEEDS_INIT` is allowed.


.. c:macro:: Py_tss_NEEDS_INIT

   This macro expands to the initializer for :c:type:`Py_tss_t` variables.
   Note that this macro won't be defined with :ref:`Py_LIMITED_API <stable>`.


Dynamic Allocation
~~~~~~~~~~~~~~~~~~

Dynamic allocation of the :c:type:`Py_tss_t`, required in extension modules
built with :ref:`Py_LIMITED_API <stable>`, where static allocation of this type
is not possible due to its implementation being opaque at build time.


.. c:function:: Py_tss_t* PyThread_tss_alloc()

   Return a value which is the same state as a value initialized with
   :c:macro:`Py_tss_NEEDS_INIT`, or ``NULL`` in the case of dynamic allocation
   failure.


.. c:function:: void PyThread_tss_free(Py_tss_t *key)

   Free the given *key* allocated by :c:func:`PyThread_tss_alloc`, after
   first calling :c:func:`PyThread_tss_delete` to ensure any associated
   thread locals have been unassigned. This is a no-op if the *key*
   argument is ``NULL``.

   .. note::
      A freed key becomes a dangling pointer. You should reset the key to
      ``NULL``.


Methods
~~~~~~~

The parameter *key* of these functions must not be ``NULL``.  Moreover, the
behaviors of :c:func:`PyThread_tss_set` and :c:func:`PyThread_tss_get` are
undefined if the given :c:type:`Py_tss_t` has not been initialized by
:c:func:`PyThread_tss_create`.


.. c:function:: int PyThread_tss_is_created(Py_tss_t *key)

   Return a non-zero value if the given :c:type:`Py_tss_t` has been initialized
   by :c:func:`PyThread_tss_create`.


.. c:function:: int PyThread_tss_create(Py_tss_t *key)

   Return a zero value on successful initialization of a TSS key.  The behavior
   is undefined if the value pointed to by the *key* argument is not
   initialized by :c:macro:`Py_tss_NEEDS_INIT`.  This function can be called
   repeatedly on the same key -- calling it on an already initialized key is a
   no-op and immediately returns success.


.. c:function:: void PyThread_tss_delete(Py_tss_t *key)

   Destroy a TSS key to forget the values associated with the key across all
   threads, and change the key's initialization state to uninitialized.  A
   destroyed key is able to be initialized again by
   :c:func:`PyThread_tss_create`. This function can be called repeatedly on
   the same key -- calling it on an already destroyed key is a no-op.


.. c:function:: int PyThread_tss_set(Py_tss_t *key, void *value)

   Return a zero value to indicate successfully associating a :c:expr:`void*`
   value with a TSS key in the current thread.  Each thread has a distinct
   mapping of the key to a :c:expr:`void*` value.


.. c:function:: void* PyThread_tss_get(Py_tss_t *key)

   Return the :c:expr:`void*` value associated with a TSS key in the current
   thread.  This returns ``NULL`` if no value is associated with the key in the
   current thread.


.. _thread-local-storage-api:

Thread Local Storage (TLS) API
------------------------------

.. deprecated:: 3.7
   This API is superseded by
   :ref:`Thread Specific Storage (TSS) API <thread-specific-storage-api>`.

.. note::
   This version of the API does not support platforms where the native TLS key
   is defined in a way that cannot be safely cast to ``int``.  On such platforms,
   :c:func:`PyThread_create_key` will return immediately with a failure status,
   and the other TLS functions will all be no-ops on such platforms.

Due to the compatibility problem noted above, this version of the API should not
be used in new code.

.. c:function:: int PyThread_create_key()
.. c:function:: void PyThread_delete_key(int key)
.. c:function:: int PyThread_set_key_value(int key, void *value)
.. c:function:: void* PyThread_get_key_value(int key)
.. c:function:: void PyThread_delete_key_value(int key)
.. c:function:: void PyThread_ReInitTLS()


Synchronization Primitives
==========================

The C-API provides a basic mutual exclusion lock.

.. c:type:: PyMutex

   A mutual exclusion lock.  The :c:type:`!PyMutex` should be initialized to
   zero to represent the unlocked state.  For example::

      PyMutex mutex = {0};

   Instances of :c:type:`!PyMutex` should not be copied or moved.  Both the
   contents and address of a :c:type:`!PyMutex` are meaningful, and it must
   remain at a fixed, writable location in memory.

   .. note::

      A :c:type:`!PyMutex` currently occupies one byte, but the size should be
      considered unstable.  The size may change in future Python releases
      without a deprecation period.

   .. versionadded:: 3.13

.. c:function:: void PyMutex_Lock(PyMutex *m)

   Lock mutex *m*.  If another thread has already locked it, the calling
   thread will block until the mutex is unlocked.  While blocked, the thread
   will temporarily detach the :term:`thread state <attached thread state>` if one exists.

   .. versionadded:: 3.13

.. c:function:: void PyMutex_Unlock(PyMutex *m)

   Unlock mutex *m*. The mutex must be locked --- otherwise, the function will
   issue a fatal error.

   .. versionadded:: 3.13

.. c:function:: int PyMutex_IsLocked(PyMutex *m)

   Returns non-zero if the mutex *m* is currently locked, zero otherwise.

   .. note::

      This function is intended for use in assertions and debugging only and
      should not be used to make concurrency control decisions, as the lock
      state may change immediately after the check.

   .. versionadded:: 3.14

.. _python-critical-section-api:

Python Critical Section API
---------------------------

The critical section API provides a deadlock avoidance layer on top of
per-object locks for :term:`free-threaded <free threading>` CPython.  They are
intended to replace reliance on the :term:`global interpreter lock`, and are
no-ops in versions of Python with the global interpreter lock.

Critical sections are intended to be used for custom types implemented
in C-API extensions. They should generally not be used with built-in types like
:class:`list` and :class:`dict` because their public C-APIs
already use critical sections internally, with the notable
exception of :c:func:`PyDict_Next`, which requires critical section
to be acquired externally.

Critical sections avoid deadlocks by implicitly suspending active critical
sections, hence, they do not provide exclusive access such as provided by
traditional locks like :c:type:`PyMutex`.  When a critical section is started,
the per-object lock for the object is acquired. If the code executed inside the
critical section calls C-API functions then it can suspend the critical section thereby
releasing the per-object lock, so other threads can acquire the per-object lock
for the same object.

Variants that accept :c:type:`PyMutex` pointers rather than Python objects are also
available. Use these variants to start a critical section in a situation where
there is no :c:type:`PyObject` -- for example, when working with a C type that
does not extend or wrap :c:type:`PyObject` but still needs to call into the C
API in a manner that might lead to deadlocks.

The functions and structs used by the macros are exposed for cases
where C macros are not available. They should only be used as in the
given macro expansions. Note that the sizes and contents of the structures may
change in future Python versions.

.. note::

   Operations that need to lock two objects at once must use
   :c:macro:`Py_BEGIN_CRITICAL_SECTION2`.  You *cannot* use nested critical
   sections to lock more than one object at once, because the inner critical
   section may suspend the outer critical sections.  This API does not provide
   a way to lock more than two objects at once.

Example usage::

   static PyObject *
   set_field(MyObject *self, PyObject *value)
   {
      Py_BEGIN_CRITICAL_SECTION(self);
      Py_SETREF(self->field, Py_XNewRef(value));
      Py_END_CRITICAL_SECTION();
      Py_RETURN_NONE;
   }

In the above example, :c:macro:`Py_SETREF` calls :c:macro:`Py_DECREF`, which
can call arbitrary code through an object's deallocation function.  The critical
section API avoids potential deadlocks due to reentrancy and lock ordering
by allowing the runtime to temporarily suspend the critical section if the
code triggered by the finalizer blocks and calls :c:func:`PyEval_SaveThread`.

.. c:macro:: Py_BEGIN_CRITICAL_SECTION(op)

   Acquires the per-object lock for the object *op* and begins a
   critical section.

   In the free-threaded build, this macro expands to::

      {
          PyCriticalSection _py_cs;
          PyCriticalSection_Begin(&_py_cs, (PyObject*)(op))

   In the default build, this macro expands to ``{``.

   .. versionadded:: 3.13

.. c:macro:: Py_BEGIN_CRITICAL_SECTION_MUTEX(m)

   Locks the mutex *m* and begins a critical section.

   In the free-threaded build, this macro expands to::

     {
          PyCriticalSection _py_cs;
          PyCriticalSection_BeginMutex(&_py_cs, m)

   Note that unlike :c:macro:`Py_BEGIN_CRITICAL_SECTION`, there is no cast for
   the argument of the macro - it must be a :c:type:`PyMutex` pointer.

   On the default build, this macro expands to ``{``.

   .. versionadded:: 3.14

.. c:macro:: Py_END_CRITICAL_SECTION()

   Ends the critical section and releases the per-object lock.

   In the free-threaded build, this macro expands to::

          PyCriticalSection_End(&_py_cs);
      }

   In the default build, this macro expands to ``}``.

   .. versionadded:: 3.13

.. c:macro:: Py_BEGIN_CRITICAL_SECTION2(a, b)

   Acquires the per-objects locks for the objects *a* and *b* and begins a
   critical section.  The locks are acquired in a consistent order (lowest
   address first) to avoid lock ordering deadlocks.

   In the free-threaded build, this macro expands to::

      {
          PyCriticalSection2 _py_cs2;
          PyCriticalSection2_Begin(&_py_cs2, (PyObject*)(a), (PyObject*)(b))

   In the default build, this macro expands to ``{``.

   .. versionadded:: 3.13

.. c:macro:: Py_BEGIN_CRITICAL_SECTION2_MUTEX(m1, m2)

   Locks the mutexes *m1* and *m2* and begins a critical section.

   In the free-threaded build, this macro expands to::

     {
          PyCriticalSection2 _py_cs2;
          PyCriticalSection2_BeginMutex(&_py_cs2, m1, m2)

   Note that unlike :c:macro:`Py_BEGIN_CRITICAL_SECTION2`, there is no cast for
   the arguments of the macro - they must be :c:type:`PyMutex` pointers.

   On the default build, this macro expands to ``{``.

   .. versionadded:: 3.14

.. c:macro:: Py_END_CRITICAL_SECTION2()

   Ends the critical section and releases the per-object locks.

   In the free-threaded build, this macro expands to::

          PyCriticalSection2_End(&_py_cs2);
      }

   In the default build, this macro expands to ``}``.

   .. versionadded:: 3.13


Legacy Locking APIs
-------------------

These APIs are obsolete since Python 3.13 with the introduction of
:c:type:`PyMutex`.

.. versionchanged:: 3.15
   These APIs are now a simple wrapper around ``PyMutex``.


.. c:type:: PyThread_type_lock

   A pointer to a mutual exclusion lock.


.. c:type:: PyLockStatus

   The result of acquiring a lock with a timeout.

   .. c:namespace:: NULL

   .. c:enumerator:: PY_LOCK_FAILURE

      Failed to acquire the lock.

   .. c:enumerator:: PY_LOCK_ACQUIRED

      The lock was successfully acquired.

   .. c:enumerator:: PY_LOCK_INTR

      The lock was interrupted by a signal.


.. c:function:: PyThread_type_lock PyThread_allocate_lock(void)

   Allocate a new lock.

   On success, this function returns a lock; on failure, this
   function returns ``0`` without an exception set.

   The caller does not need to hold an :term:`attached thread state`.

   .. versionchanged:: 3.15
      This function now always uses :c:type:`PyMutex`. In prior versions, this
      would use a lock provided by the operating system.


.. c:function:: void PyThread_free_lock(PyThread_type_lock lock)

   Destroy *lock*. The lock should not be held by any thread when calling
   this.

   The caller does not need to hold an :term:`attached thread state`.


.. c:function:: PyLockStatus PyThread_acquire_lock_timed(PyThread_type_lock lock, long long microseconds, int intr_flag)

   Acquire *lock* with a timeout.

   This will wait for *microseconds* microseconds to acquire the lock. If the
   timeout expires, this function returns :c:enumerator:`PY_LOCK_FAILURE`.
   If *microseconds* is ``-1``, this will wait indefinitely until the lock has
   been released.

   If *intr_flag* is ``1``, acquiring the lock may be interrupted by a signal,
   in which case this function returns :c:enumerator:`PY_LOCK_INTR`. Upon
   interruption, it's generally expected that the caller makes a call to
   :c:func:`Py_MakePendingCalls` to propagate an exception to Python code.

   If the lock is successfully acquired, this function returns
   :c:enumerator:`PY_LOCK_ACQUIRED`.

   The caller does not need to hold an :term:`attached thread state`.


.. c:function:: int PyThread_acquire_lock(PyThread_type_lock lock, int waitflag)

   Acquire *lock*.

   If *waitflag* is ``1`` and another thread currently holds the lock, this
   function will wait until the lock can be acquired and will always return
   ``1``.

   If *waitflag* is ``0`` and another thread holds the lock, this function will
   not wait and instead return ``0``. If the lock is not held by any other
   thread, then this function will acquire it and return ``1``.

   Unlike :c:func:`PyThread_acquire_lock_timed`, acquiring the lock cannot be
   interrupted by a signal.

   The caller does not need to hold an :term:`attached thread state`.


.. c:function:: int PyThread_release_lock(PyThread_type_lock lock)

   Release *lock*. If *lock* is not held, then this function issues a
   fatal error.

   The caller does not need to hold an :term:`attached thread state`.


Operating System Thread APIs
============================

.. c:macro:: PYTHREAD_INVALID_THREAD_ID

   Sentinel value for an invalid thread ID.

   This is currently equivalent to ``(unsigned long)-1``.


.. c:function:: unsigned long PyThread_start_new_thread(void (*func)(void *), void *arg)

   Start function *func* in a new thread with argument *arg*.
   The resulting thread is not intended to be joined.

   *func* must not be ``NULL``, but *arg* may be ``NULL``.

   On success, this function returns the identifier of the new thread; on failure,
   this returns :c:macro:`PYTHREAD_INVALID_THREAD_ID`.

   The caller does not need to hold an :term:`attached thread state`.


.. c:function:: unsigned long PyThread_get_thread_ident(void)

   Return the identifier of the current thread, which will never be zero.

   This function cannot fail, and the caller does not need to hold an
   :term:`attached thread state`.

   .. seealso::
      :py:func:`threading.get_ident`


.. c:function:: PyObject *PyThread_GetInfo(void)

   Get general information about the current thread in the form of a
   :ref:`struct sequence <struct-sequence-objects>` object. This information is
   accessible as :py:attr:`sys.thread_info` in Python.

   On success, this returns a new :term:`strong reference` to the thread
   information; on failure, this returns ``NULL`` with an exception set.

   The caller must hold an :term:`attached thread state`.


.. c:macro:: PY_HAVE_THREAD_NATIVE_ID

   This macro is defined when the system supports native thread IDs.


.. c:function:: unsigned long PyThread_get_thread_native_id(void)

   Get the native identifier of the current thread as it was assigned by the operating
   system's kernel, which will never be less than zero.

   This function is only available when :c:macro:`PY_HAVE_THREAD_NATIVE_ID` is
   defined.

   This function cannot fail, and the caller does not need to hold an
   :term:`attached thread state`.

   .. seealso::
      :py:func:`threading.get_native_id`


.. c:function:: void PyThread_exit_thread(void)

   Terminate the current thread. This function is generally considered unsafe
   and should be avoided. It is kept solely for backwards compatibility.

   This function is only safe to call if all functions in the full call
   stack are written to safely allow it.

   .. warning::

      If the current system uses POSIX threads (also known as "pthreads"),
      this calls :manpage:`pthread_exit(3)`, which attempts to unwind the stack
      and call C++ destructors on some libc implementations. However, if a
      ``noexcept`` function is reached, it may terminate the process.
      Other systems, such as macOS, do unwinding.

      On Windows, this function calls ``_endthreadex()``, which kills the thread
      without calling C++ destructors.

      In any case, there is a risk of corruption on the thread's stack.

   .. deprecated:: 3.14


.. c:function:: void PyThread_init_thread(void)

   Initialize ``PyThread*`` APIs. Python executes this function automatically,
   so there's little need to call it from an extension module.


.. c:function:: int PyThread_set_stacksize(size_t size)

   Set the stack size of the current thread to *size* bytes.

   This function returns ``0`` on success, ``-1`` if *size* is invalid, or
   ``-2`` if the system does not support changing the stack size. This function
   does not set exceptions.

   The caller does not need to hold an :term:`attached thread state`.


.. c:function:: size_t PyThread_get_stacksize(void)

   Return the stack size of the current thread in bytes, or ``0`` if the system's
   default stack size is in use.

   The caller does not need to hold an :term:`attached thread state`.
