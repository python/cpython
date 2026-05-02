/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_thread_lock_acquire__doc__,
"acquire($self, /, blocking=True, timeout=-1)\n"
"--\n"
"\n"
"Lock the lock.\n"
"\n"
"Without argument, this blocks if the lock is already\n"
"locked (even by the same thread), waiting for another thread to release\n"
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
"Allows another thread that is blocked waiting for\n"
"the lock to acquire the lock.  The lock must be in the locked state,\n"
"and must be locked by the same thread that unlocks it; otherwise a\n"
"`RuntimeError` is raised.\n"
"\n"
"Do note that if the lock was acquire()d several times in a row by the\n"
"current thread, release() needs to be called as many times for the lock\n"
"to be available for other threads.");

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

#ifndef _THREAD__GET_NAME_METHODDEF
    #define _THREAD__GET_NAME_METHODDEF
#endif /* !defined(_THREAD__GET_NAME_METHODDEF) */

#ifndef _THREAD_SET_NAME_METHODDEF
    #define _THREAD_SET_NAME_METHODDEF
#endif /* !defined(_THREAD_SET_NAME_METHODDEF) */
/*[clinic end generated code: output=1255a1520f43f97a input=a9049054013a1b77]*/
