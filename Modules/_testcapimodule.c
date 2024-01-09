/*
 * C Extension module to test Python interpreter C APIs.
 *
 * The 'test_*' functions exported by this module are run as part of the
 * standard Python regression test, via Lib/test/test_capi.py.
 */

// Include parts.h first since it takes care of NDEBUG and Py_BUILD_CORE macros
// and including Python.h.
//
// Several parts of this module are broken out into files in _testcapi/.
// Include definitions from there.
#include "_testcapi/parts.h"

#include "frameobject.h"          // PyFrame_New()
#include "interpreteridobject.h"  // PyInterpreterID_Type
#include "marshal.h"              // PyMarshal_WriteLongToFile()

#include <float.h>                // FLT_MAX
#include <signal.h>
#include <stddef.h>               // offsetof()

#ifdef HAVE_SYS_WAIT_H
#  include <sys/wait.h>           // W_STOPCODE
#endif

#ifdef bool
#  error "The public headers should not include <stdbool.h>, see gh-48924"
#endif

#include "_testcapi/util.h"


// Forward declarations
static struct PyModuleDef _testcapimodule;

// Module state
typedef struct {
    PyObject *error; // _testcapi.error object
} testcapistate_t;

static testcapistate_t*
get_testcapi_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (testcapistate_t *)state;
}

static PyObject *
get_testerror(PyObject *self) {
    testcapistate_t *state = get_testcapi_state(self);
    return state->error;
}

/* Raise _testcapi.error with test_name + ": " + msg, and return NULL. */

static PyObject *
raiseTestError(PyObject *self, const char* test_name, const char* msg)
{
    PyErr_Format(get_testerror(self), "%s: %s", test_name, msg);
    return NULL;
}

/* Test #defines from pyconfig.h (particularly the SIZEOF_* defines).

   The ones derived from autoconf on the UNIX-like OSes can be relied
   upon (in the absence of sloppy cross-compiling), but the Windows
   platforms have these hardcoded.  Better safe than sorry.
*/
static PyObject*
sizeof_error(PyObject *self, const char* fatname, const char* typname,
    int expected, int got)
{
    PyErr_Format(get_testerror(self),
        "%s #define == %d but sizeof(%s) == %d",
        fatname, expected, typname, got);
    return (PyObject*)NULL;
}

static PyObject*
test_config(PyObject *self, PyObject *Py_UNUSED(ignored))
{
#define CHECK_SIZEOF(FATNAME, TYPE) \
            if (FATNAME != sizeof(TYPE)) \
                return sizeof_error(self, #FATNAME, #TYPE, FATNAME, sizeof(TYPE))

    CHECK_SIZEOF(SIZEOF_SHORT, short);
    CHECK_SIZEOF(SIZEOF_INT, int);
    CHECK_SIZEOF(SIZEOF_LONG, long);
    CHECK_SIZEOF(SIZEOF_VOID_P, void*);
    CHECK_SIZEOF(SIZEOF_TIME_T, time_t);
    CHECK_SIZEOF(SIZEOF_LONG_LONG, long long);

#undef CHECK_SIZEOF

    Py_RETURN_NONE;
}

static PyObject*
test_sizeof_c_types(PyObject *self, PyObject *Py_UNUSED(ignored))
{
#if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5)))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#endif
#define CHECK_SIZEOF(TYPE, EXPECTED)         \
    if (EXPECTED != sizeof(TYPE))  {         \
        PyErr_Format(get_testerror(self),    \
            "sizeof(%s) = %u instead of %u", \
            #TYPE, sizeof(TYPE), EXPECTED);  \
        return (PyObject*)NULL;              \
    }
#define IS_SIGNED(TYPE) (((TYPE)-1) < (TYPE)0)
#define CHECK_SIGNNESS(TYPE, SIGNED)         \
    if (IS_SIGNED(TYPE) != SIGNED) {         \
        PyErr_Format(get_testerror(self),    \
            "%s signness is, instead of %i",  \
            #TYPE, IS_SIGNED(TYPE), SIGNED); \
        return (PyObject*)NULL;              \
    }

    /* integer types */
    CHECK_SIZEOF(Py_UCS1, 1);
    CHECK_SIZEOF(Py_UCS2, 2);
    CHECK_SIZEOF(Py_UCS4, 4);
    CHECK_SIGNNESS(Py_UCS1, 0);
    CHECK_SIGNNESS(Py_UCS2, 0);
    CHECK_SIGNNESS(Py_UCS4, 0);
    CHECK_SIZEOF(int32_t, 4);
    CHECK_SIGNNESS(int32_t, 1);
    CHECK_SIZEOF(uint32_t, 4);
    CHECK_SIGNNESS(uint32_t, 0);
    CHECK_SIZEOF(int64_t, 8);
    CHECK_SIGNNESS(int64_t, 1);
    CHECK_SIZEOF(uint64_t, 8);
    CHECK_SIGNNESS(uint64_t, 0);

    /* pointer/size types */
    CHECK_SIZEOF(size_t, sizeof(void *));
    CHECK_SIGNNESS(size_t, 0);
    CHECK_SIZEOF(Py_ssize_t, sizeof(void *));
    CHECK_SIGNNESS(Py_ssize_t, 1);

    CHECK_SIZEOF(uintptr_t, sizeof(void *));
    CHECK_SIGNNESS(uintptr_t, 0);
    CHECK_SIZEOF(intptr_t, sizeof(void *));
    CHECK_SIGNNESS(intptr_t, 1);

    Py_RETURN_NONE;

#undef IS_SIGNED
#undef CHECK_SIGNESS
#undef CHECK_SIZEOF
#if defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ > 5)))
#pragma GCC diagnostic pop
#endif
}

static PyObject*
test_list_api(PyObject *self, PyObject *Py_UNUSED(ignored))
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
        PyObject* anint = PyLong_FromLong(i);
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
        if (PyLong_AS_LONG(anint) != NLIST-1-i) {
            PyErr_SetString(get_testerror(self),
                            "test_list_api: reverse screwed up");
            Py_DECREF(list);
            return (PyObject*)NULL;
        }
    }
    Py_DECREF(list);
#undef NLIST

    Py_RETURN_NONE;
}

static int
test_dict_inner(PyObject *self, int count)
{
    Py_ssize_t pos = 0, iterations = 0;
    int i;
    PyObject *dict = PyDict_New();
    PyObject *v, *k;

    if (dict == NULL)
        return -1;

    for (i = 0; i < count; i++) {
        v = PyLong_FromLong(i);
        if (v == NULL) {
            goto error;
        }
        if (PyDict_SetItem(dict, v, v) < 0) {
            Py_DECREF(v);
            goto error;
        }
        Py_DECREF(v);
    }

    k = v = UNINITIALIZED_PTR;
    while (PyDict_Next(dict, &pos, &k, &v)) {
        PyObject *o;
        iterations++;

        assert(k != UNINITIALIZED_PTR);
        assert(v != UNINITIALIZED_PTR);
        i = PyLong_AS_LONG(v) + 1;
        o = PyLong_FromLong(i);
        if (o == NULL) {
            goto error;
        }
        if (PyDict_SetItem(dict, k, o) < 0) {
            Py_DECREF(o);
            goto error;
        }
        Py_DECREF(o);
        k = v = UNINITIALIZED_PTR;
    }
    assert(k == UNINITIALIZED_PTR);
    assert(v == UNINITIALIZED_PTR);

    Py_DECREF(dict);

    if (iterations != count) {
        PyErr_SetString(
            get_testerror(self),
            "test_dict_iteration: dict iteration went wrong ");
        return -1;
    } else {
        return 0;
    }
error:
    Py_DECREF(dict);
    return -1;
}



static PyObject*
test_dict_iteration(PyObject* self, PyObject *Py_UNUSED(ignored))
{
    int i;

    for (i = 0; i < 200; i++) {
        if (test_dict_inner(self, i) < 0) {
            return NULL;
        }
    }

    Py_RETURN_NONE;
}

/* Issue #4701: Check that PyObject_Hash implicitly calls
 *   PyType_Ready if it hasn't already been called
 */
static PyTypeObject _HashInheritanceTester_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "hashinheritancetester",            /* Name of this type */
    sizeof(PyObject),           /* Basic object size */
    0,                          /* Item size for varobject */
    (destructor)PyObject_Del, /* tp_dealloc */
    0,                          /* tp_vectorcall_offset */
    0,                          /* tp_getattr */
    0,                          /* tp_setattr */
    0,                          /* tp_as_async */
    0,                          /* tp_repr */
    0,                          /* tp_as_number */
    0,                          /* tp_as_sequence */
    0,                          /* tp_as_mapping */
    0,                          /* tp_hash */
    0,                          /* tp_call */
    0,                          /* tp_str */
    PyObject_GenericGetAttr,  /* tp_getattro */
    0,                          /* tp_setattro */
    0,                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,         /* tp_flags */
    0,                          /* tp_doc */
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    0,                          /* tp_methods */
    0,                          /* tp_members */
    0,                          /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
    0,                          /* tp_init */
    0,                          /* tp_alloc */
    PyType_GenericNew,                  /* tp_new */
};

static PyObject*
pycompilestring(PyObject* self, PyObject *obj) {
    if (PyBytes_CheckExact(obj) == 0) {
        PyErr_SetString(PyExc_ValueError, "Argument must be a bytes object");
        return NULL;
    }
    const char *the_string = PyBytes_AsString(obj);
    if (the_string == NULL) {
        return NULL;
    }
    return Py_CompileString(the_string, "<string>", Py_file_input);
}

static PyObject*
test_lazy_hash_inheritance(PyObject* self, PyObject *Py_UNUSED(ignored))
{
    PyTypeObject *type;
    PyObject *obj;
    Py_hash_t hash;

    type = &_HashInheritanceTester_Type;

    if (type->tp_dict != NULL)
        /* The type has already been initialized. This probably means
           -R is being used. */
        Py_RETURN_NONE;


    obj = PyObject_New(PyObject, type);
    if (obj == NULL) {
        PyErr_Clear();
        PyErr_SetString(
            get_testerror(self),
            "test_lazy_hash_inheritance: failed to create object");
        return NULL;
    }

    if (type->tp_dict != NULL) {
        PyErr_SetString(
            get_testerror(self),
            "test_lazy_hash_inheritance: type initialised too soon");
        Py_DECREF(obj);
        return NULL;
    }

    hash = PyObject_Hash(obj);
    if ((hash == -1) && PyErr_Occurred()) {
        PyErr_Clear();
        PyErr_SetString(
            get_testerror(self),
            "test_lazy_hash_inheritance: could not hash object");
        Py_DECREF(obj);
        return NULL;
    }

    if (type->tp_dict == NULL) {
        PyErr_SetString(
            get_testerror(self),
            "test_lazy_hash_inheritance: type not initialised by hash()");
        Py_DECREF(obj);
        return NULL;
    }

    if (type->tp_hash != PyType_Type.tp_hash) {
        PyErr_SetString(
            get_testerror(self),
            "test_lazy_hash_inheritance: unexpected hash function");
        Py_DECREF(obj);
        return NULL;
    }

    Py_DECREF(obj);

    Py_RETURN_NONE;
}

static PyObject *
return_none(void *unused)
{
    Py_RETURN_NONE;
}

static PyObject *
raise_error(void *unused)
{
    PyErr_SetNone(PyExc_ValueError);
    return NULL;
}

static PyObject *
py_buildvalue(PyObject *self, PyObject *args)
{
    const char *fmt;
    PyObject *objs[10] = {NULL};
    if (!PyArg_ParseTuple(args, "s|OOOOOOOOOO", &fmt,
            &objs[0], &objs[1], &objs[2], &objs[3], &objs[4],
            &objs[5], &objs[6], &objs[7], &objs[8], &objs[9]))
    {
        return NULL;
    }
    for(int i = 0; i < 10; i++) {
        NULLABLE(objs[i]);
    }
    return Py_BuildValue(fmt,
            objs[0], objs[1], objs[2], objs[3], objs[4],
            objs[5], objs[6], objs[7], objs[8], objs[9]);
}

static PyObject *
py_buildvalue_ints(PyObject *self, PyObject *args)
{
    const char *fmt;
    unsigned int values[10] = {0};
    if (!PyArg_ParseTuple(args, "s|IIIIIIIIII", &fmt,
            &values[0], &values[1], &values[2], &values[3], &values[4],
            &values[5], &values[6], &values[7], &values[8], &values[9]))
    {
        return NULL;
    }
    return Py_BuildValue(fmt,
            values[0], values[1], values[2], values[3], values[4],
            values[5], values[6], values[7], values[8], values[9]);
}

static int
test_buildvalue_N_error(PyObject *self, const char *fmt)
{
    PyObject *arg, *res;

    arg = PyList_New(0);
    if (arg == NULL) {
        return -1;
    }

    Py_INCREF(arg);
    res = Py_BuildValue(fmt, return_none, NULL, arg);
    if (res == NULL) {
        return -1;
    }
    Py_DECREF(res);
    if (Py_REFCNT(arg) != 1) {
        PyErr_Format(get_testerror(self), "test_buildvalue_N: "
                     "arg was not decrefed in successful "
                     "Py_BuildValue(\"%s\")", fmt);
        return -1;
    }

    Py_INCREF(arg);
    res = Py_BuildValue(fmt, raise_error, NULL, arg);
    if (res != NULL || !PyErr_Occurred()) {
        PyErr_Format(get_testerror(self), "test_buildvalue_N: "
                     "Py_BuildValue(\"%s\") didn't complain", fmt);
        return -1;
    }
    PyErr_Clear();
    if (Py_REFCNT(arg) != 1) {
        PyErr_Format(get_testerror(self), "test_buildvalue_N: "
                     "arg was not decrefed in failed "
                     "Py_BuildValue(\"%s\")", fmt);
        return -1;
    }
    Py_DECREF(arg);
    return 0;
}

static PyObject *
test_buildvalue_N(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *arg, *res;

    arg = PyList_New(0);
    if (arg == NULL) {
        return NULL;
    }
    Py_INCREF(arg);
    res = Py_BuildValue("N", arg);
    if (res == NULL) {
        return NULL;
    }
    if (res != arg) {
        return raiseTestError(self, "test_buildvalue_N",
                              "Py_BuildValue(\"N\") returned wrong result");
    }
    if (Py_REFCNT(arg) != 2) {
        return raiseTestError(self, "test_buildvalue_N",
                              "arg was not decrefed in Py_BuildValue(\"N\")");
    }
    Py_DECREF(res);
    Py_DECREF(arg);

    if (test_buildvalue_N_error(self, "O&N") < 0)
        return NULL;
    if (test_buildvalue_N_error(self, "(O&N)") < 0)
        return NULL;
    if (test_buildvalue_N_error(self, "[O&N]") < 0)
        return NULL;
    if (test_buildvalue_N_error(self, "{O&N}") < 0)
        return NULL;
    if (test_buildvalue_N_error(self, "{()O&(())N}") < 0)
        return NULL;

    Py_RETURN_NONE;
}


static PyObject *
test_get_statictype_slots(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    newfunc tp_new = PyType_GetSlot(&PyLong_Type, Py_tp_new);
    if (PyLong_Type.tp_new != tp_new) {
        PyErr_SetString(PyExc_AssertionError, "mismatch: tp_new of long");
        return NULL;
    }

    reprfunc tp_repr = PyType_GetSlot(&PyLong_Type, Py_tp_repr);
    if (PyLong_Type.tp_repr != tp_repr) {
        PyErr_SetString(PyExc_AssertionError, "mismatch: tp_repr of long");
        return NULL;
    }

    ternaryfunc tp_call = PyType_GetSlot(&PyLong_Type, Py_tp_call);
    if (tp_call != NULL) {
        PyErr_SetString(PyExc_AssertionError, "mismatch: tp_call of long");
        return NULL;
    }

    binaryfunc nb_add = PyType_GetSlot(&PyLong_Type, Py_nb_add);
    if (PyLong_Type.tp_as_number->nb_add != nb_add) {
        PyErr_SetString(PyExc_AssertionError, "mismatch: nb_add of long");
        return NULL;
    }

    lenfunc mp_length = PyType_GetSlot(&PyLong_Type, Py_mp_length);
    if (mp_length != NULL) {
        PyErr_SetString(PyExc_AssertionError, "mismatch: mp_length of long");
        return NULL;
    }

    void *over_value = PyType_GetSlot(&PyLong_Type, Py_bf_releasebuffer + 1);
    if (over_value != NULL) {
        PyErr_SetString(PyExc_AssertionError, "mismatch: max+1 of long");
        return NULL;
    }

    tp_new = PyType_GetSlot(&PyLong_Type, 0);
    if (tp_new != NULL) {
        PyErr_SetString(PyExc_AssertionError, "mismatch: slot 0 of long");
        return NULL;
    }
    if (PyErr_ExceptionMatches(PyExc_SystemError)) {
        // This is the right exception
        PyErr_Clear();
    }
    else {
        return NULL;
    }

    Py_RETURN_NONE;
}


static PyType_Slot HeapTypeNameType_slots[] = {
    {0},
};

static PyType_Spec HeapTypeNameType_Spec = {
    .name = "_testcapi.HeapTypeNameType",
    .basicsize = sizeof(PyObject),
    .flags = Py_TPFLAGS_DEFAULT,
    .slots = HeapTypeNameType_slots,
};

static PyObject *
get_heaptype_for_name(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyType_FromSpec(&HeapTypeNameType_Spec);
}

static PyObject *
test_get_type_name(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *tp_name = PyType_GetName(&PyLong_Type);
    assert(strcmp(PyUnicode_AsUTF8(tp_name), "int") == 0);
    Py_DECREF(tp_name);

    tp_name = PyType_GetName(&PyModule_Type);
    assert(strcmp(PyUnicode_AsUTF8(tp_name), "module") == 0);
    Py_DECREF(tp_name);

    PyObject *HeapTypeNameType = PyType_FromSpec(&HeapTypeNameType_Spec);
    if (HeapTypeNameType == NULL) {
        Py_RETURN_NONE;
    }
    tp_name = PyType_GetName((PyTypeObject *)HeapTypeNameType);
    assert(strcmp(PyUnicode_AsUTF8(tp_name), "HeapTypeNameType") == 0);
    Py_DECREF(tp_name);

    PyObject *name = PyUnicode_FromString("test_name");
    if (name == NULL) {
        goto done;
    }
    if (PyObject_SetAttrString(HeapTypeNameType, "__name__", name) < 0) {
        Py_DECREF(name);
        goto done;
    }
    tp_name = PyType_GetName((PyTypeObject *)HeapTypeNameType);
    assert(strcmp(PyUnicode_AsUTF8(tp_name), "test_name") == 0);
    Py_DECREF(name);
    Py_DECREF(tp_name);

  done:
    Py_DECREF(HeapTypeNameType);
    Py_RETURN_NONE;
}


static PyObject *
test_get_type_qualname(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *tp_qualname = PyType_GetQualName(&PyLong_Type);
    assert(strcmp(PyUnicode_AsUTF8(tp_qualname), "int") == 0);
    Py_DECREF(tp_qualname);

    tp_qualname = PyType_GetQualName(&PyODict_Type);
    assert(strcmp(PyUnicode_AsUTF8(tp_qualname), "OrderedDict") == 0);
    Py_DECREF(tp_qualname);

    PyObject *HeapTypeNameType = PyType_FromSpec(&HeapTypeNameType_Spec);
    if (HeapTypeNameType == NULL) {
        Py_RETURN_NONE;
    }
    tp_qualname = PyType_GetQualName((PyTypeObject *)HeapTypeNameType);
    assert(strcmp(PyUnicode_AsUTF8(tp_qualname), "HeapTypeNameType") == 0);
    Py_DECREF(tp_qualname);

    PyObject *spec_name = PyUnicode_FromString(HeapTypeNameType_Spec.name);
    if (spec_name == NULL) {
        goto done;
    }
    if (PyObject_SetAttrString(HeapTypeNameType,
                               "__qualname__", spec_name) < 0) {
        Py_DECREF(spec_name);
        goto done;
    }
    tp_qualname = PyType_GetQualName((PyTypeObject *)HeapTypeNameType);
    assert(strcmp(PyUnicode_AsUTF8(tp_qualname),
                  "_testcapi.HeapTypeNameType") == 0);
    Py_DECREF(spec_name);
    Py_DECREF(tp_qualname);

  done:
    Py_DECREF(HeapTypeNameType);
    Py_RETURN_NONE;
}

static PyObject *
test_get_type_dict(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    /* Test for PyType_GetDict */

    // Assert ints have a `to_bytes` method
    PyObject *long_dict = PyType_GetDict(&PyLong_Type);
    assert(long_dict);
    assert(PyDict_GetItemString(long_dict, "to_bytes")); // borrowed ref
    Py_DECREF(long_dict);

    // Make a new type, add an attribute to it and assert it's there
    PyObject *HeapTypeNameType = PyType_FromSpec(&HeapTypeNameType_Spec);
    assert(HeapTypeNameType);
    assert(PyObject_SetAttrString(
        HeapTypeNameType, "new_attr", Py_NewRef(Py_None)) >= 0);
    PyObject *type_dict = PyType_GetDict((PyTypeObject*)HeapTypeNameType);
    assert(type_dict);
    assert(PyDict_GetItemString(type_dict, "new_attr")); // borrowed ref
    Py_DECREF(HeapTypeNameType);
    Py_DECREF(type_dict);
    Py_RETURN_NONE;
}

static PyObject *
pyobject_repr_from_null(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyObject_Repr(NULL);
}

static PyObject *
pyobject_str_from_null(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyObject_Str(NULL);
}

static PyObject *
pyobject_bytes_from_null(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return PyObject_Bytes(NULL);
}

static PyObject *
set_errno(PyObject *self, PyObject *args)
{
    int new_errno;

    if (!PyArg_ParseTuple(args, "i:set_errno", &new_errno))
        return NULL;

    errno = new_errno;
    Py_RETURN_NONE;
}

/* test_thread_state spawns a thread of its own, and that thread releases
 * `thread_done` when it's finished.  The driver code has to know when the
 * thread finishes, because the thread uses a PyObject (the callable) that
 * may go away when the driver finishes.  The former lack of this explicit
 * synchronization caused rare segfaults, so rare that they were seen only
 * on a Mac buildbot (although they were possible on any box).
 */
static PyThread_type_lock thread_done = NULL;

static int
_make_call(void *callable)
{
    PyObject *rc;
    int success;
    PyGILState_STATE s = PyGILState_Ensure();
    rc = PyObject_CallNoArgs((PyObject *)callable);
    success = (rc != NULL);
    Py_XDECREF(rc);
    PyGILState_Release(s);
    return success;
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
    int success = 1;

    if (!PyArg_ParseTuple(args, "O:test_thread_state", &fn))
        return NULL;

    if (!PyCallable_Check(fn)) {
        PyErr_Format(PyExc_TypeError, "'%s' object is not callable",
            Py_TYPE(fn)->tp_name);
        return NULL;
    }

    thread_done = PyThread_allocate_lock();
    if (thread_done == NULL)
        return PyErr_NoMemory();
    PyThread_acquire_lock(thread_done, 1);

    /* Start a new thread with our callback. */
    PyThread_start_new_thread(_make_call_from_thread, fn);
    /* Make the callback with the thread lock held by this thread */
    success &= _make_call(fn);
    /* Do it all again, but this time with the thread-lock released */
    Py_BEGIN_ALLOW_THREADS
    success &= _make_call(fn);
    PyThread_acquire_lock(thread_done, 1);  /* wait for thread to finish */
    Py_END_ALLOW_THREADS

    /* And once more with and without a thread
       XXX - should use a lock and work out exactly what we are trying
       to test <wink>
    */
    Py_BEGIN_ALLOW_THREADS
    PyThread_start_new_thread(_make_call_from_thread, fn);
    success &= _make_call(fn);
    PyThread_acquire_lock(thread_done, 1);  /* wait for thread to finish */
    Py_END_ALLOW_THREADS

    /* Release lock we acquired above.  This is required on HP-UX. */
    PyThread_release_lock(thread_done);

    PyThread_free_lock(thread_done);
    if (!success)
        return NULL;
    Py_RETURN_NONE;
}

#ifndef MS_WINDOWS
static PyThread_type_lock wait_done = NULL;

static void wait_for_lock(void *unused) {
    PyThread_acquire_lock(wait_done, 1);
    PyThread_release_lock(wait_done);
    PyThread_free_lock(wait_done);
    wait_done = NULL;
}

// These can be used to test things that care about the existence of another
// thread that the threading module doesn't know about.

static PyObject *
spawn_pthread_waiter(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    if (wait_done) {
        PyErr_SetString(PyExc_RuntimeError, "thread already running");
        return NULL;
    }
    wait_done = PyThread_allocate_lock();
    if (wait_done == NULL)
        return PyErr_NoMemory();
    PyThread_acquire_lock(wait_done, 1);
    PyThread_start_new_thread(wait_for_lock, NULL);
    Py_RETURN_NONE;
}

static PyObject *
end_spawned_pthread(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    if (!wait_done) {
        PyErr_SetString(PyExc_RuntimeError, "call _spawn_pthread_waiter 1st");
        return NULL;
    }
    PyThread_release_lock(wait_done);
    Py_RETURN_NONE;
}
#endif  // not MS_WINDOWS

/* test Py_AddPendingCalls using threads */
static int _pending_callback(void *arg)
{
    /* we assume the argument is callable object to which we own a reference */
    PyObject *callable = (PyObject *)arg;
    PyObject *r = PyObject_CallNoArgs(callable);
    Py_DECREF(callable);
    Py_XDECREF(r);
    return r != NULL ? 0 : -1;
}

/* The following requests n callbacks to _pending_callback.  It can be
 * run from any python thread.
 */
static PyObject *
pending_threadfunc(PyObject *self, PyObject *arg)
{
    PyObject *callable;
    int r;
    if (PyArg_ParseTuple(arg, "O", &callable) == 0)
        return NULL;

    /* create the reference for the callbackwhile we hold the lock */
    Py_INCREF(callable);

    Py_BEGIN_ALLOW_THREADS
    r = Py_AddPendingCall(&_pending_callback, callable);
    Py_END_ALLOW_THREADS

    if (r<0) {
        Py_DECREF(callable); /* unsuccessful add, destroy the extra reference */
        Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}

/* Test PyOS_string_to_double. */
static PyObject *
test_string_to_double(PyObject *self, PyObject *Py_UNUSED(ignored)) {
    double result;
    const char *msg;

#define CHECK_STRING(STR, expected)                             \
    result = PyOS_string_to_double(STR, NULL, NULL);            \
    if (result == -1.0 && PyErr_Occurred())                     \
        return NULL;                                            \
    if (result != (double)expected) {                           \
        msg = "conversion of " STR " to float failed";          \
        goto fail;                                              \
    }

#define CHECK_INVALID(STR)                                              \
    result = PyOS_string_to_double(STR, NULL, NULL);                    \
    if (result == -1.0 && PyErr_Occurred()) {                           \
        if (PyErr_ExceptionMatches(PyExc_ValueError))                   \
            PyErr_Clear();                                              \
        else                                                            \
            return NULL;                                                \
    }                                                                   \
    else {                                                              \
        msg = "conversion of " STR " didn't raise ValueError";          \
        goto fail;                                                      \
    }

    CHECK_STRING("0.1", 0.1);
    CHECK_STRING("1.234", 1.234);
    CHECK_STRING("-1.35", -1.35);
    CHECK_STRING(".1e01", 1.0);
    CHECK_STRING("2.e-2", 0.02);

    CHECK_INVALID(" 0.1");
    CHECK_INVALID("\t\n-3");
    CHECK_INVALID(".123 ");
    CHECK_INVALID("3\n");
    CHECK_INVALID("123abc");

    Py_RETURN_NONE;
  fail:
    return raiseTestError(self, "test_string_to_double", msg);
#undef CHECK_STRING
#undef CHECK_INVALID
}


/* Coverage testing of capsule objects. */

static const char *capsule_name = "capsule name";
static       char *capsule_pointer = "capsule pointer";
static       char *capsule_context = "capsule context";
static const char *capsule_error = NULL;
static int
capsule_destructor_call_count = 0;

static void
capsule_destructor(PyObject *o) {
    capsule_destructor_call_count++;
    if (PyCapsule_GetContext(o) != capsule_context) {
        capsule_error = "context did not match in destructor!";
    } else if (PyCapsule_GetDestructor(o) != capsule_destructor) {
        capsule_error = "destructor did not match in destructor!  (woah!)";
    } else if (PyCapsule_GetName(o) != capsule_name) {
        capsule_error = "name did not match in destructor!";
    } else if (PyCapsule_GetPointer(o, capsule_name) != capsule_pointer) {
        capsule_error = "pointer did not match in destructor!";
    }
}

typedef struct {
    char *name;
    char *module;
    char *attribute;
} known_capsule;

static PyObject *
test_capsule(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *object;
    const char *error = NULL;
    void *pointer;
    void *pointer2;
    known_capsule known_capsules[] = {
        #define KNOWN_CAPSULE(module, name)             { module "." name, module, name }
        KNOWN_CAPSULE("_socket", "CAPI"),
        KNOWN_CAPSULE("_curses", "_C_API"),
        KNOWN_CAPSULE("datetime", "datetime_CAPI"),
        { NULL, NULL },
    };
    known_capsule *known = &known_capsules[0];

#define FAIL(x) { error = (x); goto exit; }

#define CHECK_DESTRUCTOR \
    if (capsule_error) { \
        FAIL(capsule_error); \
    } \
    else if (!capsule_destructor_call_count) {          \
        FAIL("destructor not called!"); \
    } \
    capsule_destructor_call_count = 0; \

    object = PyCapsule_New(capsule_pointer, capsule_name, capsule_destructor);
    PyCapsule_SetContext(object, capsule_context);
    capsule_destructor(object);
    CHECK_DESTRUCTOR;
    Py_DECREF(object);
    CHECK_DESTRUCTOR;

    object = PyCapsule_New(known, "ignored", NULL);
    PyCapsule_SetPointer(object, capsule_pointer);
    PyCapsule_SetName(object, capsule_name);
    PyCapsule_SetDestructor(object, capsule_destructor);
    PyCapsule_SetContext(object, capsule_context);
    capsule_destructor(object);
    CHECK_DESTRUCTOR;
    /* intentionally access using the wrong name */
    pointer2 = PyCapsule_GetPointer(object, "the wrong name");
    if (!PyErr_Occurred()) {
        FAIL("PyCapsule_GetPointer should have failed but did not!");
    }
    PyErr_Clear();
    if (pointer2) {
        if (pointer2 == capsule_pointer) {
            FAIL("PyCapsule_GetPointer should not have"
                     " returned the internal pointer!");
        } else {
            FAIL("PyCapsule_GetPointer should have "
                     "returned NULL pointer but did not!");
        }
    }
    PyCapsule_SetDestructor(object, NULL);
    Py_DECREF(object);
    if (capsule_destructor_call_count) {
        FAIL("destructor called when it should not have been!");
    }

    for (known = &known_capsules[0]; known->module != NULL; known++) {
        /* yeah, ordinarily I wouldn't do this either,
           but it's fine for this test harness.
        */
        static char buffer[256];
#undef FAIL
#define FAIL(x) \
        { \
        sprintf(buffer, "%s module: \"%s\" attribute: \"%s\"", \
            x, known->module, known->attribute); \
        error = buffer; \
        goto exit; \
        } \

        PyObject *module = PyImport_ImportModule(known->module);
        if (module) {
            pointer = PyCapsule_Import(known->name, 0);
            if (!pointer) {
                Py_DECREF(module);
                FAIL("PyCapsule_GetPointer returned NULL unexpectedly!");
            }
            object = PyObject_GetAttrString(module, known->attribute);
            if (!object) {
                Py_DECREF(module);
                return NULL;
            }
            pointer2 = PyCapsule_GetPointer(object,
                                    "weebles wobble but they don't fall down");
            if (!PyErr_Occurred()) {
                Py_DECREF(object);
                Py_DECREF(module);
                FAIL("PyCapsule_GetPointer should have failed but did not!");
            }
            PyErr_Clear();
            if (pointer2) {
                Py_DECREF(module);
                Py_DECREF(object);
                if (pointer2 == pointer) {
                    FAIL("PyCapsule_GetPointer should not have"
                             " returned its internal pointer!");
                } else {
                    FAIL("PyCapsule_GetPointer should have"
                             " returned NULL pointer but did not!");
                }
            }
            Py_DECREF(object);
            Py_DECREF(module);
        }
        else
            PyErr_Clear();
    }

  exit:
    if (error) {
        return raiseTestError(self, "test_capsule", error);
    }
    Py_RETURN_NONE;
#undef FAIL
}

#ifdef HAVE_GETTIMEOFDAY
/* Profiling of integer performance */
static void print_delta(int test, struct timeval *s, struct timeval *e)
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
            single = PyLong_FromLong(i);
            Py_DECREF(single);
        }
    gettimeofday(&stop, NULL);
    print_delta(1, &start, &stop);

    /* Test 2: Allocate and immediately deallocate
       many large integers */
    gettimeofday(&start, NULL);
    for(k=0; k < 20000; k++)
        for(i=0; i < 1000; i++) {
            single = PyLong_FromLong(i+1000000);
            Py_DECREF(single);
        }
    gettimeofday(&stop, NULL);
    print_delta(2, &start, &stop);

    /* Test 3: Allocate a few integers, then release
       them all simultaneously. */
    multiple = malloc(sizeof(PyObject*) * 1000);
    if (multiple == NULL)
        return PyErr_NoMemory();
    gettimeofday(&start, NULL);
    for(k=0; k < 20000; k++) {
        for(i=0; i < 1000; i++) {
            multiple[i] = PyLong_FromLong(i+1000000);
        }
        for(i=0; i < 1000; i++) {
            Py_DECREF(multiple[i]);
        }
    }
    gettimeofday(&stop, NULL);
    print_delta(3, &start, &stop);
    free(multiple);

    /* Test 4: Allocate many integers, then release
       them all simultaneously. */
    multiple = malloc(sizeof(PyObject*) * 1000000);
    if (multiple == NULL)
        return PyErr_NoMemory();
    gettimeofday(&start, NULL);
    for(k=0; k < 20; k++) {
        for(i=0; i < 1000000; i++) {
            multiple[i] = PyLong_FromLong(i+1000000);
        }
        for(i=0; i < 1000000; i++) {
            Py_DECREF(multiple[i]);
        }
    }
    gettimeofday(&stop, NULL);
    print_delta(4, &start, &stop);
    free(multiple);

    /* Test 5: Allocate many integers < 32000 */
    multiple = malloc(sizeof(PyObject*) * 1000000);
    if (multiple == NULL)
        return PyErr_NoMemory();
    gettimeofday(&start, NULL);
    for(k=0; k < 10; k++) {
        for(i=0; i < 1000000; i++) {
            multiple[i] = PyLong_FromLong(i+1000);
        }
        for(i=0; i < 1000000; i++) {
            Py_DECREF(multiple[i]);
        }
    }
    gettimeofday(&stop, NULL);
    print_delta(5, &start, &stop);
    free(multiple);

    /* Test 6: Perform small int addition */
    op1 = PyLong_FromLong(1);
    gettimeofday(&start, NULL);
    for(i=0; i < 10000000; i++) {
        result = PyNumber_Add(op1, op1);
        Py_DECREF(result);
    }
    gettimeofday(&stop, NULL);
    Py_DECREF(op1);
    print_delta(6, &start, &stop);

    /* Test 7: Perform medium int addition */
    op1 = PyLong_FromLong(1000);
    if (op1 == NULL)
        return NULL;
    gettimeofday(&start, NULL);
    for(i=0; i < 10000000; i++) {
        result = PyNumber_Add(op1, op1);
        Py_XDECREF(result);
    }
    gettimeofday(&stop, NULL);
    Py_DECREF(op1);
    print_delta(7, &start, &stop);

    Py_RETURN_NONE;
}
#endif

/* Issue 6012 */
static PyObject *str1, *str2;
static int
failing_converter(PyObject *obj, void *arg)
{
    /* Clone str1, then let the conversion fail. */
    assert(str1);
    str2 = Py_NewRef(str1);
    return 0;
}
static PyObject*
argparsing(PyObject *o, PyObject *args)
{
    PyObject *res;
    str1 = str2 = NULL;
    if (!PyArg_ParseTuple(args, "O&O&",
                          PyUnicode_FSConverter, &str1,
                          failing_converter, &str2)) {
        if (!str2)
            /* argument converter not called? */
            return NULL;
        /* Should be 1 */
        res = PyLong_FromSsize_t(Py_REFCNT(str2));
        Py_DECREF(str2);
        PyErr_Clear();
        return res;
    }
    Py_RETURN_NONE;
}

/* To test that the result of PyCode_NewEmpty has the right members. */
static PyObject *
code_newempty(PyObject *self, PyObject *args)
{
    const char *filename;
    const char *funcname;
    int firstlineno;

    if (!PyArg_ParseTuple(args, "ssi:code_newempty",
                          &filename, &funcname, &firstlineno))
        return NULL;

    return (PyObject *)PyCode_NewEmpty(filename, funcname, firstlineno);
}

static PyObject *
make_memoryview_from_NULL_pointer(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    Py_buffer info;
    if (PyBuffer_FillInfo(&info, NULL, NULL, 1, 1, PyBUF_FULL_RO) < 0)
        return NULL;
    return PyMemoryView_FromBuffer(&info);
}

static PyObject *
test_from_contiguous(PyObject* self, PyObject *Py_UNUSED(ignored))
{
    int data[9] = {-1,-1,-1,-1,-1,-1,-1,-1,-1};
    int init[5] = {0, 1, 2, 3, 4};
    Py_ssize_t itemsize = sizeof(int);
    Py_ssize_t shape = 5;
    Py_ssize_t strides = 2 * itemsize;
    Py_buffer view = {
        data,
        NULL,
        5 * itemsize,
        itemsize,
        1,
        1,
        NULL,
        &shape,
        &strides,
        NULL,
        NULL
    };
    int *ptr;
    int i;

    PyBuffer_FromContiguous(&view, init, view.len, 'C');
    ptr = view.buf;
    for (i = 0; i < 5; i++) {
        if (ptr[2*i] != i) {
            PyErr_SetString(get_testerror(self),
                "test_from_contiguous: incorrect result");
            return NULL;
        }
    }

    view.buf = &data[8];
    view.strides[0] = -2 * itemsize;

    PyBuffer_FromContiguous(&view, init, view.len, 'C');
    ptr = view.buf;
    for (i = 0; i < 5; i++) {
        if (*(ptr-2*i) != i) {
            PyErr_SetString(get_testerror(self),
                "test_from_contiguous: incorrect result");
            return NULL;
        }
    }

    Py_RETURN_NONE;
}

#if (defined(__linux__) || defined(__FreeBSD__)) && defined(__GNUC__)

static PyObject *
test_pep3118_obsolete_write_locks(PyObject* self, PyObject *Py_UNUSED(ignored))
{
    PyObject *b;
    char *dummy[1];
    int ret, match;

    /* PyBuffer_FillInfo() */
    ret = PyBuffer_FillInfo(NULL, NULL, dummy, 1, 0, PyBUF_SIMPLE);
    match = PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_BufferError);
    PyErr_Clear();
    if (ret != -1 || match == 0)
        goto error;

    PyObject *mod_io = PyImport_ImportModule("_io");
    if (mod_io == NULL) {
        return NULL;
    }

    /* bytesiobuf_getbuffer() */
    PyTypeObject *type = (PyTypeObject *)PyObject_GetAttrString(
            mod_io, "_BytesIOBuffer");
    Py_DECREF(mod_io);
    if (type == NULL) {
        return NULL;
    }
    b = type->tp_alloc(type, 0);
    Py_DECREF(type);
    if (b == NULL) {
        return NULL;
    }

    ret = PyObject_GetBuffer(b, NULL, PyBUF_SIMPLE);
    Py_DECREF(b);
    match = PyErr_Occurred() && PyErr_ExceptionMatches(PyExc_BufferError);
    PyErr_Clear();
    if (ret != -1 || match == 0)
        goto error;

    Py_RETURN_NONE;

error:
    PyErr_SetString(get_testerror(self),
        "test_pep3118_obsolete_write_locks: failure");
    return NULL;
}
#endif

/* This tests functions that historically supported write locks.  It is
   wrong to call getbuffer() with view==NULL and a compliant getbufferproc
   is entitled to segfault in that case. */
static PyObject *
getbuffer_with_null_view(PyObject* self, PyObject *obj)
{
    if (PyObject_GetBuffer(obj, NULL, PyBUF_SIMPLE) < 0)
        return NULL;

    Py_RETURN_NONE;
}

/* PyBuffer_SizeFromFormat() */
static PyObject *
test_PyBuffer_SizeFromFormat(PyObject *self, PyObject *args)
{
    const char *format;

    if (!PyArg_ParseTuple(args, "s:test_PyBuffer_SizeFromFormat",
                          &format)) {
        return NULL;
    }

    RETURN_SIZE(PyBuffer_SizeFromFormat(format));
}

/* Test that the fatal error from not having a current thread doesn't
   cause an infinite loop.  Run via Lib/test/test_capi.py */
static PyObject *
crash_no_current_thread(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    Py_BEGIN_ALLOW_THREADS
    /* Using PyThreadState_Get() directly allows the test to pass in
       !pydebug mode. However, the test only actually tests anything
       in pydebug mode, since that's where the infinite loop was in
       the first place. */
    PyThreadState_Get();
    Py_END_ALLOW_THREADS
    return NULL;
}

/* Test that the GILState thread and the "current" thread match. */
static PyObject *
test_current_tstate_matches(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyThreadState *orig_tstate = PyThreadState_Get();

    if (orig_tstate != PyGILState_GetThisThreadState()) {
        PyErr_SetString(PyExc_RuntimeError,
                        "current thread state doesn't match GILState");
        return NULL;
    }

    const char *err = NULL;
    PyThreadState_Swap(NULL);
    PyThreadState *substate = Py_NewInterpreter();

    if (substate != PyThreadState_Get()) {
        err = "subinterpreter thread state not current";
        goto finally;
    }
    if (substate != PyGILState_GetThisThreadState()) {
        err = "subinterpreter thread state doesn't match GILState";
        goto finally;
    }

finally:
    Py_EndInterpreter(substate);
    PyThreadState_Swap(orig_tstate);

    if (err != NULL) {
        PyErr_SetString(PyExc_RuntimeError, err);
        return NULL;
    }
    Py_RETURN_NONE;
}

/* To run some code in a sub-interpreter. */
static PyObject *
run_in_subinterp(PyObject *self, PyObject *args)
{
    const char *code;
    int r;
    PyThreadState *substate, *mainstate;
    /* only initialise 'cflags.cf_flags' to test backwards compatibility */
    PyCompilerFlags cflags = {0};

    if (!PyArg_ParseTuple(args, "s:run_in_subinterp",
                          &code))
        return NULL;

    mainstate = PyThreadState_Get();

    PyThreadState_Swap(NULL);

    substate = Py_NewInterpreter();
    if (substate == NULL) {
        /* Since no new thread state was created, there is no exception to
           propagate; raise a fresh one after swapping in the old thread
           state. */
        PyThreadState_Swap(mainstate);
        PyErr_SetString(PyExc_RuntimeError, "sub-interpreter creation failed");
        return NULL;
    }
    r = PyRun_SimpleStringFlags(code, &cflags);
    Py_EndInterpreter(substate);

    PyThreadState_Swap(mainstate);

    return PyLong_FromLong(r);
}

static PyObject *
get_interpreterid_type(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return Py_NewRef(&PyInterpreterID_Type);
}

static PyObject *
link_interpreter_refcount(PyObject *self, PyObject *idobj)
{
    PyInterpreterState *interp = PyInterpreterID_LookUp(idobj);
    if (interp == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    _PyInterpreterState_RequireIDRef(interp, 1);
    Py_RETURN_NONE;
}

static PyObject *
unlink_interpreter_refcount(PyObject *self, PyObject *idobj)
{
    PyInterpreterState *interp = PyInterpreterID_LookUp(idobj);
    if (interp == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    _PyInterpreterState_RequireIDRef(interp, 0);
    Py_RETURN_NONE;
}

static PyMethodDef ml;

static PyObject *
create_cfunction(PyObject *self, PyObject *args)
{
    return PyCFunction_NewEx(&ml, self, NULL);
}

static PyMethodDef ml = {
    "create_cfunction",
    create_cfunction,
    METH_NOARGS,
    NULL
};

static PyObject *
_test_incref(PyObject *ob)
{
    return Py_NewRef(ob);
}

static PyObject *
test_xincref_doesnt_leak(PyObject *ob, PyObject *Py_UNUSED(ignored))
{
    PyObject *obj = PyLong_FromLong(0);
    Py_XINCREF(_test_incref(obj));
    Py_DECREF(obj);
    Py_DECREF(obj);
    Py_DECREF(obj);
    Py_RETURN_NONE;
}

static PyObject *
test_incref_doesnt_leak(PyObject *ob, PyObject *Py_UNUSED(ignored))
{
    PyObject *obj = PyLong_FromLong(0);
    Py_INCREF(_test_incref(obj));
    Py_DECREF(obj);
    Py_DECREF(obj);
    Py_DECREF(obj);
    Py_RETURN_NONE;
}

static PyObject *
test_xdecref_doesnt_leak(PyObject *ob, PyObject *Py_UNUSED(ignored))
{
    Py_XDECREF(PyLong_FromLong(0));
    Py_RETURN_NONE;
}

static PyObject *
test_decref_doesnt_leak(PyObject *ob, PyObject *Py_UNUSED(ignored))
{
    Py_DECREF(PyLong_FromLong(0));
    Py_RETURN_NONE;
}

static PyObject *
test_structseq_newtype_doesnt_leak(PyObject *Py_UNUSED(self),
                              PyObject *Py_UNUSED(args))
{
    PyStructSequence_Desc descr;
    PyStructSequence_Field descr_fields[3];

    descr_fields[0] = (PyStructSequence_Field){"foo", "foo value"};
    descr_fields[1] = (PyStructSequence_Field){NULL, "some hidden value"};
    descr_fields[2] = (PyStructSequence_Field){0, NULL};

    descr.name = "_testcapi.test_descr";
    descr.doc = "This is used to test for memory leaks in NewType";
    descr.fields = descr_fields;
    descr.n_in_sequence = 1;

    PyTypeObject* structseq_type = PyStructSequence_NewType(&descr);
    if (structseq_type == NULL) {
        return NULL;
    }
    assert(PyType_Check(structseq_type));
    assert(PyType_FastSubclass(structseq_type, Py_TPFLAGS_TUPLE_SUBCLASS));
    Py_DECREF(structseq_type);

    Py_RETURN_NONE;
}

static PyObject *
test_structseq_newtype_null_descr_doc(PyObject *Py_UNUSED(self),
                              PyObject *Py_UNUSED(args))
{
    PyStructSequence_Field descr_fields[1] = {
        (PyStructSequence_Field){NULL, NULL}
    };
    // Test specifically for NULL .doc field.
    PyStructSequence_Desc descr = {"_testcapi.test_descr", NULL, &descr_fields[0], 0};

    PyTypeObject* structseq_type = PyStructSequence_NewType(&descr);
    assert(structseq_type != NULL);
    assert(PyType_Check(structseq_type));
    assert(PyType_FastSubclass(structseq_type, Py_TPFLAGS_TUPLE_SUBCLASS));
    Py_DECREF(structseq_type);

    Py_RETURN_NONE;
}

static PyObject *
test_incref_decref_API(PyObject *ob, PyObject *Py_UNUSED(ignored))
{
    PyObject *obj = PyLong_FromLong(0);
    Py_IncRef(obj);
    Py_DecRef(obj);
    Py_DecRef(obj);
    Py_RETURN_NONE;
}

typedef struct {
    PyThread_type_lock start_event;
    PyThread_type_lock exit_event;
    PyObject *callback;
} test_c_thread_t;

static void
temporary_c_thread(void *data)
{
    test_c_thread_t *test_c_thread = data;
    PyGILState_STATE state;
    PyObject *res;

    PyThread_release_lock(test_c_thread->start_event);

    /* Allocate a Python thread state for this thread */
    state = PyGILState_Ensure();

    res = PyObject_CallNoArgs(test_c_thread->callback);
    Py_CLEAR(test_c_thread->callback);

    if (res == NULL) {
        PyErr_Print();
    }
    else {
        Py_DECREF(res);
    }

    /* Destroy the Python thread state for this thread */
    PyGILState_Release(state);

    PyThread_release_lock(test_c_thread->exit_event);
}

static test_c_thread_t test_c_thread;

static PyObject *
call_in_temporary_c_thread(PyObject *self, PyObject *args)
{
    PyObject *res = NULL;
    PyObject *callback = NULL;
    long thread;
    int wait = 1;
    if (!PyArg_ParseTuple(args, "O|i", &callback, &wait))
    {
        return NULL;
    }

    test_c_thread.start_event = PyThread_allocate_lock();
    test_c_thread.exit_event = PyThread_allocate_lock();
    test_c_thread.callback = NULL;
    if (!test_c_thread.start_event || !test_c_thread.exit_event) {
        PyErr_SetString(PyExc_RuntimeError, "could not allocate lock");
        goto exit;
    }

    test_c_thread.callback = Py_NewRef(callback);

    PyThread_acquire_lock(test_c_thread.start_event, 1);
    PyThread_acquire_lock(test_c_thread.exit_event, 1);

    thread = PyThread_start_new_thread(temporary_c_thread, &test_c_thread);
    if (thread == -1) {
        PyErr_SetString(PyExc_RuntimeError, "unable to start the thread");
        PyThread_release_lock(test_c_thread.start_event);
        PyThread_release_lock(test_c_thread.exit_event);
        goto exit;
    }

    PyThread_acquire_lock(test_c_thread.start_event, 1);
    PyThread_release_lock(test_c_thread.start_event);

    if (!wait) {
        Py_RETURN_NONE;
    }

    Py_BEGIN_ALLOW_THREADS
        PyThread_acquire_lock(test_c_thread.exit_event, 1);
        PyThread_release_lock(test_c_thread.exit_event);
    Py_END_ALLOW_THREADS

    res = Py_NewRef(Py_None);

exit:
    Py_CLEAR(test_c_thread.callback);
    if (test_c_thread.start_event) {
        PyThread_free_lock(test_c_thread.start_event);
        test_c_thread.start_event = NULL;
    }
    if (test_c_thread.exit_event) {
        PyThread_free_lock(test_c_thread.exit_event);
        test_c_thread.exit_event = NULL;
    }
    return res;
}

static PyObject *
join_temporary_c_thread(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    Py_BEGIN_ALLOW_THREADS
        PyThread_acquire_lock(test_c_thread.exit_event, 1);
        PyThread_release_lock(test_c_thread.exit_event);
    Py_END_ALLOW_THREADS
    Py_CLEAR(test_c_thread.callback);
    PyThread_free_lock(test_c_thread.start_event);
    test_c_thread.start_event = NULL;
    PyThread_free_lock(test_c_thread.exit_event);
    test_c_thread.exit_event = NULL;
    Py_RETURN_NONE;
}

/* marshal */

static PyObject*
pymarshal_write_long_to_file(PyObject* self, PyObject *args)
{
    long value;
    PyObject *filename;
    int version;
    FILE *fp;

    if (!PyArg_ParseTuple(args, "lOi:pymarshal_write_long_to_file",
                          &value, &filename, &version))
        return NULL;

    fp = _Py_fopen_obj(filename, "wb");
    if (fp == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    PyMarshal_WriteLongToFile(value, fp, version);
    assert(!PyErr_Occurred());

    fclose(fp);
    Py_RETURN_NONE;
}

static PyObject*
pymarshal_write_object_to_file(PyObject* self, PyObject *args)
{
    PyObject *obj;
    PyObject *filename;
    int version;
    FILE *fp;

    if (!PyArg_ParseTuple(args, "OOi:pymarshal_write_object_to_file",
                          &obj, &filename, &version))
        return NULL;

    fp = _Py_fopen_obj(filename, "wb");
    if (fp == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    PyMarshal_WriteObjectToFile(obj, fp, version);
    assert(!PyErr_Occurred());

    fclose(fp);
    Py_RETURN_NONE;
}

static PyObject*
pymarshal_read_short_from_file(PyObject* self, PyObject *args)
{
    int value;
    long pos;
    PyObject *filename;
    FILE *fp;

    if (!PyArg_ParseTuple(args, "O:pymarshal_read_short_from_file", &filename))
        return NULL;

    fp = _Py_fopen_obj(filename, "rb");
    if (fp == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    value = PyMarshal_ReadShortFromFile(fp);
    pos = ftell(fp);

    fclose(fp);
    if (PyErr_Occurred())
        return NULL;
    return Py_BuildValue("il", value, pos);
}

static PyObject*
pymarshal_read_long_from_file(PyObject* self, PyObject *args)
{
    long value, pos;
    PyObject *filename;
    FILE *fp;

    if (!PyArg_ParseTuple(args, "O:pymarshal_read_long_from_file", &filename))
        return NULL;

    fp = _Py_fopen_obj(filename, "rb");
    if (fp == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    value = PyMarshal_ReadLongFromFile(fp);
    pos = ftell(fp);

    fclose(fp);
    if (PyErr_Occurred())
        return NULL;
    return Py_BuildValue("ll", value, pos);
}

static PyObject*
pymarshal_read_last_object_from_file(PyObject* self, PyObject *args)
{
    PyObject *filename;
    if (!PyArg_ParseTuple(args, "O:pymarshal_read_last_object_from_file", &filename))
        return NULL;

    FILE *fp = _Py_fopen_obj(filename, "rb");
    if (fp == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    PyObject *obj = PyMarshal_ReadLastObjectFromFile(fp);
    long pos = ftell(fp);

    fclose(fp);
    if (obj == NULL) {
        return NULL;
    }
    return Py_BuildValue("Nl", obj, pos);
}

static PyObject*
pymarshal_read_object_from_file(PyObject* self, PyObject *args)
{
    PyObject *filename;
    if (!PyArg_ParseTuple(args, "O:pymarshal_read_object_from_file", &filename))
        return NULL;

    FILE *fp = _Py_fopen_obj(filename, "rb");
    if (fp == NULL) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    PyObject *obj = PyMarshal_ReadObjectFromFile(fp);
    long pos = ftell(fp);

    fclose(fp);
    if (obj == NULL) {
        return NULL;
    }
    return Py_BuildValue("Nl", obj, pos);
}

static PyObject*
return_null_without_error(PyObject *self, PyObject *args)
{
    /* invalid call: return NULL without setting an error,
     * _Py_CheckFunctionResult() must detect such bug at runtime. */
    PyErr_Clear();
    return NULL;
}

static PyObject*
return_result_with_error(PyObject *self, PyObject *args)
{
    /* invalid call: return a result with an error set,
     * _Py_CheckFunctionResult() must detect such bug at runtime. */
    PyErr_SetNone(PyExc_ValueError);
    Py_RETURN_NONE;
}

static PyObject *
getitem_with_error(PyObject *self, PyObject *args)
{
    PyObject *map, *key;
    if (!PyArg_ParseTuple(args, "OO", &map, &key)) {
        return NULL;
    }

    PyErr_SetString(PyExc_ValueError, "bug");
    return PyObject_GetItem(map, key);
}

static PyObject *
dict_get_version(PyObject *self, PyObject *args)
{
    PyDictObject *dict;
    uint64_t version;

    if (!PyArg_ParseTuple(args, "O!", &PyDict_Type, &dict))
        return NULL;

    _Py_COMP_DIAG_PUSH
    _Py_COMP_DIAG_IGNORE_DEPR_DECLS
    version = dict->ma_version_tag;
    _Py_COMP_DIAG_POP

    static_assert(sizeof(unsigned long long) >= sizeof(version),
                  "version is larger than unsigned long long");
    return PyLong_FromUnsignedLongLong((unsigned long long)version);
}


static PyObject *
raise_SIGINT_then_send_None(PyObject *self, PyObject *args)
{
    PyGenObject *gen;

    if (!PyArg_ParseTuple(args, "O!", &PyGen_Type, &gen))
        return NULL;

    /* This is used in a test to check what happens if a signal arrives just
       as we're in the process of entering a yield from chain (see
       bpo-30039).

       Needs to be done in C, because:
       - we don't have a Python wrapper for raise()
       - we need to make sure that the Python-level signal handler doesn't run
         *before* we enter the generator frame, which is impossible in Python
         because we check for signals before every bytecode operation.
     */
    raise(SIGINT);
    return PyObject_CallMethod((PyObject *)gen, "send", "O", Py_None);
}


static PyObject*
stack_pointer(PyObject *self, PyObject *args)
{
    int v = 5;
    return PyLong_FromVoidPtr(&v);
}


#ifdef W_STOPCODE
static PyObject*
py_w_stopcode(PyObject *self, PyObject *args)
{
    int sig, status;
    if (!PyArg_ParseTuple(args, "i", &sig)) {
        return NULL;
    }
    status = W_STOPCODE(sig);
    return PyLong_FromLong(status);
}
#endif


static PyObject *
test_pythread_tss_key_state(PyObject *self, PyObject *args)
{
    Py_tss_t tss_key = Py_tss_NEEDS_INIT;
    if (PyThread_tss_is_created(&tss_key)) {
        return raiseTestError(self, "test_pythread_tss_key_state",
                              "TSS key not in an uninitialized state at "
                              "creation time");
    }
    if (PyThread_tss_create(&tss_key) != 0) {
        PyErr_SetString(PyExc_RuntimeError, "PyThread_tss_create failed");
        return NULL;
    }
    if (!PyThread_tss_is_created(&tss_key)) {
        return raiseTestError(self, "test_pythread_tss_key_state",
                              "PyThread_tss_create succeeded, "
                              "but with TSS key in an uninitialized state");
    }
    if (PyThread_tss_create(&tss_key) != 0) {
        return raiseTestError(self, "test_pythread_tss_key_state",
                              "PyThread_tss_create unsuccessful with "
                              "an already initialized key");
    }
#define CHECK_TSS_API(expr) \
        (void)(expr); \
        if (!PyThread_tss_is_created(&tss_key)) { \
            return raiseTestError(self, "test_pythread_tss_key_state", \
                                  "TSS key initialization state was not " \
                                  "preserved after calling " #expr); }
    CHECK_TSS_API(PyThread_tss_set(&tss_key, NULL));
    CHECK_TSS_API(PyThread_tss_get(&tss_key));
#undef CHECK_TSS_API
    PyThread_tss_delete(&tss_key);
    if (PyThread_tss_is_created(&tss_key)) {
        return raiseTestError(self, "test_pythread_tss_key_state",
                              "PyThread_tss_delete called, but did not "
                              "set the key state to uninitialized");
    }

    Py_tss_t *ptr_key = PyThread_tss_alloc();
    if (ptr_key == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "PyThread_tss_alloc failed");
        return NULL;
    }
    if (PyThread_tss_is_created(ptr_key)) {
        return raiseTestError(self, "test_pythread_tss_key_state",
                              "TSS key not in an uninitialized state at "
                              "allocation time");
    }
    PyThread_tss_free(ptr_key);
    ptr_key = NULL;
    Py_RETURN_NONE;
}


/* def bad_get(self, obj, cls):
       cls()
       return repr(self)
*/
static PyObject*
bad_get(PyObject *module, PyObject *args)
{
    PyObject *self, *obj, *cls;
    if (!PyArg_ParseTuple(args, "OOO", &self, &obj, &cls)) {
        return NULL;
    }

    PyObject *res = PyObject_CallNoArgs(cls);
    if (res == NULL) {
        return NULL;
    }
    Py_DECREF(res);

    return PyObject_Repr(self);
}


#ifdef Py_REF_DEBUG
static PyObject *
negative_refcount(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *obj = PyUnicode_FromString("negative_refcount");
    if (obj == NULL) {
        return NULL;
    }
    assert(Py_REFCNT(obj) == 1);

    Py_SET_REFCNT(obj,  0);
    /* Py_DECREF() must call _Py_NegativeRefcount() and abort Python */
    Py_DECREF(obj);

    Py_RETURN_NONE;
}

static PyObject *
decref_freed_object(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *obj = PyUnicode_FromString("decref_freed_object");
    if (obj == NULL) {
        return NULL;
    }
    assert(Py_REFCNT(obj) == 1);

    // Deallocate the memory
    Py_DECREF(obj);
    // obj is a now a dangling pointer

    // gh-109496: If Python is built in debug mode, Py_DECREF() must call
    // _Py_NegativeRefcount() and abort Python.
    Py_DECREF(obj);

    Py_RETURN_NONE;
}
#endif


/* Functions for testing C calling conventions (METH_*) are named meth_*,
 * e.g. "meth_varargs" for METH_VARARGS.
 *
 * They all return a tuple of their C-level arguments, with None instead
 * of NULL and Python tuples instead of C arrays.
 */


static PyObject*
_null_to_none(PyObject* obj)
{
    if (obj == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(obj);
}

static PyObject*
meth_varargs(PyObject* self, PyObject* args)
{
    return Py_BuildValue("NO", _null_to_none(self), args);
}

static PyObject*
meth_varargs_keywords(PyObject* self, PyObject* args, PyObject* kwargs)
{
    return Py_BuildValue("NON", _null_to_none(self), args, _null_to_none(kwargs));
}

static PyObject*
meth_o(PyObject* self, PyObject* obj)
{
    return Py_BuildValue("NO", _null_to_none(self), obj);
}

static PyObject*
meth_noargs(PyObject* self, PyObject* ignored)
{
    return _null_to_none(self);
}

static PyObject*
_fastcall_to_tuple(PyObject* const* args, Py_ssize_t nargs)
{
    PyObject *tuple = PyTuple_New(nargs);
    if (tuple == NULL) {
        return NULL;
    }
    for (Py_ssize_t i=0; i < nargs; i++) {
        Py_INCREF(args[i]);
        PyTuple_SET_ITEM(tuple, i, args[i]);
    }
    return tuple;
}

static PyObject*
meth_fastcall(PyObject* self, PyObject* const* args, Py_ssize_t nargs)
{
    return Py_BuildValue(
        "NN", _null_to_none(self), _fastcall_to_tuple(args, nargs)
    );
}

static PyObject*
meth_fastcall_keywords(PyObject* self, PyObject* const* args,
                       Py_ssize_t nargs, PyObject* kwargs)
{
    PyObject *pyargs = _fastcall_to_tuple(args, nargs);
    if (pyargs == NULL) {
        return NULL;
    }
    assert(args != NULL || nargs == 0);
    PyObject* const* args_offset = args == NULL ? NULL : args + nargs;
    PyObject *pykwargs = PyObject_Vectorcall((PyObject*)&PyDict_Type,
                                              args_offset, 0, kwargs);
    return Py_BuildValue("NNN", _null_to_none(self), pyargs, pykwargs);
}

static PyObject*
test_pycfunction_call(PyObject *module, PyObject *args)
{
    // Function removed in the Python 3.13 API but was kept in the stable ABI.
    extern PyObject* PyCFunction_Call(PyObject *callable, PyObject *args, PyObject *kwargs);

    PyObject *func, *pos_args, *kwargs = NULL;
    if (!PyArg_ParseTuple(args, "OO!|O!", &func, &PyTuple_Type, &pos_args, &PyDict_Type, &kwargs)) {
        return NULL;
    }
    return PyCFunction_Call(func, pos_args, kwargs);
}

static PyObject*
pynumber_tobase(PyObject *module, PyObject *args)
{
    PyObject *obj;
    int base;
    if (!PyArg_ParseTuple(args, "Oi:pynumber_tobase",
                          &obj, &base)) {
        return NULL;
    }
    return PyNumber_ToBase(obj, base);
}

static PyObject*
test_set_type_size(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *obj = PyList_New(0);
    if (obj == NULL) {
        return NULL;
    }

    // Ensure that following tests don't modify the object,
    // to ensure that Py_DECREF() will not crash.
    assert(Py_TYPE(obj) == &PyList_Type);
    assert(Py_SIZE(obj) == 0);

    // bpo-39573: Test Py_SET_TYPE() and Py_SET_SIZE() functions.
    Py_SET_TYPE(obj, &PyList_Type);
    Py_SET_SIZE(obj, 0);

    Py_DECREF(obj);
    Py_RETURN_NONE;
}


// Test Py_CLEAR() macro
static PyObject*
test_py_clear(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    // simple case with a variable
    PyObject *obj = PyList_New(0);
    if (obj == NULL) {
        return NULL;
    }
    Py_CLEAR(obj);
    assert(obj == NULL);

    // gh-98724: complex case, Py_CLEAR() argument has a side effect
    PyObject* array[1];
    array[0] = PyList_New(0);
    if (array[0] == NULL) {
        return NULL;
    }

    PyObject **p = array;
    Py_CLEAR(*p++);
    assert(array[0] == NULL);
    assert(p == array + 1);

    Py_RETURN_NONE;
}


// Test Py_SETREF() and Py_XSETREF() macros, similar to test_py_clear()
static PyObject*
test_py_setref(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    // Py_SETREF() simple case with a variable
    PyObject *obj = PyList_New(0);
    if (obj == NULL) {
        return NULL;
    }
    Py_SETREF(obj, NULL);
    assert(obj == NULL);

    // Py_XSETREF() simple case with a variable
    PyObject *obj2 = PyList_New(0);
    if (obj2 == NULL) {
        return NULL;
    }
    Py_XSETREF(obj2, NULL);
    assert(obj2 == NULL);
    // test Py_XSETREF() when the argument is NULL
    Py_XSETREF(obj2, NULL);
    assert(obj2 == NULL);

    // gh-98724: complex case, Py_SETREF() argument has a side effect
    PyObject* array[1];
    array[0] = PyList_New(0);
    if (array[0] == NULL) {
        return NULL;
    }

    PyObject **p = array;
    Py_SETREF(*p++, NULL);
    assert(array[0] == NULL);
    assert(p == array + 1);

    // gh-98724: complex case, Py_XSETREF() argument has a side effect
    PyObject* array2[1];
    array2[0] = PyList_New(0);
    if (array2[0] == NULL) {
        return NULL;
    }

    PyObject **p2 = array2;
    Py_XSETREF(*p2++, NULL);
    assert(array2[0] == NULL);
    assert(p2 == array2 + 1);

    // test Py_XSETREF() when the argument is NULL
    p2 = array2;
    Py_XSETREF(*p2++, NULL);
    assert(array2[0] == NULL);
    assert(p2 == array2 + 1);

    Py_RETURN_NONE;
}


#define TEST_REFCOUNT() \
    do { \
        PyObject *obj = PyList_New(0); \
        if (obj == NULL) { \
            return NULL; \
        } \
        assert(Py_REFCNT(obj) == 1); \
        \
        /* test Py_NewRef() */ \
        PyObject *ref = Py_NewRef(obj); \
        assert(ref == obj); \
        assert(Py_REFCNT(obj) == 2); \
        Py_DECREF(ref); \
        \
        /* test Py_XNewRef() */ \
        PyObject *xref = Py_XNewRef(obj); \
        assert(xref == obj); \
        assert(Py_REFCNT(obj) == 2); \
        Py_DECREF(xref); \
        \
        assert(Py_XNewRef(NULL) == NULL); \
        \
        Py_DECREF(obj); \
        Py_RETURN_NONE; \
    } while (0) \


// Test Py_NewRef() and Py_XNewRef() macros
static PyObject*
test_refcount_macros(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    TEST_REFCOUNT();
}

#undef Py_NewRef
#undef Py_XNewRef

// Test Py_NewRef() and Py_XNewRef() functions, after undefining macros.
static PyObject*
test_refcount_funcs(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    TEST_REFCOUNT();
}


// Test Py_Is() function
#define TEST_PY_IS() \
    do { \
        PyObject *o_none = Py_None; \
        PyObject *o_true = Py_True; \
        PyObject *o_false = Py_False; \
        PyObject *obj = PyList_New(0); \
        if (obj == NULL) { \
            return NULL; \
        } \
        \
        /* test Py_Is() */ \
        assert(Py_Is(obj, obj)); \
        assert(!Py_Is(obj, o_none)); \
        \
        /* test Py_None */ \
        assert(Py_Is(o_none, o_none)); \
        assert(!Py_Is(obj, o_none)); \
        \
        /* test Py_True */ \
        assert(Py_Is(o_true, o_true)); \
        assert(!Py_Is(o_false, o_true)); \
        assert(!Py_Is(obj, o_true)); \
        \
        /* test Py_False */ \
        assert(Py_Is(o_false, o_false)); \
        assert(!Py_Is(o_true, o_false)); \
        assert(!Py_Is(obj, o_false)); \
        \
        Py_DECREF(obj); \
        Py_RETURN_NONE; \
    } while (0)

// Test Py_Is() macro
static PyObject*
test_py_is_macros(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    TEST_PY_IS();
}

#undef Py_Is

// Test Py_Is() function, after undefining its macro.
static PyObject*
test_py_is_funcs(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    TEST_PY_IS();
}


// type->tp_version_tag
static PyObject *
type_get_version(PyObject *self, PyObject *type)
{
    if (!PyType_Check(type)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a type");
        return NULL;
    }
    PyObject *res = PyLong_FromUnsignedLong(
        ((PyTypeObject *)type)->tp_version_tag);
    if (res == NULL) {
        assert(PyErr_Occurred());
        return NULL;
    }
    return res;
}


static PyObject *
type_assign_version(PyObject *self, PyObject *type)
{
    if (!PyType_Check(type)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a type");
        return NULL;
    }
    int res = PyUnstable_Type_AssignVersionTag((PyTypeObject *)type);
    return PyLong_FromLong(res);
}


static PyObject *
type_get_tp_bases(PyObject *self, PyObject *type)
{
    PyObject *bases = ((PyTypeObject *)type)->tp_bases;
    if (bases == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(bases);
}

static PyObject *
type_get_tp_mro(PyObject *self, PyObject *type)
{
    PyObject *mro = ((PyTypeObject *)type)->tp_mro;
    if (mro == NULL) {
        Py_RETURN_NONE;
    }
    return Py_NewRef(mro);
}


/* We only use 2 in test_capi/test_misc.py. */
#define NUM_BASIC_STATIC_TYPES 2
static PyTypeObject BasicStaticTypes[NUM_BASIC_STATIC_TYPES] = {
#define INIT_BASIC_STATIC_TYPE \
    { \
        PyVarObject_HEAD_INIT(NULL, 0) \
        .tp_name = "BasicStaticType", \
        .tp_basicsize = sizeof(PyObject), \
    }
    INIT_BASIC_STATIC_TYPE,
    INIT_BASIC_STATIC_TYPE,
#undef INIT_BASIC_STATIC_TYPE
};
static int num_basic_static_types_used = 0;

static PyObject *
get_basic_static_type(PyObject *self, PyObject *args)
{
    PyObject *base = NULL;
    if (!PyArg_ParseTuple(args, "|O", &base)) {
        return NULL;
    }
    assert(base == NULL || PyType_Check(base));

    if(num_basic_static_types_used >= NUM_BASIC_STATIC_TYPES) {
        PyErr_SetString(PyExc_RuntimeError, "no more available basic static types");
        return NULL;
    }
    PyTypeObject *cls = &BasicStaticTypes[num_basic_static_types_used++];

    if (base != NULL) {
        cls->tp_bases = Py_BuildValue("(O)", base);
        if (cls->tp_bases == NULL) {
            return NULL;
        }
        cls->tp_base = (PyTypeObject *)Py_NewRef(base);
    }
    if (PyType_Ready(cls) < 0) {
        Py_DECREF(cls->tp_bases);
        Py_DECREF(cls->tp_base);
        return NULL;
    }
    return (PyObject *)cls;
}


// Test PyThreadState C API
static PyObject *
test_tstate_capi(PyObject *self, PyObject *Py_UNUSED(args))
{
    // PyThreadState_Get()
    PyThreadState *tstate = PyThreadState_Get();
    assert(tstate != NULL);

    // PyThreadState_GET()
    PyThreadState *tstate2 = PyThreadState_Get();
    assert(tstate2 == tstate);

    // PyThreadState_GetUnchecked()
    PyThreadState *tstate3 = PyThreadState_GetUnchecked();
    assert(tstate3 == tstate);

    // PyThreadState_EnterTracing(), PyThreadState_LeaveTracing()
    PyThreadState_EnterTracing(tstate);
    PyThreadState_LeaveTracing(tstate);

    // PyThreadState_GetDict(): no tstate argument
    PyObject *dict = PyThreadState_GetDict();
    // PyThreadState_GetDict() API can return NULL if PyDict_New() fails,
    // but it should not occur in practice.
    assert(dict != NULL);
    assert(PyDict_Check(dict));
    // dict is a borrowed reference

    // PyThreadState_GetInterpreter()
    PyInterpreterState *interp = PyThreadState_GetInterpreter(tstate);
    assert(interp != NULL);

    // PyThreadState_GetFrame()
    PyFrameObject*frame = PyThreadState_GetFrame(tstate);
    assert(frame != NULL);
    assert(PyFrame_Check(frame));
    Py_DECREF(frame);

    // PyThreadState_GetID()
    uint64_t id = PyThreadState_GetID(tstate);
    assert(id >= 1);

    Py_RETURN_NONE;
}

static PyObject *
frame_getlocals(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    return PyFrame_GetLocals((PyFrameObject *)frame);
}

static PyObject *
frame_getglobals(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    return PyFrame_GetGlobals((PyFrameObject *)frame);
}

static PyObject *
frame_getgenerator(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    return PyFrame_GetGenerator((PyFrameObject *)frame);
}

static PyObject *
frame_getbuiltins(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    return PyFrame_GetBuiltins((PyFrameObject *)frame);
}

static PyObject *
frame_getlasti(PyObject *self, PyObject *frame)
{
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }
    int lasti = PyFrame_GetLasti((PyFrameObject *)frame);
    if (lasti < 0) {
        assert(lasti == -1);
        Py_RETURN_NONE;
    }
    return PyLong_FromLong(lasti);
}

static PyObject *
frame_new(PyObject *self, PyObject *args)
{
    PyObject *code, *globals, *locals;
    if (!PyArg_ParseTuple(args, "OOO", &code, &globals, &locals)) {
        return NULL;
    }
    if (!PyCode_Check(code)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a code object");
        return NULL;
    }
    PyThreadState *tstate = PyThreadState_Get();

    return (PyObject *)PyFrame_New(tstate, (PyCodeObject *)code, globals, locals);
}

static PyObject *
test_frame_getvar(PyObject *self, PyObject *args)
{
    PyObject *frame, *name;
    if (!PyArg_ParseTuple(args, "OO", &frame, &name)) {
        return NULL;
    }
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }

    return PyFrame_GetVar((PyFrameObject *)frame, name);
}

static PyObject *
test_frame_getvarstring(PyObject *self, PyObject *args)
{
    PyObject *frame;
    const char *name;
    if (!PyArg_ParseTuple(args, "Oy", &frame, &name)) {
        return NULL;
    }
    if (!PyFrame_Check(frame)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a frame");
        return NULL;
    }

    return PyFrame_GetVarString((PyFrameObject *)frame, name);
}


static PyObject *
eval_get_func_name(PyObject *self, PyObject *func)
{
    return PyUnicode_FromString(PyEval_GetFuncName(func));
}

static PyObject *
eval_get_func_desc(PyObject *self, PyObject *func)
{
    return PyUnicode_FromString(PyEval_GetFuncDesc(func));
}

static PyObject *
gen_get_code(PyObject *self, PyObject *gen)
{
    if (!PyGen_Check(gen)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a generator object");
        return NULL;
    }
    return (PyObject *)PyGen_GetCode((PyGenObject *)gen);
}

static PyObject *
eval_eval_code_ex(PyObject *mod, PyObject *pos_args)
{
    PyObject *result = NULL;
    PyObject *code;
    PyObject *globals;
    PyObject *locals = NULL;
    PyObject *args = NULL;
    PyObject *kwargs = NULL;
    PyObject *defaults = NULL;
    PyObject *kw_defaults = NULL;
    PyObject *closure = NULL;

    PyObject **c_kwargs = NULL;

    if (!PyArg_UnpackTuple(pos_args,
                           "eval_code_ex",
                           2,
                           8,
                           &code,
                           &globals,
                           &locals,
                           &args,
                           &kwargs,
                           &defaults,
                           &kw_defaults,
                           &closure))
    {
        goto exit;
    }

    if (!PyCode_Check(code)) {
        PyErr_SetString(PyExc_TypeError,
                        "code must be a Python code object");
        goto exit;
    }

    if (!PyDict_Check(globals)) {
        PyErr_SetString(PyExc_TypeError, "globals must be a dict");
        goto exit;
    }

    if (locals && !PyMapping_Check(locals)) {
        PyErr_SetString(PyExc_TypeError, "locals must be a mapping");
        goto exit;
    }
    if (locals == Py_None) {
        locals = NULL;
    }

    PyObject **c_args = NULL;
    Py_ssize_t c_args_len = 0;

    if (args)
    {
        if (!PyTuple_Check(args)) {
            PyErr_SetString(PyExc_TypeError, "args must be a tuple");
            goto exit;
        } else {
            c_args = &PyTuple_GET_ITEM(args, 0);
            c_args_len = PyTuple_Size(args);
        }
    }

    Py_ssize_t c_kwargs_len = 0;

    if (kwargs)
    {
        if (!PyDict_Check(kwargs)) {
            PyErr_SetString(PyExc_TypeError, "keywords must be a dict");
            goto exit;
        } else {
            c_kwargs_len = PyDict_Size(kwargs);
            if (c_kwargs_len > 0) {
                c_kwargs = PyMem_NEW(PyObject*, 2 * c_kwargs_len);
                if (!c_kwargs) {
                    PyErr_NoMemory();
                    goto exit;
                }

                Py_ssize_t i = 0;
                Py_ssize_t pos = 0;

                while (PyDict_Next(kwargs,
                                   &pos,
                                   &c_kwargs[i],
                                   &c_kwargs[i + 1]))
                {
                    i += 2;
                }
                c_kwargs_len = i / 2;
                /* XXX This is broken if the caller deletes dict items! */
            }
        }
    }


    PyObject **c_defaults = NULL;
    Py_ssize_t c_defaults_len = 0;

    if (defaults && PyTuple_Check(defaults)) {
        c_defaults = &PyTuple_GET_ITEM(defaults, 0);
        c_defaults_len = PyTuple_Size(defaults);
    }

    if (kw_defaults && !PyDict_Check(kw_defaults)) {
        PyErr_SetString(PyExc_TypeError, "kw_defaults must be a dict");
        goto exit;
    }

    if (closure && !PyTuple_Check(closure)) {
        PyErr_SetString(PyExc_TypeError, "closure must be a tuple of cells");
        goto exit;
    }


    result = PyEval_EvalCodeEx(
        code,
        globals,
        locals,
        c_args,
        (int)c_args_len,
        c_kwargs,
        (int)c_kwargs_len,
        c_defaults,
        (int)c_defaults_len,
        kw_defaults,
        closure
    );

exit:
    if (c_kwargs) {
        PyMem_DEL(c_kwargs);
    }

    return result;
}

static PyObject *
get_feature_macros(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyObject *result = PyDict_New();
    if (!result) {
        return NULL;
    }
    int res;
#include "_testcapi_feature_macros.inc"
    return result;
}

static PyObject *
test_code_api(PyObject *self, PyObject *Py_UNUSED(args))
{
    PyCodeObject *co = PyCode_NewEmpty("_testcapi", "dummy", 1);
    if (co == NULL) {
        return NULL;
    }
    /* co_code */
    {
        PyObject *co_code = PyCode_GetCode(co);
        if (co_code == NULL) {
            goto fail;
        }
        assert(PyBytes_CheckExact(co_code));
        if (PyObject_Length(co_code) == 0) {
            PyErr_SetString(PyExc_ValueError, "empty co_code");
            Py_DECREF(co_code);
            goto fail;
        }
        Py_DECREF(co_code);
    }
    /* co_varnames */
    {
        PyObject *co_varnames = PyCode_GetVarnames(co);
        if (co_varnames == NULL) {
            goto fail;
        }
        if (!PyTuple_CheckExact(co_varnames)) {
            PyErr_SetString(PyExc_TypeError, "co_varnames not tuple");
            Py_DECREF(co_varnames);
            goto fail;
        }
        if (PyTuple_GET_SIZE(co_varnames) != 0) {
            PyErr_SetString(PyExc_ValueError, "non-empty co_varnames");
            Py_DECREF(co_varnames);
            goto fail;
        }
        Py_DECREF(co_varnames);
    }
    /* co_cellvars */
    {
        PyObject *co_cellvars = PyCode_GetCellvars(co);
        if (co_cellvars == NULL) {
            goto fail;
        }
        if (!PyTuple_CheckExact(co_cellvars)) {
            PyErr_SetString(PyExc_TypeError, "co_cellvars not tuple");
            Py_DECREF(co_cellvars);
            goto fail;
        }
        if (PyTuple_GET_SIZE(co_cellvars) != 0) {
            PyErr_SetString(PyExc_ValueError, "non-empty co_cellvars");
            Py_DECREF(co_cellvars);
            goto fail;
        }
        Py_DECREF(co_cellvars);
    }
    /* co_freevars */
    {
        PyObject *co_freevars = PyCode_GetFreevars(co);
        if (co_freevars == NULL) {
            goto fail;
        }
        if (!PyTuple_CheckExact(co_freevars)) {
            PyErr_SetString(PyExc_TypeError, "co_freevars not tuple");
            Py_DECREF(co_freevars);
            goto fail;
        }
        if (PyTuple_GET_SIZE(co_freevars) != 0) {
            PyErr_SetString(PyExc_ValueError, "non-empty co_freevars");
            Py_DECREF(co_freevars);
            goto fail;
        }
        Py_DECREF(co_freevars);
    }
    Py_DECREF(co);
    Py_RETURN_NONE;
fail:
    Py_DECREF(co);
    return NULL;
}

static int
record_func(PyObject *obj, PyFrameObject *f, int what, PyObject *arg)
{
    assert(PyList_Check(obj));
    PyObject *what_obj = NULL;
    PyObject *line_obj = NULL;
    PyObject *tuple = NULL;
    int res = -1;
    what_obj = PyLong_FromLong(what);
    if (what_obj == NULL) {
        goto error;
    }
    int line = PyFrame_GetLineNumber(f);
    line_obj = PyLong_FromLong(line);
    if (line_obj == NULL) {
        goto error;
    }
    tuple = PyTuple_Pack(3, what_obj, line_obj, arg);
    if (tuple == NULL) {
        goto error;
    }
    PyTuple_SET_ITEM(tuple, 0, what_obj);
    if (PyList_Append(obj, tuple)) {
        goto error;
    }
    res = 0;
error:
    Py_XDECREF(what_obj);
    Py_XDECREF(line_obj);
    Py_XDECREF(tuple);
    return res;
}

static PyObject *
settrace_to_record(PyObject *self, PyObject *list)
{

   if (!PyList_Check(list)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a list");
        return NULL;
    }
    PyEval_SetTrace(record_func, list);
    Py_RETURN_NONE;
}

static int
error_func(PyObject *obj, PyFrameObject *f, int what, PyObject *arg)
{
    assert(PyList_Check(obj));
    /* Only raise if list is empty, otherwise append None
     * This ensures that we only raise once */
    if (PyList_GET_SIZE(obj)) {
        return 0;
    }
    if (PyList_Append(obj, Py_None)) {
       return -1;
    }
    PyErr_SetString(PyExc_Exception, "an exception");
    return -1;
}

static PyObject *
settrace_to_error(PyObject *self, PyObject *list)
{
    if (!PyList_Check(list)) {
        PyErr_SetString(PyExc_TypeError, "argument must be a list");
        return NULL;
    }
    PyEval_SetTrace(error_func, list);
    Py_RETURN_NONE;
}

static PyObject *
clear_managed_dict(PyObject *self, PyObject *obj)
{
    PyObject_ClearManagedDict(obj);
    Py_RETURN_NONE;
}


static PyObject *
test_macros(PyObject *self, PyObject *Py_UNUSED(args))
{
    struct MyStruct {
        int x;
    };
    wchar_t array[3];

    // static_assert(), Py_BUILD_ASSERT()
    static_assert(1 == 1, "bug");
    Py_BUILD_ASSERT(1 == 1);


    // Py_MIN(), Py_MAX(), Py_ABS()
    assert(Py_MIN(5, 11) == 5);
    assert(Py_MAX(5, 11) == 11);
    assert(Py_ABS(-5) == 5);

    // Py_STRINGIFY()
    assert(strcmp(Py_STRINGIFY(123), "123") == 0);

    // Py_MEMBER_SIZE(), Py_ARRAY_LENGTH()
    assert(Py_MEMBER_SIZE(struct MyStruct, x) == sizeof(int));
    assert(Py_ARRAY_LENGTH(array) == 3);

    // Py_CHARMASK()
    int c = 0xab00 | 7;
    assert(Py_CHARMASK(c) == 7);

    // _Py_IS_TYPE_SIGNED()
    assert(_Py_IS_TYPE_SIGNED(int));
    assert(!_Py_IS_TYPE_SIGNED(unsigned int));

    Py_RETURN_NONE;
}

static PyObject *
function_get_code(PyObject *self, PyObject *func)
{
    PyObject *code = PyFunction_GetCode(func);
    if (code != NULL) {
        return Py_NewRef(code);
    } else {
        return NULL;
    }
}

static PyObject *
function_get_globals(PyObject *self, PyObject *func)
{
    PyObject *globals = PyFunction_GetGlobals(func);
    if (globals != NULL) {
        return Py_NewRef(globals);
    } else {
        return NULL;
    }
}

static PyObject *
function_get_module(PyObject *self, PyObject *func)
{
    PyObject *module = PyFunction_GetModule(func);
    if (module != NULL) {
        return Py_NewRef(module);
    } else {
        return NULL;
    }
}

static PyObject *
function_get_defaults(PyObject *self, PyObject *func)
{
    PyObject *defaults = PyFunction_GetDefaults(func);
    if (defaults != NULL) {
        return Py_NewRef(defaults);
    } else if (PyErr_Occurred()) {
        return NULL;
    } else {
        Py_RETURN_NONE;  // This can happen when `defaults` are set to `None`
    }
}

static PyObject *
function_set_defaults(PyObject *self, PyObject *args)
{
    PyObject *func = NULL, *defaults = NULL;
    if (!PyArg_ParseTuple(args, "OO", &func, &defaults)) {
        return NULL;
    }
    int result = PyFunction_SetDefaults(func, defaults);
    if (result == -1)
        return NULL;
    Py_RETURN_NONE;
}

static PyObject *
function_get_kw_defaults(PyObject *self, PyObject *func)
{
    PyObject *defaults = PyFunction_GetKwDefaults(func);
    if (defaults != NULL) {
        return Py_NewRef(defaults);
    } else if (PyErr_Occurred()) {
        return NULL;
    } else {
        Py_RETURN_NONE;  // This can happen when `kwdefaults` are set to `None`
    }
}

static PyObject *
function_set_kw_defaults(PyObject *self, PyObject *args)
{
    PyObject *func = NULL, *defaults = NULL;
    if (!PyArg_ParseTuple(args, "OO", &func, &defaults)) {
        return NULL;
    }
    int result = PyFunction_SetKwDefaults(func, defaults);
    if (result == -1)
        return NULL;
    Py_RETURN_NONE;
}

static PyObject *
check_pyimport_addmodule(PyObject *self, PyObject *args)
{
    const char *name;
    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }

    // test PyImport_AddModuleRef()
    PyObject *module = PyImport_AddModuleRef(name);
    if (module == NULL) {
        return NULL;
    }
    assert(PyModule_Check(module));
    // module is a strong reference

    // test PyImport_AddModule()
    PyObject *module2 = PyImport_AddModule(name);
    if (module2 == NULL) {
        goto error;
    }
    assert(PyModule_Check(module2));
    assert(module2 == module);
    // module2 is a borrowed ref

    // test PyImport_AddModuleObject()
    PyObject *name_obj = PyUnicode_FromString(name);
    if (name_obj == NULL) {
        goto error;
    }
    PyObject *module3 = PyImport_AddModuleObject(name_obj);
    Py_DECREF(name_obj);
    if (module3 == NULL) {
        goto error;
    }
    assert(PyModule_Check(module3));
    assert(module3 == module);
    // module3 is a borrowed ref

    return module;

error:
    Py_DECREF(module);
    return NULL;
}


static PyObject *
test_weakref_capi(PyObject *Py_UNUSED(module), PyObject *Py_UNUSED(args))
{
    // Ignore PyWeakref_GetObject() deprecation, we test it on purpose
    _Py_COMP_DIAG_PUSH
    _Py_COMP_DIAG_IGNORE_DEPR_DECLS

    // Create a new heap type, create an instance of this type, and delete the
    // type. This object supports weak references.
    PyObject *new_type = PyObject_CallFunction((PyObject*)&PyType_Type,
                                               "s(){}", "TypeName");
    if (new_type == NULL) {
        return NULL;
    }
    PyObject *obj = PyObject_CallNoArgs(new_type);
    Py_DECREF(new_type);
    if (obj == NULL) {
        return NULL;
    }
    Py_ssize_t refcnt = Py_REFCNT(obj);

    // test PyWeakref_NewRef(), reference is alive
    PyObject *weakref = PyWeakref_NewRef(obj, NULL);
    if (weakref == NULL) {
        Py_DECREF(obj);
        return NULL;
    }

    // test PyWeakref_Check(), valid weakref object
    assert(PyWeakref_Check(weakref));
    assert(PyWeakref_CheckRefExact(weakref));
    assert(PyWeakref_CheckRefExact(weakref));
    assert(Py_REFCNT(obj) == refcnt);

    // test PyWeakref_GetRef(), reference is alive
    PyObject *ref = UNINITIALIZED_PTR;
    assert(PyWeakref_GetRef(weakref, &ref) == 1);
    assert(ref == obj);
    assert(Py_REFCNT(obj) == (refcnt + 1));
    Py_DECREF(ref);

    // test PyWeakref_GetObject(), reference is alive
    ref = PyWeakref_GetObject(weakref);  // borrowed ref
    assert(ref == obj);

    // test PyWeakref_GET_OBJECT(), reference is alive
    ref = PyWeakref_GET_OBJECT(weakref);  // borrowed ref
    assert(ref == obj);

    // delete the referenced object: clear the weakref
    assert(Py_REFCNT(obj) == 1);
    Py_DECREF(obj);

    // test PyWeakref_GET_OBJECT(), reference is dead
    assert(PyWeakref_GET_OBJECT(weakref) == Py_None);

    // test PyWeakref_GetRef(), reference is dead
    ref = UNINITIALIZED_PTR;
    assert(PyWeakref_GetRef(weakref, &ref) == 0);
    assert(ref == NULL);

    // test PyWeakref_Check(), not a weakref object
    PyObject *invalid_weakref = Py_None;
    assert(!PyWeakref_Check(invalid_weakref));
    assert(!PyWeakref_CheckRefExact(invalid_weakref));
    assert(!PyWeakref_CheckRefExact(invalid_weakref));

    // test PyWeakref_GetRef(), invalid type
    assert(!PyErr_Occurred());
    ref = UNINITIALIZED_PTR;
    assert(PyWeakref_GetRef(invalid_weakref, &ref) == -1);
    assert(PyErr_ExceptionMatches(PyExc_TypeError));
    PyErr_Clear();
    assert(ref == NULL);

    // test PyWeakref_GetObject(), invalid type
    assert(PyWeakref_GetObject(invalid_weakref) == NULL);
    assert(PyErr_ExceptionMatches(PyExc_SystemError));
    PyErr_Clear();

    // test PyWeakref_GetRef(NULL)
    ref = UNINITIALIZED_PTR;
    assert(PyWeakref_GetRef(NULL, &ref) == -1);
    assert(PyErr_ExceptionMatches(PyExc_SystemError));
    assert(ref == NULL);
    PyErr_Clear();

    // test PyWeakref_GetObject(NULL)
    assert(PyWeakref_GetObject(NULL) == NULL);
    assert(PyErr_ExceptionMatches(PyExc_SystemError));
    PyErr_Clear();

    Py_DECREF(weakref);

    Py_RETURN_NONE;

    _Py_COMP_DIAG_POP
}


static PyMethodDef TestMethods[] = {
    {"set_errno",               set_errno,                       METH_VARARGS},
    {"test_config",             test_config,                     METH_NOARGS},
    {"test_sizeof_c_types",     test_sizeof_c_types,             METH_NOARGS},
    {"test_list_api",           test_list_api,                   METH_NOARGS},
    {"test_dict_iteration",     test_dict_iteration,             METH_NOARGS},
    {"test_lazy_hash_inheritance",      test_lazy_hash_inheritance,METH_NOARGS},
    {"test_xincref_doesnt_leak",test_xincref_doesnt_leak,        METH_NOARGS},
    {"test_incref_doesnt_leak", test_incref_doesnt_leak,         METH_NOARGS},
    {"test_xdecref_doesnt_leak",test_xdecref_doesnt_leak,        METH_NOARGS},
    {"test_decref_doesnt_leak", test_decref_doesnt_leak,         METH_NOARGS},
    {"test_structseq_newtype_doesnt_leak",
        test_structseq_newtype_doesnt_leak, METH_NOARGS},
    {"test_structseq_newtype_null_descr_doc",
        test_structseq_newtype_null_descr_doc, METH_NOARGS},
    {"test_incref_decref_API",  test_incref_decref_API,          METH_NOARGS},
    {"pyobject_repr_from_null", pyobject_repr_from_null, METH_NOARGS},
    {"pyobject_str_from_null",  pyobject_str_from_null, METH_NOARGS},
    {"pyobject_bytes_from_null", pyobject_bytes_from_null, METH_NOARGS},
    {"test_string_to_double",   test_string_to_double,           METH_NOARGS},
    {"test_capsule", (PyCFunction)test_capsule, METH_NOARGS},
    {"test_from_contiguous", (PyCFunction)test_from_contiguous, METH_NOARGS},
#if (defined(__linux__) || defined(__FreeBSD__)) && defined(__GNUC__)
    {"test_pep3118_obsolete_write_locks", (PyCFunction)test_pep3118_obsolete_write_locks, METH_NOARGS},
#endif
    {"getbuffer_with_null_view", getbuffer_with_null_view,       METH_O},
    {"PyBuffer_SizeFromFormat",  test_PyBuffer_SizeFromFormat,   METH_VARARGS},
    {"py_buildvalue",            py_buildvalue,                  METH_VARARGS},
    {"py_buildvalue_ints",       py_buildvalue_ints,             METH_VARARGS},
    {"test_buildvalue_N",        test_buildvalue_N,              METH_NOARGS},
    {"test_get_statictype_slots", test_get_statictype_slots,     METH_NOARGS},
    {"get_heaptype_for_name",     get_heaptype_for_name,         METH_NOARGS},
    {"test_get_type_name",        test_get_type_name,            METH_NOARGS},
    {"test_get_type_qualname",    test_get_type_qualname,        METH_NOARGS},
    {"test_get_type_dict",        test_get_type_dict,            METH_NOARGS},
    {"_test_thread_state",      test_thread_state,               METH_VARARGS},
#ifndef MS_WINDOWS
    {"_spawn_pthread_waiter",   spawn_pthread_waiter,            METH_NOARGS},
    {"_end_spawned_pthread",    end_spawned_pthread,             METH_NOARGS},
#endif
    {"_pending_threadfunc",     pending_threadfunc,              METH_VARARGS},
#ifdef HAVE_GETTIMEOFDAY
    {"profile_int",             profile_int,                     METH_NOARGS},
#endif
    {"argparsing",              argparsing,                      METH_VARARGS},
    {"code_newempty",           code_newempty,                   METH_VARARGS},
    {"eval_code_ex",            eval_eval_code_ex,               METH_VARARGS},
    {"make_memoryview_from_NULL_pointer", make_memoryview_from_NULL_pointer,
     METH_NOARGS},
    {"crash_no_current_thread", crash_no_current_thread,         METH_NOARGS},
    {"test_current_tstate_matches", test_current_tstate_matches, METH_NOARGS},
    {"run_in_subinterp",        run_in_subinterp,                METH_VARARGS},
    {"get_interpreterid_type",  get_interpreterid_type,          METH_NOARGS},
    {"link_interpreter_refcount", link_interpreter_refcount,     METH_O},
    {"unlink_interpreter_refcount", unlink_interpreter_refcount, METH_O},
    {"create_cfunction",        create_cfunction,                METH_NOARGS},
    {"call_in_temporary_c_thread", call_in_temporary_c_thread, METH_VARARGS,
     PyDoc_STR("set_error_class(error_class) -> None")},
    {"join_temporary_c_thread", join_temporary_c_thread, METH_NOARGS},
    {"pymarshal_write_long_to_file",
        pymarshal_write_long_to_file, METH_VARARGS},
    {"pymarshal_write_object_to_file",
        pymarshal_write_object_to_file, METH_VARARGS},
    {"pymarshal_read_short_from_file",
        pymarshal_read_short_from_file, METH_VARARGS},
    {"pymarshal_read_long_from_file",
        pymarshal_read_long_from_file, METH_VARARGS},
    {"pymarshal_read_last_object_from_file",
        pymarshal_read_last_object_from_file, METH_VARARGS},
    {"pymarshal_read_object_from_file",
        pymarshal_read_object_from_file, METH_VARARGS},
    {"return_null_without_error", return_null_without_error, METH_NOARGS},
    {"return_result_with_error", return_result_with_error, METH_NOARGS},
    {"getitem_with_error", getitem_with_error, METH_VARARGS},
    {"Py_CompileString",     pycompilestring, METH_O},
    {"dict_get_version", dict_get_version, METH_VARARGS},
    {"raise_SIGINT_then_send_None", raise_SIGINT_then_send_None, METH_VARARGS},
    {"stack_pointer", stack_pointer, METH_NOARGS},
#ifdef W_STOPCODE
    {"W_STOPCODE", py_w_stopcode, METH_VARARGS},
#endif
    {"test_pythread_tss_key_state", test_pythread_tss_key_state, METH_VARARGS},
    {"bad_get", bad_get, METH_VARARGS},
#ifdef Py_REF_DEBUG
    {"negative_refcount", negative_refcount, METH_NOARGS},
    {"decref_freed_object", decref_freed_object, METH_NOARGS},
#endif
    {"meth_varargs", meth_varargs, METH_VARARGS},
    {"meth_varargs_keywords", _PyCFunction_CAST(meth_varargs_keywords), METH_VARARGS|METH_KEYWORDS},
    {"meth_o", meth_o, METH_O},
    {"meth_noargs", meth_noargs, METH_NOARGS},
    {"meth_fastcall", _PyCFunction_CAST(meth_fastcall), METH_FASTCALL},
    {"meth_fastcall_keywords", _PyCFunction_CAST(meth_fastcall_keywords), METH_FASTCALL|METH_KEYWORDS},
    {"pycfunction_call", test_pycfunction_call, METH_VARARGS},
    {"pynumber_tobase", pynumber_tobase, METH_VARARGS},
    {"test_set_type_size", test_set_type_size, METH_NOARGS},
    {"test_py_clear", test_py_clear, METH_NOARGS},
    {"test_py_setref", test_py_setref, METH_NOARGS},
    {"test_refcount_macros", test_refcount_macros, METH_NOARGS},
    {"test_refcount_funcs", test_refcount_funcs, METH_NOARGS},
    {"test_py_is_macros", test_py_is_macros, METH_NOARGS},
    {"test_py_is_funcs", test_py_is_funcs, METH_NOARGS},
    {"type_get_version", type_get_version, METH_O, PyDoc_STR("type->tp_version_tag")},
    {"type_assign_version", type_assign_version, METH_O, PyDoc_STR("PyUnstable_Type_AssignVersionTag")},
    {"type_get_tp_bases", type_get_tp_bases, METH_O},
    {"type_get_tp_mro", type_get_tp_mro, METH_O},
    {"get_basic_static_type", get_basic_static_type, METH_VARARGS, NULL},
    {"test_tstate_capi", test_tstate_capi, METH_NOARGS, NULL},
    {"frame_getlocals", frame_getlocals, METH_O, NULL},
    {"frame_getglobals", frame_getglobals, METH_O, NULL},
    {"frame_getgenerator", frame_getgenerator, METH_O, NULL},
    {"frame_getbuiltins", frame_getbuiltins, METH_O, NULL},
    {"frame_getlasti", frame_getlasti, METH_O, NULL},
    {"frame_new", frame_new, METH_VARARGS, NULL},
    {"frame_getvar", test_frame_getvar, METH_VARARGS, NULL},
    {"frame_getvarstring", test_frame_getvarstring, METH_VARARGS, NULL},
    {"eval_get_func_name", eval_get_func_name, METH_O, NULL},
    {"eval_get_func_desc", eval_get_func_desc, METH_O, NULL},
    {"gen_get_code", gen_get_code, METH_O, NULL},
    {"get_feature_macros", get_feature_macros, METH_NOARGS, NULL},
    {"test_code_api", test_code_api, METH_NOARGS, NULL},
    {"settrace_to_error", settrace_to_error, METH_O, NULL},
    {"settrace_to_record", settrace_to_record, METH_O, NULL},
    {"test_macros", test_macros, METH_NOARGS, NULL},
    {"clear_managed_dict", clear_managed_dict, METH_O, NULL},
    {"function_get_code", function_get_code, METH_O, NULL},
    {"function_get_globals", function_get_globals, METH_O, NULL},
    {"function_get_module", function_get_module, METH_O, NULL},
    {"function_get_defaults", function_get_defaults, METH_O, NULL},
    {"function_set_defaults", function_set_defaults, METH_VARARGS, NULL},
    {"function_get_kw_defaults", function_get_kw_defaults, METH_O, NULL},
    {"function_set_kw_defaults", function_set_kw_defaults, METH_VARARGS, NULL},
    {"check_pyimport_addmodule", check_pyimport_addmodule, METH_VARARGS},
    {"test_weakref_capi", test_weakref_capi, METH_NOARGS},
    {NULL, NULL} /* sentinel */
};


typedef struct {
    PyObject_HEAD
} matmulObject;

static PyObject *
matmulType_matmul(PyObject *self, PyObject *other)
{
    return Py_BuildValue("(sOO)", "matmul", self, other);
}

static PyObject *
matmulType_imatmul(PyObject *self, PyObject *other)
{
    return Py_BuildValue("(sOO)", "imatmul", self, other);
}

static void
matmulType_dealloc(PyObject *self)
{
    Py_TYPE(self)->tp_free(self);
}

static PyNumberMethods matmulType_as_number = {
    0,                          /* nb_add */
    0,                          /* nb_subtract */
    0,                          /* nb_multiply */
    0,                          /* nb_remainde r*/
    0,                          /* nb_divmod */
    0,                          /* nb_power */
    0,                          /* nb_negative */
    0,                          /* tp_positive */
    0,                          /* tp_absolute */
    0,                          /* tp_bool */
    0,                          /* nb_invert */
    0,                          /* nb_lshift */
    0,                          /* nb_rshift */
    0,                          /* nb_and */
    0,                          /* nb_xor */
    0,                          /* nb_or */
    0,                          /* nb_int */
    0,                          /* nb_reserved */
    0,                          /* nb_float */
    0,                          /* nb_inplace_add */
    0,                          /* nb_inplace_subtract */
    0,                          /* nb_inplace_multiply */
    0,                          /* nb_inplace_remainder */
    0,                          /* nb_inplace_power */
    0,                          /* nb_inplace_lshift */
    0,                          /* nb_inplace_rshift */
    0,                          /* nb_inplace_and */
    0,                          /* nb_inplace_xor */
    0,                          /* nb_inplace_or */
    0,                          /* nb_floor_divide */
    0,                          /* nb_true_divide */
    0,                          /* nb_inplace_floor_divide */
    0,                          /* nb_inplace_true_divide */
    0,                          /* nb_index */
    matmulType_matmul,        /* nb_matrix_multiply */
    matmulType_imatmul        /* nb_matrix_inplace_multiply */
};

static PyTypeObject matmulType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "matmulType",
    sizeof(matmulObject),               /* tp_basicsize */
    0,                                  /* tp_itemsize */
    matmulType_dealloc,                 /* destructor tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_as_async */
    0,                                  /* tp_repr */
    &matmulType_as_number,              /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    PyObject_GenericSetAttr,            /* tp_setattro */
    0,                                  /* tp_as_buffer */
    0,                                  /* tp_flags */
    "C level type with matrix operations defined",
    0,                                  /* traverseproc tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    PyType_GenericNew,                  /* tp_new */
    PyObject_Del,                       /* tp_free */
};

typedef struct {
    PyObject_HEAD
} ipowObject;

static PyObject *
ipowType_ipow(PyObject *self, PyObject *other, PyObject *mod)
{
    return Py_BuildValue("OO", other, mod);
}

static PyNumberMethods ipowType_as_number = {
    .nb_inplace_power = ipowType_ipow
};

static PyTypeObject ipowType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ipowType",
    .tp_basicsize = sizeof(ipowObject),
    .tp_as_number = &ipowType_as_number,
    .tp_new = PyType_GenericNew
};

typedef struct {
    PyObject_HEAD
    PyObject *ao_iterator;
} awaitObject;


static PyObject *
awaitObject_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *v;
    awaitObject *ao;

    if (!PyArg_UnpackTuple(args, "awaitObject", 1, 1, &v))
        return NULL;

    ao = (awaitObject *)type->tp_alloc(type, 0);
    if (ao == NULL) {
        return NULL;
    }

    ao->ao_iterator = Py_NewRef(v);

    return (PyObject *)ao;
}


static void
awaitObject_dealloc(awaitObject *ao)
{
    Py_CLEAR(ao->ao_iterator);
    Py_TYPE(ao)->tp_free(ao);
}


static PyObject *
awaitObject_await(awaitObject *ao)
{
    return Py_NewRef(ao->ao_iterator);
}

static PyAsyncMethods awaitType_as_async = {
    (unaryfunc)awaitObject_await,           /* am_await */
    0,                                      /* am_aiter */
    0,                                      /* am_anext */
    0,                                      /* am_send  */
};


static PyTypeObject awaitType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "awaitType",
    sizeof(awaitObject),                /* tp_basicsize */
    0,                                  /* tp_itemsize */
    (destructor)awaitObject_dealloc,    /* destructor tp_dealloc */
    0,                                  /* tp_vectorcall_offset */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    &awaitType_as_async,                /* tp_as_async */
    0,                                  /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    PyObject_GenericGetAttr,            /* tp_getattro */
    PyObject_GenericSetAttr,            /* tp_setattro */
    0,                                  /* tp_as_buffer */
    0,                                  /* tp_flags */
    "C level type with tp_as_async",
    0,                                  /* traverseproc tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    0,                                  /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    0,                                  /* tp_methods */
    0,                                  /* tp_members */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    awaitObject_new,                    /* tp_new */
    PyObject_Del,                       /* tp_free */
};


/* Test bpo-35983: create a subclass of "list" which checks that instances
 * are not deallocated twice */

typedef struct {
    PyListObject list;
    int deallocated;
} MyListObject;

static PyObject *
MyList_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject* op = PyList_Type.tp_new(type, args, kwds);
    ((MyListObject*)op)->deallocated = 0;
    return op;
}

void
MyList_dealloc(MyListObject* op)
{
    if (op->deallocated) {
        /* We cannot raise exceptions here but we still want the testsuite
         * to fail when we hit this */
        Py_FatalError("MyList instance deallocated twice");
    }
    op->deallocated = 1;
    PyList_Type.tp_dealloc((PyObject *)op);
}

static PyTypeObject MyList_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "MyList",
    sizeof(MyListObject),
    0,
    (destructor)MyList_dealloc,                 /* tp_dealloc */
    0,                                          /* tp_vectorcall_offset */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
    0,                                          /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,  /* &PyList_Type */                      /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    MyList_new,                                 /* tp_new */
};

/* Test PEP 560 */

typedef struct {
    PyObject_HEAD
    PyObject *item;
} PyGenericAliasObject;

static void
generic_alias_dealloc(PyGenericAliasObject *self)
{
    Py_CLEAR(self->item);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
generic_alias_mro_entries(PyGenericAliasObject *self, PyObject *bases)
{
    return PyTuple_Pack(1, self->item);
}

static PyMethodDef generic_alias_methods[] = {
    {"__mro_entries__", _PyCFunction_CAST(generic_alias_mro_entries), METH_O, NULL},
    {NULL}  /* sentinel */
};

static PyTypeObject GenericAlias_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "GenericAlias",
    sizeof(PyGenericAliasObject),
    0,
    .tp_dealloc = (destructor)generic_alias_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_methods = generic_alias_methods,
};

static PyObject *
generic_alias_new(PyObject *item)
{
    PyGenericAliasObject *o = PyObject_New(PyGenericAliasObject, &GenericAlias_Type);
    if (o == NULL) {
        return NULL;
    }
    o->item = Py_NewRef(item);
    return (PyObject*) o;
}

typedef struct {
    PyObject_HEAD
} PyGenericObject;

static PyObject *
generic_class_getitem(PyObject *type, PyObject *item)
{
    return generic_alias_new(item);
}

static PyMethodDef generic_methods[] = {
    {"__class_getitem__", generic_class_getitem, METH_O|METH_CLASS, NULL},
    {NULL}  /* sentinel */
};

static PyTypeObject Generic_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "Generic",
    sizeof(PyGenericObject),
    0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_methods = generic_methods,
};

static PyMethodDef meth_instance_methods[] = {
    {"meth_varargs", meth_varargs, METH_VARARGS},
    {"meth_varargs_keywords", _PyCFunction_CAST(meth_varargs_keywords), METH_VARARGS|METH_KEYWORDS},
    {"meth_o", meth_o, METH_O},
    {"meth_noargs", meth_noargs, METH_NOARGS},
    {"meth_fastcall", _PyCFunction_CAST(meth_fastcall), METH_FASTCALL},
    {"meth_fastcall_keywords", _PyCFunction_CAST(meth_fastcall_keywords), METH_FASTCALL|METH_KEYWORDS},
    {NULL, NULL} /* sentinel */
};


static PyTypeObject MethInstance_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "MethInstance",
    sizeof(PyObject),
    .tp_new = PyType_GenericNew,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = meth_instance_methods,
    .tp_doc = (char*)PyDoc_STR(
        "Class with normal (instance) methods to test calling conventions"),
};

static PyMethodDef meth_class_methods[] = {
    {"meth_varargs", meth_varargs, METH_VARARGS|METH_CLASS},
    {"meth_varargs_keywords", _PyCFunction_CAST(meth_varargs_keywords), METH_VARARGS|METH_KEYWORDS|METH_CLASS},
    {"meth_o", meth_o, METH_O|METH_CLASS},
    {"meth_noargs", meth_noargs, METH_NOARGS|METH_CLASS},
    {"meth_fastcall", _PyCFunction_CAST(meth_fastcall), METH_FASTCALL|METH_CLASS},
    {"meth_fastcall_keywords", _PyCFunction_CAST(meth_fastcall_keywords), METH_FASTCALL|METH_KEYWORDS|METH_CLASS},
    {NULL, NULL} /* sentinel */
};


static PyTypeObject MethClass_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "MethClass",
    sizeof(PyObject),
    .tp_new = PyType_GenericNew,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = meth_class_methods,
    .tp_doc = PyDoc_STR(
        "Class with class methods to test calling conventions"),
};

static PyMethodDef meth_static_methods[] = {
    {"meth_varargs", meth_varargs, METH_VARARGS|METH_STATIC},
    {"meth_varargs_keywords", _PyCFunction_CAST(meth_varargs_keywords), METH_VARARGS|METH_KEYWORDS|METH_STATIC},
    {"meth_o", meth_o, METH_O|METH_STATIC},
    {"meth_noargs", meth_noargs, METH_NOARGS|METH_STATIC},
    {"meth_fastcall", _PyCFunction_CAST(meth_fastcall), METH_FASTCALL|METH_STATIC},
    {"meth_fastcall_keywords", _PyCFunction_CAST(meth_fastcall_keywords), METH_FASTCALL|METH_KEYWORDS|METH_STATIC},
    {NULL, NULL} /* sentinel */
};


static PyTypeObject MethStatic_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "MethStatic",
    sizeof(PyObject),
    .tp_new = PyType_GenericNew,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_methods = meth_static_methods,
    .tp_doc = PyDoc_STR(
        "Class with static methods to test calling conventions"),
};

/* ContainerNoGC -- a simple container without GC methods */

typedef struct {
    PyObject_HEAD
    PyObject *value;
} ContainerNoGCobject;

static PyObject *
ContainerNoGC_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *value;
    char *names[] = {"value", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", names, &value)) {
        return NULL;
    }
    PyObject *self = type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    Py_INCREF(value);
    ((ContainerNoGCobject *)self)->value = value;
    return self;
}

static void
ContainerNoGC_dealloc(ContainerNoGCobject *self)
{
    Py_DECREF(self->value);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyMemberDef ContainerNoGC_members[] = {
    {"value", _Py_T_OBJECT, offsetof(ContainerNoGCobject, value), Py_READONLY,
     PyDoc_STR("a container value for test purposes")},
    {0}
};

static PyTypeObject ContainerNoGC_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_testcapi.ContainerNoGC",
    sizeof(ContainerNoGCobject),
    .tp_dealloc = (destructor)ContainerNoGC_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_members = ContainerNoGC_members,
    .tp_new = ContainerNoGC_new,
};


static struct PyModuleDef _testcapimodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_testcapi",
    .m_size = sizeof(testcapistate_t),
    .m_methods = TestMethods,
};

/* Per PEP 489, this module will not be converted to multi-phase initialization
 */

PyMODINIT_FUNC
PyInit__testcapi(void)
{
    PyObject *m;

    m = PyModule_Create(&_testcapimodule);
    if (m == NULL)
        return NULL;

    Py_SET_TYPE(&_HashInheritanceTester_Type, &PyType_Type);

    if (PyType_Ready(&matmulType) < 0)
        return NULL;
    Py_INCREF(&matmulType);
    PyModule_AddObject(m, "matmulType", (PyObject *)&matmulType);
    if (PyType_Ready(&ipowType) < 0) {
        return NULL;
    }
    Py_INCREF(&ipowType);
    PyModule_AddObject(m, "ipowType", (PyObject *)&ipowType);

    if (PyType_Ready(&awaitType) < 0)
        return NULL;
    Py_INCREF(&awaitType);
    PyModule_AddObject(m, "awaitType", (PyObject *)&awaitType);

    MyList_Type.tp_base = &PyList_Type;
    if (PyType_Ready(&MyList_Type) < 0)
        return NULL;
    Py_INCREF(&MyList_Type);
    PyModule_AddObject(m, "MyList", (PyObject *)&MyList_Type);

    if (PyType_Ready(&GenericAlias_Type) < 0)
        return NULL;
    Py_INCREF(&GenericAlias_Type);
    PyModule_AddObject(m, "GenericAlias", (PyObject *)&GenericAlias_Type);

    if (PyType_Ready(&Generic_Type) < 0)
        return NULL;
    Py_INCREF(&Generic_Type);
    PyModule_AddObject(m, "Generic", (PyObject *)&Generic_Type);

    if (PyType_Ready(&MethInstance_Type) < 0)
        return NULL;
    Py_INCREF(&MethInstance_Type);
    PyModule_AddObject(m, "MethInstance", (PyObject *)&MethInstance_Type);

    if (PyType_Ready(&MethClass_Type) < 0)
        return NULL;
    Py_INCREF(&MethClass_Type);
    PyModule_AddObject(m, "MethClass", (PyObject *)&MethClass_Type);

    if (PyType_Ready(&MethStatic_Type) < 0)
        return NULL;
    Py_INCREF(&MethStatic_Type);
    PyModule_AddObject(m, "MethStatic", (PyObject *)&MethStatic_Type);

    PyModule_AddObject(m, "CHAR_MAX", PyLong_FromLong(CHAR_MAX));
    PyModule_AddObject(m, "CHAR_MIN", PyLong_FromLong(CHAR_MIN));
    PyModule_AddObject(m, "UCHAR_MAX", PyLong_FromLong(UCHAR_MAX));
    PyModule_AddObject(m, "SHRT_MAX", PyLong_FromLong(SHRT_MAX));
    PyModule_AddObject(m, "SHRT_MIN", PyLong_FromLong(SHRT_MIN));
    PyModule_AddObject(m, "USHRT_MAX", PyLong_FromLong(USHRT_MAX));
    PyModule_AddObject(m, "INT_MAX",  PyLong_FromLong(INT_MAX));
    PyModule_AddObject(m, "INT_MIN",  PyLong_FromLong(INT_MIN));
    PyModule_AddObject(m, "UINT_MAX",  PyLong_FromUnsignedLong(UINT_MAX));
    PyModule_AddObject(m, "LONG_MAX", PyLong_FromLong(LONG_MAX));
    PyModule_AddObject(m, "LONG_MIN", PyLong_FromLong(LONG_MIN));
    PyModule_AddObject(m, "ULONG_MAX", PyLong_FromUnsignedLong(ULONG_MAX));
    PyModule_AddObject(m, "FLT_MAX", PyFloat_FromDouble(FLT_MAX));
    PyModule_AddObject(m, "FLT_MIN", PyFloat_FromDouble(FLT_MIN));
    PyModule_AddObject(m, "DBL_MAX", PyFloat_FromDouble(DBL_MAX));
    PyModule_AddObject(m, "DBL_MIN", PyFloat_FromDouble(DBL_MIN));
    PyModule_AddObject(m, "LLONG_MAX", PyLong_FromLongLong(LLONG_MAX));
    PyModule_AddObject(m, "LLONG_MIN", PyLong_FromLongLong(LLONG_MIN));
    PyModule_AddObject(m, "ULLONG_MAX", PyLong_FromUnsignedLongLong(ULLONG_MAX));
    PyModule_AddObject(m, "PY_SSIZE_T_MAX", PyLong_FromSsize_t(PY_SSIZE_T_MAX));
    PyModule_AddObject(m, "PY_SSIZE_T_MIN", PyLong_FromSsize_t(PY_SSIZE_T_MIN));
    PyModule_AddObject(m, "SIZE_MAX", PyLong_FromSize_t(SIZE_MAX));
    PyModule_AddObject(m, "SIZEOF_WCHAR_T", PyLong_FromSsize_t(sizeof(wchar_t)));
    PyModule_AddObject(m, "SIZEOF_VOID_P", PyLong_FromSsize_t(sizeof(void*)));
    PyModule_AddObject(m, "SIZEOF_TIME_T", PyLong_FromSsize_t(sizeof(time_t)));
    PyModule_AddObject(m, "Py_Version", PyLong_FromUnsignedLong(Py_Version));
    Py_INCREF(&PyInstanceMethod_Type);
    PyModule_AddObject(m, "instancemethod", (PyObject *)&PyInstanceMethod_Type);

    PyModule_AddIntConstant(m, "the_number_three", 3);
    PyModule_AddIntMacro(m, Py_C_RECURSION_LIMIT);

    testcapistate_t *state = get_testcapi_state(m);
    state->error = PyErr_NewException("_testcapi.error", NULL, NULL);
    PyModule_AddObject(m, "error", state->error);

    if (PyType_Ready(&ContainerNoGC_type) < 0) {
        return NULL;
    }
    Py_INCREF(&ContainerNoGC_type);
    if (PyModule_AddObject(m, "ContainerNoGC",
                           (PyObject *) &ContainerNoGC_type) < 0)
        return NULL;

    /* Include tests from the _testcapi/ directory */
    if (_PyTestCapi_Init_Vectorcall(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Heaptype(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Abstract(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_ByteArray(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Bytes(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Unicode(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_GetArgs(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_DateTime(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Docstring(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Mem(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Watchers(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Long(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Float(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Complex(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Numbers(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Dict(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Set(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_List(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Tuple(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Structmember(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Exceptions(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Code(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Buffer(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_PyOS(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_File(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Codec(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Sys(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Immortal(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_GC(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_PyAtomic(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_VectorcallLimited(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_HeaptypeRelative(m) < 0) {
        return NULL;
    }
    if (_PyTestCapi_Init_Hash(m) < 0) {
        return NULL;
    }

    PyState_AddModule(m, &_testcapimodule);
    return m;
}
