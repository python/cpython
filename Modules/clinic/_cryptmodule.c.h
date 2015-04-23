/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(crypt_crypt__doc__,
"crypt($module, word, salt, /)\n"
"--\n"
"\n"
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

    if (!PyArg_ParseTuple(args, "ss:crypt",
        &word, &salt))
        goto exit;
    return_value = crypt_crypt_impl(module, word, salt);

exit:
    return return_value;
}
/*[clinic end generated code: output=22c295c9bce018c4 input=a9049054013a1b77]*/
