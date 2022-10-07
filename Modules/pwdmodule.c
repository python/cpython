
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


typedef struct {
    PyTypeObject *StructPwdType;
} pwdmodulestate;

static inline pwdmodulestate*
get_pwd_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (pwdmodulestate *)state;
}

static struct PyModuleDef pwdmodule;

#define DEFAULT_BUFFER_SIZE 1024

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
mkpwent(PyObject *module, struct passwd *p)
{
    int setIndex = 0;
    PyObject *v = PyStructSequence_New(get_pwd_state(module)->StructPwdType);
    if (v == NULL)
        return NULL;

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
    PyObject *retval = NULL;
    uid_t uid;
    int nomem = 0;
    struct passwd *p;
    char *buf = NULL, *buf2 = NULL;

    if (!_Py_Uid_Converter(uidobj, &uid)) {
        if (PyErr_ExceptionMatches(PyExc_OverflowError))
            PyErr_Format(PyExc_KeyError,
                         "getpwuid(): uid not found");
        return NULL;
    }
#ifdef HAVE_GETPWUID_R
    int status;
    Py_ssize_t bufsize;
    /* Note: 'pwd' will be used via pointer 'p' on getpwuid_r success. */
    struct passwd pwd;

    Py_BEGIN_ALLOW_THREADS
    bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (bufsize == -1) {
        bufsize = DEFAULT_BUFFER_SIZE;
    }

    while(1) {
        buf2 = PyMem_RawRealloc(buf, bufsize);
        if (buf2 == NULL) {
            p = NULL;
            nomem = 1;
            break;
        }
        buf = buf2;
        status = getpwuid_r(uid, &pwd, buf, bufsize, &p);
        if (status != 0) {
            p = NULL;
        }
        if (p != NULL || status != ERANGE) {
            break;
        }
        if (bufsize > (PY_SSIZE_T_MAX >> 1)) {
            nomem = 1;
            break;
        }
        bufsize <<= 1;
    }

    Py_END_ALLOW_THREADS
#else
    p = getpwuid(uid);
#endif
    if (p == NULL) {
        PyMem_RawFree(buf);
        if (nomem == 1) {
            return PyErr_NoMemory();
        }
        PyObject *uid_obj = _PyLong_FromUid(uid);
        if (uid_obj == NULL)
            return NULL;
        PyErr_Format(PyExc_KeyError,
                     "getpwuid(): uid not found: %S", uid_obj);
        Py_DECREF(uid_obj);
        return NULL;
    }
    retval = mkpwent(module, p);
#ifdef HAVE_GETPWUID_R
    PyMem_RawFree(buf);
#endif
    return retval;
}

/*[clinic input]
pwd.getpwnam

    name: unicode
    /

Return the password database entry for the given user name.

See `help(pwd)` for more on password database entries.
[clinic start generated code]*/

static PyObject *
pwd_getpwnam_impl(PyObject *module, PyObject *name)
/*[clinic end generated code: output=359ce1ddeb7a824f input=a6aeb5e3447fb9e0]*/
{
    char *buf = NULL, *buf2 = NULL, *name_chars;
    int nomem = 0;
    struct passwd *p;
    PyObject *bytes, *retval = NULL;

    if ((bytes = PyUnicode_EncodeFSDefault(name)) == NULL)
        return NULL;
    /* check for embedded null bytes */
    if (PyBytes_AsStringAndSize(bytes, &name_chars, NULL) == -1)
        goto out;
#ifdef HAVE_GETPWNAM_R
    int status;
    Py_ssize_t bufsize;
    /* Note: 'pwd' will be used via pointer 'p' on getpwnam_r success. */
    struct passwd pwd;

    Py_BEGIN_ALLOW_THREADS
    bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (bufsize == -1) {
        bufsize = DEFAULT_BUFFER_SIZE;
    }

    while(1) {
        buf2 = PyMem_RawRealloc(buf, bufsize);
        if (buf2 == NULL) {
            p = NULL;
            nomem = 1;
            break;
        }
        buf = buf2;
        status = getpwnam_r(name_chars, &pwd, buf, bufsize, &p);
        if (status != 0) {
            p = NULL;
        }
        if (p != NULL || status != ERANGE) {
            break;
        }
        if (bufsize > (PY_SSIZE_T_MAX >> 1)) {
            nomem = 1;
            break;
        }
        bufsize <<= 1;
    }

    Py_END_ALLOW_THREADS
#else
    p = getpwnam(name_chars);
#endif
    if (p == NULL) {
        if (nomem == 1) {
            PyErr_NoMemory();
        }
        else {
            PyErr_Format(PyExc_KeyError,
                         "getpwnam(): name not found: %R", name);
        }
        goto out;
    }
    retval = mkpwent(module, p);
out:
    PyMem_RawFree(buf);
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
        PyObject *v = mkpwent(module, p);
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

static int
pwdmodule_exec(PyObject *module)
{
    pwdmodulestate *state = get_pwd_state(module);

    state->StructPwdType = PyStructSequence_NewType(&struct_pwd_type_desc);
    if (state->StructPwdType == NULL) {
        return -1;
    }
    if (PyModule_AddType(module, state->StructPwdType) < 0) {
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot pwdmodule_slots[] = {
    {Py_mod_exec, pwdmodule_exec},
    {0, NULL}
};

static int pwdmodule_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(get_pwd_state(m)->StructPwdType);
    return 0;
}
static int pwdmodule_clear(PyObject *m) {
    Py_CLEAR(get_pwd_state(m)->StructPwdType);
    return 0;
}
static void pwdmodule_free(void *m) {
    pwdmodule_clear((PyObject *)m);
}

static struct PyModuleDef pwdmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "pwd",
    .m_doc = pwd__doc__,
    .m_size = sizeof(pwdmodulestate),
    .m_methods = pwd_methods,
    .m_slots = pwdmodule_slots,
    .m_traverse = pwdmodule_traverse,
    .m_clear = pwdmodule_clear,
    .m_free = pwdmodule_free,
};


PyMODINIT_FUNC
PyInit_pwd(void)
{
    return PyModuleDef_Init(&pwdmodule);
}
