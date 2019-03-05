/*
posixshmem - A Python extension that provides shm_open() and shm_unlink()
*/

#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include "structmember.h"

// for shm_open() and shm_unlink()
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
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
    const char *name = PyUnicode_AsUTF8(path);
    if (name == NULL) {
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

Remove a shared memory object (similar to unlink()).

Remove a shared memory object name, and, once all processes  have  unmapped
the object, de-allocates and destroys the contents of the associated memory
region.

[clinic start generated code]*/

static PyObject *
_posixshmem_shm_unlink_impl(PyObject *module, PyObject *path)
/*[clinic end generated code: output=42f8b23d134b9ff5 input=8dc0f87143e3b300]*/
{
    int rv;
    int async_err = 0;
    const char *name = PyUnicode_AsUTF8(path);
    if (name == NULL) {
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


static struct PyModuleDef this_module = {
    PyModuleDef_HEAD_INIT,  // m_base
    "_posixshmem",          // m_name
    "POSIX shared memory module",     // m_doc
    -1,                     // m_size (space allocated for module globals)
    module_methods,         // m_methods
};

/* Module init function */
PyMODINIT_FUNC
PyInit__posixshmem(void) {
    PyObject *module;
    module = PyModule_Create(&this_module);
    if (!module) {
        return NULL;
    }
    return module;
}
