/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_testlimitedcapi_pack_full_version__doc__,
"pack_full_version($module, major, minor, micro, level, serial, /)\n"
"--\n"
"\n");

#define _TESTLIMITEDCAPI_PACK_FULL_VERSION_METHODDEF    \
    {"pack_full_version", (PyCFunction)(void(*)(void))_testlimitedcapi_pack_full_version, METH_FASTCALL, _testlimitedcapi_pack_full_version__doc__},

static PyObject *
_testlimitedcapi_pack_full_version_impl(PyObject *module, int major,
                                        int minor, int micro, int level,
                                        int serial);

static PyObject *
_testlimitedcapi_pack_full_version(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int major;
    int minor;
    int micro;
    int level;
    int serial;

    if (nargs != 5) {
        PyErr_Format(PyExc_TypeError, "pack_full_version expected 5 arguments, got %zd", nargs);
        goto exit;
    }
    major = PyLong_AsInt(args[0]);
    if (major == -1 && PyErr_Occurred()) {
        goto exit;
    }
    minor = PyLong_AsInt(args[1]);
    if (minor == -1 && PyErr_Occurred()) {
        goto exit;
    }
    micro = PyLong_AsInt(args[2]);
    if (micro == -1 && PyErr_Occurred()) {
        goto exit;
    }
    level = PyLong_AsInt(args[3]);
    if (level == -1 && PyErr_Occurred()) {
        goto exit;
    }
    serial = PyLong_AsInt(args[4]);
    if (serial == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _testlimitedcapi_pack_full_version_impl(module, major, minor, micro, level, serial);

exit:
    return return_value;
}

PyDoc_STRVAR(_testlimitedcapi_pack_version__doc__,
"pack_version($module, major, minor, /)\n"
"--\n"
"\n");

#define _TESTLIMITEDCAPI_PACK_VERSION_METHODDEF    \
    {"pack_version", (PyCFunction)(void(*)(void))_testlimitedcapi_pack_version, METH_FASTCALL, _testlimitedcapi_pack_version__doc__},

static PyObject *
_testlimitedcapi_pack_version_impl(PyObject *module, int major, int minor);

static PyObject *
_testlimitedcapi_pack_version(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int major;
    int minor;

    if (nargs != 2) {
        PyErr_Format(PyExc_TypeError, "pack_version expected 2 arguments, got %zd", nargs);
        goto exit;
    }
    major = PyLong_AsInt(args[0]);
    if (major == -1 && PyErr_Occurred()) {
        goto exit;
    }
    minor = PyLong_AsInt(args[1]);
    if (minor == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _testlimitedcapi_pack_version_impl(module, major, minor);

exit:
    return return_value;
}
/*[clinic end generated code: output=aed3e226da77f2d2 input=a9049054013a1b77]*/
