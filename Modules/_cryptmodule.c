/* cryptmodule.c - by Steve Majewski
 */

#include "Python.h"

#include <sys/types.h>

/* Module crypt */

/*[clinic input]
module crypt
[clinic start generated code]*/
/*[clinic end generated code: checksum=da39a3ee5e6b4b0d3255bfef95601890afd80709]*/


/*[clinic input]
crypt.crypt

    word: 's'
    salt: 's'
    /

Hash a *word* with the given *salt* and return the hashed password.

*word* will usually be a user's password.  *salt* (either a random 2 or 16
character string, possibly prefixed with $digit$ to indicate the method)
will be used to perturb the encryption algorithm and produce distinct
results for a given *word*.

[clinic start generated code]*/

PyDoc_STRVAR(crypt_crypt__doc__,
"crypt(module, word, salt)\n"
"Hash a *word* with the given *salt* and return the hashed password.\n"
"\n"
"*word* will usually be a user\'s password.  *salt* (either a random 2 or 16\n"
"character string, possibly prefixed with $digit$ to indicate the method)\n"
"will be used to perturb the encryption algorithm and produce distinct\n"
"results for a given *word*.");

#define CRYPT_CRYPT_METHODDEF    \
    {"crypt", (PyCFunction)crypt_crypt, METH_VARARGS, crypt_crypt__doc__},

static PyObject *
crypt_crypt_impl(PyModuleDef *module, const char *word, const char *salt);

static PyObject *
crypt_crypt(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    const char *word;
    const char *salt;

    if (!PyArg_ParseTuple(args,
        "ss:crypt",
        &word, &salt))
        goto exit;
    return_value = crypt_crypt_impl(module, word, salt);

exit:
    return return_value;
}

static PyObject *
crypt_crypt_impl(PyModuleDef *module, const char *word, const char *salt)
/*[clinic end generated code: checksum=dbfe26a21eb335abefe6a0bbd0a682ea22b9adc0]*/
{
    /* On some platforms (AtheOS) crypt returns NULL for an invalid
       salt. Return None in that case. XXX Maybe raise an exception?  */
    return Py_BuildValue("s", crypt(word, salt));
}


static PyMethodDef crypt_methods[] = {
    CRYPT_CRYPT_METHODDEF
    {NULL,              NULL}           /* sentinel */
};


static struct PyModuleDef cryptmodule = {
    PyModuleDef_HEAD_INIT,
    "_crypt",
    NULL,
    -1,
    crypt_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__crypt(void)
{
    return PyModule_Create(&cryptmodule);
}
