/* cryptmodule.c - by Steve Majewski
 */

#include "Python.h"

#include <sys/types.h>
#ifdef HAVE_CRYPT_H
#include <crypt.h>
#endif

/* Module crypt */

/*[clinic input]
module crypt
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=c6252cf4f2f2ae81]*/

#include "clinic/_cryptmodule.c.h"

/*[clinic input]
crypt.crypt

    word: str
    salt: str
    /

Hash a *word* with the given *salt* and return the hashed password.

*word* will usually be a user's password.  *salt* (either a random 2 or 16
character string, possibly prefixed with $digit$ to indicate the method)
will be used to perturb the encryption algorithm and produce distinct
results for a given *word*.

[clinic start generated code]*/

static PyObject *
crypt_crypt_impl(PyObject *module, const char *word, const char *salt)
/*[clinic end generated code: output=0512284a03d2803c input=0e8edec9c364352b]*/
{
    char *crypt_result;
#ifdef HAVE_CRYPT_R
    struct crypt_data data;
    memset(&data, 0, sizeof(data));
    crypt_result = crypt_r(word, salt, &data);
#else
    crypt_result = crypt(word, salt);
#endif
    if (crypt_result == NULL) {
        return PyErr_SetFromErrno(PyExc_OSError);
    }
    return Py_BuildValue("s", crypt_result);
}


static PyMethodDef crypt_methods[] = {
    CRYPT_CRYPT_METHODDEF
    {NULL,              NULL}           /* sentinel */
};

static PyModuleDef_Slot _crypt_slots[] = {
    {0, NULL}
};

static struct PyModuleDef cryptmodule = {
    PyModuleDef_HEAD_INIT,
    "_crypt",
    NULL,
    0,
    crypt_methods,
    _crypt_slots,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__crypt(void)
{
    return PyModuleDef_Init(&cryptmodule);
}
