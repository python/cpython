/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

#if !defined(_thread__ThreadHandle_ident_DOCSTR)
#  define _thread__ThreadHandle_ident_DOCSTR NULL
#endif
#if defined(_THREAD__THREADHANDLE_IDENT_GETSETDEF)
#  undef _THREAD__THREADHANDLE_IDENT_GETSETDEF
#  define _THREAD__THREADHANDLE_IDENT_GETSETDEF {"ident", (getter)_thread__ThreadHandle_ident_get, (setter)_thread__ThreadHandle_ident_set, _thread__ThreadHandle_ident_DOCSTR},
#else
#  define _THREAD__THREADHANDLE_IDENT_GETSETDEF {"ident", (getter)_thread__ThreadHandle_ident_get, NULL, _thread__ThreadHandle_ident_DOCSTR},
#endif

static PyObject *
_thread__ThreadHandle_ident_get_impl(PyObject *self);

static PyObject *
_thread__ThreadHandle_ident_get(PyObject *self, void *Py_UNUSED(context))
{
    return _thread__ThreadHandle_ident_get_impl(self);
}

PyDoc_STRVAR(_thread__ThreadHandle_join__doc__,
"join($self, timeout=None, /)\n"
"--\n"
"\n");

#define _THREAD__THREADHANDLE_JOIN_METHODDEF    \
    {"join", _PyCFunction_CAST(_thread__ThreadHandle_join), METH_FASTCALL, _thread__ThreadHandle_join__doc__},

static PyObject *
_thread__ThreadHandle_join_impl(PyObject *self, PyObject *timeout_obj);

static PyObject *
_thread__ThreadHandle_join(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *timeout_obj = Py_None;

    if (!_PyArg_CheckPositional("join", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    timeout_obj = args[0];
skip_optional:
    return_value = _thread__ThreadHandle_join_impl(self, timeout_obj);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread__ThreadHandle_is_done__doc__,
"is_done($self, /)\n"
"--\n"
"\n");

#define _THREAD__THREADHANDLE_IS_DONE_METHODDEF    \
    {"is_done", (PyCFunction)_thread__ThreadHandle_is_done, METH_NOARGS, _thread__ThreadHandle_is_done__doc__},

static PyObject *
_thread__ThreadHandle_is_done_impl(PyObject *self);

static PyObject *
_thread__ThreadHandle_is_done(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _thread__ThreadHandle_is_done_impl(self);
}

PyDoc_STRVAR(_thread__ThreadHandle__set_done__doc__,
"_set_done($self, /)\n"
"--\n"
"\n");

#define _THREAD__THREADHANDLE__SET_DONE_METHODDEF    \
    {"_set_done", (PyCFunction)_thread__ThreadHandle__set_done, METH_NOARGS, _thread__ThreadHandle__set_done__doc__},

static PyObject *
_thread__ThreadHandle__set_done_impl(PyObject *self);

static PyObject *
_thread__ThreadHandle__set_done(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _thread__ThreadHandle__set_done_impl(self);
}

PyDoc_STRVAR(_thread_lock_acquire__doc__,
"acquire($self, /, blocking=True, timeout=-1)\n"
"--\n"
"\n"
"Lock the lock.\n"
"\n"
"Without argument, this blocks if the lock is already locked\n"
"(even by the same thread), waiting for another thread to release\n"
"the lock, and return True once the lock is acquired.\n"
"With an argument, this will only block if the argument is true,\n"
"and the return value reflects whether the lock is acquired.\n"
"The blocking operation is interruptible.");

#define _THREAD_LOCK_ACQUIRE_METHODDEF    \
    {"acquire", _PyCFunction_CAST(_thread_lock_acquire), METH_FASTCALL|METH_KEYWORDS, _thread_lock_acquire__doc__},

static PyObject *
_thread_lock_acquire_impl(lockobject *self, int blocking,
                          PyObject *timeoutobj);

static PyObject *
_thread_lock_acquire(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(blocking), &_Py_ID(timeout), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"blocking", "timeout", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "acquire",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int blocking = 1;
    PyObject *timeoutobj = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        blocking = PyObject_IsTrue(args[0]);
        if (blocking < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    timeoutobj = args[1];
skip_optional_pos:
    return_value = _thread_lock_acquire_impl((lockobject *)self, blocking, timeoutobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_lock_acquire_lock__doc__,
"acquire_lock($self, /, blocking=True, timeout=-1)\n"
"--\n"
"\n"
"An obsolete synonym of acquire().");

#define _THREAD_LOCK_ACQUIRE_LOCK_METHODDEF    \
    {"acquire_lock", _PyCFunction_CAST(_thread_lock_acquire_lock), METH_FASTCALL|METH_KEYWORDS, _thread_lock_acquire_lock__doc__},

static PyObject *
_thread_lock_acquire_lock_impl(lockobject *self, int blocking,
                               PyObject *timeoutobj);

static PyObject *
_thread_lock_acquire_lock(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(blocking), &_Py_ID(timeout), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"blocking", "timeout", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "acquire_lock",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int blocking = 1;
    PyObject *timeoutobj = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        blocking = PyObject_IsTrue(args[0]);
        if (blocking < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    timeoutobj = args[1];
skip_optional_pos:
    return_value = _thread_lock_acquire_lock_impl((lockobject *)self, blocking, timeoutobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_lock_release__doc__,
"release($self, /)\n"
"--\n"
"\n"
"Release the lock.\n"
"\n"
"Allows another thread that is blocked waiting for\n"
"the lock to acquire the lock.  The lock must be in the locked state,\n"
"but it needn\'t be locked by the same thread that unlocks it.");

#define _THREAD_LOCK_RELEASE_METHODDEF    \
    {"release", (PyCFunction)_thread_lock_release, METH_NOARGS, _thread_lock_release__doc__},

static PyObject *
_thread_lock_release_impl(lockobject *self);

static PyObject *
_thread_lock_release(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _thread_lock_release_impl((lockobject *)self);
}

PyDoc_STRVAR(_thread_lock_release_lock__doc__,
"release_lock($self, /)\n"
"--\n"
"\n"
"An obsolete synonym of release().");

#define _THREAD_LOCK_RELEASE_LOCK_METHODDEF    \
    {"release_lock", (PyCFunction)_thread_lock_release_lock, METH_NOARGS, _thread_lock_release_lock__doc__},

static PyObject *
_thread_lock_release_lock_impl(lockobject *self);

static PyObject *
_thread_lock_release_lock(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _thread_lock_release_lock_impl((lockobject *)self);
}

PyDoc_STRVAR(_thread_lock___enter____doc__,
"__enter__($self, /)\n"
"--\n"
"\n"
"Lock the lock.");

#define _THREAD_LOCK___ENTER___METHODDEF    \
    {"__enter__", (PyCFunction)_thread_lock___enter__, METH_NOARGS, _thread_lock___enter____doc__},

static PyObject *
_thread_lock___enter___impl(lockobject *self);

static PyObject *
_thread_lock___enter__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _thread_lock___enter___impl((lockobject *)self);
}

PyDoc_STRVAR(_thread_lock___exit____doc__,
"__exit__($self, exc_type, exc_value, exc_tb, /)\n"
"--\n"
"\n"
"Release the lock.");

#define _THREAD_LOCK___EXIT___METHODDEF    \
    {"__exit__", _PyCFunction_CAST(_thread_lock___exit__), METH_FASTCALL, _thread_lock___exit____doc__},

static PyObject *
_thread_lock___exit___impl(lockobject *self, PyObject *exc_type,
                           PyObject *exc_value, PyObject *exc_tb);

static PyObject *
_thread_lock___exit__(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *exc_type;
    PyObject *exc_value;
    PyObject *exc_tb;

    if (!_PyArg_CheckPositional("__exit__", nargs, 3, 3)) {
        goto exit;
    }
    exc_type = args[0];
    exc_value = args[1];
    exc_tb = args[2];
    return_value = _thread_lock___exit___impl((lockobject *)self, exc_type, exc_value, exc_tb);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_lock_locked__doc__,
"locked($self, /)\n"
"--\n"
"\n"
"Return whether the lock is in the locked state.");

#define _THREAD_LOCK_LOCKED_METHODDEF    \
    {"locked", (PyCFunction)_thread_lock_locked, METH_NOARGS, _thread_lock_locked__doc__},

static PyObject *
_thread_lock_locked_impl(lockobject *self);

static PyObject *
_thread_lock_locked(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _thread_lock_locked_impl((lockobject *)self);
}

PyDoc_STRVAR(_thread_lock_locked_lock__doc__,
"locked_lock($self, /)\n"
"--\n"
"\n"
"An obsolete synonym of locked().");

#define _THREAD_LOCK_LOCKED_LOCK_METHODDEF    \
    {"locked_lock", (PyCFunction)_thread_lock_locked_lock, METH_NOARGS, _thread_lock_locked_lock__doc__},

static PyObject *
_thread_lock_locked_lock_impl(lockobject *self);

static PyObject *
_thread_lock_locked_lock(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _thread_lock_locked_lock_impl((lockobject *)self);
}

#if defined(HAVE_FORK)

PyDoc_STRVAR(_thread_lock__at_fork_reinit__doc__,
"_at_fork_reinit($self, /)\n"
"--\n"
"\n");

#define _THREAD_LOCK__AT_FORK_REINIT_METHODDEF    \
    {"_at_fork_reinit", (PyCFunction)_thread_lock__at_fork_reinit, METH_NOARGS, _thread_lock__at_fork_reinit__doc__},

static PyObject *
_thread_lock__at_fork_reinit_impl(lockobject *self);

static PyObject *
_thread_lock__at_fork_reinit(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _thread_lock__at_fork_reinit_impl((lockobject *)self);
}

#endif /* defined(HAVE_FORK) */

static PyObject *
lock_new_impl(PyTypeObject *type);

static PyObject *
lock_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = clinic_state()->lock_type;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoPositional("lock", args)) {
        goto exit;
    }
    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("lock", kwargs)) {
        goto exit;
    }
    return_value = lock_new_impl(type);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_RLock_acquire__doc__,
"acquire($self, /, blocking=True, timeout=-1)\n"
"--\n"
"\n"
"Lock the lock.\n"
"\n"
"`blocking` indicates whether we should wait\n"
"for the lock to be available or not.  If `blocking` is False\n"
"and another thread holds the lock, the method will return False\n"
"immediately.  If `blocking` is True and another thread holds\n"
"the lock, the method will wait for the lock to be released,\n"
"take it and then return True.\n"
"(note: the blocking operation is interruptible.)\n"
"\n"
"In all other cases, the method will return True immediately.\n"
"Precisely, if the current thread already holds the lock, its\n"
"internal counter is simply incremented. If nobody holds the lock,\n"
"the lock is taken and its internal counter initialized to 1.");

#define _THREAD_RLOCK_ACQUIRE_METHODDEF    \
    {"acquire", _PyCFunction_CAST(_thread_RLock_acquire), METH_FASTCALL|METH_KEYWORDS, _thread_RLock_acquire__doc__},

static PyObject *
_thread_RLock_acquire_impl(rlockobject *self, int blocking,
                           PyObject *timeoutobj);

static PyObject *
_thread_RLock_acquire(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(blocking), &_Py_ID(timeout), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"blocking", "timeout", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "acquire",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int blocking = 1;
    PyObject *timeoutobj = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        blocking = PyObject_IsTrue(args[0]);
        if (blocking < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    timeoutobj = args[1];
skip_optional_pos:
    return_value = _thread_RLock_acquire_impl((rlockobject *)self, blocking, timeoutobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_RLock___enter____doc__,
"__enter__($self, /)\n"
"--\n"
"\n"
"Lock the lock.");

#define _THREAD_RLOCK___ENTER___METHODDEF    \
    {"__enter__", (PyCFunction)_thread_RLock___enter__, METH_NOARGS, _thread_RLock___enter____doc__},

static PyObject *
_thread_RLock___enter___impl(rlockobject *self);

static PyObject *
_thread_RLock___enter__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _thread_RLock___enter___impl((rlockobject *)self);
}

PyDoc_STRVAR(_thread_RLock_release__doc__,
"release($self, /)\n"
"--\n"
"\n"
"Release the lock.\n"
"\n"
"Allows another thread that is blocked waiting for the lock\n"
"to acquire the lock.  The lock must be in the locked state,\n"
"and must be locked by the same thread that unlocks it; otherwise a\n"
"`RuntimeError` is raised.\n"
"\n"
"Do note that if the lock was acquire()d several times in a row by\n"
"the current thread, release() needs to be called as many times for\n"
"the lock to be available for other threads.");

#define _THREAD_RLOCK_RELEASE_METHODDEF    \
    {"release", (PyCFunction)_thread_RLock_release, METH_NOARGS, _thread_RLock_release__doc__},

static PyObject *
_thread_RLock_release_impl(rlockobject *self);

static PyObject *
_thread_RLock_release(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _thread_RLock_release_impl((rlockobject *)self);
}

PyDoc_STRVAR(_thread_RLock___exit____doc__,
"__exit__($self, exc_type, exc_value, exc_tb, /)\n"
"--\n"
"\n"
"Release the lock.");

#define _THREAD_RLOCK___EXIT___METHODDEF    \
    {"__exit__", _PyCFunction_CAST(_thread_RLock___exit__), METH_FASTCALL, _thread_RLock___exit____doc__},

static PyObject *
_thread_RLock___exit___impl(rlockobject *self, PyObject *exc_type,
                            PyObject *exc_value, PyObject *exc_tb);

static PyObject *
_thread_RLock___exit__(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *exc_type;
    PyObject *exc_value;
    PyObject *exc_tb;

    if (!_PyArg_CheckPositional("__exit__", nargs, 3, 3)) {
        goto exit;
    }
    exc_type = args[0];
    exc_value = args[1];
    exc_tb = args[2];
    return_value = _thread_RLock___exit___impl((rlockobject *)self, exc_type, exc_value, exc_tb);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_RLock_locked__doc__,
"locked($self, /)\n"
"--\n"
"\n"
"Return a boolean indicating whether this object is locked right now.");

#define _THREAD_RLOCK_LOCKED_METHODDEF    \
    {"locked", (PyCFunction)_thread_RLock_locked, METH_NOARGS, _thread_RLock_locked__doc__},

static PyObject *
_thread_RLock_locked_impl(rlockobject *self);

static PyObject *
_thread_RLock_locked(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _thread_RLock_locked_impl((rlockobject *)self);
}

PyDoc_STRVAR(_thread_RLock__acquire_restore__doc__,
"_acquire_restore($self, state, /)\n"
"--\n"
"\n"
"For internal use by `threading.Condition`.");

#define _THREAD_RLOCK__ACQUIRE_RESTORE_METHODDEF    \
    {"_acquire_restore", (PyCFunction)_thread_RLock__acquire_restore, METH_O, _thread_RLock__acquire_restore__doc__},

static PyObject *
_thread_RLock__acquire_restore_impl(rlockobject *self, PyObject *state);

static PyObject *
_thread_RLock__acquire_restore(PyObject *self, PyObject *state)
{
    PyObject *return_value = NULL;

    return_value = _thread_RLock__acquire_restore_impl((rlockobject *)self, state);

    return return_value;
}

PyDoc_STRVAR(_thread_RLock__release_save__doc__,
"_release_save($self, /)\n"
"--\n"
"\n"
"For internal use by `threading.Condition`.");

#define _THREAD_RLOCK__RELEASE_SAVE_METHODDEF    \
    {"_release_save", (PyCFunction)_thread_RLock__release_save, METH_NOARGS, _thread_RLock__release_save__doc__},

static PyObject *
_thread_RLock__release_save_impl(rlockobject *self);

static PyObject *
_thread_RLock__release_save(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _thread_RLock__release_save_impl((rlockobject *)self);
}

PyDoc_STRVAR(_thread_RLock__recursion_count__doc__,
"_recursion_count($self, /)\n"
"--\n"
"\n"
"For internal use by reentrancy checks.");

#define _THREAD_RLOCK__RECURSION_COUNT_METHODDEF    \
    {"_recursion_count", (PyCFunction)_thread_RLock__recursion_count, METH_NOARGS, _thread_RLock__recursion_count__doc__},

static PyObject *
_thread_RLock__recursion_count_impl(rlockobject *self);

static PyObject *
_thread_RLock__recursion_count(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _thread_RLock__recursion_count_impl((rlockobject *)self);
}

PyDoc_STRVAR(_thread_RLock__is_owned__doc__,
"_is_owned($self, /)\n"
"--\n"
"\n"
"For internal use by `threading.Condition`.");

#define _THREAD_RLOCK__IS_OWNED_METHODDEF    \
    {"_is_owned", (PyCFunction)_thread_RLock__is_owned, METH_NOARGS, _thread_RLock__is_owned__doc__},

static PyObject *
_thread_RLock__is_owned_impl(rlockobject *self);

static PyObject *
_thread_RLock__is_owned(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _thread_RLock__is_owned_impl((rlockobject *)self);
}

static PyObject *
rlock_new_impl(PyTypeObject *type);

static PyObject *
rlock_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    PyTypeObject *base_tp = clinic_state()->rlock_type;

    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoPositional("RLock", args)) {
        goto exit;
    }
    if ((type == base_tp || type->tp_init == base_tp->tp_init) &&
        !_PyArg_NoKeywords("RLock", kwargs)) {
        goto exit;
    }
    return_value = rlock_new_impl(type);

exit:
    return return_value;
}

#if defined(HAVE_FORK)

PyDoc_STRVAR(_thread_RLock__at_fork_reinit__doc__,
"_at_fork_reinit($self, /)\n"
"--\n"
"\n");

#define _THREAD_RLOCK__AT_FORK_REINIT_METHODDEF    \
    {"_at_fork_reinit", (PyCFunction)_thread_RLock__at_fork_reinit, METH_NOARGS, _thread_RLock__at_fork_reinit__doc__},

static PyObject *
_thread_RLock__at_fork_reinit_impl(rlockobject *self);

static PyObject *
_thread_RLock__at_fork_reinit(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _thread_RLock__at_fork_reinit_impl((rlockobject *)self);
}

#endif /* defined(HAVE_FORK) */

PyDoc_STRVAR(_thread_daemon_threads_allowed__doc__,
"daemon_threads_allowed($module, /)\n"
"--\n"
"\n"
"Return True if daemon threads are allowed in the current interpreter.\n"
"\n"
"Return False otherwise.");

#define _THREAD_DAEMON_THREADS_ALLOWED_METHODDEF    \
    {"daemon_threads_allowed", (PyCFunction)_thread_daemon_threads_allowed, METH_NOARGS, _thread_daemon_threads_allowed__doc__},

static PyObject *
_thread_daemon_threads_allowed_impl(PyObject *module);

static PyObject *
_thread_daemon_threads_allowed(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _thread_daemon_threads_allowed_impl(module);
}

PyDoc_STRVAR(_thread_start_new_thread__doc__,
"start_new_thread($module, function, args, kwargs={}, /)\n"
"--\n"
"\n"
"Start a new thread and return its identifier.\n"
"\n"
"The thread will call the function with positional arguments from the\n"
"tuple args and keyword arguments taken from the optional dictionary\n"
"kwargs.  The thread exits when the function returns; the return value\n"
"is ignored.  The thread will also exit when the function raises an\n"
"unhandled exception; a stack trace will be printed unless the exception\n"
"is SystemExit.");

#define _THREAD_START_NEW_THREAD_METHODDEF    \
    {"start_new_thread", _PyCFunction_CAST(_thread_start_new_thread), METH_FASTCALL, _thread_start_new_thread__doc__},

static PyObject *
_thread_start_new_thread_impl(PyObject *module, PyObject *func,
                              PyObject *args, PyObject *kwargs);

static PyObject *
_thread_start_new_thread(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *func;
    PyObject *__clinic_args;
    PyObject *__clinic_kwargs = NULL;

    if (!_PyArg_CheckPositional("start_new_thread", nargs, 2, 3)) {
        goto exit;
    }
    func = args[0];
    if (!PyTuple_Check(args[1])) {
        _PyArg_BadArgument("start_new_thread", "argument 2", "tuple", args[1]);
        goto exit;
    }
    __clinic_args = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    if (!PyDict_Check(args[2])) {
        _PyArg_BadArgument("start_new_thread", "argument 3", "dict", args[2]);
        goto exit;
    }
    __clinic_kwargs = args[2];
skip_optional:
    return_value = _thread_start_new_thread_impl(module, func, __clinic_args, __clinic_kwargs);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_start_joinable_thread__doc__,
"start_joinable_thread($module, /, function, handle=None, daemon=True)\n"
"--\n"
"\n"
"*For internal use only*: start a new thread.\n"
"\n"
"Like start_new_thread(), this starts a new thread calling the given\n"
"function.  Unlike start_new_thread(), this returns a handle object with\n"
"methods to join or detach the given thread.\n"
"This function is not for third-party code, please use the `threading`\n"
"module instead.  During finalization the runtime will not wait for the\n"
"thread to exit if daemon is True.  If handle is provided it must be a\n"
"newly created thread._ThreadHandle instance.");

#define _THREAD_START_JOINABLE_THREAD_METHODDEF    \
    {"start_joinable_thread", _PyCFunction_CAST(_thread_start_joinable_thread), METH_FASTCALL|METH_KEYWORDS, _thread_start_joinable_thread__doc__},

static PyObject *
_thread_start_joinable_thread_impl(PyObject *module, PyObject *func,
                                   PyObject *hobj, int daemon);

static PyObject *
_thread_start_joinable_thread(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(function), &_Py_ID(handle), &_Py_ID(daemon), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"function", "handle", "daemon", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "start_joinable_thread",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *func;
    PyObject *hobj = Py_None;
    int daemon = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    func = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        hobj = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    daemon = PyObject_IsTrue(args[2]);
    if (daemon < 0) {
        goto exit;
    }
skip_optional_pos:
    return_value = _thread_start_joinable_thread_impl(module, func, hobj, daemon);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_exit__doc__,
"exit($module, /)\n"
"--\n"
"\n"
"Raise SystemExit.\n"
"\n"
"It will cause the current thread to exit silently unless the exception\n"
"is caught.");

#define _THREAD_EXIT_METHODDEF    \
    {"exit", (PyCFunction)_thread_exit, METH_NOARGS, _thread_exit__doc__},

static PyObject *
_thread_exit_impl(PyObject *module);

static PyObject *
_thread_exit(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _thread_exit_impl(module);
}

PyDoc_STRVAR(_thread_interrupt_main__doc__,
"interrupt_main($module, signum=signal.SIGINT, /)\n"
"--\n"
"\n"
"Simulate the arrival of the given signal in the main thread.\n"
"\n"
"The corresponding signal handler will be executed.\n"
"If *signum* is omitted, SIGINT is assumed.\n"
"A subthread can use this function to interrupt the main thread.\n"
"\n"
"Note: the default signal handler for SIGINT raises\n"
"``KeyboardInterrupt``.");

#define _THREAD_INTERRUPT_MAIN_METHODDEF    \
    {"interrupt_main", _PyCFunction_CAST(_thread_interrupt_main), METH_FASTCALL, _thread_interrupt_main__doc__},

static PyObject *
_thread_interrupt_main_impl(PyObject *module, int signum);

static PyObject *
_thread_interrupt_main(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int signum = SIGINT;

    if (!_PyArg_CheckPositional("interrupt_main", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    signum = PyLong_AsInt(args[0]);
    if (signum == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _thread_interrupt_main_impl(module, signum);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread_allocate_lock__doc__,
"allocate_lock($module, /)\n"
"--\n"
"\n"
"Create a new lock object.\n"
"\n"
"See help(type(threading.Lock())) for information about locks.");

#define _THREAD_ALLOCATE_LOCK_METHODDEF    \
    {"allocate_lock", (PyCFunction)_thread_allocate_lock, METH_NOARGS, _thread_allocate_lock__doc__},

static PyObject *
_thread_allocate_lock_impl(PyObject *module);

static PyObject *
_thread_allocate_lock(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _thread_allocate_lock_impl(module);
}

PyDoc_STRVAR(_thread_get_ident__doc__,
"get_ident($module, /)\n"
"--\n"
"\n"
"Return a non-zero integer that uniquely identifies the current thread.\n"
"\n"
"It is unique amongst other threads that exist simultaneously.\n"
"This may be used to identify per-thread resources.\n"
"Even though on some platforms threads identities may appear to be\n"
"allocated consecutive numbers starting at 1, this behavior should not\n"
"be relied upon, and the number should be seen purely as a magic cookie.\n"
"A thread\'s identity may be reused for another thread after it exits.");

#define _THREAD_GET_IDENT_METHODDEF    \
    {"get_ident", (PyCFunction)_thread_get_ident, METH_NOARGS, _thread_get_ident__doc__},

static PyObject *
_thread_get_ident_impl(PyObject *module);

static PyObject *
_thread_get_ident(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _thread_get_ident_impl(module);
}

#if defined(PY_HAVE_THREAD_NATIVE_ID)

PyDoc_STRVAR(_thread_get_native_id__doc__,
"get_native_id($module, /)\n"
"--\n"
"\n"
"Return a non-negative integer identifying the thread.\n"
"\n"
"It is reported by the OS (kernel).  This may be used to uniquely\n"
"identify a particular thread within a system.");

#define _THREAD_GET_NATIVE_ID_METHODDEF    \
    {"get_native_id", (PyCFunction)_thread_get_native_id, METH_NOARGS, _thread_get_native_id__doc__},

static PyObject *
_thread_get_native_id_impl(PyObject *module);

static PyObject *
_thread_get_native_id(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _thread_get_native_id_impl(module);
}

#endif /* defined(PY_HAVE_THREAD_NATIVE_ID) */

PyDoc_STRVAR(_thread__count__doc__,
"_count($module, /)\n"
"--\n"
"\n"
"Return the number of currently running Python threads.\n"
"\n"
"The main thread is excluded.  The returned number comprises all threads\n"
"created through `start_new_thread()` as well as `threading.Thread`, and\n"
"not yet finished.\n"
"\n"
"This function is meant for internal and specialized purposes only.\n"
"In most applications `threading.enumerate()` should be used instead.");

#define _THREAD__COUNT_METHODDEF    \
    {"_count", (PyCFunction)_thread__count, METH_NOARGS, _thread__count__doc__},

static PyObject *
_thread__count_impl(PyObject *module);

static PyObject *
_thread__count(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _thread__count_impl(module);
}

PyDoc_STRVAR(_thread_stack_size__doc__,
"stack_size($module, size=0, /)\n"
"--\n"
"\n"
"Return the thread stack size used when creating new threads.\n"
"\n"
"The optional size argument specifies the stack size (in bytes) to be\n"
"used for subsequently created threads, and must be 0 (use platform or\n"
"configured default) or a positive integer value of at least 32,768 (32\n"
"KiB).  If changing the thread stack size is unsupported, a ThreadError\n"
"exception is raised.  If the specified size is invalid, a ValueError\n"
"exception is raised, and the stack size is unmodified.  32 KiB\n"
"currently is the minimum supported stack size value to guarantee\n"
"sufficient stack space for the interpreter itself.\n"
"\n"
"Note that some systems may have particular restrictions on values for\n"
"the stack size, such as requiring a minimum stack size larger than 32\n"
"KiB or requiring allocation in multiples of the system memory page size\n"
"- platform documentation should be referred to for more information\n"
"(4 KiB pages are common; using multiples of 4096 for the stack size is\n"
"the suggested approach in the absence of more specific information).");

#define _THREAD_STACK_SIZE_METHODDEF    \
    {"stack_size", _PyCFunction_CAST(_thread_stack_size), METH_FASTCALL, _thread_stack_size__doc__},

static PyObject *
_thread_stack_size_impl(PyObject *module, Py_ssize_t new_size);

static PyObject *
_thread_stack_size(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_ssize_t new_size = 0;

    if (!_PyArg_CheckPositional("stack_size", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        new_size = ival;
    }
skip_optional:
    return_value = _thread_stack_size_impl(module, new_size);

exit:
    return return_value;
}

PyDoc_STRVAR(_thread__excepthook__doc__,
"_excepthook($module, args, /)\n"
"--\n"
"\n"
"Handle uncaught Thread.run() exception.");

#define _THREAD__EXCEPTHOOK_METHODDEF    \
    {"_excepthook", (PyCFunction)_thread__excepthook, METH_O, _thread__excepthook__doc__},

PyDoc_STRVAR(_thread__is_main_interpreter__doc__,
"_is_main_interpreter($module, /)\n"
"--\n"
"\n"
"Return True if the current interpreter is the main Python interpreter.");

#define _THREAD__IS_MAIN_INTERPRETER_METHODDEF    \
    {"_is_main_interpreter", (PyCFunction)_thread__is_main_interpreter, METH_NOARGS, _thread__is_main_interpreter__doc__},

static PyObject *
_thread__is_main_interpreter_impl(PyObject *module);

static PyObject *
_thread__is_main_interpreter(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _thread__is_main_interpreter_impl(module);
}

PyDoc_STRVAR(_thread__shutdown__doc__,
"_shutdown($module, /)\n"
"--\n"
"\n"
"Wait for all non-daemon threads (other than the calling thread) to stop.");

#define _THREAD__SHUTDOWN_METHODDEF    \
    {"_shutdown", (PyCFunction)_thread__shutdown, METH_NOARGS, _thread__shutdown__doc__},

static PyObject *
_thread__shutdown_impl(PyObject *module);

static PyObject *
_thread__shutdown(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _thread__shutdown_impl(module);
}

PyDoc_STRVAR(_thread__make_thread_handle__doc__,
"_make_thread_handle($module, ident, /)\n"
"--\n"
"\n"
"Internal only.\n"
"\n"
"Make a thread handle for threads not spawned by the _thread or\n"
"threading module.");

#define _THREAD__MAKE_THREAD_HANDLE_METHODDEF    \
    {"_make_thread_handle", (PyCFunction)_thread__make_thread_handle, METH_O, _thread__make_thread_handle__doc__},

PyDoc_STRVAR(_thread__get_main_thread_ident__doc__,
"_get_main_thread_ident($module, /)\n"
"--\n"
"\n"
"Internal only.\n"
"\n"
"Return a non-zero integer that uniquely identifies the main thread of\n"
"the main interpreter.");

#define _THREAD__GET_MAIN_THREAD_IDENT_METHODDEF    \
    {"_get_main_thread_ident", (PyCFunction)_thread__get_main_thread_ident, METH_NOARGS, _thread__get_main_thread_ident__doc__},

static PyObject *
_thread__get_main_thread_ident_impl(PyObject *module);

static PyObject *
_thread__get_main_thread_ident(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _thread__get_main_thread_ident_impl(module);
}

#if (defined(HAVE_PTHREAD_GETNAME_NP) || defined(HAVE_PTHREAD_GET_NAME_NP) || defined(MS_WINDOWS))

PyDoc_STRVAR(_thread__get_name__doc__,
"_get_name($module, /)\n"
"--\n"
"\n"
"Get the name of the current thread.");

#define _THREAD__GET_NAME_METHODDEF    \
    {"_get_name", (PyCFunction)_thread__get_name, METH_NOARGS, _thread__get_name__doc__},

static PyObject *
_thread__get_name_impl(PyObject *module);

static PyObject *
_thread__get_name(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _thread__get_name_impl(module);
}

#endif /* (defined(HAVE_PTHREAD_GETNAME_NP) || defined(HAVE_PTHREAD_GET_NAME_NP) || defined(MS_WINDOWS)) */

#if (defined(HAVE_PTHREAD_SETNAME_NP) || defined(HAVE_PTHREAD_SET_NAME_NP) || defined(MS_WINDOWS))

PyDoc_STRVAR(_thread_set_name__doc__,
"set_name($module, /, name)\n"
"--\n"
"\n"
"Set the name of the current thread.");

#define _THREAD_SET_NAME_METHODDEF    \
    {"set_name", _PyCFunction_CAST(_thread_set_name), METH_FASTCALL|METH_KEYWORDS, _thread_set_name__doc__},

static PyObject *
_thread_set_name_impl(PyObject *module, PyObject *name_obj);

static PyObject *
_thread_set_name(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_name",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *name_obj;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("set_name", "argument 'name'", "str", args[0]);
        goto exit;
    }
    name_obj = args[0];
    return_value = _thread_set_name_impl(module, name_obj);

exit:
    return return_value;
}

#endif /* (defined(HAVE_PTHREAD_SETNAME_NP) || defined(HAVE_PTHREAD_SET_NAME_NP) || defined(MS_WINDOWS)) */

#ifndef _THREAD_LOCK__AT_FORK_REINIT_METHODDEF
    #define _THREAD_LOCK__AT_FORK_REINIT_METHODDEF
#endif /* !defined(_THREAD_LOCK__AT_FORK_REINIT_METHODDEF) */

#ifndef _THREAD_RLOCK__AT_FORK_REINIT_METHODDEF
    #define _THREAD_RLOCK__AT_FORK_REINIT_METHODDEF
#endif /* !defined(_THREAD_RLOCK__AT_FORK_REINIT_METHODDEF) */

#ifndef _THREAD_GET_NATIVE_ID_METHODDEF
    #define _THREAD_GET_NATIVE_ID_METHODDEF
#endif /* !defined(_THREAD_GET_NATIVE_ID_METHODDEF) */

#ifndef _THREAD__GET_NAME_METHODDEF
    #define _THREAD__GET_NAME_METHODDEF
#endif /* !defined(_THREAD__GET_NAME_METHODDEF) */

#ifndef _THREAD_SET_NAME_METHODDEF
    #define _THREAD_SET_NAME_METHODDEF
#endif /* !defined(_THREAD_SET_NAME_METHODDEF) */
/*[clinic end generated code: output=cd1b9c78d32ab693 input=a9049054013a1b77]*/
