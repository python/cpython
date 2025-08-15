/* Low level interface to the Zstandard algorithm & the zstd library. */

/* ZstdDict class definitions */

/*[clinic input]
module _zstd
class _zstd.ZstdDict "ZstdDict *" "&zstd_dict_type_spec"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=3dcc175ec974f81c]*/

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"

#include "_zstdmodule.h"
#include "clinic/zstddict.c.h"
#include "internal/pycore_lock.h" // PyMutex_IsLocked

#include <zstd.h>                 // ZSTD_freeDDict(), ZSTD_getDictID_fromDict()

#define ZstdDict_CAST(op) ((ZstdDict *)op)

/*[clinic input]
@classmethod
_zstd.ZstdDict.__new__ as _zstd_ZstdDict_new
    dict_content: Py_buffer
        The content of a Zstandard dictionary as a bytes-like object.
    /
    *
    is_raw: bool = False
        If true, perform no checks on *dict_content*, useful for some
        advanced cases. Otherwise, check that the content represents
        a Zstandard dictionary created by the zstd library or CLI.

Represents a Zstandard dictionary.

The dictionary can be used for compression or decompression, and can be shared
by multiple ZstdCompressor or ZstdDecompressor objects.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdDict_new_impl(PyTypeObject *type, Py_buffer *dict_content,
                        int is_raw)
/*[clinic end generated code: output=685b7406a48b0949 input=9e8c493e31c98383]*/
{
    /* All dictionaries must be at least 8 bytes */
    if (dict_content->len < 8) {
        PyErr_SetString(PyExc_ValueError,
                        "Zstandard dictionary content too short "
                        "(must have at least eight bytes)");
        return NULL;
    }

    ZstdDict* self = PyObject_GC_New(ZstdDict, type);
    if (self == NULL) {
        return NULL;
    }

    self->d_dict = NULL;
    self->dict_buffer = NULL;
    self->dict_id = 0;
    self->lock = (PyMutex){0};

    /* ZSTD_CDict dict */
    self->c_dicts = PyDict_New();
    if (self->c_dicts == NULL) {
        goto error;
    }

    self->dict_buffer = PyMem_Malloc(dict_content->len);
    if (!self->dict_buffer) {
        PyErr_NoMemory();
        goto error;
    }
    memcpy(self->dict_buffer, dict_content->buf, dict_content->len);
    self->dict_len = dict_content->len;

    /* Get dict_id, 0 means "raw content" dictionary. */
    self->dict_id = ZSTD_getDictID_fromDict(self->dict_buffer, self->dict_len);

    /* Check validity for ordinary dictionary */
    if (!is_raw && self->dict_id == 0) {
        PyErr_SetString(PyExc_ValueError, "invalid Zstandard dictionary");
        goto error;
    }

    PyObject_GC_Track(self);

    return (PyObject *)self;

error:
    Py_XDECREF(self);
    return NULL;
}

static void
ZstdDict_dealloc(PyObject *ob)
{
    ZstdDict *self = ZstdDict_CAST(ob);

    PyObject_GC_UnTrack(self);

    /* Free ZSTD_DDict instance */
    if (self->d_dict) {
        ZSTD_freeDDict(self->d_dict);
    }

    assert(!PyMutex_IsLocked(&self->lock));

    /* Release dict_buffer after freeing ZSTD_CDict/ZSTD_DDict instances */
    PyMem_Free(self->dict_buffer);
    Py_CLEAR(self->c_dicts);

    PyTypeObject *tp = Py_TYPE(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

PyDoc_STRVAR(ZstdDict_dictid_doc,
"The Zstandard dictionary, an int between 0 and 2**32.\n\n"
"A non-zero value represents an ordinary Zstandard dictionary,\n"
"conforming to the standardised format.\n\n"
"A value of zero indicates a 'raw content' dictionary,\n"
"without any restrictions on format or content.");

static PyObject *
ZstdDict_repr(PyObject *ob)
{
    ZstdDict *dict = ZstdDict_CAST(ob);
    return PyUnicode_FromFormat("<ZstdDict dict_id=%u dict_size=%zd>",
                                (unsigned int)dict->dict_id, dict->dict_len);
}

static PyMemberDef ZstdDict_members[] = {
    {"dict_id", Py_T_UINT, offsetof(ZstdDict, dict_id), Py_READONLY, ZstdDict_dictid_doc},
    {NULL}
};

/*[clinic input]
@getter
_zstd.ZstdDict.dict_content

The content of a Zstandard dictionary, as a bytes object.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdDict_dict_content_get_impl(ZstdDict *self)
/*[clinic end generated code: output=0d05caa5b550eabb input=4ed526d1c151c596]*/
{
    return PyBytes_FromStringAndSize(self->dict_buffer, self->dict_len);
}

/*[clinic input]
@getter
_zstd.ZstdDict.as_digested_dict

Load as a digested dictionary to compressor.

Pass this attribute as zstd_dict argument:
compress(dat, zstd_dict=zd.as_digested_dict)

1. Some advanced compression parameters of compressor may be overridden
   by parameters of digested dictionary.
2. ZstdDict has a digested dictionaries cache for each compression level.
   It's faster when loading again a digested dictionary with the same
   compression level.
3. No need to use this for decompression.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdDict_as_digested_dict_get_impl(ZstdDict *self)
/*[clinic end generated code: output=09b086e7a7320dbb input=ee45e1b4a48f6f2c]*/
{
    return Py_BuildValue("Oi", self, DICT_TYPE_DIGESTED);
}

/*[clinic input]
@getter
_zstd.ZstdDict.as_undigested_dict

Load as an undigested dictionary to compressor.

Pass this attribute as zstd_dict argument:
compress(dat, zstd_dict=zd.as_undigested_dict)

1. The advanced compression parameters of compressor will not be overridden.
2. Loading an undigested dictionary is costly. If load an undigested dictionary
   multiple times, consider reusing a compressor object.
3. No need to use this for decompression.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdDict_as_undigested_dict_get_impl(ZstdDict *self)
/*[clinic end generated code: output=43c7a989e6d4253a input=d39210eedec76fed]*/
{
    return Py_BuildValue("Oi", self, DICT_TYPE_UNDIGESTED);
}

/*[clinic input]
@getter
_zstd.ZstdDict.as_prefix

Load as a prefix to compressor/decompressor.

Pass this attribute as zstd_dict argument:
compress(dat, zstd_dict=zd.as_prefix)

1. Prefix is compatible with long distance matching, while dictionary is not.
2. It only works for the first frame, then the compressor/decompressor will
   return to no prefix state.
3. When decompressing, must use the same prefix as when compressing.
[clinic start generated code]*/

static PyObject *
_zstd_ZstdDict_as_prefix_get_impl(ZstdDict *self)
/*[clinic end generated code: output=6f7130c356595a16 input=45b3b6110f36d127]*/
{
    return Py_BuildValue("Oi", self, DICT_TYPE_PREFIX);
}

static PyGetSetDef ZstdDict_getset[] = {
    _ZSTD_ZSTDDICT_DICT_CONTENT_GETSETDEF
    _ZSTD_ZSTDDICT_AS_DIGESTED_DICT_GETSETDEF
    _ZSTD_ZSTDDICT_AS_UNDIGESTED_DICT_GETSETDEF
    _ZSTD_ZSTDDICT_AS_PREFIX_GETSETDEF
    {NULL}
};

static Py_ssize_t
ZstdDict_length(PyObject *ob)
{
    ZstdDict *self = ZstdDict_CAST(ob);
    return self->dict_len;
}

static int
ZstdDict_traverse(PyObject *ob, visitproc visit, void *arg)
{
    ZstdDict *self = ZstdDict_CAST(ob);
    Py_VISIT(self->c_dicts);
    return 0;
}

static int
ZstdDict_clear(PyObject *ob)
{
    ZstdDict *self = ZstdDict_CAST(ob);
    Py_CLEAR(self->c_dicts);
    return 0;
}

static PyType_Slot zstddict_slots[] = {
    {Py_tp_members, ZstdDict_members},
    {Py_tp_getset, ZstdDict_getset},
    {Py_tp_new, _zstd_ZstdDict_new},
    {Py_tp_dealloc, ZstdDict_dealloc},
    {Py_tp_repr, ZstdDict_repr},
    {Py_tp_doc, (void *)_zstd_ZstdDict_new__doc__},
    {Py_sq_length, ZstdDict_length},
    {Py_tp_traverse, ZstdDict_traverse},
    {Py_tp_clear, ZstdDict_clear},
    {0, 0}
};

PyType_Spec zstd_dict_type_spec = {
    .name = "compression.zstd.ZstdDict",
    .basicsize = sizeof(ZstdDict),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE
             | Py_TPFLAGS_HAVE_GC,
    .slots = zstddict_slots,
};
