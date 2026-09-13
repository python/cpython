#include "parts.h"

#include "pycore_dict.h"

static PyObject*
dict_keys_layout(PyObject *self, PyObject *arg)
{
    PyDictObject *mp = (PyDictObject *)arg;
    PyDictKeysObject *keys = mp->ma_keys;

    size_t indices_size = DK_INDEX_BYTES(keys);

    char *base = _DK_INDICES(keys);
    char *header = (char *)keys;
    char *entries = (char *)_DK_ENTRIES(keys);

    bool ok = true;
    ok &= (header == base + indices_size);
    ok &= (entries == header + offsetof(PyDictKeysObject, dk_entries));

    return PyBool_FromLong(ok);
}

static PyObject*
dict_keys_to_base(PyObject *self, PyObject *arg)
{
    PyDictObject *mp = (PyDictObject *)arg;
    PyDictKeysObject *keys = mp->ma_keys;

    void *base = _DK_INDICES(keys);
    size_t indices_size = DK_INDEX_BYTES(keys);
    bool ok = _DK_FROM_BASE(base, indices_size) == keys;

    return PyBool_FromLong(ok);
}

static PyMethodDef test_methods[] = {
    {"dict_keys_layout", dict_keys_layout, METH_O},
    {"dict_keys_to_base", dict_keys_to_base, METH_O},
    {NULL},
};

int
_PyTestInternalCapi_Init_Dict(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }
    return 0;
}
