/*
posixshmem - A Python extension that provides shm_open() and shm_unlink()
*/

#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include <stdio.h>
#include "pycore_atomic.h"

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

/*[clinic input]
_posixshmem.shm_inc_refcount -> int
    ptr: object

Increment Reference Count of the memoryview object

[clinic start generated code]*/

static int
_posixshmem_shm_inc_refcount_impl(PyObject *module, PyObject *ptr)
/*[clinic end generated code: output=9ed5b4d016975d06 input=b7c1fe6ce39b7bb4]*/
{
    Py_buffer *buf = PyMemoryView_GET_BUFFER(ptr);
    return _Py_atomic_fetch_add_relaxed((_Py_atomic_int*)(buf->buf), 1) + 1;
}
/*[clinic input]
_posixshmem.shm_dec_refcount -> int
    ptr: object

Decrement Reference Count of the memoryview object

[clinic start generated code]*/

static int
_posixshmem_shm_dec_refcount_impl(PyObject *module, PyObject *ptr)
/*[clinic end generated code: output=16ab284487281c72 input=0aab6ded127aa5c3]*/
{
    Py_buffer *buf = PyMemoryView_GET_BUFFER(ptr);
    return _Py_atomic_fetch_sub_relaxed((_Py_atomic_int*)(buf->buf), 1) - 1;
}

#include "clinic/posixshmem.c.h"

static PyMethodDef module_methods[ ] = {
    _POSIXSHMEM_SHM_OPEN_METHODDEF
    _POSIXSHMEM_SHM_UNLINK_METHODDEF
    _POSIXSHMEM_SHM_INC_REFCOUNT_METHODDEF
    _POSIXSHMEM_SHM_DEC_REFCOUNT_METHODDEF 
    {NULL} /* Sentinel */
};


static struct PyModuleDef this_module = {
    PyModuleDef_HEAD_INIT,  // m_base
    "_posixshmem",          // m_name
    "POSIX shared memory module",     // m_doc
    -1,                     // m_size (space allocated for module globals)
    module_methods,         // m_methods
};

const char *NAME_REFCOUNT_SIZE = "REFCOUNT_SIZE";

/* Module init function */
PyMODINIT_FUNC
PyInit__posixshmem(void) {
    PyObject *module;
    module = PyModule_Create(&this_module);
    if (!module) {
        return NULL;
    }
    if (PyModule_AddIntConstant(module, NAME_REFCOUNT_SIZE, sizeof(_Py_atomic_int))) {
        Py_XDECREF(module);
        return NULL;
    }
    return module;
}
