/*
posixshmem - A Python module for accessing POSIX 1003.1b-1993 shared memory.

Copyright (c) 2012, Philip Semanchuk
Copyright (c) 2018, 2019, Davin Potts
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of posixshmem nor the names of its contributors may
      be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY ITS CONTRIBUTORS ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL Philip Semanchuk BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include "structmember.h"

#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

// For shared memory stuff
#include <sys/stat.h>
#include <sys/mman.h>

/* SEM_FAILED is defined as an int in Apple's headers, and this makes the
compiler complain when I compare it to a pointer. Python faced the same
problem (issue 9586) and I copied their solution here.
ref: http://bugs.python.org/issue9586

Note that in /Developer/SDKs/MacOSX10.4u.sdk/usr/include/sys/semaphore.h,
SEM_FAILED is #defined as -1 and that's apparently the definition used by
Python when building. In /usr/include/sys/semaphore.h, it's defined
as ((sem_t *)-1).
*/
#ifdef __APPLE__
    #undef  SEM_FAILED
    #define SEM_FAILED ((sem_t *)-1)
#endif

/* POSIX says that a mode_t "shall be an integer type". To avoid the need
for a specific get_mode function for each type, I'll just stuff the mode into
a long and mention it in the Xxx_members list for each type.
ref: http://www.opengroup.org/onlinepubs/000095399/basedefs/sys/types.h.html
*/

typedef struct {
    PyObject_HEAD
    char *name;
    long mode;
    int fd;
} SharedMemory;


// FreeBSD (and perhaps other BSDs) limit names to 14 characters. In the
// code below, strings of this length are allocated on the stack, so
// increase this gently or change that code to use malloc().
#define MAX_SAFE_NAME_LENGTH  14


/* Struct to contain an IPC object name which can be None */
typedef struct {
    int is_none;
    char *name;
} NoneableName;


/*
      Exceptions for this module
*/

static PyObject *pBaseException;
static PyObject *pPermissionsException;
static PyObject *pExistentialException;


#ifdef POSIX_IPC_DEBUG
#define DPRINTF(fmt, args...) fprintf(stderr, "+++ " fmt, ## args)
#else
#define DPRINTF(fmt, args...)
#endif

static char *
bytes_to_c_string(PyObject* o, int lock) {
/* Convert a bytes object to a char *. Optionally lock the buffer if it is a
   bytes array.
   This code swiped directly from Python 3.1's posixmodule.c by Philip S.
   The name there is bytes2str().
*/
    if (PyBytes_Check(o))
        return PyBytes_AsString(o);
    else if (PyByteArray_Check(o)) {
        if (lock && PyObject_GetBuffer(o, NULL, 0) < 0)
            /* On a bytearray, this should not fail. */
            PyErr_BadInternalCall();
        return PyByteArray_AsString(o);
    } else {
        /* The FS converter should have verified that this
           is either bytes or bytearray. */
        Py_FatalError("bad object passed to bytes2str");
        /* not reached. */
        return "";
    }
}

static void
release_bytes(PyObject* o)
    /* Release the lock, decref the object.
   This code swiped directly from Python 3.1's posixmodule.c by Philip S.
   */
{
    if (PyByteArray_Check(o))
        o->ob_type->tp_as_buffer->bf_releasebuffer(NULL, 0);
    Py_DECREF(o);
}


static int
random_in_range(int min, int max) {
    // returns a random int N such that min <= N <= max
    int diff = (max - min) + 1;

    // ref: http://www.c-faq.com/lib/randrange.html
    return ((int)((double)rand() / ((double)RAND_MAX + 1) * diff)) + min;
}


static
int create_random_name(char *name) {
    // The random name is always lowercase so that this code will work
    // on case-insensitive file systems. It always starts with a forward
    // slash.
    int length;
    char *alphabet = "abcdefghijklmnopqrstuvwxyz";
    int i;

    // Generate a random length for the name. I subtract 1 from the
    // MAX_SAFE_NAME_LENGTH in order to allow for the name's leading "/".
    length = random_in_range(6, MAX_SAFE_NAME_LENGTH - 1);

    name[0] = '/';
    name[length] = '\0';
    i = length;
    while (--i)
        name[i] = alphabet[random_in_range(0, 25)];

    return length;
}


static int
convert_name_param(PyObject *py_name_param, void *checked_name) {
    /* Verifies that the py_name_param is either None or a string.
    If it's a string, checked_name->name points to a PyMalloc-ed buffer
    holding a NULL-terminated C version of the string when this function
    concludes. The caller is responsible for releasing the buffer.
    */
    int rc = 0;
    NoneableName *p_name = (NoneableName *)checked_name;
    PyObject *py_name_as_bytes = NULL;
    char *p_name_as_c_string = NULL;

    DPRINTF("inside convert_name_param\n");
    DPRINTF("PyBytes_Check() = %d \n", PyBytes_Check(py_name_param));
    DPRINTF("PyString_Check() = %d \n", PyString_Check(py_name_param));
    DPRINTF("PyUnicode_Check() = %d \n", PyUnicode_Check(py_name_param));

    p_name->is_none = 0;

    // The name can be None or a Python string
    if (py_name_param == Py_None) {
        DPRINTF("name is None\n");
        rc = 1;
        p_name->is_none = 1;
    }
    else if (PyUnicode_Check(py_name_param) || PyBytes_Check(py_name_param)) {
        DPRINTF("name is Unicode or bytes\n");
        // The caller passed me a Unicode string or a byte array; I need a
        // char *. Getting from one to the other takes a couple steps.

        if (PyUnicode_Check(py_name_param)) {
            DPRINTF("name is Unicode\n");
            // PyUnicode_FSConverter() converts the Unicode object into a
            // bytes or a bytearray object. (Why can't it be one or the other?)
            PyUnicode_FSConverter(py_name_param, &py_name_as_bytes);
        }
        else {
            DPRINTF("name is bytes\n");
            // Make a copy of the name param.
            py_name_as_bytes = PyBytes_FromObject(py_name_param);
        }

        // bytes_to_c_string() returns a pointer to the buffer.
        p_name_as_c_string = bytes_to_c_string(py_name_as_bytes, 0);

        // PyMalloc memory and copy the user-supplied name to it.
        p_name->name = (char *)PyMem_Malloc(strlen(p_name_as_c_string) + 1);
        if (p_name->name) {
            rc = 1;
            strcpy(p_name->name, p_name_as_c_string);
        }
        else
            PyErr_SetString(PyExc_MemoryError, "Out of memory");

        // The bytes version of the name isn't useful to me, and per the
        // documentation for PyUnicode_FSConverter(), I am responsible for
        // releasing it when I'm done.
        release_bytes(py_name_as_bytes);
    }
    else
        PyErr_SetString(PyExc_TypeError, "Name must be None or a string");

    return rc;
}



/*   =====  Begin Shared Memory implementation functions ===== */

static PyObject *
shm_str(SharedMemory *self) {
    return PyUnicode_FromString(self->name ? self->name : "(no name)");
}

static PyObject *
shm_repr(SharedMemory *self) {
    char mode[32];

    sprintf(mode, "0%o", (int)(self->mode));

    return PyUnicode_FromFormat("_posixshmem.SharedMemory(\"%s\", mode=%s)",
                                self->name, mode);
}

static PyObject *
my_shm_unlink(const char *name) {
    DPRINTF("unlinking shm name %s\n", name);
    if (-1 == shm_unlink(name)) {
        switch (errno) {
            case EACCES:
                PyErr_SetString(pPermissionsException, "Permission denied");
            break;

            case ENOENT:
                PyErr_SetString(pExistentialException,
                    "No shared memory exists with the specified name");
            break;

            case ENAMETOOLONG:
                PyErr_SetString(PyExc_ValueError, "The name is too long");
            break;

            default:
                PyErr_SetFromErrno(PyExc_OSError);
            break;
        }

        goto error_return;
    }

    Py_RETURN_NONE;

    error_return:
    return NULL;
}


static PyObject *
SharedMemory_new(PyTypeObject *type, PyObject *args, PyObject *kwlist) {
    SharedMemory *self;

    self = (SharedMemory *)type->tp_alloc(type, 0);

    return (PyObject *)self;
}


static int
SharedMemory_init(SharedMemory *self, PyObject *args, PyObject *keywords) {
    NoneableName name;
    char temp_name[MAX_SAFE_NAME_LENGTH + 1];
    unsigned int flags = 0;
    unsigned long size = 0;
    int read_only = 0;
    static char *keyword_list[ ] = {"name", "flags", "mode", "size", "read_only", NULL};

    // First things first -- initialize the self struct.
    self->name = NULL;
    self->fd = 0;
    self->mode = 0600;

    if (!PyArg_ParseTupleAndKeywords(args, keywords, "O&|Iiki", keyword_list,
                                    &convert_name_param, &name, &flags,
                                    &(self->mode), &size, &read_only))
        goto error_return;

    if ( !(flags & O_CREAT) && (flags & O_EXCL) ) {
        PyErr_SetString(PyExc_ValueError,
                "O_EXCL must be combined with O_CREAT");
        goto error_return;
    }

    if (name.is_none && ((flags & O_EXCL) != O_EXCL)) {
        PyErr_SetString(PyExc_ValueError,
                "Name can only be None if O_EXCL is set");
        goto error_return;
    }

    flags |= (read_only ? O_RDONLY : O_RDWR);

    if (name.is_none) {
        // (name == None) ==> generate a name for the caller
        do {
            errno = 0;
            create_random_name(temp_name);

            DPRINTF("calling shm_open, name=%s, flags=0x%x, mode=0%o\n",
                        temp_name, flags, (int)self->mode);
            self->fd = shm_open(temp_name, flags, (mode_t)self->mode);

        } while ( (-1 == self->fd) && (EEXIST == errno) );

        // PyMalloc memory and copy the randomly-generated name to it.
        self->name = (char *)PyMem_Malloc(strlen(temp_name) + 1);
        if (self->name)
            strcpy(self->name, temp_name);
        else {
            PyErr_SetString(PyExc_MemoryError, "Out of memory");
            goto error_return;
        }
    }
    else {
        // (name != None) ==> use name supplied by the caller. It was
        // already converted to C by convert_name_param().
        self->name = name.name;

        DPRINTF("calling shm_open, name=%s, flags=0x%x, mode=0%o\n",
                    self->name, flags, (int)self->mode);
        self->fd = shm_open(self->name, flags, (mode_t)self->mode);
    }

    DPRINTF("shm fd = %d\n", self->fd);

    if (-1 == self->fd) {
        self->fd = 0;
        switch (errno) {
            case EACCES:
                PyErr_Format(pPermissionsException,
                                "No permission to %s this segment",
                                (flags & O_TRUNC) ? "truncate" : "access"
                                );
            break;

            case EEXIST:
                PyErr_SetString(pExistentialException,
                                "Shared memory with the specified name already exists");
            break;

            case ENOENT:
                PyErr_SetString(pExistentialException,
                                "No shared memory exists with the specified name");
            break;

            case EINVAL:
                PyErr_SetString(PyExc_ValueError, "Invalid parameter(s)");
            break;

            case EMFILE:
                PyErr_SetString(PyExc_OSError,
                                 "This process already has the maximum number of files open");
            break;

            case ENFILE:
                PyErr_SetString(PyExc_OSError,
                                 "The system limit on the total number of open files has been reached");
            break;

            case ENAMETOOLONG:
                PyErr_SetString(PyExc_ValueError,
                                 "The name is too long");
            break;

            default:
                PyErr_SetFromErrno(PyExc_OSError);
            break;
        }

        goto error_return;
    }
    else {
        if (size) {
            DPRINTF("calling ftruncate, fd = %d, size = %ld\n", self->fd, size);
            if (-1 == ftruncate(self->fd, (off_t)size)) {
                // The code below will raise a Python error. Since that error
                // is raised during __init__(), it will look to the caller
                // as if object creation failed entirely. Here I clean up
                // the system object I just created.
                close(self->fd);
                shm_unlink(self->name);

                // ftruncate can return a ton of different errors, but most
                // are not relevant or are extremely unlikely.
                switch (errno) {
                    case EINVAL:
                        PyErr_SetString(PyExc_ValueError,
                                        "The size is invalid or the memory is read-only");
                    break;

                    case EFBIG:
                        PyErr_SetString(PyExc_ValueError,
                                        "The size is too large");
                    break;

                    case EROFS:
                    case EACCES:
                        PyErr_SetString(pPermissionsException,
                                        "The memory is read-only");
                    break;

                    default:
                        PyErr_SetFromErrno(PyExc_OSError);
                    break;
                }

                goto error_return;
            }
        }
    }

    return 0;

    error_return:
    return -1;
}


static void SharedMemory_dealloc(SharedMemory *self) {
    DPRINTF("dealloc\n");
    PyMem_Free(self->name);
    self->name = NULL;

    Py_TYPE(self)->tp_free((PyObject*)self);
}


PyObject *
SharedMemory_getsize(SharedMemory *self, void *closure) {
    struct stat fileinfo;
    off_t size = -1;

    if (0 == fstat(self->fd, &fileinfo))
        size = fileinfo.st_size;
    else {
        switch (errno) {
            case EBADF:
            case EINVAL:
                PyErr_SetString(pExistentialException,
                                "The segment does not exist");
            break;

            default:
                PyErr_SetFromErrno(PyExc_OSError);
            break;
        }

        goto error_return;
    }

    return Py_BuildValue("k", (unsigned long)size);

    error_return:
    return NULL;
}


PyObject *
SharedMemory_close_fd(SharedMemory *self) {
    if (self->fd) {
        if (-1 == close(self->fd)) {
            switch (errno) {
                case EBADF:
                    PyErr_SetString(PyExc_ValueError,
                                    "The file descriptor is invalid");
                break;

                default:
                    PyErr_SetFromErrno(PyExc_OSError);
                break;
            }

            goto error_return;
        }
    }

    Py_RETURN_NONE;

    error_return:
    return NULL;
}


PyObject *
SharedMemory_unlink(SharedMemory *self) {
    return my_shm_unlink(self->name);
}


/*   =====  End Shared Memory functions =====           */


/*
 *
 * Shared memory meta stuff for describing myself to Python
 *
 */


static PyMemberDef SharedMemory_members[] = {
    {   "name",
        T_STRING,
        offsetof(SharedMemory, name),
        READONLY,
        "The name specified in the constructor"
    },
    {   "fd",
        T_INT,
        offsetof(SharedMemory, fd),
        READONLY,
        "Shared memory segment file descriptor"
    },
    {   "mode",
        T_LONG,
        offsetof(SharedMemory, mode),
        READONLY,
        "The mode specified in the constructor"
    },
    {NULL} /* Sentinel */
};


static PyMethodDef SharedMemory_methods[] = {
    {   "close_fd",
        (PyCFunction)SharedMemory_close_fd,
        METH_NOARGS,
        "Closes the file descriptor associated with the shared memory."
    },
    {   "unlink",
        (PyCFunction)SharedMemory_unlink,
        METH_NOARGS,
        "Unlink (remove) the shared memory."
    },
    {NULL, NULL, 0, NULL} /* Sentinel */
};


static PyGetSetDef SharedMemory_getseters[] = {
    // size is read-only
    {   "size",
        (getter)SharedMemory_getsize,
        (setter)NULL,
        "size",
        NULL
    },
    {NULL} /* Sentinel */
};


static PyTypeObject SharedMemoryType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_posixshmem._PosixSharedMemory",   // tp_name
    sizeof(SharedMemory),               // tp_basicsize
    0,                                  // tp_itemsize
    (destructor) SharedMemory_dealloc,  // tp_dealloc
    0,                                  // tp_print
    0,                                  // tp_getattr
    0,                                  // tp_setattr
    0,                                  // tp_compare
    (reprfunc) shm_repr,                // tp_repr
    0,                                  // tp_as_number
    0,                                  // tp_as_sequence
    0,                                  // tp_as_mapping
    0,                                  // tp_hash
    0,                                  // tp_call
    (reprfunc) shm_str,                 // tp_str
    0,                                  // tp_getattro
    0,                                  // tp_setattro
    0,                                  // tp_as_buffer
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
                                        // tp_flags
    "POSIX shared memory object",       // tp_doc
    0,                                  // tp_traverse
    0,                                  // tp_clear
    0,                                  // tp_richcompare
    0,                                  // tp_weaklistoffset
    0,                                  // tp_iter
    0,                                  // tp_iternext
    SharedMemory_methods,               // tp_methods
    SharedMemory_members,               // tp_members
    SharedMemory_getseters,             // tp_getset
    0,                                  // tp_base
    0,                                  // tp_dict
    0,                                  // tp_descr_get
    0,                                  // tp_descr_set
    0,                                  // tp_dictoffset
    (initproc) SharedMemory_init,       // tp_init
    0,                                  // tp_alloc
    (newfunc) SharedMemory_new,         // tp_new
    0,                                  // tp_free
    0,                                  // tp_is_gc
    0                                   // tp_bases
};


/*
 *
 * Module-level functions & meta stuff
 *
 */

static PyObject *
posixshmem_unlink_shared_memory(PyObject *self, PyObject *args) {
    const char *name;

    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;
    else
        return my_shm_unlink(name);
}


static PyMethodDef module_methods[ ] = {
    {   "unlink_shared_memory",
        (PyCFunction)posixshmem_unlink_shared_memory,
        METH_VARARGS,
        "Unlink shared memory"
    },
    {NULL} /* Sentinel */
};


static struct PyModuleDef this_module = {
    PyModuleDef_HEAD_INIT,  // m_base
    "_posixshmem",          // m_name
    "POSIX shared memory module",     // m_doc
    -1,                     // m_size (space allocated for module globals)
    module_methods,         // m_methods
    NULL,                   // m_reload
    NULL,                   // m_traverse
    NULL,                   // m_clear
    NULL                    // m_free
};

/* Module init function */
PyMODINIT_FUNC
PyInit__posixshmem(void) {
    PyObject *module;
    PyObject *module_dict;

    // I call this in case I'm asked to create any random names.
    srand((unsigned int)time(NULL));

    module = PyModule_Create(&this_module);

    if (!module)
        goto error_return;

    if (PyType_Ready(&SharedMemoryType) < 0)
        goto error_return;

    Py_INCREF(&SharedMemoryType);
    PyModule_AddObject(module, "_PosixSharedMemory", (PyObject *)&SharedMemoryType);


    PyModule_AddStringConstant(module, "__copyright__", "Copyright 2012 Philip Semanchuk, 2018-2019 Davin Potts");

    PyModule_AddIntConstant(module, "O_CREAT", O_CREAT);
    PyModule_AddIntConstant(module, "O_EXCL", O_EXCL);
    PyModule_AddIntConstant(module, "O_CREX", O_CREAT | O_EXCL);
    PyModule_AddIntConstant(module, "O_TRUNC", O_TRUNC);

    if (!(module_dict = PyModule_GetDict(module)))
        goto error_return;

    // Exceptions
    if (!(pBaseException = PyErr_NewException("_posixshmem.Error", NULL, NULL)))
        goto error_return;
    else
        PyDict_SetItemString(module_dict, "Error", pBaseException);

    if (!(pPermissionsException = PyErr_NewException("_posixshmem.PermissionsError", pBaseException, NULL)))
        goto error_return;
    else
        PyDict_SetItemString(module_dict, "PermissionsError", pPermissionsException);

    if (!(pExistentialException = PyErr_NewException("_posixshmem.ExistentialError", pBaseException, NULL)))
        goto error_return;
    else
        PyDict_SetItemString(module_dict, "ExistentialError", pExistentialException);

    return module;

    error_return:
    return NULL;
}
