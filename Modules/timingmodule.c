/*
 * Author: George V. Neville-Neil
 */

#include "Python.h"

/* Our stuff... */
#include "timing.h"

static PyObject *
start_timing(PyObject *self, PyObject *args)
{
	if (!PyArg_Parse(args, ""))
		return NULL;

	Py_INCREF(Py_None);
	BEGINTIMING;
	return Py_None;
}

static PyObject *
finish_timing(PyObject *self, PyObject *args)
{
	if (!PyArg_Parse(args, ""))
		return NULL;

	ENDTIMING    
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
seconds(PyObject *self, PyObject *args)
{
	if (!PyArg_Parse(args, ""))
		return NULL;

	return PyInt_FromLong(TIMINGS);

}

static PyObject *
milli(PyObject *self, PyObject *args)
{
	if (!PyArg_Parse(args, ""))
		return NULL;

	return PyInt_FromLong(TIMINGMS);

}
static PyObject *
micro(PyObject *self, PyObject *args)
{
	if (!PyArg_Parse(args, ""))
		return NULL;

	return PyInt_FromLong(TIMINGUS);

}


static PyMethodDef timing_methods[] = {
	{"start",   start_timing},
	{"finish",  finish_timing},
	{"seconds", seconds},
	{"milli",   milli},
	{"micro",   micro},
	{NULL,      NULL}
};


DL_EXPORT(void) inittiming(void)
{
	(void)Py_InitModule("timing", timing_methods);
}
