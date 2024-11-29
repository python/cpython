/* fcntl module */

// Need limited C API version 3.13 for PyLong_AsInt()
#include "pyconfig.h"   // Py_GIL_DISABLED
#ifndef Py_GIL_DISABLED
#  define Py_LIMITED_API 0x030d0000
#endif

#include "Python.h"

#include <errno.h>                // EINTR
#include <fcntl.h>                // fcntl()
#include <string.h>               // memcpy()
#include <sys/ioctl.h>            // ioctl()
#ifdef HAVE_SYS_FILE_H
#  include <sys/file.h>           // flock()
#endif
#ifdef HAVE_LINUX_FS_H
#  include <linux/fs.h>
#endif
#ifdef HAVE_STROPTS_H
#  include <stropts.h>            // I_FLUSHBAND
#endif

/*[clinic input]
module fcntl
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=124b58387c158179]*/

#include "clinic/fcntlmodule.c.h"

/*[clinic input]
fcntl.fcntl

    fd: fildes
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
/*[clinic end generated code: output=888fc93b51c295bd input=7955340198e5f334]*/
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

    fd: fildes
    request as code: unsigned_long(bitwise=True)
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
fcntl_ioctl_impl(PyObject *module, int fd, unsigned long code,
                 PyObject *ob_arg, int mutate_arg)
/*[clinic end generated code: output=3d8eb6828666cea1 input=cee70f6a27311e58]*/
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

    if (PySys_Audit("fcntl.ioctl", "ikO", fd, code,
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
            if (ret < 0) {
                PyErr_SetFromErrno(PyExc_OSError);
                PyBuffer_Release(&pstr);
                return NULL;
            }
            PyBuffer_Release(&pstr);
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
                PyErr_SetFromErrno(PyExc_OSError);
                PyBuffer_Release(&pstr);
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

    fd: fildes
    operation as code: int
    /

Perform the lock operation `operation` on file descriptor `fd`.

See the Unix manual page for flock(2) for details (On some systems, this
function is emulated using fcntl()).
[clinic start generated code]*/

static PyObject *
fcntl_flock_impl(PyObject *module, int fd, int code)
/*[clinic end generated code: output=84059e2b37d2fc64 input=0bfc00f795953452]*/
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

    fd: fildes
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
/*[clinic end generated code: output=4985e7a172e7461a input=5480479fc63a04b8]*/
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
#define ADD_INT_MACRO(MOD, INT) do {            \
    if (PyModule_AddIntMacro(MOD, INT) != 0) {  \
        return -1;                              \
    }                                           \
} while(0)
    ADD_INT_MACRO(m, LOCK_SH);
    ADD_INT_MACRO(m, LOCK_EX);
    ADD_INT_MACRO(m, LOCK_NB);
    ADD_INT_MACRO(m, LOCK_UN);
/* GNU extensions, as of glibc 2.2.4 */
#ifdef LOCK_MAND
    ADD_INT_MACRO(m, LOCK_MAND);
#endif
#ifdef LOCK_READ
    ADD_INT_MACRO(m, LOCK_READ);
#endif
#ifdef LOCK_WRITE
    ADD_INT_MACRO(m, LOCK_WRITE);
#endif
#ifdef LOCK_RW
    ADD_INT_MACRO(m, LOCK_RW);
#endif

#ifdef F_DUPFD
    ADD_INT_MACRO(m, F_DUPFD);
#endif
#ifdef F_DUPFD_CLOEXEC
    ADD_INT_MACRO(m, F_DUPFD_CLOEXEC);
#endif
#ifdef F_GETFD
    ADD_INT_MACRO(m, F_GETFD);
#endif
#ifdef F_SETFD
    ADD_INT_MACRO(m, F_SETFD);
#endif
#ifdef F_GETFL
    ADD_INT_MACRO(m, F_GETFL);
#endif
#ifdef F_SETFL
    ADD_INT_MACRO(m, F_SETFL);
#endif
#ifdef F_GETLK
    ADD_INT_MACRO(m, F_GETLK);
#endif
#ifdef F_SETLK
    ADD_INT_MACRO(m, F_SETLK);
#endif
#ifdef F_SETLKW
    ADD_INT_MACRO(m, F_SETLKW);
#endif
#ifdef F_OFD_GETLK
    ADD_INT_MACRO(m, F_OFD_GETLK);
#endif
#ifdef F_OFD_SETLK
    ADD_INT_MACRO(m, F_OFD_SETLK);
#endif
#ifdef F_OFD_SETLKW
    ADD_INT_MACRO(m, F_OFD_SETLKW);
#endif
#ifdef F_GETOWN
    ADD_INT_MACRO(m, F_GETOWN);
#endif
#ifdef F_SETOWN
    ADD_INT_MACRO(m, F_SETOWN);
#endif
#ifdef F_GETPATH
    ADD_INT_MACRO(m, F_GETPATH);
#endif
#ifdef F_GETSIG
    ADD_INT_MACRO(m, F_GETSIG);
#endif
#ifdef F_SETSIG
    ADD_INT_MACRO(m, F_SETSIG);
#endif
#ifdef F_RDLCK
    ADD_INT_MACRO(m, F_RDLCK);
#endif
#ifdef F_WRLCK
    ADD_INT_MACRO(m, F_WRLCK);
#endif
#ifdef F_UNLCK
    ADD_INT_MACRO(m, F_UNLCK);
#endif
/* LFS constants */
#ifdef F_GETLK64
    ADD_INT_MACRO(m, F_GETLK64);
#endif
#ifdef F_SETLK64
    ADD_INT_MACRO(m, F_SETLK64);
#endif
#ifdef F_SETLKW64
    ADD_INT_MACRO(m, F_SETLKW64);
#endif
/* GNU extensions, as of glibc 2.2.4. */
#ifdef FASYNC
    ADD_INT_MACRO(m, FASYNC);
#endif
#ifdef F_SETLEASE
    ADD_INT_MACRO(m, F_SETLEASE);
#endif
#ifdef F_GETLEASE
    ADD_INT_MACRO(m, F_GETLEASE);
#endif
#ifdef F_NOTIFY
    ADD_INT_MACRO(m, F_NOTIFY);
#endif
#ifdef F_DUPFD_QUERY
    ADD_INT_MACRO(m, F_DUPFD_QUERY);
#endif
/* Old BSD flock(). */
#ifdef F_EXLCK
    ADD_INT_MACRO(m, F_EXLCK);
#endif
#ifdef F_SHLCK
    ADD_INT_MACRO(m, F_SHLCK);
#endif

/* Linux specifics */
#ifdef F_SETPIPE_SZ
    ADD_INT_MACRO(m, F_SETPIPE_SZ);
#endif
#ifdef F_GETPIPE_SZ
    ADD_INT_MACRO(m, F_GETPIPE_SZ);
#endif

/* On Android, FICLONE is blocked by SELinux. */
#ifndef __ANDROID__
#ifdef FICLONE
    ADD_INT_MACRO(m, FICLONE);
#endif
#ifdef FICLONERANGE
    ADD_INT_MACRO(m, FICLONERANGE);
#endif
#endif

#ifdef F_GETOWN_EX
    // since Linux 2.6.32
    ADD_INT_MACRO(m, F_GETOWN_EX);
    ADD_INT_MACRO(m, F_SETOWN_EX);
    ADD_INT_MACRO(m, F_OWNER_TID);
    ADD_INT_MACRO(m, F_OWNER_PID);
    ADD_INT_MACRO(m, F_OWNER_PGRP);
#endif
#ifdef F_GET_RW_HINT
    // since Linux 4.13
    ADD_INT_MACRO(m, F_GET_RW_HINT);
    ADD_INT_MACRO(m, F_SET_RW_HINT);
    ADD_INT_MACRO(m, F_GET_FILE_RW_HINT);
    ADD_INT_MACRO(m, F_SET_FILE_RW_HINT);
#ifndef RWH_WRITE_LIFE_NOT_SET  // typo in Linux < 5.5
# define RWH_WRITE_LIFE_NOT_SET RWF_WRITE_LIFE_NOT_SET
#endif
    ADD_INT_MACRO(m, RWH_WRITE_LIFE_NOT_SET);
    ADD_INT_MACRO(m, RWH_WRITE_LIFE_NONE);
    ADD_INT_MACRO(m, RWH_WRITE_LIFE_SHORT);
    ADD_INT_MACRO(m, RWH_WRITE_LIFE_MEDIUM);
    ADD_INT_MACRO(m, RWH_WRITE_LIFE_LONG);
    ADD_INT_MACRO(m, RWH_WRITE_LIFE_EXTREME);
#endif

/* OS X specifics */
#ifdef F_FULLFSYNC
    ADD_INT_MACRO(m, F_FULLFSYNC);
#endif
#ifdef F_NOCACHE
    ADD_INT_MACRO(m, F_NOCACHE);
#endif

/* FreeBSD specifics */
#ifdef F_DUP2FD
    ADD_INT_MACRO(m, F_DUP2FD);
#endif
#ifdef F_DUP2FD_CLOEXEC
    ADD_INT_MACRO(m, F_DUP2FD_CLOEXEC);
#endif
#ifdef F_READAHEAD
    ADD_INT_MACRO(m, F_READAHEAD);
#endif
#ifdef F_RDAHEAD
    ADD_INT_MACRO(m, F_RDAHEAD);
#endif
#ifdef F_ISUNIONSTACK
    ADD_INT_MACRO(m, F_ISUNIONSTACK);
#endif
#ifdef F_KINFO
    ADD_INT_MACRO(m, F_KINFO);
#endif

/* NetBSD specifics */
#ifdef F_CLOSEM
    ADD_INT_MACRO(m, F_CLOSEM);
#endif
#ifdef F_MAXFD
    ADD_INT_MACRO(m, F_MAXFD);
#endif
#ifdef F_GETNOSIGPIPE
    ADD_INT_MACRO(m, F_GETNOSIGPIPE);
#endif
#ifdef F_SETNOSIGPIPE
    ADD_INT_MACRO(m, F_SETNOSIGPIPE);
#endif

/* For F_{GET|SET}FL */
#ifdef FD_CLOEXEC
    ADD_INT_MACRO(m, FD_CLOEXEC);
#endif

/* For F_NOTIFY */
#ifdef DN_ACCESS
    ADD_INT_MACRO(m, DN_ACCESS);
#endif
#ifdef DN_MODIFY
    ADD_INT_MACRO(m, DN_MODIFY);
#endif
#ifdef DN_CREATE
    ADD_INT_MACRO(m, DN_CREATE);
#endif
#ifdef DN_DELETE
    ADD_INT_MACRO(m, DN_DELETE);
#endif
#ifdef DN_RENAME
    ADD_INT_MACRO(m, DN_RENAME);
#endif
#ifdef DN_ATTRIB
    ADD_INT_MACRO(m, DN_ATTRIB);
#endif
#ifdef DN_MULTISHOT
    ADD_INT_MACRO(m, DN_MULTISHOT);
#endif

#ifdef HAVE_STROPTS_H
    /* Unix 98 guarantees that these are in stropts.h. */
    ADD_INT_MACRO(m, I_PUSH);
    ADD_INT_MACRO(m, I_POP);
    ADD_INT_MACRO(m, I_LOOK);
    ADD_INT_MACRO(m, I_FLUSH);
    ADD_INT_MACRO(m, I_FLUSHBAND);
    ADD_INT_MACRO(m, I_SETSIG);
    ADD_INT_MACRO(m, I_GETSIG);
    ADD_INT_MACRO(m, I_FIND);
    ADD_INT_MACRO(m, I_PEEK);
    ADD_INT_MACRO(m, I_SRDOPT);
    ADD_INT_MACRO(m, I_GRDOPT);
    ADD_INT_MACRO(m, I_NREAD);
    ADD_INT_MACRO(m, I_FDINSERT);
    ADD_INT_MACRO(m, I_STR);
    ADD_INT_MACRO(m, I_SWROPT);
#ifdef I_GWROPT
    /* despite the comment above, old-ish glibcs miss a couple... */
    ADD_INT_MACRO(m, I_GWROPT);
#endif
    ADD_INT_MACRO(m, I_SENDFD);
    ADD_INT_MACRO(m, I_RECVFD);
    ADD_INT_MACRO(m, I_LIST);
    ADD_INT_MACRO(m, I_ATMARK);
    ADD_INT_MACRO(m, I_CKBAND);
    ADD_INT_MACRO(m, I_GETBAND);
    ADD_INT_MACRO(m, I_CANPUT);
    ADD_INT_MACRO(m, I_SETCLTIME);
#ifdef I_GETCLTIME
    ADD_INT_MACRO(m, I_GETCLTIME);
#endif
    ADD_INT_MACRO(m, I_LINK);
    ADD_INT_MACRO(m, I_UNLINK);
    ADD_INT_MACRO(m, I_PLINK);
    ADD_INT_MACRO(m, I_PUNLINK);
#endif
#ifdef F_ADD_SEALS
    /* Linux: file sealing for memfd_create() */
    ADD_INT_MACRO(m, F_ADD_SEALS);
    ADD_INT_MACRO(m, F_GET_SEALS);
    ADD_INT_MACRO(m, F_SEAL_SEAL);
    ADD_INT_MACRO(m, F_SEAL_SHRINK);
    ADD_INT_MACRO(m, F_SEAL_GROW);
    ADD_INT_MACRO(m, F_SEAL_WRITE);
#ifdef F_SEAL_FUTURE_WRITE
    ADD_INT_MACRO(m, F_SEAL_FUTURE_WRITE);
#endif
#endif

#undef ADD_INT_MACRO

    return 0;
}

static int
fcntl_exec(PyObject *module)
{
    if (all_ins(module) < 0) {
        return -1;
    }
    return 0;
}

static PyModuleDef_Slot fcntl_slots[] = {
    {Py_mod_exec, fcntl_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef fcntlmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "fcntl",
    .m_doc = module_doc,
    .m_size = 0,
    .m_methods = fcntl_methods,
    .m_slots = fcntl_slots,
};

PyMODINIT_FUNC
PyInit_fcntl(void)
{
    return PyModuleDef_Init(&fcntlmodule);
}
