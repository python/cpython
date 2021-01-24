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
    {"crypt", (PyCFunction)(void(*)(void))crypt_crypt, METH_FASTCALL, crypt_crypt__doc__},

static PyObject *
crypt_crypt_impl(PyObject *module, const char *word, const char *salt);

static PyObject *
crypt_crypt(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *word;
    const char *salt;

    if (!_PyArg_CheckPositional("crypt", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("crypt", "argument 1", "str", args[0]);
        goto exit;
    }
    Py_ssize_t word_length;
    word = PyUnicode_AsUTF8AndSize(args[0], &word_length);
    if (word == NULL) {
        goto exit;
    }
    if (strlen(word) != (size_t)word_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("crypt", "argument 2", "str", args[1]);
        goto exit;
    }
    Py_ssize_t salt_length;
    salt = PyUnicode_AsUTF8AndSize(args[1], &salt_length);
    if (salt == NULL) {
        goto exit;
    }
    if (strlen(salt) != (size_t)salt_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = crypt_crypt_impl(module, word, salt);

exit:
    return return_value;
}
/*[clinic end generated code: output=549de0d43b030126 input=a9049054013a1b77]*/
