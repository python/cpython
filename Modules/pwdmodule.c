
/* UNIX password file access module */

#include "Python.h"
#include "posixmodule.h"

#include <pwd.h>

#include "clinic/pwdmodule.c.h"
/*[clinic input]
module pwd
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=60f628ef356b97b6]*/

static PyStructSequence_Field struct_pwd_type_fields[] = {
    {"pw_name", "user name"},
    {"pw_passwd", "password"},
    {"pw_uid", "user id"},
    {"pw_gid", "group id"},
    {"pw_gecos", "real name"},
    {"pw_dir", "home directory"},
    {"pw_shell", "shell program"},
    {0}
};

PyDoc_STRVAR(struct_passwd__doc__,
"pwd.struct_passwd: Results from getpw*() routines.\n\n\
This object may be accessed either as a tuple of\n\
  (pw_name,pw_passwd,pw_uid,pw_gid,pw_gecos,pw_dir,pw_shell)\n\
or via the object attributes as named in the above tuple.");

static PyStructSequence_Desc struct_pwd_type_desc = {
    "pwd.struct_passwd",
    struct_passwd__doc__,
    struct_pwd_type_fields,
    7,
};

PyDoc_STRVAR(pwd__doc__,
"This module provides access to the Unix password database.\n\
It is available on all Unix versions.\n\
\n\
Password database entries are reported as 7-tuples containing the following\n\
items from the password database (see `<pwd.h>'), in order:\n\
pw_name, pw_passwd, pw_uid, pw_gid, pw_gecos, pw_dir, pw_shell.\n\
The uid and gid items are integers, all others are strings. An\n\
exception is raised if the entry asked for cannot be found.");


static int initialized;
static PyTypeObject StructPwdType;

static void
sets(PyObject *v, int i, const char* val)
{
  if (val) {
      PyObject *o = PyUnicode_DecodeFSDefault(val);
      PyStructSequence_SET_ITEM(v, i, o);
  }
  else {
      PyStructSequence_SET_ITEM(v, i, Py_None);
      Py_INCREF(Py_None);
  }
}

static PyObject *
mkpwent(struct passwd *p)
{
    int setIndex = 0;
    PyObject *v = PyStructSequence_New(&StructPwdType);
    if (v == NULL)
        return NULL;

#define SETI(i,val) PyStructSequence_SET_ITEM(v, i, PyLong_FromLong((long) val))
#define SETS(i,val) sets(v, i, val)

    SETS(setIndex++, p->pw_name);
#if defined(HAVE_STRUCT_PASSWD_PW_PASSWD) && !defined(__ANDROID__)
    SETS(setIndex++, p->pw_passwd);
#else
    SETS(setIndex++, "");
#endif
    PyStructSequence_SET_ITEM(v, setIndex++, _PyLong_FromUid(p->pw_uid));
    PyStructSequence_SET_ITEM(v, setIndex++, _PyLong_FromGid(p->pw_gid));
#if defined(HAVE_STRUCT_PASSWD_PW_GECOS)
    SETS(setIndex++, p->pw_gecos);
#else
    SETS(setIndex++, "");
#endif
    SETS(setIndex++, p->pw_dir);
    SETS(setIndex++, p->pw_shell);

#undef SETS
#undef SETI

    if (PyErr_Occurred()) {
        Py_XDECREF(v);
        return NULL;
    }

    return v;
}

/*[clinic input]
pwd.getpwuid

    uidobj: object
    /

Return the password database entry for the given numeric user ID.

See `help(pwd)` for more on password database entries.
[clinic start generated code]*/

static PyObject *
pwd_getpwuid(PyObject *module, PyObject *uidobj)
/*[clinic end generated code: output=c4ee1d4d429b86c4 input=ae64d507a1c6d3e8]*/
{
    uid_t uid;
    struct passwd *p;

    if (!_Py_Uid_Converter(uidobj, &uid)) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError))
            PyErr_Format(PyExc_KeyError,
                         "getpwuid(): uid not found");
        return NULL;
    }
    if ((p = getpwuid(uid)) == NULL) {
        PyObject *uid_obj = _PyLong_FromUid(uid);
        if (uid_obj == NULL)
            return NULL;
        PyErr_Format(PyExc_KeyError,
                     "getpwuid(): uid not found: %S", uid_obj);
        Py_DECREF(uid_obj);
        return NULL;
    }
    return mkpwent(p);
}

/*[clinic input]
pwd.getpwnam

    arg: unicode
    /

Return the password database entry for the given user name.

See `help(pwd)` for more on password database entries.
[clinic start generated code]*/

static PyObject *
pwd_getpwnam_impl(PyObject *module, PyObject *arg)
/*[clinic end generated code: output=6abeee92430e43d2 input=d5f7e700919b02d3]*/
{
    char *name;
    struct passwd *p;
    PyObject *bytes, *retval = NULL;

    if ((bytes = PyUnicode_EncodeFSDefault(arg)) == NULL)
        return NULL;
    /* check for embedded null bytes */
    if (PyBytes_AsStringAndSize(bytes, &name, NULL) == -1)
        goto out;
    if ((p = getpwnam(name)) == NULL) {
        PyErr_Format(PyExc_KeyError,
                     "getpwnam(): name not found: %R", arg);
        goto out;
    }
    retval = mkpwent(p);
out:
    Py_DECREF(bytes);
    return retval;
}

#ifdef HAVE_GETPWENT
/*[clinic input]
pwd.getpwall

Return a list of all available password database entries, in arbitrary order.

See help(pwd) for more on password database entries.
[clinic start generated code]*/

static PyObject *
pwd_getpwall_impl(PyObject *module)
/*[clinic end generated code: output=4853d2f5a0afac8a input=d7ecebfd90219b85]*/
{
    PyObject *d;
    struct passwd *p;
    if ((d = PyList_New(0)) == NULL)
        return NULL;
    setpwent();
    while ((p = getpwent()) != NULL) {
        PyObject *v = mkpwent(p);
        if (v == NULL || PyList_Append(d, v) != 0) {
            Py_XDECREF(v);
            Py_DECREF(d);
            endpwent();
            return NULL;
        }
        Py_DECREF(v);
    }
    endpwent();
    return d;
}
#endif

static PyMethodDef pwd_methods[] = {
    PWD_GETPWUID_METHODDEF
    PWD_GETPWNAM_METHODDEF
#ifdef HAVE_GETPWENT
    PWD_GETPWALL_METHODDEF
#endif
    {NULL,              NULL}           /* sentinel */
};

static struct PyModuleDef pwdmodule = {
    PyModuleDef_HEAD_INIT,
    "pwd",
    pwd__doc__,
    -1,
    pwd_methods,
    NULL,
    NULL,
    NULL,
    NULL
};


PyMODINIT_FUNC
PyInit_pwd(void)
{
    PyObject *m;
    m = PyModule_Create(&pwdmodule);
    if (m == NULL)
        return NULL;

    if (!initialized) {
        if (PyStructSequence_InitType2(&StructPwdType,
                                       &struct_pwd_type_desc) < 0)
            return NULL;
        initialized = 1;
    }
    Py_INCREF((PyObject *) &StructPwdType);
    PyModule_AddObject(m, "struct_passwd", (PyObject *) &StructPwdType);
    return m;
}
