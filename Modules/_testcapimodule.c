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
		PyOS_snprintf(buf, sizeof(buf), "%s: %s", test_name, msg);
		PyErr_SetString(TestError, buf);
	}
	return NULL;
}

/* Test #defines from pyconfig.h (particularly the SIZEOF_* defines).

   The ones derived from autoconf on the UNIX-like OSes can be relied
   upon (in the absence of sloppy cross-compiling), but the Windows
   platforms have these hardcoded.  Better safe than sorry.
*/
static PyObject*
sizeof_error(const char* fatname, const char* typename,
        int expected, int got)
{
	char buf[1024];
	PyOS_snprintf(buf, sizeof(buf), 
		"%.200s #define == %d but sizeof(%.200s) == %d",
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

	CHECK_SIZEOF(SIZEOF_SHORT, short);
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


/* Tests of PyLong_{As, From}{Unsigned,}Long(), and (#ifdef HAVE_LONG_LONG)
   PyLong_{As, From}{Unsigned,}LongLong().

   Note that the meat of the test is contained in testcapi_long.h.
   This is revolting, but delicate code duplication is worse:  "almost
   exactly the same" code is needed to test LONG_LONG, but the ubiquitous
   dependence on type names makes it impossible to use a parameterized
   function.  A giant macro would be even worse than this.  A C++ template
   would be perfect.

   The "report an error" functions are deliberately not part of the #include
   file:  if the test fails, you can set a breakpoint in the appropriate
   error function directly, and crawl back from there in the debugger.
*/

#define UNBIND(X)  Py_DECREF(X); (X) = NULL

static PyObject *
raise_test_long_error(const char* msg)
{
	return raiseTestError("test_long_api", msg);
}

#define TESTNAME	test_long_api_inner
#define TYPENAME	long
#define F_S_TO_PY	PyLong_FromLong
#define F_PY_TO_S	PyLong_AsLong
#define F_U_TO_PY	PyLong_FromUnsignedLong
#define F_PY_TO_U	PyLong_AsUnsignedLong

#include "testcapi_long.h"

static PyObject *
test_long_api(PyObject* self, PyObject* args)
{
        if (!PyArg_ParseTuple(args, ":test_long_api"))
                return NULL;

	return TESTNAME(raise_test_long_error);
}

#undef TESTNAME
#undef TYPENAME
#undef F_S_TO_PY
#undef F_PY_TO_S
#undef F_U_TO_PY
#undef F_PY_TO_U

#ifdef HAVE_LONG_LONG

static PyObject *
raise_test_longlong_error(const char* msg)
{
	return raiseTestError("test_longlong_api", msg);
}

#define TESTNAME	test_longlong_api_inner
#define TYPENAME	LONG_LONG
#define F_S_TO_PY	PyLong_FromLongLong
#define F_PY_TO_S	PyLong_AsLongLong
#define F_U_TO_PY	PyLong_FromUnsignedLongLong
#define F_PY_TO_U	PyLong_AsUnsignedLongLong

#include "testcapi_long.h"

static PyObject *
test_longlong_api(PyObject* self, PyObject* args)
{
        if (!PyArg_ParseTuple(args, ":test_longlong_api"))
                return NULL;

	return TESTNAME(raise_test_longlong_error);
}

#undef TESTNAME
#undef TYPENAME
#undef F_S_TO_PY
#undef F_PY_TO_S
#undef F_U_TO_PY
#undef F_PY_TO_U

/* Test the L code for PyArg_ParseTuple.  This should deliver a LONG_LONG
   for both long and int arguments.  The test may leak a little memory if
   it fails.
*/
static PyObject *
test_L_code(PyObject *self, PyObject *args)
{
	PyObject *tuple, *num;
	LONG_LONG value;

        if (!PyArg_ParseTuple(args, ":test_L_code"))
                return NULL;

        tuple = PyTuple_New(1);
        if (tuple == NULL)
        	return NULL;

        num = PyLong_FromLong(42);
        if (num == NULL)
        	return NULL;

        PyTuple_SET_ITEM(tuple, 0, num);

        value = -1;
        if (PyArg_ParseTuple(tuple, "L:test_L_code", &value) < 0)
        	return NULL;
        if (value != 42)
        	return raiseTestError("test_L_code",
			"L code returned wrong value for long 42");

	Py_DECREF(num);
        num = PyInt_FromLong(42);
        if (num == NULL)
        	return NULL;

        PyTuple_SET_ITEM(tuple, 0, num);

	value = -1;
        if (PyArg_ParseTuple(tuple, "L:test_L_code", &value) < 0)
        	return NULL;
        if (value != 42)
        	return raiseTestError("test_L_code",
			"L code returned wrong value for int 42");

	Py_DECREF(tuple);
	Py_INCREF(Py_None);
	return Py_None;
}

#endif	/* ifdef HAVE_LONG_LONG */

static PyObject *
raise_exception(PyObject *self, PyObject *args)
{
	PyObject *exc;
	PyObject *exc_args, *v;
	int num_args, i;

	if (!PyArg_ParseTuple(args, "Oi:raise_exception",
			      &exc, &num_args))
		return NULL;

	exc_args = PyTuple_New(num_args);
	if (exc_args == NULL)
		return NULL;
	for (i = 0; i < num_args; ++i) {
		v = PyInt_FromLong(i);
		if (v == NULL) {
			Py_DECREF(exc_args);
			return NULL;
		}
		PyTuple_SET_ITEM(exc_args, i, v);
	}
	PyErr_SetObject(exc, exc_args);
	return NULL;
}

static PyMethodDef TestMethods[] = {
	{"raise_exception",	raise_exception,	METH_VARARGS},
	{"test_config",		test_config,		METH_VARARGS},
	{"test_list_api",	test_list_api,		METH_VARARGS},
	{"test_dict_iteration",	test_dict_iteration,	METH_VARARGS},
	{"test_long_api",	test_long_api,		METH_VARARGS},
#ifdef HAVE_LONG_LONG
	{"test_longlong_api",	test_longlong_api,	METH_VARARGS},
	{"test_L_code",		test_L_code,		METH_VARARGS},
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
