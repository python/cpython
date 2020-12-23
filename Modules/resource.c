
#include "Python.h"
#include <sys/resource.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/* On some systems, these aren't in any header file.
   On others they are, with inconsistent prototypes.
   We declare the (default) return type, to shut up gcc -Wall;
   but we can't declare the prototype, to avoid errors
   when the header files declare it different.
   Worse, on some Linuxes, getpagesize() returns a size_t... */

#define doubletime(TV) ((double)(TV).tv_sec + (TV).tv_usec * 0.000001)

/*[clinic input]
module resource
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=e89d38ed52609d7c]*/

/*[python input]
class pid_t_converter(CConverter):
    type = 'pid_t'
    format_unit = '" _Py_PARSE_PID "'
[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=0c1d19f640d57e48]*/

#include "clinic/resource.c.h"

PyDoc_STRVAR(struct_rusage__doc__,
"struct_rusage: Result from getrusage.\n\n"
"This object may be accessed either as a tuple of\n"
"    (utime,stime,maxrss,ixrss,idrss,isrss,minflt,majflt,\n"
"    nswap,inblock,oublock,msgsnd,msgrcv,nsignals,nvcsw,nivcsw)\n"
"or via the attributes ru_utime, ru_stime, ru_maxrss, and so on.");

static PyStructSequence_Field struct_rusage_fields[] = {
    {"ru_utime",        "user time used"},
    {"ru_stime",        "system time used"},
    {"ru_maxrss",       "max. resident set size"},
    {"ru_ixrss",        "shared memory size"},
    {"ru_idrss",        "unshared data size"},
    {"ru_isrss",        "unshared stack size"},
    {"ru_minflt",       "page faults not requiring I/O"},
    {"ru_majflt",       "page faults requiring I/O"},
    {"ru_nswap",        "number of swap outs"},
    {"ru_inblock",      "block input operations"},
    {"ru_oublock",      "block output operations"},
    {"ru_msgsnd",       "IPC messages sent"},
    {"ru_msgrcv",       "IPC messages received"},
    {"ru_nsignals",     "signals received"},
    {"ru_nvcsw",        "voluntary context switches"},
    {"ru_nivcsw",       "involuntary context switches"},
    {0}
};

static PyStructSequence_Desc struct_rusage_desc = {
    "resource.struct_rusage",           /* name */
    struct_rusage__doc__,       /* doc */
    struct_rusage_fields,       /* fields */
    16          /* n_in_sequence */
};

typedef struct {
  PyTypeObject *StructRUsageType;
} resourcemodulestate;


static inline resourcemodulestate*
get_resource_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (resourcemodulestate *)state;
}

static struct PyModuleDef resourcemodule;

/*[clinic input]
resource.getrusage

    who: int
    /

[clinic start generated code]*/

static PyObject *
resource_getrusage_impl(PyObject *module, int who)
/*[clinic end generated code: output=8fad2880ba6a9843 input=5c857bcc5b9ccb1b]*/
{
    struct rusage ru;
    PyObject *result;

    if (getrusage(who, &ru) == -1) {
        if (errno == EINVAL) {
            PyErr_SetString(PyExc_ValueError,
                            "invalid who parameter");
            return NULL;
        }
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    result = PyStructSequence_New(
        get_resource_state(module)->StructRUsageType);
    if (!result)
        return NULL;

    PyStructSequence_SET_ITEM(result, 0,
                    PyFloat_FromDouble(doubletime(ru.ru_utime)));
    PyStructSequence_SET_ITEM(result, 1,
                    PyFloat_FromDouble(doubletime(ru.ru_stime)));
    PyStructSequence_SET_ITEM(result, 2, PyLong_FromLong(ru.ru_maxrss));
    PyStructSequence_SET_ITEM(result, 3, PyLong_FromLong(ru.ru_ixrss));
    PyStructSequence_SET_ITEM(result, 4, PyLong_FromLong(ru.ru_idrss));
    PyStructSequence_SET_ITEM(result, 5, PyLong_FromLong(ru.ru_isrss));
    PyStructSequence_SET_ITEM(result, 6, PyLong_FromLong(ru.ru_minflt));
    PyStructSequence_SET_ITEM(result, 7, PyLong_FromLong(ru.ru_majflt));
    PyStructSequence_SET_ITEM(result, 8, PyLong_FromLong(ru.ru_nswap));
    PyStructSequence_SET_ITEM(result, 9, PyLong_FromLong(ru.ru_inblock));
    PyStructSequence_SET_ITEM(result, 10, PyLong_FromLong(ru.ru_oublock));
    PyStructSequence_SET_ITEM(result, 11, PyLong_FromLong(ru.ru_msgsnd));
    PyStructSequence_SET_ITEM(result, 12, PyLong_FromLong(ru.ru_msgrcv));
    PyStructSequence_SET_ITEM(result, 13, PyLong_FromLong(ru.ru_nsignals));
    PyStructSequence_SET_ITEM(result, 14, PyLong_FromLong(ru.ru_nvcsw));
    PyStructSequence_SET_ITEM(result, 15, PyLong_FromLong(ru.ru_nivcsw));

    if (PyErr_Occurred()) {
        Py_DECREF(result);
        return NULL;
    }

    return result;
}

static int
py2rlimit(PyObject *limits, struct rlimit *rl_out)
{
    PyObject *curobj, *maxobj;
    limits = PySequence_Tuple(limits);
    if (!limits)
        /* Here limits is a borrowed reference */
        return -1;

    if (PyTuple_GET_SIZE(limits) != 2) {
        PyErr_SetString(PyExc_ValueError,
                        "expected a tuple of 2 integers");
        goto error;
    }
    curobj = PyTuple_GET_ITEM(limits, 0);
    maxobj = PyTuple_GET_ITEM(limits, 1);
#if !defined(HAVE_LARGEFILE_SUPPORT)
    rl_out->rlim_cur = PyLong_AsLong(curobj);
    if (rl_out->rlim_cur == (rlim_t)-1 && PyErr_Occurred())
        goto error;
    rl_out->rlim_max = PyLong_AsLong(maxobj);
    if (rl_out->rlim_max == (rlim_t)-1 && PyErr_Occurred())
        goto error;
#else
    /* The limits are probably bigger than a long */
    rl_out->rlim_cur = PyLong_AsLongLong(curobj);
    if (rl_out->rlim_cur == (rlim_t)-1 && PyErr_Occurred())
        goto error;
    rl_out->rlim_max = PyLong_AsLongLong(maxobj);
    if (rl_out->rlim_max == (rlim_t)-1 && PyErr_Occurred())
        goto error;
#endif

    Py_DECREF(limits);
    rl_out->rlim_cur = rl_out->rlim_cur & RLIM_INFINITY;
    rl_out->rlim_max = rl_out->rlim_max & RLIM_INFINITY;
    return 0;

error:
    Py_DECREF(limits);
    return -1;
}

static PyObject*
rlimit2py(struct rlimit rl)
{
    if (sizeof(rl.rlim_cur) > sizeof(long)) {
        return Py_BuildValue("LL",
                             (long long) rl.rlim_cur,
                             (long long) rl.rlim_max);
    }
    return Py_BuildValue("ll", (long) rl.rlim_cur, (long) rl.rlim_max);
}

/*[clinic input]
resource.getrlimit

    resource: int
    /

[clinic start generated code]*/

static PyObject *
resource_getrlimit_impl(PyObject *module, int resource)
/*[clinic end generated code: output=98327b25061ffe39 input=a697cb0004cb3c36]*/
{
    struct rlimit rl;

    if (resource < 0 || resource >= RLIM_NLIMITS) {
        PyErr_SetString(PyExc_ValueError,
                        "invalid resource specified");
        return NULL;
    }

    if (getrlimit(resource, &rl) == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    return rlimit2py(rl);
}

/*[clinic input]
resource.setrlimit

    resource: int
    limits: object
    /

[clinic start generated code]*/

static PyObject *
resource_setrlimit_impl(PyObject *module, int resource, PyObject *limits)
/*[clinic end generated code: output=4e82ec3f34d013d1 input=6235a6ce23b4ca75]*/
{
    struct rlimit rl;

    if (resource < 0 || resource >= RLIM_NLIMITS) {
        PyErr_SetString(PyExc_ValueError,
                        "invalid resource specified");
        return NULL;
    }

    if (PySys_Audit("resource.setrlimit", "iO", resource,
                    limits ? limits : Py_None) < 0) {
        return NULL;
    }

    if (py2rlimit(limits, &rl) < 0) {
        return NULL;
    }

    if (setrlimit(resource, &rl) == -1) {
        if (errno == EINVAL)
            PyErr_SetString(PyExc_ValueError,
                            "current limit exceeds maximum limit");
        else if (errno == EPERM)
            PyErr_SetString(PyExc_ValueError,
                            "not allowed to raise maximum limit");
        else
            PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    Py_RETURN_NONE;
}

#ifdef HAVE_PRLIMIT
/*[clinic input]
resource.prlimit

    pid: pid_t
    resource: int
    [
    limits: object
    ]
    /

[clinic start generated code]*/

static PyObject *
resource_prlimit_impl(PyObject *module, pid_t pid, int resource,
                      int group_right_1, PyObject *limits)
/*[clinic end generated code: output=ee976b393187a7a3 input=b77743bdccc83564]*/
{
    struct rlimit old_limit, new_limit;
    int retval;

    if (resource < 0 || resource >= RLIM_NLIMITS) {
        PyErr_SetString(PyExc_ValueError,
                        "invalid resource specified");
        return NULL;
    }

    if (PySys_Audit("resource.prlimit", "iiO", pid, resource,
                    limits ? limits : Py_None) < 0) {
        return NULL;
    }

    if (group_right_1) {
        if (py2rlimit(limits, &new_limit) < 0) {
            return NULL;
        }
        retval = prlimit(pid, resource, &new_limit, &old_limit);
    }
    else {
        retval = prlimit(pid, resource, NULL, &old_limit);
    }

    if (retval == -1) {
        if (errno == EINVAL) {
            PyErr_SetString(PyExc_ValueError,
                            "current limit exceeds maximum limit");
        } else {
            PyErr_SetFromErrno(PyExc_OSError);
        }
        return NULL;
    }
    return rlimit2py(old_limit);
}
#endif /* HAVE_PRLIMIT */

/*[clinic input]
resource.getpagesize -> int
[clinic start generated code]*/

static int
resource_getpagesize_impl(PyObject *module)
/*[clinic end generated code: output=9ba93eb0f3d6c3a9 input=546545e8c1f42085]*/
{
    long pagesize = 0;
#if defined(HAVE_GETPAGESIZE)
    pagesize = getpagesize();
#elif defined(HAVE_SYSCONF) && defined(_SC_PAGE_SIZE)
    pagesize = sysconf(_SC_PAGE_SIZE);
#else
#   error "unsupported platform: resource.getpagesize()"
#endif
    return pagesize;
}

/* List of functions */

static struct PyMethodDef
resource_methods[] = {
    RESOURCE_GETRUSAGE_METHODDEF
    RESOURCE_GETRLIMIT_METHODDEF
    RESOURCE_PRLIMIT_METHODDEF
    RESOURCE_SETRLIMIT_METHODDEF
    RESOURCE_GETPAGESIZE_METHODDEF
    {NULL, NULL}                             /* sentinel */
};


/* Module initialization */

static int
resource_exec(PyObject *module)
{
    resourcemodulestate *state = get_resource_state(module);
#define ADD_INT(module, value)                                    \
    do {                                                          \
        if (PyModule_AddIntConstant(module, #value, value) < 0) { \
            return -1;                                            \
        }                                                         \
    } while (0)

    /* Add some symbolic constants to the module */
    Py_INCREF(PyExc_OSError);
    if (PyModule_AddObject(module, "error", PyExc_OSError) < 0) {
        Py_DECREF(PyExc_OSError);
        return -1;
    }

    state->StructRUsageType = PyStructSequence_NewType(&struct_rusage_desc);
    if (state->StructRUsageType == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->StructRUsageType) < 0) {
        return -1;
    }

    /* insert constants */
#ifdef RLIMIT_CPU
    ADD_INT(module, RLIMIT_CPU);
#endif

#ifdef RLIMIT_FSIZE
    ADD_INT(module, RLIMIT_FSIZE);
#endif

#ifdef RLIMIT_DATA
    ADD_INT(module, RLIMIT_DATA);
#endif

#ifdef RLIMIT_STACK
    ADD_INT(module, RLIMIT_STACK);
#endif

#ifdef RLIMIT_CORE
    ADD_INT(module, RLIMIT_CORE);
#endif

#ifdef RLIMIT_NOFILE
    ADD_INT(module, RLIMIT_NOFILE);
#endif

#ifdef RLIMIT_OFILE
    ADD_INT(module, RLIMIT_OFILE);
#endif

#ifdef RLIMIT_VMEM
    ADD_INT(module, RLIMIT_VMEM);
#endif

#ifdef RLIMIT_AS
    ADD_INT(module, RLIMIT_AS);
#endif

#ifdef RLIMIT_RSS
    ADD_INT(module, RLIMIT_RSS);
#endif

#ifdef RLIMIT_NPROC
    ADD_INT(module, RLIMIT_NPROC);
#endif

#ifdef RLIMIT_MEMLOCK
    ADD_INT(module, RLIMIT_MEMLOCK);
#endif

#ifdef RLIMIT_SBSIZE
    ADD_INT(module, RLIMIT_SBSIZE);
#endif

/* Linux specific */
#ifdef RLIMIT_MSGQUEUE
    ADD_INT(module, RLIMIT_MSGQUEUE);
#endif

#ifdef RLIMIT_NICE
    ADD_INT(module, RLIMIT_NICE);
#endif

#ifdef RLIMIT_RTPRIO
    ADD_INT(module, RLIMIT_RTPRIO);
#endif

#ifdef RLIMIT_RTTIME
    ADD_INT(module, RLIMIT_RTTIME);
#endif

#ifdef RLIMIT_SIGPENDING
    ADD_INT(module, RLIMIT_SIGPENDING);
#endif

/* target */
#ifdef RUSAGE_SELF
    ADD_INT(module, RUSAGE_SELF);
#endif

#ifdef RUSAGE_CHILDREN
    ADD_INT(module, RUSAGE_CHILDREN);
#endif

#ifdef RUSAGE_BOTH
    ADD_INT(module, RUSAGE_BOTH);
#endif

#ifdef RUSAGE_THREAD
    ADD_INT(module, RUSAGE_THREAD);
#endif

/* FreeBSD specific */

#ifdef RLIMIT_SWAP
    ADD_INT(module, RLIMIT_SWAP);
#endif

#ifdef RLIMIT_SBSIZE
    ADD_INT(module, RLIMIT_SBSIZE);
#endif

#ifdef RLIMIT_NPTS
    ADD_INT(module, RLIMIT_NPTS);
#endif

    PyObject *v;
    if (sizeof(RLIM_INFINITY) > sizeof(long)) {
        v = PyLong_FromLongLong((long long) RLIM_INFINITY);
    } else
    {
        v = PyLong_FromLong((long) RLIM_INFINITY);
    }
    if (!v) {
        return -1;
    }

    if (PyModule_AddObject(module, "RLIM_INFINITY", v) < 0) {
        Py_DECREF(v);
        return -1;
    }
    return 0;

#undef ADD_INT
}

static struct PyModuleDef_Slot resource_slots[] = {
    {Py_mod_exec, resource_exec},
    {0, NULL}
};

static int
resourcemodule_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(get_resource_state(m)->StructRUsageType);
    return 0;
}

static int
resourcemodule_clear(PyObject *m) {
    Py_CLEAR(get_resource_state(m)->StructRUsageType);
    return 0;
}

static void
resourcemodule_free(void *m) {
    resourcemodule_clear((PyObject *)m);
}

static struct PyModuleDef resourcemodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "resource",
    .m_size = sizeof(resourcemodulestate),
    .m_methods = resource_methods,
    .m_slots = resource_slots,
    .m_traverse = resourcemodule_traverse,
    .m_clear = resourcemodule_clear,
    .m_free = resourcemodule_free,
};

PyMODINIT_FUNC
PyInit_resource(void)
{
    return PyModuleDef_Init(&resourcemodule);
}
