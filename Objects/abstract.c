/* Abstract Object Interface (many thanks to Jim Fulton) */

#include "Python.h"
#include "pycore_abstract.h"      // _PyIndex_Check()
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_ceval.h"         // _Py_EnterRecursiveCallTstate()
#include "pycore_crossinterp.h"   // _Py_CallInInterpreter()
#include "pycore_genobject.h"     // _PyGen_FetchStopIterationValue()
#include "pycore_list.h"          // _PyList_AppendTakeRef()
#include "pycore_long.h"          // _PyLong_IsNegative()
#include "pycore_object.h"        // _Py_CheckSlotResult()
#include "pycore_pybuffer.h"      // _PyBuffer_ReleaseInInterpreterAndRawFree()
#include "pycore_pyerrors.h"      // _PyErr_Occurred()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_tuple.h"         // _PyTuple_FromArraySteal()
#include "pycore_unionobject.h"   // _PyUnion_Check()

#include <stddef.h>               // offsetof()


/* Shorthands to return certain errors */

static PyObject *
type_error(const char *msg, PyObject *obj)
{
    PyErr_Format(PyExc_TypeError, msg, Py_TYPE(obj)->tp_name);
    return NULL;
}

static PyObject *
null_error(void)
{
    PyThreadState *tstate = _PyThreadState_GET();
    if (!_PyErr_Occurred(tstate)) {
        _PyErr_SetString(tstate, PyExc_SystemError,
                         "null argument to internal routine");
    }
    return NULL;
}

/* Operations on any object */

PyObject *
PyObject_Type(PyObject *o)
{
    PyObject *v;

    if (o == NULL) {
        return null_error();
    }

    v = (PyObject *)Py_TYPE(o);
    return Py_NewRef(v);
}

Py_ssize_t
PyObject_Size(PyObject *o)
{
    if (o == NULL) {
        null_error();
        return -1;
    }

    PySequenceMethods *m = Py_TYPE(o)->tp_as_sequence;
    if (m && m->sq_length) {
        Py_ssize_t len = m->sq_length(o);
        assert(_Py_CheckSlotResult(o, "__len__", len >= 0));
        return len;
    }

    return PyMapping_Size(o);
}

#undef PyObject_Length
Py_ssize_t
PyObject_Length(PyObject *o)
{
    return PyObject_Size(o);
}
#define PyObject_Length PyObject_Size

int
_PyObject_HasLen(PyObject *o) {
    return (Py_TYPE(o)->tp_as_sequence && Py_TYPE(o)->tp_as_sequence->sq_length) ||
        (Py_TYPE(o)->tp_as_mapping && Py_TYPE(o)->tp_as_mapping->mp_length);
}

/* The length hint function returns a non-negative value from o.__len__()
   or o.__length_hint__(). If those methods aren't found the defaultvalue is
   returned.  If one of the calls fails with an exception other than TypeError
   this function returns -1.
*/

Py_ssize_t
PyObject_LengthHint(PyObject *o, Py_ssize_t defaultvalue)
{
    PyObject *hint, *result;
    Py_ssize_t res;
    if (_PyObject_HasLen(o)) {
        res = PyObject_Length(o);
        if (res < 0) {
            PyThreadState *tstate = _PyThreadState_GET();
            assert(_PyErr_Occurred(tstate));
            if (!_PyErr_ExceptionMatches(tstate, PyExc_TypeError)) {
                return -1;
            }
            _PyErr_Clear(tstate);
        }
        else {
            return res;
        }
    }
    hint = _PyObject_LookupSpecial(o, &_Py_ID(__length_hint__));
    if (hint == NULL) {
        if (PyErr_Occurred()) {
            return -1;
        }
        return defaultvalue;
    }
    result = _PyObject_CallNoArgs(hint);
    Py_DECREF(hint);
    if (result == NULL) {
        PyThreadState *tstate = _PyThreadState_GET();
        if (_PyErr_ExceptionMatches(tstate, PyExc_TypeError)) {
            _PyErr_Clear(tstate);
            return defaultvalue;
        }
        return -1;
    }
    else if (result == Py_NotImplemented) {
        Py_DECREF(result);
        return defaultvalue;
    }
    if (!PyLong_Check(result)) {
        PyErr_Format(PyExc_TypeError, "__length_hint__ must be an integer, not %.100s",
            Py_TYPE(result)->tp_name);
        Py_DECREF(result);
        return -1;
    }
    res = PyLong_AsSsize_t(result);
    Py_DECREF(result);
    if (res < 0 && PyErr_Occurred()) {
        return -1;
    }
    if (res < 0) {
        PyErr_Format(PyExc_ValueError, "__length_hint__() should return >= 0");
        return -1;
    }
    return res;
}

PyObject *
PyObject_GetItem(PyObject *o, PyObject *key)
{
    if (o == NULL || key == NULL) {
        return null_error();
    }

    PyMappingMethods *m = Py_TYPE(o)->tp_as_mapping;
    if (m && m->mp_subscript) {
        PyObject *item = m->mp_subscript(o, key);
        assert(_Py_CheckSlotResult(o, "__getitem__", item != NULL));
        return item;
    }

    PySequenceMethods *ms = Py_TYPE(o)->tp_as_sequence;
    if (ms && ms->sq_item) {
        if (_PyIndex_Check(key)) {
            Py_ssize_t key_value;
            key_value = PyNumber_AsSsize_t(key, PyExc_IndexError);
            if (key_value == -1 && PyErr_Occurred())
                return NULL;
            return PySequence_GetItem(o, key_value);
        }
        else {
            return type_error("sequence index must "
                              "be integer, not '%.200s'", key);
        }
    }

    if (PyType_Check(o)) {
        PyObject *meth, *result;

        // Special case type[int], but disallow other types so str[int] fails
        if ((PyTypeObject*)o == &PyType_Type) {
            return Py_GenericAlias(o, key);
        }

        if (PyObject_GetOptionalAttr(o, &_Py_ID(__class_getitem__), &meth) < 0) {
            return NULL;
        }
        if (meth && meth != Py_None) {
            result = PyObject_CallOneArg(meth, key);
            Py_DECREF(meth);
            return result;
        }
        Py_XDECREF(meth);
        PyErr_Format(PyExc_TypeError, "type '%.200s' is not subscriptable",
                     ((PyTypeObject *)o)->tp_name);
        return NULL;
    }

    return type_error("'%.200s' object is not subscriptable", o);
}

int
PyMapping_GetOptionalItem(PyObject *obj, PyObject *key, PyObject **result)
{
    if (PyDict_CheckExact(obj)) {
        return PyDict_GetItemRef(obj, key, result);
    }

    *result = PyObject_GetItem(obj, key);
    if (*result) {
        return 1;
    }
    assert(PyErr_Occurred());
    if (!PyErr_ExceptionMatches(PyExc_KeyError)) {
        return -1;
    }
    PyErr_Clear();
    return 0;
}

int
PyObject_SetItem(PyObject *o, PyObject *key, PyObject *value)
{
    if (o == NULL || key == NULL || value == NULL) {
        null_error();
        return -1;
    }

    PyMappingMethods *m = Py_TYPE(o)->tp_as_mapping;
    if (m && m->mp_ass_subscript) {
        int res = m->mp_ass_subscript(o, key, value);
        assert(_Py_CheckSlotResult(o, "__setitem__", res >= 0));
        return res;
    }

    if (Py_TYPE(o)->tp_as_sequence) {
        if (_PyIndex_Check(key)) {
            Py_ssize_t key_value;
            key_value = PyNumber_AsSsize_t(key, PyExc_IndexError);
            if (key_value == -1 && PyErr_Occurred())
                return -1;
            return PySequence_SetItem(o, key_value, value);
        }
        else if (Py_TYPE(o)->tp_as_sequence->sq_ass_item) {
            type_error("sequence index must be "
                       "integer, not '%.200s'", key);
            return -1;
        }
    }

    type_error("'%.200s' object does not support item assignment", o);
    return -1;
}

int
PyObject_DelItem(PyObject *o, PyObject *key)
{
    if (o == NULL || key == NULL) {
        null_error();
        return -1;
    }

    PyMappingMethods *m = Py_TYPE(o)->tp_as_mapping;
    if (m && m->mp_ass_subscript) {
        int res = m->mp_ass_subscript(o, key, (PyObject*)NULL);
        assert(_Py_CheckSlotResult(o, "__delitem__", res >= 0));
        return res;
    }

    if (Py_TYPE(o)->tp_as_sequence) {
        if (_PyIndex_Check(key)) {
            Py_ssize_t key_value;
            key_value = PyNumber_AsSsize_t(key, PyExc_IndexError);
            if (key_value == -1 && PyErr_Occurred())
                return -1;
            return PySequence_DelItem(o, key_value);
        }
        else if (Py_TYPE(o)->tp_as_sequence->sq_ass_item) {
            type_error("sequence index must be "
                       "integer, not '%.200s'", key);
            return -1;
        }
    }

    type_error("'%.200s' object does not support item deletion", o);
    return -1;
}

int
PyObject_DelItemString(PyObject *o, const char *key)
{
    PyObject *okey;
    int ret;

    if (o == NULL || key == NULL) {
        null_error();
        return -1;
    }
    okey = PyUnicode_FromString(key);
    if (okey == NULL)
        return -1;
    ret = PyObject_DelItem(o, okey);
    Py_DECREF(okey);
    return ret;
}


/* Return 1 if the getbuffer function is available, otherwise return 0. */
int
PyObject_CheckBuffer(PyObject *obj)
{
    PyBufferProcs *tp_as_buffer = Py_TYPE(obj)->tp_as_buffer;
    return (tp_as_buffer != NULL && tp_as_buffer->bf_getbuffer != NULL);
}

// Old buffer protocols (deprecated, abi only)

/* Checks whether an arbitrary object supports the (character, single segment)
   buffer interface.

   Returns 1 on success, 0 on failure.

   We release the buffer right after use of this function which could
   cause issues later on.  Don't use these functions in new code.
 */
PyAPI_FUNC(int) /* abi_only */
PyObject_CheckReadBuffer(PyObject *obj)
{
    PyBufferProcs *pb = Py_TYPE(obj)->tp_as_buffer;
    Py_buffer view;

    if (pb == NULL ||
        pb->bf_getbuffer == NULL)
        return 0;
    if ((*pb->bf_getbuffer)(obj, &view, PyBUF_SIMPLE) == -1) {
        PyErr_Clear();
        return 0;
    }
    PyBuffer_Release(&view);
    return 1;
}

static int
as_read_buffer(PyObject *obj, const void **buffer, Py_ssize_t *buffer_len)
{
    Py_buffer view;

    if (obj == NULL || buffer == NULL || buffer_len == NULL) {
        null_error();
        return -1;
    }
    if (PyObject_GetBuffer(obj, &view, PyBUF_SIMPLE) != 0)
        return -1;

    *buffer = view.buf;
    *buffer_len = view.len;
    PyBuffer_Release(&view);
    return 0;
}

/* Takes an arbitrary object which must support the (character, single segment)
   buffer interface and returns a pointer to a read-only memory location
   usable as character based input for subsequent processing.

   Return 0 on success.  buffer and buffer_len are only set in case no error
   occurs. Otherwise, -1 is returned and an exception set. */
PyAPI_FUNC(int) /* abi_only */
PyObject_AsCharBuffer(PyObject *obj,
                      const char **buffer,
                      Py_ssize_t *buffer_len)
{
    return as_read_buffer(obj, (const void **)buffer, buffer_len);
}

/* Same as PyObject_AsCharBuffer() except that this API expects (readable,
   single segment) buffer interface and returns a pointer to a read-only memory
   location which can contain arbitrary data.

   0 is returned on success.  buffer and buffer_len are only set in case no
   error occurs.  Otherwise, -1 is returned and an exception set. */
PyAPI_FUNC(int) /* abi_only */
PyObject_AsReadBuffer(PyObject *obj,
                      const void **buffer,
                      Py_ssize_t *buffer_len)
{
    return as_read_buffer(obj, buffer, buffer_len);
}

/* Takes an arbitrary object which must support the (writable, single segment)
   buffer interface and returns a pointer to a writable memory location in
   buffer of size 'buffer_len'.

   Return 0 on success.  buffer and buffer_len are only set in case no error
   occurs. Otherwise, -1 is returned and an exception set. */
PyAPI_FUNC(int) /* abi_only */
PyObject_AsWriteBuffer(PyObject *obj,
                       void **buffer,
                       Py_ssize_t *buffer_len)
{
    PyBufferProcs *pb;
    Py_buffer view;

    if (obj == NULL || buffer == NULL || buffer_len == NULL) {
        null_error();
        return -1;
    }
    pb = Py_TYPE(obj)->tp_as_buffer;
    if (pb == NULL ||
        pb->bf_getbuffer == NULL ||
        ((*pb->bf_getbuffer)(obj, &view, PyBUF_WRITABLE) != 0)) {
        PyErr_SetString(PyExc_TypeError,
                        "expected a writable bytes-like object");
        return -1;
    }

    *buffer = view.buf;
    *buffer_len = view.len;
    PyBuffer_Release(&view);
    return 0;
}

/* Buffer C-API for Python 3.0 */

int
PyObject_GetBuffer(PyObject *obj, Py_buffer *view, int flags)
{
    if (flags != PyBUF_SIMPLE) {  /* fast path */
        if (flags == PyBUF_READ || flags == PyBUF_WRITE) {
            PyErr_BadInternalCall();
            return -1;
        }
    }
    PyBufferProcs *pb = Py_TYPE(obj)->tp_as_buffer;

    if (pb == NULL || pb->bf_getbuffer == NULL) {
        PyErr_Format(PyExc_TypeError,
                     "a bytes-like object is required, not '%.100s'",
                     Py_TYPE(obj)->tp_name);
        return -1;
    }
    int res = (*pb->bf_getbuffer)(obj, view, flags);
    assert(_Py_CheckSlotResult(obj, "getbuffer", res >= 0));
    return res;
}

static int
_IsFortranContiguous(const Py_buffer *view)
{
    Py_ssize_t sd, dim;
    int i;

    /* 1) len = product(shape) * itemsize
       2) itemsize > 0
       3) len = 0 <==> exists i: shape[i] = 0 */
    if (view->len == 0) return 1;
    if (view->strides == NULL) {  /* C-contiguous by definition */
        /* Trivially F-contiguous */
        if (view->ndim <= 1) return 1;

        /* ndim > 1 implies shape != NULL */
        assert(view->shape != NULL);

        /* Effectively 1-d */
        sd = 0;
        for (i=0; i<view->ndim; i++) {
            if (view->shape[i] > 1) sd += 1;
        }
        return sd <= 1;
    }

    /* strides != NULL implies both of these */
    assert(view->ndim > 0);
    assert(view->shape != NULL);

    sd = view->itemsize;
    for (i=0; i<view->ndim; i++) {
        dim = view->shape[i];
        if (dim > 1 && view->strides[i] != sd) {
            return 0;
        }
        sd *= dim;
    }
    return 1;
}

static int
_IsCContiguous(const Py_buffer *view)
{
    Py_ssize_t sd, dim;
    int i;

    /* 1) len = product(shape) * itemsize
       2) itemsize > 0
       3) len = 0 <==> exists i: shape[i] = 0 */
    if (view->len == 0) return 1;
    if (view->strides == NULL) return 1; /* C-contiguous by definition */

    /* strides != NULL implies both of these */
    assert(view->ndim > 0);
    assert(view->shape != NULL);

    sd = view->itemsize;
    for (i=view->ndim-1; i>=0; i--) {
        dim = view->shape[i];
        if (dim > 1 && view->strides[i] != sd) {
            return 0;
        }
        sd *= dim;
    }
    return 1;
}

int
PyBuffer_IsContiguous(const Py_buffer *view, char order)
{

    if (view->suboffsets != NULL) return 0;

    if (order == 'C')
        return _IsCContiguous(view);
    else if (order == 'F')
        return _IsFortranContiguous(view);
    else if (order == 'A')
        return (_IsCContiguous(view) || _IsFortranContiguous(view));
    return 0;
}


void*
PyBuffer_GetPointer(const Py_buffer *view, const Py_ssize_t *indices)
{
    char* pointer;
    int i;
    pointer = (char *)view->buf;
    for (i = 0; i < view->ndim; i++) {
        pointer += view->strides[i]*indices[i];
        if ((view->suboffsets != NULL) && (view->suboffsets[i] >= 0)) {
            pointer = *((char**)pointer) + view->suboffsets[i];
        }
    }
    return (void*)pointer;
}


static void
_Py_add_one_to_index_F(int nd, Py_ssize_t *index, const Py_ssize_t *shape)
{
    int k;

    for (k=0; k<nd; k++) {
        if (index[k] < shape[k]-1) {
            index[k]++;
            break;
        }
        else {
            index[k] = 0;
        }
    }
}

static void
_Py_add_one_to_index_C(int nd, Py_ssize_t *index, const Py_ssize_t *shape)
{
    int k;

    for (k=nd-1; k>=0; k--) {
        if (index[k] < shape[k]-1) {
            index[k]++;
            break;
        }
        else {
            index[k] = 0;
        }
    }
}

Py_ssize_t
PyBuffer_SizeFromFormat(const char *format)
{
    PyObject *calcsize = NULL;
    PyObject *res = NULL;
    PyObject *fmt = NULL;
    Py_ssize_t itemsize = -1;

    calcsize = PyImport_ImportModuleAttrString("struct", "calcsize");
    if (calcsize == NULL) {
        goto done;
    }

    fmt = PyUnicode_FromString(format);
    if (fmt == NULL) {
        goto done;
    }

    res = PyObject_CallFunctionObjArgs(calcsize, fmt, NULL);
    if (res == NULL) {
        goto done;
    }

    itemsize = PyLong_AsSsize_t(res);
    if (itemsize < 0) {
        goto done;
    }

done:
    Py_XDECREF(calcsize);
    Py_XDECREF(fmt);
    Py_XDECREF(res);
    return itemsize;
}

int
PyBuffer_FromContiguous(const Py_buffer *view, const void *buf, Py_ssize_t len, char fort)
{
    int k;
    void (*addone)(int, Py_ssize_t *, const Py_ssize_t *);
    Py_ssize_t *indices, elements;
    char *ptr;
    const char *src;

    if (len > view->len) {
        len = view->len;
    }

    if (PyBuffer_IsContiguous(view, fort)) {
        /* simplest copy is all that is needed */
        memcpy(view->buf, buf, len);
        return 0;
    }

    /* Otherwise a more elaborate scheme is needed */

    /* view->ndim <= 64 */
    indices = (Py_ssize_t *)PyMem_Malloc(sizeof(Py_ssize_t)*(view->ndim));
    if (indices == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    for (k=0; k<view->ndim;k++) {
        indices[k] = 0;
    }

    if (fort == 'F') {
        addone = _Py_add_one_to_index_F;
    }
    else {
        addone = _Py_add_one_to_index_C;
    }
    src = buf;
    /* XXX : This is not going to be the fastest code in the world
             several optimizations are possible.
     */
    elements = len / view->itemsize;
    while (elements--) {
        ptr = PyBuffer_GetPointer(view, indices);
        memcpy(ptr, src, view->itemsize);
        src += view->itemsize;
        addone(view->ndim, indices, view->shape);
    }

    PyMem_Free(indices);
    return 0;
}

int PyObject_CopyData(PyObject *dest, PyObject *src)
{
    Py_buffer view_dest, view_src;
    int k;
    Py_ssize_t *indices, elements;
    char *dptr, *sptr;

    if (!PyObject_CheckBuffer(dest) ||
        !PyObject_CheckBuffer(src)) {
        PyErr_SetString(PyExc_TypeError,
                        "both destination and source must be "\
                        "bytes-like objects");
        return -1;
    }

    if (PyObject_GetBuffer(dest, &view_dest, PyBUF_FULL) != 0) return -1;
    if (PyObject_GetBuffer(src, &view_src, PyBUF_FULL_RO) != 0) {
        PyBuffer_Release(&view_dest);
        return -1;
    }

    if (view_dest.len < view_src.len) {
        PyErr_SetString(PyExc_BufferError,
                        "destination is too small to receive data from source");
        PyBuffer_Release(&view_dest);
        PyBuffer_Release(&view_src);
        return -1;
    }

    if ((PyBuffer_IsContiguous(&view_dest, 'C') &&
         PyBuffer_IsContiguous(&view_src, 'C')) ||
        (PyBuffer_IsContiguous(&view_dest, 'F') &&
         PyBuffer_IsContiguous(&view_src, 'F'))) {
        /* simplest copy is all that is needed */
        memcpy(view_dest.buf, view_src.buf, view_src.len);
        PyBuffer_Release(&view_dest);
        PyBuffer_Release(&view_src);
        return 0;
    }

    /* Otherwise a more elaborate copy scheme is needed */

    /* XXX(nnorwitz): need to check for overflow! */
    indices = (Py_ssize_t *)PyMem_Malloc(sizeof(Py_ssize_t)*view_src.ndim);
    if (indices == NULL) {
        PyErr_NoMemory();
        PyBuffer_Release(&view_dest);
        PyBuffer_Release(&view_src);
        return -1;
    }
    for (k=0; k<view_src.ndim;k++) {
        indices[k] = 0;
    }
    elements = 1;
    for (k=0; k<view_src.ndim; k++) {
        /* XXX(nnorwitz): can this overflow? */
        elements *= view_src.shape[k];
    }
    while (elements--) {
        _Py_add_one_to_index_C(view_src.ndim, indices, view_src.shape);
        dptr = PyBuffer_GetPointer(&view_dest, indices);
        sptr = PyBuffer_GetPointer(&view_src, indices);
        memcpy(dptr, sptr, view_src.itemsize);
    }
    PyMem_Free(indices);
    PyBuffer_Release(&view_dest);
    PyBuffer_Release(&view_src);
    return 0;
}

void
PyBuffer_FillContiguousStrides(int nd, Py_ssize_t *shape,
                               Py_ssize_t *strides, int itemsize,
                               char fort)
{
    int k;
    Py_ssize_t sd;

    sd = itemsize;
    if (fort == 'F') {
        for (k=0; k<nd; k++) {
            strides[k] = sd;
            sd *= shape[k];
        }
    }
    else {
        for (k=nd-1; k>=0; k--) {
            strides[k] = sd;
            sd *= shape[k];
        }
    }
    return;
}

int
PyBuffer_FillInfo(Py_buffer *view, PyObject *obj, void *buf, Py_ssize_t len,
                  int readonly, int flags)
{
    if (view == NULL) {
        PyErr_SetString(PyExc_BufferError,
                        "PyBuffer_FillInfo: view==NULL argument is obsolete");
        return -1;
    }

    if (flags != PyBUF_SIMPLE) {  /* fast path */
        if (flags == PyBUF_READ || flags == PyBUF_WRITE) {
            PyErr_BadInternalCall();
            return -1;
        }
        if (((flags & PyBUF_WRITABLE) == PyBUF_WRITABLE) &&
            (readonly == 1)) {
            PyErr_SetString(PyExc_BufferError,
                            "Object is not writable.");
            return -1;
        }
    }

    view->obj = Py_XNewRef(obj);
    view->buf = buf;
    view->len = len;
    view->readonly = readonly;
    view->itemsize = 1;
    view->format = NULL;
    if ((flags & PyBUF_FORMAT) == PyBUF_FORMAT)
        view->format = "B";
    view->ndim = 1;
    view->shape = NULL;
    if ((flags & PyBUF_ND) == PyBUF_ND)
        view->shape = &(view->len);
    view->strides = NULL;
    if ((flags & PyBUF_STRIDES) == PyBUF_STRIDES)
        view->strides = &(view->itemsize);
    view->suboffsets = NULL;
    view->internal = NULL;
    return 0;
}

void
PyBuffer_Release(Py_buffer *view)
{
    PyObject *obj = view->obj;
    PyBufferProcs *pb;
    if (obj == NULL)
        return;
    pb = Py_TYPE(obj)->tp_as_buffer;
    if (pb && pb->bf_releasebuffer) {
        pb->bf_releasebuffer(obj, view);
    }
    view->obj = NULL;
    Py_DECREF(obj);
}

static int
_buffer_release_call(void *arg)
{
    PyBuffer_Release((Py_buffer *)arg);
    return 0;
}

int
_PyBuffer_ReleaseInInterpreter(PyInterpreterState *interp,
                               Py_buffer *view)
{
    return _Py_CallInInterpreter(interp, _buffer_release_call, view);
}

int
_PyBuffer_ReleaseInInterpreterAndRawFree(PyInterpreterState *interp,
                                         Py_buffer *view)
{
    return _Py_CallInInterpreterAndRawFree(interp, _buffer_release_call, view);
}

PyObject *
PyObject_Format(PyObject *obj, PyObject *format_spec)
{
    PyObject *meth;
    PyObject *empty = NULL;
    PyObject *result = NULL;

    if (format_spec != NULL && !PyUnicode_Check(format_spec)) {
        PyErr_Format(PyExc_SystemError,
                     "Format specifier must be a string, not %.200s",
                     Py_TYPE(format_spec)->tp_name);
        return NULL;
    }

    /* Fast path for common types. */
    if (format_spec == NULL || PyUnicode_GET_LENGTH(format_spec) == 0) {
        if (PyUnicode_CheckExact(obj)) {
            return Py_NewRef(obj);
        }
        if (PyLong_CheckExact(obj)) {
            return PyObject_Str(obj);
        }
    }

    /* If no format_spec is provided, use an empty string */
    if (format_spec == NULL) {
        empty = Py_GetConstant(Py_CONSTANT_EMPTY_STR);
        format_spec = empty;
    }

    /* Find the (unbound!) __format__ method */
    meth = _PyObject_LookupSpecial(obj, &_Py_ID(__format__));
    if (meth == NULL) {
        PyThreadState *tstate = _PyThreadState_GET();
        if (!_PyErr_Occurred(tstate)) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "Type %.100s doesn't define __format__",
                          Py_TYPE(obj)->tp_name);
        }
        goto done;
    }

    /* And call it. */
    result = PyObject_CallOneArg(meth, format_spec);
    Py_DECREF(meth);

    if (result && !PyUnicode_Check(result)) {
        PyErr_Format(PyExc_TypeError,
                     "__format__ must return a str, not %.200s",
                     Py_TYPE(result)->tp_name);
        Py_SETREF(result, NULL);
        goto done;
    }

done:
    Py_XDECREF(empty);
    return result;
}
/* Operations on numbers */

int
PyNumber_Check(PyObject *o)
{
    if (o == NULL)
        return 0;
    PyNumberMethods *nb = Py_TYPE(o)->tp_as_number;
    return nb && (nb->nb_index || nb->nb_int || nb->nb_float || PyComplex_Check(o));
}

/* Binary operators */

#define NB_SLOT(x) offsetof(PyNumberMethods, x)
#define NB_BINOP(nb_methods, slot) \
        (*(binaryfunc*)(& ((char*)nb_methods)[slot]))
#define NB_TERNOP(nb_methods, slot) \
        (*(ternaryfunc*)(& ((char*)nb_methods)[slot]))

/*
  Calling scheme used for binary operations:

  Order operations are tried until either a valid result or error:
    w.op(v,w)[*], v.op(v,w), w.op(v,w)

  [*] only when Py_TYPE(v) != Py_TYPE(w) && Py_TYPE(w) is a subclass of
      Py_TYPE(v)
 */

static PyObject *
binary_op1(PyObject *v, PyObject *w, const int op_slot
#ifndef NDEBUG
           , const char *op_name
#endif
           )
{
    binaryfunc slotv;
    if (Py_TYPE(v)->tp_as_number != NULL) {
        slotv = NB_BINOP(Py_TYPE(v)->tp_as_number, op_slot);
    }
    else {
        slotv = NULL;
    }

    binaryfunc slotw;
    if (!Py_IS_TYPE(w, Py_TYPE(v)) && Py_TYPE(w)->tp_as_number != NULL) {
        slotw = NB_BINOP(Py_TYPE(w)->tp_as_number, op_slot);
        if (slotw == slotv) {
            slotw = NULL;
        }
    }
    else {
        slotw = NULL;
    }

    if (slotv) {
        PyObject *x;
        if (slotw && PyType_IsSubtype(Py_TYPE(w), Py_TYPE(v))) {
            x = slotw(v, w);
            if (x != Py_NotImplemented)
                return x;
            Py_DECREF(x); /* can't do it */
            slotw = NULL;
        }
        x = slotv(v, w);
        assert(_Py_CheckSlotResult(v, op_name, x != NULL));
        if (x != Py_NotImplemented) {
            return x;
        }
        Py_DECREF(x); /* can't do it */
    }
    if (slotw) {
        PyObject *x = slotw(v, w);
        assert(_Py_CheckSlotResult(w, op_name, x != NULL));
        if (x != Py_NotImplemented) {
            return x;
        }
        Py_DECREF(x); /* can't do it */
    }
    Py_RETURN_NOTIMPLEMENTED;
}

#ifdef NDEBUG
#  define BINARY_OP1(v, w, op_slot, op_name) binary_op1(v, w, op_slot)
#else
#  define BINARY_OP1(v, w, op_slot, op_name) binary_op1(v, w, op_slot, op_name)
#endif

static PyObject *
binop_type_error(PyObject *v, PyObject *w, const char *op_name)
{
    PyErr_Format(PyExc_TypeError,
                 "unsupported operand type(s) for %.100s: "
                 "'%.100s' and '%.100s'",
                 op_name,
                 Py_TYPE(v)->tp_name,
                 Py_TYPE(w)->tp_name);
    return NULL;
}

static PyObject *
binary_op(PyObject *v, PyObject *w, const int op_slot, const char *op_name)
{
    PyObject *result = BINARY_OP1(v, w, op_slot, op_name);
    if (result == Py_NotImplemented) {
        Py_DECREF(result);
        return binop_type_error(v, w, op_name);
    }
    return result;
}


/*
  Calling scheme used for ternary operations:

  Order operations are tried until either a valid result or error:
    v.op(v,w,z), w.op(v,w,z), z.op(v,w,z)
 */

static PyObject *
ternary_op(PyObject *v,
           PyObject *w,
           PyObject *z,
           const int op_slot,
           const char *op_name
           )
{
    PyNumberMethods *mv = Py_TYPE(v)->tp_as_number;
    PyNumberMethods *mw = Py_TYPE(w)->tp_as_number;

    ternaryfunc slotv;
    if (mv != NULL) {
        slotv = NB_TERNOP(mv, op_slot);
    }
    else {
        slotv = NULL;
    }

    ternaryfunc slotw;
    if (!Py_IS_TYPE(w, Py_TYPE(v)) && mw != NULL) {
        slotw = NB_TERNOP(mw, op_slot);
        if (slotw == slotv) {
            slotw = NULL;
        }
    }
    else {
        slotw = NULL;
    }

    if (slotv) {
        PyObject *x;
        if (slotw && PyType_IsSubtype(Py_TYPE(w), Py_TYPE(v))) {
            x = slotw(v, w, z);
            if (x != Py_NotImplemented) {
                return x;
            }
            Py_DECREF(x); /* can't do it */
            slotw = NULL;
        }
        x = slotv(v, w, z);
        assert(_Py_CheckSlotResult(v, op_name, x != NULL));
        if (x != Py_NotImplemented) {
            return x;
        }
        Py_DECREF(x); /* can't do it */
    }
    if (slotw) {
        PyObject *x = slotw(v, w, z);
        assert(_Py_CheckSlotResult(w, op_name, x != NULL));
        if (x != Py_NotImplemented) {
            return x;
        }
        Py_DECREF(x); /* can't do it */
    }

    PyNumberMethods *mz = Py_TYPE(z)->tp_as_number;
    if (mz != NULL) {
        ternaryfunc slotz = NB_TERNOP(mz, op_slot);
        if (slotz == slotv || slotz == slotw) {
            slotz = NULL;
        }
        if (slotz) {
            PyObject *x = slotz(v, w, z);
            assert(_Py_CheckSlotResult(z, op_name, x != NULL));
            if (x != Py_NotImplemented) {
                return x;
            }
            Py_DECREF(x); /* can't do it */
        }
    }

    if (z == Py_None) {
        PyErr_Format(
            PyExc_TypeError,
            "unsupported operand type(s) for %.100s: "
            "'%.100s' and '%.100s'",
            op_name,
            Py_TYPE(v)->tp_name,
            Py_TYPE(w)->tp_name);
    }
    else {
        PyErr_Format(
            PyExc_TypeError,
            "unsupported operand type(s) for %.100s: "
            "'%.100s', '%.100s', '%.100s'",
            op_name,
            Py_TYPE(v)->tp_name,
            Py_TYPE(w)->tp_name,
            Py_TYPE(z)->tp_name);
    }
    return NULL;
}

#define BINARY_FUNC(func, op, op_name) \
    PyObject * \
    func(PyObject *v, PyObject *w) { \
        return binary_op(v, w, NB_SLOT(op), op_name); \
    }

BINARY_FUNC(PyNumber_Or, nb_or, "|")
BINARY_FUNC(PyNumber_Xor, nb_xor, "^")
BINARY_FUNC(PyNumber_And, nb_and, "&")
BINARY_FUNC(PyNumber_Lshift, nb_lshift, "<<")
BINARY_FUNC(PyNumber_Rshift, nb_rshift, ">>")
BINARY_FUNC(PyNumber_Subtract, nb_subtract, "-")
BINARY_FUNC(PyNumber_Divmod, nb_divmod, "divmod()")

PyObject *
PyNumber_Add(PyObject *v, PyObject *w)
{
    PyObject *result = BINARY_OP1(v, w, NB_SLOT(nb_add), "+");
    if (result != Py_NotImplemented) {
        return result;
    }
    Py_DECREF(result);

    PySequenceMethods *m = Py_TYPE(v)->tp_as_sequence;
    if (m && m->sq_concat) {
        result = (*m->sq_concat)(v, w);
        assert(_Py_CheckSlotResult(v, "+", result != NULL));
        return result;
    }

    return binop_type_error(v, w, "+");
}

static PyObject *
sequence_repeat(ssizeargfunc repeatfunc, PyObject *seq, PyObject *n)
{
    Py_ssize_t count;
    if (_PyIndex_Check(n)) {
        count = PyNumber_AsSsize_t(n, PyExc_OverflowError);
        if (count == -1 && PyErr_Occurred()) {
            return NULL;
        }
    }
    else {
        return type_error("can't multiply sequence by "
                          "non-int of type '%.200s'", n);
    }
    PyObject *res = (*repeatfunc)(seq, count);
    assert(_Py_CheckSlotResult(seq, "*", res != NULL));
    return res;
}

PyObject *
PyNumber_Multiply(PyObject *v, PyObject *w)
{
    PyObject *result = BINARY_OP1(v, w, NB_SLOT(nb_multiply), "*");
    if (result == Py_NotImplemented) {
        PySequenceMethods *mv = Py_TYPE(v)->tp_as_sequence;
        PySequenceMethods *mw = Py_TYPE(w)->tp_as_sequence;
        Py_DECREF(result);
        if  (mv && mv->sq_repeat) {
            return sequence_repeat(mv->sq_repeat, v, w);
        }
        else if (mw && mw->sq_repeat) {
            return sequence_repeat(mw->sq_repeat, w, v);
        }
        result = binop_type_error(v, w, "*");
    }
    return result;
}

BINARY_FUNC(PyNumber_MatrixMultiply, nb_matrix_multiply, "@")
BINARY_FUNC(PyNumber_FloorDivide, nb_floor_divide, "//")
BINARY_FUNC(PyNumber_TrueDivide, nb_true_divide, "/")
BINARY_FUNC(PyNumber_Remainder, nb_remainder, "%")

PyObject *
PyNumber_Power(PyObject *v, PyObject *w, PyObject *z)
{
    return ternary_op(v, w, z, NB_SLOT(nb_power), "** or pow()");
}

PyObject *
_PyNumber_PowerNoMod(PyObject *lhs, PyObject *rhs)
{
    return PyNumber_Power(lhs, rhs, Py_None);
}

/* Binary in-place operators */

/* The in-place operators are defined to fall back to the 'normal',
   non in-place operations, if the in-place methods are not in place.

   - If the left hand object has the appropriate struct members, and
     they are filled, call the appropriate function and return the
     result.  No coercion is done on the arguments; the left-hand object
     is the one the operation is performed on, and it's up to the
     function to deal with the right-hand object.

   - Otherwise, in-place modification is not supported. Handle it exactly as
     a non in-place operation of the same kind.

   */

static PyObject *
binary_iop1(PyObject *v, PyObject *w, const int iop_slot, const int op_slot
#ifndef NDEBUG
            , const char *op_name
#endif
            )
{
    PyNumberMethods *mv = Py_TYPE(v)->tp_as_number;
    if (mv != NULL) {
        binaryfunc slot = NB_BINOP(mv, iop_slot);
        if (slot) {
            PyObject *x = (slot)(v, w);
            assert(_Py_CheckSlotResult(v, op_name, x != NULL));
            if (x != Py_NotImplemented) {
                return x;
            }
            Py_DECREF(x);
        }
    }
#ifdef NDEBUG
    return binary_op1(v, w, op_slot);
#else
    return binary_op1(v, w, op_slot, op_name);
#endif
}

#ifdef NDEBUG
#  define BINARY_IOP1(v, w, iop_slot, op_slot, op_name) binary_iop1(v, w, iop_slot, op_slot)
#else
#  define BINARY_IOP1(v, w, iop_slot, op_slot, op_name) binary_iop1(v, w, iop_slot, op_slot, op_name)
#endif

static PyObject *
binary_iop(PyObject *v, PyObject *w, const int iop_slot, const int op_slot,
                const char *op_name)
{
    PyObject *result = BINARY_IOP1(v, w, iop_slot, op_slot, op_name);
    if (result == Py_NotImplemented) {
        Py_DECREF(result);
        return binop_type_error(v, w, op_name);
    }
    return result;
}

static PyObject *
ternary_iop(PyObject *v, PyObject *w, PyObject *z, const int iop_slot, const int op_slot,
                const char *op_name)
{
    PyNumberMethods *mv = Py_TYPE(v)->tp_as_number;
    if (mv != NULL) {
        ternaryfunc slot = NB_TERNOP(mv, iop_slot);
        if (slot) {
            PyObject *x = (slot)(v, w, z);
            if (x != Py_NotImplemented) {
                return x;
            }
            Py_DECREF(x);
        }
    }
    return ternary_op(v, w, z, op_slot, op_name);
}

#define INPLACE_BINOP(func, iop, op, op_name) \
    PyObject * \
    func(PyObject *v, PyObject *w) { \
        return binary_iop(v, w, NB_SLOT(iop), NB_SLOT(op), op_name); \
    }

INPLACE_BINOP(PyNumber_InPlaceOr, nb_inplace_or, nb_or, "|=")
INPLACE_BINOP(PyNumber_InPlaceXor, nb_inplace_xor, nb_xor, "^=")
INPLACE_BINOP(PyNumber_InPlaceAnd, nb_inplace_and, nb_and, "&=")
INPLACE_BINOP(PyNumber_InPlaceLshift, nb_inplace_lshift, nb_lshift, "<<=")
INPLACE_BINOP(PyNumber_InPlaceRshift, nb_inplace_rshift, nb_rshift, ">>=")
INPLACE_BINOP(PyNumber_InPlaceSubtract, nb_inplace_subtract, nb_subtract, "-=")
INPLACE_BINOP(PyNumber_InPlaceMatrixMultiply, nb_inplace_matrix_multiply, nb_matrix_multiply, "@=")
INPLACE_BINOP(PyNumber_InPlaceFloorDivide, nb_inplace_floor_divide, nb_floor_divide, "//=")
INPLACE_BINOP(PyNumber_InPlaceTrueDivide, nb_inplace_true_divide, nb_true_divide,  "/=")
INPLACE_BINOP(PyNumber_InPlaceRemainder, nb_inplace_remainder, nb_remainder, "%=")

PyObject *
PyNumber_InPlaceAdd(PyObject *v, PyObject *w)
{
    PyObject *result = BINARY_IOP1(v, w, NB_SLOT(nb_inplace_add),
                                   NB_SLOT(nb_add), "+=");
    if (result == Py_NotImplemented) {
        PySequenceMethods *m = Py_TYPE(v)->tp_as_sequence;
        Py_DECREF(result);
        if (m != NULL) {
            binaryfunc func = m->sq_inplace_concat;
            if (func == NULL)
                func = m->sq_concat;
            if (func != NULL) {
                result = func(v, w);
                assert(_Py_CheckSlotResult(v, "+=", result != NULL));
                return result;
            }
        }
        result = binop_type_error(v, w, "+=");
    }
    return result;
}

PyObject *
PyNumber_InPlaceMultiply(PyObject *v, PyObject *w)
{
    PyObject *result = BINARY_IOP1(v, w, NB_SLOT(nb_inplace_multiply),
                                   NB_SLOT(nb_multiply), "*=");
    if (result == Py_NotImplemented) {
        ssizeargfunc f = NULL;
        PySequenceMethods *mv = Py_TYPE(v)->tp_as_sequence;
        PySequenceMethods *mw = Py_TYPE(w)->tp_as_sequence;
        Py_DECREF(result);
        if (mv != NULL) {
            f = mv->sq_inplace_repeat;
            if (f == NULL)
                f = mv->sq_repeat;
            if (f != NULL)
                return sequence_repeat(f, v, w);
        }
        else if (mw != NULL) {
            /* Note that the right hand operand should not be
             * mutated in this case so sq_inplace_repeat is not
             * used. */
            if (mw->sq_repeat)
                return sequence_repeat(mw->sq_repeat, w, v);
        }
        result = binop_type_error(v, w, "*=");
    }
    return result;
}

PyObject *
PyNumber_InPlacePower(PyObject *v, PyObject *w, PyObject *z)
{
    return ternary_iop(v, w, z, NB_SLOT(nb_inplace_power),
                                NB_SLOT(nb_power), "**=");
}

PyObject *
_PyNumber_InPlacePowerNoMod(PyObject *lhs, PyObject *rhs)
{
    return PyNumber_InPlacePower(lhs, rhs, Py_None);
}


/* Unary operators and functions */

#define UNARY_FUNC(func, op, meth_name, descr)                           \
    PyObject *                                                           \
    func(PyObject *o) {                                                  \
        if (o == NULL) {                                                 \
            return null_error();                                         \
        }                                                                \
                                                                         \
        PyNumberMethods *m = Py_TYPE(o)->tp_as_number;                   \
        if (m && m->op) {                                                \
            PyObject *res = (*m->op)(o);                                 \
            assert(_Py_CheckSlotResult(o, #meth_name, res != NULL));     \
            return res;                                                  \
        }                                                                \
                                                                         \
        return type_error("bad operand type for "descr": '%.200s'", o);  \
    }

UNARY_FUNC(PyNumber_Negative, nb_negative, __neg__, "unary -")
UNARY_FUNC(PyNumber_Positive, nb_positive, __pos__, "unary +")
UNARY_FUNC(PyNumber_Invert, nb_invert, __invert__, "unary ~")
UNARY_FUNC(PyNumber_Absolute, nb_absolute, __abs__, "abs()")


int
PyIndex_Check(PyObject *obj)
{
    return _PyIndex_Check(obj);
}


/* Return a Python int from the object item.
   Can return an instance of int subclass.
   Raise TypeError if the result is not an int
   or if the object cannot be interpreted as an index.
*/
PyObject *
_PyNumber_Index(PyObject *item)
{
    if (item == NULL) {
        return null_error();
    }

    if (PyLong_Check(item)) {
        return Py_NewRef(item);
    }
    if (!_PyIndex_Check(item)) {
        PyErr_Format(PyExc_TypeError,
                     "'%.200s' object cannot be interpreted "
                     "as an integer", Py_TYPE(item)->tp_name);
        return NULL;
    }

    PyObject *result = Py_TYPE(item)->tp_as_number->nb_index(item);
    assert(_Py_CheckSlotResult(item, "__index__", result != NULL));
    if (!result || PyLong_CheckExact(result)) {
        return result;
    }

    if (!PyLong_Check(result)) {
        PyErr_Format(PyExc_TypeError,
                     "__index__ returned non-int (type %.200s)",
                     Py_TYPE(result)->tp_name);
        Py_DECREF(result);
        return NULL;
    }
    /* Issue #17576: warn if 'result' not of exact type int. */
    if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
            "__index__ returned non-int (type %.200s).  "
            "The ability to return an instance of a strict subclass of int "
            "is deprecated, and may be removed in a future version of Python.",
            Py_TYPE(result)->tp_name)) {
        Py_DECREF(result);
        return NULL;
    }
    return result;
}

/* Return an exact Python int from the object item.
   Raise TypeError if the result is not an int
   or if the object cannot be interpreted as an index.
*/
PyObject *
PyNumber_Index(PyObject *item)
{
    PyObject *result = _PyNumber_Index(item);
    if (result != NULL && !PyLong_CheckExact(result)) {
        Py_SETREF(result, _PyLong_Copy((PyLongObject *)result));
    }
    return result;
}

/* Return an error on Overflow only if err is not NULL*/

Py_ssize_t
PyNumber_AsSsize_t(PyObject *item, PyObject *err)
{
    Py_ssize_t result;
    PyObject *runerr;
    PyObject *value = _PyNumber_Index(item);
    if (value == NULL)
        return -1;

    /* We're done if PyLong_AsSsize_t() returns without error. */
    result = PyLong_AsSsize_t(value);
    if (result != -1)
        goto finish;

    PyThreadState *tstate = _PyThreadState_GET();
    runerr = _PyErr_Occurred(tstate);
    if (!runerr) {
        goto finish;
    }

    /* Error handling code -- only manage OverflowError differently */
    if (!PyErr_GivenExceptionMatches(runerr, PyExc_OverflowError)) {
        goto finish;
    }
    _PyErr_Clear(tstate);

    /* If no error-handling desired then the default clipping
       is sufficient. */
    if (!err) {
        assert(PyLong_Check(value));
        /* Whether or not it is less than or equal to
           zero is determined by the sign of ob_size
        */
        if (_PyLong_IsNegative((PyLongObject *)value))
            result = PY_SSIZE_T_MIN;
        else
            result = PY_SSIZE_T_MAX;
    }
    else {
        /* Otherwise replace the error with caller's error object. */
        _PyErr_Format(tstate, err,
                      "cannot fit '%.200s' into an index-sized integer",
                      Py_TYPE(item)->tp_name);
    }

 finish:
    Py_DECREF(value);
    return result;
}


PyObject *
PyNumber_Long(PyObject *o)
{
    PyObject *result;
    PyNumberMethods *m;
    Py_buffer view;

    if (o == NULL) {
        return null_error();
    }

    if (PyLong_CheckExact(o)) {
        return Py_NewRef(o);
    }
    m = Py_TYPE(o)->tp_as_number;
    if (m && m->nb_int) { /* This should include subclasses of int */
        /* Convert using the nb_int slot, which should return something
           of exact type int. */
        result = m->nb_int(o);
        assert(_Py_CheckSlotResult(o, "__int__", result != NULL));
        if (!result || PyLong_CheckExact(result)) {
            return result;
        }

        if (!PyLong_Check(result)) {
            PyErr_Format(PyExc_TypeError,
                         "__int__ returned non-int (type %.200s)",
                         Py_TYPE(result)->tp_name);
            Py_DECREF(result);
            return NULL;
        }
        /* Issue #17576: warn if 'result' not of exact type int. */
        if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
                "__int__ returned non-int (type %.200s).  "
                "The ability to return an instance of a strict subclass of int "
                "is deprecated, and may be removed in a future version of Python.",
                Py_TYPE(result)->tp_name)) {
            Py_DECREF(result);
            return NULL;
        }
        Py_SETREF(result, _PyLong_Copy((PyLongObject *)result));
        return result;
    }
    if (m && m->nb_index) {
        return PyNumber_Index(o);
    }

    if (PyUnicode_Check(o))
        /* The below check is done in PyLong_FromUnicodeObject(). */
        return PyLong_FromUnicodeObject(o, 10);

    if (PyBytes_Check(o))
        /* need to do extra error checking that PyLong_FromString()
         * doesn't do.  In particular int('9\x005') must raise an
         * exception, not truncate at the null.
         */
        return _PyLong_FromBytes(PyBytes_AS_STRING(o),
                                 PyBytes_GET_SIZE(o), 10);

    if (PyByteArray_Check(o))
        return _PyLong_FromBytes(PyByteArray_AS_STRING(o),
                                 PyByteArray_GET_SIZE(o), 10);

    if (PyObject_GetBuffer(o, &view, PyBUF_SIMPLE) == 0) {
        PyObject *bytes;

        /* Copy to NUL-terminated buffer. */
        bytes = PyBytes_FromStringAndSize((const char *)view.buf, view.len);
        if (bytes == NULL) {
            PyBuffer_Release(&view);
            return NULL;
        }
        result = _PyLong_FromBytes(PyBytes_AS_STRING(bytes),
                                   PyBytes_GET_SIZE(bytes), 10);
        Py_DECREF(bytes);
        PyBuffer_Release(&view);
        return result;
    }

    return type_error("int() argument must be a string, a bytes-like object "
                      "or a real number, not '%.200s'", o);
}

PyObject *
PyNumber_Float(PyObject *o)
{
    if (o == NULL) {
        return null_error();
    }

    if (PyFloat_CheckExact(o)) {
        return Py_NewRef(o);
    }

    PyNumberMethods *m = Py_TYPE(o)->tp_as_number;
    if (m && m->nb_float) { /* This should include subclasses of float */
        PyObject *res = m->nb_float(o);
        assert(_Py_CheckSlotResult(o, "__float__", res != NULL));
        if (!res || PyFloat_CheckExact(res)) {
            return res;
        }

        if (!PyFloat_Check(res)) {
            PyErr_Format(PyExc_TypeError,
                         "%.50s.__float__ returned non-float (type %.50s)",
                         Py_TYPE(o)->tp_name, Py_TYPE(res)->tp_name);
            Py_DECREF(res);
            return NULL;
        }
        /* Issue #26983: warn if 'res' not of exact type float. */
        if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
                "%.50s.__float__ returned non-float (type %.50s).  "
                "The ability to return an instance of a strict subclass of float "
                "is deprecated, and may be removed in a future version of Python.",
                Py_TYPE(o)->tp_name, Py_TYPE(res)->tp_name)) {
            Py_DECREF(res);
            return NULL;
        }
        double val = PyFloat_AS_DOUBLE(res);
        Py_DECREF(res);
        return PyFloat_FromDouble(val);
    }

    if (m && m->nb_index) {
        PyObject *res = _PyNumber_Index(o);
        if (!res) {
            return NULL;
        }
        double val = PyLong_AsDouble(res);
        Py_DECREF(res);
        if (val == -1.0 && PyErr_Occurred()) {
            return NULL;
        }
        return PyFloat_FromDouble(val);
    }

    /* A float subclass with nb_float == NULL */
    if (PyFloat_Check(o)) {
        return PyFloat_FromDouble(PyFloat_AS_DOUBLE(o));
    }
    return PyFloat_FromString(o);
}


PyObject *
PyNumber_ToBase(PyObject *n, int base)
{
    if (!(base == 2 || base == 8 || base == 10 || base == 16)) {
        PyErr_SetString(PyExc_SystemError,
                        "PyNumber_ToBase: base must be 2, 8, 10 or 16");
        return NULL;
    }
    PyObject *index = _PyNumber_Index(n);
    if (!index)
        return NULL;
    PyObject *res = _PyLong_Format(index, base);
    Py_DECREF(index);
    return res;
}


/* Operations on sequences */

int
PySequence_Check(PyObject *s)
{
    if (PyDict_Check(s))
        return 0;
    return Py_TYPE(s)->tp_as_sequence &&
        Py_TYPE(s)->tp_as_sequence->sq_item != NULL;
}

Py_ssize_t
PySequence_Size(PyObject *s)
{
    if (s == NULL) {
        null_error();
        return -1;
    }

    PySequenceMethods *m = Py_TYPE(s)->tp_as_sequence;
    if (m && m->sq_length) {
        Py_ssize_t len = m->sq_length(s);
        assert(_Py_CheckSlotResult(s, "__len__", len >= 0));
        return len;
    }

    if (Py_TYPE(s)->tp_as_mapping && Py_TYPE(s)->tp_as_mapping->mp_length) {
        type_error("%.200s is not a sequence", s);
        return -1;
    }
    type_error("object of type '%.200s' has no len()", s);
    return -1;
}

#undef PySequence_Length
Py_ssize_t
PySequence_Length(PyObject *s)
{
    return PySequence_Size(s);
}
#define PySequence_Length PySequence_Size

PyObject *
PySequence_Concat(PyObject *s, PyObject *o)
{
    if (s == NULL || o == NULL) {
        return null_error();
    }

    PySequenceMethods *m = Py_TYPE(s)->tp_as_sequence;
    if (m && m->sq_concat) {
        PyObject *res = m->sq_concat(s, o);
        assert(_Py_CheckSlotResult(s, "+", res != NULL));
        return res;
    }

    /* Instances of user classes defining an __add__() method only
       have an nb_add slot, not an sq_concat slot.      So we fall back
       to nb_add if both arguments appear to be sequences. */
    if (PySequence_Check(s) && PySequence_Check(o)) {
        PyObject *result = BINARY_OP1(s, o, NB_SLOT(nb_add), "+");
        if (result != Py_NotImplemented)
            return result;
        Py_DECREF(result);
    }
    return type_error("'%.200s' object can't be concatenated", s);
}

PyObject *
PySequence_Repeat(PyObject *o, Py_ssize_t count)
{
    if (o == NULL) {
        return null_error();
    }

    PySequenceMethods *m = Py_TYPE(o)->tp_as_sequence;
    if (m && m->sq_repeat) {
        PyObject *res = m->sq_repeat(o, count);
        assert(_Py_CheckSlotResult(o, "*", res != NULL));
        return res;
    }

    /* Instances of user classes defining a __mul__() method only
       have an nb_multiply slot, not an sq_repeat slot. so we fall back
       to nb_multiply if o appears to be a sequence. */
    if (PySequence_Check(o)) {
        PyObject *n, *result;
        n = PyLong_FromSsize_t(count);
        if (n == NULL)
            return NULL;
        result = BINARY_OP1(o, n, NB_SLOT(nb_multiply), "*");
        Py_DECREF(n);
        if (result != Py_NotImplemented)
            return result;
        Py_DECREF(result);
    }
    return type_error("'%.200s' object can't be repeated", o);
}

PyObject *
PySequence_InPlaceConcat(PyObject *s, PyObject *o)
{
    if (s == NULL || o == NULL) {
        return null_error();
    }

    PySequenceMethods *m = Py_TYPE(s)->tp_as_sequence;
    if (m && m->sq_inplace_concat) {
        PyObject *res = m->sq_inplace_concat(s, o);
        assert(_Py_CheckSlotResult(s, "+=", res != NULL));
        return res;
    }
    if (m && m->sq_concat) {
        PyObject *res = m->sq_concat(s, o);
        assert(_Py_CheckSlotResult(s, "+", res != NULL));
        return res;
    }

    if (PySequence_Check(s) && PySequence_Check(o)) {
        PyObject *result = BINARY_IOP1(s, o, NB_SLOT(nb_inplace_add),
                                       NB_SLOT(nb_add), "+=");
        if (result != Py_NotImplemented)
            return result;
        Py_DECREF(result);
    }
    return type_error("'%.200s' object can't be concatenated", s);
}

PyObject *
PySequence_InPlaceRepeat(PyObject *o, Py_ssize_t count)
{
    if (o == NULL) {
        return null_error();
    }

    PySequenceMethods *m = Py_TYPE(o)->tp_as_sequence;
    if (m && m->sq_inplace_repeat) {
        PyObject *res = m->sq_inplace_repeat(o, count);
        assert(_Py_CheckSlotResult(o, "*=", res != NULL));
        return res;
    }
    if (m && m->sq_repeat) {
        PyObject *res = m->sq_repeat(o, count);
        assert(_Py_CheckSlotResult(o, "*", res != NULL));
        return res;
    }

    if (PySequence_Check(o)) {
        PyObject *n, *result;
        n = PyLong_FromSsize_t(count);
        if (n == NULL)
            return NULL;
        result = BINARY_IOP1(o, n, NB_SLOT(nb_inplace_multiply),
                             NB_SLOT(nb_multiply), "*=");
        Py_DECREF(n);
        if (result != Py_NotImplemented)
            return result;
        Py_DECREF(result);
    }
    return type_error("'%.200s' object can't be repeated", o);
}

PyObject *
PySequence_GetItem(PyObject *s, Py_ssize_t i)
{
    if (s == NULL) {
        return null_error();
    }

    PySequenceMethods *m = Py_TYPE(s)->tp_as_sequence;
    if (m && m->sq_item) {
        if (i < 0) {
            if (m->sq_length) {
                Py_ssize_t l = (*m->sq_length)(s);
                assert(_Py_CheckSlotResult(s, "__len__", l >= 0));
                if (l < 0) {
                    return NULL;
                }
                i += l;
            }
        }
        PyObject *res = m->sq_item(s, i);
        assert(_Py_CheckSlotResult(s, "__getitem__", res != NULL));
        return res;
    }

    if (Py_TYPE(s)->tp_as_mapping && Py_TYPE(s)->tp_as_mapping->mp_subscript) {
        return type_error("%.200s is not a sequence", s);
    }
    return type_error("'%.200s' object does not support indexing", s);
}

PyObject *
PySequence_GetSlice(PyObject *s, Py_ssize_t i1, Py_ssize_t i2)
{
    if (!s) {
        return null_error();
    }

    PyMappingMethods *mp = Py_TYPE(s)->tp_as_mapping;
    if (mp && mp->mp_subscript) {
        PyObject *slice = _PySlice_FromIndices(i1, i2);
        if (!slice) {
            return NULL;
        }
        PyObject *res = mp->mp_subscript(s, slice);
        assert(_Py_CheckSlotResult(s, "__getitem__", res != NULL));
        Py_DECREF(slice);
        return res;
    }

    return type_error("'%.200s' object is unsliceable", s);
}

int
PySequence_SetItem(PyObject *s, Py_ssize_t i, PyObject *o)
{
    if (s == NULL) {
        null_error();
        return -1;
    }

    PySequenceMethods *m = Py_TYPE(s)->tp_as_sequence;
    if (m && m->sq_ass_item) {
        if (i < 0) {
            if (m->sq_length) {
                Py_ssize_t l = (*m->sq_length)(s);
                assert(_Py_CheckSlotResult(s, "__len__", l >= 0));
                if (l < 0) {
                    return -1;
                }
                i += l;
            }
        }
        int res = m->sq_ass_item(s, i, o);
        assert(_Py_CheckSlotResult(s, "__setitem__", res >= 0));
        return res;
    }

    if (Py_TYPE(s)->tp_as_mapping && Py_TYPE(s)->tp_as_mapping->mp_ass_subscript) {
        type_error("%.200s is not a sequence", s);
        return -1;
    }
    type_error("'%.200s' object does not support item assignment", s);
    return -1;
}

int
PySequence_DelItem(PyObject *s, Py_ssize_t i)
{
    if (s == NULL) {
        null_error();
        return -1;
    }

    PySequenceMethods *m = Py_TYPE(s)->tp_as_sequence;
    if (m && m->sq_ass_item) {
        if (i < 0) {
            if (m->sq_length) {
                Py_ssize_t l = (*m->sq_length)(s);
                assert(_Py_CheckSlotResult(s, "__len__", l >= 0));
                if (l < 0) {
                    return -1;
                }
                i += l;
            }
        }
        int res = m->sq_ass_item(s, i, (PyObject *)NULL);
        assert(_Py_CheckSlotResult(s, "__delitem__", res >= 0));
        return res;
    }

    if (Py_TYPE(s)->tp_as_mapping && Py_TYPE(s)->tp_as_mapping->mp_ass_subscript) {
        type_error("%.200s is not a sequence", s);
        return -1;
    }
    type_error("'%.200s' object doesn't support item deletion", s);
    return -1;
}

int
PySequence_SetSlice(PyObject *s, Py_ssize_t i1, Py_ssize_t i2, PyObject *o)
{
    if (s == NULL) {
        null_error();
        return -1;
    }

    PyMappingMethods *mp = Py_TYPE(s)->tp_as_mapping;
    if (mp && mp->mp_ass_subscript) {
        PyObject *slice = _PySlice_FromIndices(i1, i2);
        if (!slice)
            return -1;
        int res = mp->mp_ass_subscript(s, slice, o);
        assert(_Py_CheckSlotResult(s, "__setitem__", res >= 0));
        Py_DECREF(slice);
        return res;
    }

    type_error("'%.200s' object doesn't support slice assignment", s);
    return -1;
}

int
PySequence_DelSlice(PyObject *s, Py_ssize_t i1, Py_ssize_t i2)
{
    if (s == NULL) {
        null_error();
        return -1;
    }

    PyMappingMethods *mp = Py_TYPE(s)->tp_as_mapping;
    if (mp && mp->mp_ass_subscript) {
        PyObject *slice = _PySlice_FromIndices(i1, i2);
        if (!slice) {
            return -1;
        }
        int res = mp->mp_ass_subscript(s, slice, NULL);
        assert(_Py_CheckSlotResult(s, "__delitem__", res >= 0));
        Py_DECREF(slice);
        return res;
    }
    type_error("'%.200s' object doesn't support slice deletion", s);
    return -1;
}

PyObject *
PySequence_Tuple(PyObject *v)
{
    PyObject *it;  /* iter(v) */

    if (v == NULL) {
        return null_error();
    }

    /* Special-case the common tuple and list cases, for efficiency. */
    if (PyTuple_CheckExact(v)) {
        /* Note that we can't know whether it's safe to return
           a tuple *subclass* instance as-is, hence the restriction
           to exact tuples here.  In contrast, lists always make
           a copy, so there's no need for exactness below. */
        return Py_NewRef(v);
    }
    if (PyList_CheckExact(v))
        return PyList_AsTuple(v);

    /* Get iterator. */
    it = PyObject_GetIter(v);
    if (it == NULL)
        return NULL;

    Py_ssize_t n;
    PyObject *buffer[8];
    for (n = 0; n < 8; n++) {
        PyObject *item = PyIter_Next(it);
        if (item == NULL) {
            if (PyErr_Occurred()) {
                goto fail;
            }
            Py_DECREF(it);
            return _PyTuple_FromArraySteal(buffer, n);
        }
        buffer[n] = item;
    }
    PyListObject *list = (PyListObject *)PyList_New(16);
    if (list == NULL) {
        goto fail;
    }
    assert(n == 8);
    Py_SET_SIZE(list, n);
    for (Py_ssize_t j = 0; j < n; j++) {
        PyList_SET_ITEM(list, j, buffer[j]);
    }
    for (;;) {
        PyObject *item = PyIter_Next(it);
        if (item == NULL) {
            if (PyErr_Occurred()) {
                Py_DECREF(list);
                Py_DECREF(it);
                return NULL;
            }
            break;
        }
        if (_PyList_AppendTakeRef(list, item) < 0) {
            Py_DECREF(list);
            Py_DECREF(it);
            return NULL;
        }
    }
    Py_DECREF(it);
    PyObject *res = _PyList_AsTupleAndClear(list);
    Py_DECREF(list);
    return res;
fail:
    Py_DECREF(it);
    while (n > 0) {
        n--;
        Py_DECREF(buffer[n]);
    }
    return NULL;
}

PyObject *
PySequence_List(PyObject *v)
{
    PyObject *result;  /* result list */
    PyObject *rv;          /* return value from PyList_Extend */

    if (v == NULL) {
        return null_error();
    }

    result = PyList_New(0);
    if (result == NULL)
        return NULL;

    rv = _PyList_Extend((PyListObject *)result, v);
    if (rv == NULL) {
        Py_DECREF(result);
        return NULL;
    }
    Py_DECREF(rv);
    return result;
}

PyObject *
PySequence_Fast(PyObject *v, const char *m)
{
    PyObject *it;

    if (v == NULL) {
        return null_error();
    }

    if (PyList_CheckExact(v) || PyTuple_CheckExact(v)) {
        return Py_NewRef(v);
    }

    it = PyObject_GetIter(v);
    if (it == NULL) {
        PyThreadState *tstate = _PyThreadState_GET();
        if (_PyErr_ExceptionMatches(tstate, PyExc_TypeError)) {
            _PyErr_SetString(tstate, PyExc_TypeError, m);
        }
        return NULL;
    }

    v = PySequence_List(it);
    Py_DECREF(it);

    return v;
}

/* Iterate over seq.  Result depends on the operation:
   PY_ITERSEARCH_COUNT:  -1 if error, else # of times obj appears in seq.
   PY_ITERSEARCH_INDEX:  0-based index of first occurrence of obj in seq;
    set ValueError and return -1 if none found; also return -1 on error.
   PY_ITERSEARCH_CONTAINS:  return 1 if obj in seq, else 0; -1 on error.
*/
Py_ssize_t
_PySequence_IterSearch(PyObject *seq, PyObject *obj, int operation)
{
    Py_ssize_t n;
    int wrapped;  /* for PY_ITERSEARCH_INDEX, true iff n wrapped around */
    PyObject *it;  /* iter(seq) */

    if (seq == NULL || obj == NULL) {
        null_error();
        return -1;
    }

    it = PyObject_GetIter(seq);
    if (it == NULL) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            if (operation == PY_ITERSEARCH_CONTAINS) {
                type_error(
                    "argument of type '%.200s' is not a container or iterable",
                    seq
                    );
            }
            else {
                type_error("argument of type '%.200s' is not iterable", seq);
            }
        }
        return -1;
    }

    n = wrapped = 0;
    for (;;) {
        int cmp;
        PyObject *item = PyIter_Next(it);
        if (item == NULL) {
            if (PyErr_Occurred())
                goto Fail;
            break;
        }

        cmp = PyObject_RichCompareBool(item, obj, Py_EQ);
        Py_DECREF(item);
        if (cmp < 0)
            goto Fail;
        if (cmp > 0) {
            switch (operation) {
            case PY_ITERSEARCH_COUNT:
                if (n == PY_SSIZE_T_MAX) {
                    PyErr_SetString(PyExc_OverflowError,
                           "count exceeds C integer size");
                    goto Fail;
                }
                ++n;
                break;

            case PY_ITERSEARCH_INDEX:
                if (wrapped) {
                    PyErr_SetString(PyExc_OverflowError,
                           "index exceeds C integer size");
                    goto Fail;
                }
                goto Done;

            case PY_ITERSEARCH_CONTAINS:
                n = 1;
                goto Done;

            default:
                Py_UNREACHABLE();
            }
        }

        if (operation == PY_ITERSEARCH_INDEX) {
            if (n == PY_SSIZE_T_MAX)
                wrapped = 1;
            ++n;
        }
    }

    if (operation != PY_ITERSEARCH_INDEX)
        goto Done;

    PyErr_SetString(PyExc_ValueError,
                    "sequence.index(x): x not in sequence");
    /* fall into failure code */
Fail:
    n = -1;
    /* fall through */
Done:
    Py_DECREF(it);
    return n;

}

/* Return # of times o appears in s. */
Py_ssize_t
PySequence_Count(PyObject *s, PyObject *o)
{
    return _PySequence_IterSearch(s, o, PY_ITERSEARCH_COUNT);
}

/* Return -1 if error; 1 if ob in seq; 0 if ob not in seq.
 * Use sq_contains if possible, else defer to _PySequence_IterSearch().
 */
int
PySequence_Contains(PyObject *seq, PyObject *ob)
{
    PySequenceMethods *sqm = Py_TYPE(seq)->tp_as_sequence;
    if (sqm != NULL && sqm->sq_contains != NULL) {
        int res = (*sqm->sq_contains)(seq, ob);
        assert(_Py_CheckSlotResult(seq, "__contains__", res >= 0));
        return res;
    }
    Py_ssize_t result = _PySequence_IterSearch(seq, ob, PY_ITERSEARCH_CONTAINS);
    return Py_SAFE_DOWNCAST(result, Py_ssize_t, int);
}

/* Backwards compatibility */
#undef PySequence_In
int
PySequence_In(PyObject *w, PyObject *v)
{
    return PySequence_Contains(w, v);
}

Py_ssize_t
PySequence_Index(PyObject *s, PyObject *o)
{
    return _PySequence_IterSearch(s, o, PY_ITERSEARCH_INDEX);
}

/* Operations on mappings */

int
PyMapping_Check(PyObject *o)
{
    return o && Py_TYPE(o)->tp_as_mapping &&
        Py_TYPE(o)->tp_as_mapping->mp_subscript;
}

Py_ssize_t
PyMapping_Size(PyObject *o)
{
    if (o == NULL) {
        null_error();
        return -1;
    }

    PyMappingMethods *m = Py_TYPE(o)->tp_as_mapping;
    if (m && m->mp_length) {
        Py_ssize_t len = m->mp_length(o);
        assert(_Py_CheckSlotResult(o, "__len__", len >= 0));
        return len;
    }

    if (Py_TYPE(o)->tp_as_sequence && Py_TYPE(o)->tp_as_sequence->sq_length) {
        type_error("%.200s is not a mapping", o);
        return -1;
    }
    /* PyMapping_Size() can be called from PyObject_Size(). */
    type_error("object of type '%.200s' has no len()", o);
    return -1;
}

#undef PyMapping_Length
Py_ssize_t
PyMapping_Length(PyObject *o)
{
    return PyMapping_Size(o);
}
#define PyMapping_Length PyMapping_Size

PyObject *
PyMapping_GetItemString(PyObject *o, const char *key)
{
    PyObject *okey, *r;

    if (key == NULL) {
        return null_error();
    }

    okey = PyUnicode_FromString(key);
    if (okey == NULL)
        return NULL;
    r = PyObject_GetItem(o, okey);
    Py_DECREF(okey);
    return r;
}

int
PyMapping_GetOptionalItemString(PyObject *obj, const char *key, PyObject **result)
{
    if (key == NULL) {
        *result = NULL;
        null_error();
        return -1;
    }
    PyObject *okey = PyUnicode_FromString(key);
    if (okey == NULL) {
        *result = NULL;
        return -1;
    }
    int rc = PyMapping_GetOptionalItem(obj, okey, result);
    Py_DECREF(okey);
    return rc;
}

int
PyMapping_SetItemString(PyObject *o, const char *key, PyObject *value)
{
    PyObject *okey;
    int r;

    if (key == NULL) {
        null_error();
        return -1;
    }

    okey = PyUnicode_FromString(key);
    if (okey == NULL)
        return -1;
    r = PyObject_SetItem(o, okey, value);
    Py_DECREF(okey);
    return r;
}

int
PyMapping_HasKeyStringWithError(PyObject *obj, const char *key)
{
    PyObject *res;
    int rc = PyMapping_GetOptionalItemString(obj, key, &res);
    Py_XDECREF(res);
    return rc;
}

int
PyMapping_HasKeyWithError(PyObject *obj, PyObject *key)
{
    PyObject *res;
    int rc = PyMapping_GetOptionalItem(obj, key, &res);
    Py_XDECREF(res);
    return rc;
}

int
PyMapping_HasKeyString(PyObject *obj, const char *key)
{
    PyObject *value;
    int rc;
    if (obj == NULL) {
        // For backward compatibility.
        // PyMapping_GetOptionalItemString() crashes if obj is NULL.
        null_error();
        rc = -1;
    }
    else {
        rc = PyMapping_GetOptionalItemString(obj, key, &value);
    }
    if (rc < 0) {
        PyErr_FormatUnraisable(
            "Exception ignored in PyMapping_HasKeyString(); consider using "
            "PyMapping_HasKeyStringWithError(), "
            "PyMapping_GetOptionalItemString() or PyMapping_GetItemString()");
        return 0;
    }
    Py_XDECREF(value);
    return rc;
}

int
PyMapping_HasKey(PyObject *obj, PyObject *key)
{
    PyObject *value;
    int rc;
    if (obj == NULL || key == NULL) {
        // For backward compatibility.
        // PyMapping_GetOptionalItem() crashes if any of them is NULL.
        null_error();
        rc = -1;
    }
    else {
        rc = PyMapping_GetOptionalItem(obj, key, &value);
    }
    if (rc < 0) {
        PyErr_FormatUnraisable(
            "Exception ignored in PyMapping_HasKey(); consider using "
            "PyMapping_HasKeyWithError(), "
            "PyMapping_GetOptionalItem() or PyObject_GetItem()");
        return 0;
    }
    Py_XDECREF(value);
    return rc;
}

/* This function is quite similar to PySequence_Fast(), but specialized to be
   a helper for PyMapping_Keys(), PyMapping_Items() and PyMapping_Values().
 */
static PyObject *
method_output_as_list(PyObject *o, PyObject *meth)
{
    PyObject *it, *result, *meth_output;

    assert(o != NULL);
    meth_output = PyObject_CallMethodNoArgs(o, meth);
    if (meth_output == NULL || PyList_CheckExact(meth_output)) {
        return meth_output;
    }
    it = PyObject_GetIter(meth_output);
    if (it == NULL) {
        PyThreadState *tstate = _PyThreadState_GET();
        if (_PyErr_ExceptionMatches(tstate, PyExc_TypeError)) {
            _PyErr_Format(tstate, PyExc_TypeError,
                          "%.200s.%U() returned a non-iterable (type %.200s)",
                          Py_TYPE(o)->tp_name,
                          meth,
                          Py_TYPE(meth_output)->tp_name);
        }
        Py_DECREF(meth_output);
        return NULL;
    }
    Py_DECREF(meth_output);
    result = PySequence_List(it);
    Py_DECREF(it);
    return result;
}

PyObject *
PyMapping_Keys(PyObject *o)
{
    if (o == NULL) {
        return null_error();
    }
    if (PyDict_CheckExact(o)) {
        return PyDict_Keys(o);
    }
    return method_output_as_list(o, &_Py_ID(keys));
}

PyObject *
PyMapping_Items(PyObject *o)
{
    if (o == NULL) {
        return null_error();
    }
    if (PyDict_CheckExact(o)) {
        return PyDict_Items(o);
    }
    return method_output_as_list(o, &_Py_ID(items));
}

PyObject *
PyMapping_Values(PyObject *o)
{
    if (o == NULL) {
        return null_error();
    }
    if (PyDict_CheckExact(o)) {
        return PyDict_Values(o);
    }
    return method_output_as_list(o, &_Py_ID(values));
}

/* isinstance(), issubclass() */

/* abstract_get_bases() has logically 4 return states:
 *
 * 1. getattr(cls, '__bases__') could raise an AttributeError
 * 2. getattr(cls, '__bases__') could raise some other exception
 * 3. getattr(cls, '__bases__') could return a tuple
 * 4. getattr(cls, '__bases__') could return something other than a tuple
 *
 * Only state #3 is a non-error state and only it returns a non-NULL object
 * (it returns the retrieved tuple).
 *
 * Any raised AttributeErrors are masked by clearing the exception and
 * returning NULL.  If an object other than a tuple comes out of __bases__,
 * then again, the return value is NULL.  So yes, these two situations
 * produce exactly the same results: NULL is returned and no error is set.
 *
 * If some exception other than AttributeError is raised, then NULL is also
 * returned, but the exception is not cleared.  That's because we want the
 * exception to be propagated along.
 *
 * Callers are expected to test for PyErr_Occurred() when the return value
 * is NULL to decide whether a valid exception should be propagated or not.
 * When there's no exception to propagate, it's customary for the caller to
 * set a TypeError.
 */
static PyObject *
abstract_get_bases(PyObject *cls)
{
    PyObject *bases;

    (void)PyObject_GetOptionalAttr(cls, &_Py_ID(__bases__), &bases);
    if (bases != NULL && !PyTuple_Check(bases)) {
        Py_DECREF(bases);
        return NULL;
    }
    return bases;
}


static int
abstract_issubclass(PyObject *derived, PyObject *cls)
{
    PyObject *bases = NULL;
    Py_ssize_t i, n;
    int r = 0;

    while (1) {
        if (derived == cls) {
            Py_XDECREF(bases); /* See below comment */
            return 1;
        }
        /* Use XSETREF to drop bases reference *after* finishing with
           derived; bases might be the only reference to it.
           XSETREF is used instead of SETREF, because bases is NULL on the
           first iteration of the loop.
        */
        Py_XSETREF(bases, abstract_get_bases(derived));
        if (bases == NULL) {
            if (PyErr_Occurred())
                return -1;
            return 0;
        }
        n = PyTuple_GET_SIZE(bases);
        if (n == 0) {
            Py_DECREF(bases);
            return 0;
        }
        /* Avoid recursivity in the single inheritance case */
        if (n == 1) {
            derived = PyTuple_GET_ITEM(bases, 0);
            continue;
        }
        break;
    }
    assert(n >= 2);
    if (_Py_EnterRecursiveCall(" in __issubclass__")) {
        Py_DECREF(bases);
        return -1;
    }
    for (i = 0; i < n; i++) {
        r = abstract_issubclass(PyTuple_GET_ITEM(bases, i), cls);
        if (r != 0) {
            break;
        }
    }
    _Py_LeaveRecursiveCall();
    Py_DECREF(bases);
    return r;
}

static int
check_class(PyObject *cls, const char *error)
{
    PyObject *bases = abstract_get_bases(cls);
    if (bases == NULL) {
        /* Do not mask errors. */
        PyThreadState *tstate = _PyThreadState_GET();
        if (!_PyErr_Occurred(tstate)) {
            _PyErr_SetString(tstate, PyExc_TypeError, error);
        }
        return 0;
    }
    Py_DECREF(bases);
    return -1;
}

static int
object_isinstance(PyObject *inst, PyObject *cls)
{
    PyObject *icls;
    int retval;
    if (PyType_Check(cls)) {
        retval = PyObject_TypeCheck(inst, (PyTypeObject *)cls);
        if (retval == 0) {
            retval = PyObject_GetOptionalAttr(inst, &_Py_ID(__class__), &icls);
            if (icls != NULL) {
                if (icls != (PyObject *)(Py_TYPE(inst)) && PyType_Check(icls)) {
                    retval = PyType_IsSubtype(
                        (PyTypeObject *)icls,
                        (PyTypeObject *)cls);
                }
                else {
                    retval = 0;
                }
                Py_DECREF(icls);
            }
        }
    }
    else {
        if (!check_class(cls,
            "isinstance() arg 2 must be a type, a tuple of types, or a union"))
            return -1;
        retval = PyObject_GetOptionalAttr(inst, &_Py_ID(__class__), &icls);
        if (icls != NULL) {
            retval = abstract_issubclass(icls, cls);
            Py_DECREF(icls);
        }
    }

    return retval;
}

static int
object_recursive_isinstance(PyThreadState *tstate, PyObject *inst, PyObject *cls)
{
    /* Quick test for an exact match */
    if (Py_IS_TYPE(inst, (PyTypeObject *)cls)) {
        return 1;
    }

    /* We know what type's __instancecheck__ does. */
    if (PyType_CheckExact(cls)) {
        return object_isinstance(inst, cls);
    }

    if (_PyUnion_Check(cls)) {
        cls = _Py_union_args(cls);
    }

    if (PyTuple_Check(cls)) {
        /* Not a general sequence -- that opens up the road to
           recursion and stack overflow. */
        if (_Py_EnterRecursiveCallTstate(tstate, " in __instancecheck__")) {
            return -1;
        }
        Py_ssize_t n = PyTuple_GET_SIZE(cls);
        int r = 0;
        for (Py_ssize_t i = 0; i < n; ++i) {
            PyObject *item = PyTuple_GET_ITEM(cls, i);
            r = object_recursive_isinstance(tstate, inst, item);
            if (r != 0) {
                /* either found it, or got an error */
                break;
            }
        }
        _Py_LeaveRecursiveCallTstate(tstate);
        return r;
    }

    PyObject *checker = _PyObject_LookupSpecial(cls, &_Py_ID(__instancecheck__));
    if (checker != NULL) {
        if (_Py_EnterRecursiveCallTstate(tstate, " in __instancecheck__")) {
            Py_DECREF(checker);
            return -1;
        }

        PyObject *res = PyObject_CallOneArg(checker, inst);
        _Py_LeaveRecursiveCallTstate(tstate);
        Py_DECREF(checker);

        if (res == NULL) {
            return -1;
        }
        int ok = PyObject_IsTrue(res);
        Py_DECREF(res);

        return ok;
    }
    else if (_PyErr_Occurred(tstate)) {
        return -1;
    }

    /* cls has no __instancecheck__() method */
    return object_isinstance(inst, cls);
}


int
PyObject_IsInstance(PyObject *inst, PyObject *cls)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return object_recursive_isinstance(tstate, inst, cls);
}


static  int
recursive_issubclass(PyObject *derived, PyObject *cls)
{
    if (PyType_Check(cls) && PyType_Check(derived)) {
        /* Fast path (non-recursive) */
        return PyType_IsSubtype((PyTypeObject *)derived, (PyTypeObject *)cls);
    }
    if (!check_class(derived,
                     "issubclass() arg 1 must be a class"))
        return -1;

    if (!_PyUnion_Check(cls) && !check_class(cls,
                            "issubclass() arg 2 must be a class,"
                            " a tuple of classes, or a union")) {
        return -1;
    }

    return abstract_issubclass(derived, cls);
}

static int
object_issubclass(PyThreadState *tstate, PyObject *derived, PyObject *cls)
{
    PyObject *checker;

    /* We know what type's __subclasscheck__ does. */
    if (PyType_CheckExact(cls)) {
        /* Quick test for an exact match */
        if (derived == cls)
            return 1;
        return recursive_issubclass(derived, cls);
    }

    if (_PyUnion_Check(cls)) {
        cls = _Py_union_args(cls);
    }

    if (PyTuple_Check(cls)) {

        if (_Py_EnterRecursiveCallTstate(tstate, " in __subclasscheck__")) {
            return -1;
        }
        Py_ssize_t n = PyTuple_GET_SIZE(cls);
        int r = 0;
        for (Py_ssize_t i = 0; i < n; ++i) {
            PyObject *item = PyTuple_GET_ITEM(cls, i);
            r = object_issubclass(tstate, derived, item);
            if (r != 0)
                /* either found it, or got an error */
                break;
        }
        _Py_LeaveRecursiveCallTstate(tstate);
        return r;
    }

    checker = _PyObject_LookupSpecial(cls, &_Py_ID(__subclasscheck__));
    if (checker != NULL) {
        int ok = -1;
        if (_Py_EnterRecursiveCallTstate(tstate, " in __subclasscheck__")) {
            Py_DECREF(checker);
            return ok;
        }
        PyObject *res = PyObject_CallOneArg(checker, derived);
        _Py_LeaveRecursiveCallTstate(tstate);
        Py_DECREF(checker);
        if (res != NULL) {
            ok = PyObject_IsTrue(res);
            Py_DECREF(res);
        }
        return ok;
    }
    else if (_PyErr_Occurred(tstate)) {
        return -1;
    }

    /* Can be reached when infinite recursion happens. */
    return recursive_issubclass(derived, cls);
}


int
PyObject_IsSubclass(PyObject *derived, PyObject *cls)
{
    PyThreadState *tstate = _PyThreadState_GET();
    return object_issubclass(tstate, derived, cls);
}


int
_PyObject_RealIsInstance(PyObject *inst, PyObject *cls)
{
    return object_isinstance(inst, cls);
}

int
_PyObject_RealIsSubclass(PyObject *derived, PyObject *cls)
{
    return recursive_issubclass(derived, cls);
}


PyObject *
PyObject_GetIter(PyObject *o)
{
    PyTypeObject *t = Py_TYPE(o);
    getiterfunc f;

    f = t->tp_iter;
    if (f == NULL) {
        if (PySequence_Check(o))
            return PySeqIter_New(o);
        return type_error("'%.200s' object is not iterable", o);
    }
    else {
        PyObject *res = (*f)(o);
        if (res != NULL && !PyIter_Check(res)) {
            PyErr_Format(PyExc_TypeError,
                         "iter() returned non-iterator "
                         "of type '%.100s'",
                         Py_TYPE(res)->tp_name);
            Py_SETREF(res, NULL);
        }
        return res;
    }
}

PyObject *
PyObject_GetAIter(PyObject *o) {
    PyTypeObject *t = Py_TYPE(o);
    unaryfunc f;

    if (t->tp_as_async == NULL || t->tp_as_async->am_aiter == NULL) {
        return type_error("'%.200s' object is not an async iterable", o);
    }
    f = t->tp_as_async->am_aiter;
    PyObject *it = (*f)(o);
    if (it != NULL && !PyAIter_Check(it)) {
        PyErr_Format(PyExc_TypeError,
                     "aiter() returned not an async iterator of type '%.100s'",
                     Py_TYPE(it)->tp_name);
        Py_SETREF(it, NULL);
    }
    return it;
}

int
PyIter_Check(PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    return (tp->tp_iternext != NULL &&
            tp->tp_iternext != &_PyObject_NextNotImplemented);
}

int
PyAIter_Check(PyObject *obj)
{
    PyTypeObject *tp = Py_TYPE(obj);
    return (tp->tp_as_async != NULL &&
            tp->tp_as_async->am_anext != NULL &&
            tp->tp_as_async->am_anext != &_PyObject_NextNotImplemented);
}

static int
iternext(PyObject *iter, PyObject **item)
{
    iternextfunc tp_iternext = Py_TYPE(iter)->tp_iternext;
    if ((*item = tp_iternext(iter))) {
        return 1;
    }

    PyThreadState *tstate = _PyThreadState_GET();
    /* When the iterator is exhausted it must return NULL;
     * a StopIteration exception may or may not be set. */
    if (!_PyErr_Occurred(tstate)) {
        return 0;
    }
    if (_PyErr_ExceptionMatches(tstate, PyExc_StopIteration)) {
        _PyErr_Clear(tstate);
        return 0;
    }

    /* Error case: an exception (different than StopIteration) is set. */
    return -1;
}

/* Return 1 and set 'item' to the next item of 'iter' on success.
 * Return 0 and set 'item' to NULL when there are no remaining values.
 * Return -1, set 'item' to NULL and set an exception on error.
 */
int
PyIter_NextItem(PyObject *iter, PyObject **item)
{
    assert(iter != NULL);
    assert(item != NULL);

    if (Py_TYPE(iter)->tp_iternext == NULL) {
        *item = NULL;
        PyErr_Format(PyExc_TypeError, "expected an iterator, got '%T'", iter);
        return -1;
    }

    return iternext(iter, item);
}

/* Return next item.
 *
 * If an error occurs, return NULL.  PyErr_Occurred() will be true.
 * If the iteration terminates normally, return NULL and clear the
 * PyExc_StopIteration exception (if it was set).  PyErr_Occurred()
 * will be false.
 * Else return the next object.  PyErr_Occurred() will be false.
 */
PyObject *
PyIter_Next(PyObject *iter)
{
    PyObject *item;
    (void)iternext(iter, &item);
    return item;
}

PySendResult
PyIter_Send(PyObject *iter, PyObject *arg, PyObject **result)
{
    assert(arg != NULL);
    assert(result != NULL);
    if (Py_TYPE(iter)->tp_as_async && Py_TYPE(iter)->tp_as_async->am_send) {
        PySendResult res = Py_TYPE(iter)->tp_as_async->am_send(iter, arg, result);
        assert(_Py_CheckSlotResult(iter, "am_send", res != PYGEN_ERROR));
        return res;
    }
    if (arg == Py_None && PyIter_Check(iter)) {
        *result = Py_TYPE(iter)->tp_iternext(iter);
    }
    else {
        *result = PyObject_CallMethodOneArg(iter, &_Py_ID(send), arg);
    }
    if (*result != NULL) {
        return PYGEN_NEXT;
    }
    if (_PyGen_FetchStopIterationValue(result) == 0) {
        return PYGEN_RETURN;
    }
    return PYGEN_ERROR;
}
