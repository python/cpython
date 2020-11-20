#include "Python.h"
#include "../_ssl.h"

#include "openssl/x509_vfy.h"


/*[clinic input]
module _ssl
class _ssl.TrustStore "PySSLTrustStore *" "PySSLTrustStore_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=07c50a283a64cefa]*/

#include "clinic/truststore.c.h"

static PyObject *
newTrustStore(PyTypeObject *type, X509_STORE *store, int upref)
{
    PySSLTrustStore *self;

    assert(type != NULL && type->tp_alloc != NULL);
    assert(store != NULL);

    self = (PySSLTrustStore *) type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }
    if (upref == 1) {
        X509_STORE_up_ref(store);
    }
    self->store = store;
    self->hash_dirs = NULL;

    return (PyObject *) self;
}

static PyObject *
_PySSL_TrustStoreFromStore(X509_STORE *store, int upref)
{
    return newTrustStore(PySSLTrustStore_Type, store, upref);
}

static int
truststore_traverse(PySSLTrustStore *self, visitproc visit, void *arg)
{
    if (self->hash_dirs != NULL) {
        Py_VISIT(self->hash_dirs);
    }
    return 0;
}

static int
truststore_clear(PySSLTrustStore *self)
{
    if (self->hash_dirs != NULL) {
        Py_CLEAR(self->hash_dirs);
    }
    return 0;
}

static void
truststore_dealloc(PySSLTrustStore *self)
{
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    truststore_clear(self);
    X509_STORE_free(self->store);
    Py_TYPE(self)->tp_free(self);
    Py_DECREF(tp);
}

static PyGetSetDef truststore_getsetlist[] = {
    {NULL}             /* sentinel */
};

static PyMethodDef truststore_methods[] = {
    /* methods */
    {NULL, NULL}
};

static PyType_Slot truststore_slots[] = {
    {Py_tp_dealloc, truststore_dealloc},
    {Py_tp_traverse, truststore_traverse},
    {Py_tp_clear, truststore_clear},
    {Py_tp_methods, truststore_methods},
    {Py_tp_getset, truststore_getsetlist},
    {0, 0},
};

static PyType_Spec PySSLTrustStore_spec = {
    "_ssl.TrustStore",
    sizeof(PySSLTrustStore),
    0,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    truststore_slots,
};
