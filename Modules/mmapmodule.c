/*
 /  Author: Sam Rushing <rushing@nightmare.com>
 /  Hacked for Unix by AMK
 /  $Id$

 / Modified to support mmap with offset - to map a 'window' of a file
 /   Author:  Yotam Medini  yotamm@mellanox.co.il
 /
 / mmapmodule.cpp -- map a view of a file into memory
 /
 / todo: need permission flags, perhaps a 'chsize' analog
 /   not all functions check range yet!!!
 /
 /
 / This version of mmapmodule.c has been changed significantly
 / from the original mmapfile.c on which it was based.
 / The original version of mmapfile is maintained by Sam at
 / ftp://squirl.nightmare.com/pub/python/python-ext.
*/

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

#ifndef MS_WINDOWS
#define UNIX
# ifdef HAVE_FCNTL_H
#  include <fcntl.h>
# endif /* HAVE_FCNTL_H */
#endif

#ifdef MS_WINDOWS
#include <windows.h>
static int
my_getpagesize(void)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
}

static int
my_getallocationgranularity (void)
{

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwAllocationGranularity;
}

#endif

#ifdef UNIX
#include <sys/mman.h>
#include <sys/stat.h>

#if defined(HAVE_SYSCONF) && defined(_SC_PAGESIZE)
static int
my_getpagesize(void)
{
    return sysconf(_SC_PAGESIZE);
}

#define my_getallocationgranularity my_getpagesize
#else
#define my_getpagesize getpagesize
#endif

#endif /* UNIX */

#include <string.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

/* Prefer MAP_ANONYMOUS since MAP_ANON is deprecated according to man page. */
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#  define MAP_ANONYMOUS MAP_ANON
#endif

typedef enum
{
    ACCESS_DEFAULT,
    ACCESS_READ,
    ACCESS_WRITE,
    ACCESS_COPY
} access_mode;

typedef struct {
    PyObject_HEAD
    char *      data;
    Py_ssize_t  size;
    Py_ssize_t  pos;    /* relative to offset */
#ifdef MS_WINDOWS
    long long offset;
#else
    off_t       offset;
#endif
    int     exports;

#ifdef MS_WINDOWS
    HANDLE      map_handle;
    HANDLE      file_handle;
    char *      tagname;
#endif

#ifdef UNIX
    int fd;
#endif

    PyObject *weakreflist;
    access_mode access;
} mmap_object;


static void
mmap_object_dealloc(mmap_object *m_obj)
{
#ifdef MS_WINDOWS
    if (m_obj->data != NULL)
        UnmapViewOfFile (m_obj->data);
    if (m_obj->map_handle != NULL)
        CloseHandle (m_obj->map_handle);
    if (m_obj->file_handle != INVALID_HANDLE_VALUE)
        CloseHandle (m_obj->file_handle);
    if (m_obj->tagname)
        PyMem_Free(m_obj->tagname);
#endif /* MS_WINDOWS */

#ifdef UNIX
    if (m_obj->fd >= 0)
        (void) close(m_obj->fd);
    if (m_obj->data!=NULL) {
        munmap(m_obj->data, m_obj->size);
    }
#endif /* UNIX */

    if (m_obj->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) m_obj);
    Py_TYPE(m_obj)->tp_free((PyObject*)m_obj);
}

static PyObject *
mmap_close_method(mmap_object *self, PyObject *unused)
{
    if (self->exports > 0) {
        PyErr_SetString(PyExc_BufferError, "cannot close "\
                        "exported pointers exist");
        return NULL;
    }
#ifdef MS_WINDOWS
    /* For each resource we maintain, we need to check
       the value is valid, and if so, free the resource
       and set the member value to an invalid value so
       the dealloc does not attempt to resource clearing
       again.
       TODO - should we check for errors in the close operations???
    */
    if (self->data != NULL) {
        UnmapViewOfFile(self->data);
        self->data = NULL;
    }
    if (self->map_handle != NULL) {
        CloseHandle(self->map_handle);
        self->map_handle = NULL;
    }
    if (self->file_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(self->file_handle);
        self->file_handle = INVALID_HANDLE_VALUE;
    }
#endif /* MS_WINDOWS */

#ifdef UNIX
    if (0 <= self->fd)
        (void) close(self->fd);
    self->fd = -1;
    if (self->data != NULL) {
        munmap(self->data, self->size);
        self->data = NULL;
    }
#endif

    Py_INCREF(Py_None);
    return Py_None;
}

#ifdef MS_WINDOWS
#define CHECK_VALID(err)                                                \
do {                                                                    \
    if (self->map_handle == NULL) {                                     \
    PyErr_SetString(PyExc_ValueError, "mmap closed or invalid");        \
    return err;                                                         \
    }                                                                   \
} while (0)
#endif /* MS_WINDOWS */

#ifdef UNIX
#define CHECK_VALID(err)                                                \
do {                                                                    \
    if (self->data == NULL) {                                           \
    PyErr_SetString(PyExc_ValueError, "mmap closed or invalid");        \
    return err;                                                         \
    }                                                                   \
} while (0)
#endif /* UNIX */

static PyObject *
mmap_read_byte_method(mmap_object *self,
                      PyObject *unused)
{
    CHECK_VALID(NULL);
    if (self->pos >= self->size) {
        PyErr_SetString(PyExc_ValueError, "read byte out of range");
        return NULL;
    }
    return PyLong_FromLong((unsigned char)self->data[self->pos++]);
}

static PyObject *
mmap_read_line_method(mmap_object *self,
                      PyObject *unused)
{
    Py_ssize_t remaining;
    char *start, *eol;
    PyObject *result;

    CHECK_VALID(NULL);

    remaining = (self->pos < self->size) ? self->size - self->pos : 0;
    if (!remaining)
        return PyBytes_FromString("");
    start = self->data + self->pos;
    eol = memchr(start, '\n', remaining);
    if (!eol)
        eol = self->data + self->size;
    else
        ++eol; /* advance past newline */
    result = PyBytes_FromStringAndSize(start, (eol - start));
    self->pos += (eol - start);
    return result;
}

/* Basically the "n" format code with the ability to turn None into -1. */
static int
mmap_convert_ssize_t(PyObject *obj, void *result) {
    Py_ssize_t limit;
    if (obj == Py_None) {
        limit = -1;
    }
    else if (PyNumber_Check(obj)) {
        limit = PyNumber_AsSsize_t(obj, PyExc_OverflowError);
        if (limit == -1 && PyErr_Occurred())
            return 0;
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "integer argument expected, got '%.200s'",
                     Py_TYPE(obj)->tp_name);
        return 0;
    }
    *((Py_ssize_t *)result) = limit;
    return 1;
}

static PyObject *
mmap_read_method(mmap_object *self,
                 PyObject *args)
{
    Py_ssize_t num_bytes = PY_SSIZE_T_MAX, remaining;
    PyObject *result;

    CHECK_VALID(NULL);
    if (!PyArg_ParseTuple(args, "|O&:read", mmap_convert_ssize_t, &num_bytes))
        return(NULL);

    /* silently 'adjust' out-of-range requests */
    remaining = (self->pos < self->size) ? self->size - self->pos : 0;
    if (num_bytes < 0 || num_bytes > remaining)
        num_bytes = remaining;
    result = PyBytes_FromStringAndSize(&self->data[self->pos], num_bytes);
    self->pos += num_bytes;
    return result;
}

static PyObject *
mmap_gfind(mmap_object *self,
           PyObject *args,
           int reverse)
{
    Py_ssize_t start = self->pos;
    Py_ssize_t end = self->size;
    Py_buffer view;

    CHECK_VALID(NULL);
    if (!PyArg_ParseTuple(args, reverse ? "y*|nn:rfind" : "y*|nn:find",
                          &view, &start, &end)) {
        return NULL;
    } else {
        const char *p, *start_p, *end_p;
        int sign = reverse ? -1 : 1;
        const char *needle = view.buf;
        Py_ssize_t len = view.len;

        if (start < 0)
            start += self->size;
        if (start < 0)
            start = 0;
        else if (start > self->size)
            start = self->size;

        if (end < 0)
            end += self->size;
        if (end < 0)
            end = 0;
        else if (end > self->size)
            end = self->size;

        start_p = self->data + start;
        end_p = self->data + end;

        for (p = (reverse ? end_p - len : start_p);
             (p >= start_p) && (p + len <= end_p); p += sign) {
            Py_ssize_t i;
            for (i = 0; i < len && needle[i] == p[i]; ++i)
                /* nothing */;
            if (i == len) {
                PyBuffer_Release(&view);
                return PyLong_FromSsize_t(p - self->data);
            }
        }
        PyBuffer_Release(&view);
        return PyLong_FromLong(-1);
    }
}

static PyObject *
mmap_find_method(mmap_object *self,
                 PyObject *args)
{
    return mmap_gfind(self, args, 0);
}

static PyObject *
mmap_rfind_method(mmap_object *self,
                 PyObject *args)
{
    return mmap_gfind(self, args, 1);
}

static int
is_writable(mmap_object *self)
{
    if (self->access != ACCESS_READ)
        return 1;
    PyErr_Format(PyExc_TypeError, "mmap can't modify a readonly memory map.");
    return 0;
}

static int
is_resizeable(mmap_object *self)
{
    if (self->exports > 0) {
        PyErr_SetString(PyExc_BufferError,
                        "mmap can't resize with extant buffers exported.");
        return 0;
    }
    if ((self->access == ACCESS_WRITE) || (self->access == ACCESS_DEFAULT))
        return 1;
    PyErr_Format(PyExc_TypeError,
                 "mmap can't resize a readonly or copy-on-write memory map.");
    return 0;
}


static PyObject *
mmap_write_method(mmap_object *self,
                  PyObject *args)
{
    Py_buffer data;

    CHECK_VALID(NULL);
    if (!PyArg_ParseTuple(args, "y*:write", &data))
        return(NULL);

    if (!is_writable(self)) {
        PyBuffer_Release(&data);
        return NULL;
    }

    if (self->pos > self->size || self->size - self->pos < data.len) {
        PyBuffer_Release(&data);
        PyErr_SetString(PyExc_ValueError, "data out of range");
        return NULL;
    }

    memcpy(&self->data[self->pos], data.buf, data.len);
    self->pos += data.len;
    PyBuffer_Release(&data);
    return PyLong_FromSsize_t(data.len);
}

static PyObject *
mmap_write_byte_method(mmap_object *self,
                       PyObject *args)
{
    char value;

    CHECK_VALID(NULL);
    if (!PyArg_ParseTuple(args, "b:write_byte", &value))
        return(NULL);

    if (!is_writable(self))
        return NULL;

    if (self->pos < self->size) {
        self->data[self->pos++] = value;
        Py_INCREF(Py_None);
        return Py_None;
    }
    else {
        PyErr_SetString(PyExc_ValueError, "write byte out of range");
        return NULL;
    }
}

static PyObject *
mmap_size_method(mmap_object *self,
                 PyObject *unused)
{
    CHECK_VALID(NULL);

#ifdef MS_WINDOWS
    if (self->file_handle != INVALID_HANDLE_VALUE) {
        DWORD low,high;
        long long size;
        low = GetFileSize(self->file_handle, &high);
        if (low == INVALID_FILE_SIZE) {
            /* It might be that the function appears to have failed,
               when indeed its size equals INVALID_FILE_SIZE */
            DWORD error = GetLastError();
            if (error != NO_ERROR)
                return PyErr_SetFromWindowsErr(error);
        }
        if (!high && low < LONG_MAX)
            return PyLong_FromLong((long)low);
        size = (((long long)high)<<32) + low;
        return PyLong_FromLongLong(size);
    } else {
        return PyLong_FromSsize_t(self->size);
    }
#endif /* MS_WINDOWS */

#ifdef UNIX
    {
        struct _Py_stat_struct status;
        if (_Py_fstat(self->fd, &status) == -1)
            return NULL;
#ifdef HAVE_LARGEFILE_SUPPORT
        return PyLong_FromLongLong(status.st_size);
#else
        return PyLong_FromLong(status.st_size);
#endif
    }
#endif /* UNIX */
}

/* This assumes that you want the entire file mapped,
 / and when recreating the map will make the new file
 / have the new size
 /
 / Is this really necessary?  This could easily be done
 / from python by just closing and re-opening with the
 / new size?
 */

static PyObject *
mmap_resize_method(mmap_object *self,
                   PyObject *args)
{
    Py_ssize_t new_size;
    CHECK_VALID(NULL);
    if (!PyArg_ParseTuple(args, "n:resize", &new_size) ||
        !is_resizeable(self)) {
        return NULL;
    }
    if (new_size < 0 || PY_SSIZE_T_MAX - new_size < self->offset) {
        PyErr_SetString(PyExc_ValueError, "new size out of range");
        return NULL;
    }

    {
#ifdef MS_WINDOWS
        DWORD dwErrCode = 0;
        DWORD off_hi, off_lo, newSizeLow, newSizeHigh;
        /* First, unmap the file view */
        UnmapViewOfFile(self->data);
        self->data = NULL;
        /* Close the mapping object */
        CloseHandle(self->map_handle);
        self->map_handle = NULL;
        /* Move to the desired EOF position */
        newSizeHigh = (DWORD)((self->offset + new_size) >> 32);
        newSizeLow = (DWORD)((self->offset + new_size) & 0xFFFFFFFF);
        off_hi = (DWORD)(self->offset >> 32);
        off_lo = (DWORD)(self->offset & 0xFFFFFFFF);
        SetFilePointer(self->file_handle,
                       newSizeLow, &newSizeHigh, FILE_BEGIN);
        /* Change the size of the file */
        SetEndOfFile(self->file_handle);
        /* Create another mapping object and remap the file view */
        self->map_handle = CreateFileMapping(
            self->file_handle,
            NULL,
            PAGE_READWRITE,
            0,
            0,
            self->tagname);
        if (self->map_handle != NULL) {
            self->data = (char *) MapViewOfFile(self->map_handle,
                                                FILE_MAP_WRITE,
                                                off_hi,
                                                off_lo,
                                                new_size);
            if (self->data != NULL) {
                self->size = new_size;
                Py_INCREF(Py_None);
                return Py_None;
            } else {
                dwErrCode = GetLastError();
                CloseHandle(self->map_handle);
                self->map_handle = NULL;
            }
        } else {
            dwErrCode = GetLastError();
        }
        PyErr_SetFromWindowsErr(dwErrCode);
        return NULL;
#endif /* MS_WINDOWS */

#ifdef UNIX
#ifndef HAVE_MREMAP
        PyErr_SetString(PyExc_SystemError,
                        "mmap: resizing not available--no mremap()");
        return NULL;
#else
        void *newmap;

        if (self->fd != -1 && ftruncate(self->fd, self->offset + new_size) == -1) {
            PyErr_SetFromErrno(PyExc_OSError);
            return NULL;
        }

#ifdef MREMAP_MAYMOVE
        newmap = mremap(self->data, self->size, new_size, MREMAP_MAYMOVE);
#else
#if defined(__NetBSD__)
        newmap = mremap(self->data, self->size, self->data, new_size, 0);
#else
        newmap = mremap(self->data, self->size, new_size, 0);
#endif /* __NetBSD__ */
#endif
        if (newmap == (void *)-1)
        {
            PyErr_SetFromErrno(PyExc_OSError);
            return NULL;
        }
        self->data = newmap;
        self->size = new_size;
        Py_INCREF(Py_None);
        return Py_None;
#endif /* HAVE_MREMAP */
#endif /* UNIX */
    }
}

static PyObject *
mmap_tell_method(mmap_object *self, PyObject *unused)
{
    CHECK_VALID(NULL);
    return PyLong_FromSize_t(self->pos);
}

static PyObject *
mmap_flush_method(mmap_object *self, PyObject *args)
{
    Py_ssize_t offset = 0;
    Py_ssize_t size = self->size;
    CHECK_VALID(NULL);
    if (!PyArg_ParseTuple(args, "|nn:flush", &offset, &size))
        return NULL;
    if (size < 0 || offset < 0 || self->size - offset < size) {
        PyErr_SetString(PyExc_ValueError, "flush values out of range");
        return NULL;
    }

    if (self->access == ACCESS_READ || self->access == ACCESS_COPY)
        return PyLong_FromLong(0);

#ifdef MS_WINDOWS
    return PyLong_FromLong((long) FlushViewOfFile(self->data+offset, size));
#elif defined(UNIX)
    /* XXX semantics of return value? */
    /* XXX flags for msync? */
    if (-1 == msync(self->data + offset, size, MS_SYNC)) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    return PyLong_FromLong(0);
#else
    PyErr_SetString(PyExc_ValueError, "flush not supported on this system");
    return NULL;
#endif
}

static PyObject *
mmap_seek_method(mmap_object *self, PyObject *args)
{
    Py_ssize_t dist;
    int how=0;
    CHECK_VALID(NULL);
    if (!PyArg_ParseTuple(args, "n|i:seek", &dist, &how))
        return NULL;
    else {
        Py_ssize_t where;
        switch (how) {
        case 0: /* relative to start */
            where = dist;
            break;
        case 1: /* relative to current position */
            if (PY_SSIZE_T_MAX - self->pos < dist)
                goto onoutofrange;
            where = self->pos + dist;
            break;
        case 2: /* relative to end */
            if (PY_SSIZE_T_MAX - self->size < dist)
                goto onoutofrange;
            where = self->size + dist;
            break;
        default:
            PyErr_SetString(PyExc_ValueError, "unknown seek type");
            return NULL;
        }
        if (where > self->size || where < 0)
            goto onoutofrange;
        self->pos = where;
        Py_INCREF(Py_None);
        return Py_None;
    }

  onoutofrange:
    PyErr_SetString(PyExc_ValueError, "seek out of range");
    return NULL;
}

static PyObject *
mmap_move_method(mmap_object *self, PyObject *args)
{
    Py_ssize_t dest, src, cnt;
    CHECK_VALID(NULL);
    if (!PyArg_ParseTuple(args, "nnn:move", &dest, &src, &cnt) ||
        !is_writable(self)) {
        return NULL;
    } else {
        /* bounds check the values */
        if (dest < 0 || src < 0 || cnt < 0)
            goto bounds;
        if (self->size - dest < cnt || self->size - src < cnt)
            goto bounds;

        memmove(&self->data[dest], &self->data[src], cnt);

        Py_INCREF(Py_None);
        return Py_None;

      bounds:
        PyErr_SetString(PyExc_ValueError,
                        "source, destination, or count out of range");
        return NULL;
    }
}

static PyObject *
mmap_closed_get(mmap_object *self)
{
#ifdef MS_WINDOWS
    return PyBool_FromLong(self->map_handle == NULL ? 1 : 0);
#elif defined(UNIX)
    return PyBool_FromLong(self->data == NULL ? 1 : 0);
#endif
}

static PyObject *
mmap__enter__method(mmap_object *self, PyObject *args)
{
    CHECK_VALID(NULL);

    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
mmap__exit__method(PyObject *self, PyObject *args)
{
    _Py_IDENTIFIER(close);

    return _PyObject_CallMethodId(self, &PyId_close, NULL);
}

#ifdef MS_WINDOWS
static PyObject *
mmap__sizeof__method(mmap_object *self, void *unused)
{
    Py_ssize_t res;

    res = _PyObject_SIZE(Py_TYPE(self));
    if (self->tagname)
        res += strlen(self->tagname) + 1;
    return PyLong_FromSsize_t(res);
}
#endif

static struct PyMethodDef mmap_object_methods[] = {
    {"close",           (PyCFunction) mmap_close_method,        METH_NOARGS},
    {"find",            (PyCFunction) mmap_find_method,         METH_VARARGS},
    {"rfind",           (PyCFunction) mmap_rfind_method,        METH_VARARGS},
    {"flush",           (PyCFunction) mmap_flush_method,        METH_VARARGS},
    {"move",            (PyCFunction) mmap_move_method,         METH_VARARGS},
    {"read",            (PyCFunction) mmap_read_method,         METH_VARARGS},
    {"read_byte",       (PyCFunction) mmap_read_byte_method,    METH_NOARGS},
    {"readline",        (PyCFunction) mmap_read_line_method,    METH_NOARGS},
    {"resize",          (PyCFunction) mmap_resize_method,       METH_VARARGS},
    {"seek",            (PyCFunction) mmap_seek_method,         METH_VARARGS},
    {"size",            (PyCFunction) mmap_size_method,         METH_NOARGS},
    {"tell",            (PyCFunction) mmap_tell_method,         METH_NOARGS},
    {"write",           (PyCFunction) mmap_write_method,        METH_VARARGS},
    {"write_byte",      (PyCFunction) mmap_write_byte_method,   METH_VARARGS},
    {"__enter__",       (PyCFunction) mmap__enter__method,      METH_NOARGS},
    {"__exit__",        (PyCFunction) mmap__exit__method,       METH_VARARGS},
#ifdef MS_WINDOWS
    {"__sizeof__",      (PyCFunction) mmap__sizeof__method,     METH_NOARGS},
#endif
    {NULL,         NULL}       /* sentinel */
};

static PyGetSetDef mmap_object_getset[] = {
    {"closed", (getter) mmap_closed_get, NULL, NULL},
    {NULL}
};


/* Functions for treating an mmap'ed file as a buffer */

static int
mmap_buffer_getbuf(mmap_object *self, Py_buffer *view, int flags)
{
    CHECK_VALID(-1);
    if (PyBuffer_FillInfo(view, (PyObject*)self, self->data, self->size,
                          (self->access == ACCESS_READ), flags) < 0)
        return -1;
    self->exports++;
    return 0;
}

static void
mmap_buffer_releasebuf(mmap_object *self, Py_buffer *view)
{
    self->exports--;
}

static Py_ssize_t
mmap_length(mmap_object *self)
{
    CHECK_VALID(-1);
    return self->size;
}

static PyObject *
mmap_item(mmap_object *self, Py_ssize_t i)
{
    CHECK_VALID(NULL);
    if (i < 0 || i >= self->size) {
        PyErr_SetString(PyExc_IndexError, "mmap index out of range");
        return NULL;
    }
    return PyBytes_FromStringAndSize(self->data + i, 1);
}

static PyObject *
mmap_subscript(mmap_object *self, PyObject *item)
{
    CHECK_VALID(NULL);
    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        if (i == -1 && PyErr_Occurred())
            return NULL;
        if (i < 0)
            i += self->size;
        if (i < 0 || i >= self->size) {
            PyErr_SetString(PyExc_IndexError,
                "mmap index out of range");
            return NULL;
        }
        return PyLong_FromLong(Py_CHARMASK(self->data[i]));
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelen;

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return NULL;
        }
        slicelen = PySlice_AdjustIndices(self->size, &start, &stop, step);

        if (slicelen <= 0)
            return PyBytes_FromStringAndSize("", 0);
        else if (step == 1)
            return PyBytes_FromStringAndSize(self->data + start,
                                              slicelen);
        else {
            char *result_buf = (char *)PyMem_Malloc(slicelen);
            Py_ssize_t cur, i;
            PyObject *result;

            if (result_buf == NULL)
                return PyErr_NoMemory();
            for (cur = start, i = 0; i < slicelen;
                 cur += step, i++) {
                result_buf[i] = self->data[cur];
            }
            result = PyBytes_FromStringAndSize(result_buf,
                                                slicelen);
            PyMem_Free(result_buf);
            return result;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "mmap indices must be integers");
        return NULL;
    }
}

static PyObject *
mmap_concat(mmap_object *self, PyObject *bb)
{
    CHECK_VALID(NULL);
    PyErr_SetString(PyExc_SystemError,
                    "mmaps don't support concatenation");
    return NULL;
}

static PyObject *
mmap_repeat(mmap_object *self, Py_ssize_t n)
{
    CHECK_VALID(NULL);
    PyErr_SetString(PyExc_SystemError,
                    "mmaps don't support repeat operation");
    return NULL;
}

static int
mmap_ass_item(mmap_object *self, Py_ssize_t i, PyObject *v)
{
    const char *buf;

    CHECK_VALID(-1);
    if (i < 0 || i >= self->size) {
        PyErr_SetString(PyExc_IndexError, "mmap index out of range");
        return -1;
    }
    if (v == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "mmap object doesn't support item deletion");
        return -1;
    }
    if (! (PyBytes_Check(v) && PyBytes_Size(v)==1) ) {
        PyErr_SetString(PyExc_IndexError,
                        "mmap assignment must be length-1 bytes()");
        return -1;
    }
    if (!is_writable(self))
        return -1;
    buf = PyBytes_AsString(v);
    self->data[i] = buf[0];
    return 0;
}

static int
mmap_ass_subscript(mmap_object *self, PyObject *item, PyObject *value)
{
    CHECK_VALID(-1);

    if (!is_writable(self))
        return -1;

    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        Py_ssize_t v;

        if (i == -1 && PyErr_Occurred())
            return -1;
        if (i < 0)
            i += self->size;
        if (i < 0 || i >= self->size) {
            PyErr_SetString(PyExc_IndexError,
                            "mmap index out of range");
            return -1;
        }
        if (value == NULL) {
            PyErr_SetString(PyExc_TypeError,
                            "mmap doesn't support item deletion");
            return -1;
        }
        if (!PyIndex_Check(value)) {
            PyErr_SetString(PyExc_TypeError,
                            "mmap item value must be an int");
            return -1;
        }
        v = PyNumber_AsSsize_t(value, PyExc_TypeError);
        if (v == -1 && PyErr_Occurred())
            return -1;
        if (v < 0 || v > 255) {
            PyErr_SetString(PyExc_ValueError,
                            "mmap item value must be "
                            "in range(0, 256)");
            return -1;
        }
        self->data[i] = (char) v;
        return 0;
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelen;
        Py_buffer vbuf;

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return -1;
        }
        slicelen = PySlice_AdjustIndices(self->size, &start, &stop, step);
        if (value == NULL) {
            PyErr_SetString(PyExc_TypeError,
                "mmap object doesn't support slice deletion");
            return -1;
        }
        if (PyObject_GetBuffer(value, &vbuf, PyBUF_SIMPLE) < 0)
            return -1;
        if (vbuf.len != slicelen) {
            PyErr_SetString(PyExc_IndexError,
                "mmap slice assignment is wrong size");
            PyBuffer_Release(&vbuf);
            return -1;
        }

        if (slicelen == 0) {
        }
        else if (step == 1) {
            memcpy(self->data + start, vbuf.buf, slicelen);
        }
        else {
            Py_ssize_t cur, i;

            for (cur = start, i = 0;
                 i < slicelen;
                 cur += step, i++)
            {
                self->data[cur] = ((char *)vbuf.buf)[i];
            }
        }
        PyBuffer_Release(&vbuf);
        return 0;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "mmap indices must be integer");
        return -1;
    }
}

static PySequenceMethods mmap_as_sequence = {
    (lenfunc)mmap_length,            /*sq_length*/
    (binaryfunc)mmap_concat,         /*sq_concat*/
    (ssizeargfunc)mmap_repeat,       /*sq_repeat*/
    (ssizeargfunc)mmap_item,         /*sq_item*/
    0,                               /*sq_slice*/
    (ssizeobjargproc)mmap_ass_item,  /*sq_ass_item*/
    0,                               /*sq_ass_slice*/
};

static PyMappingMethods mmap_as_mapping = {
    (lenfunc)mmap_length,
    (binaryfunc)mmap_subscript,
    (objobjargproc)mmap_ass_subscript,
};

static PyBufferProcs mmap_as_buffer = {
    (getbufferproc)mmap_buffer_getbuf,
    (releasebufferproc)mmap_buffer_releasebuf,
};

static PyObject *
new_mmap_object(PyTypeObject *type, PyObject *args, PyObject *kwdict);

PyDoc_STRVAR(mmap_doc,
"Windows: mmap(fileno, length[, tagname[, access[, offset]]])\n\
\n\
Maps length bytes from the file specified by the file handle fileno,\n\
and returns a mmap object.  If length is larger than the current size\n\
of the file, the file is extended to contain length bytes.  If length\n\
is 0, the maximum length of the map is the current size of the file,\n\
except that if the file is empty Windows raises an exception (you cannot\n\
create an empty mapping on Windows).\n\
\n\
Unix: mmap(fileno, length[, flags[, prot[, access[, offset]]]])\n\
\n\
Maps length bytes from the file specified by the file descriptor fileno,\n\
and returns a mmap object.  If length is 0, the maximum length of the map\n\
will be the current size of the file when mmap is called.\n\
flags specifies the nature of the mapping. MAP_PRIVATE creates a\n\
private copy-on-write mapping, so changes to the contents of the mmap\n\
object will be private to this process, and MAP_SHARED creates a mapping\n\
that's shared with all other processes mapping the same areas of the file.\n\
The default value is MAP_SHARED.\n\
\n\
To map anonymous memory, pass -1 as the fileno (both versions).");


static PyTypeObject mmap_object_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "mmap.mmap",                                /* tp_name */
    sizeof(mmap_object),                        /* tp_size */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor) mmap_object_dealloc,           /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    &mmap_as_sequence,                          /*tp_as_sequence*/
    &mmap_as_mapping,                           /*tp_as_mapping*/
    0,                                          /*tp_hash*/
    0,                                          /*tp_call*/
    0,                                          /*tp_str*/
    PyObject_GenericGetAttr,                    /*tp_getattro*/
    0,                                          /*tp_setattro*/
    &mmap_as_buffer,                            /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /*tp_flags*/
    mmap_doc,                                   /*tp_doc*/
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    offsetof(mmap_object, weakreflist),         /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    mmap_object_methods,                        /* tp_methods */
    0,                                          /* tp_members */
    mmap_object_getset,                         /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    new_mmap_object,                            /* tp_new */
    PyObject_Del,                               /* tp_free */
};


#ifdef UNIX
#ifdef HAVE_LARGEFILE_SUPPORT
#define _Py_PARSE_OFF_T "L"
#else
#define _Py_PARSE_OFF_T "l"
#endif

static PyObject *
new_mmap_object(PyTypeObject *type, PyObject *args, PyObject *kwdict)
{
    struct _Py_stat_struct status;
    mmap_object *m_obj;
    Py_ssize_t map_size;
    off_t offset = 0;
    int fd, flags = MAP_SHARED, prot = PROT_WRITE | PROT_READ;
    int devzero = -1;
    int access = (int)ACCESS_DEFAULT;
    static char *keywords[] = {"fileno", "length",
                               "flags", "prot",
                               "access", "offset", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwdict, "in|iii" _Py_PARSE_OFF_T, keywords,
                                     &fd, &map_size, &flags, &prot,
                                     &access, &offset))
        return NULL;
    if (map_size < 0) {
        PyErr_SetString(PyExc_OverflowError,
                        "memory mapped length must be postiive");
        return NULL;
    }
    if (offset < 0) {
        PyErr_SetString(PyExc_OverflowError,
            "memory mapped offset must be positive");
        return NULL;
    }

    if ((access != (int)ACCESS_DEFAULT) &&
        ((flags != MAP_SHARED) || (prot != (PROT_WRITE | PROT_READ))))
        return PyErr_Format(PyExc_ValueError,
                            "mmap can't specify both access and flags, prot.");
    switch ((access_mode)access) {
    case ACCESS_READ:
        flags = MAP_SHARED;
        prot = PROT_READ;
        break;
    case ACCESS_WRITE:
        flags = MAP_SHARED;
        prot = PROT_READ | PROT_WRITE;
        break;
    case ACCESS_COPY:
        flags = MAP_PRIVATE;
        prot = PROT_READ | PROT_WRITE;
        break;
    case ACCESS_DEFAULT:
        /* map prot to access type */
        if ((prot & PROT_READ) && (prot & PROT_WRITE)) {
            /* ACCESS_DEFAULT */
        }
        else if (prot & PROT_WRITE) {
            access = ACCESS_WRITE;
        }
        else {
            access = ACCESS_READ;
        }
        break;
    default:
        return PyErr_Format(PyExc_ValueError,
                            "mmap invalid access parameter.");
    }

#ifdef __APPLE__
    /* Issue #11277: fsync(2) is not enough on OS X - a special, OS X specific
       fcntl(2) is necessary to force DISKSYNC and get around mmap(2) bug */
    if (fd != -1)
        (void)fcntl(fd, F_FULLFSYNC);
#endif
    if (fd != -1 && _Py_fstat_noraise(fd, &status) == 0
        && S_ISREG(status.st_mode)) {
        if (map_size == 0) {
            if (status.st_size == 0) {
                PyErr_SetString(PyExc_ValueError,
                                "cannot mmap an empty file");
                return NULL;
            }
            if (offset >= status.st_size) {
                PyErr_SetString(PyExc_ValueError,
                                "mmap offset is greater than file size");
                return NULL;
            }
            if (status.st_size - offset > PY_SSIZE_T_MAX) {
                PyErr_SetString(PyExc_ValueError,
                                 "mmap length is too large");
                return NULL;
            }
            map_size = (Py_ssize_t) (status.st_size - offset);
        } else if (offset > status.st_size || status.st_size - offset < map_size) {
            PyErr_SetString(PyExc_ValueError,
                            "mmap length is greater than file size");
            return NULL;
        }
    }
    m_obj = (mmap_object *)type->tp_alloc(type, 0);
    if (m_obj == NULL) {return NULL;}
    m_obj->data = NULL;
    m_obj->size = map_size;
    m_obj->pos = 0;
    m_obj->weakreflist = NULL;
    m_obj->exports = 0;
    m_obj->offset = offset;
    if (fd == -1) {
        m_obj->fd = -1;
        /* Assume the caller wants to map anonymous memory.
           This is the same behaviour as Windows.  mmap.mmap(-1, size)
           on both Windows and Unix map anonymous memory.
        */
#ifdef MAP_ANONYMOUS
        /* BSD way to map anonymous memory */
        flags |= MAP_ANONYMOUS;
#else
        /* SVR4 method to map anonymous memory is to open /dev/zero */
        fd = devzero = _Py_open("/dev/zero", O_RDWR);
        if (devzero == -1) {
            Py_DECREF(m_obj);
            return NULL;
        }
#endif
    }
    else {
        m_obj->fd = _Py_dup(fd);
        if (m_obj->fd == -1) {
            Py_DECREF(m_obj);
            return NULL;
        }
    }

    m_obj->data = mmap(NULL, map_size,
                       prot, flags,
                       fd, offset);

    if (devzero != -1) {
        close(devzero);
    }

    if (m_obj->data == (char *)-1) {
        m_obj->data = NULL;
        Py_DECREF(m_obj);
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    m_obj->access = (access_mode)access;
    return (PyObject *)m_obj;
}
#endif /* UNIX */

#ifdef MS_WINDOWS

/* A note on sizes and offsets: while the actual map size must hold in a
   Py_ssize_t, both the total file size and the start offset can be longer
   than a Py_ssize_t, so we use long long which is always 64-bit.
*/

static PyObject *
new_mmap_object(PyTypeObject *type, PyObject *args, PyObject *kwdict)
{
    mmap_object *m_obj;
    Py_ssize_t map_size;
    long long offset = 0, size;
    DWORD off_hi;       /* upper 32 bits of offset */
    DWORD off_lo;       /* lower 32 bits of offset */
    DWORD size_hi;      /* upper 32 bits of size */
    DWORD size_lo;      /* lower 32 bits of size */
    char *tagname = "";
    DWORD dwErr = 0;
    int fileno;
    HANDLE fh = 0;
    int access = (access_mode)ACCESS_DEFAULT;
    DWORD flProtect, dwDesiredAccess;
    static char *keywords[] = { "fileno", "length",
                                "tagname",
                                "access", "offset", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwdict, "in|ziL", keywords,
                                     &fileno, &map_size,
                                     &tagname, &access, &offset)) {
        return NULL;
    }

    switch((access_mode)access) {
    case ACCESS_READ:
        flProtect = PAGE_READONLY;
        dwDesiredAccess = FILE_MAP_READ;
        break;
    case ACCESS_DEFAULT:  case ACCESS_WRITE:
        flProtect = PAGE_READWRITE;
        dwDesiredAccess = FILE_MAP_WRITE;
        break;
    case ACCESS_COPY:
        flProtect = PAGE_WRITECOPY;
        dwDesiredAccess = FILE_MAP_COPY;
        break;
    default:
        return PyErr_Format(PyExc_ValueError,
                            "mmap invalid access parameter.");
    }

    if (map_size < 0) {
        PyErr_SetString(PyExc_OverflowError,
                        "memory mapped length must be postiive");
        return NULL;
    }
    if (offset < 0) {
        PyErr_SetString(PyExc_OverflowError,
            "memory mapped offset must be positive");
        return NULL;
    }

    /* assume -1 and 0 both mean invalid filedescriptor
       to 'anonymously' map memory.
       XXX: fileno == 0 is a valid fd, but was accepted prior to 2.5.
       XXX: Should this code be added?
       if (fileno == 0)
        PyErr_WarnEx(PyExc_DeprecationWarning,
                     "don't use 0 for anonymous memory",
                     1);
     */
    if (fileno != -1 && fileno != 0) {
        /* Ensure that fileno is within the CRT's valid range */
        _Py_BEGIN_SUPPRESS_IPH
        fh = (HANDLE)_get_osfhandle(fileno);
        _Py_END_SUPPRESS_IPH
        if (fh==(HANDLE)-1) {
            PyErr_SetFromErrno(PyExc_OSError);
            return NULL;
        }
        /* Win9x appears to need us seeked to zero */
        lseek(fileno, 0, SEEK_SET);
    }

    m_obj = (mmap_object *)type->tp_alloc(type, 0);
    if (m_obj == NULL)
        return NULL;
    /* Set every field to an invalid marker, so we can safely
       destruct the object in the face of failure */
    m_obj->data = NULL;
    m_obj->file_handle = INVALID_HANDLE_VALUE;
    m_obj->map_handle = NULL;
    m_obj->tagname = NULL;
    m_obj->offset = offset;

    if (fh) {
        /* It is necessary to duplicate the handle, so the
           Python code can close it on us */
        if (!DuplicateHandle(
            GetCurrentProcess(), /* source process handle */
            fh, /* handle to be duplicated */
            GetCurrentProcess(), /* target proc handle */
            (LPHANDLE)&m_obj->file_handle, /* result */
            0, /* access - ignored due to options value */
            FALSE, /* inherited by child processes? */
            DUPLICATE_SAME_ACCESS)) { /* options */
            dwErr = GetLastError();
            Py_DECREF(m_obj);
            PyErr_SetFromWindowsErr(dwErr);
            return NULL;
        }
        if (!map_size) {
            DWORD low,high;
            low = GetFileSize(fh, &high);
            /* low might just happen to have the value INVALID_FILE_SIZE;
               so we need to check the last error also. */
            if (low == INVALID_FILE_SIZE &&
                (dwErr = GetLastError()) != NO_ERROR) {
                Py_DECREF(m_obj);
                return PyErr_SetFromWindowsErr(dwErr);
            }

            size = (((long long) high) << 32) + low;
            if (size == 0) {
                PyErr_SetString(PyExc_ValueError,
                                "cannot mmap an empty file");
                Py_DECREF(m_obj);
                return NULL;
            }
            if (offset >= size) {
                PyErr_SetString(PyExc_ValueError,
                                "mmap offset is greater than file size");
                Py_DECREF(m_obj);
                return NULL;
            }
            if (size - offset > PY_SSIZE_T_MAX) {
                PyErr_SetString(PyExc_ValueError,
                                "mmap length is too large");
                Py_DECREF(m_obj);
                return NULL;
            }
            m_obj->size = (Py_ssize_t) (size - offset);
        } else {
            m_obj->size = map_size;
            size = offset + map_size;
        }
    }
    else {
        m_obj->size = map_size;
        size = offset + map_size;
    }

    /* set the initial position */
    m_obj->pos = (size_t) 0;

    m_obj->weakreflist = NULL;
    m_obj->exports = 0;
    /* set the tag name */
    if (tagname != NULL && *tagname != '\0') {
        m_obj->tagname = PyMem_Malloc(strlen(tagname)+1);
        if (m_obj->tagname == NULL) {
            PyErr_NoMemory();
            Py_DECREF(m_obj);
            return NULL;
        }
        strcpy(m_obj->tagname, tagname);
    }
    else
        m_obj->tagname = NULL;

    m_obj->access = (access_mode)access;
    size_hi = (DWORD)(size >> 32);
    size_lo = (DWORD)(size & 0xFFFFFFFF);
    off_hi = (DWORD)(offset >> 32);
    off_lo = (DWORD)(offset & 0xFFFFFFFF);
    /* For files, it would be sufficient to pass 0 as size.
       For anonymous maps, we have to pass the size explicitly. */
    m_obj->map_handle = CreateFileMapping(m_obj->file_handle,
                                          NULL,
                                          flProtect,
                                          size_hi,
                                          size_lo,
                                          m_obj->tagname);
    if (m_obj->map_handle != NULL) {
        m_obj->data = (char *) MapViewOfFile(m_obj->map_handle,
                                             dwDesiredAccess,
                                             off_hi,
                                             off_lo,
                                             m_obj->size);
        if (m_obj->data != NULL)
            return (PyObject *)m_obj;
        else {
            dwErr = GetLastError();
            CloseHandle(m_obj->map_handle);
            m_obj->map_handle = NULL;
        }
    } else
        dwErr = GetLastError();
    Py_DECREF(m_obj);
    PyErr_SetFromWindowsErr(dwErr);
    return NULL;
}
#endif /* MS_WINDOWS */

static void
setint(PyObject *d, const char *name, long value)
{
    PyObject *o = PyLong_FromLong(value);
    if (o && PyDict_SetItemString(d, name, o) == 0) {
        Py_DECREF(o);
    }
}


static struct PyModuleDef mmapmodule = {
    PyModuleDef_HEAD_INIT,
    "mmap",
    NULL,
    -1,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_mmap(void)
{
    PyObject *dict, *module;

    if (PyType_Ready(&mmap_object_type) < 0)
        return NULL;

    module = PyModule_Create(&mmapmodule);
    if (module == NULL)
        return NULL;
    dict = PyModule_GetDict(module);
    if (!dict)
        return NULL;
    PyDict_SetItemString(dict, "error", PyExc_OSError);
    PyDict_SetItemString(dict, "mmap", (PyObject*) &mmap_object_type);
#ifdef PROT_EXEC
    setint(dict, "PROT_EXEC", PROT_EXEC);
#endif
#ifdef PROT_READ
    setint(dict, "PROT_READ", PROT_READ);
#endif
#ifdef PROT_WRITE
    setint(dict, "PROT_WRITE", PROT_WRITE);
#endif

#ifdef MAP_SHARED
    setint(dict, "MAP_SHARED", MAP_SHARED);
#endif
#ifdef MAP_PRIVATE
    setint(dict, "MAP_PRIVATE", MAP_PRIVATE);
#endif
#ifdef MAP_DENYWRITE
    setint(dict, "MAP_DENYWRITE", MAP_DENYWRITE);
#endif
#ifdef MAP_EXECUTABLE
    setint(dict, "MAP_EXECUTABLE", MAP_EXECUTABLE);
#endif
#ifdef MAP_ANONYMOUS
    setint(dict, "MAP_ANON", MAP_ANONYMOUS);
    setint(dict, "MAP_ANONYMOUS", MAP_ANONYMOUS);
#endif

    setint(dict, "PAGESIZE", (long)my_getpagesize());

    setint(dict, "ALLOCATIONGRANULARITY", (long)my_getallocationgranularity());

    setint(dict, "ACCESS_READ", ACCESS_READ);
    setint(dict, "ACCESS_WRITE", ACCESS_WRITE);
    setint(dict, "ACCESS_COPY", ACCESS_COPY);
    return module;
}
