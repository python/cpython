/*
    An implementation of the I/O abstract base classes hierarchy
    as defined by PEP 3116 - "New I/O"

    Classes defined here: IOBase, RawIOBase.

    Written by Amaury Forgeot d'Arc and Antoine Pitrou
*/


#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallMethod()
#include "pycore_fileutils.h"           // _PyFile_Flush
#include "pycore_long.h"          // _PyLong_GetOne()
#include "pycore_object.h"        // _PyType_HasFeature()
#include "pycore_pyerrors.h"      // _PyErr_ChainExceptions1()
#include "pycore_weakref.h"       // FT_CLEAR_WEAKREFS()

#include <stddef.h>               // offsetof()
#include "_iomodule.h"

/*[clinic input]
module _io
class _io._IOBase "PyObject *" "clinic_state()->PyIOBase_Type"
class _io._RawIOBase "PyObject *" "clinic_state()->PyRawIOBase_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=9006b7802ab8ea85]*/

/*
 * IOBase class, an abstract class
 */

typedef struct {
    PyObject_HEAD

    PyObject *dict;
    PyObject *weakreflist;
} iobase;

#define iobase_CAST(op) ((iobase *)(op))

PyDoc_STRVAR(iobase_doc,
    "The abstract base class for all I/O classes.\n"
    "\n"
    "This class provides dummy implementations for many methods that\n"
    "derived classes can override selectively; the default implementations\n"
    "represent a file that cannot be read, written or seeked.\n"
    "\n"
    "Even though IOBase does not declare read, readinto, or write because\n"
    "their signatures will vary, implementations and clients should\n"
    "consider those methods part of the interface. Also, implementations\n"
    "may raise UnsupportedOperation when operations they do not support are\n"
    "called.\n"
    "\n"
    "The basic type used for binary data read from or written to a file is\n"
    "bytes. Other bytes-like objects are accepted as method arguments too.\n"
    "In some cases (such as readinto), a writable object is required. Text\n"
    "I/O classes work with str data.\n"
    "\n"
    "Note that calling any method (except additional calls to close(),\n"
    "which are ignored) on a closed stream should raise a ValueError.\n"
    "\n"
    "IOBase (and its subclasses) support the iterator protocol, meaning\n"
    "that an IOBase object can be iterated over yielding the lines in a\n"
    "stream.\n"
    "\n"
    "IOBase also supports the :keyword:`with` statement. In this example,\n"
    "fp is closed after the suite of the with statement is complete:\n"
    "\n"
    "with open('spam.txt', 'r') as fp:\n"
    "    fp.write('Spam and eggs!')\n");


/* Internal methods */

/* Use this function whenever you want to check the internal `closed` status
   of the IOBase object rather than the virtual `closed` attribute as returned
   by whatever subclass. */

static int
iobase_is_closed(PyObject *self)
{
    return PyObject_HasAttrWithError(self, &_Py_ID(__IOBase_closed));
}

static PyObject *
iobase_unsupported(_PyIO_State *state, const char *message)
{
    PyErr_SetString(state->unsupported_operation, message);
    return NULL;
}

/* Positioning */

/*[clinic input]
_io._IOBase.seek
    cls: defining_class
    offset: int(unused=True)
      The stream position, relative to 'whence'.
    whence: int(unused=True, c_default='0') = os.SEEK_SET
      The relative position to seek from.
    /

Change the stream position to the given byte offset.

The offset is interpreted relative to the position indicated by whence.
Values for whence are:

* os.SEEK_SET or 0 -- start of stream (the default); offset should be zero or positive
* os.SEEK_CUR or 1 -- current stream position; offset may be negative
* os.SEEK_END or 2 -- end of stream; offset is usually negative

Return the new absolute position.
[clinic start generated code]*/

static PyObject *
_io__IOBase_seek_impl(PyObject *self, PyTypeObject *cls,
                      int Py_UNUSED(offset), int Py_UNUSED(whence))
/*[clinic end generated code: output=8bd74ea6538ded53 input=74211232b363363e]*/
{
    _PyIO_State *state = get_io_state_by_cls(cls);
    return iobase_unsupported(state, "seek");
}

/*[clinic input]
_io._IOBase.tell

Return current stream position.
[clinic start generated code]*/

static PyObject *
_io__IOBase_tell_impl(PyObject *self)
/*[clinic end generated code: output=89a1c0807935abe2 input=04e615fec128801f]*/
{
    return _PyObject_CallMethod(self, &_Py_ID(seek), "ii", 0, 1);
}

/*[clinic input]
_io._IOBase.truncate
    cls: defining_class
    size: object(unused=True) = None
    /

Truncate file to size bytes.

File pointer is left unchanged. Size defaults to the current IO position
as reported by tell(). Return the new size.
[clinic start generated code]*/

static PyObject *
_io__IOBase_truncate_impl(PyObject *self, PyTypeObject *cls,
                          PyObject *Py_UNUSED(size))
/*[clinic end generated code: output=2013179bff1fe8ef input=660ac20936612c27]*/
{
    _PyIO_State *state = get_io_state_by_cls(cls);
    return iobase_unsupported(state, "truncate");
}

/* Flush and close methods */

/*[clinic input]
_io._IOBase.flush

Flush write buffers, if applicable.

This is not implemented for read-only and non-blocking streams.
[clinic start generated code]*/

static PyObject *
_io__IOBase_flush_impl(PyObject *self)
/*[clinic end generated code: output=7cef4b4d54656a3b input=773be121abe270aa]*/
{
    /* XXX Should this return the number of bytes written??? */
    int closed = iobase_is_closed(self);

    if (!closed) {
        Py_RETURN_NONE;
    }
    if (closed > 0) {
        PyErr_SetString(PyExc_ValueError, "I/O operation on closed file.");
    }
    return NULL;
}

static PyObject *
iobase_closed_get(PyObject *self, void *context)
{
    int closed = iobase_is_closed(self);
    if (closed < 0) {
        return NULL;
    }
    return PyBool_FromLong(closed);
}

static int
iobase_check_closed(PyObject *self)
{
    PyObject *res;
    int closed;
    /* This gets the derived attribute, which is *not* __IOBase_closed
       in most cases! */
    closed = PyObject_GetOptionalAttr(self, &_Py_ID(closed), &res);
    if (closed > 0) {
        closed = PyObject_IsTrue(res);
        Py_DECREF(res);
        if (closed > 0) {
            PyErr_SetString(PyExc_ValueError, "I/O operation on closed file.");
            return -1;
        }
    }
    return closed;
}

PyObject *
_PyIOBase_check_closed(PyObject *self, PyObject *args)
{
    if (iobase_check_closed(self)) {
        return NULL;
    }
    if (args == Py_True) {
        return Py_None;
    }
    Py_RETURN_NONE;
}

static PyObject *
iobase_check_seekable(PyObject *self, PyObject *args)
{
    _PyIO_State *state = find_io_state_by_def(Py_TYPE(self));
    return _PyIOBase_check_seekable(state, self, args);
}

static PyObject *
iobase_check_readable(PyObject *self, PyObject *args)
{
    _PyIO_State *state = find_io_state_by_def(Py_TYPE(self));
    return _PyIOBase_check_readable(state, self, args);
}

static PyObject *
iobase_check_writable(PyObject *self, PyObject *args)
{
    _PyIO_State *state = find_io_state_by_def(Py_TYPE(self));
    return _PyIOBase_check_writable(state, self, args);
}

PyObject *
_PyIOBase_cannot_pickle(PyObject *self, PyObject *args)
{
    PyErr_Format(PyExc_TypeError,
        "cannot pickle '%.100s' instances", _PyType_Name(Py_TYPE(self)));
    return NULL;
}

/* XXX: IOBase thinks it has to maintain its own internal state in
   `__IOBase_closed` and call flush() by itself, but it is redundant with
   whatever behaviour a non-trivial derived class will implement. */

/*[clinic input]
_io._IOBase.close

Flush and close the IO object.

This method has no effect if the file is already closed.
[clinic start generated code]*/

static PyObject *
_io__IOBase_close_impl(PyObject *self)
/*[clinic end generated code: output=63c6a6f57d783d6d input=f4494d5c31dbc6b7]*/
{
    int rc1, rc2, closed = iobase_is_closed(self);

    if (closed < 0) {
        return NULL;
    }
    if (closed) {
        Py_RETURN_NONE;
    }

    rc1 = _PyFile_Flush(self);
    PyObject *exc = PyErr_GetRaisedException();
    rc2 = PyObject_SetAttr(self, &_Py_ID(__IOBase_closed), Py_True);
    _PyErr_ChainExceptions1(exc);
    if (rc1 < 0 || rc2 < 0) {
        return NULL;
    }

    Py_RETURN_NONE;
}

/* Finalization and garbage collection support */

static void
iobase_finalize(PyObject *self)
{
    PyObject *res;
    int closed;

    /* Save the current exception, if any. */
    PyObject *exc = PyErr_GetRaisedException();

    /* If `closed` doesn't exist or can't be evaluated as bool, then the
       object is probably in an unusable state, so ignore. */
    if (PyObject_GetOptionalAttr(self, &_Py_ID(closed), &res) <= 0) {
        PyErr_Clear();
        closed = -1;
    }
    else {
        closed = PyObject_IsTrue(res);
        Py_DECREF(res);
        if (closed == -1)
            PyErr_Clear();
    }
    if (closed == 0) {
        /* Signal close() that it was called as part of the object
           finalization process. */
        if (PyObject_SetAttr(self, &_Py_ID(_finalizing), Py_True))
            PyErr_Clear();
        res = PyObject_CallMethodNoArgs((PyObject *)self, &_Py_ID(close));
        if (res == NULL) {
            PyErr_FormatUnraisable("Exception ignored "
                                   "while finalizing file %R", self);
        }
        else {
            Py_DECREF(res);
        }
    }

    /* Restore the saved exception. */
    PyErr_SetRaisedException(exc);
}

int
_PyIOBase_finalize(PyObject *self)
{
    int is_zombie;

    /* If _PyIOBase_finalize() is called from a destructor, we need to
       resurrect the object as calling close() can invoke arbitrary code. */
    is_zombie = (Py_REFCNT(self) == 0);
    if (is_zombie)
        return PyObject_CallFinalizerFromDealloc(self);
    else {
        PyObject_CallFinalizer(self);
        return 0;
    }
}

static int
iobase_traverse(PyObject *op, visitproc visit, void *arg)
{
    iobase *self = iobase_CAST(op);
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->dict);
    return 0;
}

static int
iobase_clear(PyObject *op)
{
    iobase *self = iobase_CAST(op);
    Py_CLEAR(self->dict);
    return 0;
}

/* Destructor */

static void
iobase_dealloc(PyObject *op)
{
    /* NOTE: since IOBaseObject has its own dict, Python-defined attributes
       are still available here for close() to use.
       However, if the derived class declares a __slots__, those slots are
       already gone.
    */
    iobase *self = iobase_CAST(op);
    if (_PyIOBase_finalize(op) < 0) {
        /* When called from a heap type's dealloc, the type will be
           decref'ed on return (see e.g. subtype_dealloc in typeobject.c). */
        if (_PyType_HasFeature(Py_TYPE(self), Py_TPFLAGS_HEAPTYPE)) {
            Py_INCREF(Py_TYPE(self));
        }
        return;
    }
    PyTypeObject *tp = Py_TYPE(self);
    _PyObject_GC_UNTRACK(self);
    FT_CLEAR_WEAKREFS(op, self->weakreflist);
    Py_CLEAR(self->dict);
    tp->tp_free(self);
    Py_DECREF(tp);
}

/* Inquiry methods */

/*[clinic input]
_io._IOBase.seekable

Return whether object supports random access.

If False, seek(), tell() and truncate() will raise OSError.
This method may need to do a test seek().
[clinic start generated code]*/

static PyObject *
_io__IOBase_seekable_impl(PyObject *self)
/*[clinic end generated code: output=4c24c67f5f32a43d input=b976622f7fdf3063]*/
{
    Py_RETURN_FALSE;
}

PyObject *
_PyIOBase_check_seekable(_PyIO_State *state, PyObject *self, PyObject *args)
{
    PyObject *res  = PyObject_CallMethodNoArgs(self, &_Py_ID(seekable));
    if (res == NULL)
        return NULL;
    if (res != Py_True) {
        Py_CLEAR(res);
        iobase_unsupported(state, "File or stream is not seekable.");
        return NULL;
    }
    if (args == Py_True) {
        Py_DECREF(res);
    }
    return res;
}

/*[clinic input]
_io._IOBase.readable

Return whether object was opened for reading.

If False, read() will raise OSError.
[clinic start generated code]*/

static PyObject *
_io__IOBase_readable_impl(PyObject *self)
/*[clinic end generated code: output=e48089250686388b input=285b3b866a0ec35f]*/
{
    Py_RETURN_FALSE;
}

/* May be called with any object */
PyObject *
_PyIOBase_check_readable(_PyIO_State *state, PyObject *self, PyObject *args)
{
    PyObject *res = PyObject_CallMethodNoArgs(self, &_Py_ID(readable));
    if (res == NULL)
        return NULL;
    if (res != Py_True) {
        Py_CLEAR(res);
        iobase_unsupported(state, "File or stream is not readable.");
        return NULL;
    }
    if (args == Py_True) {
        Py_DECREF(res);
    }
    return res;
}

/*[clinic input]
_io._IOBase.writable

Return whether object was opened for writing.

If False, write() will raise OSError.
[clinic start generated code]*/

static PyObject *
_io__IOBase_writable_impl(PyObject *self)
/*[clinic end generated code: output=406001d0985be14f input=9dcac18a013a05b5]*/
{
    Py_RETURN_FALSE;
}

/* May be called with any object */
PyObject *
_PyIOBase_check_writable(_PyIO_State *state, PyObject *self, PyObject *args)
{
    PyObject *res = PyObject_CallMethodNoArgs(self, &_Py_ID(writable));
    if (res == NULL)
        return NULL;
    if (res != Py_True) {
        Py_CLEAR(res);
        iobase_unsupported(state, "File or stream is not writable.");
        return NULL;
    }
    if (args == Py_True) {
        Py_DECREF(res);
    }
    return res;
}

/* Context manager */

static PyObject *
iobase_enter(PyObject *self, PyObject *args)
{
    if (iobase_check_closed(self))
        return NULL;

    return Py_NewRef(self);
}

static PyObject *
iobase_exit(PyObject *self, PyObject *args)
{
    return PyObject_CallMethodNoArgs(self, &_Py_ID(close));
}

/* Lower-level APIs */

/* XXX Should these be present even if unimplemented? */

/*[clinic input]
_io._IOBase.fileno
    cls: defining_class
    /

Return underlying file descriptor if one exists.

Raise OSError if the IO object does not use a file descriptor.
[clinic start generated code]*/

static PyObject *
_io__IOBase_fileno_impl(PyObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=7caaa32a6f4ada3d input=1927c8bea5c85099]*/
{
    _PyIO_State *state = get_io_state_by_cls(cls);
    return iobase_unsupported(state, "fileno");
}

/*[clinic input]
_io._IOBase.isatty

Return whether this is an 'interactive' stream.

Return False if it can't be determined.
[clinic start generated code]*/

static PyObject *
_io__IOBase_isatty_impl(PyObject *self)
/*[clinic end generated code: output=60cab77cede41cdd input=9ef76530d368458b]*/
{
    if (iobase_check_closed(self))
        return NULL;
    Py_RETURN_FALSE;
}

/* Readline(s) and writelines */

/*[clinic input]
_io._IOBase.readline
    size as limit: Py_ssize_t(accept={int, NoneType}) = -1
    /

Read and return a line from the stream.

If size is specified, at most size bytes will be read.

The line terminator is always b'\n' for binary files; for text
files, the newlines argument to open can be used to select the line
terminator(s) recognized.
[clinic start generated code]*/

static PyObject *
_io__IOBase_readline_impl(PyObject *self, Py_ssize_t limit)
/*[clinic end generated code: output=4479f79b58187840 input=d0c596794e877bff]*/
{
    /* For backwards compatibility, a (slowish) readline(). */

    PyObject *peek, *buffer, *result;
    Py_ssize_t old_size = -1;

    if (PyObject_GetOptionalAttr(self, &_Py_ID(peek), &peek) < 0) {
        return NULL;
    }

    buffer = PyByteArray_FromStringAndSize(NULL, 0);
    if (buffer == NULL) {
        Py_XDECREF(peek);
        return NULL;
    }

    while (limit < 0 || PyByteArray_GET_SIZE(buffer) < limit) {
        Py_ssize_t nreadahead = 1;
        PyObject *b;

        if (peek != NULL) {
            PyObject *readahead = PyObject_CallOneArg(peek, _PyLong_GetOne());
            if (readahead == NULL) {
                /* NOTE: PyErr_SetFromErrno() calls PyErr_CheckSignals()
                   when EINTR occurs so we needn't do it ourselves. */
                if (_PyIO_trap_eintr()) {
                    continue;
                }
                goto fail;
            }
            if (!PyBytes_Check(readahead)) {
                PyErr_Format(PyExc_OSError,
                             "peek() should have returned a bytes object, "
                             "not '%.200s'", Py_TYPE(readahead)->tp_name);
                Py_DECREF(readahead);
                goto fail;
            }
            if (PyBytes_GET_SIZE(readahead) > 0) {
                Py_ssize_t n = 0;
                const char *buf = PyBytes_AS_STRING(readahead);
                if (limit >= 0) {
                    do {
                        if (n >= PyBytes_GET_SIZE(readahead) || n >= limit)
                            break;
                        if (buf[n++] == '\n')
                            break;
                    } while (1);
                }
                else {
                    do {
                        if (n >= PyBytes_GET_SIZE(readahead))
                            break;
                        if (buf[n++] == '\n')
                            break;
                    } while (1);
                }
                nreadahead = n;
            }
            Py_DECREF(readahead);
        }

        b = _PyObject_CallMethod(self, &_Py_ID(read), "n", nreadahead);
        if (b == NULL) {
            /* NOTE: PyErr_SetFromErrno() calls PyErr_CheckSignals()
               when EINTR occurs so we needn't do it ourselves. */
            if (_PyIO_trap_eintr()) {
                continue;
            }
            goto fail;
        }
        if (!PyBytes_Check(b)) {
            PyErr_Format(PyExc_OSError,
                         "read() should have returned a bytes object, "
                         "not '%.200s'", Py_TYPE(b)->tp_name);
            Py_DECREF(b);
            goto fail;
        }
        if (PyBytes_GET_SIZE(b) == 0) {
            Py_DECREF(b);
            break;
        }

        old_size = PyByteArray_GET_SIZE(buffer);
        if (PyByteArray_Resize(buffer, old_size + PyBytes_GET_SIZE(b)) < 0) {
            Py_DECREF(b);
            goto fail;
        }
        memcpy(PyByteArray_AS_STRING(buffer) + old_size,
               PyBytes_AS_STRING(b), PyBytes_GET_SIZE(b));

        Py_DECREF(b);

        if (PyByteArray_AS_STRING(buffer)[PyByteArray_GET_SIZE(buffer) - 1] == '\n')
            break;
    }

    result = PyBytes_FromStringAndSize(PyByteArray_AS_STRING(buffer),
                                       PyByteArray_GET_SIZE(buffer));
    Py_XDECREF(peek);
    Py_DECREF(buffer);
    return result;
  fail:
    Py_XDECREF(peek);
    Py_DECREF(buffer);
    return NULL;
}

static PyObject *
iobase_iter(PyObject *self)
{
    if (iobase_check_closed(self))
        return NULL;

    return Py_NewRef(self);
}

static PyObject *
iobase_iternext(PyObject *self)
{
    PyObject *line = PyObject_CallMethodNoArgs(self, &_Py_ID(readline));

    if (line == NULL)
        return NULL;

    if (PyObject_Size(line) <= 0) {
        /* Error or empty */
        Py_DECREF(line);
        return NULL;
    }

    return line;
}

/*[clinic input]
_io._IOBase.readlines
    hint: Py_ssize_t(accept={int, NoneType}) = -1
    /

Return a list of lines from the stream.

hint can be specified to control the number of lines read: no more
lines will be read if the total size (in bytes/characters) of all
lines so far exceeds hint.
[clinic start generated code]*/

static PyObject *
_io__IOBase_readlines_impl(PyObject *self, Py_ssize_t hint)
/*[clinic end generated code: output=2f50421677fa3dea input=9400c786ea9dc416]*/
{
    Py_ssize_t length = 0;
    PyObject *result, *it = NULL;

    result = PyList_New(0);
    if (result == NULL)
        return NULL;

    if (hint <= 0) {
        /* XXX special-casing this made sense in the Python version in order
           to remove the bytecode interpretation overhead, but it could
           probably be removed here. */
        PyObject *ret = PyObject_CallMethodObjArgs(result, &_Py_ID(extend),
                                                   self, NULL);
        if (ret == NULL) {
            goto error;
        }
        Py_DECREF(ret);
        return result;
    }

    it = PyObject_GetIter(self);
    if (it == NULL) {
        goto error;
    }

    while (1) {
        Py_ssize_t line_length;
        PyObject *line = PyIter_Next(it);
        if (line == NULL) {
            if (PyErr_Occurred()) {
                goto error;
            }
            else
                break; /* StopIteration raised */
        }

        if (PyList_Append(result, line) < 0) {
            Py_DECREF(line);
            goto error;
        }
        line_length = PyObject_Size(line);
        Py_DECREF(line);
        if (line_length < 0) {
            goto error;
        }
        if (line_length > hint - length)
            break;
        length += line_length;
    }

    Py_DECREF(it);
    return result;

 error:
    Py_XDECREF(it);
    Py_DECREF(result);
    return NULL;
}

/*[clinic input]
_io._IOBase.writelines
    lines: object
    /

Write a list of lines to stream.

Line separators are not added, so it is usual for each of the
lines provided to have a line separator at the end.
[clinic start generated code]*/

static PyObject *
_io__IOBase_writelines(PyObject *self, PyObject *lines)
/*[clinic end generated code: output=976eb0a9b60a6628 input=cac3fc8864183359]*/
{
    PyObject *iter, *res;

    if (iobase_check_closed(self))
        return NULL;

    iter = PyObject_GetIter(lines);
    if (iter == NULL)
        return NULL;

    while (1) {
        PyObject *line = PyIter_Next(iter);
        if (line == NULL) {
            if (PyErr_Occurred()) {
                Py_DECREF(iter);
                return NULL;
            }
            else
                break; /* Stop Iteration */
        }

        res = NULL;
        do {
            res = PyObject_CallMethodObjArgs(self, &_Py_ID(write), line, NULL);
        } while (res == NULL && _PyIO_trap_eintr());
        Py_DECREF(line);
        if (res == NULL) {
            Py_DECREF(iter);
            return NULL;
        }
        Py_DECREF(res);
    }
    Py_DECREF(iter);
    Py_RETURN_NONE;
}

#define clinic_state() (find_io_state_by_def(Py_TYPE(self)))
#include "clinic/iobase.c.h"
#undef clinic_state

static PyMethodDef iobase_methods[] = {
    _IO__IOBASE_SEEK_METHODDEF
    _IO__IOBASE_TELL_METHODDEF
    _IO__IOBASE_TRUNCATE_METHODDEF
    _IO__IOBASE_FLUSH_METHODDEF
    _IO__IOBASE_CLOSE_METHODDEF

    _IO__IOBASE_SEEKABLE_METHODDEF
    _IO__IOBASE_READABLE_METHODDEF
    _IO__IOBASE_WRITABLE_METHODDEF

    {"_checkClosed",   _PyIOBase_check_closed, METH_NOARGS},
    {"_checkSeekable", iobase_check_seekable, METH_NOARGS},
    {"_checkReadable", iobase_check_readable, METH_NOARGS},
    {"_checkWritable", iobase_check_writable, METH_NOARGS},

    _IO__IOBASE_FILENO_METHODDEF
    _IO__IOBASE_ISATTY_METHODDEF

    {"__enter__", iobase_enter, METH_NOARGS},
    {"__exit__", iobase_exit, METH_VARARGS},

    _IO__IOBASE_READLINE_METHODDEF
    _IO__IOBASE_READLINES_METHODDEF
    _IO__IOBASE_WRITELINES_METHODDEF

    {NULL, NULL}
};

static PyGetSetDef iobase_getset[] = {
    {"__dict__", PyObject_GenericGetDict, NULL, NULL},
    {"closed", iobase_closed_get, NULL, NULL},
    {NULL}
};

static struct PyMemberDef iobase_members[] = {
    {"__weaklistoffset__", Py_T_PYSSIZET, offsetof(iobase, weakreflist), Py_READONLY},
    {"__dictoffset__", Py_T_PYSSIZET, offsetof(iobase, dict), Py_READONLY},
    {NULL},
};


static PyType_Slot iobase_slots[] = {
    {Py_tp_dealloc, iobase_dealloc},
    {Py_tp_doc, (void *)iobase_doc},
    {Py_tp_traverse, iobase_traverse},
    {Py_tp_clear, iobase_clear},
    {Py_tp_iter, iobase_iter},
    {Py_tp_iternext, iobase_iternext},
    {Py_tp_methods, iobase_methods},
    {Py_tp_members, iobase_members},
    {Py_tp_getset, iobase_getset},
    {Py_tp_finalize, iobase_finalize},
    {0, NULL},
};

PyType_Spec iobase_spec = {
    .name = "_io._IOBase",
    .basicsize = sizeof(iobase),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = iobase_slots,
};

/*
 * RawIOBase class, Inherits from IOBase.
 */
PyDoc_STRVAR(rawiobase_doc,
             "Base class for raw binary I/O.");

/*
 * The read() method is implemented by calling readinto(); derived classes
 * that want to support read() only need to implement readinto() as a
 * primitive operation.  In general, readinto() can be more efficient than
 * read().
 *
 * (It would be tempting to also provide an implementation of readinto() in
 * terms of read(), in case the latter is a more suitable primitive operation,
 * but that would lead to nasty recursion in case a subclass doesn't implement
 * either.)
*/

/*[clinic input]
_io._RawIOBase.read
    size as n: Py_ssize_t = -1
    /
[clinic start generated code]*/

static PyObject *
_io__RawIOBase_read_impl(PyObject *self, Py_ssize_t n)
/*[clinic end generated code: output=6cdeb731e3c9f13c input=b6d0dcf6417d1374]*/
{
    PyObject *b, *res;

    if (n < 0) {
        return PyObject_CallMethodNoArgs(self, &_Py_ID(readall));
    }

    /* TODO: allocate a bytes object directly instead and manually construct
       a writable memoryview pointing to it. */
    b = PyByteArray_FromStringAndSize(NULL, n);
    if (b == NULL)
        return NULL;

    res = PyObject_CallMethodObjArgs(self, &_Py_ID(readinto), b, NULL);
    if (res == NULL || res == Py_None) {
        Py_DECREF(b);
        return res;
    }

    n = PyNumber_AsSsize_t(res, PyExc_ValueError);
    Py_DECREF(res);
    if (n == -1 && PyErr_Occurred()) {
        Py_DECREF(b);
        return NULL;
    }

    res = PyBytes_FromStringAndSize(PyByteArray_AsString(b), n);
    Py_DECREF(b);
    return res;
}


/*[clinic input]
_io._RawIOBase.readall

Read until EOF, using multiple read() call.
[clinic start generated code]*/

static PyObject *
_io__RawIOBase_readall_impl(PyObject *self)
/*[clinic end generated code: output=1987b9ce929425a0 input=688874141213622a]*/
{
    int r;
    PyObject *chunks = PyList_New(0);
    PyObject *result;

    if (chunks == NULL)
        return NULL;

    while (1) {
        PyObject *data = _PyObject_CallMethod(self, &_Py_ID(read),
                                              "i", DEFAULT_BUFFER_SIZE);
        if (!data) {
            /* NOTE: PyErr_SetFromErrno() calls PyErr_CheckSignals()
               when EINTR occurs so we needn't do it ourselves. */
            if (_PyIO_trap_eintr()) {
                continue;
            }
            Py_DECREF(chunks);
            return NULL;
        }
        if (data == Py_None) {
            if (PyList_GET_SIZE(chunks) == 0) {
                Py_DECREF(chunks);
                return data;
            }
            Py_DECREF(data);
            break;
        }
        if (!PyBytes_Check(data)) {
            Py_DECREF(chunks);
            Py_DECREF(data);
            PyErr_SetString(PyExc_TypeError, "read() should return bytes");
            return NULL;
        }
        if (PyBytes_GET_SIZE(data) == 0) {
            /* EOF */
            Py_DECREF(data);
            break;
        }
        r = PyList_Append(chunks, data);
        Py_DECREF(data);
        if (r < 0) {
            Py_DECREF(chunks);
            return NULL;
        }
    }
    result = PyBytes_Join((PyObject *)&_Py_SINGLETON(bytes_empty), chunks);
    Py_DECREF(chunks);
    return result;
}

static PyObject *
rawiobase_readinto(PyObject *self, PyObject *args)
{
    PyErr_SetNone(PyExc_NotImplementedError);
    return NULL;
}

static PyObject *
rawiobase_write(PyObject *self, PyObject *args)
{
    PyErr_SetNone(PyExc_NotImplementedError);
    return NULL;
}

static PyMethodDef rawiobase_methods[] = {
    _IO__RAWIOBASE_READ_METHODDEF
    _IO__RAWIOBASE_READALL_METHODDEF
    {"readinto", rawiobase_readinto, METH_VARARGS},
    {"write", rawiobase_write, METH_VARARGS},
    {NULL, NULL}
};

static PyType_Slot rawiobase_slots[] = {
    {Py_tp_doc, (void *)rawiobase_doc},
    {Py_tp_methods, rawiobase_methods},
    {0, NULL},
};

/* Do not set Py_TPFLAGS_HAVE_GC so that tp_traverse and tp_clear are inherited */
PyType_Spec rawiobase_spec = {
    .name = "_io._RawIOBase",
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE |
              Py_TPFLAGS_IMMUTABLETYPE),
    .slots = rawiobase_slots,
};
