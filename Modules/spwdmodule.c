
/* UNIX shadow password file access module */
/* A lot of code has been taken from pwdmodule.c */
/* For info also see http://www.unixpapa.com/incnote/passwd.html */

#include "Python.h"

#include <sys/types.h>
#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif


PyDoc_STRVAR(spwd__doc__,
"This module provides access to the Unix shadow password database.\n\
It is available on various Unix versions.\n\
\n\
Shadow password database entries are reported as 9-tuples of type struct_spwd,\n\
containing the following items from the password database (see `<shadow.h>'):\n\
sp_namp, sp_pwdp, sp_lstchg, sp_min, sp_max, sp_warn, sp_inact, sp_expire, sp_flag.\n\
The sp_namp and sp_pwdp are strings, the rest are integers.\n\
An exception is raised if the entry asked for cannot be found.\n\
You have to be root to be able to use this module.");


#if defined(HAVE_GETSPNAM) || defined(HAVE_GETSPENT)

static PyStructSequence_Field struct_spwd_type_fields[] = {
    {"sp_namp", "login name"},
    {"sp_pwdp", "encrypted password"},
    {"sp_lstchg", "date of last change"},
    {"sp_min", "min #days between changes"},
    {"sp_max", "max #days between changes"},
    {"sp_warn", "#days before pw expires to warn user about it"},
    {"sp_inact", "#days after pw expires until account is disabled"},
    {"sp_expire", "#days since 1970-01-01 when account expires"},
    {"sp_flag", "reserved"},
    {"sp_nam", "login name; deprecated"}, /* Backward compatibility */
    {"sp_pwd", "encrypted password; deprecated"}, /* Backward compatibility */
    {0}
};

PyDoc_STRVAR(struct_spwd__doc__,
"spwd.struct_spwd: Results from getsp*() routines.\n\n\
This object may be accessed either as a 9-tuple of\n\
  (sp_namp,sp_pwdp,sp_lstchg,sp_min,sp_max,sp_warn,sp_inact,sp_expire,sp_flag)\n\
or via the object attributes as named in the above tuple.");

static PyStructSequence_Desc struct_spwd_type_desc = {
    "spwd.struct_spwd",
    struct_spwd__doc__,
    struct_spwd_type_fields,
    9,
};

static int initialized;
static PyTypeObject StructSpwdType;


static void
sets(PyObject *v, int i, const char* val)
{
  if (val) {
      PyObject *o = PyUnicode_DecodeFSDefault(val);
      PyStructSequence_SET_ITEM(v, i, o);
  } else {
      PyStructSequence_SET_ITEM(v, i, Py_None);
      Py_INCREF(Py_None);
  }
}

static PyObject *mkspent(struct spwd *p)
{
    int setIndex = 0;
    PyObject *v = PyStructSequence_New(&StructSpwdType);
    if (v == NULL)
        return NULL;

#define SETI(i,val) PyStructSequence_SET_ITEM(v, i, PyLong_FromLong((long) val))
#define SETS(i,val) sets(v, i, val)

    SETS(setIndex++, p->sp_namp);
    SETS(setIndex++, p->sp_pwdp);
    SETI(setIndex++, p->sp_lstchg);
    SETI(setIndex++, p->sp_min);
    SETI(setIndex++, p->sp_max);
    SETI(setIndex++, p->sp_warn);
    SETI(setIndex++, p->sp_inact);
    SETI(setIndex++, p->sp_expire);
    SETI(setIndex++, p->sp_flag);
    SETS(setIndex++, p->sp_namp); /* Backward compatibility for sp_nam */
    SETS(setIndex++, p->sp_pwdp); /* Backward compatibility for sp_pwd */

#undef SETS
#undef SETI

    if (PyErr_Occurred()) {
        Py_DECREF(v);
        return NULL;
    }

    return v;
}

#endif  /* HAVE_GETSPNAM || HAVE_GETSPENT */


#ifdef HAVE_GETSPNAM

PyDoc_STRVAR(spwd_getspnam__doc__,
"getspnam(name) -> (sp_namp, sp_pwdp, sp_lstchg, sp_min, sp_max,\n\
                    sp_warn, sp_inact, sp_expire, sp_flag)\n\
Return the shadow password database entry for the given user name.\n\
See spwd.__doc__ for more on shadow password database entries.");

static PyObject* spwd_getspnam(PyObject *self, PyObject *args)
{
    char *name;
    struct spwd *p;
    PyObject *arg, *bytes, *retval = NULL;

    if (!PyArg_ParseTuple(args, "U:getspnam", &arg))
        return NULL;
    if ((bytes = PyUnicode_EncodeFSDefault(arg)) == NULL)
        return NULL;
    if (PyBytes_AsStringAndSize(bytes, &name, NULL) == -1)
        goto out;
    if ((p = getspnam(name)) == NULL) {
        PyErr_SetString(PyExc_KeyError, "getspnam(): name not found");
        goto out;
    }
    retval = mkspent(p);
out:
    Py_DECREF(bytes);
    return retval;
}

#endif /* HAVE_GETSPNAM */

#ifdef HAVE_GETSPENT

PyDoc_STRVAR(spwd_getspall__doc__,
"getspall() -> list_of_entries\n\
Return a list of all available shadow password database entries, \
in arbitrary order.\n\
See spwd.__doc__ for more on shadow password database entries.");

static PyObject *
spwd_getspall(PyObject *self, PyObject *args)
{
    PyObject *d;
    struct spwd *p;
    if ((d = PyList_New(0)) == NULL)
        return NULL;
    setspent();
    while ((p = getspent()) != NULL) {
        PyObject *v = mkspent(p);
        if (v == NULL || PyList_Append(d, v) != 0) {
            Py_XDECREF(v);
            Py_DECREF(d);
            endspent();
            return NULL;
        }
        Py_DECREF(v);
    }
    endspent();
    return d;
}

#endif /* HAVE_GETSPENT */

static PyMethodDef spwd_methods[] = {
#ifdef HAVE_GETSPNAM
    {"getspnam",        spwd_getspnam, METH_VARARGS, spwd_getspnam__doc__},
#endif
#ifdef HAVE_GETSPENT
    {"getspall",        spwd_getspall, METH_NOARGS, spwd_getspall__doc__},
#endif
    {NULL,              NULL}           /* sentinel */
};



static struct PyModuleDef spwdmodule = {
    PyModuleDef_HEAD_INIT,
    "spwd",
    spwd__doc__,
    -1,
    spwd_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_spwd(void)
{
    PyObject *m;
    m=PyModule_Create(&spwdmodule);
    if (m == NULL)
        return NULL;
    if (!initialized) {
        if (PyStructSequence_InitType2(&StructSpwdType,
                                       &struct_spwd_type_desc) < 0)
            return NULL;
    }
    Py_INCREF((PyObject *) &StructSpwdType);
    PyModule_AddObject(m, "struct_spwd", (PyObject *) &StructSpwdType);
    initialized = 1;
    return m;
}
