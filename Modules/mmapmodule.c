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

static PyObject *mmap_module_error;

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
    PY_LONG_LONG offset;
#else
    off_t       offset;
#endif

#ifdef MS_WINDOWS
    HANDLE      map_handle;
    HANDLE      file_handle;
    char *      tagname;
#endif

#ifdef UNIX
    int fd;
#endif

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
        if (m_obj->access != ACCESS_READ && m_obj->access != ACCESS_COPY)
            msync(m_obj->data, m_obj->size, MS_SYNC);
        munmap(m_obj->data, m_obj->size);
    }
#endif /* UNIX */

    Py_TYPE(m_obj)->tp_free((PyObject*)m_obj);
}

static PyObject *
mmap_close_method(mmap_object *self, PyObject *unused)
{
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
    return PyString_FromStringAndSize(&self->data[self->pos++], 1);
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
        return PyString_FromString("");
    start = self->data + self->pos;
    eol = memchr(start, '\n', remaining);
    if (!eol)
        eol = self->data + self->size;
    else
        ++eol; /* advance past newline */
    result = PyString_FromStringAndSize(start, (eol - start));
    self->pos += (eol - start);
    return result;
}

static PyObject *
mmap_read_method(mmap_object *self,
                 PyObject *args)
{
    Py_ssize_t num_bytes, remaining;
    PyObject *result;

    CHECK_VALID(NULL);
    if (!PyArg_ParseTuple(args, "n:read", &num_bytes))
        return NULL;

    /* silently 'adjust' out-of-range requests */
    remaining = (self->pos < self->size) ? self->size - self->pos : 0;
    if (num_bytes < 0 || num_bytes > remaining)
        num_bytes = remaining;
    result = PyString_FromStringAndSize(&self->data[self->pos], num_bytes);
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
    const char *needle;
    Py_ssize_t len;

    CHECK_VALID(NULL);
    if (!PyArg_ParseTuple(args, reverse ? "s#|nn:rfind" : "s#|nn:find",
                          &needle, &len, &start, &end)) {
        return NULL;
    } else {
        const char *p, *start_p, *end_p;
        int sign = reverse ? -1 : 1;

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
                return PyInt_FromSsize_t(p - self->data);
            }
        }
        return PyInt_FromLong(-1);
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
is_writeable(mmap_object *self)
{
    if (self->access != ACCESS_READ)
        return 1;
    PyErr_Format(PyExc_TypeError, "mmap can't modify a readonly memory map.");
    return 0;
}

static int
is_resizeable(mmap_object *self)
{
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
    Py_ssize_t length;
    char *data;

    CHECK_VALID(NULL);
    if (!PyArg_ParseTuple(args, "s#:write", &data, &length))
        return(NULL);

    if (!is_writeable(self))
        return NULL;

    if (self->pos > self->size || self->size - self->pos < length) {
        PyErr_SetString(PyExc_ValueError, "data out of range");
        return NULL;
    }
    memcpy(&self->data[self->pos], data, length);
    self->pos += length;
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
mmap_write_byte_method(mmap_object *self,
                       PyObject *args)
{
    char value;

    CHECK_VALID(NULL);
    if (!PyArg_ParseTuple(args, "c:write_byte", &value))
        return(NULL);

    if (!is_writeable(self))
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
        PY_LONG_LONG size;
        low = GetFileSize(self->file_handle, &high);
        if (low == INVALID_FILE_SIZE) {
            /* It might be that the function appears to have failed,
               when indeed its size equals INVALID_FILE_SIZE */
            DWORD error = GetLastError();
            if (error != NO_ERROR)
                return PyErr_SetFromWindowsErr(error);
        }
        if (!high && low < LONG_MAX)
            return PyInt_FromLong((long)low);
        size = (((PY_LONG_LONG)high)<<32) + low;
        return PyLong_FromLongLong(size);
    } else {
        return PyInt_FromSsize_t(self->size);
    }
#endif /* MS_WINDOWS */

#ifdef UNIX
    {
        struct stat buf;
        if (-1 == fstat(self->fd, &buf)) {
            PyErr_SetFromErrno(mmap_module_error);
            return NULL;
        }
#ifdef HAVE_LARGEFILE_SUPPORT
        return PyLong_FromLongLong(buf.st_size);
#else
        return PyInt_FromLong(buf.st_size);
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
            PyErr_SetFromErrno(mmap_module_error);
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
            PyErr_SetFromErrno(mmap_module_error);
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
    return PyInt_FromSize_t(self->pos);
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
    return PyInt_FromLong((long) FlushViewOfFile(self->data+offset, size));
#elif defined(UNIX)
    /* XXX semantics of return value? */
    /* XXX flags for msync? */
    if (-1 == msync(self->data + offset, size, MS_SYNC)) {
        PyErr_SetFromErrno(mmap_module_error);
        return NULL;
    }
    return PyInt_FromLong(0);
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
        !is_writeable(self)) {
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
#ifdef MS_WINDOWS
    {"__sizeof__",      (PyCFunction) mmap__sizeof__method,     METH_NOARGS},
#endif
    {NULL,         NULL}       /* sentinel */
};

/* Functions for treating an mmap'ed file as a buffer */

static Py_ssize_t
mmap_buffer_getreadbuf(mmap_object *self, Py_ssize_t index, const void **ptr)
{
    CHECK_VALID(-1);
    if (index != 0) {
        PyErr_SetString(PyExc_SystemError,
                        "Accessing non-existent mmap segment");
        return -1;
    }
    *ptr = self->data;
    return self->size;
}

static Py_ssize_t
mmap_buffer_getwritebuf(mmap_object *self, Py_ssize_t index, const void **ptr)
{
    CHECK_VALID(-1);
    if (index != 0) {
        PyErr_SetString(PyExc_SystemError,
                        "Accessing non-existent mmap segment");
        return -1;
    }
    if (!is_writeable(self))
        return -1;
    *ptr = self->data;
    return self->size;
}

static Py_ssize_t
mmap_buffer_getsegcount(mmap_object *self, Py_ssize_t *lenp)
{
    CHECK_VALID(-1);
    if (lenp)
        *lenp = self->size;
    return 1;
}

static Py_ssize_t
mmap_buffer_getcharbuffer(mmap_object *self, Py_ssize_t index, const void **ptr)
{
    if (index != 0) {
        PyErr_SetString(PyExc_SystemError,
                        "accessing non-existent buffer segment");
        return -1;
    }
    *ptr = (const char *)self->data;
    return self->size;
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
    return PyString_FromStringAndSize(self->data + i, 1);
}

static PyObject *
mmap_slice(mmap_object *self, Py_ssize_t ilow, Py_ssize_t ihigh)
{
    CHECK_VALID(NULL);
    if (ilow < 0)
        ilow = 0;
    else if (ilow > self->size)
        ilow = self->size;
    if (ihigh < 0)
        ihigh = 0;
    if (ihigh < ilow)
        ihigh = ilow;
    else if (ihigh > self->size)
        ihigh = self->size;

    return PyString_FromStringAndSize(self->data + ilow, ihigh-ilow);
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
        return PyString_FromStringAndSize(self->data + i, 1);
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelen;

        if (_PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return NULL;
        }
        slicelen = _PySlice_AdjustIndices(self->size, &start, &stop, step);

        if (slicelen <= 0)
            return PyString_FromStringAndSize("", 0);
        else if (step == 1)
            return PyString_FromStringAndSize(self->data + start,
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
            result = PyString_FromStringAndSize(result_buf,
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

static int
mmap_ass_slice(mmap_object *self, Py_ssize_t ilow, Py_ssize_t ihigh, PyObject *v)
{
    const char *buf;

    CHECK_VALID(-1);
    if (ilow < 0)
        ilow = 0;
    else if (ilow > self->size)
        ilow = self->size;
    if (ihigh < 0)
        ihigh = 0;
    if (ihigh < ilow)
        ihigh = ilow;
    else if (ihigh > self->size)
        ihigh = self->size;

    if (v == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "mmap object doesn't support slice deletion");
        return -1;
    }
    if (! (PyString_Check(v)) ) {
        PyErr_SetString(PyExc_IndexError,
                        "mmap slice assignment must be a string");
        return -1;
    }
    if (PyString_Size(v) != (ihigh - ilow)) {
        PyErr_SetString(PyExc_IndexError,
                        "mmap slice assignment is wrong size");
        return -1;
    }
    if (!is_writeable(self))
        return -1;
    buf = PyString_AsString(v);
    memcpy(self->data + ilow, buf, ihigh-ilow);
    return 0;
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
    if (! (PyString_Check(v) && PyString_Size(v)==1) ) {
        PyErr_SetString(PyExc_IndexError,
                        "mmap assignment must be single-character string");
        return -1;
    }
    if (!is_writeable(self))
        return -1;
    buf = PyString_AsString(v);
    self->data[i] = buf[0];
    return 0;
}

static int
mmap_ass_subscript(mmap_object *self, PyObject *item, PyObject *value)
{
    CHECK_VALID(-1);

    if (PyIndex_Check(item)) {
        Py_ssize_t i = PyNumber_AsSsize_t(item, PyExc_IndexError);
        const char *buf;

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
                "mmap object doesn't support item deletion");
            return -1;
        }
        if (!PyString_Check(value) || PyString_Size(value) != 1) {
            PyErr_SetString(PyExc_IndexError,
              "mmap assignment must be single-character string");
            return -1;
        }
        if (!is_writeable(self))
            return -1;
        buf = PyString_AsString(value);
        self->data[i] = buf[0];
        return 0;
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelen;

        if (_PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return -1;
        }
        slicelen = _PySlice_AdjustIndices(self->size, &start, &stop, step);
        if (value == NULL) {
            PyErr_SetString(PyExc_TypeError,
                "mmap object doesn't support slice deletion");
            return -1;
        }
        if (!PyString_Check(value)) {
            PyErr_SetString(PyExc_IndexError,
                "mmap slice assignment must be a string");
            return -1;
        }
        if (PyString_Size(value) != slicelen) {
            PyErr_SetString(PyExc_IndexError,
                "mmap slice assignment is wrong size");
            return -1;
        }
        if (!is_writeable(self))
            return -1;

        if (slicelen == 0)
            return 0;
        else if (step == 1) {
            const char *buf = PyString_AsString(value);

            if (buf == NULL)
                return -1;
            memcpy(self->data + start, buf, slicelen);
            return 0;
        }
        else {
            Py_ssize_t cur, i;
            const char *buf = PyString_AsString(value);

            if (buf == NULL)
                return -1;
            for (cur = start, i = 0; i < slicelen;
                 cur += step, i++) {
                self->data[cur] = buf[i];
            }
            return 0;
        }
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "mmap indices must be integer");
        return -1;
    }
}

static PySequenceMethods mmap_as_sequence = {
    (lenfunc)mmap_length,                      /*sq_length*/
    0,                                         /*sq_concat*/
    0,                                         /*sq_repeat*/
    (ssizeargfunc)mmap_item,                   /*sq_item*/
    (ssizessizeargfunc)mmap_slice,             /*sq_slice*/
    (ssizeobjargproc)mmap_ass_item,            /*sq_ass_item*/
    (ssizessizeobjargproc)mmap_ass_slice,      /*sq_ass_slice*/
};

static PyMappingMethods mmap_as_mapping = {
    (lenfunc)mmap_length,
    (binaryfunc)mmap_subscript,
    (objobjargproc)mmap_ass_subscript,
};

static PyBufferProcs mmap_as_buffer = {
    (readbufferproc)mmap_buffer_getreadbuf,
    (writebufferproc)mmap_buffer_getwritebuf,
    (segcountproc)mmap_buffer_getsegcount,
    (charbufferproc)mmap_buffer_getcharbuffer,
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
    sizeof(mmap_object),                        /* tp_basicsize */
    0,                                          /* tp_itemsize */
    /* methods */
    (destructor) mmap_object_dealloc,           /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_compare */
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
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GETCHARBUFFER,                   /*tp_flags*/
    mmap_doc,                                   /*tp_doc*/
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    mmap_object_methods,                        /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                      /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    new_mmap_object,                            /* tp_new */
    PyObject_Del,                           /* tp_free */
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
#ifdef HAVE_FSTAT
    struct stat st;
#endif
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
                        "memory mapped length must be positive");
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
#ifdef HAVE_FSTAT
#  ifdef __VMS
    /* on OpenVMS we must ensure that all bytes are written to the file */
    if (fd != -1) {
        fsync(fd);
    }
#  endif
    if (fd != -1 && fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) {
        if (map_size == 0) {
            if (st.st_size == 0) {
                PyErr_SetString(PyExc_ValueError,
                                "cannot mmap an empty file");
                return NULL;
            }
            if (offset >= st.st_size) {
                PyErr_SetString(PyExc_ValueError,
                                "mmap offset is greater than file size");
                return NULL;
            }
            if (st.st_size - offset > PY_SSIZE_T_MAX) {
                PyErr_SetString(PyExc_ValueError,
                                 "mmap length is too large");
                return NULL;
            }
            map_size = (Py_ssize_t) (st.st_size - offset);
        } else if (offset > st.st_size || st.st_size - offset < map_size) {
            PyErr_SetString(PyExc_ValueError,
                            "mmap length is greater than file size");
            return NULL;
        }
    }
#endif
    m_obj = (mmap_object *)type->tp_alloc(type, 0);
    if (m_obj == NULL) {return NULL;}
    m_obj->data = NULL;
    m_obj->size = map_size;
    m_obj->pos = 0;
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
        fd = devzero = open("/dev/zero", O_RDWR);
        if (devzero == -1) {
            Py_DECREF(m_obj);
            PyErr_SetFromErrno(mmap_module_error);
            return NULL;
        }
#endif
    } else {
        m_obj->fd = dup(fd);
        if (m_obj->fd == -1) {
            Py_DECREF(m_obj);
            PyErr_SetFromErrno(mmap_module_error);
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
        PyErr_SetFromErrno(mmap_module_error);
        return NULL;
    }
    m_obj->access = (access_mode)access;
    return (PyObject *)m_obj;
}
#endif /* UNIX */

#ifdef MS_WINDOWS

/* A note on sizes and offsets: while the actual map size must hold in a
   Py_ssize_t, both the total file size and the start offset can be longer
   than a Py_ssize_t, so we use PY_LONG_LONG which is always 64-bit.
*/

static PyObject *
new_mmap_object(PyTypeObject *type, PyObject *args, PyObject *kwdict)
{
    mmap_object *m_obj;
    Py_ssize_t map_size;
    PY_LONG_LONG offset = 0, size;
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
                        "memory mapped length must be positive");
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
        PyErr_Warn(PyExc_DeprecationWarning,
                   "don't use 0 for anonymous memory");
     */
    if (fileno != -1 && fileno != 0) {
        /* Ensure that fileno is within the CRT's valid range */
        if (_PyVerify_fd(fileno) == 0) {
            PyErr_SetFromErrno(mmap_module_error);
            return NULL;
        }
        fh = (HANDLE)_get_osfhandle(fileno);
        if (fh==(HANDLE)-1) {
            PyErr_SetFromErrno(mmap_module_error);
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

            size = (((PY_LONG_LONG) high) << 32) + low;
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
    PyObject *o = PyInt_FromLong(value);
    if (o && PyDict_SetItemString(d, name, o) == 0) {
        Py_DECREF(o);
    }
}

PyMODINIT_FUNC
initmmap(void)
{
    PyObject *dict, *module;

    if (PyType_Ready(&mmap_object_type) < 0)
        return;

    module = Py_InitModule("mmap", NULL);
    if (module == NULL)
        return;
    dict = PyModule_GetDict(module);
    if (!dict)
        return;
    mmap_module_error = PyErr_NewException("mmap.error",
        PyExc_EnvironmentError , NULL);
    if (mmap_module_error == NULL)
        return;
    PyDict_SetItemString(dict, "error", mmap_module_error);
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
}
