
/* Emulation of PyFile_Check() and PyFile_AsFile() for Python 3. */

static PyObject *PyIOBase_TypeObj;

static int init_file_emulator(void)
{
    if (PyIOBase_TypeObj == NULL) {
        PyObject *io = PyImport_ImportModule("_io");
        if (io == NULL)
            return -1;
        PyIOBase_TypeObj = PyObject_GetAttrString(io, "_IOBase");
        if (PyIOBase_TypeObj == NULL)
            return -1;
    }
    return 0;
}


#define PyFile_Check(p)  PyObject_IsInstance(p, PyIOBase_TypeObj)


static void _close_file_capsule(PyObject *ob_capsule)
{
    FILE *f = (FILE *)PyCapsule_GetPointer(ob_capsule, "FILE");
    if (f != NULL)
        fclose(f);
}


static FILE *PyFile_AsFile(PyObject *ob_file)
{
    PyObject *ob, *ob_capsule = NULL, *ob_mode = NULL;
    FILE *f;
    int fd;
    const char *mode;

    ob = PyObject_CallMethod(ob_file, "flush", NULL);
    if (ob == NULL)
        goto fail;
    Py_DECREF(ob);

    ob_capsule = PyObject_GetAttrString(ob_file, "__cffi_FILE");
    if (ob_capsule == NULL) {
        PyErr_Clear();

        fd = PyObject_AsFileDescriptor(ob_file);
        if (fd < 0)
            goto fail;

        ob_mode = PyObject_GetAttrString(ob_file, "mode");
        if (ob_mode == NULL)
            goto fail;
        mode = PyText_AsUTF8(ob_mode);
        if (mode == NULL)
            goto fail;

        fd = dup(fd);
        if (fd < 0) {
            PyErr_SetFromErrno(PyExc_OSError);
            goto fail;
        }

        f = fdopen(fd, mode);
        if (f == NULL) {
            close(fd);
            PyErr_SetFromErrno(PyExc_OSError);
            goto fail;
        }
        setbuf(f, NULL);    /* non-buffered */
        Py_DECREF(ob_mode);
        ob_mode = NULL;

        ob_capsule = PyCapsule_New(f, "FILE", _close_file_capsule);
        if (ob_capsule == NULL) {
            fclose(f);
            goto fail;
        }

        if (PyObject_SetAttrString(ob_file, "__cffi_FILE", ob_capsule) < 0)
            goto fail;
    }
    else {
        f = PyCapsule_GetPointer(ob_capsule, "FILE");
    }
    Py_DECREF(ob_capsule);   /* assumes still at least one reference */
    return f;

 fail:
    Py_XDECREF(ob_mode);
    Py_XDECREF(ob_capsule);
    return NULL;
}
