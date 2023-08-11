#include <stddef.h>               // ptrdiff_t

#include "parts.h"
#include "clinic/dict.c.h"


/*[clinic input]
module _testcapi
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=6361033e795369fc]*/

#define NULLABLE(x) do { if (x == Py_None) x = NULL; } while (0);

#define RETURN_INT(value) do {          \
        int _ret = (value);             \
        if (_ret == -1) {               \
            return NULL;                \
        }                               \
        return PyLong_FromLong(_ret);   \
    } while (0)

#define RETURN_SIZE(value) do {             \
        Py_ssize_t _ret = (value);          \
        if (_ret == -1) {                   \
            return NULL;                    \
        }                                   \
        return PyLong_FromSsize_t(_ret);    \
    } while (0)


/*[clinic input]
_testcapi.dict_check
    obj: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_check(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=db5de1afc9b8e1e5 input=099d8ccf1c8e38b8]*/
{
    NULLABLE(obj);
    return PyLong_FromLong(PyDict_Check(obj));
}


/*[clinic input]
_testcapi.dict_checkexact
    obj: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_checkexact(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=a53a2f2b969ce16c input=3b06ddc29bf6276f]*/
{
    NULLABLE(obj);
    return PyLong_FromLong(PyDict_CheckExact(obj));
}


/*[clinic input]
_testcapi.dict_new
[clinic start generated code]*/

static PyObject *
_testcapi_dict_new_impl(PyObject *module)
/*[clinic end generated code: output=22fafce00b067bd1 input=b4b463dfb073824f]*/
{
    return PyDict_New();
}


/*[clinic input]
_testcapi.dictproxy_new
    obj: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dictproxy_new(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=f1cd75e4f9772ffb input=19b081e487df3067]*/
{
    NULLABLE(obj);
    return PyDictProxy_New(obj);
}


/*[clinic input]
_testcapi.dict_clear
    obj: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_clear(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=f3ae3c6547b16dad input=bb427bbf5d143860]*/
{
    PyDict_Clear(obj);
    Py_RETURN_NONE;
}


/*[clinic input]
_testcapi.dict_copy
    obj: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_copy(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=b6b7df7c11cb1a4c input=3e8300ceb63995f0]*/
{
    NULLABLE(obj);
    return PyDict_Copy(obj);
}


/*[clinic input]
_testcapi.dict_contains
    obj: object
    key: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_contains_impl(PyObject *module, PyObject *obj, PyObject *key)
/*[clinic end generated code: output=d29efe25b3d9cb07 input=581c486b6c9502a5]*/
{
    NULLABLE(obj);
    NULLABLE(key);
    RETURN_INT(PyDict_Contains(obj, key));
}


/*[clinic input]
_testcapi.dict_size
    obj: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_size(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=3ef99b229e9c3b93 input=d1a87dff92a98060]*/
{
    NULLABLE(obj);
    RETURN_SIZE(PyDict_Size(obj));
}


/*[clinic input]
_testcapi.dict_getitem
    mapping: object
    key: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_getitem_impl(PyObject *module, PyObject *mapping,
                            PyObject *key)
/*[clinic end generated code: output=8656b2737d0ad225 input=c4b843cc6a837905]*/
{
    NULLABLE(mapping);
    NULLABLE(key);
    PyObject *value = PyDict_GetItem(mapping, key);
    if (value == NULL) {
        if (PyErr_Occurred()) {
            return NULL;
        }
        return Py_NewRef(PyExc_KeyError);
    }
    return Py_NewRef(value);
}


/*[clinic input]
_testcapi.dict_getitemstring
    mapping: object
    key: str(accept={robuffer}, zeroes=True)
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_getitemstring_impl(PyObject *module, PyObject *mapping,
                                  const char *key, Py_ssize_t key_length)
/*[clinic end generated code: output=13ef172dc70898df input=777403906725d839]*/
{
    NULLABLE(mapping);
    PyObject *value = PyDict_GetItemString(mapping, key);
    if (value == NULL) {
        if (PyErr_Occurred()) {
            return NULL;
        }
        return Py_NewRef(PyExc_KeyError);
    }
    return Py_NewRef(value);
}


/*[clinic input]
_testcapi.dict_getitemwitherror
    mapping: object
    key: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_getitemwitherror_impl(PyObject *module, PyObject *mapping,
                                     PyObject *key)
/*[clinic end generated code: output=8673d9881daf90cf input=3967cd021c1c9445]*/
{
    NULLABLE(mapping);
    NULLABLE(key);
    PyObject *value = PyDict_GetItemWithError(mapping, key);
    if (value == NULL) {
        if (PyErr_Occurred()) {
            return NULL;
        }
        return Py_NewRef(PyExc_KeyError);
    }
    return Py_NewRef(value);
}


/*[clinic input]
_testcapi.dict_getitemref
    obj: object
    attr_name: object
    value: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_getitemref_impl(PyObject *module, PyObject *obj,
                               PyObject *attr_name, PyObject *value)
/*[clinic end generated code: output=ba81181c4916a6a1 input=9871862311c904dd]*/
{
    NULLABLE(obj);
    NULLABLE(attr_name);

    switch (PyDict_GetItemRef(obj, attr_name, &value)) {
        case -1:
            assert(value == NULL);
            return NULL;
        case 0:
            assert(value == NULL);
            return Py_NewRef(PyExc_KeyError);
        case 1:
            return value;
        default:
            Py_FatalError("PyMapping_GetItemRef() returned invalid code");
            Py_UNREACHABLE();
    }
}


/*[clinic input]
_testcapi.dict_getitemstringref
    obj: object
    value: object
    attr_name: str(accept={robuffer}, zeroes=True)
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_getitemstringref_impl(PyObject *module, PyObject *obj,
                                     PyObject *value, const char *attr_name,
                                     Py_ssize_t attr_name_length)
/*[clinic end generated code: output=22f68fcfb2d0e65e input=3613695804310a2e]*/
{
    NULLABLE(obj);

    switch (PyDict_GetItemStringRef(obj, attr_name, &value)) {
        case -1:
            assert(value == NULL);
            return NULL;
        case 0:
            assert(value == NULL);
            return Py_NewRef(PyExc_KeyError);
        case 1:
            return value;
        default:
            Py_FatalError("PyDict_GetItemStringRef() returned invalid code");
            Py_UNREACHABLE();
    }
}


/*[clinic input]
_testcapi.dict_setitem
    mapping: object
    key: object
    value: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_setitem_impl(PyObject *module, PyObject *mapping,
                            PyObject *key, PyObject *value)
/*[clinic end generated code: output=0870405a87c747cf input=b99779c745fd9397]*/
{
    NULLABLE(mapping);
    NULLABLE(key);
    NULLABLE(value);
    RETURN_INT(PyDict_SetItem(mapping, key, value));
}


/*[clinic input]
_testcapi.dict_setitemstring
    mapping: object
    value: object
    key: str(accept={robuffer}, zeroes=True)
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_setitemstring_impl(PyObject *module, PyObject *mapping,
                                  PyObject *value, const char *key,
                                  Py_ssize_t key_length)
/*[clinic end generated code: output=65cdbcf61f17b87c input=4d5e4ea66f092051]*/
{
    NULLABLE(mapping);
    NULLABLE(value);
    RETURN_INT(PyDict_SetItemString(mapping, key, value));
}


/*[clinic input]
_testcapi.dict_setdefault
    mapping: object
    key: object
    defaultobj: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_setdefault_impl(PyObject *module, PyObject *mapping,
                               PyObject *key, PyObject *defaultobj)
/*[clinic end generated code: output=59c54ee6f6b5fec6 input=17ec3f91916e0537]*/
{
    NULLABLE(mapping);
    NULLABLE(key);
    NULLABLE(defaultobj);
    return PyDict_SetDefault(mapping, key, defaultobj);
}


/*[clinic input]
_testcapi.dict_delitem
    mapping: object
    key: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_delitem_impl(PyObject *module, PyObject *mapping,
                            PyObject *key)
/*[clinic end generated code: output=9fbff076cb508138 input=a55daafa5802bbb6]*/
{
    NULLABLE(mapping);
    NULLABLE(key);
    RETURN_INT(PyDict_DelItem(mapping, key));
}


/*[clinic input]
_testcapi.dict_delitemstring
    mapping: object
    key: str(accept={robuffer}, zeroes=True)
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_delitemstring_impl(PyObject *module, PyObject *mapping,
                                  const char *key, Py_ssize_t key_length)
/*[clinic end generated code: output=9de0731724962620 input=5014e819b1836a3c]*/
{
    NULLABLE(mapping);
    RETURN_INT(PyDict_DelItemString(mapping, key));
}


/*[clinic input]
_testcapi.dict_keys
    obj: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_keys(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=00e71422734d046c input=e5e245988a0269f1]*/
{
    NULLABLE(obj);
    return PyDict_Keys(obj);
}


/*[clinic input]
_testcapi.dict_values
    obj: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_values(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=4914dc42ae946905 input=ae6cad73caa19486]*/
{
    NULLABLE(obj);
    return PyDict_Values(obj);
}


/*[clinic input]
_testcapi.dict_items
    obj: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_items(PyObject *module, PyObject *obj)
/*[clinic end generated code: output=f9ddf49667eb4de7 input=d064428cb12cbbf6]*/
{
    NULLABLE(obj);
    return PyDict_Items(obj);
}


/*[clinic input]
_testcapi.dict_next
    mapping: object
    pos: Py_ssize_t
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_next_impl(PyObject *module, PyObject *mapping, Py_ssize_t pos)
/*[clinic end generated code: output=354ca34afaaf45d8 input=7decf2072b219a1d]*/
{
    PyObject  *key, *value;
    NULLABLE(mapping);
    int rc = PyDict_Next(mapping, &pos, &key, &value);
    if (rc != 0) {
        return Py_BuildValue("inOO", rc, pos, key, value);
    }
    if (PyErr_Occurred()) {
        return NULL;
    }
    Py_RETURN_NONE;
}


/*[clinic input]
_testcapi.dict_merge
    mapping: object
    mapping2: object
    override: int
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_merge_impl(PyObject *module, PyObject *mapping,
                          PyObject *mapping2, int override)
/*[clinic end generated code: output=0ccb5ae3aa8a8439 input=aad455e20a680da0]*/
{
    NULLABLE(mapping);
    NULLABLE(mapping2);
    RETURN_INT(PyDict_Merge(mapping, mapping2, override));
}


/*[clinic input]
_testcapi.dict_update
    mapping: object
    mapping2: object
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_update_impl(PyObject *module, PyObject *mapping,
                           PyObject *mapping2)
/*[clinic end generated code: output=03f5749b47b5a702 input=d9c80387e8370633]*/
{
    NULLABLE(mapping);
    NULLABLE(mapping2);
    RETURN_INT(PyDict_Update(mapping, mapping2));
}


/*[clinic input]
_testcapi.dict_mergefromseq2
    mapping: object
    seq: object
    override: int
    /
[clinic start generated code]*/

static PyObject *
_testcapi_dict_mergefromseq2_impl(PyObject *module, PyObject *mapping,
                                  PyObject *seq, int override)
/*[clinic end generated code: output=ed47a724c1b40d8e input=ebd3ffc60eb3470c]*/
{
    NULLABLE(mapping);
    NULLABLE(seq);
    RETURN_INT(PyDict_MergeFromSeq2(mapping, seq, override));
}


static PyMethodDef test_methods[] = {
    _TESTCAPI_DICT_CHECK_METHODDEF
    _TESTCAPI_DICT_CHECKEXACT_METHODDEF
    _TESTCAPI_DICT_NEW_METHODDEF
    _TESTCAPI_DICTPROXY_NEW_METHODDEF
    _TESTCAPI_DICT_CLEAR_METHODDEF
    _TESTCAPI_DICT_COPY_METHODDEF
    _TESTCAPI_DICT_SIZE_METHODDEF
    _TESTCAPI_DICT_GETITEM_METHODDEF
    _TESTCAPI_DICT_GETITEMWITHERROR_METHODDEF
    _TESTCAPI_DICT_GETITEMSTRING_METHODDEF
    _TESTCAPI_DICT_GETITEMREF_METHODDEF
    _TESTCAPI_DICT_GETITEMSTRINGREF_METHODDEF
    _TESTCAPI_DICT_CONTAINS_METHODDEF
    _TESTCAPI_DICT_SETITEM_METHODDEF
    _TESTCAPI_DICT_SETITEMSTRING_METHODDEF
    _TESTCAPI_DICT_DELITEM_METHODDEF
    _TESTCAPI_DICT_DELITEMSTRING_METHODDEF
    _TESTCAPI_DICT_SETDEFAULT_METHODDEF
    _TESTCAPI_DICT_KEYS_METHODDEF
    _TESTCAPI_DICT_VALUES_METHODDEF
    _TESTCAPI_DICT_ITEMS_METHODDEF
    _TESTCAPI_DICT_NEXT_METHODDEF
    _TESTCAPI_DICT_MERGE_METHODDEF
    _TESTCAPI_DICT_UPDATE_METHODDEF
    _TESTCAPI_DICT_MERGEFROMSEQ2_METHODDEF

    {NULL},
};

int
_PyTestCapi_Init_Dict(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    return 0;
}
