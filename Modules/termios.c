/* termiosmodule.c -- POSIX terminal I/O module implementation.  */

#include <Python.h>

#define PyInit_termios inittermios

#include <termios.h>

#define BAD "bad termios argument"

static PyObject *TermiosError;

/* termios = tcgetattr(fd)
   termios is
   [iflag, oflag, cflag, lflag, ispeed, ospeed, [cc[0], ..., cc[NCCS]]] 

   Return the attributes of the terminal device.  */

static PyObject *
termios_tcgetattr(self, args)
	PyObject *self;
	PyObject *args;
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
		PyErr_SetFromErrno(TermiosError);

	ispeed = cfgetispeed(&mode);
	ospeed = cfgetospeed(&mode);

	cc = PyList_New(NCCS);
	if (cc == NULL)
		return NULL;
	for (i = 0; i < NCCS; i++) {
		ch = (char)mode.c_cc[i];
		v = PyString_FromStringAndSize(&ch, 1);
		if (v == NULL)
			return NULL;
		PyList_SetItem(cc, i, v);
	}

	/* Convert the MIN and TIME slots to integer.  On some systems, the
	   MIN and TIME slots are the same as the EOF and EOL slots.  So we
	   only do this in noncanonical input mode.  */
	if (mode.c_lflag & ICANON == 0) {
		v = PyInt_FromLong((long)mode.c_cc[VMIN]);
		if (v == NULL)
			return NULL;
		PyList_SetItem(cc, VMIN, v);
		v = PyInt_FromLong((long)mode.c_cc[VTIME]);
		if (v == NULL)
			return NULL;
		PyList_SetItem(cc, VTIME, v);
	}

	v = PyList_New(7);
	PyList_SetItem(v, 0, PyInt_FromLong((long)mode.c_iflag));
	PyList_SetItem(v, 1, PyInt_FromLong((long)mode.c_oflag));
	PyList_SetItem(v, 2, PyInt_FromLong((long)mode.c_cflag));
	PyList_SetItem(v, 3, PyInt_FromLong((long)mode.c_lflag));
	PyList_SetItem(v, 4, PyInt_FromLong((long)ispeed));
	PyList_SetItem(v, 5, PyInt_FromLong((long)ospeed));
	PyList_SetItem(v, 6, cc);

	return v;
}

/* tcsetattr(fd, when, termios)
   Set the attributes of the terminal device.  */

static PyObject *
termios_tcsetattr(self, args)
	PyObject *self;
	PyObject *args;
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
	for (i = 0; i < 6; i++)
		if (!PyInt_Check(PyList_GetItem(term, i))) {
			PyErr_SetString(PyExc_TypeError, BAD);
			return NULL;
		}

	mode.c_iflag = (tcflag_t) PyInt_AsLong(PyList_GetItem(term, 0));
	mode.c_oflag = (tcflag_t) PyInt_AsLong(PyList_GetItem(term, 1));
	mode.c_cflag = (tcflag_t) PyInt_AsLong(PyList_GetItem(term, 2));
	mode.c_lflag = (tcflag_t) PyInt_AsLong(PyList_GetItem(term, 3));
	ispeed = (speed_t) PyInt_AsLong(PyList_GetItem(term, 4));
	ospeed = (speed_t) PyInt_AsLong(PyList_GetItem(term, 5));
	cc = PyList_GetItem(term, 6);

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
		PyErr_SetFromErrno(TermiosError);
	if (cfsetospeed(&mode, (speed_t) ospeed) == -1)
		PyErr_SetFromErrno(TermiosError);
	if (tcsetattr(fd, when, &mode) == -1)
		PyErr_SetFromErrno(TermiosError);

	Py_INCREF(Py_None);
	return Py_None;
}

/* tcsendbreak(fd, duration)
   Generate a break condition.  */

static PyObject *
termios_tcsendbreak(self, args)
	PyObject *self;
	PyObject *args;
{
	int fd, duration;

	if (!PyArg_Parse(args, "(ii)", &fd, &duration))
		return NULL;
	if (tcsendbreak(fd, duration) == -1)
		PyErr_SetFromErrno(TermiosError);

	Py_INCREF(Py_None);
	return Py_None;
}

/* tcdrain(fd) 
   Wait until all queued output to the terminal has been 
   transmitted.  */

static PyObject *
termios_tcdrain(self, args)
	PyObject *self;
	PyObject *args;
{
	int fd;

	if (!PyArg_Parse(args, "i", &fd))
		return NULL;
	if (tcdrain(fd) == -1)
		PyErr_SetFromErrno(TermiosError);

	Py_INCREF(Py_None);
	return Py_None;
}

/* tcflush(fd, queue) 
   Clear the input and/or output queues associated with 
   the terminal.  */

static PyObject *
termios_tcflush(self, args)
	PyObject *self;
	PyObject *args;
{
	int fd, queue;

	if (!PyArg_Parse(args, "(ii)", &fd, &queue))
		return NULL;
	if (tcflush(fd, queue) == -1)
		PyErr_SetFromErrno(TermiosError);

	Py_INCREF(Py_None);
	return Py_None;
}

/* tcflow(fd, action) 
   Perform operations relating to XON/XOFF flow control on 
   the terminal.  */

static PyObject *
termios_tcflow(self, args)
	PyObject *self;
	PyObject *args;
{
	int fd, action;

	if (!PyArg_Parse(args, "(ii)", &fd, &action))
		return NULL;
	if (tcflow(fd, action) == -1)
		PyErr_SetFromErrno(TermiosError);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef termios_methods[] =
{
	{"tcgetattr", termios_tcgetattr},
	{"tcsetattr", termios_tcsetattr},
	{"tcsendbreak", termios_tcsendbreak},
	{"tcdrain", termios_tcdrain},
	{"tcflush", termios_tcflush},
	{"tcflow", termios_tcflow},
	{NULL, NULL}
};

void
PyInit_termios()
{
	PyObject *m, *d;

	m = Py_InitModule("termios", termios_methods);

	d = PyModule_GetDict(m);
	TermiosError = Py_BuildValue("s", "termios.error");
	PyDict_SetItemString(d, "error", TermiosError);

	if (PyErr_Occurred())
		Py_FatalError("can't initialize module termios");
}
