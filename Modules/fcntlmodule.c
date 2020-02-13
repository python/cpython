
/* fcntl module */

#define PY_SSIZE_T_CLEAN

#include "Python.h"

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#include <sys/ioctl.h>
#include <fcntl.h>
#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif

/*[clinic input]
module fcntl
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=124b58387c158179]*/

static int
conv_descriptor(PyObject *object, int *target)
{
    int fd = PyObject_AsFileDescriptor(object);

    if (fd < 0)
        return 0;
    *target = fd;
    return 1;
}

/* Must come after conv_descriptor definition. */
#include "clinic/fcntlmodule.c.h"

/*[clinic input]
fcntl.fcntl

    fd: object(type='int', converter='conv_descriptor')
    cmd as code: int
    arg: object(c_default='NULL') = 0
    /

Perform the operation `cmd` on file descriptor fd.

The values used for `cmd` are operating system dependent, and are available
as constants in the fcntl module, using the same names as used in
the relevant C header files.  The argument arg is optional, and
defaults to 0; it may be an int or a string.  If arg is given as a string,
the return value of fcntl is a string of that length, containing the
resulting value put in the arg buffer by the operating system.  The length
of the arg string is not allowed to exceed 1024 bytes.  If the arg given
is an integer or if none is specified, the result value is an integer
corresponding to the return value of the fcntl call in the C code.
[clinic start generated code]*/

static PyObject *
fcntl_fcntl_impl(PyObject *module, int fd, int code, PyObject *arg)
/*[clinic end generated code: output=888fc93b51c295bd input=8cefbe59b29efbe2]*/
{
    unsigned int int_arg = 0;
    int ret;
    char *str;
    Py_ssize_t len;
    char buf[1024];
    int async_err = 0;

    if (PySys_Audit("fcntl.fcntl", "iiO", fd, code, arg ? arg : Py_None) < 0) {
        return NULL;
    }

    if (arg != NULL) {
        int parse_result;

        if (PyArg_Parse(arg, "s#", &str, &len)) {
            if ((size_t)len > sizeof buf) {
                PyErr_SetString(PyExc_ValueError,
                                "fcntl string arg too long");
                return NULL;
            }
            memcpy(buf, str, len);
            do {
                Py_BEGIN_ALLOW_THREADS
                ret = fcntl(fd, code, buf);
                Py_END_ALLOW_THREADS
            } while (ret == -1 && errno == EINTR && !(async_err = PyErr_CheckSignals()));
            if (ret < 0) {
                return !async_err ? PyErr_SetFromErrno(PyExc_OSError) : NULL;
            }
            return PyBytes_FromStringAndSize(buf, len);
        }

        PyErr_Clear();
        parse_result = PyArg_Parse(arg,
            "I;fcntl requires a file or file descriptor,"
            " an integer and optionally a third integer or a string",
            &int_arg);
        if (!parse_result) {
          return NULL;
        }
    }

    do {
        Py_BEGIN_ALLOW_THREADS
        ret = fcntl(fd, code, (int)int_arg);
        Py_END_ALLOW_THREADS
    } while (ret == -1 && errno == EINTR && !(async_err = PyErr_CheckSignals()));
    if (ret < 0) {
        return !async_err ? PyErr_SetFromErrno(PyExc_OSError) : NULL;
    }
    return PyLong_FromLong((long)ret);
}


/*[clinic input]
fcntl.ioctl

    fd: object(type='int', converter='conv_descriptor')
    request as code: unsigned_int(bitwise=True)
    arg as ob_arg: object(c_default='NULL') = 0
    mutate_flag as mutate_arg: bool = True
    /

Perform the operation `request` on file descriptor `fd`.

The values used for `request` are operating system dependent, and are available
as constants in the fcntl or termios library modules, using the same names as
used in the relevant C header files.

The argument `arg` is optional, and defaults to 0; it may be an int or a
buffer containing character data (most likely a string or an array).

If the argument is a mutable buffer (such as an array) and if the
mutate_flag argument (which is only allowed in this case) is true then the
buffer is (in effect) passed to the operating system and changes made by
the OS will be reflected in the contents of the buffer after the call has
returned.  The return value is the integer returned by the ioctl system
call.

If the argument is a mutable buffer and the mutable_flag argument is false,
the behavior is as if a string had been passed.

If the argument is an immutable buffer (most likely a string) then a copy
of the buffer is passed to the operating system and the return value is a
string of the same length containing whatever the operating system put in
the buffer.  The length of the arg buffer in this case is not allowed to
exceed 1024 bytes.

If the arg given is an integer or if none is specified, the result value is
an integer corresponding to the return value of the ioctl call in the C
code.
[clinic start generated code]*/

static PyObject *
fcntl_ioctl_impl(PyObject *module, int fd, unsigned int code,
                 PyObject *ob_arg, int mutate_arg)
/*[clinic end generated code: output=7f7f5840c65991be input=ede70c433cccbbb2]*/
{
#define IOCTL_BUFSZ 1024
    /* We use the unsigned non-checked 'I' format for the 'code' parameter
       because the system expects it to be a 32bit bit field value
       regardless of it being passed as an int or unsigned long on
       various platforms.  See the termios.TIOCSWINSZ constant across
       platforms for an example of this.

       If any of the 64bit platforms ever decide to use more than 32bits
       in their unsigned long ioctl codes this will break and need
       special casing based on the platform being built on.
     */
    int arg = 0;
    int ret;
    Py_buffer pstr;
    char *str;
    Py_ssize_t len;
    char buf[IOCTL_BUFSZ+1];  /* argument plus NUL byte */

    if (PySys_Audit("fcntl.ioctl", "iIO", fd, code,
                    ob_arg ? ob_arg : Py_None) < 0) {
        return NULL;
    }

    if (ob_arg != NULL) {
        if (PyArg_Parse(ob_arg, "w*:ioctl", &pstr)) {
            char *arg;
            str = pstr.buf;
            len = pstr.len;

            if (mutate_arg) {
                if (len <= IOCTL_BUFSZ) {
                    memcpy(buf, str, len);
                    buf[len] = '\0';
                    arg = buf;
                }
                else {
                    arg = str;
                }
            }
            else {
                if (len > IOCTL_BUFSZ) {
                    PyBuffer_Release(&pstr);
                    PyErr_SetString(PyExc_ValueError,
                        "ioctl string arg too long");
                    return NULL;
                }
                else {
                    memcpy(buf, str, len);
                    buf[len] = '\0';
                    arg = buf;
                }
            }
            if (buf == arg) {
                Py_BEGIN_ALLOW_THREADS /* think array.resize() */
                ret = ioctl(fd, code, arg);
                Py_END_ALLOW_THREADS
            }
            else {
                ret = ioctl(fd, code, arg);
            }
            if (mutate_arg && (len <= IOCTL_BUFSZ)) {
                memcpy(str, buf, len);
            }
            PyBuffer_Release(&pstr); /* No further access to str below this point */
            if (ret < 0) {
                PyErr_SetFromErrno(PyExc_OSError);
                return NULL;
            }
            if (mutate_arg) {
                return PyLong_FromLong(ret);
            }
            else {
                return PyBytes_FromStringAndSize(buf, len);
            }
        }

        PyErr_Clear();
        if (PyArg_Parse(ob_arg, "s*:ioctl", &pstr)) {
            str = pstr.buf;
            len = pstr.len;
            if (len > IOCTL_BUFSZ) {
                PyBuffer_Release(&pstr);
                PyErr_SetString(PyExc_ValueError,
                                "ioctl string arg too long");
                return NULL;
            }
            memcpy(buf, str, len);
            buf[len] = '\0';
            Py_BEGIN_ALLOW_THREADS
            ret = ioctl(fd, code, buf);
            Py_END_ALLOW_THREADS
            if (ret < 0) {
                PyBuffer_Release(&pstr);
                PyErr_SetFromErrno(PyExc_OSError);
                return NULL;
            }
            PyBuffer_Release(&pstr);
            return PyBytes_FromStringAndSize(buf, len);
        }

        PyErr_Clear();
        if (!PyArg_Parse(ob_arg,
             "i;ioctl requires a file or file descriptor,"
             " an integer and optionally an integer or buffer argument",
             &arg)) {
          return NULL;
        }
        // Fall-through to outside the 'if' statement.
    }
    Py_BEGIN_ALLOW_THREADS
    ret = ioctl(fd, code, arg);
    Py_END_ALLOW_THREADS
    if (ret < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    return PyLong_FromLong((long)ret);
#undef IOCTL_BUFSZ
}

/*[clinic input]
fcntl.flock

    fd: object(type='int', converter='conv_descriptor')
    operation as code: int
    /

Perform the lock operation `operation` on file descriptor `fd`.

See the Unix manual page for flock(2) for details (On some systems, this
function is emulated using fcntl()).
[clinic start generated code]*/

static PyObject *
fcntl_flock_impl(PyObject *module, int fd, int code)
/*[clinic end generated code: output=84059e2b37d2fc64 input=b70a0a41ca22a8a0]*/
{
    int ret;
    int async_err = 0;

    if (PySys_Audit("fcntl.flock", "ii", fd, code) < 0) {
        return NULL;
    }

#ifdef HAVE_FLOCK
    do {
        Py_BEGIN_ALLOW_THREADS
        ret = flock(fd, code);
        Py_END_ALLOW_THREADS
    } while (ret == -1 && errno == EINTR && !(async_err = PyErr_CheckSignals()));
#else

#ifndef LOCK_SH
#define LOCK_SH         1       /* shared lock */
#define LOCK_EX         2       /* exclusive lock */
#define LOCK_NB         4       /* don't block when locking */
#define LOCK_UN         8       /* unlock */
#endif
    {
        struct flock l;
        if (code == LOCK_UN)
            l.l_type = F_UNLCK;
        else if (code & LOCK_SH)
            l.l_type = F_RDLCK;
        else if (code & LOCK_EX)
            l.l_type = F_WRLCK;
        else {
            PyErr_SetString(PyExc_ValueError,
                            "unrecognized flock argument");
            return NULL;
        }
        l.l_whence = l.l_start = l.l_len = 0;
        do {
            Py_BEGIN_ALLOW_THREADS
            ret = fcntl(fd, (code & LOCK_NB) ? F_SETLK : F_SETLKW, &l);
            Py_END_ALLOW_THREADS
        } while (ret == -1 && errno == EINTR && !(async_err = PyErr_CheckSignals()));
    }
#endif /* HAVE_FLOCK */
    if (ret < 0) {
        return !async_err ? PyErr_SetFromErrno(PyExc_OSError) : NULL;
    }
    Py_RETURN_NONE;
}


/*[clinic input]
fcntl.lockf

    fd: object(type='int', converter='conv_descriptor')
    cmd as code: int
    len as lenobj: object(c_default='NULL') = 0
    start as startobj: object(c_default='NULL') = 0
    whence: int = 0
    /

A wrapper around the fcntl() locking calls.

`fd` is the file descriptor of the file to lock or unlock, and operation is one
of the following values:

    LOCK_UN - unlock
    LOCK_SH - acquire a shared lock
    LOCK_EX - acquire an exclusive lock

When operation is LOCK_SH or LOCK_EX, it can also be bitwise ORed with
LOCK_NB to avoid blocking on lock acquisition.  If LOCK_NB is used and the
lock cannot be acquired, an OSError will be raised and the exception will
have an errno attribute set to EACCES or EAGAIN (depending on the operating
system -- for portability, check for either value).

`len` is the number of bytes to lock, with the default meaning to lock to
EOF.  `start` is the byte offset, relative to `whence`, to that the lock
starts.  `whence` is as with fileobj.seek(), specifically:

    0 - relative to the start of the file (SEEK_SET)
    1 - relative to the current buffer position (SEEK_CUR)
    2 - relative to the end of the file (SEEK_END)
[clinic start generated code]*/

static PyObject *
fcntl_lockf_impl(PyObject *module, int fd, int code, PyObject *lenobj,
                 PyObject *startobj, int whence)
/*[clinic end generated code: output=4985e7a172e7461a input=3a5dc01b04371f1a]*/
{
    int ret;
    int async_err = 0;

    if (PySys_Audit("fcntl.lockf", "iiOOi", fd, code, lenobj ? lenobj : Py_None,
                    startobj ? startobj : Py_None, whence) < 0) {
        return NULL;
    }

#ifndef LOCK_SH
#define LOCK_SH         1       /* shared lock */
#define LOCK_EX         2       /* exclusive lock */
#define LOCK_NB         4       /* don't block when locking */
#define LOCK_UN         8       /* unlock */
#endif  /* LOCK_SH */
    {
        struct flock l;
        if (code == LOCK_UN)
            l.l_type = F_UNLCK;
        else if (code & LOCK_SH)
            l.l_type = F_RDLCK;
        else if (code & LOCK_EX)
            l.l_type = F_WRLCK;
        else {
            PyErr_SetString(PyExc_ValueError,
                            "unrecognized lockf argument");
            return NULL;
        }
        l.l_start = l.l_len = 0;
        if (startobj != NULL) {
#if !defined(HAVE_LARGEFILE_SUPPORT)
            l.l_start = PyLong_AsLong(startobj);
#else
            l.l_start = PyLong_Check(startobj) ?
                            PyLong_AsLongLong(startobj) :
                    PyLong_AsLong(startobj);
#endif
            if (PyErr_Occurred())
                return NULL;
        }
        if (lenobj != NULL) {
#if !defined(HAVE_LARGEFILE_SUPPORT)
            l.l_len = PyLong_AsLong(lenobj);
#else
            l.l_len = PyLong_Check(lenobj) ?
                            PyLong_AsLongLong(lenobj) :
                    PyLong_AsLong(lenobj);
#endif
            if (PyErr_Occurred())
                return NULL;
        }
        l.l_whence = whence;
        do {
            Py_BEGIN_ALLOW_THREADS
            ret = fcntl(fd, (code & LOCK_NB) ? F_SETLK : F_SETLKW, &l);
            Py_END_ALLOW_THREADS
        } while (ret == -1 && errno == EINTR && !(async_err = PyErr_CheckSignals()));
    }
    if (ret < 0) {
        return !async_err ? PyErr_SetFromErrno(PyExc_OSError) : NULL;
    }
    Py_RETURN_NONE;
}

/* List of functions */

static PyMethodDef fcntl_methods[] = {
    FCNTL_FCNTL_METHODDEF
    FCNTL_IOCTL_METHODDEF
    FCNTL_FLOCK_METHODDEF
    FCNTL_LOCKF_METHODDEF
    {NULL, NULL}  /* sentinel */
};


PyDoc_STRVAR(module_doc,
"This module performs file control and I/O control on file\n\
descriptors.  It is an interface to the fcntl() and ioctl() Unix\n\
routines.  File descriptors can be obtained with the fileno() method of\n\
a file or socket object.");

/* Module initialisation */


static int
all_ins(PyObject* m)
{
    if (PyModule_AddIntMacro(m, LOCK_SH)) return -1;
    if (PyModule_AddIntMacro(m, LOCK_EX)) return -1;
    if (PyModule_AddIntMacro(m, LOCK_NB)) return -1;
    if (PyModule_AddIntMacro(m, LOCK_UN)) return -1;
/* GNU extensions, as of glibc 2.2.4 */
#ifdef LOCK_MAND
    if (PyModule_AddIntMacro(m, LOCK_MAND)) return -1;
#endif
#ifdef LOCK_READ
    if (PyModule_AddIntMacro(m, LOCK_READ)) return -1;
#endif
#ifdef LOCK_WRITE
    if (PyModule_AddIntMacro(m, LOCK_WRITE)) return -1;
#endif
#ifdef LOCK_RW
    if (PyModule_AddIntMacro(m, LOCK_RW)) return -1;
#endif

#ifdef F_DUPFD
    if (PyModule_AddIntMacro(m, F_DUPFD)) return -1;
#endif
#ifdef F_DUPFD_CLOEXEC
    if (PyModule_AddIntMacro(m, F_DUPFD_CLOEXEC)) return -1;
#endif
#ifdef F_GETFD
    if (PyModule_AddIntMacro(m, F_GETFD)) return -1;
#endif
#ifdef F_SETFD
    if (PyModule_AddIntMacro(m, F_SETFD)) return -1;
#endif
#ifdef F_GETFL
    if (PyModule_AddIntMacro(m, F_GETFL)) return -1;
#endif
#ifdef F_SETFL
    if (PyModule_AddIntMacro(m, F_SETFL)) return -1;
#endif
#ifdef F_GETLK
    if (PyModule_AddIntMacro(m, F_GETLK)) return -1;
#endif
#ifdef F_SETLK
    if (PyModule_AddIntMacro(m, F_SETLK)) return -1;
#endif
#ifdef F_SETLKW
    if (PyModule_AddIntMacro(m, F_SETLKW)) return -1;
#endif
#ifdef F_GETOWN
    if (PyModule_AddIntMacro(m, F_GETOWN)) return -1;
#endif
#ifdef F_SETOWN
    if (PyModule_AddIntMacro(m, F_SETOWN)) return -1;
#endif
#ifdef F_GETSIG
    if (PyModule_AddIntMacro(m, F_GETSIG)) return -1;
#endif
#ifdef F_SETSIG
    if (PyModule_AddIntMacro(m, F_SETSIG)) return -1;
#endif
#ifdef F_RDLCK
    if (PyModule_AddIntMacro(m, F_RDLCK)) return -1;
#endif
#ifdef F_WRLCK
    if (PyModule_AddIntMacro(m, F_WRLCK)) return -1;
#endif
#ifdef F_UNLCK
    if (PyModule_AddIntMacro(m, F_UNLCK)) return -1;
#endif
/* LFS constants */
#ifdef F_GETLK64
    if (PyModule_AddIntMacro(m, F_GETLK64)) return -1;
#endif
#ifdef F_SETLK64
    if (PyModule_AddIntMacro(m, F_SETLK64)) return -1;
#endif
#ifdef F_SETLKW64
    if (PyModule_AddIntMacro(m, F_SETLKW64)) return -1;
#endif
/* GNU extensions, as of glibc 2.2.4. */
#ifdef FASYNC
    if (PyModule_AddIntMacro(m, FASYNC)) return -1;
#endif
#ifdef F_SETLEASE
    if (PyModule_AddIntMacro(m, F_SETLEASE)) return -1;
#endif
#ifdef F_GETLEASE
    if (PyModule_AddIntMacro(m, F_GETLEASE)) return -1;
#endif
#ifdef F_NOTIFY
    if (PyModule_AddIntMacro(m, F_NOTIFY)) return -1;
#endif
/* Old BSD flock(). */
#ifdef F_EXLCK
    if (PyModule_AddIntMacro(m, F_EXLCK)) return -1;
#endif
#ifdef F_SHLCK
    if (PyModule_AddIntMacro(m, F_SHLCK)) return -1;
#endif

/* OS X specifics */
#ifdef F_FULLFSYNC
    if (PyModule_AddIntMacro(m, F_FULLFSYNC)) return -1;
#endif
#ifdef F_NOCACHE
    if (PyModule_AddIntMacro(m, F_NOCACHE)) return -1;
#endif

/* For F_{GET|SET}FL */
#ifdef FD_CLOEXEC
    if (PyModule_AddIntMacro(m, FD_CLOEXEC)) return -1;
#endif

/* For F_NOTIFY */
#ifdef DN_ACCESS
    if (PyModule_AddIntMacro(m, DN_ACCESS)) return -1;
#endif
#ifdef DN_MODIFY
    if (PyModule_AddIntMacro(m, DN_MODIFY)) return -1;
#endif
#ifdef DN_CREATE
    if (PyModule_AddIntMacro(m, DN_CREATE)) return -1;
#endif
#ifdef DN_DELETE
    if (PyModule_AddIntMacro(m, DN_DELETE)) return -1;
#endif
#ifdef DN_RENAME
    if (PyModule_AddIntMacro(m, DN_RENAME)) return -1;
#endif
#ifdef DN_ATTRIB
    if (PyModule_AddIntMacro(m, DN_ATTRIB)) return -1;
#endif
#ifdef DN_MULTISHOT
    if (PyModule_AddIntMacro(m, DN_MULTISHOT)) return -1;
#endif

#ifdef HAVE_STROPTS_H
    /* Unix 98 guarantees that these are in stropts.h. */
    if (PyModule_AddIntMacro(m, I_PUSH)) return -1;
    if (PyModule_AddIntMacro(m, I_POP)) return -1;
    if (PyModule_AddIntMacro(m, I_LOOK)) return -1;
    if (PyModule_AddIntMacro(m, I_FLUSH)) return -1;
    if (PyModule_AddIntMacro(m, I_FLUSHBAND)) return -1;
    if (PyModule_AddIntMacro(m, I_SETSIG)) return -1;
    if (PyModule_AddIntMacro(m, I_GETSIG)) return -1;
    if (PyModule_AddIntMacro(m, I_FIND)) return -1;
    if (PyModule_AddIntMacro(m, I_PEEK)) return -1;
    if (PyModule_AddIntMacro(m, I_SRDOPT)) return -1;
    if (PyModule_AddIntMacro(m, I_GRDOPT)) return -1;
    if (PyModule_AddIntMacro(m, I_NREAD)) return -1;
    if (PyModule_AddIntMacro(m, I_FDINSERT)) return -1;
    if (PyModule_AddIntMacro(m, I_STR)) return -1;
    if (PyModule_AddIntMacro(m, I_SWROPT)) return -1;
#ifdef I_GWROPT
    /* despite the comment above, old-ish glibcs miss a couple... */
    if (PyModule_AddIntMacro(m, I_GWROPT)) return -1;
#endif
    if (PyModule_AddIntMacro(m, I_SENDFD)) return -1;
    if (PyModule_AddIntMacro(m, I_RECVFD)) return -1;
    if (PyModule_AddIntMacro(m, I_LIST)) return -1;
    if (PyModule_AddIntMacro(m, I_ATMARK)) return -1;
    if (PyModule_AddIntMacro(m, I_CKBAND)) return -1;
    if (PyModule_AddIntMacro(m, I_GETBAND)) return -1;
    if (PyModule_AddIntMacro(m, I_CANPUT)) return -1;
    if (PyModule_AddIntMacro(m, I_SETCLTIME)) return -1;
#ifdef I_GETCLTIME
    if (PyModule_AddIntMacro(m, I_GETCLTIME)) return -1;
#endif
    if (PyModule_AddIntMacro(m, I_LINK)) return -1;
    if (PyModule_AddIntMacro(m, I_UNLINK)) return -1;
    if (PyModule_AddIntMacro(m, I_PLINK)) return -1;
    if (PyModule_AddIntMacro(m, I_PUNLINK)) return -1;
#endif
#ifdef F_ADD_SEALS
    /* Linux: file sealing for memfd_create() */
    if (PyModule_AddIntMacro(m, F_ADD_SEALS)) return -1;
    if (PyModule_AddIntMacro(m, F_GET_SEALS)) return -1;
    if (PyModule_AddIntMacro(m, F_SEAL_SEAL)) return -1;
    if (PyModule_AddIntMacro(m, F_SEAL_SHRINK)) return -1;
    if (PyModule_AddIntMacro(m, F_SEAL_GROW)) return -1;
    if (PyModule_AddIntMacro(m, F_SEAL_WRITE)) return -1;
#endif
    return 0;
}


static struct PyModuleDef fcntlmodule = {
    PyModuleDef_HEAD_INIT,
    "fcntl",
    module_doc,
    -1,
    fcntl_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_fcntl(void)
{
    PyObject *m;

    /* Create the module and add the functions and documentation */
    m = PyModule_Create(&fcntlmodule);
    if (m == NULL)
        return NULL;

    /* Add some symbolic constants to the module */
    if (all_ins(m) < 0)
        return NULL;

    return m;
}
