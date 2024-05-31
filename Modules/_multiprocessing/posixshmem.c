/*
posixshmem - A Python extension that provides shm_open() and shm_unlink()
*/

// Need limited C API version 3.13 for Py_mod_gil
#include "pyconfig.h"   // Py_GIL_DISABLED
#ifndef Py_GIL_DISABLED
#  define Py_LIMITED_API 0x030d0000
#endif

#include <Python.h>

#include <string.h>               // strlen()
#include <errno.h>                // EINTR
#ifdef HAVE_SYS_MMAN_H
#  include <sys/mman.h>           // shm_open(), shm_unlink()
#endif


/*[clinic input]
module _posixshmem
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=a416734e49164bf8]*/

/*
 *
 * Module-level functions & meta stuff
 *
 */

#ifdef HAVE_SHM_OPEN
/*[clinic input]
_posixshmem.shm_open -> int
    path: unicode
    flags: int
    mode: int = 0o777

# "shm_open(path, flags, mode=0o777)\n\n\

Open a shared memory object.  Returns a file descriptor (integer).

[clinic start generated code]*/

static int
_posixshmem_shm_open_impl(PyObject *module, PyObject *path, int flags,
                          int mode)
/*[clinic end generated code: output=8d110171a4fa20df input=e83b58fa802fac25]*/
{
    int fd;
    int async_err = 0;
    Py_ssize_t name_size;
    const char *name = PyUnicode_AsUTF8AndSize(path, &name_size);
    if (name == NULL) {
        return -1;
    }
    if (strlen(name) != (size_t)name_size) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        return -1;
    }
    do {
        Py_BEGIN_ALLOW_THREADS
        fd = shm_open(name, flags, mode);
        Py_END_ALLOW_THREADS
    } while (fd < 0 && errno == EINTR && !(async_err = PyErr_CheckSignals()));

    if (fd < 0) {
        if (!async_err)
            PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, path);
        return -1;
    }

    return fd;
}
#endif /* HAVE_SHM_OPEN */

#ifdef HAVE_SHM_UNLINK
/*[clinic input]
_posixshmem.shm_unlink
    path: unicode
    /

Remove a shared memory object (similar to unlink()).

Remove a shared memory object name, and, once all processes  have  unmapped
the object, de-allocates and destroys the contents of the associated memory
region.

[clinic start generated code]*/

static PyObject *
_posixshmem_shm_unlink_impl(PyObject *module, PyObject *path)
/*[clinic end generated code: output=42f8b23d134b9ff5 input=298369d013dcad63]*/
{
    int rv;
    int async_err = 0;
    Py_ssize_t name_size;
    const char *name = PyUnicode_AsUTF8AndSize(path, &name_size);
    if (name == NULL) {
        return NULL;
    }
    if (strlen(name) != (size_t)name_size) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        return NULL;
    }
    do {
        Py_BEGIN_ALLOW_THREADS
        rv = shm_unlink(name);
        Py_END_ALLOW_THREADS
    } while (rv < 0 && errno == EINTR && !(async_err = PyErr_CheckSignals()));

    if (rv < 0) {
        if (!async_err)
            PyErr_SetFromErrnoWithFilenameObject(PyExc_OSError, path);
        return NULL;
    }

    Py_RETURN_NONE;
}
#endif /* HAVE_SHM_UNLINK */

#include "clinic/posixshmem.c.h"

static PyMethodDef module_methods[ ] = {
    _POSIXSHMEM_SHM_OPEN_METHODDEF
    _POSIXSHMEM_SHM_UNLINK_METHODDEF
    {NULL} /* Sentinel */
};


static PyModuleDef_Slot module_slots[] = {
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};


static struct PyModuleDef _posixshmemmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_posixshmem",
    .m_doc = "POSIX shared memory module",
    .m_size = 0,
    .m_methods = module_methods,
    .m_slots = module_slots,
};

/* Module init function */
PyMODINIT_FUNC
PyInit__posixshmem(void)
{
    return PyModuleDef_Init(&_posixshmemmodule);
}
