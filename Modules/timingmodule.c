/*
 * Author: George V. Neville-Neil
 */

#include "Python.h"

/* Our stuff... */
#include "timing.h"

static PyObject *
start_timing(PyObject *self)
{
	Py_INCREF(Py_None);
	BEGINTIMING;
	return Py_None;
}

static PyObject *
finish_timing(PyObject *self)
{
	ENDTIMING    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
seconds(PyObject *self)
{
	return PyInt_FromLong(TIMINGS);
}

static PyObject *
milli(PyObject *self)
{
	return PyInt_FromLong(TIMINGMS);
}

static PyObject *
micro(PyObject *self)
{
	return PyInt_FromLong(TIMINGUS);
}


static PyMethodDef timing_methods[] = {
	{"start",   (PyCFunction)start_timing, METH_NOARGS},
	{"finish",  (PyCFunction)finish_timing, METH_NOARGS},
	{"seconds", (PyCFunction)seconds, METH_NOARGS},
	{"milli",   (PyCFunction)milli, METH_NOARGS},
	{"micro",   (PyCFunction)micro, METH_NOARGS},
	{NULL,      NULL}
};


PyMODINIT_FUNC inittiming(void)
{
	(void)Py_InitModule("timing", timing_methods);
}
