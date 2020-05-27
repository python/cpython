/* termiosmodule.c -- POSIX terminal I/O module implementation.  */

#include "Python.h"

/* Apparently, on SGI, termios.h won't define CTRL if _XOPEN_SOURCE
   is defined, so we define it here. */
#if defined(__sgi)
#define CTRL(c) ((c)&037)
#endif

#if defined(__sun)
/* We could do better. Check issue-32660 */
#include <sys/filio.h>
#include <sys/sockio.h>
#endif

#include <termios.h>
#include <sys/ioctl.h>

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

#define modulestate_global get_termios_state(PyState_FindModule(&termiosmodule))

static int fdconv(PyObject* obj, void* p)
{
    int fd;

    fd = PyObject_AsFileDescriptor(obj);
    if (fd >= 0) {
        *(int*)p = fd;
        return 1;
    }
    return 0;
}

static struct PyModuleDef termiosmodule;

PyDoc_STRVAR(termios_tcgetattr__doc__,
"tcgetattr(fd) -> list_of_attrs\n\
\n\
Get the tty attributes for file descriptor fd, as follows:\n\
[iflag, oflag, cflag, lflag, ispeed, ospeed, cc] where cc is a list\n\
of the tty special characters (each a string of length 1, except the items\n\
with indices VMIN and VTIME, which are integers when these fields are\n\
defined).  The interpretation of the flags and the speeds as well as the\n\
indexing in the cc array must be done using the symbolic constants defined\n\
in this module.");

static PyObject *
termios_tcgetattr(PyObject *self, PyObject *args)
{
    int fd;
    struct termios mode;
    PyObject *cc;
    speed_t ispeed, ospeed;
    PyObject *v;
    int i;
    char ch;

    if (!PyArg_ParseTuple(args, "O&:tcgetattr",
                          fdconv, (void*)&fd))
        return NULL;

    if (tcgetattr(fd, &mode) == -1)
        return PyErr_SetFromErrno(modulestate_global->TermiosError);

    ispeed = cfgetispeed(&mode);
    ospeed = cfgetospeed(&mode);

    cc = PyList_New(NCCS);
    if (cc == NULL)
        return NULL;
    for (i = 0; i < NCCS; i++) {
        ch = (char)mode.c_cc[i];
        v = PyBytes_FromStringAndSize(&ch, 1);
        if (v == NULL)
            goto err;
        PyList_SetItem(cc, i, v);
    }

    /* Convert the MIN and TIME slots to integer.  On some systems, the
       MIN and TIME slots are the same as the EOF and EOL slots.  So we
       only do this in noncanonical input mode.  */
    if ((mode.c_lflag & ICANON) == 0) {
        v = PyLong_FromLong((long)mode.c_cc[VMIN]);
        if (v == NULL)
            goto err;
        PyList_SetItem(cc, VMIN, v);
        v = PyLong_FromLong((long)mode.c_cc[VTIME]);
        if (v == NULL)
            goto err;
        PyList_SetItem(cc, VTIME, v);
    }

    if (!(v = PyList_New(7)))
        goto err;

    PyList_SetItem(v, 0, PyLong_FromLong((long)mode.c_iflag));
    PyList_SetItem(v, 1, PyLong_FromLong((long)mode.c_oflag));
    PyList_SetItem(v, 2, PyLong_FromLong((long)mode.c_cflag));
    PyList_SetItem(v, 3, PyLong_FromLong((long)mode.c_lflag));
    PyList_SetItem(v, 4, PyLong_FromLong((long)ispeed));
    PyList_SetItem(v, 5, PyLong_FromLong((long)ospeed));
    if (PyErr_Occurred()) {
        Py_DECREF(v);
        goto err;
    }
    PyList_SetItem(v, 6, cc);
    return v;
  err:
    Py_DECREF(cc);
    return NULL;
}

PyDoc_STRVAR(termios_tcsetattr__doc__,
"tcsetattr(fd, when, attributes) -> None\n\
\n\
Set the tty attributes for file descriptor fd.\n\
The attributes to be set are taken from the attributes argument, which\n\
is a list like the one returned by tcgetattr(). The when argument\n\
determines when the attributes are changed: termios.TCSANOW to\n\
change immediately, termios.TCSADRAIN to change after transmitting all\n\
queued output, or termios.TCSAFLUSH to change after transmitting all\n\
queued output and discarding all queued input. ");

static PyObject *
termios_tcsetattr(PyObject *self, PyObject *args)
{
    int fd, when;
    struct termios mode;
    speed_t ispeed, ospeed;
    PyObject *term, *cc, *v;
    int i;

    if (!PyArg_ParseTuple(args, "O&iO:tcsetattr",
                          fdconv, &fd, &when, &term))
        return NULL;
    if (!PyList_Check(term) || PyList_Size(term) != 7) {
        PyErr_SetString(PyExc_TypeError,
                     "tcsetattr, arg 3: must be 7 element list");
        return NULL;
    }

    /* Get the old mode, in case there are any hidden fields... */
    termiosmodulestate *state = modulestate_global;
    if (tcgetattr(fd, &mode) == -1)
        return PyErr_SetFromErrno(state->TermiosError);
    mode.c_iflag = (tcflag_t) PyLong_AsLong(PyList_GetItem(term, 0));
    mode.c_oflag = (tcflag_t) PyLong_AsLong(PyList_GetItem(term, 1));
    mode.c_cflag = (tcflag_t) PyLong_AsLong(PyList_GetItem(term, 2));
    mode.c_lflag = (tcflag_t) PyLong_AsLong(PyList_GetItem(term, 3));
    ispeed = (speed_t) PyLong_AsLong(PyList_GetItem(term, 4));
    ospeed = (speed_t) PyLong_AsLong(PyList_GetItem(term, 5));
    cc = PyList_GetItem(term, 6);
    if (PyErr_Occurred())
        return NULL;

    if (!PyList_Check(cc) || PyList_Size(cc) != NCCS) {
        PyErr_Format(PyExc_TypeError,
            "tcsetattr: attributes[6] must be %d element list",
                 NCCS);
        return NULL;
    }

    for (i = 0; i < NCCS; i++) {
        v = PyList_GetItem(cc, i);

        if (PyBytes_Check(v) && PyBytes_Size(v) == 1)
            mode.c_cc[i] = (cc_t) * PyBytes_AsString(v);
        else if (PyLong_Check(v))
            mode.c_cc[i] = (cc_t) PyLong_AsLong(v);
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
    if (tcsetattr(fd, when, &mode) == -1)
        return PyErr_SetFromErrno(state->TermiosError);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(termios_tcsendbreak__doc__,
"tcsendbreak(fd, duration) -> None\n\
\n\
Send a break on file descriptor fd.\n\
A zero duration sends a break for 0.25-0.5 seconds; a nonzero duration\n\
has a system dependent meaning.");

static PyObject *
termios_tcsendbreak(PyObject *self, PyObject *args)
{
    int fd, duration;

    if (!PyArg_ParseTuple(args, "O&i:tcsendbreak",
                          fdconv, &fd, &duration))
        return NULL;
    if (tcsendbreak(fd, duration) == -1)
        return PyErr_SetFromErrno(modulestate_global->TermiosError);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(termios_tcdrain__doc__,
"tcdrain(fd) -> None\n\
\n\
Wait until all output written to file descriptor fd has been transmitted.");

static PyObject *
termios_tcdrain(PyObject *self, PyObject *args)
{
    int fd;

    if (!PyArg_ParseTuple(args, "O&:tcdrain",
                          fdconv, &fd))
        return NULL;
    if (tcdrain(fd) == -1)
        return PyErr_SetFromErrno(modulestate_global->TermiosError);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(termios_tcflush__doc__,
"tcflush(fd, queue) -> None\n\
\n\
Discard queued data on file descriptor fd.\n\
The queue selector specifies which queue: termios.TCIFLUSH for the input\n\
queue, termios.TCOFLUSH for the output queue, or termios.TCIOFLUSH for\n\
both queues. ");

static PyObject *
termios_tcflush(PyObject *self, PyObject *args)
{
    int fd, queue;

    if (!PyArg_ParseTuple(args, "O&i:tcflush",
                          fdconv, &fd, &queue))
        return NULL;
    if (tcflush(fd, queue) == -1)
        return PyErr_SetFromErrno(modulestate_global->TermiosError);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(termios_tcflow__doc__,
"tcflow(fd, action) -> None\n\
\n\
Suspend or resume input or output on file descriptor fd.\n\
The action argument can be termios.TCOOFF to suspend output,\n\
termios.TCOON to restart output, termios.TCIOFF to suspend input,\n\
or termios.TCION to restart input.");

static PyObject *
termios_tcflow(PyObject *self, PyObject *args)
{
    int fd, action;

    if (!PyArg_ParseTuple(args, "O&i:tcflow",
                          fdconv, &fd, &action))
        return NULL;
    if (tcflow(fd, action) == -1)
        return PyErr_SetFromErrno(modulestate_global->TermiosError);

    Py_RETURN_NONE;
}

static PyMethodDef termios_methods[] =
{
    {"tcgetattr", termios_tcgetattr,
     METH_VARARGS, termios_tcgetattr__doc__},
    {"tcsetattr", termios_tcsetattr,
     METH_VARARGS, termios_tcsetattr__doc__},
    {"tcsendbreak", termios_tcsendbreak,
     METH_VARARGS, termios_tcsendbreak__doc__},
    {"tcdrain", termios_tcdrain,
     METH_VARARGS, termios_tcdrain__doc__},
    {"tcflush", termios_tcflush,
     METH_VARARGS, termios_tcflush__doc__},
    {"tcflow", termios_tcflow,
     METH_VARARGS, termios_tcflow__doc__},
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

    /* struct termios.c_cflag-related values (character size) */
    {"CS5", CS5},
    {"CS6", CS6},
    {"CS7", CS7},
    {"CS8", CS8},

    /* struct termios.c_lflag constants */
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
    {"NOFLSH", NOFLSH},
    {"TOSTOP", TOSTOP},
#ifdef PENDIN
    {"PENDIN", PENDIN},
#endif
    {"IEXTEN", IEXTEN},

    /* indexes into the control chars array returned by tcgetattr() */
    {"VINTR", VINTR},
    {"VQUIT", VQUIT},
    {"VERASE", VERASE},
    {"VKILL", VKILL},
    {"VEOF", VEOF},
    {"VTIME", VTIME},
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

static struct PyModuleDef termiosmodule = {
    PyModuleDef_HEAD_INIT,
    "termios",
    termios__doc__,
    sizeof(termiosmodulestate),
    termios_methods,
    NULL,
    termiosmodule_traverse,
    termiosmodule_clear,
    termiosmodule_free,
};

PyMODINIT_FUNC
PyInit_termios(void)
{
    PyObject *m;
    struct constant *constant = termios_constants;

    if ((m = PyState_FindModule(&termiosmodule)) != NULL) {
        Py_INCREF(m);
        return m;
    }

    if ((m = PyModule_Create(&termiosmodule)) == NULL) {
        return NULL;
    }

    termiosmodulestate *state = get_termios_state(m);
    state->TermiosError = PyErr_NewException("termios.error", NULL, NULL);
    if (state->TermiosError == NULL) {
        return NULL;
    }
    Py_INCREF(state->TermiosError);
    PyModule_AddObject(m, "error", state->TermiosError);

    while (constant->name != NULL) {
        PyModule_AddIntConstant(m, constant->name, constant->value);
        ++constant;
    }
    return m;
}
