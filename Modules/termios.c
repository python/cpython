/* termiosmodule.c -- POSIX terminal I/O module implementation.  */

#include "Python.h"

#define PyInit_termios inittermios

#include <termios.h>

static char termios__doc__[] = "\
This module provides an interface to the Posix calls for tty I/O control.\n\
For a complete description of these calls, see the Posix or Unix manual\n\
pages. It is only available for those Unix versions that support Posix\n\
termios style tty I/O control (and then only if configured at installation\n\
time).\n\
\n\
All functions in this module take a file descriptor fd as their first\n\
argument. This must be an integer file descriptor, such as returned by\n\
sys.stdin.fileno().\n\
\n\
This module should be used in conjunction with the TERMIOS module,\n\
which defines the relevant symbolic constants.";


#ifdef __BEOS__
#include <unistd.h>
#endif

#define BAD "bad termios argument"

static PyObject *TermiosError;

/* termios = tcgetattr(fd)
   termios is
   [iflag, oflag, cflag, lflag, ispeed, ospeed, [cc[0], ..., cc[NCCS]]] 

   Return the attributes of the terminal device.  */

static char termios_tcgetattr__doc__[] = "\
tcgetattr(fd) -> list_of_attrs\n\
Get the tty attributes for file descriptor fd, as follows:\n\
[iflag, oflag, cflag, lflag, ispeed, ospeed, cc] where cc is a list\n\
of the tty special characters (each a string of length 1, except the items\n\
with indices VMIN and VTIME, which are integers when these fields are\n\
defined).  The interpretation of the flags and the speeds as well as the\n\
indexing in the cc array must be done using the symbolic constants defined\n\
in the TERMIOS module.";

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

	if (!PyArg_Parse(args, "i", &fd))
		return NULL;

	if (tcgetattr(fd, &mode) == -1)
		return PyErr_SetFromErrno(TermiosError);

	ispeed = cfgetispeed(&mode);
	ospeed = cfgetospeed(&mode);

	cc = PyList_New(NCCS);
	if (cc == NULL)
		return NULL;
	for (i = 0; i < NCCS; i++) {
		ch = (char)mode.c_cc[i];
		v = PyString_FromStringAndSize(&ch, 1);
		if (v == NULL)
			goto err;
		PyList_SetItem(cc, i, v);
	}

	/* Convert the MIN and TIME slots to integer.  On some systems, the
	   MIN and TIME slots are the same as the EOF and EOL slots.  So we
	   only do this in noncanonical input mode.  */
	if ((mode.c_lflag & ICANON) == 0) {
		v = PyInt_FromLong((long)mode.c_cc[VMIN]);
		if (v == NULL)
			goto err;
		PyList_SetItem(cc, VMIN, v);
		v = PyInt_FromLong((long)mode.c_cc[VTIME]);
		if (v == NULL)
			goto err;
		PyList_SetItem(cc, VTIME, v);
	}

	if (!(v = PyList_New(7)))
		goto err;

	PyList_SetItem(v, 0, PyInt_FromLong((long)mode.c_iflag));
	PyList_SetItem(v, 1, PyInt_FromLong((long)mode.c_oflag));
	PyList_SetItem(v, 2, PyInt_FromLong((long)mode.c_cflag));
	PyList_SetItem(v, 3, PyInt_FromLong((long)mode.c_lflag));
	PyList_SetItem(v, 4, PyInt_FromLong((long)ispeed));
	PyList_SetItem(v, 5, PyInt_FromLong((long)ospeed));
	PyList_SetItem(v, 6, cc);
	if (PyErr_Occurred()){
		Py_DECREF(v);
		goto err;
	}
	return v;
  err:
	Py_DECREF(cc);
	return NULL;
}

/* tcsetattr(fd, when, termios)
   Set the attributes of the terminal device.  */

static char termios_tcsetattr__doc__[] = "\
tcsetattr(fd, when, attributes) -> None\n\
Set the tty attributes for file descriptor fd.\n\
The attributes to be set are taken from the attributes argument, which\n\
is a list like the one returned by tcgetattr(). The when argument\n\
determines when the attributes are changed: TERMIOS.TCSANOW to\n\
change immediately, TERMIOS.TCSADRAIN to change after transmitting all\n\
queued output, or TERMIOS.TCSAFLUSH to change after transmitting all\n\
queued output and discarding all queued input. ";

static PyObject *
termios_tcsetattr(PyObject *self, PyObject *args)
{
	int fd, when;
	struct termios mode;
	speed_t ispeed, ospeed;
	PyObject *term, *cc, *v;
	int i;

	if (!PyArg_Parse(args, "(iiO)", &fd, &when, &term))
		return NULL;
	if (!PyList_Check(term) || PyList_Size(term) != 7) {
		PyErr_SetString(PyExc_TypeError, BAD);
		return NULL;
	}

	/* Get the old mode, in case there are any hidden fields... */
	if (tcgetattr(fd, &mode) == -1)
		return PyErr_SetFromErrno(TermiosError);
	mode.c_iflag = (tcflag_t) PyInt_AsLong(PyList_GetItem(term, 0));
	mode.c_oflag = (tcflag_t) PyInt_AsLong(PyList_GetItem(term, 1));
	mode.c_cflag = (tcflag_t) PyInt_AsLong(PyList_GetItem(term, 2));
	mode.c_lflag = (tcflag_t) PyInt_AsLong(PyList_GetItem(term, 3));
	ispeed = (speed_t) PyInt_AsLong(PyList_GetItem(term, 4));
	ospeed = (speed_t) PyInt_AsLong(PyList_GetItem(term, 5));
	cc = PyList_GetItem(term, 6);
	if (PyErr_Occurred())
		return NULL;

	if (!PyList_Check(cc) || PyList_Size(cc) != NCCS) {
		PyErr_SetString(PyExc_TypeError, BAD);
		return NULL;
	}

	for (i = 0; i < NCCS; i++) {
		v = PyList_GetItem(cc, i);

		if (PyString_Check(v) && PyString_Size(v) == 1)
			mode.c_cc[i] = (cc_t) * PyString_AsString(v);
		else if (PyInt_Check(v))
			mode.c_cc[i] = (cc_t) PyInt_AsLong(v);
		else {
			PyErr_SetString(PyExc_TypeError, BAD);
			return NULL;
		}
	}

	if (cfsetispeed(&mode, (speed_t) ispeed) == -1)
		return PyErr_SetFromErrno(TermiosError);
	if (cfsetospeed(&mode, (speed_t) ospeed) == -1)
		return PyErr_SetFromErrno(TermiosError);
	if (tcsetattr(fd, when, &mode) == -1)
		return PyErr_SetFromErrno(TermiosError);

	Py_INCREF(Py_None);
	return Py_None;
}

/* tcsendbreak(fd, duration)
   Generate a break condition.  */

static char termios_tcsendbreak__doc__[] = "\
tcsendbreak(fd, duration) -> None\n\
Send a break on file descriptor fd.\n\
A zero duration sends a break for 0.25-0.5 seconds; a nonzero duration \n\
has a system dependent meaning. ";

static PyObject *
termios_tcsendbreak(PyObject *self, PyObject *args)
{
	int fd, duration;

	if (!PyArg_Parse(args, "(ii)", &fd, &duration))
		return NULL;
	if (tcsendbreak(fd, duration) == -1)
		return PyErr_SetFromErrno(TermiosError);

	Py_INCREF(Py_None);
	return Py_None;
}

/* tcdrain(fd) 
   Wait until all queued output to the terminal has been 
   transmitted.  */

static char termios_tcdrain__doc__[] = "\
tcdrain(fd) -> None\n\
Wait until all output written to file descriptor fd has been transmitted. ";

static PyObject *
termios_tcdrain(PyObject *self, PyObject *args)
{
	int fd;

	if (!PyArg_Parse(args, "i", &fd))
		return NULL;
	if (tcdrain(fd) == -1)
		return PyErr_SetFromErrno(TermiosError);

	Py_INCREF(Py_None);
	return Py_None;
}

/* tcflush(fd, queue) 
   Clear the input and/or output queues associated with 
   the terminal.  */

static char termios_tcflush__doc__[] = "\
tcflush(fd, queue) -> None\n\
Discard queued data on file descriptor fd.\n\
The queue selector specifies which queue: TERMIOS.TCIFLUSH for the input\n\
queue, TERMIOS.TCOFLUSH for the output queue, or TERMIOS.TCIOFLUSH for\n\
both queues. ";

static PyObject *
termios_tcflush(PyObject *self, PyObject *args)
{
	int fd, queue;

	if (!PyArg_Parse(args, "(ii)", &fd, &queue))
		return NULL;
	if (tcflush(fd, queue) == -1)
		return PyErr_SetFromErrno(TermiosError);

	Py_INCREF(Py_None);
	return Py_None;
}

/* tcflow(fd, action) 
   Perform operations relating to XON/XOFF flow control on 
   the terminal.  */

static char termios_tcflow__doc__[] = "\
tcflow(fd, action) -> None\n\
Suspend or resume input or output on file descriptor fd.\n\
The action argument can be TERMIOS.TCOOFF to suspend output,\n\
TERMIOS.TCOON to restart output, TERMIOS.TCIOFF to suspend input,\n\
or TERMIOS.TCION to restart input. ";

static PyObject *
termios_tcflow(PyObject *self, PyObject *args)
{
	int fd, action;

	if (!PyArg_Parse(args, "(ii)", &fd, &action))
		return NULL;
	if (tcflow(fd, action) == -1)
		return PyErr_SetFromErrno(TermiosError);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef termios_methods[] =
{
	{"tcgetattr", termios_tcgetattr, 
	 METH_OLDARGS, termios_tcgetattr__doc__},
	{"tcsetattr", termios_tcsetattr, 
	 METH_OLDARGS, termios_tcsetattr__doc__},
	{"tcsendbreak", termios_tcsendbreak, 
	 METH_OLDARGS, termios_tcsendbreak__doc__},
	{"tcdrain", termios_tcdrain, 
	 METH_OLDARGS, termios_tcdrain__doc__},
	{"tcflush", termios_tcflush, 
	 METH_OLDARGS, termios_tcflush__doc__},
	{"tcflow", termios_tcflow, 
	 METH_OLDARGS, termios_tcflow__doc__},
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
	{"B57600", B57600},
	{"B115200", B115200},
#ifdef B230400
	{"B230400", B230400},
#endif
#ifdef CBAUDEX
	{"CBAUDEX", CBAUDEX},
#endif

	/* tcsetattr() constants */
	{"TCSANOW", TCSANOW},
	{"TCSADRAIN", TCSADRAIN},
	{"TCSAFLUSH", TCSAFLUSH},

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
	{"IMAXBEL", IMAXBEL},

	/* struct termios.c_oflag constants */
	{"OPOST", OPOST},
#ifdef OLCUC
	{"OLCUC", OLCUC},
#endif
	{"ONLCR", ONLCR},
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
	{"ECHOCTL", ECHOCTL},
#ifdef ECHOPRT
	{"ECHOPRT", ECHOPRT},
#endif
	{"ECHOKE", ECHOKE},
	{"FLUSHO", FLUSHO},
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
	{"VREPRINT", VREPRINT},
	{"VDISCARD", VDISCARD},
	{"VWERASE", VWERASE},
	{"VLNEXT", VLNEXT},
	{"VEOL2", VEOL2},

	/* sentinel */
	{NULL, 0}
};


DL_EXPORT(void)
PyInit_termios(void)
{
	PyObject *m, *d;
	struct constant *constant = termios_constants;

	m = Py_InitModule4("termios", termios_methods, termios__doc__,
                           (PyObject *)NULL, PYTHON_API_VERSION);

	d = PyModule_GetDict(m);
	TermiosError = PyErr_NewException("termios.error", NULL, NULL);
	PyDict_SetItemString(d, "error", TermiosError);

	while (constant->name != NULL) {
		PyModule_AddIntConstant(m, constant->name, constant->value);
		++constant;
	}
}
