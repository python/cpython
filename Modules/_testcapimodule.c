/*
 * C Extension module to test Python interpreter C APIs.
 *
 * The 'test_*' functions exported by this module are run as part of the
 * standard Python regression test, via Lib/test/test_capi.py.
 */

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include <float.h>
#include "structmember.h"

#ifdef WITH_THREAD
#include "pythread.h"
#endif /* WITH_THREAD */
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
sizeof_error(const char* fatname, const char* typname,
        int expected, int got)
{
	char buf[1024];
	PyOS_snprintf(buf, sizeof(buf),
		"%.200s #define == %d but sizeof(%.200s) == %d",
		fatname, expected, typname, got);
	PyErr_SetString(TestError, buf);
	return (PyObject*)NULL;
}

static PyObject*
test_config(PyObject *self)
{
#define CHECK_SIZEOF(FATNAME, TYPE) \
	    if (FATNAME != sizeof(TYPE)) \
    	    	return sizeof_error(#FATNAME, #TYPE, FATNAME, sizeof(TYPE))

	CHECK_SIZEOF(SIZEOF_SHORT, short);
	CHECK_SIZEOF(SIZEOF_INT, int);
	CHECK_SIZEOF(SIZEOF_LONG, long);
	CHECK_SIZEOF(SIZEOF_VOID_P, void*);
	CHECK_SIZEOF(SIZEOF_TIME_T, time_t);
#ifdef HAVE_LONG_LONG
	CHECK_SIZEOF(SIZEOF_LONG_LONG, PY_LONG_LONG);
#endif

#undef CHECK_SIZEOF

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject*
test_list_api(PyObject *self)
{
	PyObject* list;
	int i;

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
	Py_ssize_t pos = 0, iterations = 0;
	int i;
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
test_dict_iteration(PyObject* self)
{
	int i;

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
   exactly the same" code is needed to test PY_LONG_LONG, but the ubiquitous
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
test_long_api(PyObject* self)
{
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
#define TYPENAME	PY_LONG_LONG
#define F_S_TO_PY	PyLong_FromLongLong
#define F_PY_TO_S	PyLong_AsLongLong
#define F_U_TO_PY	PyLong_FromUnsignedLongLong
#define F_PY_TO_U	PyLong_AsUnsignedLongLong

#include "testcapi_long.h"

static PyObject *
test_longlong_api(PyObject* self, PyObject *args)
{
	return TESTNAME(raise_test_longlong_error);
}

#undef TESTNAME
#undef TYPENAME
#undef F_S_TO_PY
#undef F_PY_TO_S
#undef F_U_TO_PY
#undef F_PY_TO_U

/* Test the L code for PyArg_ParseTuple.  This should deliver a PY_LONG_LONG
   for both long and int arguments.  The test may leak a little memory if
   it fails.
*/
static PyObject *
test_L_code(PyObject *self)
{
	PyObject *tuple, *num;
	PY_LONG_LONG value;

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

/* Test tuple argument processing */
static PyObject *
getargs_tuple(PyObject *self, PyObject *args)
{
	int a, b, c;
	if (!PyArg_ParseTuple(args, "i(ii)", &a, &b, &c))
		return NULL;
	return Py_BuildValue("iii", a, b, c);
}

/* Functions to call PyArg_ParseTuple with integer format codes,
   and return the result.
*/
static PyObject *
getargs_b(PyObject *self, PyObject *args)
{
	unsigned char value;
	if (!PyArg_ParseTuple(args, "b", &value))
		return NULL;
	return PyLong_FromUnsignedLong((unsigned long)value);
}

static PyObject *
getargs_B(PyObject *self, PyObject *args)
{
	unsigned char value;
	if (!PyArg_ParseTuple(args, "B", &value))
		return NULL;
	return PyLong_FromUnsignedLong((unsigned long)value);
}

static PyObject *
getargs_H(PyObject *self, PyObject *args)
{
	unsigned short value;
	if (!PyArg_ParseTuple(args, "H", &value))
		return NULL;
	return PyLong_FromUnsignedLong((unsigned long)value);
}

static PyObject *
getargs_I(PyObject *self, PyObject *args)
{
	unsigned int value;
	if (!PyArg_ParseTuple(args, "I", &value))
		return NULL;
	return PyLong_FromUnsignedLong((unsigned long)value);
}

static PyObject *
getargs_k(PyObject *self, PyObject *args)
{
	unsigned long value;
	if (!PyArg_ParseTuple(args, "k", &value))
		return NULL;
	return PyLong_FromUnsignedLong(value);
}

static PyObject *
getargs_i(PyObject *self, PyObject *args)
{
	int value;
	if (!PyArg_ParseTuple(args, "i", &value))
		return NULL;
	return PyLong_FromLong((long)value);
}

static PyObject *
getargs_l(PyObject *self, PyObject *args)
{
	long value;
	if (!PyArg_ParseTuple(args, "l", &value))
		return NULL;
	return PyLong_FromLong(value);
}

static PyObject *
getargs_n(PyObject *self, PyObject *args)
{
	Py_ssize_t value;
	if (!PyArg_ParseTuple(args, "n", &value))
		return NULL;
	return PyLong_FromSsize_t(value);
}

#ifdef HAVE_LONG_LONG
static PyObject *
getargs_L(PyObject *self, PyObject *args)
{
	PY_LONG_LONG value;
	if (!PyArg_ParseTuple(args, "L", &value))
		return NULL;
	return PyLong_FromLongLong(value);
}

static PyObject *
getargs_K(PyObject *self, PyObject *args)
{
	unsigned PY_LONG_LONG value;
	if (!PyArg_ParseTuple(args, "K", &value))
		return NULL;
	return PyLong_FromUnsignedLongLong(value);
}
#endif

/* This function not only tests the 'k' getargs code, but also the
   PyInt_AsUnsignedLongMask() and PyInt_AsUnsignedLongMask() functions. */
static PyObject *
test_k_code(PyObject *self)
{
	PyObject *tuple, *num;
	unsigned long value;

        tuple = PyTuple_New(1);
        if (tuple == NULL)
        	return NULL;

	/* a number larger than ULONG_MAX even on 64-bit platforms */
        num = PyLong_FromString("FFFFFFFFFFFFFFFFFFFFFFFF", NULL, 16);
        if (num == NULL)
        	return NULL;

	value = PyInt_AsUnsignedLongMask(num);
	if (value != ULONG_MAX)
        	return raiseTestError("test_k_code",
	    "PyInt_AsUnsignedLongMask() returned wrong value for long 0xFFF...FFF");

        PyTuple_SET_ITEM(tuple, 0, num);

        value = 0;
        if (PyArg_ParseTuple(tuple, "k:test_k_code", &value) < 0)
        	return NULL;
        if (value != ULONG_MAX)
        	return raiseTestError("test_k_code",
			"k code returned wrong value for long 0xFFF...FFF");

	Py_DECREF(num);
        num = PyLong_FromString("-FFFFFFFF000000000000000042", NULL, 16);
        if (num == NULL)
        	return NULL;

	value = PyInt_AsUnsignedLongMask(num);
	if (value != (unsigned long)-0x42)
        	return raiseTestError("test_k_code",
	    "PyInt_AsUnsignedLongMask() returned wrong value for long 0xFFF...FFF");

        PyTuple_SET_ITEM(tuple, 0, num);

	value = 0;
        if (PyArg_ParseTuple(tuple, "k:test_k_code", &value) < 0)
        	return NULL;
        if (value != (unsigned long)-0x42)
        	return raiseTestError("test_k_code",
			"k code returned wrong value for long -0xFFF..000042");

	Py_DECREF(tuple);
	Py_INCREF(Py_None);
	return Py_None;
}


/* Test the u and u# codes for PyArg_ParseTuple. May leak memory in case
   of an error.
*/
static PyObject *
test_u_code(PyObject *self)
{
	PyObject *tuple, *obj;
	Py_UNICODE *value;
	Py_ssize_t len;

        tuple = PyTuple_New(1);
        if (tuple == NULL)
        	return NULL;

        obj = PyUnicode_Decode("test", strlen("test"),
			       "ascii", NULL);
        if (obj == NULL)
        	return NULL;

        PyTuple_SET_ITEM(tuple, 0, obj);

        value = 0;
        if (PyArg_ParseTuple(tuple, "u:test_u_code", &value) < 0)
        	return NULL;
        if (value != PyUnicode_AS_UNICODE(obj))
        	return raiseTestError("test_u_code",
			"u code returned wrong value for u'test'");
        value = 0;
        if (PyArg_ParseTuple(tuple, "u#:test_u_code", &value, &len) < 0)
        	return NULL;
        if (value != PyUnicode_AS_UNICODE(obj) ||
	    len != PyUnicode_GET_SIZE(obj))
        	return raiseTestError("test_u_code",
			"u# code returned wrong values for u'test'");

	Py_DECREF(tuple);
	Py_INCREF(Py_None);
	return Py_None;
}

/* Test Z and Z# codes for PyArg_ParseTuple */
static PyObject *
test_Z_code(PyObject *self)
{
	PyObject *tuple, *obj;
	Py_UNICODE *value1, *value2;
	Py_ssize_t len1, len2;

        tuple = PyTuple_New(2);
        if (tuple == NULL)
        	return NULL;

	obj = PyUnicode_FromString("test");
	PyTuple_SET_ITEM(tuple, 0, obj);
	Py_INCREF(Py_None);
	PyTuple_SET_ITEM(tuple, 1, Py_None);

	/* swap values on purpose */
        value1 = NULL;
	value2 = PyUnicode_AS_UNICODE(obj);

	/* Test Z for both values */
        if (PyArg_ParseTuple(tuple, "ZZ:test_Z_code", &value1, &value2) < 0)
		return NULL;
        if (value1 != PyUnicode_AS_UNICODE(obj))
        	return raiseTestError("test_Z_code",
			"Z code returned wrong value for 'test'");
        if (value2 != NULL)
        	return raiseTestError("test_Z_code",
			"Z code returned wrong value for None");

        value1 = NULL;
	value2 = PyUnicode_AS_UNICODE(obj);
	len1 = -1;
	len2 = -1;

	/* Test Z# for both values */
        if (PyArg_ParseTuple(tuple, "Z#Z#:test_Z_code", &value1, &len1, 
			     &value2, &len2) < 0)
        	return NULL;
        if (value1 != PyUnicode_AS_UNICODE(obj) ||
	    len1 != PyUnicode_GET_SIZE(obj))
        	return raiseTestError("test_Z_code",
			"Z# code returned wrong values for 'test'");
        if (value2 != NULL ||
	    len2 != 0)
        	return raiseTestError("test_Z_code",
			"Z# code returned wrong values for None'");

	Py_DECREF(tuple);
	Py_RETURN_NONE;
}

static PyObject *
codec_incrementalencoder(PyObject *self, PyObject *args)
{
	const char *encoding, *errors = NULL;
	if (!PyArg_ParseTuple(args, "s|s:test_incrementalencoder",
			      &encoding, &errors))
		return NULL;
	return PyCodec_IncrementalEncoder(encoding, errors);
}

static PyObject *
codec_incrementaldecoder(PyObject *self, PyObject *args)
{
	const char *encoding, *errors = NULL;
	if (!PyArg_ParseTuple(args, "s|s:test_incrementaldecoder",
			      &encoding, &errors))
		return NULL;
	return PyCodec_IncrementalDecoder(encoding, errors);
}


/* Simple test of _PyLong_NumBits and _PyLong_Sign. */
static PyObject *
test_long_numbits(PyObject *self)
{
	struct triple {
		long input;
		size_t nbits;
		int sign;
	} testcases[] = {{0, 0, 0},
			 {1L, 1, 1},
			 {-1L, 1, -1},
			 {2L, 2, 1},
			 {-2L, 2, -1},
			 {3L, 2, 1},
			 {-3L, 2, -1},
			 {4L, 3, 1},
			 {-4L, 3, -1},
			 {0x7fffL, 15, 1},	/* one Python long digit */
			 {-0x7fffL, 15, -1},
			 {0xffffL, 16, 1},
			 {-0xffffL, 16, -1},
			 {0xfffffffL, 28, 1},
			 {-0xfffffffL, 28, -1}};
	int i;

	for (i = 0; i < sizeof(testcases) / sizeof(struct triple); ++i) {
		PyObject *plong = PyLong_FromLong(testcases[i].input);
		size_t nbits = _PyLong_NumBits(plong);
		int sign = _PyLong_Sign(plong);

		Py_DECREF(plong);
		if (nbits != testcases[i].nbits)
			return raiseTestError("test_long_numbits",
					"wrong result for _PyLong_NumBits");
		if (sign != testcases[i].sign)
			return raiseTestError("test_long_numbits",
					"wrong result for _PyLong_Sign");
	}
	Py_INCREF(Py_None);
	return Py_None;
}

/* Example passing NULLs to PyObject_Str(NULL). */

static PyObject *
test_null_strings(PyObject *self)
{
	PyObject *o1 = PyObject_Str(NULL), *o2 = PyObject_Str(NULL);
	PyObject *tuple = PyTuple_Pack(2, o1, o2);
	Py_XDECREF(o1);
	Py_XDECREF(o2);
	return tuple;
}

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
	Py_DECREF(exc_args);
	return NULL;
}

#ifdef WITH_THREAD

/* test_thread_state spawns a thread of its own, and that thread releases
 * `thread_done` when it's finished.  The driver code has to know when the
 * thread finishes, because the thread uses a PyObject (the callable) that
 * may go away when the driver finishes.  The former lack of this explicit
 * synchronization caused rare segfaults, so rare that they were seen only
 * on a Mac buildbot (although they were possible on any box).
 */
static PyThread_type_lock thread_done = NULL;

static void
_make_call(void *callable)
{
	PyObject *rc;
	PyGILState_STATE s = PyGILState_Ensure();
	rc = PyObject_CallFunction((PyObject *)callable, "");
	Py_XDECREF(rc);
	PyGILState_Release(s);
}

/* Same thing, but releases `thread_done` when it returns.  This variant
 * should be called only from threads spawned by test_thread_state().
 */
static void
_make_call_from_thread(void *callable)
{
	_make_call(callable);
	PyThread_release_lock(thread_done);
}

static PyObject *
test_thread_state(PyObject *self, PyObject *args)
{
	PyObject *fn;

	if (!PyArg_ParseTuple(args, "O:test_thread_state", &fn))
		return NULL;

	/* Ensure Python is set up for threading */
	PyEval_InitThreads();
	thread_done = PyThread_allocate_lock();
	if (thread_done == NULL)
		return PyErr_NoMemory();
	PyThread_acquire_lock(thread_done, 1);

	/* Start a new thread with our callback. */
	PyThread_start_new_thread(_make_call_from_thread, fn);
	/* Make the callback with the thread lock held by this thread */
	_make_call(fn);
	/* Do it all again, but this time with the thread-lock released */
	Py_BEGIN_ALLOW_THREADS
	_make_call(fn);
	PyThread_acquire_lock(thread_done, 1);  /* wait for thread to finish */
	Py_END_ALLOW_THREADS

	/* And once more with and without a thread
	   XXX - should use a lock and work out exactly what we are trying
	   to test <wink>
	*/
	Py_BEGIN_ALLOW_THREADS
	PyThread_start_new_thread(_make_call_from_thread, fn);
	_make_call(fn);
	PyThread_acquire_lock(thread_done, 1);  /* wait for thread to finish */
	Py_END_ALLOW_THREADS

	/* Release lock we acquired above.  This is required on HP-UX. */
	PyThread_release_lock(thread_done);

	PyThread_free_lock(thread_done);
	Py_RETURN_NONE;
}
#endif

/* Some tests of PyUnicode_FromFormat().  This needs more tests. */
static PyObject *
test_string_from_format(PyObject *self, PyObject *args)
{
	PyObject *result;
	char *msg;

#define CHECK_1_FORMAT(FORMAT, TYPE) 			\
	result = PyUnicode_FromFormat(FORMAT, (TYPE)1);	\
	if (result == NULL)				\
		return NULL;				\
	if (strcmp(PyUnicode_AsString(result), "1")) {	\
		msg = FORMAT " failed at 1";		\
		goto Fail;				\
	}						\
	Py_DECREF(result)

	CHECK_1_FORMAT("%d", int);
	CHECK_1_FORMAT("%ld", long);
	/* The z width modifier was added in Python 2.5. */
	CHECK_1_FORMAT("%zd", Py_ssize_t);

	/* The u type code was added in Python 2.5. */
	CHECK_1_FORMAT("%u", unsigned int);
	CHECK_1_FORMAT("%lu", unsigned long);
	CHECK_1_FORMAT("%zu", size_t);

	Py_RETURN_NONE;

 Fail:
 	Py_XDECREF(result);
	return raiseTestError("test_string_from_format", msg);

#undef CHECK_1_FORMAT
}

/* This is here to provide a docstring for test_descr. */
static PyObject *
test_with_docstring(PyObject *self)
{
	Py_RETURN_NONE;
}

#ifdef HAVE_GETTIMEOFDAY
/* Profiling of integer performance */
void print_delta(int test, struct timeval *s, struct timeval *e)
{
	e->tv_sec -= s->tv_sec;
	e->tv_usec -= s->tv_usec;
	if (e->tv_usec < 0) {
		e->tv_sec -=1;
		e->tv_usec += 1000000;
	}
	printf("Test %d: %d.%06ds\n", test, (int)e->tv_sec, (int)e->tv_usec);
}

static PyObject *
profile_int(PyObject *self, PyObject* args)
{
	int i, k;
	struct timeval start, stop;
	PyObject *single, **multiple, *op1, *result;

	/* Test 1: Allocate and immediately deallocate
	   many small integers */
	gettimeofday(&start, NULL);
	for(k=0; k < 20000; k++)
		for(i=0; i < 1000; i++) {
			single = PyInt_FromLong(i);
			Py_DECREF(single);
		}
	gettimeofday(&stop, NULL);
	print_delta(1, &start, &stop);

	/* Test 2: Allocate and immediately deallocate
	   many large integers */
	gettimeofday(&start, NULL);
	for(k=0; k < 20000; k++)
		for(i=0; i < 1000; i++) {
			single = PyInt_FromLong(i+1000000);
			Py_DECREF(single);
		}
	gettimeofday(&stop, NULL);
	print_delta(2, &start, &stop);

	/* Test 3: Allocate a few integers, then release
	   them all simultaneously. */
	multiple = malloc(sizeof(PyObject*) * 1000);
	gettimeofday(&start, NULL);
	for(k=0; k < 20000; k++) {
		for(i=0; i < 1000; i++) {
			multiple[i] = PyInt_FromLong(i+1000000);
		}
		for(i=0; i < 1000; i++) {
			Py_DECREF(multiple[i]);
		}
	}
	gettimeofday(&stop, NULL);
	print_delta(3, &start, &stop);

	/* Test 4: Allocate many integers, then release
	   them all simultaneously. */
	multiple = malloc(sizeof(PyObject*) * 1000000);
	gettimeofday(&start, NULL);
	for(k=0; k < 20; k++) {
		for(i=0; i < 1000000; i++) {
			multiple[i] = PyInt_FromLong(i+1000000);
		}
		for(i=0; i < 1000000; i++) {
			Py_DECREF(multiple[i]);
		}
	}
	gettimeofday(&stop, NULL);
	print_delta(4, &start, &stop);

	/* Test 5: Allocate many integers < 32000 */
	multiple = malloc(sizeof(PyObject*) * 1000000);
	gettimeofday(&start, NULL);
	for(k=0; k < 10; k++) {
		for(i=0; i < 1000000; i++) {
			multiple[i] = PyInt_FromLong(i+1000);
		}
		for(i=0; i < 1000000; i++) {
			Py_DECREF(multiple[i]);
		}
	}
	gettimeofday(&stop, NULL);
	print_delta(5, &start, &stop);

	/* Test 6: Perform small int addition */
	op1 = PyInt_FromLong(1);
	gettimeofday(&start, NULL);
	for(i=0; i < 10000000; i++) {
		result = PyNumber_Add(op1, op1);
		Py_DECREF(result);
	}
	gettimeofday(&stop, NULL);
	Py_DECREF(op1);
	print_delta(6, &start, &stop);

	/* Test 7: Perform medium int addition */
	op1 = PyInt_FromLong(1000);
	gettimeofday(&start, NULL);
	for(i=0; i < 10000000; i++) {
		result = PyNumber_Add(op1, op1);
		Py_DECREF(result);
	}
	gettimeofday(&stop, NULL);
	Py_DECREF(op1);
	print_delta(7, &start, &stop);

	Py_INCREF(Py_None);
	return Py_None;
}
#endif

static PyMethodDef TestMethods[] = {
	{"raise_exception",	raise_exception,		 METH_VARARGS},
	{"test_config",		(PyCFunction)test_config,	 METH_NOARGS},
	{"test_list_api",	(PyCFunction)test_list_api,	 METH_NOARGS},
	{"test_dict_iteration",	(PyCFunction)test_dict_iteration,METH_NOARGS},
	{"test_long_api",	(PyCFunction)test_long_api,	 METH_NOARGS},
	{"test_long_numbits",	(PyCFunction)test_long_numbits,	 METH_NOARGS},
	{"test_k_code",		(PyCFunction)test_k_code,	 METH_NOARGS},
	{"test_null_strings",	(PyCFunction)test_null_strings,	 METH_NOARGS},
	{"test_string_from_format", (PyCFunction)test_string_from_format, METH_NOARGS},
	{"test_with_docstring", (PyCFunction)test_with_docstring, METH_NOARGS,
	 PyDoc_STR("This is a pretty normal docstring.")},

	{"getargs_tuple",	getargs_tuple,			 METH_VARARGS},
	{"getargs_b",		getargs_b,			 METH_VARARGS},
	{"getargs_B",		getargs_B,			 METH_VARARGS},
	{"getargs_H",		getargs_H,			 METH_VARARGS},
	{"getargs_I",		getargs_I,			 METH_VARARGS},
	{"getargs_k",		getargs_k,			 METH_VARARGS},
	{"getargs_i",		getargs_i,			 METH_VARARGS},
	{"getargs_l",		getargs_l,			 METH_VARARGS},
	{"getargs_n",		getargs_n, 			 METH_VARARGS},
#ifdef HAVE_LONG_LONG
	{"getargs_L",		getargs_L,			 METH_VARARGS},
	{"getargs_K",		getargs_K,			 METH_VARARGS},
	{"test_longlong_api",	test_longlong_api,		 METH_NOARGS},
	{"test_L_code",		(PyCFunction)test_L_code,	 METH_NOARGS},
	{"codec_incrementalencoder",
	 (PyCFunction)codec_incrementalencoder,	 METH_VARARGS},
	{"codec_incrementaldecoder",
	 (PyCFunction)codec_incrementaldecoder,	 METH_VARARGS},
#endif
	{"test_u_code",		(PyCFunction)test_u_code,	 METH_NOARGS},
	{"test_Z_code",		(PyCFunction)test_Z_code,	 METH_NOARGS},
#ifdef WITH_THREAD
	{"_test_thread_state",  test_thread_state, 		 METH_VARARGS},
#endif
#ifdef HAVE_GETTIMEOFDAY
	{"profile_int",		profile_int,			METH_NOARGS},
#endif
	{NULL, NULL} /* sentinel */
};

#define AddSym(d, n, f, v) {PyObject *o = f(v); PyDict_SetItemString(d, n, o); Py_DECREF(o);}

typedef struct {
	char byte_member;
	unsigned char ubyte_member;
	short short_member;
	unsigned short ushort_member;
	int int_member;
	unsigned int uint_member;
	long long_member;
	unsigned long ulong_member;
	float float_member;
	double double_member;
#ifdef HAVE_LONG_LONG
	PY_LONG_LONG longlong_member;
	unsigned PY_LONG_LONG ulonglong_member;
#endif
} all_structmembers;

typedef struct {
    PyObject_HEAD
	all_structmembers structmembers;
} test_structmembers;

static struct PyMemberDef test_members[] = {
	{"T_BYTE", T_BYTE, offsetof(test_structmembers, structmembers.byte_member), 0, NULL},
	{"T_UBYTE", T_UBYTE, offsetof(test_structmembers, structmembers.ubyte_member), 0, NULL},
	{"T_SHORT", T_SHORT, offsetof(test_structmembers, structmembers.short_member), 0, NULL},
	{"T_USHORT", T_USHORT, offsetof(test_structmembers, structmembers.ushort_member), 0, NULL},
	{"T_INT", T_INT, offsetof(test_structmembers, structmembers.int_member), 0, NULL},
	{"T_UINT", T_UINT, offsetof(test_structmembers, structmembers.uint_member), 0, NULL},
	{"T_LONG", T_LONG, offsetof(test_structmembers, structmembers.long_member), 0, NULL},
	{"T_ULONG", T_ULONG, offsetof(test_structmembers, structmembers.ulong_member), 0, NULL},
	{"T_FLOAT", T_FLOAT, offsetof(test_structmembers, structmembers.float_member), 0, NULL},
	{"T_DOUBLE", T_DOUBLE, offsetof(test_structmembers, structmembers.double_member), 0, NULL},
#ifdef HAVE_LONG_LONG
	{"T_LONGLONG", T_LONGLONG, offsetof(test_structmembers, structmembers.longlong_member), 0, NULL},
	{"T_ULONGLONG", T_ULONGLONG, offsetof(test_structmembers, structmembers.ulonglong_member), 0, NULL},
#endif
	{NULL}
};


static PyObject *test_structmembers_new(PyTypeObject *type, PyObject *args, PyObject *kwargs){
	static char *keywords[]={"T_BYTE", "T_UBYTE", "T_SHORT", "T_USHORT", "T_INT", "T_UINT",
		"T_LONG", "T_ULONG", "T_FLOAT", "T_DOUBLE",
		#ifdef HAVE_LONG_LONG	
		"T_LONGLONG", "T_ULONGLONG",
		#endif
		NULL};
	static char *fmt="|bBhHiIlkfd"
		#ifdef HAVE_LONG_LONG
		"LK"
		#endif
		;
	test_structmembers *ob=PyObject_New(test_structmembers, type);
	if (ob==NULL)
		return NULL;
	memset(&ob->structmembers, 0, sizeof(all_structmembers));
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, fmt, keywords,
		&ob->structmembers.byte_member, &ob->structmembers.ubyte_member,
		&ob->structmembers.short_member, &ob->structmembers.ushort_member,
		&ob->structmembers.int_member, &ob->structmembers.uint_member, 
		&ob->structmembers.long_member, &ob->structmembers.ulong_member,
		&ob->structmembers.float_member, &ob->structmembers.double_member
		#ifdef HAVE_LONG_LONG
		,&ob->structmembers.longlong_member, &ob->structmembers.ulonglong_member
		#endif
		)){
		Py_DECREF(ob);
		return NULL;
		}
	return (PyObject *)ob;
}

static void test_structmembers_free(PyObject *ob){
	PyObject_FREE(ob);
}

static PyTypeObject test_structmembersType = {
    PyVarObject_HEAD_INIT(NULL, 0)
	"test_structmembersType",
	sizeof(test_structmembers),	/* tp_basicsize */
	0,				/* tp_itemsize */
	test_structmembers_free,	/* destructor tp_dealloc */
	0,				/* tp_print */
	0,				/* tp_getattr */
	0,				/* tp_setattr */
	0,				/* tp_compare */
	0,				/* tp_repr */
	0,				/* tp_as_number */
	0,				/* tp_as_sequence */
	0,				/* tp_as_mapping */
	0,				/* tp_hash */
	0,				/* tp_call */
	0,				/* tp_str */
	PyObject_GenericGetAttr,
	PyObject_GenericSetAttr,
	0,				/* tp_as_buffer */
	0,				/* tp_flags */
	"Type containing all structmember types",
	0,				/* traverseproc tp_traverse */
	0,				/* tp_clear */
	0,				/* tp_richcompare */
	0,				/* tp_weaklistoffset */
	0,				/* tp_iter */
	0,				/* tp_iternext */
	0,				/* tp_methods */
	test_members,	/* tp_members */
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	test_structmembers_new,			/* tp_new */
};


PyMODINIT_FUNC
init_testcapi(void)
{
	PyObject *m;

	m = Py_InitModule("_testcapi", TestMethods);
	if (m == NULL)
		return;

	Py_Type(&test_structmembersType)=&PyType_Type;
	Py_INCREF(&test_structmembersType);
	PyModule_AddObject(m, "test_structmembersType", (PyObject *)&test_structmembersType);

	PyModule_AddObject(m, "CHAR_MAX", PyInt_FromLong(CHAR_MAX));
	PyModule_AddObject(m, "CHAR_MIN", PyInt_FromLong(CHAR_MIN));
	PyModule_AddObject(m, "UCHAR_MAX", PyInt_FromLong(UCHAR_MAX));
	PyModule_AddObject(m, "SHRT_MAX", PyInt_FromLong(SHRT_MAX));
	PyModule_AddObject(m, "SHRT_MIN", PyInt_FromLong(SHRT_MIN));
	PyModule_AddObject(m, "USHRT_MAX", PyInt_FromLong(USHRT_MAX));
	PyModule_AddObject(m, "INT_MAX",  PyLong_FromLong(INT_MAX));
	PyModule_AddObject(m, "INT_MIN",  PyLong_FromLong(INT_MIN));
	PyModule_AddObject(m, "UINT_MAX",  PyLong_FromUnsignedLong(UINT_MAX));
	PyModule_AddObject(m, "LONG_MAX", PyInt_FromLong(LONG_MAX));
	PyModule_AddObject(m, "LONG_MIN", PyInt_FromLong(LONG_MIN));
	PyModule_AddObject(m, "ULONG_MAX", PyLong_FromUnsignedLong(ULONG_MAX));
	PyModule_AddObject(m, "FLT_MAX", PyFloat_FromDouble(FLT_MAX));
	PyModule_AddObject(m, "FLT_MIN", PyFloat_FromDouble(FLT_MIN));
	PyModule_AddObject(m, "DBL_MAX", PyFloat_FromDouble(DBL_MAX));
	PyModule_AddObject(m, "DBL_MIN", PyFloat_FromDouble(DBL_MIN));
	PyModule_AddObject(m, "LLONG_MAX", PyLong_FromLongLong(PY_LLONG_MAX));
	PyModule_AddObject(m, "LLONG_MIN", PyLong_FromLongLong(PY_LLONG_MIN));
	PyModule_AddObject(m, "ULLONG_MAX", PyLong_FromUnsignedLongLong(PY_ULLONG_MAX));
	PyModule_AddObject(m, "PY_SSIZE_T_MAX", PyInt_FromSsize_t(PY_SSIZE_T_MAX));
	PyModule_AddObject(m, "PY_SSIZE_T_MIN", PyInt_FromSsize_t(PY_SSIZE_T_MIN));

	TestError = PyErr_NewException("_testcapi.error", NULL, NULL);
	Py_INCREF(TestError);
	PyModule_AddObject(m, "error", TestError);
}
