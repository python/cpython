/* C Extension module to test all aspects of PEP-3118.
   Written by Stefan Krah. */


#define PY_SSIZE_T_CLEAN

#include "Python.h"


/* struct module */
PyObject *structmodule = NULL;
PyObject *Struct = NULL;
PyObject *calcsize = NULL;

/* cache simple format string */
static const char *simple_fmt = "B";
PyObject *simple_format = NULL;
#define SIMPLE_FORMAT(fmt) (fmt == NULL || strcmp(fmt, "B") == 0)


/**************************************************************************/
/*                             NDArray Object                             */
/**************************************************************************/

static PyTypeObject NDArray_Type;
#define NDArray_Check(v) (Py_TYPE(v) == &NDArray_Type)

#define CHECK_LIST_OR_TUPLE(v) \
    if (!PyList_Check(v) && !PyTuple_Check(v)) { \
        PyErr_SetString(PyExc_TypeError,         \
            #v " must be a list or a tuple");    \
        return NULL;                             \
    }                                            \

#define PyMem_XFree(v) \
    do { if (v) PyMem_Free(v); } while (0)

/* Maximum number of dimensions. */
#define ND_MAX_NDIM (2 * PyBUF_MAX_NDIM)

/* Check for the presence of suboffsets in the first dimension. */
#define HAVE_PTR(suboffsets) (suboffsets && suboffsets[0] >= 0)
/* Adjust ptr if suboffsets are present. */
#define ADJUST_PTR(ptr, suboffsets) \
    (HAVE_PTR(suboffsets) ? *((char**)ptr) + suboffsets[0] : ptr)

/* Default: NumPy style (strides), read-only, no var-export, C-style layout */
#define ND_DEFAULT          0x000
/* User configurable flags for the ndarray */
#define ND_VAREXPORT        0x001   /* change layout while buffers are exported */
/* User configurable flags for each base buffer */
#define ND_WRITABLE         0x002   /* mark base buffer as writable */
#define ND_FORTRAN          0x004   /* Fortran contiguous layout */
#define ND_SCALAR           0x008   /* scalar: ndim = 0 */
#define ND_PIL              0x010   /* convert to PIL-style array (suboffsets) */
#define ND_REDIRECT         0x020   /* redirect buffer requests */
#define ND_GETBUF_FAIL      0x040   /* trigger getbuffer failure */
#define ND_GETBUF_UNDEFINED 0x080   /* undefined view.obj */
/* Internal flags for the base buffer */
#define ND_C                0x100   /* C contiguous layout (default) */
#define ND_OWN_ARRAYS       0x200   /* consumer owns arrays */

/* ndarray properties */
#define ND_IS_CONSUMER(nd) \
    (((NDArrayObject *)nd)->head == &((NDArrayObject *)nd)->staticbuf)

/* ndbuf->flags properties */
#define ND_C_CONTIGUOUS(flags) (!!(flags&(ND_SCALAR|ND_C)))
#define ND_FORTRAN_CONTIGUOUS(flags) (!!(flags&(ND_SCALAR|ND_FORTRAN)))
#define ND_ANY_CONTIGUOUS(flags) (!!(flags&(ND_SCALAR|ND_C|ND_FORTRAN)))

/* getbuffer() requests */
#define REQ_INDIRECT(flags) ((flags&PyBUF_INDIRECT) == PyBUF_INDIRECT)
#define REQ_C_CONTIGUOUS(flags) ((flags&PyBUF_C_CONTIGUOUS) == PyBUF_C_CONTIGUOUS)
#define REQ_F_CONTIGUOUS(flags) ((flags&PyBUF_F_CONTIGUOUS) == PyBUF_F_CONTIGUOUS)
#define REQ_ANY_CONTIGUOUS(flags) ((flags&PyBUF_ANY_CONTIGUOUS) == PyBUF_ANY_CONTIGUOUS)
#define REQ_STRIDES(flags) ((flags&PyBUF_STRIDES) == PyBUF_STRIDES)
#define REQ_SHAPE(flags) ((flags&PyBUF_ND) == PyBUF_ND)
#define REQ_WRITABLE(flags) (flags&PyBUF_WRITABLE)
#define REQ_FORMAT(flags) (flags&PyBUF_FORMAT)


/* Single node of a list of base buffers. The list is needed to implement
   changes in memory layout while exported buffers are active. */
static PyTypeObject NDArray_Type;

struct ndbuf;
typedef struct ndbuf {
    struct ndbuf *next;
    struct ndbuf *prev;
    Py_ssize_t len;     /* length of data */
    Py_ssize_t offset;  /* start of the array relative to data */
    char *data;         /* raw data */
    int flags;          /* capabilities of the base buffer */
    Py_ssize_t exports; /* number of exports */
    Py_buffer base;     /* base buffer */
} ndbuf_t;

typedef struct {
    PyObject_HEAD
    int flags;          /* ndarray flags */
    ndbuf_t staticbuf;  /* static buffer for re-exporting mode */
    ndbuf_t *head;      /* currently active base buffer */
} NDArrayObject;


static ndbuf_t *
ndbuf_new(Py_ssize_t nitems, Py_ssize_t itemsize, Py_ssize_t offset, int flags)
{
    ndbuf_t *ndbuf;
    Py_buffer *base;
    Py_ssize_t len;

    len = nitems * itemsize;
    if (offset % itemsize) {
        PyErr_SetString(PyExc_ValueError,
            "offset must be a multiple of itemsize");
        return NULL;
    }
    if (offset < 0 || offset+itemsize > len) {
        PyErr_SetString(PyExc_ValueError, "offset out of bounds");
        return NULL;
    }

    ndbuf = PyMem_Malloc(sizeof *ndbuf);
    if (ndbuf == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    ndbuf->next = NULL;
    ndbuf->prev = NULL;
    ndbuf->len = len;
    ndbuf->offset= offset;

    ndbuf->data = PyMem_Malloc(len);
    if (ndbuf->data == NULL) {
        PyErr_NoMemory();
        PyMem_Free(ndbuf);
        return NULL;
    }

    ndbuf->flags = flags;
    ndbuf->exports = 0;

    base = &ndbuf->base;
    base->obj = NULL;
    base->buf = ndbuf->data;
    base->len = len;
    base->itemsize = 1;
    base->readonly = 0;
    base->format = NULL;
    base->ndim = 1;
    base->shape = NULL;
    base->strides = NULL;
    base->suboffsets = NULL;
    base->internal = ndbuf;

    return ndbuf;
}

static void
ndbuf_free(ndbuf_t *ndbuf)
{
    Py_buffer *base = &ndbuf->base;

    PyMem_XFree(ndbuf->data);
    PyMem_XFree(base->format);
    PyMem_XFree(base->shape);
    PyMem_XFree(base->strides);
    PyMem_XFree(base->suboffsets);

    PyMem_Free(ndbuf);
}

static void
ndbuf_push(NDArrayObject *nd, ndbuf_t *elt)
{
    elt->next = nd->head;
    if (nd->head) nd->head->prev = elt;
    nd->head = elt;
    elt->prev = NULL;
}

static void
ndbuf_delete(NDArrayObject *nd, ndbuf_t *elt)
{
    if (elt->prev)
        elt->prev->next = elt->next;
    else
        nd->head = elt->next;
    
    if (elt->next)
        elt->next->prev = elt->prev;

    ndbuf_free(elt);
}

static void
ndbuf_pop(NDArrayObject *nd)
{
    ndbuf_delete(nd, nd->head);
}


static PyObject *
ndarray_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    NDArrayObject *nd;

    nd = PyObject_New(NDArrayObject, &NDArray_Type);
    if (nd == NULL)
        return NULL;

    nd->flags = 0;
    nd->head = NULL;
    return (PyObject *)nd;
}

static void
ndarray_dealloc(NDArrayObject *self)
{
    if (self->head) {
        if (ND_IS_CONSUMER(self)) {
            Py_buffer *base = &self->head->base;
            if (self->head->flags & ND_OWN_ARRAYS) {
                PyMem_XFree(base->shape);
                PyMem_XFree(base->strides);
                PyMem_XFree(base->suboffsets);
            }
            PyBuffer_Release(base);
        }
        else {
            while (self->head)
                ndbuf_pop(self);
        }
    }
    PyObject_Del(self);
}

static int
ndarray_init_staticbuf(PyObject *exporter, NDArrayObject *nd, int flags)
{
    Py_buffer *base = &nd->staticbuf.base;

    if (PyObject_GetBuffer(exporter, base, flags) < 0)
        return -1;

    nd->head = &nd->staticbuf;

    nd->head->next = NULL;
    nd->head->prev = NULL;
    nd->head->len = -1;
    nd->head->offset = -1;
    nd->head->data = NULL;

    nd->head->flags = base->readonly ? 0 : ND_WRITABLE;
    nd->head->exports = 0;

    return 0;
}

static void
init_flags(ndbuf_t *ndbuf)
{
    if (ndbuf->base.ndim == 0)
        ndbuf->flags |= ND_SCALAR;
    if (ndbuf->base.suboffsets)
        ndbuf->flags |= ND_PIL;
    if (PyBuffer_IsContiguous(&ndbuf->base, 'C'))
        ndbuf->flags |= ND_C;
    if (PyBuffer_IsContiguous(&ndbuf->base, 'F'))
        ndbuf->flags |= ND_FORTRAN;
}


/****************************************************************************/
/*                          Buffer/List conversions                         */
/****************************************************************************/

static Py_ssize_t *strides_from_shape(const ndbuf_t *, int flags);

/* Get number of members in a struct: see issue #12740 */
typedef struct {
    PyObject_HEAD
    Py_ssize_t s_size;
    Py_ssize_t s_len;
} PyPartialStructObject;

static Py_ssize_t
get_nmemb(PyObject *s)
{
    return ((PyPartialStructObject *)s)->s_len;
}

/* Pack all items into the buffer of 'obj'. The 'format' parameter must be
   in struct module syntax. For standard C types, a single item is an integer.
   For compound types, a single item is a tuple of integers. */
static int
pack_from_list(PyObject *obj, PyObject *items, PyObject *format,
               Py_ssize_t itemsize)
{
    PyObject *structobj, *pack_into;
    PyObject *args, *offset;
    PyObject *item, *tmp;
    Py_ssize_t nitems; /* number of items */
    Py_ssize_t nmemb;  /* number of members in a single item */
    Py_ssize_t i, j;
    int ret = 0;

    assert(PyObject_CheckBuffer(obj));
    assert(PyList_Check(items) || PyTuple_Check(items));

    structobj = PyObject_CallFunctionObjArgs(Struct, format, NULL);
    if (structobj == NULL)
        return -1;

    nitems = PySequence_Fast_GET_SIZE(items);
    nmemb = get_nmemb(structobj);
    assert(nmemb >= 1);

    pack_into = PyObject_GetAttrString(structobj, "pack_into");
    if (pack_into == NULL) {
        Py_DECREF(structobj);
        return -1;
    }

    /* nmemb >= 1 */
    args = PyTuple_New(2 + nmemb);
    if (args == NULL) {
        Py_DECREF(pack_into);
        Py_DECREF(structobj);
        return -1;
    }

    offset = NULL;
    for (i = 0; i < nitems; i++) {
        /* Loop invariant: args[j] are borrowed references or NULL. */
        PyTuple_SET_ITEM(args, 0, obj);
        for (j = 1; j < 2+nmemb; j++)
            PyTuple_SET_ITEM(args, j, NULL);

        Py_XDECREF(offset);
        offset = PyLong_FromSsize_t(i*itemsize);
        if (offset == NULL) {
            ret = -1;
            break;
        }
        PyTuple_SET_ITEM(args, 1, offset);

        item = PySequence_Fast_GET_ITEM(items, i);
        if ((PyBytes_Check(item) || PyLong_Check(item) ||
             PyFloat_Check(item)) && nmemb == 1) {
            PyTuple_SET_ITEM(args, 2, item);
        }
        else if ((PyList_Check(item) || PyTuple_Check(item)) &&
                 PySequence_Length(item) == nmemb) {
            for (j = 0; j < nmemb; j++) {
                tmp = PySequence_Fast_GET_ITEM(item, j);
                PyTuple_SET_ITEM(args, 2+j, tmp);
            }
        }
        else {
            PyErr_SetString(PyExc_ValueError,
                "mismatch between initializer element and format string");
            ret = -1;
            break;
        }

        tmp = PyObject_CallObject(pack_into, args);
        if (tmp == NULL) {
            ret = -1;
            break;
        }
        Py_DECREF(tmp);
    }

    Py_INCREF(obj); /* args[0] */
    /* args[1]: offset is either NULL or should be dealloc'd */
    for (i = 2; i < 2+nmemb; i++) {
        tmp = PyTuple_GET_ITEM(args, i);
        Py_XINCREF(tmp);
    }
    Py_DECREF(args);

    Py_DECREF(pack_into);
    Py_DECREF(structobj);
    return ret;

}

/* Pack single element */
static int
pack_single(char *ptr, PyObject *item, const char *fmt, Py_ssize_t itemsize)
{
    PyObject *structobj = NULL, *pack_into = NULL, *args = NULL;
    PyObject *format = NULL, *mview = NULL, *zero = NULL;
    Py_ssize_t i, nmemb;
    int ret = -1;
    PyObject *x;

    if (fmt == NULL) fmt = "B";

    format = PyUnicode_FromString(fmt);
    if (format == NULL)
        goto out;

    structobj = PyObject_CallFunctionObjArgs(Struct, format, NULL);
    if (structobj == NULL)
        goto out;

    nmemb = get_nmemb(structobj);
    assert(nmemb >= 1);

    mview = PyMemoryView_FromMemory(ptr, itemsize, PyBUF_WRITE);
    if (mview == NULL)
        goto out;

    zero = PyLong_FromLong(0);
    if (zero == NULL)
        goto out;

    pack_into = PyObject_GetAttrString(structobj, "pack_into");
    if (pack_into == NULL)
        goto out;

    args = PyTuple_New(2+nmemb);
    if (args == NULL)
        goto out;

    PyTuple_SET_ITEM(args, 0, mview);
    PyTuple_SET_ITEM(args, 1, zero);

    if ((PyBytes_Check(item) || PyLong_Check(item) ||
         PyFloat_Check(item)) && nmemb == 1) {
         PyTuple_SET_ITEM(args, 2, item);
    }
    else if ((PyList_Check(item) || PyTuple_Check(item)) &&
             PySequence_Length(item) == nmemb) {
        for (i = 0; i < nmemb; i++) {
            x = PySequence_Fast_GET_ITEM(item, i);
            PyTuple_SET_ITEM(args, 2+i, x);
        }
    }
    else {
        PyErr_SetString(PyExc_ValueError,
            "mismatch between initializer element and format string");
        goto args_out;
    }

    x = PyObject_CallObject(pack_into, args);
    if (x != NULL) {
        Py_DECREF(x);
        ret = 0;
    }


args_out:
    for (i = 0; i < 2+nmemb; i++)
        Py_XINCREF(PyTuple_GET_ITEM(args, i));
    Py_XDECREF(args);
out:
    Py_XDECREF(pack_into);
    Py_XDECREF(zero);
    Py_XDECREF(mview);
    Py_XDECREF(structobj);
    Py_XDECREF(format);
    return ret;
}

static void
copy_rec(const Py_ssize_t *shape, Py_ssize_t ndim, Py_ssize_t itemsize,
         char *dptr, const Py_ssize_t *dstrides, const Py_ssize_t *dsuboffsets,
         char *sptr, const Py_ssize_t *sstrides, const Py_ssize_t *ssuboffsets,
         char *mem)
{
    Py_ssize_t i;

    assert(ndim >= 1);

    if (ndim == 1) {
        if (!HAVE_PTR(dsuboffsets) && !HAVE_PTR(ssuboffsets) &&
            dstrides[0] == itemsize && sstrides[0] == itemsize) {
            memmove(dptr, sptr, shape[0] * itemsize);
        }
        else {
            char *p;
            assert(mem != NULL);
            for (i=0, p=mem; i<shape[0]; p+=itemsize, sptr+=sstrides[0], i++) {
                char *xsptr = ADJUST_PTR(sptr, ssuboffsets);
                memcpy(p, xsptr, itemsize);
            }
            for (i=0, p=mem; i<shape[0]; p+=itemsize, dptr+=dstrides[0], i++) {
                char *xdptr = ADJUST_PTR(dptr, dsuboffsets);
                memcpy(xdptr, p, itemsize);
            }
        }
        return;
    }

    for (i = 0; i < shape[0]; dptr+=dstrides[0], sptr+=sstrides[0], i++) {
        char *xdptr = ADJUST_PTR(dptr, dsuboffsets);
        char *xsptr = ADJUST_PTR(sptr, ssuboffsets);

        copy_rec(shape+1, ndim-1, itemsize,
                 xdptr, dstrides+1, dsuboffsets ? dsuboffsets+1 : NULL,
                 xsptr, sstrides+1, ssuboffsets ? ssuboffsets+1 : NULL,
                 mem);
    }
}

static int
cmp_structure(Py_buffer *dest, Py_buffer *src)
{
    Py_ssize_t i;
    int same_fmt = ((dest->format == NULL && src->format == NULL) || \
                    (strcmp(dest->format, src->format) == 0));

    if (!same_fmt ||
        dest->itemsize != src->itemsize ||
        dest->ndim != src->ndim)
        return -1;

    for (i = 0; i < dest->ndim; i++) {
        if (dest->shape[i] != src->shape[i])
            return -1;
        if (dest->shape[i] == 0)
            break;
    }

    return 0;
}

/* Copy src to dest. Both buffers must have the same format, itemsize,
   ndim and shape. Copying is atomic, the function never fails with
   a partial copy. */
static int
copy_buffer(Py_buffer *dest, Py_buffer *src)
{
    char *mem = NULL;

    assert(dest->ndim > 0);

    if (cmp_structure(dest, src) < 0) {
        PyErr_SetString(PyExc_ValueError,
            "ndarray assignment: lvalue and rvalue have different structures");
        return -1;
    }

    if ((dest->suboffsets && dest->suboffsets[dest->ndim-1] >= 0) ||
        (src->suboffsets && src->suboffsets[src->ndim-1] >= 0) ||
        dest->strides[dest->ndim-1] != dest->itemsize ||
        src->strides[src->ndim-1] != src->itemsize) {
        mem = PyMem_Malloc(dest->shape[dest->ndim-1] * dest->itemsize);
        if (mem == NULL) {
            PyErr_NoMemory();
            return -1;
        }
    }

    copy_rec(dest->shape, dest->ndim, dest->itemsize,
             dest->buf, dest->strides, dest->suboffsets,
             src->buf, src->strides, src->suboffsets,
             mem);

    PyMem_XFree(mem);
    return 0;
}


/* Unpack single element */
static PyObject *
unpack_single(char *ptr, const char *fmt, Py_ssize_t itemsize)
{
    PyObject *x, *unpack_from, *mview;

    if (fmt == NULL) {
        fmt = "B";
        itemsize = 1;
    }

    unpack_from = PyObject_GetAttrString(structmodule, "unpack_from");
    if (unpack_from == NULL)
        return NULL;

    mview = PyMemoryView_FromMemory(ptr, itemsize, PyBUF_READ);
    if (mview == NULL) {
        Py_DECREF(unpack_from);
        return NULL;
    }

    x = PyObject_CallFunction(unpack_from, "sO", fmt, mview);
    Py_DECREF(unpack_from);
    Py_DECREF(mview);
    if (x == NULL)
        return NULL;

    if (PyTuple_GET_SIZE(x) == 1) {
        PyObject *tmp = PyTuple_GET_ITEM(x, 0);
        Py_INCREF(tmp);
        Py_DECREF(x);
        return tmp;
    }

    return x;
}

/* Unpack a multi-dimensional matrix into a nested list. Return a scalar
   for ndim = 0. */
static PyObject *
unpack_rec(PyObject *unpack_from, char *ptr, PyObject *mview, char *item,
           const Py_ssize_t *shape, const Py_ssize_t *strides,
           const Py_ssize_t *suboffsets, Py_ssize_t ndim, Py_ssize_t itemsize)
{
    PyObject *lst, *x;
    Py_ssize_t i;

    assert(ndim >= 0);
    assert(shape != NULL);
    assert(strides != NULL);

    if (ndim == 0) {
        memcpy(item, ptr, itemsize);
        x = PyObject_CallFunctionObjArgs(unpack_from, mview, NULL);
        if (x == NULL)
            return NULL;
        if (PyTuple_GET_SIZE(x) == 1) {
            PyObject *tmp = PyTuple_GET_ITEM(x, 0);
            Py_INCREF(tmp);
            Py_DECREF(x);
            return tmp;
        }
        return x;
    }

    lst = PyList_New(shape[0]);
    if (lst == NULL)
        return NULL;

    for (i = 0; i < shape[0]; ptr+=strides[0], i++) {
        char *nextptr = ADJUST_PTR(ptr, suboffsets);

        x = unpack_rec(unpack_from, nextptr, mview, item,
                       shape+1, strides+1, suboffsets ? suboffsets+1 : NULL,
                       ndim-1, itemsize);
        if (x == NULL) {
            Py_DECREF(lst);
            return NULL;
        }

        PyList_SET_ITEM(lst, i, x);
    }

    return lst;
}


static PyObject *
ndarray_as_list(NDArrayObject *nd)
{
    PyObject *structobj = NULL, *unpack_from = NULL;
    PyObject *lst = NULL, *mview = NULL;
    Py_buffer *base = &nd->head->base;
    Py_ssize_t *shape = base->shape;
    Py_ssize_t *strides = base->strides;
    Py_ssize_t simple_shape[1];
    Py_ssize_t simple_strides[1];
    char *item = NULL;
    PyObject *format;
    char *fmt = base->format;

    base = &nd->head->base;

    if (fmt == NULL) {
        PyErr_SetString(PyExc_ValueError,
            "ndarray: tolist() does not support format=NULL, use "
            "tobytes()");
        return NULL;
    }
    if (shape == NULL) {
        assert(ND_C_CONTIGUOUS(nd->head->flags));
        assert(base->strides == NULL);
        assert(base->ndim <= 1);
        shape = simple_shape;
        shape[0] = base->len;
        strides = simple_strides;
        strides[0] = base->itemsize;
    }
    else if (strides == NULL) {
        assert(ND_C_CONTIGUOUS(nd->head->flags));
        strides = strides_from_shape(nd->head, 0);
        if (strides == NULL)
            return NULL;
    }

    format = PyUnicode_FromString(fmt);
    if (format == NULL)
        goto out;

    structobj = PyObject_CallFunctionObjArgs(Struct, format, NULL);
    Py_DECREF(format);
    if (structobj == NULL)
        goto out;

    unpack_from = PyObject_GetAttrString(structobj, "unpack_from");
    if (unpack_from == NULL)
        goto out;

    item = PyMem_Malloc(base->itemsize);
    if (item == NULL) {
        PyErr_NoMemory();
        goto out;
    }

    mview = PyMemoryView_FromMemory(item, base->itemsize, PyBUF_WRITE);
    if (mview == NULL)
        goto out;

    lst = unpack_rec(unpack_from, base->buf, mview, item,
                     shape, strides, base->suboffsets,
                     base->ndim, base->itemsize);

out:
    Py_XDECREF(mview);
    PyMem_XFree(item);
    Py_XDECREF(unpack_from);
    Py_XDECREF(structobj);
    if (strides != base->strides && strides != simple_strides)
        PyMem_XFree(strides);

    return lst;
}


/****************************************************************************/
/*                            Initialize ndbuf                              */
/****************************************************************************/

/*
   State of a new ndbuf during initialization. 'OK' means that initialization
   is complete. 'PTR' means that a pointer has been initialized, but the
   state of the memory is still undefined and ndbuf->offset is disregarded.

  +-----------------+-----------+-------------+----------------+
  |                 | ndbuf_new | init_simple | init_structure |
  +-----------------+-----------+-------------+----------------+
  | next            | OK (NULL) |     OK      |       OK       |
  +-----------------+-----------+-------------+----------------+
  | prev            | OK (NULL) |     OK      |       OK       |
  +-----------------+-----------+-------------+----------------+
  | len             |    OK     |     OK      |       OK       |
  +-----------------+-----------+-------------+----------------+
  | offset          |    OK     |     OK      |       OK       |
  +-----------------+-----------+-------------+----------------+
  | data            |    PTR    |     OK      |       OK       |
  +-----------------+-----------+-------------+----------------+
  | flags           |    user   |    user     |       OK       |
  +-----------------+-----------+-------------+----------------+
  | exports         |   OK (0)  |     OK      |       OK       |
  +-----------------+-----------+-------------+----------------+
  | base.obj        | OK (NULL) |     OK      |       OK       |
  +-----------------+-----------+-------------+----------------+
  | base.buf        |    PTR    |     PTR     |       OK       |
  +-----------------+-----------+-------------+----------------+
  | base.len        | len(data) |  len(data)  |       OK       |
  +-----------------+-----------+-------------+----------------+
  | base.itemsize   |     1     |     OK      |       OK       |
  +-----------------+-----------+-------------+----------------+
  | base.readonly   |     0     |     OK      |       OK       |
  +-----------------+-----------+-------------+----------------+
  | base.format     |    NULL   |     OK      |       OK       |  
  +-----------------+-----------+-------------+----------------+
  | base.ndim       |     1     |      1      |       OK       |
  +-----------------+-----------+-------------+----------------+
  | base.shape      |    NULL   |    NULL     |       OK       |
  +-----------------+-----------+-------------+----------------+
  | base.strides    |    NULL   |    NULL     |       OK       |
  +-----------------+-----------+-------------+----------------+
  | base.suboffsets |    NULL   |    NULL     |       OK       |
  +-----------------+-----------+-------------+----------------+
  | base.internal   |    OK     |    OK       |       OK       |
  +-----------------+-----------+-------------+----------------+

*/

static Py_ssize_t
get_itemsize(PyObject *format)
{
    PyObject *tmp;
    Py_ssize_t itemsize;

    tmp = PyObject_CallFunctionObjArgs(calcsize, format, NULL);
    if (tmp == NULL)
        return -1;
    itemsize = PyLong_AsSsize_t(tmp);
    Py_DECREF(tmp);

    return itemsize;
}

static char *
get_format(PyObject *format)
{
    PyObject *tmp;
    char *fmt;

    tmp = PyUnicode_AsASCIIString(format);
    if (tmp == NULL)
        return NULL;
    fmt = PyMem_Malloc(PyBytes_GET_SIZE(tmp)+1);
    if (fmt == NULL) {
        PyErr_NoMemory();
        Py_DECREF(tmp);
        return NULL;
    }
    strcpy(fmt, PyBytes_AS_STRING(tmp));
    Py_DECREF(tmp);

    return fmt;
}

static int
init_simple(ndbuf_t *ndbuf, PyObject *items, PyObject *format,
            Py_ssize_t itemsize)
{
    PyObject *mview;
    Py_buffer *base = &ndbuf->base;
    int ret;

    mview = PyMemoryView_FromBuffer(base);
    if (mview == NULL)
        return -1;

    ret = pack_from_list(mview, items, format, itemsize);
    Py_DECREF(mview);
    if (ret < 0)
        return -1;

    base->readonly = !(ndbuf->flags & ND_WRITABLE);
    base->itemsize = itemsize;
    base->format = get_format(format);
    if (base->format == NULL)
        return -1;

    return 0;
}

static Py_ssize_t *
seq_as_ssize_array(PyObject *seq, Py_ssize_t len, int is_shape)
{
    Py_ssize_t *dest;
    Py_ssize_t x, i;

    dest = PyMem_Malloc(len * (sizeof *dest));
    if (dest == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    for (i = 0; i < len; i++) {
        PyObject *tmp = PySequence_Fast_GET_ITEM(seq, i);
        if (!PyLong_Check(tmp)) {
            PyErr_Format(PyExc_ValueError,
                "elements of %s must be integers",
                is_shape ? "shape" : "strides");
            PyMem_Free(dest);
            return NULL;
        }
        x = PyLong_AsSsize_t(tmp);
        if (PyErr_Occurred()) {
            PyMem_Free(dest);
            return NULL;
        }
        if (is_shape && x < 0) {
            PyErr_Format(PyExc_ValueError,
                "elements of shape must be integers >= 0");
            PyMem_Free(dest);
            return NULL;
        }
        dest[i] = x;
    }

    return dest;
}

static Py_ssize_t *
strides_from_shape(const ndbuf_t *ndbuf, int flags)
{
    const Py_buffer *base = &ndbuf->base;
    Py_ssize_t *s, i;

    s = PyMem_Malloc(base->ndim * (sizeof *s));
    if (s == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    if (flags & ND_FORTRAN) {
        s[0] = base->itemsize;
        for (i = 1; i < base->ndim; i++)
            s[i] = s[i-1] * base->shape[i-1];
    }
    else {
        s[base->ndim-1] = base->itemsize;
        for (i = base->ndim-2; i >= 0; i--)
            s[i] = s[i+1] * base->shape[i+1];
    }

    return s;
}

/* Bounds check:

     len := complete length of allocated memory
     offset := start of the array

     A single array element is indexed by:

       i = indices[0] * strides[0] + indices[1] * strides[1] + ...

     imin is reached when all indices[n] combined with positive strides are 0
     and all indices combined with negative strides are shape[n]-1, which is
     the maximum index for the nth dimension.

     imax is reached when all indices[n] combined with negative strides are 0
     and all indices combined with positive strides are shape[n]-1.
*/
static int
verify_structure(Py_ssize_t len, Py_ssize_t itemsize, Py_ssize_t offset,
                 const Py_ssize_t *shape, const Py_ssize_t *strides,
                 Py_ssize_t ndim)
{
    Py_ssize_t imin, imax;
    Py_ssize_t n;

    assert(ndim >= 0);

    if (ndim == 0 && (offset < 0 || offset+itemsize > len))
        goto invalid_combination;

    for (n = 0; n < ndim; n++)
        if (strides[n] % itemsize) {
            PyErr_SetString(PyExc_ValueError,
            "strides must be a multiple of itemsize");
            return -1;
        }

    for (n = 0; n < ndim; n++)
        if (shape[n] == 0)
            return 0;

    imin = imax = 0;
    for (n = 0; n < ndim; n++)
        if (strides[n] <= 0)
            imin += (shape[n]-1) * strides[n];
        else
            imax += (shape[n]-1) * strides[n];

    if (imin + offset < 0 || imax + offset + itemsize > len)
        goto invalid_combination;

    return 0;


invalid_combination:
    PyErr_SetString(PyExc_ValueError,
        "invalid combination of buffer, shape and strides");
    return -1;
}

/*
   Convert a NumPy-style array to an array using suboffsets to stride in
   the first dimension. Requirements: ndim > 0.

   Contiguous example
   ==================

     Input:
     ------
       shape      = {2, 2, 3};
       strides    = {6, 3, 1};
       suboffsets = NULL;
       data       = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
       buf        = &data[0]

     Output:
     -------
       shape      = {2, 2, 3};
       strides    = {sizeof(char *), 3, 1};
       suboffsets = {0, -1, -1};
       data       = {p1, p2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
                     |   |   ^                 ^
                     `---'---'                 |
                         |                     |
                         `---------------------'
       buf        = &data[0]

     So, in the example the input resembles the three-dimensional array
     char v[2][2][3], while the output resembles an array of two pointers
     to two-dimensional arrays: char (*v[2])[2][3].


   Non-contiguous example:
   =======================

     Input (with offset and negative strides):
     -----------------------------------------
       shape      = {2, 2, 3};
       strides    = {-6, 3, -1};
       offset     = 8
       suboffsets = NULL;
       data       = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

     Output:
     -------
       shape      = {2, 2, 3};
       strides    = {-sizeof(char *), 3, -1};
       suboffsets = {2, -1, -1};
       newdata    = {p1, p2, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
                     |   |   ^     ^           ^     ^
                     `---'---'     |           |     `- p2+suboffsets[0]
                         |         `-----------|--- p1+suboffsets[0]
                         `---------------------'
       buf        = &newdata[1]  # striding backwards over the pointers.

     suboffsets[0] is the same as the offset that one would specify if
     the two {2, 3} subarrays were created directly, hence the name.
*/
static int
init_suboffsets(ndbuf_t *ndbuf)
{
    Py_buffer *base = &ndbuf->base;
    Py_ssize_t start, step;
    Py_ssize_t imin, suboffset0;
    Py_ssize_t addsize;
    Py_ssize_t n;
    char *data;

    assert(base->ndim > 0);
    assert(base->suboffsets == NULL);

    /* Allocate new data with additional space for shape[0] pointers. */
    addsize = base->shape[0] * (sizeof (char *));

    /* Align array start to a multiple of 8. */
    addsize = 8 * ((addsize + 7) / 8);

    data = PyMem_Malloc(ndbuf->len + addsize);
    if (data == NULL) {
        PyErr_NoMemory();
        return -1;
    }

    memcpy(data + addsize, ndbuf->data, ndbuf->len);

    PyMem_Free(ndbuf->data);
    ndbuf->data = data;
    ndbuf->len += addsize;
    base->buf = ndbuf->data;

    /* imin: minimum index of the input array relative to ndbuf->offset.
       suboffset0: offset for each sub-array of the output. This is the
                   same as calculating -imin' for a sub-array of ndim-1. */
    imin = suboffset0 = 0;
    for (n = 0; n < base->ndim; n++) {
        if (base->shape[n] == 0)
            break;
        if (base->strides[n] <= 0) {
            Py_ssize_t x = (base->shape[n]-1) * base->strides[n];
            imin += x;
            suboffset0 += (n >= 1) ? -x : 0;
        }
    }

    /* Initialize the array of pointers to the sub-arrays. */
    start = addsize + ndbuf->offset + imin;
    step = base->strides[0] < 0 ? -base->strides[0] : base->strides[0];

    for (n = 0; n < base->shape[0]; n++)
        ((char **)base->buf)[n] = (char *)base->buf + start + n*step;

    /* Initialize suboffsets. */
    base->suboffsets = PyMem_Malloc(base->ndim * (sizeof *base->suboffsets));
    if (base->suboffsets == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    base->suboffsets[0] = suboffset0;
    for (n = 1; n < base->ndim; n++)
        base->suboffsets[n] = -1;

    /* Adjust strides for the first (zeroth) dimension. */
    if (base->strides[0] >= 0) {
        base->strides[0] = sizeof(char *);
    }
    else {
        /* Striding backwards. */
        base->strides[0] = -(Py_ssize_t)sizeof(char *);
        if (base->shape[0] > 0)
            base->buf = (char *)base->buf + (base->shape[0]-1) * sizeof(char *);
    }

    ndbuf->flags &= ~(ND_C|ND_FORTRAN);
    ndbuf->offset = 0;
    return 0;
}

static void
init_len(Py_buffer *base)
{
    Py_ssize_t i;

    base->len = 1;
    for (i = 0; i < base->ndim; i++)
        base->len *= base->shape[i];
    base->len *= base->itemsize;
}

static int
init_structure(ndbuf_t *ndbuf, PyObject *shape, PyObject *strides,
               Py_ssize_t ndim)
{
    Py_buffer *base = &ndbuf->base;

    base->ndim = (int)ndim;
    if (ndim == 0) {
        if (ndbuf->flags & ND_PIL) {
            PyErr_SetString(PyExc_TypeError,
                "ndim = 0 cannot be used in conjunction with ND_PIL");
            return -1;
        }
        ndbuf->flags |= (ND_SCALAR|ND_C|ND_FORTRAN);
        return 0;
    }

    /* shape */
    base->shape = seq_as_ssize_array(shape, ndim, 1);
    if (base->shape == NULL)
        return -1;

    /* strides */
    if (strides) {
        base->strides = seq_as_ssize_array(strides, ndim, 0);
    }
    else {
        base->strides = strides_from_shape(ndbuf, ndbuf->flags);
    }
    if (base->strides == NULL)
        return -1;
    if (verify_structure(base->len, base->itemsize, ndbuf->offset,
                         base->shape, base->strides, ndim) < 0)
        return -1;

    /* buf */
    base->buf = ndbuf->data + ndbuf->offset;

    /* len */
    init_len(base);

    /* ndbuf->flags */
    if (PyBuffer_IsContiguous(base, 'C'))
        ndbuf->flags |= ND_C;
    if (PyBuffer_IsContiguous(base, 'F'))
        ndbuf->flags |= ND_FORTRAN;


    /* convert numpy array to suboffset representation */
    if (ndbuf->flags & ND_PIL) {
        /* modifies base->buf, base->strides and base->suboffsets **/
        return init_suboffsets(ndbuf);
    }

    return 0;
}

static ndbuf_t *
init_ndbuf(PyObject *items, PyObject *shape, PyObject *strides,
           Py_ssize_t offset, PyObject *format, int flags)
{
    ndbuf_t *ndbuf;
    Py_ssize_t ndim;
    Py_ssize_t nitems;
    Py_ssize_t itemsize;

    /* ndim = len(shape) */
    CHECK_LIST_OR_TUPLE(shape)
    ndim = PySequence_Fast_GET_SIZE(shape);
    if (ndim > ND_MAX_NDIM) {
        PyErr_Format(PyExc_ValueError,
            "ndim must not exceed %d", ND_MAX_NDIM);
        return NULL;
    }

    /* len(strides) = len(shape) */
    if (strides) {
        CHECK_LIST_OR_TUPLE(strides)
        if (PySequence_Fast_GET_SIZE(strides) == 0)
            strides = NULL;
        else if (flags & ND_FORTRAN) {
            PyErr_SetString(PyExc_TypeError,
                "ND_FORTRAN cannot be used together with strides");
            return NULL;
        }
        else if (PySequence_Fast_GET_SIZE(strides) != ndim) {
            PyErr_SetString(PyExc_ValueError,
                "len(shape) != len(strides)");
            return NULL;
        }
    }

    /* itemsize */
    itemsize = get_itemsize(format);
    if (itemsize <= 0) {
        if (itemsize == 0) {
            PyErr_SetString(PyExc_ValueError,
                "itemsize must not be zero");
        }
        return NULL;
    }

    /* convert scalar to list */
    if (ndim == 0) {
        items = Py_BuildValue("(O)", items);
        if (items == NULL)
            return NULL;
    }
    else {
        CHECK_LIST_OR_TUPLE(items)
        Py_INCREF(items);
    }

    /* number of items */
    nitems = PySequence_Fast_GET_SIZE(items);
    if (nitems == 0) {
        PyErr_SetString(PyExc_ValueError,
            "initializer list or tuple must not be empty");
        Py_DECREF(items);
        return NULL;
    }

    ndbuf = ndbuf_new(nitems, itemsize, offset, flags);
    if (ndbuf == NULL) {
        Py_DECREF(items);
        return NULL;
    }


    if (init_simple(ndbuf, items, format, itemsize) < 0)
        goto error;
    if (init_structure(ndbuf, shape, strides, ndim) < 0)
        goto error;

    Py_DECREF(items);
    return ndbuf;

error:
    Py_DECREF(items);
    ndbuf_free(ndbuf);
    return NULL;
}

/* initialize and push a new base onto the linked list */
static int
ndarray_push_base(NDArrayObject *nd, PyObject *items,
                  PyObject *shape, PyObject *strides,
                  Py_ssize_t offset, PyObject *format, int flags)
{
    ndbuf_t *ndbuf;

    ndbuf = init_ndbuf(items, shape, strides, offset, format, flags);
    if (ndbuf == NULL)
        return -1;

    ndbuf_push(nd, ndbuf);
    return 0;
}

#define PyBUF_UNUSED 0x10000
static int
ndarray_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    NDArrayObject *nd = (NDArrayObject *)self;
    static char *kwlist[] = {
        "obj", "shape", "strides", "offset", "format", "flags", "getbuf", NULL
    };
    PyObject *v = NULL;  /* initializer: scalar, list, tuple or base object */
    PyObject *shape = NULL;   /* size of each dimension */
    PyObject *strides = NULL; /* number of bytes to the next elt in each dim */
    Py_ssize_t offset = 0;            /* buffer offset */
    PyObject *format = simple_format; /* struct module specifier: "B" */
    int flags = ND_DEFAULT;           /* base buffer and ndarray flags */

    int getbuf = PyBUF_UNUSED; /* re-exporter: getbuffer request flags */


    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|OOnOii", kwlist,
            &v, &shape, &strides, &offset, &format, &flags, &getbuf))
        return -1;

    /* NDArrayObject is re-exporter */
    if (PyObject_CheckBuffer(v) && shape == NULL) {
        if (strides || offset || format != simple_format ||
            !(flags == ND_DEFAULT || flags == ND_REDIRECT)) {
            PyErr_SetString(PyExc_TypeError,
               "construction from exporter object only takes 'obj', 'getbuf' "
               "and 'flags' arguments");
            return -1;
        }

        getbuf = (getbuf == PyBUF_UNUSED) ? PyBUF_FULL_RO : getbuf;

        if (ndarray_init_staticbuf(v, nd, getbuf) < 0)
            return -1;

        init_flags(nd->head);
        nd->head->flags |= flags;

        return 0;
    }

    /* NDArrayObject is the original base object. */
    if (getbuf != PyBUF_UNUSED) {
        PyErr_SetString(PyExc_TypeError,
            "getbuf argument only valid for construction from exporter "
            "object");
        return -1;
    }
    if (shape == NULL) {
        PyErr_SetString(PyExc_TypeError,
            "shape is a required argument when constructing from "
            "list, tuple or scalar");
        return -1;
    }

    if (flags & ND_VAREXPORT) {
        nd->flags |= ND_VAREXPORT;
        flags &= ~ND_VAREXPORT;
    }

    /* Initialize and push the first base buffer onto the linked list. */
    return ndarray_push_base(nd, v, shape, strides, offset, format, flags);
}

/* Push an additional base onto the linked list. */
static PyObject *
ndarray_push(PyObject *self, PyObject *args, PyObject *kwds)
{
    NDArrayObject *nd = (NDArrayObject *)self;
    static char *kwlist[] = {
        "items", "shape", "strides", "offset", "format", "flags", NULL
    };
    PyObject *items = NULL;   /* initializer: scalar, list or tuple */
    PyObject *shape = NULL;   /* size of each dimension */
    PyObject *strides = NULL; /* number of bytes to the next elt in each dim */
    PyObject *format = simple_format;  /* struct module specifier: "B" */
    Py_ssize_t offset = 0;             /* buffer offset */
    int flags = ND_DEFAULT;            /* base buffer flags */

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO|OnOi", kwlist,
            &items, &shape, &strides, &offset, &format, &flags))
        return NULL;

    if (flags & ND_VAREXPORT) {
        PyErr_SetString(PyExc_ValueError,
            "ND_VAREXPORT flag can only be used during object creation");
        return NULL;
    }
    if (ND_IS_CONSUMER(nd)) {
        PyErr_SetString(PyExc_BufferError,
            "structure of re-exporting object is immutable");
        return NULL;
    }
    if (!(nd->flags&ND_VAREXPORT) && nd->head->exports > 0) {
        PyErr_Format(PyExc_BufferError,
            "cannot change structure: %zd exported buffer%s",
            nd->head->exports, nd->head->exports==1 ? "" : "s");
        return NULL;
    }

    if (ndarray_push_base(nd, items, shape, strides,
                          offset, format, flags) < 0)
        return NULL;
    Py_RETURN_NONE;
}

/* Pop a base from the linked list (if possible). */
static PyObject *
ndarray_pop(PyObject *self, PyObject *dummy)
{
    NDArrayObject *nd = (NDArrayObject *)self;
    if (ND_IS_CONSUMER(nd)) {
        PyErr_SetString(PyExc_BufferError,
            "structure of re-exporting object is immutable");
        return NULL;
    }
    if (nd->head->exports > 0) {
        PyErr_Format(PyExc_BufferError,
            "cannot change structure: %zd exported buffer%s",
            nd->head->exports, nd->head->exports==1 ? "" : "s");
        return NULL;
    }
    if (nd->head->next == NULL) {
        PyErr_SetString(PyExc_BufferError,
            "list only has a single base");
        return NULL;
    }

    ndbuf_pop(nd);
    Py_RETURN_NONE;
}

/**************************************************************************/
/*                               getbuffer                                */
/**************************************************************************/

static int
ndarray_getbuf(NDArrayObject *self, Py_buffer *view, int flags)
{
    ndbuf_t *ndbuf = self->head;
    Py_buffer *base = &ndbuf->base;
    int baseflags = ndbuf->flags;

    /* redirect mode */
    if (base->obj != NULL && (baseflags&ND_REDIRECT)) {
        return PyObject_GetBuffer(base->obj, view, flags);
    }

    /* start with complete information */
    *view = *base;
    view->obj = NULL;

    /* reconstruct format */
    if (view->format == NULL)
        view->format = "B";

    if (base->ndim != 0 &&
        ((REQ_SHAPE(flags) && base->shape == NULL) ||
         (REQ_STRIDES(flags) && base->strides == NULL))) {
        /* The ndarray is a re-exporter that has been created without full
           information for testing purposes. In this particular case the
           ndarray is not a PEP-3118 compliant buffer provider. */
        PyErr_SetString(PyExc_BufferError,
            "re-exporter does not provide format, shape or strides");
        return -1;
    }

    if (baseflags & ND_GETBUF_FAIL) {
        PyErr_SetString(PyExc_BufferError,
            "ND_GETBUF_FAIL: forced test exception");
        if (baseflags & ND_GETBUF_UNDEFINED)
            view->obj = (PyObject *)0x1; /* wrong but permitted in <= 3.2 */
        return -1;
    }

    if (REQ_WRITABLE(flags) && base->readonly) {
        PyErr_SetString(PyExc_BufferError,
            "ndarray is not writable");
        return -1;
    }
    if (!REQ_FORMAT(flags)) {
        /* NULL indicates that the buffer's data type has been cast to 'B'.
           view->itemsize is the _previous_ itemsize. If shape is present,
           the equality product(shape) * itemsize = len still holds at this
           point. The equality calcsize(format) = itemsize does _not_ hold
           from here on! */
        view->format = NULL;
    }

    if (REQ_C_CONTIGUOUS(flags) && !ND_C_CONTIGUOUS(baseflags)) {
        PyErr_SetString(PyExc_BufferError,
            "ndarray is not C-contiguous");
        return -1;
    }
    if (REQ_F_CONTIGUOUS(flags) && !ND_FORTRAN_CONTIGUOUS(baseflags)) {
        PyErr_SetString(PyExc_BufferError,
            "ndarray is not Fortran contiguous");
        return -1;
    }
    if (REQ_ANY_CONTIGUOUS(flags) && !ND_ANY_CONTIGUOUS(baseflags)) {
        PyErr_SetString(PyExc_BufferError,
            "ndarray is not contiguous");
        return -1;
    }
    if (!REQ_INDIRECT(flags) && (baseflags & ND_PIL)) {
        PyErr_SetString(PyExc_BufferError,
            "ndarray cannot be represented without suboffsets");
        return -1;
    }
    if (!REQ_STRIDES(flags)) {
        if (!ND_C_CONTIGUOUS(baseflags)) {
            PyErr_SetString(PyExc_BufferError,
                "ndarray is not C-contiguous");
            return -1;
        }
        view->strides = NULL;
    }
    if (!REQ_SHAPE(flags)) {
        /* PyBUF_SIMPLE or PyBUF_WRITABLE: at this point buf is C-contiguous,
           so base->buf = ndbuf->data. */
        if (view->format != NULL) {
            /* PyBUF_SIMPLE|PyBUF_FORMAT and PyBUF_WRITABLE|PyBUF_FORMAT do
               not make sense. */
            PyErr_Format(PyExc_BufferError,
                "ndarray: cannot cast to unsigned bytes if the format flag "
                "is present");
            return -1;
        }
        /* product(shape) * itemsize = len and calcsize(format) = itemsize
           do _not_ hold from here on! */
        view->ndim = 1;
        view->shape = NULL;
    }

    view->obj = (PyObject *)self;
    Py_INCREF(view->obj);
    self->head->exports++;

    return 0;
}

static int
ndarray_releasebuf(NDArrayObject *self, Py_buffer *view)
{
    if (!ND_IS_CONSUMER(self)) {
        ndbuf_t *ndbuf = view->internal;
        if (--ndbuf->exports == 0 && ndbuf != self->head)
            ndbuf_delete(self, ndbuf);
    }

    return 0;
}

static PyBufferProcs ndarray_as_buffer = {
    (getbufferproc)ndarray_getbuf,        /* bf_getbuffer */
    (releasebufferproc)ndarray_releasebuf /* bf_releasebuffer */
};


/**************************************************************************/
/*                           indexing/slicing                             */
/**************************************************************************/

static char *
ptr_from_index(Py_buffer *base, Py_ssize_t index)
{
    char *ptr;
    Py_ssize_t nitems; /* items in the first dimension */

    if (base->shape)
        nitems = base->shape[0];
    else {
        assert(base->ndim == 1 && SIMPLE_FORMAT(base->format));
        nitems = base->len;
    }

    if (index < 0) {
        index += nitems;
    }
    if (index < 0 || index >= nitems) {
        PyErr_SetString(PyExc_IndexError, "index out of bounds");
        return NULL;
    }

    ptr = (char *)base->buf;

    if (base->strides == NULL)
         ptr += base->itemsize * index;
    else
         ptr += base->strides[0] * index;

    ptr = ADJUST_PTR(ptr, base->suboffsets);

    return ptr;
}

static PyObject *
ndarray_item(NDArrayObject *self, Py_ssize_t index)
{
    ndbuf_t *ndbuf = self->head;
    Py_buffer *base = &ndbuf->base;
    char *ptr;

    if (base->ndim == 0) {
        PyErr_SetString(PyExc_TypeError, "invalid indexing of scalar");
        return NULL;
    }

    ptr = ptr_from_index(base, index);
    if (ptr == NULL)
        return NULL;

    if (base->ndim == 1) {
        return unpack_single(ptr, base->format, base->itemsize);
    }
    else {
        NDArrayObject *nd;
        Py_buffer *subview;

        nd = (NDArrayObject *)ndarray_new(&NDArray_Type, NULL, NULL);
        if (nd == NULL)
            return NULL;

        if (ndarray_init_staticbuf((PyObject *)self, nd, PyBUF_FULL_RO) < 0) {
            Py_DECREF(nd);
            return NULL;
        }

        subview = &nd->staticbuf.base;

        subview->buf = ptr;
        subview->len /= subview->shape[0];

        subview->ndim--;
        subview->shape++;
        if (subview->strides) subview->strides++;
        if (subview->suboffsets) subview->suboffsets++;

        init_flags(&nd->staticbuf);

        return (PyObject *)nd;
    }
}

/*
  For each dimension, we get valid (start, stop, step, slicelength) quadruples
  from PySlice_GetIndicesEx().

  Slicing NumPy arrays
  ====================

    A pointer to an element in a NumPy array is defined by:

      ptr = (char *)buf + indices[0] * strides[0] +
                          ... +
                          indices[ndim-1] * strides[ndim-1]

    Adjust buf:
    -----------
      Adding start[n] for each dimension effectively adds the constant:

        c = start[0] * strides[0] + ... + start[ndim-1] * strides[ndim-1]

      Therefore init_slice() adds all start[n] directly to buf.

    Adjust shape:
    -------------
      Obviously shape[n] = slicelength[n]

    Adjust strides:
    ---------------
      In the original array, the next element in a dimension is reached
      by adding strides[n] to the pointer. In the sliced array, elements
      may be skipped, so the next element is reached by adding:

        strides[n] * step[n]

  Slicing PIL arrays
  ==================

    Layout:
    -------
      In the first (zeroth) dimension, PIL arrays have an array of pointers
      to sub-arrays of ndim-1. Striding in the first dimension is done by
      getting the index of the nth pointer, dereference it and then add a
      suboffset to it. The arrays pointed to can best be seen a regular
      NumPy arrays.

    Adjust buf:
    -----------
      In the original array, buf points to a location (usually the start)
      in the array of pointers. For the sliced array, start[0] can be
      added to buf in the same manner as for NumPy arrays.

    Adjust suboffsets:
    ------------------
      Due to the dereferencing step in the addressing scheme, it is not
      possible to adjust buf for higher dimensions. Recall that the
      sub-arrays pointed to are regular NumPy arrays, so for each of
      those arrays adding start[n] effectively adds the constant:

        c = start[1] * strides[1] + ... + start[ndim-1] * strides[ndim-1]

      This constant is added to suboffsets[0]. suboffsets[0] in turn is
      added to each pointer right after dereferencing.

    Adjust shape and strides:
    -------------------------
      Shape and strides are not influenced by the dereferencing step, so
      they are adjusted in the same manner as for NumPy arrays.

  Multiple levels of suboffsets
  =============================

      For a construct like an array of pointers to array of pointers to
      sub-arrays of ndim-2:

        suboffsets[0] = start[1] * strides[1]
        suboffsets[1] = start[2] * strides[2] + ...
*/
static int
init_slice(Py_buffer *base, PyObject *key, int dim)
{
    Py_ssize_t start, stop, step, slicelength;

    if (PySlice_GetIndicesEx(key, base->shape[dim],
                             &start, &stop, &step, &slicelength) < 0) {
        return -1;
    }


    if (base->suboffsets == NULL || dim == 0) {
    adjust_buf:
        base->buf = (char *)base->buf + base->strides[dim] * start;
    }
    else {
        Py_ssize_t n = dim-1;
        while (n >= 0 && base->suboffsets[n] < 0)
            n--;
        if (n < 0)
            goto adjust_buf; /* all suboffsets are negative */
        base->suboffsets[n] = base->suboffsets[n] + base->strides[dim] * start;
    }
    base->shape[dim] = slicelength;
    base->strides[dim] = base->strides[dim] * step;

    return 0;
}

static int
copy_structure(Py_buffer *base)
{
    Py_ssize_t *shape = NULL, *strides = NULL, *suboffsets = NULL;
    Py_ssize_t i;

    shape = PyMem_Malloc(base->ndim * (sizeof *shape));
    strides = PyMem_Malloc(base->ndim * (sizeof *strides));
    if (shape == NULL || strides == NULL)
        goto err_nomem;

    suboffsets = NULL;
    if (base->suboffsets) {
        suboffsets = PyMem_Malloc(base->ndim * (sizeof *suboffsets));
        if (suboffsets == NULL)
            goto err_nomem;
    }

    for (i = 0; i < base->ndim; i++) {
        shape[i] = base->shape[i];
        strides[i] = base->strides[i];
        if (suboffsets)
            suboffsets[i] = base->suboffsets[i];
    }

    base->shape = shape;
    base->strides = strides;
    base->suboffsets = suboffsets;

    return 0;

err_nomem:
    PyErr_NoMemory();
    PyMem_XFree(shape);
    PyMem_XFree(strides);
    PyMem_XFree(suboffsets);
    return -1;
}

static PyObject *
ndarray_subscript(NDArrayObject *self, PyObject *key)
{
    NDArrayObject *nd;
    ndbuf_t *ndbuf;
    Py_buffer *base = &self->head->base;

    if (base->ndim == 0) {
        if (PyTuple_Check(key) && PyTuple_GET_SIZE(key) == 0) {
            return unpack_single(base->buf, base->format, base->itemsize);
        }
        else if (key == Py_Ellipsis) {
            Py_INCREF(self);
            return (PyObject *)self;
        }
        else {
            PyErr_SetString(PyExc_TypeError, "invalid indexing of scalar");
            return NULL;
        }
    }
    if (PyIndex_Check(key)) {
        Py_ssize_t index = PyLong_AsSsize_t(key);
        if (index == -1 && PyErr_Occurred())
            return NULL;
        return ndarray_item(self, index);
    }

    nd = (NDArrayObject *)ndarray_new(&NDArray_Type, NULL, NULL);
    if (nd == NULL)
        return NULL;

    /* new ndarray is a consumer */
    if (ndarray_init_staticbuf((PyObject *)self, nd, PyBUF_FULL_RO) < 0) {
        Py_DECREF(nd);
        return NULL;
    }

    /* copy shape, strides and suboffsets */
    ndbuf = nd->head;
    base = &ndbuf->base;
    if (copy_structure(base) < 0) {
        Py_DECREF(nd);
        return NULL;
    }
    ndbuf->flags |= ND_OWN_ARRAYS;

    if (PySlice_Check(key)) {
        /* one-dimensional slice */
        if (init_slice(base, key, 0) < 0)
            goto err_occurred;
    }
    else if PyTuple_Check(key) {
        /* multi-dimensional slice */
        PyObject *tuple = key;
        Py_ssize_t i, n;

        n = PyTuple_GET_SIZE(tuple);
        for (i = 0; i < n; i++) {
            key = PyTuple_GET_ITEM(tuple, i);
            if (!PySlice_Check(key))
                goto type_error;
            if (init_slice(base, key, (int)i) < 0)
                goto err_occurred;
        }
    }
    else {
        goto type_error;
    }

    init_len(base);
    init_flags(ndbuf);

    return (PyObject *)nd;


type_error:
    PyErr_Format(PyExc_TypeError,
        "cannot index memory using \"%.200s\"",
        key->ob_type->tp_name);
err_occurred:
    Py_DECREF(nd);
    return NULL;
}


static int
ndarray_ass_subscript(NDArrayObject *self, PyObject *key, PyObject *value)
{
    NDArrayObject *nd;
    Py_buffer *dest = &self->head->base;
    Py_buffer src;
    char *ptr;
    Py_ssize_t index;
    int ret = -1;

    if (dest->readonly) {
        PyErr_SetString(PyExc_TypeError, "ndarray is not writable");
        return -1;
    }
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "ndarray data cannot be deleted");
        return -1;
    }
    if (dest->ndim == 0) {
        if (key == Py_Ellipsis ||
            (PyTuple_Check(key) && PyTuple_GET_SIZE(key) == 0)) {
            ptr = (char *)dest->buf;
            return pack_single(ptr, value, dest->format, dest->itemsize);
        }
        else {
            PyErr_SetString(PyExc_TypeError, "invalid indexing of scalar");
            return -1;
        }
    }
    if (dest->ndim == 1 && PyIndex_Check(key)) {
        /* rvalue must be a single item */
        index = PyLong_AsSsize_t(key);
        if (index == -1 && PyErr_Occurred())
            return -1;
        else {
            ptr = ptr_from_index(dest, index);
            if (ptr == NULL)
                return -1;
        }
        return pack_single(ptr, value, dest->format, dest->itemsize);
    }

    /* rvalue must be an exporter */
    if (PyObject_GetBuffer(value, &src, PyBUF_FULL_RO) == -1)
        return -1;

    nd = (NDArrayObject *)ndarray_subscript(self, key);
    if (nd != NULL) {
        dest = &nd->head->base;
        ret = copy_buffer(dest, &src);
        Py_DECREF(nd);
    }

    PyBuffer_Release(&src);
    return ret;
}

static PyObject *
slice_indices(PyObject *self, PyObject *args)
{
    PyObject *ret, *key, *tmp;
    Py_ssize_t s[4]; /* start, stop, step, slicelength */
    Py_ssize_t i, len;

    if (!PyArg_ParseTuple(args, "On", &key, &len)) {
        return NULL;
    }
    if (!PySlice_Check(key)) {
        PyErr_SetString(PyExc_TypeError,
            "first argument must be a slice object");
        return NULL;
    }
    if (PySlice_GetIndicesEx(key, len, &s[0], &s[1], &s[2], &s[3]) < 0) {
        return NULL;
    }

    ret = PyTuple_New(4);
    if (ret == NULL)
        return NULL;

    for (i = 0; i < 4; i++) {
        tmp = PyLong_FromSsize_t(s[i]);
        if (tmp == NULL)
            goto error;
        PyTuple_SET_ITEM(ret, i, tmp);
    }

    return ret;

error:
    Py_DECREF(ret);
    return NULL;
}


static PyMappingMethods ndarray_as_mapping = {
    NULL,                                 /* mp_length */
    (binaryfunc)ndarray_subscript,        /* mp_subscript */
    (objobjargproc)ndarray_ass_subscript  /* mp_ass_subscript */
};

static PySequenceMethods ndarray_as_sequence = {
        0,                                /* sq_length */
        0,                                /* sq_concat */
        0,                                /* sq_repeat */
        (ssizeargfunc)ndarray_item,       /* sq_item */
};


/**************************************************************************/
/*                                 getters                                */
/**************************************************************************/

static PyObject *
ssize_array_as_tuple(Py_ssize_t *array, Py_ssize_t len)
{
    PyObject *tuple, *x;
    Py_ssize_t i;

    if (array == NULL)
        return PyTuple_New(0);

    tuple = PyTuple_New(len);
    if (tuple == NULL)
        return NULL;

    for (i = 0; i < len; i++) {
        x = PyLong_FromSsize_t(array[i]);
        if (x == NULL) {
            Py_DECREF(tuple);
            return NULL;
        }
        PyTuple_SET_ITEM(tuple, i, x);
    }

    return tuple;
}

static PyObject *
ndarray_get_flags(NDArrayObject *self, void *closure)
{
    return PyLong_FromLong(self->head->flags);
}

static PyObject *
ndarray_get_offset(NDArrayObject *self, void *closure)
{
    ndbuf_t *ndbuf = self->head;
    return PyLong_FromSsize_t(ndbuf->offset);
}

static PyObject *
ndarray_get_obj(NDArrayObject *self, void *closure)
{
    Py_buffer *base = &self->head->base;

    if (base->obj == NULL) { 
        Py_RETURN_NONE;
    }
    Py_INCREF(base->obj);
    return base->obj;
}

static PyObject *
ndarray_get_nbytes(NDArrayObject *self, void *closure)
{
    Py_buffer *base = &self->head->base;
    return PyLong_FromSsize_t(base->len);
}

static PyObject *
ndarray_get_readonly(NDArrayObject *self, void *closure)
{
    Py_buffer *base = &self->head->base;
    return PyLong_FromLong(base->readonly);
}

static PyObject *
ndarray_get_itemsize(NDArrayObject *self, void *closure)
{
    Py_buffer *base = &self->head->base;
    return PyLong_FromSsize_t(base->itemsize);
}

static PyObject *
ndarray_get_format(NDArrayObject *self, void *closure)
{
    Py_buffer *base = &self->head->base;
    char *fmt = base->format ? base->format : "";
    return PyUnicode_FromString(fmt);
}

static PyObject *
ndarray_get_ndim(NDArrayObject *self, void *closure)
{
    Py_buffer *base = &self->head->base;
    return PyLong_FromSsize_t(base->ndim);
}

static PyObject *
ndarray_get_shape(NDArrayObject *self, void *closure)
{
    Py_buffer *base = &self->head->base;
    return ssize_array_as_tuple(base->shape, base->ndim);
}

static PyObject *
ndarray_get_strides(NDArrayObject *self, void *closure)
{
    Py_buffer *base = &self->head->base;
    return ssize_array_as_tuple(base->strides, base->ndim);
}

static PyObject *
ndarray_get_suboffsets(NDArrayObject *self, void *closure)
{
    Py_buffer *base = &self->head->base;
    return ssize_array_as_tuple(base->suboffsets, base->ndim);
}

static PyObject *
ndarray_c_contig(PyObject *self, PyObject *dummy)
{
    NDArrayObject *nd = (NDArrayObject *)self;
    int ret = PyBuffer_IsContiguous(&nd->head->base, 'C');

    if (ret != ND_C_CONTIGUOUS(nd->head->flags)) {
        PyErr_SetString(PyExc_RuntimeError,
            "results from PyBuffer_IsContiguous() and flags differ");
        return NULL;
    }
    return PyBool_FromLong(ret);
}

static PyObject *
ndarray_fortran_contig(PyObject *self, PyObject *dummy)
{
    NDArrayObject *nd = (NDArrayObject *)self;
    int ret = PyBuffer_IsContiguous(&nd->head->base, 'F');

    if (ret != ND_FORTRAN_CONTIGUOUS(nd->head->flags)) {
        PyErr_SetString(PyExc_RuntimeError,
            "results from PyBuffer_IsContiguous() and flags differ");
        return NULL;
    }
    return PyBool_FromLong(ret);
}

static PyObject *
ndarray_contig(PyObject *self, PyObject *dummy)
{
    NDArrayObject *nd = (NDArrayObject *)self;
    int ret = PyBuffer_IsContiguous(&nd->head->base, 'A');

    if (ret != ND_ANY_CONTIGUOUS(nd->head->flags)) {
        PyErr_SetString(PyExc_RuntimeError,
            "results from PyBuffer_IsContiguous() and flags differ");
        return NULL;
    }
    return PyBool_FromLong(ret);
}


static PyGetSetDef ndarray_getset [] =
{
  /* ndbuf */
  { "flags",        (getter)ndarray_get_flags,      NULL, NULL, NULL},
  { "offset",       (getter)ndarray_get_offset,     NULL, NULL, NULL},
  /* ndbuf.base */
  { "obj",          (getter)ndarray_get_obj,        NULL, NULL, NULL},
  { "nbytes",       (getter)ndarray_get_nbytes,     NULL, NULL, NULL},
  { "readonly",     (getter)ndarray_get_readonly,   NULL, NULL, NULL},
  { "itemsize",     (getter)ndarray_get_itemsize,   NULL, NULL, NULL},
  { "format",       (getter)ndarray_get_format,     NULL, NULL, NULL},
  { "ndim",         (getter)ndarray_get_ndim,       NULL, NULL, NULL},
  { "shape",        (getter)ndarray_get_shape,      NULL, NULL, NULL},
  { "strides",      (getter)ndarray_get_strides,    NULL, NULL, NULL},
  { "suboffsets",   (getter)ndarray_get_suboffsets, NULL, NULL, NULL},
  { "c_contiguous", (getter)ndarray_c_contig,       NULL, NULL, NULL},
  { "f_contiguous", (getter)ndarray_fortran_contig, NULL, NULL, NULL},
  { "contiguous",   (getter)ndarray_contig,         NULL, NULL, NULL},
  {NULL}
};

static PyObject *
ndarray_tolist(PyObject *self, PyObject *dummy)
{
    return ndarray_as_list((NDArrayObject *)self);
}

static PyObject *
ndarray_tobytes(PyObject *self, PyObject *dummy)
{
    ndbuf_t *ndbuf = ((NDArrayObject *)self)->head;
    Py_buffer *src = &ndbuf->base;
    Py_buffer dest;
    PyObject *ret = NULL;
    char *mem;

    if (ND_C_CONTIGUOUS(ndbuf->flags))
        return PyBytes_FromStringAndSize(src->buf, src->len);

    assert(src->shape != NULL);
    assert(src->strides != NULL);
    assert(src->ndim > 0);

    mem = PyMem_Malloc(src->len);
    if (mem == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    dest = *src;
    dest.buf = mem;
    dest.suboffsets = NULL;
    dest.strides = strides_from_shape(ndbuf, 0);
    if (dest.strides == NULL)
        goto out;
    if (copy_buffer(&dest, src) < 0)
        goto out;

    ret = PyBytes_FromStringAndSize(mem, src->len);

out:
    PyMem_XFree(dest.strides);
    PyMem_Free(mem);
    return ret;
}

/* add redundant (negative) suboffsets for testing */
static PyObject *
ndarray_add_suboffsets(PyObject *self, PyObject *dummy)
{
    NDArrayObject *nd = (NDArrayObject *)self;
    Py_buffer *base = &nd->head->base;
    Py_ssize_t i;

    if (base->suboffsets != NULL) {
        PyErr_SetString(PyExc_TypeError,
            "cannot add suboffsets to PIL-style array");
            return NULL;
    }
    if (base->strides == NULL) {
        PyErr_SetString(PyExc_TypeError,
            "cannot add suboffsets to array without strides");
            return NULL;
    }

    base->suboffsets = PyMem_Malloc(base->ndim * (sizeof *base->suboffsets));
    if (base->suboffsets == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    for (i = 0; i < base->ndim; i++)
        base->suboffsets[i] = -1;

    Py_RETURN_NONE;
}

/* Test PyMemoryView_FromBuffer(): return a memoryview from a static buffer.
   Obviously this is fragile and only one such view may be active at any
   time. Never use anything like this in real code! */
static char *infobuf = NULL;
static PyObject *
ndarray_memoryview_from_buffer(PyObject *self, PyObject *dummy)
{
    const NDArrayObject *nd = (NDArrayObject *)self;
    const Py_buffer *view = &nd->head->base;
    const ndbuf_t *ndbuf;
    static char format[ND_MAX_NDIM+1];
    static Py_ssize_t shape[ND_MAX_NDIM];
    static Py_ssize_t strides[ND_MAX_NDIM];
    static Py_ssize_t suboffsets[ND_MAX_NDIM];
    static Py_buffer info;
    char *p;

    if (!ND_IS_CONSUMER(nd))
        ndbuf = nd->head; /* self is ndarray/original exporter */
    else if (NDArray_Check(view->obj) && !ND_IS_CONSUMER(view->obj))
        /* self is ndarray and consumer from ndarray/original exporter */
        ndbuf = ((NDArrayObject *)view->obj)->head;
    else {
        PyErr_SetString(PyExc_TypeError,
        "memoryview_from_buffer(): ndarray must be original exporter or "
        "consumer from ndarray/original exporter");
         return NULL;
    }

    info = *view;
    p = PyMem_Realloc(infobuf, ndbuf->len);
    if (p == NULL) {
        PyMem_Free(infobuf);
        PyErr_NoMemory();
        infobuf = NULL;
        return NULL;
    }
    else {
        infobuf = p;
    }
    /* copy the complete raw data */
    memcpy(infobuf, ndbuf->data, ndbuf->len);
    info.buf = infobuf + ((char *)view->buf - ndbuf->data);

    if (view->format) {
        if (strlen(view->format) > ND_MAX_NDIM) {
            PyErr_Format(PyExc_TypeError,
                "memoryview_from_buffer: format is limited to %d characters",
                ND_MAX_NDIM);
                return NULL;
        }
        strcpy(format, view->format);
        info.format = format;
    }
    if (view->ndim > ND_MAX_NDIM) {
        PyErr_Format(PyExc_TypeError,
            "memoryview_from_buffer: ndim is limited to %d", ND_MAX_NDIM);
            return NULL;
    }
    if (view->shape) {
        memcpy(shape, view->shape, view->ndim * sizeof(Py_ssize_t));
        info.shape = shape;
    }
    if (view->strides) {
        memcpy(strides, view->strides, view->ndim * sizeof(Py_ssize_t));
        info.strides = strides;
    }
    if (view->suboffsets) {
        memcpy(suboffsets, view->suboffsets, view->ndim * sizeof(Py_ssize_t));
        info.suboffsets = suboffsets;
    }

    return PyMemoryView_FromBuffer(&info);
}

/* Get a single item from bufobj at the location specified by seq.
   seq is a list or tuple of indices. The purpose of this function
   is to check other functions against PyBuffer_GetPointer(). */
static PyObject *
get_pointer(PyObject *self, PyObject *args)
{
    PyObject *ret = NULL, *bufobj, *seq;
    Py_buffer view;
    Py_ssize_t indices[ND_MAX_NDIM];
    Py_ssize_t i;
    void *ptr;

    if (!PyArg_ParseTuple(args, "OO", &bufobj, &seq)) {
        return NULL;
    }

    CHECK_LIST_OR_TUPLE(seq);
    if (PyObject_GetBuffer(bufobj, &view, PyBUF_FULL_RO) < 0)
        return NULL;

    if (view.ndim > ND_MAX_NDIM) {
        PyErr_Format(PyExc_ValueError,
            "get_pointer(): ndim > %d", ND_MAX_NDIM);
        goto out;
    }
    if (PySequence_Fast_GET_SIZE(seq) != view.ndim) {
        PyErr_SetString(PyExc_ValueError,
            "get_pointer(): len(indices) != ndim");
        goto out;
    }

    for (i = 0; i < view.ndim; i++) {
        PyObject *x = PySequence_Fast_GET_ITEM(seq, i);
        indices[i] = PyLong_AsSsize_t(x);
        if (PyErr_Occurred())
            goto out;
        if (indices[i] < 0 || indices[i] >= view.shape[i]) {
            PyErr_Format(PyExc_ValueError,
                "get_pointer(): invalid index %zd at position %zd",
                indices[i], i);
            goto out;
        }
    }

    ptr = PyBuffer_GetPointer(&view, indices);
    ret = unpack_single(ptr, view.format, view.itemsize);

out:
    PyBuffer_Release(&view);
    return ret;
}

static PyObject *
get_sizeof_void_p(PyObject *self)
{
    return PyLong_FromSize_t(sizeof(void *));
}

static char
get_ascii_order(PyObject *order)
{
    PyObject *ascii_order;
    char ord;

    if (!PyUnicode_Check(order)) {
        PyErr_SetString(PyExc_TypeError,
            "order must be a string");
        return CHAR_MAX;
    }

    ascii_order = PyUnicode_AsASCIIString(order);
    if (ascii_order == NULL) {
        return CHAR_MAX;
    }

    ord = PyBytes_AS_STRING(ascii_order)[0];
    Py_DECREF(ascii_order);
    return ord;
}

/* Get a contiguous memoryview. */
static PyObject *
get_contiguous(PyObject *self, PyObject *args)
{
    PyObject *obj;
    PyObject *buffertype;
    PyObject *order;
    long type;
    char ord;

    if (!PyArg_ParseTuple(args, "OOO", &obj, &buffertype, &order)) {
        return NULL;
    }

    if (!PyLong_Check(buffertype)) {
        PyErr_SetString(PyExc_TypeError,
            "buffertype must be PyBUF_READ or PyBUF_WRITE");
        return NULL;
    }
    type = PyLong_AsLong(buffertype);
    if (type == -1 && PyErr_Occurred()) {
        return NULL;
    }

    ord = get_ascii_order(order);
    if (ord == CHAR_MAX) {
        return NULL;
    }

    return PyMemoryView_GetContiguous(obj, (int)type, ord);
}

static int
fmtcmp(const char *fmt1, const char *fmt2)
{
    if (fmt1 == NULL) {
        return fmt2 == NULL || strcmp(fmt2, "B") == 0;
    }
    if (fmt2 == NULL) {
        return fmt1 == NULL || strcmp(fmt1, "B") == 0;
    }
    return strcmp(fmt1, fmt2) == 0;
}

static int
arraycmp(const Py_ssize_t *a1, const Py_ssize_t *a2, const Py_ssize_t *shape,
         Py_ssize_t ndim)
{
    Py_ssize_t i;

    if (ndim == 1 && shape && shape[0] == 1) {
        /* This is for comparing strides: For example, the array
           [175], shape=[1], strides=[-5] is considered contiguous. */
        return 1;
    }

    for (i = 0; i < ndim; i++) {
        if (a1[i] != a2[i]) {
            return 0;
        }
    }

    return 1;
}

/* Compare two contiguous buffers for physical equality. */
static PyObject *
cmp_contig(PyObject *self, PyObject *args)
{
    PyObject *b1, *b2; /* buffer objects */
    Py_buffer v1, v2;
    PyObject *ret;
    int equal = 0;

    if (!PyArg_ParseTuple(args, "OO", &b1, &b2)) {
        return NULL;
    }

    if (PyObject_GetBuffer(b1, &v1, PyBUF_FULL_RO) < 0) {
        PyErr_SetString(PyExc_TypeError,
            "cmp_contig: first argument does not implement the buffer "
            "protocol");
        return NULL;
    }
    if (PyObject_GetBuffer(b2, &v2, PyBUF_FULL_RO) < 0) {
        PyErr_SetString(PyExc_TypeError,
            "cmp_contig: second argument does not implement the buffer "
            "protocol");
        PyBuffer_Release(&v1);
        return NULL;
    }

    if (!(PyBuffer_IsContiguous(&v1, 'C')&&PyBuffer_IsContiguous(&v2, 'C')) &&
        !(PyBuffer_IsContiguous(&v1, 'F')&&PyBuffer_IsContiguous(&v2, 'F'))) {
        goto result;
    }

    /* readonly may differ if created from non-contiguous */
    if (v1.len != v2.len ||
        v1.itemsize != v2.itemsize ||
        v1.ndim != v2.ndim ||
        !fmtcmp(v1.format, v2.format) ||
        !!v1.shape != !!v2.shape ||
        !!v1.strides != !!v2.strides ||
        !!v1.suboffsets != !!v2.suboffsets) {
        goto result;
    }

    if ((v1.shape && !arraycmp(v1.shape, v2.shape, NULL, v1.ndim)) ||
        (v1.strides && !arraycmp(v1.strides, v2.strides, v1.shape, v1.ndim)) ||
        (v1.suboffsets && !arraycmp(v1.suboffsets, v2.suboffsets, NULL,
                                    v1.ndim))) {
        goto result;
    }

    if (memcmp((char *)v1.buf, (char *)v2.buf, v1.len) != 0) {
        goto result;
    }

    equal = 1;

result:
    PyBuffer_Release(&v1);
    PyBuffer_Release(&v2);

    ret = equal ? Py_True : Py_False; 
    Py_INCREF(ret);
    return ret;
}

static PyObject *
is_contiguous(PyObject *self, PyObject *args)
{
    PyObject *obj;
    PyObject *order;
    PyObject *ret = NULL;
    Py_buffer view;
    char ord;

    if (!PyArg_ParseTuple(args, "OO", &obj, &order)) {
        return NULL;
    }

    if (PyObject_GetBuffer(obj, &view, PyBUF_FULL_RO) < 0) {
        PyErr_SetString(PyExc_TypeError,
            "is_contiguous: object does not implement the buffer "
            "protocol");
        return NULL;
    }

    ord = get_ascii_order(order);
    if (ord == CHAR_MAX) {
        goto release;
    }

    ret = PyBuffer_IsContiguous(&view, ord) ? Py_True : Py_False;
    Py_INCREF(ret);

release:
    PyBuffer_Release(&view);
    return ret;
}

static Py_hash_t
ndarray_hash(PyObject *self)
{
    const NDArrayObject *nd = (NDArrayObject *)self;
    const Py_buffer *view = &nd->head->base;
    PyObject *bytes;
    Py_hash_t hash;

    if (!view->readonly) {
         PyErr_SetString(PyExc_ValueError,
             "cannot hash writable ndarray object");
         return -1;
    }
    if (view->obj != NULL && PyObject_Hash(view->obj) == -1) {
         return -1;
    }

    bytes = ndarray_tobytes(self, NULL);
    if (bytes == NULL) {
        return -1;
    }

    hash = PyObject_Hash(bytes);
    Py_DECREF(bytes);
    return hash;
}


static PyMethodDef ndarray_methods [] =
{
    { "tolist", ndarray_tolist, METH_NOARGS, NULL },
    { "tobytes", ndarray_tobytes, METH_NOARGS, NULL },
    { "push", (PyCFunction)ndarray_push, METH_VARARGS|METH_KEYWORDS, NULL },
    { "pop", ndarray_pop, METH_NOARGS, NULL },
    { "add_suboffsets", ndarray_add_suboffsets, METH_NOARGS, NULL },
    { "memoryview_from_buffer", ndarray_memoryview_from_buffer, METH_NOARGS, NULL },
    {NULL}
};

static PyTypeObject NDArray_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "ndarray",                   /* Name of this type */
    sizeof(NDArrayObject),       /* Basic object size */
    0,                           /* Item size for varobject */
    (destructor)ndarray_dealloc, /* tp_dealloc */
    0,                           /* tp_print */
    0,                           /* tp_getattr */
    0,                           /* tp_setattr */
    0,                           /* tp_compare */
    0,                           /* tp_repr */
    0,                           /* tp_as_number */
    &ndarray_as_sequence,        /* tp_as_sequence */
    &ndarray_as_mapping,         /* tp_as_mapping */
    (hashfunc)ndarray_hash,      /* tp_hash */
    0,                           /* tp_call */
    0,                           /* tp_str */
    PyObject_GenericGetAttr,     /* tp_getattro */
    0,                           /* tp_setattro */
    &ndarray_as_buffer,          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,          /* tp_flags */
    0,                           /* tp_doc */
    0,                           /* tp_traverse */
    0,                           /* tp_clear */
    0,                           /* tp_richcompare */
    0,                           /* tp_weaklistoffset */
    0,                           /* tp_iter */
    0,                           /* tp_iternext */
    ndarray_methods,             /* tp_methods */
    0,                           /* tp_members */
    ndarray_getset,              /* tp_getset */
    0,                           /* tp_base */
    0,                           /* tp_dict */
    0,                           /* tp_descr_get */
    0,                           /* tp_descr_set */
    0,                           /* tp_dictoffset */
    ndarray_init,                /* tp_init */
    0,                           /* tp_alloc */
    ndarray_new,                 /* tp_new */
};

/**************************************************************************/
/*                          StaticArray Object                            */
/**************************************************************************/

static PyTypeObject StaticArray_Type;

typedef struct {
    PyObject_HEAD
    int legacy_mode; /* if true, use the view.obj==NULL hack */
} StaticArrayObject;

static char static_mem[12] = {0,1,2,3,4,5,6,7,8,9,10,11};
static Py_ssize_t static_shape[1] = {12};
static Py_ssize_t static_strides[1] = {1};
static Py_buffer static_buffer = {
    static_mem,     /* buf */
    NULL,           /* obj */
    12,             /* len */
    1,              /* itemsize */
    1,              /* readonly */
    1,              /* ndim */
    "B",            /* format */
    static_shape,   /* shape */
    static_strides, /* strides */
    NULL,           /* suboffsets */
    NULL            /* internal */
};

static PyObject *
staticarray_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    return (PyObject *)PyObject_New(StaticArrayObject, &StaticArray_Type);
}

static int
staticarray_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    StaticArrayObject *a = (StaticArrayObject *)self;
    static char *kwlist[] = {
        "legacy_mode", NULL
    };
    PyObject *legacy_mode = Py_False;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O", kwlist, &legacy_mode))
        return -1;

    a->legacy_mode = (legacy_mode != Py_False);
    return 0;
}

static void
staticarray_dealloc(StaticArrayObject *self)
{
    PyObject_Del(self);
}

/* Return a buffer for a PyBUF_FULL_RO request. Flags are not checked,
   which makes this object a non-compliant exporter! */
static int
staticarray_getbuf(StaticArrayObject *self, Py_buffer *view, int flags)
{
    *view = static_buffer;

    if (self->legacy_mode) {
        view->obj = NULL; /* Don't use this in new code. */
    }
    else {
        view->obj = (PyObject *)self;
        Py_INCREF(view->obj);
    }

    return 0;
}

static PyBufferProcs staticarray_as_buffer = {
    (getbufferproc)staticarray_getbuf, /* bf_getbuffer */
    NULL,                              /* bf_releasebuffer */
};

static PyTypeObject StaticArray_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "staticarray",                   /* Name of this type */
    sizeof(StaticArrayObject),       /* Basic object size */
    0,                               /* Item size for varobject */
    (destructor)staticarray_dealloc, /* tp_dealloc */
    0,                               /* tp_print */
    0,                               /* tp_getattr */
    0,                               /* tp_setattr */
    0,                               /* tp_compare */
    0,                               /* tp_repr */
    0,                               /* tp_as_number */
    0,                               /* tp_as_sequence */
    0,                               /* tp_as_mapping */
    0,                               /* tp_hash */
    0,                               /* tp_call */
    0,                               /* tp_str */
    0,                               /* tp_getattro */
    0,                               /* tp_setattro */
    &staticarray_as_buffer,          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,              /* tp_flags */
    0,                               /* tp_doc */
    0,                               /* tp_traverse */
    0,                               /* tp_clear */
    0,                               /* tp_richcompare */
    0,                               /* tp_weaklistoffset */
    0,                               /* tp_iter */
    0,                               /* tp_iternext */
    0,                               /* tp_methods */
    0,                               /* tp_members */
    0,                               /* tp_getset */
    0,                               /* tp_base */
    0,                               /* tp_dict */
    0,                               /* tp_descr_get */
    0,                               /* tp_descr_set */
    0,                               /* tp_dictoffset */
    staticarray_init,                /* tp_init */
    0,                               /* tp_alloc */
    staticarray_new,                 /* tp_new */
};


static struct PyMethodDef _testbuffer_functions[] = {
    {"slice_indices", slice_indices, METH_VARARGS, NULL},
    {"get_pointer", get_pointer, METH_VARARGS, NULL},
    {"get_sizeof_void_p", (PyCFunction)get_sizeof_void_p, METH_NOARGS, NULL},
    {"get_contiguous", get_contiguous, METH_VARARGS, NULL},
    {"is_contiguous", is_contiguous, METH_VARARGS, NULL},
    {"cmp_contig", cmp_contig, METH_VARARGS, NULL},
    {NULL, NULL}
};

static struct PyModuleDef _testbuffermodule = {
    PyModuleDef_HEAD_INIT,
    "_testbuffer",
    NULL,
    -1,
    _testbuffer_functions,
    NULL,
    NULL,
    NULL,
    NULL
};


PyMODINIT_FUNC
PyInit__testbuffer(void)
{
    PyObject *m;

    m = PyModule_Create(&_testbuffermodule);
    if (m == NULL)
        return NULL;

    Py_TYPE(&NDArray_Type) = &PyType_Type;
    Py_INCREF(&NDArray_Type);
    PyModule_AddObject(m, "ndarray", (PyObject *)&NDArray_Type);

    Py_TYPE(&StaticArray_Type) = &PyType_Type;
    Py_INCREF(&StaticArray_Type);
    PyModule_AddObject(m, "staticarray", (PyObject *)&StaticArray_Type);

    structmodule = PyImport_ImportModule("struct");
    if (structmodule == NULL)
        return NULL;

    Struct = PyObject_GetAttrString(structmodule, "Struct");
    calcsize = PyObject_GetAttrString(structmodule, "calcsize");
    if (Struct == NULL || calcsize == NULL)
        return NULL;

    simple_format = PyUnicode_FromString(simple_fmt);
    if (simple_format == NULL)
        return NULL;

    PyModule_AddIntConstant(m, "ND_MAX_NDIM", ND_MAX_NDIM);
    PyModule_AddIntConstant(m, "ND_VAREXPORT", ND_VAREXPORT);
    PyModule_AddIntConstant(m, "ND_WRITABLE", ND_WRITABLE);
    PyModule_AddIntConstant(m, "ND_FORTRAN", ND_FORTRAN);
    PyModule_AddIntConstant(m, "ND_SCALAR", ND_SCALAR);
    PyModule_AddIntConstant(m, "ND_PIL", ND_PIL);
    PyModule_AddIntConstant(m, "ND_GETBUF_FAIL", ND_GETBUF_FAIL);
    PyModule_AddIntConstant(m, "ND_GETBUF_UNDEFINED", ND_GETBUF_UNDEFINED);
    PyModule_AddIntConstant(m, "ND_REDIRECT", ND_REDIRECT);

    PyModule_AddIntConstant(m, "PyBUF_SIMPLE", PyBUF_SIMPLE);
    PyModule_AddIntConstant(m, "PyBUF_WRITABLE", PyBUF_WRITABLE);
    PyModule_AddIntConstant(m, "PyBUF_FORMAT", PyBUF_FORMAT);
    PyModule_AddIntConstant(m, "PyBUF_ND", PyBUF_ND);
    PyModule_AddIntConstant(m, "PyBUF_STRIDES", PyBUF_STRIDES);
    PyModule_AddIntConstant(m, "PyBUF_INDIRECT", PyBUF_INDIRECT);
    PyModule_AddIntConstant(m, "PyBUF_C_CONTIGUOUS", PyBUF_C_CONTIGUOUS);
    PyModule_AddIntConstant(m, "PyBUF_F_CONTIGUOUS", PyBUF_F_CONTIGUOUS);
    PyModule_AddIntConstant(m, "PyBUF_ANY_CONTIGUOUS", PyBUF_ANY_CONTIGUOUS);
    PyModule_AddIntConstant(m, "PyBUF_FULL", PyBUF_FULL);
    PyModule_AddIntConstant(m, "PyBUF_FULL_RO", PyBUF_FULL_RO);
    PyModule_AddIntConstant(m, "PyBUF_RECORDS", PyBUF_RECORDS);
    PyModule_AddIntConstant(m, "PyBUF_RECORDS_RO", PyBUF_RECORDS_RO);
    PyModule_AddIntConstant(m, "PyBUF_STRIDED", PyBUF_STRIDED);
    PyModule_AddIntConstant(m, "PyBUF_STRIDED_RO", PyBUF_STRIDED_RO);
    PyModule_AddIntConstant(m, "PyBUF_CONTIG", PyBUF_CONTIG);
    PyModule_AddIntConstant(m, "PyBUF_CONTIG_RO", PyBUF_CONTIG_RO);

    PyModule_AddIntConstant(m, "PyBUF_READ", PyBUF_READ);
    PyModule_AddIntConstant(m, "PyBUF_WRITE", PyBUF_WRITE);

    return m;
}



