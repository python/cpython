#include "Python.h"
#include "asdl.h"

asdl_seq *
_Py_asdl_seq_new(Py_ssize_t size, PyArena *arena)
{
    asdl_seq *seq = NULL;
    size_t n;

    /* check size is sane */
    if (size < 0 ||
        (size && (((size_t)size - 1) > (PY_SIZE_MAX / sizeof(void *))))) {
        PyErr_NoMemory();
        return NULL;
    }
    n = (size ? (sizeof(void *) * (size - 1)) : 0);

    /* check if size can be added safely */
    if (n > PY_SIZE_MAX - sizeof(asdl_seq)) {
        PyErr_NoMemory();
        return NULL;
    }
    n += sizeof(asdl_seq);

    seq = (asdl_seq *)PyArena_Malloc(arena, n);
    if (!seq) {
        PyErr_NoMemory();
        return NULL;
    }
    memset(seq, 0, n);
    seq->size = size;
    return seq;
}

asdl_int_seq *
_Py_asdl_int_seq_new(Py_ssize_t size, PyArena *arena)
{
    asdl_int_seq *seq = NULL;
    size_t n;

    /* check size is sane */
    if (size < 0 ||
        (size && (((size_t)size - 1) > (PY_SIZE_MAX / sizeof(void *))))) {
            PyErr_NoMemory();
            return NULL;
    }
    n = (size ? (sizeof(void *) * (size - 1)) : 0);

    /* check if size can be added safely */
    if (n > PY_SIZE_MAX - sizeof(asdl_seq)) {
        PyErr_NoMemory();
        return NULL;
    }
    n += sizeof(asdl_seq);

    seq = (asdl_int_seq *)PyArena_Malloc(arena, n);
    if (!seq) {
        PyErr_NoMemory();
        return NULL;
    }
    memset(seq, 0, n);
    seq->size = size;
    return seq;
}
