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
raise_test_longlong_error(const char* msg)
{
	return raiseTestError("test_longlong_api", msg);
}

#define UNBIND(X)  Py_DECREF(X); (X) = NULL

static PyObject *
test_longlong_api(PyObject* self, PyObject* args)
{
	const int NBITS = SIZEOF_LONG_LONG * 8;
	unsigned LONG_LONG base;
	PyObject *pyresult;
	int i;

        if (!PyArg_ParseTuple(args, ":test_longlong_api"))
                return NULL;


	/* Note:  This test lets PyObjects leak if an error is raised.  Since
	   an error should never be raised, leaks are impossible <wink>. */

	/* Test native -> PyLong -> native roundtrip identity.
	 * Generate all powers of 2, and test them and their negations,
	 * plus the numbers +-1 off from them.
	 */
	base = 1;
	for (i = 0;
	     i < NBITS + 1;  /* on last, base overflows to 0 */
	     ++i, base <<= 1)
	{
		int j;
		for (j = 0; j < 6; ++j) {
			LONG_LONG in, out;
			unsigned LONG_LONG uin, uout;

			/* For 0, 1, 2 use base; for 3, 4, 5 use -base */
			uin = j < 3 ? base
				    : (unsigned LONG_LONG)(-(LONG_LONG)base);

			/* For 0 & 3, subtract 1.
			 * For 1 & 4, leave alone.
			 * For 2 & 5, add 1.
			 */
			uin += (unsigned LONG_LONG)(LONG_LONG)(j % 3 - 1);

			pyresult = PyLong_FromUnsignedLongLong(uin);
			if (pyresult == NULL)
				return raise_test_longlong_error(
					"unsigned unexpected null result");

			uout = PyLong_AsUnsignedLongLong(pyresult);
			if (uout == (unsigned LONG_LONG)-1 && PyErr_Occurred())
				return raise_test_longlong_error(
					"unsigned unexpected -1 result");
			if (uout != uin)
				return raise_test_longlong_error(
					"unsigned output != input");
			UNBIND(pyresult);

			in = (LONG_LONG)uin;
			pyresult = PyLong_FromLongLong(in);
			if (pyresult == NULL)
				return raise_test_longlong_error(
					"signed unexpected null result");

			out = PyLong_AsLongLong(pyresult);
			if (out == (LONG_LONG)-1 && PyErr_Occurred())
				return raise_test_longlong_error(
					"signed unexpected -1 result");
			if (out != in)
				return raise_test_longlong_error(
					"signed output != input");
			UNBIND(pyresult);
		}
	}

	/* Overflow tests.  The loop above ensured that all limit cases that
	 * should not overflow don't overflow, so all we need to do here is
	 * provoke one-over-the-limit cases (not exhaustive, but sharp).
	 */
	{
		PyObject *one, *x, *y;
		LONG_LONG out;
		unsigned LONG_LONG uout;

		one = PyLong_FromLong(1);
		if (one == NULL)
			return raise_test_longlong_error(
				"unexpected NULL from PyLong_FromLong");

		/* Unsigned complains about -1? */
		x = PyNumber_Negative(one);
		if (x == NULL)
			return raise_test_longlong_error(
				"unexpected NULL from PyNumber_Negative");

		uout = PyLong_AsUnsignedLongLong(x);
		if (uout != (unsigned LONG_LONG)-1 || !PyErr_Occurred())
			return raise_test_longlong_error(
				"PyLong_AsUnsignedLongLong(-1) didn't "
				"complain");
		PyErr_Clear();
		UNBIND(x);

		/* Unsigned complains about 2**NBITS? */
		y = PyLong_FromLong((long)NBITS);
		if (y == NULL)
			return raise_test_longlong_error(
				"unexpected NULL from PyLong_FromLong");

		x = PyNumber_Lshift(one, y); /* 1L << NBITS, == 2**NBITS */
		UNBIND(y);
		if (x == NULL)
			return raise_test_longlong_error(
				"unexpected NULL from PyNumber_Lshift");

  		uout = PyLong_AsUnsignedLongLong(x);
		if (uout != (unsigned LONG_LONG)-1 || !PyErr_Occurred())
			return raise_test_longlong_error(
				"PyLong_AsUnsignedLongLong(2**NBITS) didn't "
				"complain");
		PyErr_Clear();

		/* Signed complains about 2**(NBITS-1)?
		   x still has 2**NBITS. */
		y = PyNumber_Rshift(x, one); /* 2**(NBITS-1) */
		UNBIND(x);
		if (y == NULL)
			return raise_test_longlong_error(
				"unexpected NULL from PyNumber_Rshift");

		out = PyLong_AsLongLong(y);
		if (out != (LONG_LONG)-1 || !PyErr_Occurred())
			return raise_test_longlong_error(
				"PyLong_AsLongLong(2**(NBITS-1)) didn't "
				"complain");
		PyErr_Clear();

		/* Signed complains about -2**(NBITS-1)-1?;
		   y still has 2**(NBITS-1). */
		x = PyNumber_Negative(y);  /* -(2**(NBITS-1)) */
		UNBIND(y);
		if (x == NULL)
			return raise_test_longlong_error(
				"unexpected NULL from PyNumber_Negative");

		y = PyNumber_Subtract(x, one); /* -(2**(NBITS-1))-1 */
		UNBIND(x);
		if (y == NULL)
			return raise_test_longlong_error(
				"unexpected NULL from PyNumber_Subtract");

		out = PyLong_AsLongLong(y);
		if (out != (LONG_LONG)-1 || !PyErr_Occurred())
			return raise_test_longlong_error(
				"PyLong_AsLongLong(-2**(NBITS-1)-1) didn't "
				"complain");
		PyErr_Clear();
		UNBIND(y);

		Py_XDECREF(x);
		Py_XDECREF(y);
		Py_DECREF(one);
	}

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
