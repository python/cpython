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

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include <Python.h>
#include "pycore_abstract.h"      // _Py_convert_optional_to_ssize_t()
#include "pycore_bytesobject.h"   // _PyBytes_Find()
#include "pycore_fileutils.h"     // _Py_stat_struct
#include "pycore_weakref.h"       // FT_CLEAR_WEAKREFS()

#include <stddef.h>               // offsetof()
#ifndef MS_WINDOWS
#  include <unistd.h>             // close()
#endif

#ifndef MS_WINDOWS
#define UNIX
# ifdef HAVE_FCNTL_H
#  include <fcntl.h>
# endif /* HAVE_FCNTL_H */
#endif

#ifdef MS_WINDOWS
#include <windows.h>
#include <ntsecapi.h> // LsaNtStatusToWinError
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

/*[clinic input]
module mmap
class mmap.mmap "mmap_object *" ""
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=82a9f8a529905b9b]*/

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
    Py_ssize_t  exports;

#ifdef MS_WINDOWS
    HANDLE      map_handle;
    HANDLE      file_handle;
    wchar_t *   tagname;
#endif

#ifdef UNIX
    int fd;
    int flags;
#endif

    PyObject *weakreflist;
    access_mode access;
    _Bool trackfd;
} mmap_object;

#define mmap_object_CAST(op)    ((mmap_object *)(op))

#include "clinic/mmapmodule.c.h"


/* Return a Py_ssize_t from the object arg. This conversion logic is similar
   to what AC uses for `Py_ssize_t` arguments.

   Returns -1 on error. Use PyErr_Occurred() to disambiguate.
*/
static Py_ssize_t
_As_Py_ssize_t(PyObject *arg) {
    assert(arg != NULL);
    Py_ssize_t ival = -1;
    PyObject *iobj = _PyNumber_Index(arg);
    if (iobj != NULL) {
        ival = PyLong_AsSsize_t(iobj);
        Py_DECREF(iobj);
    }
    return ival;
}

static void
mmap_object_dealloc(PyObject *op)
{
    mmap_object *m_obj = mmap_object_CAST(op);
    PyTypeObject *tp = Py_TYPE(m_obj);
    PyObject_GC_UnTrack(m_obj);

#ifdef MS_WINDOWS
    Py_BEGIN_ALLOW_THREADS
    if (m_obj->data != NULL)
        UnmapViewOfFile (m_obj->data);
    if (m_obj->map_handle != NULL)
        CloseHandle (m_obj->map_handle);
    if (m_obj->file_handle != INVALID_HANDLE_VALUE)
        CloseHandle (m_obj->file_handle);
    Py_END_ALLOW_THREADS
    if (m_obj->tagname)
        PyMem_Free(m_obj->tagname);
#endif /* MS_WINDOWS */

#ifdef UNIX
    Py_BEGIN_ALLOW_THREADS
    if (m_obj->fd >= 0)
        (void) close(m_obj->fd);
    if (m_obj->data!=NULL) {
        munmap(m_obj->data, m_obj->size);
    }
    Py_END_ALLOW_THREADS
#endif /* UNIX */

    FT_CLEAR_WEAKREFS(op, m_obj->weakreflist);

    tp->tp_free(m_obj);
    Py_DECREF(tp);
}

/*[clinic input]
@critical_section
mmap.mmap.close

[clinic start generated code]*/

static PyObject *
mmap_mmap_close_impl(mmap_object *self)
/*[clinic end generated code: output=a1ae0c727546f78d input=25020035f047eae1]*/
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
    HANDLE map_handle = self->map_handle;
    HANDLE file_handle = self->file_handle;
    char *data = self->data;
    self->map_handle = NULL;
    self->file_handle = INVALID_HANDLE_VALUE;
    self->data = NULL;
    Py_BEGIN_ALLOW_THREADS
    if (data != NULL) {
        UnmapViewOfFile(data);
    }
    if (map_handle != NULL) {
        CloseHandle(map_handle);
    }
    if (file_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(file_handle);
    }
    Py_END_ALLOW_THREADS
#endif /* MS_WINDOWS */

#ifdef UNIX
    int fd = self->fd;
    char *data = self->data;
    self->fd = -1;
    self->data = NULL;
    Py_BEGIN_ALLOW_THREADS
    if (0 <= fd)
        (void) close(fd);
    if (data != NULL) {
        munmap(data, self->size);
    }
    Py_END_ALLOW_THREADS
#endif

    Py_RETURN_NONE;
}

#ifdef MS_WINDOWS
#define CHECK_VALID(err)                                                \
do {                                                                    \
    if (self->map_handle == NULL) {                                     \
    PyErr_SetString(PyExc_ValueError, "mmap closed or invalid");        \
    return err;                                                         \
    }                                                                   \
} while (0)
#define CHECK_VALID_OR_RELEASE(err, buffer)                             \
do {                                                                    \
    if (self->map_handle == NULL) {                                     \
    PyErr_SetString(PyExc_ValueError, "mmap closed or invalid");        \
    PyBuffer_Release(&(buffer));                                        \
    return (err);                                                       \
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
#define CHECK_VALID_OR_RELEASE(err, buffer)                             \
do {                                                                    \
    if (self->data == NULL) {                                           \
    PyErr_SetString(PyExc_ValueError, "mmap closed or invalid");        \
    PyBuffer_Release(&(buffer));                                        \
    return (err);                                                       \
    }                                                                   \
} while (0)
#endif /* UNIX */

#if defined(MS_WINDOWS) && !defined(DONT_USE_SEH)
static DWORD
filter_page_exception(EXCEPTION_POINTERS *ptrs, EXCEPTION_RECORD *record)
{
    *record = *ptrs->ExceptionRecord;
    if (record->ExceptionCode == EXCEPTION_IN_PAGE_ERROR ||
        record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        return EXCEPTION_EXECUTE_HANDLER;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static DWORD
filter_page_exception_method(mmap_object *self, EXCEPTION_POINTERS *ptrs,
                             EXCEPTION_RECORD *record)
{
    *record = *ptrs->ExceptionRecord;
    if (record->ExceptionCode == EXCEPTION_IN_PAGE_ERROR ||
        record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {

        ULONG_PTR address = record->ExceptionInformation[1];
        if (address >= (ULONG_PTR) self->data &&
            address < (ULONG_PTR) self->data + (ULONG_PTR) self->size)
        {
            return EXCEPTION_EXECUTE_HANDLER;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static void
_PyErr_SetFromNTSTATUS(ULONG status)
{
#if defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_SYSTEM)
    PyErr_SetFromWindowsErr(LsaNtStatusToWinError((NTSTATUS)status));
#else
    if (status & 0x80000000) {
        // HRESULT-shaped codes are supported by PyErr_SetFromWindowsErr
        PyErr_SetFromWindowsErr((int)status);
    }
    else {
        // No mapping for NTSTATUS values, so just return it for diagnostic purposes
        // If we provide it as winerror it could incorrectly change the type of the exception.
        PyErr_Format(PyExc_OSError, "Operating system error NTSTATUS=0x%08lX", status);
    }
#endif
}
#endif

#if defined(MS_WINDOWS) && !defined(DONT_USE_SEH)
#define HANDLE_INVALID_MEM(sourcecode)                                     \
do {                                                                       \
    EXCEPTION_RECORD record;                                               \
    __try {                                                                \
        sourcecode                                                         \
    }                                                                      \
    __except (filter_page_exception(GetExceptionInformation(), &record)) { \
        assert(record.ExceptionCode == EXCEPTION_IN_PAGE_ERROR ||          \
               record.ExceptionCode == EXCEPTION_ACCESS_VIOLATION);        \
        if (record.ExceptionCode == EXCEPTION_IN_PAGE_ERROR) {             \
            _PyErr_SetFromNTSTATUS((ULONG)record.ExceptionInformation[2]); \
        }                                                                  \
        else if (record.ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {     \
            PyErr_SetFromWindowsErr(ERROR_NOACCESS);                       \
        }                                                                  \
        return -1;                                                         \
    }                                                                      \
} while (0)
#else
#define HANDLE_INVALID_MEM(sourcecode)                                     \
do {                                                                       \
    sourcecode                                                             \
} while (0)
#endif

#if defined(MS_WINDOWS) && !defined(DONT_USE_SEH)
#define HANDLE_INVALID_MEM_METHOD(self, sourcecode)                           \
do {                                                                          \
    EXCEPTION_RECORD record;                                                  \
    __try {                                                                   \
        sourcecode                                                            \
    }                                                                         \
    __except (filter_page_exception_method(self, GetExceptionInformation(),   \
                                           &record)) {                        \
        assert(record.ExceptionCode == EXCEPTION_IN_PAGE_ERROR ||             \
               record.ExceptionCode == EXCEPTION_ACCESS_VIOLATION);           \
        if (record.ExceptionCode == EXCEPTION_IN_PAGE_ERROR) {                \
            _PyErr_SetFromNTSTATUS((ULONG)record.ExceptionInformation[2]);    \
        }                                                                     \
        else if (record.ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {        \
            PyErr_SetFromWindowsErr(ERROR_NOACCESS);                          \
        }                                                                     \
        return -1;                                                            \
    }                                                                         \
} while (0)
#else
#define HANDLE_INVALID_MEM_METHOD(self, sourcecode)                           \
do {                                                                          \
    sourcecode                                                                \
} while (0)
#endif

int
safe_memcpy(void *dest, const void *src, size_t count)
{
    HANDLE_INVALID_MEM(
        memcpy(dest, src, count);
    );
    return 0;
}

int
safe_byte_copy(char *dest, const char *src)
{
    HANDLE_INVALID_MEM(
        *dest = *src;
    );
    return 0;
}

int
safe_memchr(char **out, const void *ptr, int ch, size_t count)
{
    HANDLE_INVALID_MEM(
        *out = (char *) memchr(ptr, ch, count);
    );
    return 0;
}

int
safe_memmove(void *dest, const void *src, size_t count)
{
    HANDLE_INVALID_MEM(
        memmove(dest, src, count);
    );
    return 0;
}

int
safe_copy_from_slice(char *dest, const char *src, Py_ssize_t start,
                     Py_ssize_t step, Py_ssize_t slicelen)
{
    HANDLE_INVALID_MEM(
        size_t cur;
        Py_ssize_t i;
        for (cur = start, i = 0; i < slicelen; cur += step, i++) {
            dest[cur] = src[i];
        }
    );
    return 0;
}

int
safe_copy_to_slice(char *dest, const char *src, Py_ssize_t start,
                   Py_ssize_t step, Py_ssize_t slicelen)
{
    HANDLE_INVALID_MEM(
        size_t cur;
        Py_ssize_t i;
        for (cur = start, i = 0; i < slicelen; cur += step, i++) {
            dest[i] = src[cur];
        }
    );
    return 0;
}


int
_safe_PyBytes_Find(Py_ssize_t *out, mmap_object *self, const char *haystack,
                   Py_ssize_t len_haystack, const char *needle,
                   Py_ssize_t len_needle, Py_ssize_t offset)
{
    HANDLE_INVALID_MEM_METHOD(self,
        *out = _PyBytes_Find(haystack, len_haystack, needle, len_needle, offset);
    );
    return 0;
}

int
_safe_PyBytes_ReverseFind(Py_ssize_t *out, mmap_object *self,
                          const char *haystack, Py_ssize_t len_haystack,
                          const char *needle, Py_ssize_t len_needle,
                          Py_ssize_t offset)
{
    HANDLE_INVALID_MEM_METHOD(self,
        *out = _PyBytes_ReverseFind(haystack, len_haystack, needle, len_needle,
                                    offset);
    );
    return 0;
}

PyObject *
_safe_PyBytes_FromStringAndSize(char *start, size_t num_bytes)
{
    if (num_bytes == 1) {
        char dest;
        if (safe_byte_copy(&dest, start) < 0) {
            return NULL;
        }
        else {
            return PyBytes_FromStringAndSize(&dest, 1);
        }
    }
    else {
        PyBytesWriter *writer = PyBytesWriter_Create(num_bytes);
        if (writer == NULL) {
            return NULL;
        }
        if (safe_memcpy(PyBytesWriter_GetData(writer), start, num_bytes) < 0) {
            PyBytesWriter_Discard(writer);
            return NULL;
        }
        return PyBytesWriter_Finish(writer);
    }
}

/*[clinic input]
@critical_section
mmap.mmap.read_byte

[clinic start generated code]*/

static PyObject *
mmap_mmap_read_byte_impl(mmap_object *self)
/*[clinic end generated code: output=d931da1319f3869b input=5b8c6a904bdddda9]*/
{
    CHECK_VALID(NULL);
    if (self->pos >= self->size) {
        PyErr_SetString(PyExc_ValueError, "read byte out of range");
        return NULL;
    }
    char dest;
    if (safe_byte_copy(&dest, self->data + self->pos) < 0) {
        return NULL;
    }
    self->pos++;
    return PyLong_FromLong((unsigned char) dest);
}

/*[clinic input]
@critical_section
mmap.mmap.readline

[clinic start generated code]*/

static PyObject *
mmap_mmap_readline_impl(mmap_object *self)
/*[clinic end generated code: output=b9d2bf9999283311 input=2c4efd1d06e1cdd1]*/
{
    Py_ssize_t remaining;
    char *start, *eol;

    CHECK_VALID(NULL);

    remaining = (self->pos < self->size) ? self->size - self->pos : 0;
    if (!remaining)
        return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);
    start = self->data + self->pos;

    if (safe_memchr(&eol, start, '\n', remaining) < 0) {
        return NULL;
    }

    if (!eol)
        eol = self->data + self->size;
    else
        ++eol; /* advance past newline */

    PyObject *result = _safe_PyBytes_FromStringAndSize(start, eol - start);
    if (result != NULL) {
        self->pos += (eol - start);
    }
    return result;
}

/*[clinic input]
@critical_section
mmap.mmap.read

  n as num_bytes: object(converter='_Py_convert_optional_to_ssize_t', type='Py_ssize_t', c_default='PY_SSIZE_T_MAX') = None
  /

[clinic start generated code]*/

static PyObject *
mmap_mmap_read_impl(mmap_object *self, Py_ssize_t num_bytes)
/*[clinic end generated code: output=3b4d4f3704ed0969 input=8f97f361d435e357]*/
{
    Py_ssize_t remaining;

    CHECK_VALID(NULL);

    /* silently 'adjust' out-of-range requests */
    remaining = (self->pos < self->size) ? self->size - self->pos : 0;
    if (num_bytes < 0 || num_bytes > remaining)
        num_bytes = remaining;

    PyObject *result = _safe_PyBytes_FromStringAndSize(self->data + self->pos,
                                                       num_bytes);
    if (result != NULL) {
        self->pos += num_bytes;
    }
    return result;
}

static PyObject *
mmap_gfind_lock_held(mmap_object *self, Py_buffer *view, PyObject *start_obj,
                    PyObject *end_obj, int reverse)
{
    Py_ssize_t start = self->pos;
    Py_ssize_t end = self->size;

    CHECK_VALID(NULL);
    if (start_obj != Py_None) {
        start = _As_Py_ssize_t(start_obj);
        if (start == -1 && PyErr_Occurred()) {
            return NULL;
        }

        if (end_obj != Py_None) {
            end = _As_Py_ssize_t(end_obj);
            if (end == -1 && PyErr_Occurred()) {
                return NULL;
            }
        }
    }

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

    Py_ssize_t index;
    PyObject *result;
    CHECK_VALID(NULL);
    if (end < start) {
        result = PyLong_FromSsize_t(-1);
    }
    else if (reverse) {
        assert(0 <= start && start <= end && end <= self->size);
        if (_safe_PyBytes_ReverseFind(&index, self,
            self->data + start, end - start,
            view->buf, view->len, start) < 0)
        {
            result = NULL;
        }
        else {
            result = PyLong_FromSsize_t(index);
        }
    }
    else {
        assert(0 <= start && start <= end && end <= self->size);
        if (_safe_PyBytes_Find(&index, self,
            self->data + start, end - start,
            view->buf, view->len, start) < 0)
        {
            result = NULL;
        }
        else {
            result = PyLong_FromSsize_t(index);
        }
    }
    return result;
}

/*[clinic input]
@critical_section
mmap.mmap.find

  view: Py_buffer
  start: object = None
  end: object = None
  /

[clinic start generated code]*/

static PyObject *
mmap_mmap_find_impl(mmap_object *self, Py_buffer *view, PyObject *start,
                    PyObject *end)
/*[clinic end generated code: output=ef8878a322f00192 input=0135504494b52c2b]*/
{
    return mmap_gfind_lock_held(self, view, start, end, 0);
}

/*[clinic input]
@critical_section
mmap.mmap.rfind = mmap.mmap.find

[clinic start generated code]*/

static PyObject *
mmap_mmap_rfind_impl(mmap_object *self, Py_buffer *view, PyObject *start,
                     PyObject *end)
/*[clinic end generated code: output=73b918940d67c2b8 input=8aecdd1f70c06c62]*/
{
    return mmap_gfind_lock_held(self, view, start, end, 1);
}

static int
is_writable(mmap_object *self)
{
    if (self->access != ACCESS_READ)
        return 1;
    PyErr_Format(PyExc_TypeError, "mmap can't modify a readonly memory map.");
    return 0;
}

#if defined(MS_WINDOWS) || defined(HAVE_MREMAP)
static int
is_resizeable(mmap_object *self)
{
    if (self->exports > 0) {
        PyErr_SetString(PyExc_BufferError,
            "mmap can't resize with extant buffers exported.");
        return 0;
    }
    if (!self->trackfd) {
        PyErr_SetString(PyExc_ValueError,
            "mmap can't resize with trackfd=False.");
        return 0;
    }
    if ((self->access == ACCESS_WRITE) || (self->access == ACCESS_DEFAULT))
        return 1;
    PyErr_Format(PyExc_TypeError,
        "mmap can't resize a readonly or copy-on-write memory map.");
    return 0;

}
#endif /* MS_WINDOWS || HAVE_MREMAP */


/*[clinic input]
@critical_section
mmap.mmap.write

    bytes as data: Py_buffer
    /

[clinic start generated code]*/

static PyObject *
mmap_mmap_write_impl(mmap_object *self, Py_buffer *data)
/*[clinic end generated code: output=9e97063efb6fb27b input=3f16fa79aa89d6f7]*/
{
    CHECK_VALID(NULL);
    if (!is_writable(self)) {
        return NULL;
    }

    if (self->pos > self->size || self->size - self->pos < data->len) {
        PyErr_SetString(PyExc_ValueError, "data out of range");
        return NULL;
    }

    CHECK_VALID(NULL);
    PyObject *result;
    if (safe_memcpy(self->data + self->pos, data->buf, data->len) < 0) {
        result = NULL;
    }
    else {
        self->pos += data->len;
        result = PyLong_FromSsize_t(data->len);
    }
    return result;
}

/*[clinic input]
@critical_section
mmap.mmap.write_byte

    byte as value: unsigned_char
    /

[clinic start generated code]*/

static PyObject *
mmap_mmap_write_byte_impl(mmap_object *self, unsigned char value)
/*[clinic end generated code: output=aa11adada9b17510 input=32740bfa174f0991]*/
{
    CHECK_VALID(NULL);
    if (!is_writable(self))
        return NULL;

    CHECK_VALID(NULL);
    if (self->pos >= self->size) {
        PyErr_SetString(PyExc_ValueError, "write byte out of range");
        return NULL;
    }

    if (safe_byte_copy(self->data + self->pos, (const char*)&value) < 0) {
        return NULL;
    }
    self->pos++;
    Py_RETURN_NONE;
}

/*[clinic input]
@critical_section
mmap.mmap.size

[clinic start generated code]*/

static PyObject *
mmap_mmap_size_impl(mmap_object *self)
/*[clinic end generated code: output=c177e65e83a648ff input=f69c072efd2e1595]*/
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
    }
#endif /* MS_WINDOWS */

#ifdef UNIX
    if (self->fd != -1) {
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
    else if (self->trackfd) {
        return PyLong_FromSsize_t(self->size);
    }
    else {
        PyErr_SetString(PyExc_ValueError,
            "can't get size with trackfd=False");
        return NULL;
    }
}

/* This assumes that you want the entire file mapped,
 / and when recreating the map will make the new file
 / have the new size
 /
 / Is this really necessary?  This could easily be done
 / from python by just closing and re-opening with the
 / new size?
 */

#if defined(MS_WINDOWS) || defined(HAVE_MREMAP)
/*[clinic input]
@critical_section
mmap.mmap.resize

    newsize as new_size: Py_ssize_t
    /

[clinic start generated code]*/

static PyObject *
mmap_mmap_resize_impl(mmap_object *self, Py_ssize_t new_size)
/*[clinic end generated code: output=6f262537ce9c2dcc input=b6b5dee52a41b79f]*/
{
    CHECK_VALID(NULL);
    if (!is_resizeable(self)) {
        return NULL;
    }
    if (new_size < 0 || PY_SSIZE_T_MAX - new_size < self->offset) {
        PyErr_SetString(PyExc_ValueError, "new size out of range");
        return NULL;
    }

    {
#ifdef MS_WINDOWS
        DWORD error = 0, file_resize_error = 0;
        char* old_data = self->data;
        LARGE_INTEGER offset, max_size;
        offset.QuadPart = self->offset;
        max_size.QuadPart = self->offset + new_size;
        /* close the file mapping */
        CloseHandle(self->map_handle);
        /* if the file mapping still exists, it cannot be resized. */
        if (self->tagname) {
            self->map_handle = OpenFileMappingW(FILE_MAP_WRITE, FALSE,
                                    self->tagname);
            if (self->map_handle) {
                PyErr_SetFromWindowsErr(ERROR_USER_MAPPED_FILE);
                return NULL;
            }
        } else {
            self->map_handle = NULL;
        }

        /* if it's not the paging file, unmap the view and resize the file */
        if (self->file_handle != INVALID_HANDLE_VALUE) {
            if (!UnmapViewOfFile(self->data)) {
                return PyErr_SetFromWindowsErr(GetLastError());
            };
            self->data = NULL;
            /* resize the file */
            if (!SetFilePointerEx(self->file_handle, max_size, NULL,
                FILE_BEGIN) ||
                !SetEndOfFile(self->file_handle)) {
                /* resizing failed. try to remap the file */
                file_resize_error = GetLastError();
                max_size.QuadPart = self->size;
                new_size = self->size;
            }
        }

        /* create a new file mapping and map a new view */
        /* FIXME: call CreateFileMappingW with wchar_t tagname */
        self->map_handle = CreateFileMappingW(
            self->file_handle,
            NULL,
            PAGE_READWRITE,
            max_size.HighPart,
            max_size.LowPart,
            self->tagname);

        error = GetLastError();
        /* ERROR_ALREADY_EXISTS implies that between our closing the handle above and
        calling CreateFileMapping here, someone's created a different mapping with
        the same name. There's nothing we can usefully do so we invalidate our
        mapping and error out.
        */
        if (error == ERROR_ALREADY_EXISTS) {
            CloseHandle(self->map_handle);
            self->map_handle = NULL;
        }
        else if (self->map_handle != NULL) {
            self->data = MapViewOfFile(self->map_handle,
                FILE_MAP_WRITE,
                offset.HighPart,
                offset.LowPart,
                new_size);
            if (self->data != NULL) {
                /* copy the old view if using the paging file */
                if (self->file_handle == INVALID_HANDLE_VALUE) {
                    memcpy(self->data, old_data,
                           self->size < new_size ? self->size : new_size);
                    if (!UnmapViewOfFile(old_data)) {
                        error = GetLastError();
                    }
                }
                self->size = new_size;
            }
            else {
                error = GetLastError();
                CloseHandle(self->map_handle);
                self->map_handle = NULL;
            }
        }

        if (error) {
            return PyErr_SetFromWindowsErr(error);
            return NULL;
        }
        /* It's possible for a resize to fail, typically because another mapping
        is still held against the same underlying file. Even if nothing has
        failed -- ie we're still returning a valid file mapping -- raise the
        error as an exception as the resize won't have happened
        */
        if (file_resize_error) {
            PyErr_SetFromWindowsErr(file_resize_error);
            return NULL;
        }
        Py_RETURN_NONE;
#endif /* MS_WINDOWS */

#ifdef UNIX
        void *newmap;

#ifdef __linux__
        if (self->fd == -1 && !(self->flags & MAP_PRIVATE) && new_size > self->size) {
            PyErr_Format(PyExc_ValueError,
                "mmap: can't expand a shared anonymous mapping on Linux");
            return NULL;
        }
#endif
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
        Py_RETURN_NONE;
#endif /* UNIX */
    }
}
#endif /* MS_WINDOWS || HAVE_MREMAP */

/*[clinic input]
@critical_section
mmap.mmap.tell

[clinic start generated code]*/

static PyObject *
mmap_mmap_tell_impl(mmap_object *self)
/*[clinic end generated code: output=6034958630e1b1d1 input=fd163acacf45c3a5]*/
{
    CHECK_VALID(NULL);
    return PyLong_FromSize_t(self->pos);
}

/*[clinic input]
@critical_section
mmap.mmap.flush

    offset: Py_ssize_t = 0
    size: Py_ssize_t = -1
    /

[clinic start generated code]*/

static PyObject *
mmap_mmap_flush_impl(mmap_object *self, Py_ssize_t offset, Py_ssize_t size)
/*[clinic end generated code: output=956ced67466149cf input=c50b893bc69520ec]*/
{
    CHECK_VALID(NULL);
    if (size == -1) {
        size = self->size - offset;
    }
    if (size < 0 || offset < 0 || self->size - offset < size) {
        PyErr_SetString(PyExc_ValueError, "flush values out of range");
        return NULL;
    }

    if (self->access == ACCESS_READ || self->access == ACCESS_COPY)
        Py_RETURN_NONE;

#if defined(MS_WINDOWS_DESKTOP) || defined(MS_WINDOWS_APP) || defined(MS_WINDOWS_SYSTEM)
    if (!FlushViewOfFile(self->data+offset, size)) {
        PyErr_SetFromWindowsErr(GetLastError());
        return NULL;
    }
    Py_RETURN_NONE;
#elif defined(UNIX)
    /* XXX flags for msync? */
    if (-1 == msync(self->data + offset, size, MS_SYNC)) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    Py_RETURN_NONE;
#else
    PyErr_SetString(PyExc_ValueError, "flush not supported on this system");
    return NULL;
#endif
}

/*[clinic input]
@critical_section
mmap.mmap.seek

    pos as dist: Py_ssize_t
    whence as how: int = 0
    /

[clinic start generated code]*/

static PyObject *
mmap_mmap_seek_impl(mmap_object *self, Py_ssize_t dist, int how)
/*[clinic end generated code: output=00310494e8b8c592 input=e2fda5d081c3db22]*/
{
    CHECK_VALID(NULL);
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
    return PyLong_FromSsize_t(self->pos);

  onoutofrange:
    PyErr_SetString(PyExc_ValueError, "seek out of range");
    return NULL;
}

/*[clinic input]
mmap.mmap.seekable

[clinic start generated code]*/

static PyObject *
mmap_mmap_seekable_impl(mmap_object *self)
/*[clinic end generated code: output=6311dc3ea300fa38 input=5132505f6e259001]*/
{
    Py_RETURN_TRUE;
}

/*[clinic input]
@critical_section
mmap.mmap.move

    dest: Py_ssize_t
    src: Py_ssize_t
    count as cnt: Py_ssize_t
    /

[clinic start generated code]*/

static PyObject *
mmap_mmap_move_impl(mmap_object *self, Py_ssize_t dest, Py_ssize_t src,
                    Py_ssize_t cnt)
/*[clinic end generated code: output=391f549a44181793 input=cf8cfe10d9f6b448]*/
{
    CHECK_VALID(NULL);
    if (!is_writable(self)) {
        return NULL;
    } else {
        /* bounds check the values */
        if (dest < 0 || src < 0 || cnt < 0)
            goto bounds;
        if (self->size - dest < cnt || self->size - src < cnt)
            goto bounds;

        CHECK_VALID(NULL);
        if (safe_memmove(self->data + dest, self->data + src, cnt) < 0) {
            return NULL;
        };
        Py_RETURN_NONE;

      bounds:
        PyErr_SetString(PyExc_ValueError,
                        "source, destination, or count out of range");
        return NULL;
    }
}

static PyObject *
mmap_closed_get(PyObject *op, void *Py_UNUSED(closure))
{
    mmap_object *self = mmap_object_CAST(op);
    PyObject *result;
    Py_BEGIN_CRITICAL_SECTION(op);
#ifdef MS_WINDOWS
    result = PyBool_FromLong(self->map_handle == NULL ? 1 : 0);
#elif defined(UNIX)
    result = PyBool_FromLong(self->data == NULL ? 1 : 0);
#endif
    Py_END_CRITICAL_SECTION();
    return result;
}

/*[clinic input]
@critical_section
mmap.mmap.__enter__

[clinic start generated code]*/

static PyObject *
mmap_mmap___enter___impl(mmap_object *self)
/*[clinic end generated code: output=92cfc59f4c4e2d26 input=a446541fbfe0b890]*/
{
    CHECK_VALID(NULL);

    return Py_NewRef(self);
}

/*[clinic input]
@critical_section
mmap.mmap.__exit__

    exc_type: object
    exc_value: object
    traceback: object
    /

[clinic start generated code]*/

static PyObject *
mmap_mmap___exit___impl(mmap_object *self, PyObject *exc_type,
                        PyObject *exc_value, PyObject *traceback)
/*[clinic end generated code: output=bec7e3e319c1f07e input=5f28e91cf752bc64]*/
{
    return mmap_mmap_close_impl(self);
}

static PyObject *
mmap__repr__method_lock_held(PyObject *op)
{
    mmap_object *mobj = mmap_object_CAST(op);

#ifdef MS_WINDOWS
#define _Py_FORMAT_OFFSET "lld"
    if (mobj->map_handle == NULL)
#elif defined(UNIX)
# ifdef HAVE_LARGEFILE_SUPPORT
# define _Py_FORMAT_OFFSET "lld"
# else
# define _Py_FORMAT_OFFSET "ld"
# endif
    if (mobj->data == NULL)
#endif
    {
        return PyUnicode_FromFormat("<%s closed=True>", Py_TYPE(op)->tp_name);
    } else {
        const char *access_str;

        switch (mobj->access) {
            case ACCESS_DEFAULT:
                access_str = "ACCESS_DEFAULT";
                break;
            case ACCESS_READ:
                access_str = "ACCESS_READ";
                break;
            case ACCESS_WRITE:
                access_str = "ACCESS_WRITE";
                break;
            case ACCESS_COPY:
                access_str = "ACCESS_COPY";
                break;
            default:
                Py_UNREACHABLE();
        }

        return PyUnicode_FromFormat("<%s closed=False, access=%s, length=%zd, "
                                    "pos=%zd, offset=%" _Py_FORMAT_OFFSET ">",
                                    Py_TYPE(op)->tp_name, access_str,
                                    mobj->size, mobj->pos, mobj->offset);
    }
}

static PyObject *
mmap__repr__method(PyObject *op)
{
    PyObject *result;
    Py_BEGIN_CRITICAL_SECTION(op);
    result = mmap__repr__method_lock_held(op);
    Py_END_CRITICAL_SECTION();
    return result;
}

#ifdef MS_WINDOWS
/*[clinic input]
@critical_section
mmap.mmap.__sizeof__

[clinic start generated code]*/

static PyObject *
mmap_mmap___sizeof___impl(mmap_object *self)
/*[clinic end generated code: output=1aed30daff807d09 input=8a648868a089553c]*/
{
    size_t res = _PyObject_SIZE(Py_TYPE(self));
    if (self->tagname) {
        res += (wcslen(self->tagname) + 1) * sizeof(self->tagname[0]);
    }
    return PyLong_FromSize_t(res);
}
#endif

#if defined(MS_WINDOWS) && defined(Py_DEBUG)
/*[clinic input]
@critical_section
mmap.mmap._protect

    flNewProtect: unsigned_int(bitwise=True)
    start: Py_ssize_t
    length: Py_ssize_t
    /

[clinic start generated code]*/

static PyObject *
mmap_mmap__protect_impl(mmap_object *self, unsigned int flNewProtect,
                        Py_ssize_t start, Py_ssize_t length)
/*[clinic end generated code: output=a87271a34d1ad6cf input=9170498c5e1482da]*/
{
    DWORD flOldProtect;

    CHECK_VALID(NULL);

    if (!VirtualProtect((void *) (self->data + start), length, flNewProtect,
                        &flOldProtect))
    {
        PyErr_SetFromWindowsErr(GetLastError());
        return NULL;
    }

    Py_RETURN_NONE;
}
#endif

#ifdef HAVE_MADVISE
/*[clinic input]
@critical_section
mmap.mmap.madvise

  option: int
  start: Py_ssize_t = 0
  length as length_obj: object = None
  /

[clinic start generated code]*/

static PyObject *
mmap_mmap_madvise_impl(mmap_object *self, int option, Py_ssize_t start,
                       PyObject *length_obj)
/*[clinic end generated code: output=816be656f08c0e3c input=2d37f7a4c87f1053]*/
{
    Py_ssize_t length;

    CHECK_VALID(NULL);
    if (length_obj == Py_None) {
        length = self->size;
    } else {
        length = _As_Py_ssize_t(length_obj);
        if (length == -1 && PyErr_Occurred()) {
            return NULL;
        }
    }

    if (start < 0 || start >= self->size) {
        PyErr_SetString(PyExc_ValueError, "madvise start out of bounds");
        return NULL;
    }
    if (length < 0) {
        PyErr_SetString(PyExc_ValueError, "madvise length invalid");
        return NULL;
    }
    if (PY_SSIZE_T_MAX - start < length) {
        PyErr_SetString(PyExc_OverflowError, "madvise length too large");
        return NULL;
    }

    if (start + length > self->size) {
        length = self->size - start;
    }

    CHECK_VALID(NULL);
    if (madvise(self->data + start, length, option) != 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    Py_RETURN_NONE;
}
#endif // HAVE_MADVISE

static struct PyMemberDef mmap_object_members[] = {
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(mmap_object, weakreflist), Py_READONLY},
    {NULL},
};

static struct PyMethodDef mmap_object_methods[] = {
    MMAP_MMAP_CLOSE_METHODDEF
    MMAP_MMAP_FIND_METHODDEF
    MMAP_MMAP_RFIND_METHODDEF
    MMAP_MMAP_FLUSH_METHODDEF
    MMAP_MMAP_MADVISE_METHODDEF
    MMAP_MMAP_MOVE_METHODDEF
    MMAP_MMAP_READ_METHODDEF
    MMAP_MMAP_READ_BYTE_METHODDEF
    MMAP_MMAP_READLINE_METHODDEF
    MMAP_MMAP_RESIZE_METHODDEF
    MMAP_MMAP_SEEK_METHODDEF
    MMAP_MMAP_SEEKABLE_METHODDEF
    MMAP_MMAP_SIZE_METHODDEF
    MMAP_MMAP_TELL_METHODDEF
    MMAP_MMAP_WRITE_METHODDEF
    MMAP_MMAP_WRITE_BYTE_METHODDEF
    MMAP_MMAP___ENTER___METHODDEF
    MMAP_MMAP___EXIT___METHODDEF
    MMAP_MMAP___SIZEOF___METHODDEF
    MMAP_MMAP__PROTECT_METHODDEF
    {NULL,         NULL}       /* sentinel */
};

static PyGetSetDef mmap_object_getset[] = {
    {"closed", mmap_closed_get, NULL, NULL},
    {NULL}
};


/* Functions for treating an mmap'ed file as a buffer */

static int
mmap_buffer_getbuf_lock_held(PyObject *op, Py_buffer *view, int flags)
{
    mmap_object *self = mmap_object_CAST(op);
    CHECK_VALID(-1);
    if (PyBuffer_FillInfo(view, op, self->data, self->size,
                          (self->access == ACCESS_READ), flags) < 0)
        return -1;
    self->exports++;
    return 0;
}

static int
mmap_buffer_getbuf(PyObject *op, Py_buffer *view, int flags)
{
    int result;
    Py_BEGIN_CRITICAL_SECTION(op);
    result = mmap_buffer_getbuf_lock_held(op, view, flags);
    Py_END_CRITICAL_SECTION();
    return result;
}

static void
mmap_buffer_releasebuf(PyObject *op, Py_buffer *Py_UNUSED(view))
{
    mmap_object *self = mmap_object_CAST(op);
    Py_BEGIN_CRITICAL_SECTION(self);
    self->exports--;
    Py_END_CRITICAL_SECTION();
}

static Py_ssize_t
mmap_length_lock_held(PyObject *op)
{
    mmap_object *self = mmap_object_CAST(op);
    CHECK_VALID(-1);
    return self->size;
}

static Py_ssize_t
mmap_length(PyObject *op)
{
    Py_ssize_t result;
    Py_BEGIN_CRITICAL_SECTION(op);
    result = mmap_length_lock_held(op);
    Py_END_CRITICAL_SECTION();
    return result;
}

static PyObject *
mmap_item_lock_held(PyObject *op, Py_ssize_t i)
{
    mmap_object *self = mmap_object_CAST(op);
    CHECK_VALID(NULL);
    if (i < 0 || i >= self->size) {
        PyErr_SetString(PyExc_IndexError, "mmap index out of range");
        return NULL;
    }

    char dest;
    if (safe_byte_copy(&dest, self->data + i) < 0) {
        return NULL;
    }
    return PyBytes_FromStringAndSize(&dest, 1);
}

static PyObject *
mmap_item(PyObject *op, Py_ssize_t i) {
    PyObject *result;
    Py_BEGIN_CRITICAL_SECTION(op);
    result = mmap_item_lock_held(op, i);
    Py_END_CRITICAL_SECTION();
    return result;
}

static PyObject *
mmap_subscript_lock_held(PyObject *op, PyObject *item)
{
    mmap_object *self = mmap_object_CAST(op);
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
        CHECK_VALID(NULL);

        char dest;
        if (safe_byte_copy(&dest, self->data + i) < 0) {
            return NULL;
        }
        return PyLong_FromLong(Py_CHARMASK(dest));
    }
    else if (PySlice_Check(item)) {
        Py_ssize_t start, stop, step, slicelen;

        if (PySlice_Unpack(item, &start, &stop, &step) < 0) {
            return NULL;
        }
        slicelen = PySlice_AdjustIndices(self->size, &start, &stop, step);

        CHECK_VALID(NULL);
        if (slicelen <= 0)
            return Py_GetConstant(Py_CONSTANT_EMPTY_BYTES);
        else if (step == 1)
            return _safe_PyBytes_FromStringAndSize(self->data + start, slicelen);
        else {
            char *result_buf = (char *)PyMem_Malloc(slicelen);
            PyObject *result;

            if (result_buf == NULL)
                return PyErr_NoMemory();

            if (safe_copy_to_slice(result_buf, self->data, start, step,
                                   slicelen) < 0)
            {
                result = NULL;
            }
            else {
                result = PyBytes_FromStringAndSize(result_buf, slicelen);
            }
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
mmap_subscript(PyObject *op, PyObject *item)
{
    PyObject *result;
    Py_BEGIN_CRITICAL_SECTION(op);
    result = mmap_subscript_lock_held(op, item);
    Py_END_CRITICAL_SECTION();
    return result;
}

static int
mmap_ass_item_lock_held(PyObject *op, Py_ssize_t i, PyObject *v)
{
    const char *buf;
    mmap_object *self = mmap_object_CAST(op);

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

    if (safe_byte_copy(self->data + i, buf) < 0) {
        return -1;
    }
    return 0;
}

static int
mmap_ass_item(PyObject *op, Py_ssize_t i, PyObject *v)
{
    int result;
    Py_BEGIN_CRITICAL_SECTION(op);
    result = mmap_ass_item_lock_held(op, i, v);
    Py_END_CRITICAL_SECTION();
    return result;
}

static int
mmap_ass_subscript_lock_held(PyObject *op, PyObject *item, PyObject *value)
{
    mmap_object *self = mmap_object_CAST(op);
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
        CHECK_VALID(-1);

        char v_char = (char) v;
        if (safe_byte_copy(self->data + i, &v_char) < 0) {
            return -1;
        }
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

        CHECK_VALID_OR_RELEASE(-1, vbuf);
        int result = 0;
        if (slicelen == 0) {
        }
        else if (step == 1) {
            if (safe_memcpy(self->data + start, vbuf.buf, slicelen) < 0) {
                result = -1;
            }
        }
        else {
            if (safe_copy_from_slice(self->data, (char *)vbuf.buf, start, step,
                                     slicelen) < 0)
            {
                result = -1;
            }
        }
        PyBuffer_Release(&vbuf);
        return result;
    }
    else {
        PyErr_SetString(PyExc_TypeError,
                        "mmap indices must be integer");
        return -1;
    }
}

static int
mmap_ass_subscript(PyObject *op, PyObject *item, PyObject *value)
{
    int result;
    Py_BEGIN_CRITICAL_SECTION(op);
    result = mmap_ass_subscript_lock_held(op, item, value);
    Py_END_CRITICAL_SECTION();
    return result;
}

static PyObject *
new_mmap_object(PyTypeObject *type, PyObject *args, PyObject *kwdict);

PyDoc_STRVAR(mmap_doc,
"Windows: mmap(fileno, length[, tagname[, access[, offset[, trackfd]]]])\n\
\n\
Maps length bytes from the file specified by the file handle fileno,\n\
and returns a mmap object.  If length is larger than the current size\n\
of the file, the file is extended to contain length bytes.  If length\n\
is 0, the maximum length of the map is the current size of the file,\n\
except that if the file is empty Windows raises an exception (you cannot\n\
create an empty mapping on Windows).\n\
\n\
Unix: mmap(fileno, length[, flags[, prot[, access[, offset[, trackfd]]]]])\n\
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


static PyType_Slot mmap_object_slots[] = {
    {Py_tp_new, new_mmap_object},
    {Py_tp_dealloc, mmap_object_dealloc},
    {Py_tp_repr, mmap__repr__method},
    {Py_tp_doc, (void *)mmap_doc},
    {Py_tp_methods, mmap_object_methods},
    {Py_tp_members, mmap_object_members},
    {Py_tp_getset, mmap_object_getset},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_traverse, _PyObject_VisitType},

    /* as sequence */
    {Py_sq_length, mmap_length},
    {Py_sq_item, mmap_item},
    {Py_sq_ass_item, mmap_ass_item},

    /* as mapping */
    {Py_mp_length, mmap_length},
    {Py_mp_subscript, mmap_subscript},
    {Py_mp_ass_subscript, mmap_ass_subscript},

    /* as buffer */
    {Py_bf_getbuffer, mmap_buffer_getbuf},
    {Py_bf_releasebuffer, mmap_buffer_releasebuf},
    {0, NULL},
};

static PyType_Spec mmap_object_spec = {
    .name = "mmap.mmap",
    .basicsize = sizeof(mmap_object),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_IMMUTABLETYPE),
    .slots = mmap_object_slots,
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
    int fstat_result = -1;
    mmap_object *m_obj;
    Py_ssize_t map_size;
    off_t offset = 0;
    int fd, flags = MAP_SHARED, prot = PROT_WRITE | PROT_READ;
    int devzero = -1;
    int access = (int)ACCESS_DEFAULT, trackfd = 1;
    static char *keywords[] = {"fileno", "length",
                               "flags", "prot",
                               "access", "offset", "trackfd", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwdict,
                                     "in|iii" _Py_PARSE_OFF_T "$p", keywords,
                                     &fd, &map_size, &flags, &prot,
                                     &access, &offset, &trackfd)) {
        return NULL;
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

    if (PySys_Audit("mmap.__new__", "ini" _Py_PARSE_OFF_T,
                    fd, map_size, access, offset) < 0) {
        return NULL;
    }

#ifdef __APPLE__
    /* Issue #11277: fsync(2) is not enough on OS X - a special, OS X specific
       fcntl(2) is necessary to force DISKSYNC and get around mmap(2) bug */
    if (fd != -1)
        (void)fcntl(fd, F_FULLFSYNC);
#endif

    if (fd != -1) {
        Py_BEGIN_ALLOW_THREADS
        fstat_result = _Py_fstat_noraise(fd, &status);
        Py_END_ALLOW_THREADS
    }

    if (fd != -1 && fstat_result == 0 && S_ISREG(status.st_mode)) {
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
    m_obj->trackfd = trackfd;
    if (fd == -1) {
        m_obj->fd = -1;
        /* Assume the caller wants to map anonymous memory.
           This is the same behaviour as Windows.  mmap.mmap(-1, size)
           on both Windows and Unix map anonymous memory.
        */
#ifdef MAP_ANONYMOUS
        /* BSD way to map anonymous memory */
        flags |= MAP_ANONYMOUS;

        /* VxWorks only supports MAP_ANONYMOUS with MAP_PRIVATE flag */
#ifdef __VXWORKS__
        flags &= ~MAP_SHARED;
        flags |= MAP_PRIVATE;
#endif

#else
        /* SVR4 method to map anonymous memory is to open /dev/zero */
        fd = devzero = _Py_open("/dev/zero", O_RDWR);
        if (devzero == -1) {
            Py_DECREF(m_obj);
            return NULL;
        }
#endif
    }
    else if (trackfd) {
        m_obj->fd = _Py_dup(fd);
        if (m_obj->fd == -1) {
            Py_DECREF(m_obj);
            return NULL;
        }
    }
    else {
        m_obj->fd = -1;
    }
    m_obj->flags = flags;

    Py_BEGIN_ALLOW_THREADS
    m_obj->data = mmap(NULL, map_size, prot, flags, fd, offset);
    Py_END_ALLOW_THREADS

    int saved_errno = errno;
    if (devzero != -1) {
        close(devzero);
    }

    if (m_obj->data == (char *)-1) {
        m_obj->data = NULL;
        Py_DECREF(m_obj);
        errno = saved_errno;
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
    PyObject *tagname = Py_None;
    DWORD dwErr = 0;
    int fileno;
    HANDLE fh = INVALID_HANDLE_VALUE;
    int access = (access_mode)ACCESS_DEFAULT;
    int trackfd = 1;
    DWORD flProtect, dwDesiredAccess;
    static char *keywords[] = { "fileno", "length",
                                "tagname",
                                "access", "offset", "trackfd", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwdict, "in|OiL$p", keywords,
                                     &fileno, &map_size,
                                     &tagname, &access, &offset, &trackfd)) {
        return NULL;
    }

    if (PySys_Audit("mmap.__new__", "iniL",
                    fileno, map_size, access, offset) < 0) {
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
        PyErr_WarnEx(PyExc_DeprecationWarning,
                     "don't use 0 for anonymous memory",
                     1);
     */
    if (fileno != -1 && fileno != 0) {
        /* Ensure that fileno is within the CRT's valid range */
        fh = _Py_get_osfhandle(fileno);
        if (fh == INVALID_HANDLE_VALUE)
            return NULL;

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
    m_obj->trackfd = trackfd;

    if (fh != INVALID_HANDLE_VALUE) {
        if (trackfd) {
            /* It is necessary to duplicate the handle, so the
               Python code can close it on us */
            if (!DuplicateHandle(
                GetCurrentProcess(), /* source process handle */
                fh, /* handle to be duplicated */
                GetCurrentProcess(), /* target proc handle */
                &fh, /* result */
                0, /* access - ignored due to options value */
                FALSE, /* inherited by child processes? */
                DUPLICATE_SAME_ACCESS)) /* options */
            {
                dwErr = GetLastError();
                Py_DECREF(m_obj);
                PyErr_SetFromWindowsErr(dwErr);
                return NULL;
            }
            m_obj->file_handle = fh;
        }
        if (!map_size) {
            DWORD low,high;
            low = GetFileSize(fh, &high);
            /* low might just happen to have the value INVALID_FILE_SIZE;
               so we need to check the last error also. */
            if (low == INVALID_FILE_SIZE &&
                (dwErr = GetLastError()) != NO_ERROR)
            {
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
    if (!Py_IsNone(tagname)) {
        if (!PyUnicode_Check(tagname)) {
            Py_DECREF(m_obj);
            return PyErr_Format(PyExc_TypeError, "expected str or None for "
                                "'tagname', not %.200s",
                                Py_TYPE(tagname)->tp_name);
        }
        m_obj->tagname = PyUnicode_AsWideCharString(tagname, NULL);
        if (m_obj->tagname == NULL) {
            Py_DECREF(m_obj);
            return NULL;
        }
    }

    m_obj->access = (access_mode)access;
    size_hi = (DWORD)(size >> 32);
    size_lo = (DWORD)(size & 0xFFFFFFFF);
    off_hi = (DWORD)(offset >> 32);
    off_lo = (DWORD)(offset & 0xFFFFFFFF);
    /* For files, it would be sufficient to pass 0 as size.
       For anonymous maps, we have to pass the size explicitly. */
    m_obj->map_handle = CreateFileMappingW(fh,
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

static int
mmap_exec(PyObject *module)
{
    if (PyModule_AddObjectRef(module, "error", PyExc_OSError) < 0) {
        return -1;
    }

    PyObject *mmap_object_type = PyType_FromModuleAndSpec(module,
                                                  &mmap_object_spec, NULL);
    if (mmap_object_type == NULL) {
        return -1;
    }
    int rc = PyModule_AddType(module, (PyTypeObject *)mmap_object_type);
    Py_DECREF(mmap_object_type);
    if (rc < 0) {
        return -1;
    }

#define ADD_INT_MACRO(module, constant)                                     \
    do {                                                                    \
        if (PyModule_AddIntConstant(module, #constant, constant) < 0) {     \
            return -1;                                                      \
        }                                                                   \
    } while (0)

#ifdef PROT_EXEC
    ADD_INT_MACRO(module, PROT_EXEC);
#endif
#ifdef PROT_READ
    ADD_INT_MACRO(module, PROT_READ);
#endif
#ifdef PROT_WRITE
    ADD_INT_MACRO(module, PROT_WRITE);
#endif

#ifdef MAP_SHARED
    ADD_INT_MACRO(module, MAP_SHARED);
#endif
#ifdef MAP_PRIVATE
    ADD_INT_MACRO(module, MAP_PRIVATE);
#endif
#ifdef MAP_DENYWRITE
    ADD_INT_MACRO(module, MAP_DENYWRITE);
#endif
#ifdef MAP_EXECUTABLE
    ADD_INT_MACRO(module, MAP_EXECUTABLE);
#endif
#ifdef MAP_ANONYMOUS
    if (PyModule_AddIntConstant(module, "MAP_ANON", MAP_ANONYMOUS) < 0 ) {
        return -1;
    }
    ADD_INT_MACRO(module, MAP_ANONYMOUS);
#endif
#ifdef MAP_POPULATE
    ADD_INT_MACRO(module, MAP_POPULATE);
#endif
#ifdef MAP_STACK
    // Mostly a no-op on Linux and NetBSD, but useful on OpenBSD
    // for stack usage (even on x86 arch)
    ADD_INT_MACRO(module, MAP_STACK);
#endif
#ifdef MAP_ALIGNED_SUPER
    ADD_INT_MACRO(module, MAP_ALIGNED_SUPER);
#endif
#ifdef MAP_CONCEAL
    ADD_INT_MACRO(module, MAP_CONCEAL);
#endif
#ifdef MAP_NORESERVE
    ADD_INT_MACRO(module, MAP_NORESERVE);
#endif
#ifdef MAP_NOEXTEND
    ADD_INT_MACRO(module, MAP_NOEXTEND);
#endif
#ifdef MAP_HASSEMAPHORE
    ADD_INT_MACRO(module, MAP_HASSEMAPHORE);
#endif
#ifdef MAP_NOCACHE
    ADD_INT_MACRO(module, MAP_NOCACHE);
#endif
#ifdef MAP_JIT
    ADD_INT_MACRO(module, MAP_JIT);
#endif
#ifdef MAP_RESILIENT_CODESIGN
    ADD_INT_MACRO(module, MAP_RESILIENT_CODESIGN);
#endif
#ifdef MAP_RESILIENT_MEDIA
    ADD_INT_MACRO(module, MAP_RESILIENT_MEDIA);
#endif
#ifdef MAP_32BIT
    ADD_INT_MACRO(module, MAP_32BIT);
#endif
#ifdef MAP_TRANSLATED_ALLOW_EXECUTE
    ADD_INT_MACRO(module, MAP_TRANSLATED_ALLOW_EXECUTE);
#endif
#ifdef MAP_UNIX03
    ADD_INT_MACRO(module, MAP_UNIX03);
#endif
#ifdef MAP_TPRO
    ADD_INT_MACRO(module, MAP_TPRO);
#endif
    if (PyModule_AddIntConstant(module, "PAGESIZE", (long)my_getpagesize()) < 0 ) {
        return -1;
    }

    if (PyModule_AddIntConstant(module, "ALLOCATIONGRANULARITY", (long)my_getallocationgranularity()) < 0 ) {
        return -1;
    }

    ADD_INT_MACRO(module, ACCESS_DEFAULT);
    ADD_INT_MACRO(module, ACCESS_READ);
    ADD_INT_MACRO(module, ACCESS_WRITE);
    ADD_INT_MACRO(module, ACCESS_COPY);

#ifdef HAVE_MADVISE
    // Conventional advice values
#ifdef MADV_NORMAL
    ADD_INT_MACRO(module, MADV_NORMAL);
#endif
#ifdef MADV_RANDOM
    ADD_INT_MACRO(module, MADV_RANDOM);
#endif
#ifdef MADV_SEQUENTIAL
    ADD_INT_MACRO(module, MADV_SEQUENTIAL);
#endif
#ifdef MADV_WILLNEED
    ADD_INT_MACRO(module, MADV_WILLNEED);
#endif
#ifdef MADV_DONTNEED
    ADD_INT_MACRO(module, MADV_DONTNEED);
#endif

    // Linux-specific advice values
#ifdef MADV_REMOVE
    ADD_INT_MACRO(module, MADV_REMOVE);
#endif
#ifdef MADV_DONTFORK
    ADD_INT_MACRO(module, MADV_DONTFORK);
#endif
#ifdef MADV_DOFORK
    ADD_INT_MACRO(module, MADV_DOFORK);
#endif
#ifdef MADV_HWPOISON
    ADD_INT_MACRO(module, MADV_HWPOISON);
#endif
#ifdef MADV_MERGEABLE
    ADD_INT_MACRO(module, MADV_MERGEABLE);
#endif
#ifdef MADV_UNMERGEABLE
    ADD_INT_MACRO(module, MADV_UNMERGEABLE);
#endif
#ifdef MADV_SOFT_OFFLINE
    ADD_INT_MACRO(module, MADV_SOFT_OFFLINE);
#endif
#ifdef MADV_HUGEPAGE
    ADD_INT_MACRO(module, MADV_HUGEPAGE);
#endif
#ifdef MADV_NOHUGEPAGE
    ADD_INT_MACRO(module, MADV_NOHUGEPAGE);
#endif
#ifdef MADV_DONTDUMP
    ADD_INT_MACRO(module, MADV_DONTDUMP);
#endif
#ifdef MADV_DODUMP
    ADD_INT_MACRO(module, MADV_DODUMP);
#endif
#ifdef MADV_FREE // (Also present on FreeBSD and macOS.)
    ADD_INT_MACRO(module, MADV_FREE);
#endif

    // FreeBSD-specific
#ifdef MADV_NOSYNC
    ADD_INT_MACRO(module, MADV_NOSYNC);
#endif
#ifdef MADV_AUTOSYNC
    ADD_INT_MACRO(module, MADV_AUTOSYNC);
#endif
#ifdef MADV_NOCORE
    ADD_INT_MACRO(module, MADV_NOCORE);
#endif
#ifdef MADV_CORE
    ADD_INT_MACRO(module, MADV_CORE);
#endif
#ifdef MADV_PROTECT
    ADD_INT_MACRO(module, MADV_PROTECT);
#endif

    // Darwin-specific
#ifdef MADV_FREE_REUSABLE // (As MADV_FREE but reclaims more faithful for task_info/Activity Monitor...)
    ADD_INT_MACRO(module, MADV_FREE_REUSABLE);
#endif
#ifdef MADV_FREE_REUSE // (Reuse pages previously tagged as reusable)
    ADD_INT_MACRO(module, MADV_FREE_REUSE);
#endif
#endif // HAVE_MADVISE
    return 0;
}

static PyModuleDef_Slot mmap_slots[] = {
    {Py_mod_exec, mmap_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef mmapmodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "mmap",
    .m_size = 0,
    .m_slots = mmap_slots,
};

PyMODINIT_FUNC
PyInit_mmap(void)
{
    return PyModuleDef_Init(&mmapmodule);
}
