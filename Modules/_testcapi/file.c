#include "parts.h"
#include "util.h"

static PyObject *
file_from_fd(PyObject *Py_UNUSED(self), PyObject *args)
{
    int fd;
    const char *name;
    const char *mode;
    int buffering;
    const char *encoding;
    const char *errors;
    const char *newline;
    int closefd;
    if (!PyArg_ParseTuple(args, "izzizzzi",
                          &fd,
                          &name, &mode,
                          &buffering,
                          &encoding, &errors, &newline,
                          &closefd)) {
        return NULL;
    }

    return PyFile_FromFd(fd,
                         name, mode,
                         buffering,
                         encoding, errors, newline,
                         closefd);
}

static PyObject *
file_get_line(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *obj;
    int n;
    if (!PyArg_ParseTuple(args, "Oi", &obj, &n)) {
        return NULL;
    }

    NULLABLE(obj);
    return PyFile_GetLine(obj, n);
}

static PyObject *
file_write_object(PyObject *Py_UNUSED(self), PyObject *args)
{
    PyObject *obj, *p;
    int flags;
    if (!PyArg_ParseTuple(args, "OOi", &obj, &p, &flags)) {
        return NULL;
    }

    NULLABLE(obj);
    NULLABLE(p);
    RETURN_INT(PyFile_WriteObject(obj, p, flags));
}

static PyObject *
file_write_string(PyObject *Py_UNUSED(self), PyObject *args)
{
    const char *string;
    PyObject *f;
    if (!PyArg_ParseTuple(args, "zO", &string, &f)) {
        return NULL;
    }

    NULLABLE(f);
    RETURN_INT(PyFile_WriteString(string, f));
}

static PyObject *
object_as_file_descriptor(PyObject *Py_UNUSED(self), PyObject *obj)
{
    NULLABLE(obj);
    RETURN_INT(PyObject_AsFileDescriptor(obj));
}

static PyMethodDef test_methods[] = {
    {"file_from_fd", file_from_fd, METH_VARARGS},
    {"file_get_line", file_get_line, METH_VARARGS},
    {"file_write_object", file_write_object, METH_VARARGS},
    {"file_write_string", file_write_string, METH_VARARGS},
    {"object_as_file_descriptor", object_as_file_descriptor, METH_O},
    {NULL},
};

int
_PyTestCapi_Init_File(PyObject *m)
{
    if (PyModule_AddFunctions(m, test_methods) < 0) {
        return -1;
    }

    if (PyModule_AddIntMacro(m, Py_PRINT_RAW) < 0) {
        return -1;
    }

    return 0;
}
