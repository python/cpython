/*
 * C Extension module to test Python interpreter C APIs.
 *
 * The 'test_*' functions exported by this module are run as part of the
 * standard Python regression test, via Lib/test/test_capi.py.
 */

#include "Python.h"

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
test_longlong_api(PyObject* self)
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

        value = -1;
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

	value = -1;
        if (PyArg_ParseTuple(tuple, "k:test_k_code", &value) < 0)
        	return NULL;
        if (value != (unsigned long)-0x42)
        	return raiseTestError("test_k_code",
			"k code returned wrong value for long -0xFFF..000042");

	Py_DECREF(tuple);
	Py_INCREF(Py_None);
	return Py_None;
}

#ifdef Py_USING_UNICODE

/* Test the u and u# codes for PyArg_ParseTuple. May leak memory in case
   of an error.
*/
static PyObject *
test_u_code(PyObject *self)
{
	PyObject *tuple, *obj;
	Py_UNICODE *value;
	int len;

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

#endif

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

void _make_call(void *callable)
{
	PyObject *rc;
	PyGILState_STATE s = PyGILState_Ensure();
	rc = PyObject_CallFunction(callable, "");
	Py_XDECREF(rc);
	PyGILState_Release(s);
}

static PyObject *
test_thread_state(PyObject *self, PyObject *args)
{
	PyObject *fn;
	if (!PyArg_ParseTuple(args, "O:test_thread_state", &fn))
		return NULL;
	/* Ensure Python is setup for threading */
	PyEval_InitThreads();
	/* Start a new thread for our callback. */
	PyThread_start_new_thread( _make_call, fn);
	/* Make the callback with the thread lock held by this thread */
	_make_call(fn);
	/* Do it all again, but this time with the thread-lock released */
	Py_BEGIN_ALLOW_THREADS
	_make_call(fn);
	Py_END_ALLOW_THREADS
	/* And once more with and without a thread
	   XXX - should use a lock and work out exactly what we are trying 
	   to test <wink> 
	*/
	Py_BEGIN_ALLOW_THREADS
	PyThread_start_new_thread( _make_call, fn);
	_make_call(fn);
	Py_END_ALLOW_THREADS
	Py_INCREF(Py_None);
	return Py_None;
}
#endif

/* a classic-type with copyable instances */

typedef struct {
	PyObject_HEAD
	/* instance tag (a string). */
	PyObject* tag;
} CopyableObject;

staticforward PyTypeObject Copyable_Type;

#define Copyable_CheckExact(op) ((op)->ob_type == &Copyable_Type)

/* -------------------------------------------------------------------- */

/* copyable constructor and destructor */
static PyObject*
copyable_new(PyObject* tag)
{
	CopyableObject* self;

	self = PyObject_New(CopyableObject, &Copyable_Type);
	if (self == NULL)
		return NULL;
	Py_INCREF(tag);
	self->tag = tag;
	return (PyObject*) self;
}

static PyObject*
copyable(PyObject* self, PyObject* args, PyObject* kw)
{
	PyObject* elem;
	PyObject* tag;
	if (!PyArg_ParseTuple(args, "O:Copyable", &tag))
		return NULL;
	elem = copyable_new(tag);
	return elem;
}

static void
copyable_dealloc(CopyableObject* self)
{
	/* discard attributes */
	Py_DECREF(self->tag);
	PyObject_Del(self);
}

/* copyable methods */

static PyObject*
copyable_copy(CopyableObject* self, PyObject* args)
{
	CopyableObject* copyable;
	if (!PyArg_ParseTuple(args, ":__copy__"))
		return NULL;
	copyable = (CopyableObject*)copyable_new(self->tag);
	if (!copyable)
		return NULL;
	return (PyObject*) copyable;
}

PyObject* _copy_deepcopy;

static PyObject*
copyable_deepcopy(CopyableObject* self, PyObject* args)
{
	CopyableObject* copyable = 0;
	PyObject* memo;
	PyObject* tag_copy;
	if (!PyArg_ParseTuple(args, "O:__deepcopy__", &memo))
		return NULL;

	tag_copy = PyObject_CallFunctionObjArgs(_copy_deepcopy, self->tag, memo, NULL);

	if(tag_copy) {
		copyable = (CopyableObject*)copyable_new(tag_copy);
		Py_DECREF(tag_copy);
	}
	return (PyObject*) copyable;
}

static PyObject*
copyable_repr(CopyableObject* self)
{
	PyObject* repr;
	char buffer[100];
	
	repr = PyString_FromString("<Copyable {");

	PyString_ConcatAndDel(&repr, PyObject_Repr(self->tag));

	sprintf(buffer, "} at %p>", self);
	PyString_ConcatAndDel(&repr, PyString_FromString(buffer));

	return repr;
}

static int
copyable_compare(CopyableObject* obj1, CopyableObject* obj2)
{
	return PyObject_Compare(obj1->tag, obj2->tag);
}

static PyMethodDef copyable_methods[] = {
	{"__copy__", (PyCFunction) copyable_copy, METH_VARARGS},
	{"__deepcopy__", (PyCFunction) copyable_deepcopy, METH_VARARGS},
	{NULL, NULL}
};

static PyObject*  
copyable_getattr(CopyableObject* self, char* name)
{
	PyObject* res;
	res = Py_FindMethod(copyable_methods, (PyObject*) self, name);
	if (res)
	return res;
	PyErr_Clear();
	if (strcmp(name, "tag") == 0) {
	res = self->tag;
	} else {
		PyErr_SetString(PyExc_AttributeError, name);
		return NULL;
	}
	if (!res)
		return NULL;
	Py_INCREF(res);
	return res;
}

static int
copyable_setattr(CopyableObject* self, const char* name, PyObject* value)
{
	if (value == NULL) {
		PyErr_SetString(
			PyExc_AttributeError,
			"can't delete copyable attributes"
			);
		return -1;
	}
	if (strcmp(name, "tag") == 0) {
		Py_DECREF(self->tag);
		self->tag = value;
		Py_INCREF(self->tag);
	} else {
		PyErr_SetString(PyExc_AttributeError, name);
		return -1;
	}
	return 0;
}

statichere PyTypeObject Copyable_Type = {
	PyObject_HEAD_INIT(NULL)
	0, "Copyable", sizeof(CopyableObject), 0,
	/* methods */
	(destructor)copyable_dealloc, /* tp_dealloc */
	0, /* tp_print */
	(getattrfunc)copyable_getattr, /* tp_getattr */
	(setattrfunc)copyable_setattr, /* tp_setattr */
	(cmpfunc)copyable_compare, /* tp_compare */
	(reprfunc)copyable_repr, /* tp_repr */
	0, /* tp_as_number */
};

static PyMethodDef TestMethods[] = {
	{"raise_exception",	raise_exception,		 METH_VARARGS},
	{"test_config",		(PyCFunction)test_config,	 METH_NOARGS},
	{"test_list_api",	(PyCFunction)test_list_api,	 METH_NOARGS},
	{"test_dict_iteration",	(PyCFunction)test_dict_iteration,METH_NOARGS},
	{"test_long_api",	(PyCFunction)test_long_api,	 METH_NOARGS},
	{"test_long_numbits",	(PyCFunction)test_long_numbits,	 METH_NOARGS},
	{"test_k_code",		(PyCFunction)test_k_code,	 METH_NOARGS},

	{"getargs_b",		(PyCFunction)getargs_b,		 METH_VARARGS},
	{"getargs_B",		(PyCFunction)getargs_B,		 METH_VARARGS},
	{"getargs_H",		(PyCFunction)getargs_H,		 METH_VARARGS},
	{"getargs_I",		(PyCFunction)getargs_I,		 METH_VARARGS},
	{"getargs_k",		(PyCFunction)getargs_k,		 METH_VARARGS},
	{"getargs_i",		(PyCFunction)getargs_i,		 METH_VARARGS},
	{"getargs_l",		(PyCFunction)getargs_l,		 METH_VARARGS},
#ifdef HAVE_LONG_LONG
	{"getargs_L",		(PyCFunction)getargs_L,		 METH_VARARGS},
	{"getargs_K",		(PyCFunction)getargs_K,		 METH_VARARGS},
	{"test_longlong_api",	(PyCFunction)test_longlong_api,	 METH_NOARGS},
	{"test_L_code",		(PyCFunction)test_L_code,	 METH_NOARGS},
#endif
#ifdef Py_USING_UNICODE
	{"test_u_code",		(PyCFunction)test_u_code,	 METH_NOARGS},
#endif
#ifdef WITH_THREAD
	{"_test_thread_state",	(PyCFunction)test_thread_state, METH_VARARGS},
#endif
	{"make_copyable",	(PyCFunction) copyable,		METH_VARARGS},
	{NULL, NULL} /* sentinel */

};

#define AddSym(d, n, f, v) {PyObject *o = f(v); PyDict_SetItemString(d, n, o); Py_DECREF(o);}

PyMODINIT_FUNC
init_testcapi(void)
{
	PyObject *m;
	PyObject *copy_module;


	copy_module = PyImport_ImportModule("copy");
	if(!copy_module)
		return;
	_copy_deepcopy = PyObject_GetAttrString(copy_module, "deepcopy");
	Py_DECREF(copy_module);
	Copyable_Type.ob_type = &PyType_Type;

	m = Py_InitModule("_testcapi", TestMethods);

	PyModule_AddObject(m, "UCHAR_MAX", PyInt_FromLong(UCHAR_MAX));
	PyModule_AddObject(m, "USHRT_MAX", PyInt_FromLong(USHRT_MAX));
	PyModule_AddObject(m, "UINT_MAX",  PyLong_FromUnsignedLong(UINT_MAX));
	PyModule_AddObject(m, "ULONG_MAX", PyLong_FromUnsignedLong(ULONG_MAX));
	PyModule_AddObject(m, "INT_MIN", PyInt_FromLong(INT_MIN));
	PyModule_AddObject(m, "LONG_MIN", PyInt_FromLong(LONG_MIN));
	PyModule_AddObject(m, "INT_MAX", PyInt_FromLong(INT_MAX));
	PyModule_AddObject(m, "LONG_MAX", PyInt_FromLong(LONG_MAX));

	TestError = PyErr_NewException("_testcapi.error", NULL, NULL);
	Py_INCREF(TestError);
	PyModule_AddObject(m, "error", TestError);
}
