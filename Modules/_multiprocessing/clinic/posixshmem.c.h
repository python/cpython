/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(HAVE_SHM_OPEN)

PyDoc_STRVAR(_posixshmem_shm_open__doc__,
"shm_open($module, /, path, flags, mode=511)\n"
"--\n"
"\n"
"Open a shared memory object.  Returns a file descriptor (integer).");

#define _POSIXSHMEM_SHM_OPEN_METHODDEF    \
    {"shm_open", (PyCFunction)(void(*)(void))_posixshmem_shm_open, METH_VARARGS|METH_KEYWORDS, _posixshmem_shm_open__doc__},

static int
_posixshmem_shm_open_impl(PyObject *module, PyObject *path, int flags,
                          int mode);

static PyObject *
_posixshmem_shm_open(PyObject *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"path", "flags", "mode", NULL};
    PyObject *path;
    int flags;
    int mode = 511;
    int _return_value;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Ui|i:shm_open", _keywords,
        &path, &flags, &mode))
        goto exit;
    _return_value = _posixshmem_shm_open_impl(module, path, flags, mode);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_SHM_OPEN) */

#if defined(HAVE_SHM_RENAME)

PyDoc_STRVAR(_posixshmem_shm_rename__doc__,
"shm_rename($module, path_from, path_to, flags, /)\n"
"--\n"
"\n"
"Rename a shared memory object.\n"
"\n"
"Remove a shared memory object and relink to another path.\n"
"By default, if the destination path already exist, it will be unlinked.\n"
"With the SHM_RENAME_EXCHANGE flag, source and destination paths\n"
"will be exchanged.\n"
"With the SHM_RENAME_NOREPLACE flag, an error will be triggered\n"
"if the destination alredady exists.");

#define _POSIXSHMEM_SHM_RENAME_METHODDEF    \
    {"shm_rename", (PyCFunction)(void(*)(void))_posixshmem_shm_rename, METH_FASTCALL, _posixshmem_shm_rename__doc__},

static int
_posixshmem_shm_rename_impl(PyObject *module, PyObject *path_from,
                            PyObject *path_to, int flags);

static PyObject *
_posixshmem_shm_rename(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *path_from;
    PyObject *path_to;
    int flags;
    int _return_value;

    if (nargs != 3) {
        PyErr_Format(PyExc_TypeError, "shm_rename expected 3 arguments, got %zd", nargs);
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        PyErr_Format(PyExc_TypeError, "shm_rename() argument 1 must be str, not %T", args[0]);
        goto exit;
    }
    path_from = args[0];
    if (!PyUnicode_Check(args[1])) {
        PyErr_Format(PyExc_TypeError, "shm_rename() argument 2 must be str, not %T", args[1]);
        goto exit;
    }
    path_to = args[1];
    flags = PyLong_AsInt(args[2]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _posixshmem_shm_rename_impl(module, path_from, path_to, flags);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

#endif /* defined(HAVE_SHM_RENAME) */

#if defined(HAVE_SHM_UNLINK)

PyDoc_STRVAR(_posixshmem_shm_unlink__doc__,
"shm_unlink($module, path, /)\n"
"--\n"
"\n"
"Remove a shared memory object (similar to unlink()).\n"
"\n"
"Remove a shared memory object name, and, once all processes  have  unmapped\n"
"the object, de-allocates and destroys the contents of the associated memory\n"
"region.");

#define _POSIXSHMEM_SHM_UNLINK_METHODDEF    \
    {"shm_unlink", (PyCFunction)_posixshmem_shm_unlink, METH_O, _posixshmem_shm_unlink__doc__},

static PyObject *
_posixshmem_shm_unlink_impl(PyObject *module, PyObject *path);

static PyObject *
_posixshmem_shm_unlink(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *path;

    if (!PyUnicode_Check(arg)) {
        PyErr_Format(PyExc_TypeError, "shm_unlink() argument must be str, not %T", arg);
        goto exit;
    }
    path = arg;
    return_value = _posixshmem_shm_unlink_impl(module, path);

exit:
    return return_value;
}

#endif /* defined(HAVE_SHM_UNLINK) */

#ifndef _POSIXSHMEM_SHM_OPEN_METHODDEF
    #define _POSIXSHMEM_SHM_OPEN_METHODDEF
#endif /* !defined(_POSIXSHMEM_SHM_OPEN_METHODDEF) */

#ifndef _POSIXSHMEM_SHM_RENAME_METHODDEF
    #define _POSIXSHMEM_SHM_RENAME_METHODDEF
#endif /* !defined(_POSIXSHMEM_SHM_RENAME_METHODDEF) */

#ifndef _POSIXSHMEM_SHM_UNLINK_METHODDEF
    #define _POSIXSHMEM_SHM_UNLINK_METHODDEF
#endif /* !defined(_POSIXSHMEM_SHM_UNLINK_METHODDEF) */
/*[clinic end generated code: output=3b3a888f3ea27db0 input=a9049054013a1b77]*/
