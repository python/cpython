/*
 * C Extension module to test Python interpreter C APIs.
 *
 * The 'test_*' functions exported by this module are run as part of the
 * standard Python regression test, via Lib/test/test_capi.py.
 */

#include "Python.h"

static PyObject *TestError;	/* set to exception object in init */

/* Raise TestError with test_name + ": " + msg, and return NULL. */

static PyObject *
raiseTestError(const char* test_name, const char* msg)
{
	char buf[2048];

	if (strlen(test_name) + strlen(msg) > sizeof(buf) - 50)
		PyErr_SetString(TestError, "internal error msg too large");
	else {
		sprintf(buf, "%s: %s", test_name, msg);
		PyErr_SetString(TestError, buf);
	}
	return NULL;
}

/* Test #defines from config.h (particularly the SIZEOF_* defines).

   The ones derived from autoconf on the UNIX-like OSes can be relied
   upon (in the absence of sloppy cross-compiling), but the Windows
   platforms have these hardcoded.  Better safe than sorry.
*/
static PyObject*
sizeof_error(const char* fatname, const char* typename,
        int expected, int got)
{
	char buf[1024];
	sprintf(buf, "%s #define == %d but sizeof(%s) == %d",
		fatname, expected, typename, got);
	PyErr_SetString(TestError, buf);
	return (PyObject*)NULL;
}

static PyObject*
test_config(PyObject *self, PyObject *args)
{
        if (!PyArg_ParseTuple(args, ":test_config"))
                return NULL;

#define CHECK_SIZEOF(FATNAME, TYPE) \
	    if (FATNAME != sizeof(TYPE)) \
    	    	return sizeof_error(#FATNAME, #TYPE, FATNAME, sizeof(TYPE))

	CHECK_SIZEOF(SIZEOF_INT, int);
	CHECK_SIZEOF(SIZEOF_LONG, long);
	CHECK_SIZEOF(SIZEOF_VOID_P, void*);
	CHECK_SIZEOF(SIZEOF_TIME_T, time_t);
#ifdef HAVE_LONG_LONG
	CHECK_SIZEOF(SIZEOF_LONG_LONG, LONG_LONG);
#endif

#undef CHECK_SIZEOF

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject*
test_list_api(PyObject *self, PyObject *args)
{
	PyObject* list;
	int i;
        if (!PyArg_ParseTuple(args, ":test_list_api"))
                return NULL;

	/* SF bug 132008:  PyList_Reverse segfaults */
#define NLIST 30
	list = PyList_New(NLIST);
	if (list == (PyObject*)NULL)
		return (PyObject*)NULL;
	/* list = range(NLIST) */
	for (i = 0; i < NLIST; ++i) {
		PyObject* anint = PyInt_FromLong(i);
		if (anint == (PyObject*)NULL) {
			Py_DECREF(list);
			return (PyObject*)NULL;
		}
		PyList_SET_ITEM(list, i, anint);
	}
	/* list.reverse(), via PyList_Reverse() */
	i = PyList_Reverse(list);   /* should not blow up! */
	if (i != 0) {
		Py_DECREF(list);
		return (PyObject*)NULL;
	}
	/* Check that list == range(29, -1, -1) now */
	for (i = 0; i < NLIST; ++i) {
		PyObject* anint = PyList_GET_ITEM(list, i);
		if (PyInt_AS_LONG(anint) != NLIST-1-i) {
			PyErr_SetString(TestError,
			                "test_list_api: reverse screwed up");
			Py_DECREF(list);
			return (PyObject*)NULL;
		}
	}
	Py_DECREF(list);
#undef NLIST

	Py_INCREF(Py_None);
	return Py_None;
}

static int
test_dict_inner(int count)
{
	int pos = 0, iterations = 0, i;
	PyObject *dict = PyDict_New();
	PyObject *v, *k;

	if (dict == NULL)
		return -1;

	for (i = 0; i < count; i++) {
		v = PyInt_FromLong(i);
		PyDict_SetItem(dict, v, v);
		Py_DECREF(v);
	}

	while (PyDict_Next(dict, &pos, &k, &v)) {
		PyObject *o;
		iterations++;

		i = PyInt_AS_LONG(v) + 1;
		o = PyInt_FromLong(i);
		if (o == NULL)
			return -1;
		if (PyDict_SetItem(dict, k, o) < 0) {
			Py_DECREF(o);
			return -1;
		}
		Py_DECREF(o);
	}

	Py_DECREF(dict);

	if (iterations != count) {
		PyErr_SetString(
			TestError,
			"test_dict_iteration: dict iteration went wrong ");
		return -1;
	} else {
		return 0;
	}
}

static PyObject*
test_dict_iteration(PyObject* self, PyObject* args)
{
	int i;

        if (!PyArg_ParseTuple(args, ":test_dict_iteration"))
                return NULL;

	for (i = 0; i < 200; i++) {
		if (test_dict_inner(i) < 0) {
			return NULL;
		}
	}

	Py_INCREF(Py_None);
	return Py_None;
}

#ifdef HAVE_LONG_LONG

/* Basic sanity checks for PyLong_{As, From}{Unsigned,}LongLong(). */

static PyObject *
test_longlong_api(PyObject* self, PyObject* args)
{
	/* unsigned LONG_LONG uinput, uoutput; */
	LONG_LONG input, output;
	PyObject *pyresult;

        if (!PyArg_ParseTuple(args, ":test_longlong_api"))
                return NULL;

	input = 0;
	pyresult = PyLong_FromLongLong(input);
	if (pyresult == NULL)
		return raiseTestError("test_longlong_api",
				      "unexpected null result");
	output = PyLong_AsLongLong(pyresult);
	if (output == (LONG_LONG)-1 && PyErr_Occurred())
		return raiseTestError("test_longlong_api",
				      "unexpected -1 result");
	if (output != input)
		return raiseTestError("test_longlong_api",
				       "output != input");
	Py_DECREF(pyresult);

	Py_INCREF(Py_None);
	return Py_None;
}

#endif

static PyMethodDef TestMethods[] = {
	{"test_config",		test_config,		METH_VARARGS},
	{"test_list_api",	test_list_api,		METH_VARARGS},
	{"test_dict_iteration",	test_dict_iteration,	METH_VARARGS},
#ifdef HAVE_LONG_LONG
	{"test_longlong_api",	test_longlong_api,	METH_VARARGS},
#endif
	{NULL, NULL} /* sentinel */
};

DL_EXPORT(void)
init_testcapi(void)
{
	PyObject *m, *d;

	m = Py_InitModule("_testcapi", TestMethods);

	TestError = PyErr_NewException("_testcapi.error", NULL, NULL);
	d = PyModule_GetDict(m);
	PyDict_SetItemString(d, "error", TestError);
}
