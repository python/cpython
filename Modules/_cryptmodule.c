/* cryptmodule.c - by Steve Majewski
 */

#include "Python.h"

#include <sys/types.h>

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
crypt_crypt_impl(PyModuleDef *module, const char *word, const char *salt)
/*[clinic end generated code: output=995ad1e854d83069 input=0e8edec9c364352b]*/
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
