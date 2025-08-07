
/* UNIX password file access module */

#include "Python.h"
#include "posixmodule.h"

#include <errno.h>                // ERANGE
#include <pwd.h>                  // getpwuid()
#include <unistd.h>               // sysconf()

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

/* Mutex to protect calls to getpwuid(), getpwnam(), and getpwent().
 * These functions return pointer to static data structure, which
 * may be overwritten by any subsequent calls. */
static PyMutex pwd_db_mutex = {0};

#define DEFAULT_BUFFER_SIZE 1024

static PyObject *
mkpwent(PyObject *module, struct passwd *p)
{
    PyObject *v = PyStructSequence_New(get_pwd_state(module)->StructPwdType);
    if (v == NULL) {
        return NULL;
    }

    int setIndex = 0;

#define SET_STRING(VAL) \
    SET_RESULT((VAL) ? PyUnicode_DecodeFSDefault((VAL)) : Py_NewRef(Py_None))

#define SET_RESULT(CALL)                                     \
    do {                                                     \
        PyObject *item = (CALL);                             \
        if (item == NULL) {                                  \
            goto error;                                      \
        }                                                    \
        PyStructSequence_SetItem(v, setIndex++, item);       \
    } while(0)

    SET_STRING(p->pw_name);
#if defined(HAVE_STRUCT_PASSWD_PW_PASSWD) && !defined(__ANDROID__)
    SET_STRING(p->pw_passwd);
#else
    SET_STRING("");
#endif
    SET_RESULT(_PyLong_FromUid(p->pw_uid));
    SET_RESULT(_PyLong_FromGid(p->pw_gid));
#if defined(HAVE_STRUCT_PASSWD_PW_GECOS)
    SET_STRING(p->pw_gecos);
#else
    SET_STRING("");
#endif
    SET_STRING(p->pw_dir);
    SET_STRING(p->pw_shell);

#undef SET_STRING
#undef SET_RESULT

    return v;

error:
    Py_DECREF(v);
    return NULL;
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
    PyMutex_Lock(&pwd_db_mutex);
    // The getpwuid() function is not required to be thread-safe.
    // https://pubs.opengroup.org/onlinepubs/009604499/functions/getpwuid.html
    p = getpwuid(uid);
#endif
    if (p == NULL) {
#ifndef HAVE_GETPWUID_R
        PyMutex_Unlock(&pwd_db_mutex);
#endif
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
#else
    PyMutex_Unlock(&pwd_db_mutex);
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
    PyMutex_Lock(&pwd_db_mutex);
    // The getpwnam() function is not required to be thread-safe.
    // https://pubs.opengroup.org/onlinepubs/009604599/functions/getpwnam.html
    p = getpwnam(name_chars);
#endif
    if (p == NULL) {
#ifndef HAVE_GETPWNAM_R
        PyMutex_Unlock(&pwd_db_mutex);
#endif
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
#ifndef HAVE_GETPWNAM_R
    PyMutex_Unlock(&pwd_db_mutex);
#endif
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

    PyMutex_Lock(&pwd_db_mutex);
    int failure = 0;
    PyObject *v = NULL;
    // The setpwent(), getpwent() and endpwent() functions are not required to
    // be thread-safe.
    // https://pubs.opengroup.org/onlinepubs/009696799/functions/setpwent.html
    setpwent();
    while ((p = getpwent()) != NULL) {
        v = mkpwent(module, p);
        if (v == NULL || PyList_Append(d, v) != 0) {
            /* NOTE: cannot dec-ref here, while holding the mutex. */
            failure = 1;
            goto done;
        }
        Py_DECREF(v);
    }

done:
    endpwent();
    PyMutex_Unlock(&pwd_db_mutex);
    if (failure) {
        Py_XDECREF(v);
        Py_CLEAR(d);
    }
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
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
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
