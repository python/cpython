/* cryptmodule.c - by Steve Majewski
 */

#include "Python.h"

#include <sys/types.h>

#ifdef __VMS
#include <openssl/des.h>
#endif

/* Module crypt */


static PyObject *crypt_crypt(PyObject *self, PyObject *args)
{
    char *word, *salt;
#ifndef __VMS
    extern char * crypt(const char *, const char *);
#endif

    if (!PyArg_ParseTuple(args, "ss:crypt", &word, &salt)) {
        return NULL;
    }
    /* On some platforms (AtheOS) crypt returns NULL for an invalid
       salt. Return None in that case. XXX Maybe raise an exception?  */
    return Py_BuildValue("s", crypt(word, salt));

}

PyDoc_STRVAR(crypt_crypt__doc__,
"crypt(word, salt) -> string\n\
word will usually be a user's password. salt is a 2-character string\n\
which will be used to select one of 4096 variations of DES. The characters\n\
in salt must be either \".\", \"/\", or an alphanumeric character. Returns\n\
the hashed password as a string, which will be composed of characters from\n\
the same alphabet as the salt.");


static PyMethodDef crypt_methods[] = {
    {"crypt",           crypt_crypt, METH_VARARGS, crypt_crypt__doc__},
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
