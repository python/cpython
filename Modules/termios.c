/* termios.c -- POSIX terminal I/O module implementation.  */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

// On QNX 6, struct termio must be declared by including sys/termio.h
// if TCGETA, TCSETA, TCSETAW, or TCSETAF are used. sys/termio.h must
// be included before termios.h or it will generate an error.
#if defined(HAVE_SYS_TERMIO_H) && !defined(__hpux)
#  include <sys/termio.h>
#endif

// Apparently, on SGI, termios.h won't define CTRL if _XOPEN_SOURCE
// is defined, so we define it here.
#if defined(__sgi)
#  define CTRL(c) ((c)&037)
#endif

#if defined(__sun)
/* We could do better. Check issue-32660 */
#include <sys/filio.h>
#include <sys/sockio.h>
#endif

#include <termios.h>
#include <sys/ioctl.h>
#if defined(__sun) && defined(__SVR4)
#  include <unistd.h>             // ioctl()
#endif

/* HP-UX requires that this be included to pick up MDCD, MCTS, MDSR,
 * MDTR, MRI, and MRTS (apparently used internally by some things
 * defined as macros; these are not used here directly).
 */
#ifdef HAVE_SYS_MODEM_H
#include <sys/modem.h>
#endif
/* HP-UX requires that this be included to pick up TIOCGPGRP and friends */
#ifdef HAVE_SYS_BSDTTY_H
#include <sys/bsdtty.h>
#endif

/*[clinic input]
module termios
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=01105c85d0ca7252]*/

#include "clinic/termios.c.h"

PyDoc_STRVAR(termios__doc__,
"This module provides an interface to the Posix calls for tty I/O control.\n\
For a complete description of these calls, see the Posix or Unix manual\n\
pages. It is only available for those Unix versions that support Posix\n\
termios style tty I/O control.\n\
\n\
All functions in this module take a file descriptor fd as their first\n\
argument. This can be an integer file descriptor, such as returned by\n\
sys.stdin.fileno(), or a file object, such as sys.stdin itself.");

typedef struct {
  PyObject *TermiosError;
} termiosmodulestate;

static inline termiosmodulestate*
get_termios_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (termiosmodulestate *)state;
}

static struct PyModuleDef termiosmodule;

/*[clinic input]
termios.tcgetattr

    fd: fildes
    /

Get the tty attributes for file descriptor fd.

Returns a list [iflag, oflag, cflag, lflag, ispeed, ospeed, cc]
where cc is a list of the tty special characters (each a string of
length 1, except the items with indices VMIN and VTIME, which are
integers when these fields are defined).  The interpretation of the
flags and the speeds as well as the indexing in the cc array must be
done using the symbolic constants defined in this module.
[clinic start generated code]*/

static PyObject *
termios_tcgetattr_impl(PyObject *module, int fd)
/*[clinic end generated code: output=2b3da39db870e629 input=54dad9779ebe74b1]*/
{
    termiosmodulestate *state = PyModule_GetState(module);
    struct termios mode;
    int r;

    Py_BEGIN_ALLOW_THREADS
    r = tcgetattr(fd, &mode);
    Py_END_ALLOW_THREADS
    if (r == -1) {
        return PyErr_SetFromErrno(state->TermiosError);
    }

    speed_t ispeed = cfgetispeed(&mode);
    speed_t ospeed = cfgetospeed(&mode);

    PyObject *cc = PyList_New(NCCS);
    if (cc == NULL) {
        return NULL;
    }

    PyObject *v;
    int i;
    for (i = 0; i < NCCS; i++) {
        char ch = (char)mode.c_cc[i];
        v = PyBytes_FromStringAndSize(&ch, 1);
        if (v == NULL)
            goto err;
        PyList_SET_ITEM(cc, i, v);
    }

    /* Convert the MIN and TIME slots to integer.  On some systems, the
       MIN and TIME slots are the same as the EOF and EOL slots.  So we
       only do this in noncanonical input mode.  */
    if ((mode.c_lflag & ICANON) == 0) {
        v = PyLong_FromLong((long)mode.c_cc[VMIN]);
        if (v == NULL) {
            goto err;
        }
        if (PyList_SetItem(cc, VMIN, v) < 0) {
            goto err;
        }
        v = PyLong_FromLong((long)mode.c_cc[VTIME]);
        if (v == NULL) {
            goto err;
        }
        if (PyList_SetItem(cc, VTIME, v) < 0) {
            goto err;
        }
    }

    if (!(v = PyList_New(7))) {
        goto err;
    }

#define ADD_LONG_ITEM(index, val) \
    do { \
        PyObject *l = PyLong_FromLong((long)val); \
        if (l == NULL) { \
            Py_DECREF(v); \
            goto err; \
        } \
        PyList_SET_ITEM(v, index, l); \
    } while (0)

    ADD_LONG_ITEM(0, mode.c_iflag);
    ADD_LONG_ITEM(1, mode.c_oflag);
    ADD_LONG_ITEM(2, mode.c_cflag);
    ADD_LONG_ITEM(3, mode.c_lflag);
    ADD_LONG_ITEM(4, ispeed);
    ADD_LONG_ITEM(5, ospeed);
#undef ADD_LONG_ITEM

    PyList_SET_ITEM(v, 6, cc);
    return v;
  err:
    Py_DECREF(cc);
    return NULL;
}

/*[clinic input]
termios.tcsetattr

    fd: fildes
    when: int
    attributes as term: object
    /

Set the tty attributes for file descriptor fd.

The attributes to be set are taken from the attributes argument, which
is a list like the one returned by tcgetattr(). The when argument
determines when the attributes are changed: termios.TCSANOW to
change immediately, termios.TCSADRAIN to change after transmitting all
queued output, or termios.TCSAFLUSH to change after transmitting all
queued output and discarding all queued input.
[clinic start generated code]*/

static PyObject *
termios_tcsetattr_impl(PyObject *module, int fd, int when, PyObject *term)
/*[clinic end generated code: output=bcd2b0a7b98a4bf5 input=5dafabdd5a08f018]*/
{
    if (!PyList_Check(term) || PyList_Size(term) != 7) {
        PyErr_SetString(PyExc_TypeError,
                     "tcsetattr, arg 3: must be 7 element list");
        return NULL;
    }

    /* Get the old mode, in case there are any hidden fields... */
    termiosmodulestate *state = PyModule_GetState(module);
    struct termios mode;
    int r;

    Py_BEGIN_ALLOW_THREADS
    r = tcgetattr(fd, &mode);
    Py_END_ALLOW_THREADS
    if (r == -1) {
        return PyErr_SetFromErrno(state->TermiosError);
    }

    speed_t ispeed, ospeed;
#define SET_FROM_LIST(TYPE, VAR, LIST, N) do {  \
    PyObject *item = PyList_GET_ITEM(LIST, N);  \
    long num = PyLong_AsLong(item);             \
    if (num == -1 && PyErr_Occurred()) {        \
        return NULL;                            \
    }                                           \
    VAR = (TYPE)num;                            \
} while (0)

    SET_FROM_LIST(tcflag_t, mode.c_iflag, term, 0);
    SET_FROM_LIST(tcflag_t, mode.c_oflag, term, 1);
    SET_FROM_LIST(tcflag_t, mode.c_cflag, term, 2);
    SET_FROM_LIST(tcflag_t, mode.c_lflag, term, 3);
    SET_FROM_LIST(speed_t, ispeed, term, 4);
    SET_FROM_LIST(speed_t, ospeed, term, 5);
#undef SET_FROM_LIST

    PyObject *cc = PyList_GET_ITEM(term, 6);
    if (!PyList_Check(cc) || PyList_Size(cc) != NCCS) {
        PyErr_Format(PyExc_TypeError,
            "tcsetattr: attributes[6] must be %d element list",
                 NCCS);
        return NULL;
    }

    int i;
    PyObject *v;
    for (i = 0; i < NCCS; i++) {
        v = PyList_GetItem(cc, i);

        if (PyBytes_Check(v) && PyBytes_Size(v) == 1)
            mode.c_cc[i] = (cc_t) * PyBytes_AsString(v);
        else if (PyLong_Check(v)) {
            long num = PyLong_AsLong(v);
            if (num == -1 && PyErr_Occurred()) {
                return NULL;
            }
            mode.c_cc[i] = (cc_t)num;
        }
        else {
            PyErr_SetString(PyExc_TypeError,
     "tcsetattr: elements of attributes must be characters or integers");
                        return NULL;
                }
    }

    if (cfsetispeed(&mode, (speed_t) ispeed) == -1)
        return PyErr_SetFromErrno(state->TermiosError);
    if (cfsetospeed(&mode, (speed_t) ospeed) == -1)
        return PyErr_SetFromErrno(state->TermiosError);

    Py_BEGIN_ALLOW_THREADS
    r = tcsetattr(fd, when, &mode);
    Py_END_ALLOW_THREADS

    if (r == -1)
        return PyErr_SetFromErrno(state->TermiosError);

    Py_RETURN_NONE;
}

/*[clinic input]
termios.tcsendbreak

    fd: fildes
    duration: int
    /

Send a break on file descriptor fd.

A zero duration sends a break for 0.25-0.5 seconds; a nonzero duration
has a system dependent meaning.
[clinic start generated code]*/

static PyObject *
termios_tcsendbreak_impl(PyObject *module, int fd, int duration)
/*[clinic end generated code: output=5945f589b5d3ac66 input=dc2f32417691f8ed]*/
{
    termiosmodulestate *state = PyModule_GetState(module);
    int r;

    Py_BEGIN_ALLOW_THREADS
    r = tcsendbreak(fd, duration);
    Py_END_ALLOW_THREADS

    if (r == -1) {
        return PyErr_SetFromErrno(state->TermiosError);
    }

    Py_RETURN_NONE;
}

/*[clinic input]
termios.tcdrain

    fd: fildes
    /

Wait until all output written to file descriptor fd has been transmitted.
[clinic start generated code]*/

static PyObject *
termios_tcdrain_impl(PyObject *module, int fd)
/*[clinic end generated code: output=5fd86944c6255955 input=c99241b140b32447]*/
{
    termiosmodulestate *state = PyModule_GetState(module);
    int r;

    Py_BEGIN_ALLOW_THREADS
    r = tcdrain(fd);
    Py_END_ALLOW_THREADS

    if (r == -1) {
        return PyErr_SetFromErrno(state->TermiosError);
    }

    Py_RETURN_NONE;
}

/*[clinic input]
termios.tcflush

    fd: fildes
    queue: int
    /

Discard queued data on file descriptor fd.

The queue selector specifies which queue: termios.TCIFLUSH for the input
queue, termios.TCOFLUSH for the output queue, or termios.TCIOFLUSH for
both queues.
[clinic start generated code]*/

static PyObject *
termios_tcflush_impl(PyObject *module, int fd, int queue)
/*[clinic end generated code: output=2424f80312ec2f21 input=0f7d08122ddc07b5]*/
{
    termiosmodulestate *state = PyModule_GetState(module);
    int r;

    Py_BEGIN_ALLOW_THREADS
    r = tcflush(fd, queue);
    Py_END_ALLOW_THREADS

    if (r == -1) {
        return PyErr_SetFromErrno(state->TermiosError);
    }

    Py_RETURN_NONE;
}

/*[clinic input]
termios.tcflow

    fd: fildes
    action: int
    /

Suspend or resume input or output on file descriptor fd.

The action argument can be termios.TCOOFF to suspend output,
termios.TCOON to restart output, termios.TCIOFF to suspend input,
or termios.TCION to restart input.
[clinic start generated code]*/

static PyObject *
termios_tcflow_impl(PyObject *module, int fd, int action)
/*[clinic end generated code: output=afd10928e6ea66eb input=c6aff0640b6efd9c]*/
{
    termiosmodulestate *state = PyModule_GetState(module);
    int r;

    Py_BEGIN_ALLOW_THREADS
    r = tcflow(fd, action);
    Py_END_ALLOW_THREADS

    if (r == -1) {
        return PyErr_SetFromErrno(state->TermiosError);
    }

    Py_RETURN_NONE;
}

/*[clinic input]
termios.tcgetwinsize

    fd: fildes
    /

Get the tty winsize for file descriptor fd.

Returns a tuple (ws_row, ws_col).
[clinic start generated code]*/

static PyObject *
termios_tcgetwinsize_impl(PyObject *module, int fd)
/*[clinic end generated code: output=31825977d5325fb6 input=5706c379d7fd984d]*/
{
#if defined(TIOCGWINSZ)
    termiosmodulestate *state = PyModule_GetState(module);
    struct winsize w;
    int r;

    Py_BEGIN_ALLOW_THREADS
    r = ioctl(fd, TIOCGWINSZ, &w);
    Py_END_ALLOW_THREADS

    if (r == -1) {
        return PyErr_SetFromErrno(state->TermiosError);
    }

    PyObject *v;
    if (!(v = PyTuple_New(2))) {
        return NULL;
    }

    PyTuple_SetItem(v, 0, PyLong_FromLong((long)w.ws_row));
    PyTuple_SetItem(v, 1, PyLong_FromLong((long)w.ws_col));
    if (PyErr_Occurred()) {
        Py_DECREF(v);
        return NULL;
    }
    return v;
#elif defined(TIOCGSIZE)
    termiosmodulestate *state = PyModule_GetState(module);
    struct ttysize s;
    int r;

    Py_BEGIN_ALLOW_THREADS
    r = ioctl(fd, TIOCGSIZE, &s);
    Py_END_ALLOW_THREADS
    if (r == -1) {
        return PyErr_SetFromErrno(state->TermiosError);
    }

    PyObject *v;
    if (!(v = PyTuple_New(2))) {
        return NULL;
    }

    PyTuple_SetItem(v, 0, PyLong_FromLong((long)s.ts_lines));
    PyTuple_SetItem(v, 1, PyLong_FromLong((long)s.ts_cols));
    if (PyErr_Occurred()) {
        Py_DECREF(v);
        return NULL;
    }
    return v;
#else
    PyErr_SetString(PyExc_NotImplementedError,
                    "requires termios.TIOCGWINSZ and/or termios.TIOCGSIZE");
    return NULL;
#endif /* defined(TIOCGWINSZ) */
}

/*[clinic input]
termios.tcsetwinsize

    fd: fildes
    winsize as winsz: object
    /

Set the tty winsize for file descriptor fd.

The winsize to be set is taken from the winsize argument, which
is a two-item tuple (ws_row, ws_col) like the one returned by tcgetwinsize().
[clinic start generated code]*/

static PyObject *
termios_tcsetwinsize_impl(PyObject *module, int fd, PyObject *winsz)
/*[clinic end generated code: output=2ac3c9bb6eda83e1 input=4a06424465b24aee]*/
{
    if (!PySequence_Check(winsz) || PySequence_Size(winsz) != 2) {
        PyErr_SetString(PyExc_TypeError,
                     "tcsetwinsize, arg 2: must be a two-item sequence");
        return NULL;
    }

    PyObject *tmp_item;
    long winsz_0, winsz_1;
    tmp_item = PySequence_GetItem(winsz, 0);
    winsz_0 = PyLong_AsLong(tmp_item);
    if (winsz_0 == -1 && PyErr_Occurred()) {
        Py_XDECREF(tmp_item);
        return NULL;
    }
    Py_XDECREF(tmp_item);
    tmp_item = PySequence_GetItem(winsz, 1);
    winsz_1 = PyLong_AsLong(tmp_item);
    if (winsz_1 == -1 && PyErr_Occurred()) {
        Py_XDECREF(tmp_item);
        return NULL;
    }
    Py_XDECREF(tmp_item);

    termiosmodulestate *state = PyModule_GetState(module);

#if defined(TIOCGWINSZ) && defined(TIOCSWINSZ)
    struct winsize w;
    /* Get the old winsize because it might have
       more fields such as xpixel, ypixel. */
    if (ioctl(fd, TIOCGWINSZ, &w) == -1) {
        return PyErr_SetFromErrno(state->TermiosError);
    }

    w.ws_row = (unsigned short) winsz_0;
    w.ws_col = (unsigned short) winsz_1;
    if ((((long)w.ws_row) != winsz_0) || (((long)w.ws_col) != winsz_1)) {
        PyErr_SetString(PyExc_OverflowError,
                        "winsize value(s) out of range.");
        return NULL;
    }

    int r;
    Py_BEGIN_ALLOW_THREADS
    r = ioctl(fd, TIOCSWINSZ, &w);
    Py_END_ALLOW_THREADS

    if (r == -1) {
        return PyErr_SetFromErrno(state->TermiosError);
    }

    Py_RETURN_NONE;
#elif defined(TIOCGSIZE) && defined(TIOCSSIZE)
    struct ttysize s;
    int r;
    /* Get the old ttysize because it might have more fields. */
    Py_BEGIN_ALLOW_THREADS
    r = ioctl(fd, TIOCGSIZE, &s);
    Py_END_ALLOW_THREADS

    if (r == -1) {
        return PyErr_SetFromErrno(state->TermiosError);
    }

    s.ts_lines = (int) winsz_0;
    s.ts_cols = (int) winsz_1;
    if ((((long)s.ts_lines) != winsz_0) || (((long)s.ts_cols) != winsz_1)) {
        PyErr_SetString(PyExc_OverflowError,
                        "winsize value(s) out of range.");
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    r = ioctl(fd, TIOCSSIZE, &s);
    Py_END_ALLOW_THREADS

    if (r == -1) {
        return PyErr_SetFromErrno(state->TermiosError);
    }

    Py_RETURN_NONE;
#else
    PyErr_SetString(PyExc_NotImplementedError,
                    "requires termios.TIOCGWINSZ, termios.TIOCSWINSZ and/or termios.TIOCGSIZE, termios.TIOCSSIZE");
    return NULL;
#endif /* defined(TIOCGWINSZ) && defined(TIOCSWINSZ) */
}

static PyMethodDef termios_methods[] =
{
    TERMIOS_TCGETATTR_METHODDEF
    TERMIOS_TCSETATTR_METHODDEF
    TERMIOS_TCSENDBREAK_METHODDEF
    TERMIOS_TCDRAIN_METHODDEF
    TERMIOS_TCFLUSH_METHODDEF
    TERMIOS_TCFLOW_METHODDEF
    TERMIOS_TCGETWINSIZE_METHODDEF
    TERMIOS_TCSETWINSIZE_METHODDEF
    {NULL, NULL}
};


#if defined(VSWTCH) && !defined(VSWTC)
#define VSWTC VSWTCH
#endif

#if defined(VSWTC) && !defined(VSWTCH)
#define VSWTCH VSWTC
#endif

static struct constant {
    char *name;
    long value;
} termios_constants[] = {
    /* cfgetospeed(), cfsetospeed() constants */
    {"B0", B0},
    {"B50", B50},
    {"B75", B75},
    {"B110", B110},
    {"B134", B134},
    {"B150", B150},
    {"B200", B200},
    {"B300", B300},
    {"B600", B600},
    {"B1200", B1200},
    {"B1800", B1800},
    {"B2400", B2400},
    {"B4800", B4800},
    {"B9600", B9600},
    {"B19200", B19200},
    {"B38400", B38400},
#ifdef B57600
    {"B57600", B57600},
#endif
#ifdef B115200
    {"B115200", B115200},
#endif
#ifdef B230400
    {"B230400", B230400},
#endif
#ifdef B460800
    {"B460800", B460800},
#endif
#ifdef B500000
    {"B500000", B500000},
#endif
#ifdef B576000
    {"B576000", B576000},
#endif
#ifdef B921600
    {"B921600", B921600},
#endif
#ifdef B1000000
    {"B1000000", B1000000},
#endif
#ifdef B1152000
    {"B1152000", B1152000},
#endif
#ifdef B1500000
    {"B1500000", B1500000},
#endif
#ifdef B2000000
    {"B2000000", B2000000},
#endif
#ifdef B2500000
    {"B2500000", B2500000},
#endif
#ifdef B3000000
    {"B3000000", B3000000},
#endif
#ifdef B3500000
    {"B3500000", B3500000},
#endif
#ifdef B4000000
    {"B4000000", B4000000},
#endif

#ifdef CBAUDEX
    {"CBAUDEX", CBAUDEX},
#endif

    /* tcsetattr() constants */
    {"TCSANOW", TCSANOW},
    {"TCSADRAIN", TCSADRAIN},
    {"TCSAFLUSH", TCSAFLUSH},
#ifdef TCSASOFT
    {"TCSASOFT", TCSASOFT},
#endif

    /* tcflush() constants */
    {"TCIFLUSH", TCIFLUSH},
    {"TCOFLUSH", TCOFLUSH},
    {"TCIOFLUSH", TCIOFLUSH},

    /* tcflow() constants */
    {"TCOOFF", TCOOFF},
    {"TCOON", TCOON},
    {"TCIOFF", TCIOFF},
    {"TCION", TCION},

    /* struct termios.c_iflag constants */
    {"IGNBRK", IGNBRK},
    {"BRKINT", BRKINT},
    {"IGNPAR", IGNPAR},
    {"PARMRK", PARMRK},
    {"INPCK", INPCK},
    {"ISTRIP", ISTRIP},
    {"INLCR", INLCR},
    {"IGNCR", IGNCR},
    {"ICRNL", ICRNL},
#ifdef IUCLC
    {"IUCLC", IUCLC},
#endif
    {"IXON", IXON},
    {"IXANY", IXANY},
    {"IXOFF", IXOFF},
#ifdef IMAXBEL
    {"IMAXBEL", IMAXBEL},
#endif
#ifdef IUTF8
    {"IUTF8", IUTF8},
#endif

    /* struct termios.c_oflag constants */
    {"OPOST", OPOST},
#ifdef OLCUC
    {"OLCUC", OLCUC},
#endif
#ifdef ONLCR
    {"ONLCR", ONLCR},
#endif
#ifdef OCRNL
    {"OCRNL", OCRNL},
#endif
#ifdef ONOCR
    {"ONOCR", ONOCR},
#endif
#ifdef ONLRET
    {"ONLRET", ONLRET},
#endif
#ifdef OFILL
    {"OFILL", OFILL},
#endif
#ifdef OFDEL
    {"OFDEL", OFDEL},
#endif
#ifdef OXTABS
    {"OXTABS", OXTABS},
#endif
#ifdef ONOEOT
    {"ONOEOT", ONOEOT},
#endif
#ifdef NLDLY
    {"NLDLY", NLDLY},
#endif
#ifdef CRDLY
    {"CRDLY", CRDLY},
#endif
#ifdef TABDLY
    {"TABDLY", TABDLY},
#endif
#ifdef BSDLY
    {"BSDLY", BSDLY},
#endif
#ifdef VTDLY
    {"VTDLY", VTDLY},
#endif
#ifdef FFDLY
    {"FFDLY", FFDLY},
#endif

    /* struct termios.c_oflag-related values (delay mask) */
#ifdef NL0
    {"NL0", NL0},
#endif
#ifdef NL1
    {"NL1", NL1},
#endif
#ifdef NL2
    {"NL2", NL2},
#endif
#ifdef NL3
    {"NL3", NL3},
#endif
#ifdef CR0
    {"CR0", CR0},
#endif
#ifdef CR1
    {"CR1", CR1},
#endif
#ifdef CR2
    {"CR2", CR2},
#endif
#ifdef CR3
    {"CR3", CR3},
#endif
#ifdef TAB0
    {"TAB0", TAB0},
#endif
#ifdef TAB1
    {"TAB1", TAB1},
#endif
#ifdef TAB2
    {"TAB2", TAB2},
#endif
#ifdef TAB3
    {"TAB3", TAB3},
#endif
#ifdef XTABS
    {"XTABS", XTABS},
#endif
#ifdef BS0
    {"BS0", BS0},
#endif
#ifdef BS1
    {"BS1", BS1},
#endif
#ifdef VT0
    {"VT0", VT0},
#endif
#ifdef VT1
    {"VT1", VT1},
#endif
#ifdef FF0
    {"FF0", FF0},
#endif
#ifdef FF1
    {"FF1", FF1},
#endif

    /* struct termios.c_cflag constants */
#ifdef CIGNORE
    {"CIGNORE", CIGNORE},
#endif
    {"CSIZE", CSIZE},
    {"CSTOPB", CSTOPB},
    {"CREAD", CREAD},
    {"PARENB", PARENB},
    {"PARODD", PARODD},
    {"HUPCL", HUPCL},
    {"CLOCAL", CLOCAL},
#ifdef CIBAUD
    {"CIBAUD", CIBAUD},
#endif
#ifdef CRTSCTS
    {"CRTSCTS", (long)CRTSCTS},
#endif

#ifdef CRTS_IFLOW
    {"CRTS_IFLOW", CRTS_IFLOW},
#endif
#ifdef CDTR_IFLOW
    {"CDTR_IFLOW", CDTR_IFLOW},
#endif
#ifdef CDSR_OFLOW
    {"CDSR_OFLOW", CDSR_OFLOW},
#endif
#ifdef CCTS_OFLOW
    {"CCTS_OFLOW", CCTS_OFLOW},
#endif
#ifdef CCAR_OFLOW
    {"CCAR_OFLOW", CCAR_OFLOW},
#endif
#ifdef MDMBUF
    {"MDMBUF", MDMBUF},
#endif

    /* struct termios.c_cflag-related values (character size) */
    {"CS5", CS5},
    {"CS6", CS6},
    {"CS7", CS7},
    {"CS8", CS8},

    /* struct termios.c_lflag constants */
#ifdef ALTWERASE
    {"ALTWERASE", ALTWERASE},
#endif
    {"ISIG", ISIG},
    {"ICANON", ICANON},
#ifdef XCASE
    {"XCASE", XCASE},
#endif
    {"ECHO", ECHO},
    {"ECHOE", ECHOE},
    {"ECHOK", ECHOK},
    {"ECHONL", ECHONL},
#ifdef ECHOCTL
    {"ECHOCTL", ECHOCTL},
#endif
#ifdef ECHOPRT
    {"ECHOPRT", ECHOPRT},
#endif
#ifdef ECHOKE
    {"ECHOKE", ECHOKE},
#endif
#ifdef FLUSHO
    {"FLUSHO", FLUSHO},
#endif
#ifdef NOKERNINFO
    {"NOKERNINFO", NOKERNINFO},
#endif
    {"NOFLSH", NOFLSH},
    {"TOSTOP", TOSTOP},
#ifdef PENDIN
    {"PENDIN", PENDIN},
#endif
    {"IEXTEN", IEXTEN},
#ifdef EXTPROC
    {"EXTPROC", EXTPROC},
#endif

    /* indexes into the control chars array returned by tcgetattr() */
    {"VINTR", VINTR},
    {"VQUIT", VQUIT},
    {"VERASE", VERASE},
    {"VKILL", VKILL},
    {"VEOF", VEOF},
    {"VTIME", VTIME},
#ifdef VSTATUS
    {"VSTATUS", VSTATUS},
#endif
    {"VMIN", VMIN},
#ifdef VSWTC
    /* The #defines above ensure that if either is defined, both are,
     * but both may be omitted by the system headers.  ;-(  */
    {"VSWTC", VSWTC},
    {"VSWTCH", VSWTCH},
#endif
    {"VSTART", VSTART},
    {"VSTOP", VSTOP},
    {"VSUSP", VSUSP},
#ifdef VDSUSP
    {"VDSUSP", VDSUSP},
#endif
    {"VEOL", VEOL},
#ifdef VREPRINT
    {"VREPRINT", VREPRINT},
#endif
#ifdef VDISCARD
    {"VDISCARD", VDISCARD},
#endif
#ifdef VWERASE
    {"VWERASE", VWERASE},
#endif
#ifdef VLNEXT
    {"VLNEXT", VLNEXT},
#endif
#ifdef VEOL2
    {"VEOL2", VEOL2},
#endif


#ifdef B7200
    {"B7200", B7200},
#endif
#ifdef B14400
    {"B14400", B14400},
#endif
#ifdef B28800
    {"B28800", B28800},
#endif
#ifdef B76800
    {"B76800", B76800},
#endif
#ifdef B460800
    {"B460800", B460800},
#endif
#ifdef B500000
    {"B500000", B500000},
#endif
#ifdef B576000
    { "B576000", B576000},
#endif
#ifdef B921600
    { "B921600", B921600},
#endif
#ifdef B1000000
    { "B1000000", B1000000},
#endif
#ifdef B1152000
    { "B1152000", B1152000},
#endif
#ifdef B1500000
    { "B1500000", B1500000},
#endif
#ifdef B2000000
    { "B2000000", B2000000},
#endif
#ifdef B2500000
    { "B2500000", B2500000},
#endif
#ifdef B3000000
    { "B3000000", B3000000},
#endif
#ifdef B3500000
    { "B3500000", B3500000},
#endif
#ifdef B4000000
    { "B4000000", B4000000},
#endif
#ifdef CBAUD
    {"CBAUD", CBAUD},
#endif
#ifdef CDEL
    {"CDEL", CDEL},
#endif
#ifdef CDSUSP
    {"CDSUSP", CDSUSP},
#endif
#ifdef CEOF
    {"CEOF", CEOF},
#endif
#ifdef CEOL
    {"CEOL", CEOL},
#endif
#ifdef CEOL2
    {"CEOL2", CEOL2},
#endif
#ifdef CEOT
    {"CEOT", CEOT},
#endif
#ifdef CERASE
    {"CERASE", CERASE},
#endif
#ifdef CESC
    {"CESC", CESC},
#endif
#ifdef CFLUSH
    {"CFLUSH", CFLUSH},
#endif
#ifdef CINTR
    {"CINTR", CINTR},
#endif
#ifdef CKILL
    {"CKILL", CKILL},
#endif
#ifdef CLNEXT
    {"CLNEXT", CLNEXT},
#endif
#ifdef CNUL
    {"CNUL", CNUL},
#endif
#ifdef COMMON
    {"COMMON", COMMON},
#endif
#ifdef CQUIT
    {"CQUIT", CQUIT},
#endif
#ifdef CRPRNT
    {"CRPRNT", CRPRNT},
#endif
#ifdef CSTART
    {"CSTART", CSTART},
#endif
#ifdef CSTOP
    {"CSTOP", CSTOP},
#endif
#ifdef CSUSP
    {"CSUSP", CSUSP},
#endif
#ifdef CSWTCH
    {"CSWTCH", CSWTCH},
#endif
#ifdef CWERASE
    {"CWERASE", CWERASE},
#endif
#ifdef EXTA
    {"EXTA", EXTA},
#endif
#ifdef EXTB
    {"EXTB", EXTB},
#endif
#ifdef FIOASYNC
    {"FIOASYNC", FIOASYNC},
#endif
#ifdef FIOCLEX
    {"FIOCLEX", FIOCLEX},
#endif
#ifdef FIONBIO
    {"FIONBIO", FIONBIO},
#endif
#ifdef FIONCLEX
    {"FIONCLEX", FIONCLEX},
#endif
#ifdef FIONREAD
    {"FIONREAD", FIONREAD},
#endif
#ifdef IBSHIFT
    {"IBSHIFT", IBSHIFT},
#endif
#ifdef INIT_C_CC
    {"INIT_C_CC", INIT_C_CC},
#endif
#ifdef IOCSIZE_MASK
    {"IOCSIZE_MASK", IOCSIZE_MASK},
#endif
#ifdef IOCSIZE_SHIFT
    {"IOCSIZE_SHIFT", IOCSIZE_SHIFT},
#endif
#ifdef NCC
    {"NCC", NCC},
#endif
#ifdef NCCS
    {"NCCS", NCCS},
#endif
#ifdef NSWTCH
    {"NSWTCH", NSWTCH},
#endif
#ifdef N_MOUSE
    {"N_MOUSE", N_MOUSE},
#endif
#ifdef N_PPP
    {"N_PPP", N_PPP},
#endif
#ifdef N_SLIP
    {"N_SLIP", N_SLIP},
#endif
#ifdef N_STRIP
    {"N_STRIP", N_STRIP},
#endif
#ifdef N_TTY
    {"N_TTY", N_TTY},
#endif
#ifdef TCFLSH
    {"TCFLSH", TCFLSH},
#endif
#ifdef TCGETA
    {"TCGETA", TCGETA},
#endif
#ifdef TCGETS
    {"TCGETS", TCGETS},
#endif
#ifdef TCSBRK
    {"TCSBRK", TCSBRK},
#endif
#ifdef TCSBRKP
    {"TCSBRKP", TCSBRKP},
#endif
#ifdef TCSETA
    {"TCSETA", TCSETA},
#endif
#ifdef TCSETAF
    {"TCSETAF", TCSETAF},
#endif
#ifdef TCSETAW
    {"TCSETAW", TCSETAW},
#endif
#ifdef TCSETS
    {"TCSETS", TCSETS},
#endif
#ifdef TCSETSF
    {"TCSETSF", TCSETSF},
#endif
#ifdef TCSETSW
    {"TCSETSW", TCSETSW},
#endif
#ifdef TCXONC
    {"TCXONC", TCXONC},
#endif
#ifdef TIOCCONS
    {"TIOCCONS", TIOCCONS},
#endif
#ifdef TIOCEXCL
    {"TIOCEXCL", TIOCEXCL},
#endif
#ifdef TIOCGETD
    {"TIOCGETD", TIOCGETD},
#endif
#ifdef TIOCGICOUNT
    {"TIOCGICOUNT", TIOCGICOUNT},
#endif
#ifdef TIOCGLCKTRMIOS
    {"TIOCGLCKTRMIOS", TIOCGLCKTRMIOS},
#endif
#ifdef TIOCGPGRP
    {"TIOCGPGRP", TIOCGPGRP},
#endif
#ifdef TIOCGSERIAL
    {"TIOCGSERIAL", TIOCGSERIAL},
#endif
#ifdef TIOCGSIZE
    {"TIOCGSIZE", TIOCGSIZE},
#endif
#ifdef TIOCGSOFTCAR
    {"TIOCGSOFTCAR", TIOCGSOFTCAR},
#endif
#ifdef TIOCGWINSZ
    {"TIOCGWINSZ", TIOCGWINSZ},
#endif
#ifdef TIOCINQ
    {"TIOCINQ", TIOCINQ},
#endif
#ifdef TIOCLINUX
    {"TIOCLINUX", TIOCLINUX},
#endif
#ifdef TIOCMBIC
    {"TIOCMBIC", TIOCMBIC},
#endif
#ifdef TIOCMBIS
    {"TIOCMBIS", TIOCMBIS},
#endif
#ifdef TIOCMGET
    {"TIOCMGET", TIOCMGET},
#endif
#ifdef TIOCMIWAIT
    {"TIOCMIWAIT", TIOCMIWAIT},
#endif
#ifdef TIOCMSET
    {"TIOCMSET", TIOCMSET},
#endif
#ifdef TIOCM_CAR
    {"TIOCM_CAR", TIOCM_CAR},
#endif
#ifdef TIOCM_CD
    {"TIOCM_CD", TIOCM_CD},
#endif
#ifdef TIOCM_CTS
    {"TIOCM_CTS", TIOCM_CTS},
#endif
#ifdef TIOCM_DSR
    {"TIOCM_DSR", TIOCM_DSR},
#endif
#ifdef TIOCM_DTR
    {"TIOCM_DTR", TIOCM_DTR},
#endif
#ifdef TIOCM_LE
    {"TIOCM_LE", TIOCM_LE},
#endif
#ifdef TIOCM_RI
    {"TIOCM_RI", TIOCM_RI},
#endif
#ifdef TIOCM_RNG
    {"TIOCM_RNG", TIOCM_RNG},
#endif
#ifdef TIOCM_RTS
    {"TIOCM_RTS", TIOCM_RTS},
#endif
#ifdef TIOCM_SR
    {"TIOCM_SR", TIOCM_SR},
#endif
#ifdef TIOCM_ST
    {"TIOCM_ST", TIOCM_ST},
#endif
#ifdef TIOCNOTTY
    {"TIOCNOTTY", TIOCNOTTY},
#endif
#ifdef TIOCNXCL
    {"TIOCNXCL", TIOCNXCL},
#endif
#ifdef TIOCOUTQ
    {"TIOCOUTQ", TIOCOUTQ},
#endif
#ifdef TIOCPKT
    {"TIOCPKT", TIOCPKT},
#endif
#ifdef TIOCPKT_DATA
    {"TIOCPKT_DATA", TIOCPKT_DATA},
#endif
#ifdef TIOCPKT_DOSTOP
    {"TIOCPKT_DOSTOP", TIOCPKT_DOSTOP},
#endif
#ifdef TIOCPKT_FLUSHREAD
    {"TIOCPKT_FLUSHREAD", TIOCPKT_FLUSHREAD},
#endif
#ifdef TIOCPKT_FLUSHWRITE
    {"TIOCPKT_FLUSHWRITE", TIOCPKT_FLUSHWRITE},
#endif
#ifdef TIOCPKT_NOSTOP
    {"TIOCPKT_NOSTOP", TIOCPKT_NOSTOP},
#endif
#ifdef TIOCPKT_START
    {"TIOCPKT_START", TIOCPKT_START},
#endif
#ifdef TIOCPKT_STOP
    {"TIOCPKT_STOP", TIOCPKT_STOP},
#endif
#ifdef TIOCSCTTY
    {"TIOCSCTTY", TIOCSCTTY},
#endif
#ifdef TIOCSERCONFIG
    {"TIOCSERCONFIG", TIOCSERCONFIG},
#endif
#ifdef TIOCSERGETLSR
    {"TIOCSERGETLSR", TIOCSERGETLSR},
#endif
#ifdef TIOCSERGETMULTI
    {"TIOCSERGETMULTI", TIOCSERGETMULTI},
#endif
#ifdef TIOCSERGSTRUCT
    {"TIOCSERGSTRUCT", TIOCSERGSTRUCT},
#endif
#ifdef TIOCSERGWILD
    {"TIOCSERGWILD", TIOCSERGWILD},
#endif
#ifdef TIOCSERSETMULTI
    {"TIOCSERSETMULTI", TIOCSERSETMULTI},
#endif
#ifdef TIOCSERSWILD
    {"TIOCSERSWILD", TIOCSERSWILD},
#endif
#ifdef TIOCSER_TEMT
    {"TIOCSER_TEMT", TIOCSER_TEMT},
#endif
#ifdef TIOCSETD
    {"TIOCSETD", TIOCSETD},
#endif
#ifdef TIOCSLCKTRMIOS
    {"TIOCSLCKTRMIOS", TIOCSLCKTRMIOS},
#endif
#ifdef TIOCSPGRP
    {"TIOCSPGRP", TIOCSPGRP},
#endif
#ifdef TIOCSSERIAL
    {"TIOCSSERIAL", TIOCSSERIAL},
#endif
#ifdef TIOCSSIZE
    {"TIOCSSIZE", TIOCSSIZE},
#endif
#ifdef TIOCSSOFTCAR
    {"TIOCSSOFTCAR", TIOCSSOFTCAR},
#endif
#ifdef TIOCSTI
    {"TIOCSTI", TIOCSTI},
#endif
#ifdef TIOCSWINSZ
    {"TIOCSWINSZ", TIOCSWINSZ},
#endif
#ifdef TIOCTTYGSTRUCT
    {"TIOCTTYGSTRUCT", TIOCTTYGSTRUCT},
#endif

    /* sentinel */
    {NULL, 0}
};

static int termiosmodule_traverse(PyObject *m, visitproc visit, void *arg) {
    Py_VISIT(get_termios_state(m)->TermiosError);
    return 0;
}

static int termiosmodule_clear(PyObject *m) {
    Py_CLEAR(get_termios_state(m)->TermiosError);
    return 0;
}

static void termiosmodule_free(void *m) {
    termiosmodule_clear((PyObject *)m);
}

static int
termios_exec(PyObject *mod)
{
    struct constant *constant = termios_constants;
    termiosmodulestate *state = get_termios_state(mod);
    state->TermiosError = PyErr_NewException("termios.error", NULL, NULL);
    if (PyModule_AddObjectRef(mod, "error", state->TermiosError) < 0) {
        return -1;
    }

    while (constant->name != NULL) {
        if (PyModule_AddIntConstant(
            mod, constant->name, constant->value) < 0) {
            return -1;
        }
        ++constant;
    }
    return 0;
}

static PyModuleDef_Slot termios_slots[] = {
    {Py_mod_exec, termios_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL}
};

static struct PyModuleDef termiosmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "termios",
    .m_doc = termios__doc__,
    .m_size = sizeof(termiosmodulestate),
    .m_methods = termios_methods,
    .m_slots = termios_slots,
    .m_traverse = termiosmodule_traverse,
    .m_clear = termiosmodule_clear,
    .m_free = termiosmodule_free,
};

PyMODINIT_FUNC PyInit_termios(void)
{
    return PyModuleDef_Init(&termiosmodule);
}
