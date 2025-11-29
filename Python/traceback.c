
/* Traceback implementation */

#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallMethodFormat()
#include "pycore_fileutils.h"     // _Py_BEGIN_SUPPRESS_IPH
#include "pycore_frame.h"         // PyFrameObject
#include "pycore_interp.h"        // PyInterpreterState.gc
#include "pycore_interpframe.h"   // _PyFrame_GetCode()
#include "pycore_pyerrors.h"      // _PyErr_GetRaisedException()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_traceback.h"     // EXCEPTION_TB_HEADER

#include "frameobject.h"          // PyFrame_New()

#include "osdefs.h"               // SEP
#ifdef HAVE_UNISTD_H
#  include <unistd.h>             // lseek()
#endif

#if (defined(HAVE_EXECINFO_H) && defined(HAVE_DLFCN_H) && defined(HAVE_LINK_H))
#  define _PY_HAS_BACKTRACE_HEADERS 1
#endif

#if (defined(__APPLE__) && defined(HAVE_EXECINFO_H) && defined(HAVE_DLFCN_H))
#  define _PY_HAS_BACKTRACE_HEADERS 1
#endif

#ifdef _PY_HAS_BACKTRACE_HEADERS
#  include <execinfo.h>           // backtrace(), backtrace_symbols()
#  include <dlfcn.h>              // dladdr1()
#ifdef HAVE_LINK_H
#    include <link.h>               // struct DL_info
#endif
#  if defined(__APPLE__) && defined(HAVE_BACKTRACE) && defined(HAVE_DLADDR)
#    define CAN_C_BACKTRACE
#  elif defined(HAVE_BACKTRACE) && defined(HAVE_DLADDR1)
#    define CAN_C_BACKTRACE
#  endif
#endif

#if defined(__STDC_NO_VLA__) && (__STDC_NO_VLA__ == 1)
/* Use alloca() for VLAs. */
#  define VLA(type, name, size) type *name = alloca(size)
#elif !defined(__STDC_NO_VLA__) || (__STDC_NO_VLA__ == 0)
/* Use actual C VLAs.*/
#  define VLA(type, name, size) type name[size]
#elif defined(CAN_C_BACKTRACE)
/* VLAs are not possible. Disable C stack trace functions. */
#  undef CAN_C_BACKTRACE
#endif

#define OFF(x) offsetof(PyTracebackObject, x)
#define PUTS(fd, str) (void)_Py_write_noraise(fd, str, strlen(str))

#define MAX_STRING_LENGTH 500
#define MAX_FRAME_DEPTH 100
#define MAX_NTHREADS 100

/* Function from Parser/tokenizer/file_tokenizer.c */
extern char* _PyTokenizer_FindEncodingFilename(int, PyObject *);

/*[clinic input]
class traceback "PyTracebackObject *" "&PyTraceback_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=cf96294b2bebc811]*/

#define _PyTracebackObject_CAST(op)   ((PyTracebackObject *)(op))

#include "clinic/traceback.c.h"


#ifdef MS_WINDOWS
typedef HRESULT (WINAPI *PF_GET_THREAD_DESCRIPTION)(HANDLE, PCWSTR*);
static PF_GET_THREAD_DESCRIPTION pGetThreadDescription = NULL;
#endif


static PyObject *
tb_create_raw(PyTracebackObject *next, PyFrameObject *frame, int lasti,
              int lineno)
{
    PyTracebackObject *tb;
    if ((next != NULL && !PyTraceBack_Check(next)) ||
                    frame == NULL || !PyFrame_Check(frame)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    tb = PyObject_GC_New(PyTracebackObject, &PyTraceBack_Type);
    if (tb != NULL) {
        tb->tb_next = (PyTracebackObject*)Py_XNewRef(next);
        tb->tb_frame = (PyFrameObject*)Py_XNewRef(frame);
        tb->tb_lasti = lasti;
        tb->tb_lineno = lineno;
        PyObject_GC_Track(tb);
    }
    return (PyObject *)tb;
}

/*[clinic input]
@classmethod
traceback.__new__ as tb_new

  tb_next: object
  tb_frame: object(type='PyFrameObject *', subclass_of='&PyFrame_Type')
  tb_lasti: int
  tb_lineno: int

Create a new traceback object.
[clinic start generated code]*/

static PyObject *
tb_new_impl(PyTypeObject *type, PyObject *tb_next, PyFrameObject *tb_frame,
            int tb_lasti, int tb_lineno)
/*[clinic end generated code: output=fa077debd72d861a input=b88143145454cb59]*/
{
    if (tb_next == Py_None) {
        tb_next = NULL;
    } else if (!PyTraceBack_Check(tb_next)) {
        return PyErr_Format(PyExc_TypeError,
                            "expected traceback object or None, got '%s'",
                            Py_TYPE(tb_next)->tp_name);
    }

    return tb_create_raw((PyTracebackObject *)tb_next, tb_frame, tb_lasti,
                         tb_lineno);
}

static PyObject *
tb_dir(PyObject *Py_UNUSED(self), PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("[ssss]", "tb_frame", "tb_next",
                                   "tb_lasti", "tb_lineno");
}

/*[clinic input]
@critical_section
@getter
traceback.tb_next
[clinic start generated code]*/

static PyObject *
traceback_tb_next_get_impl(PyTracebackObject *self)
/*[clinic end generated code: output=963634df7d5fc837 input=8f6345f2b73cb965]*/
{
    PyObject* ret = (PyObject*)self->tb_next;
    if (!ret) {
        ret = Py_None;
    }
    return Py_NewRef(ret);
}

static int
tb_get_lineno(PyObject *op)
{
    PyTracebackObject *tb = _PyTracebackObject_CAST(op);
    _PyInterpreterFrame* frame = tb->tb_frame->f_frame;
    assert(frame != NULL);
    return PyCode_Addr2Line(_PyFrame_GetCode(frame), tb->tb_lasti);
}

static PyObject *
tb_lineno_get(PyObject *op, void *Py_UNUSED(_))
{
    PyTracebackObject *self = _PyTracebackObject_CAST(op);
    int lineno = self->tb_lineno;
    if (lineno == -1) {
        lineno = tb_get_lineno(op);
        if (lineno < 0) {
            Py_RETURN_NONE;
        }
    }
    return PyLong_FromLong(lineno);
}

/*[clinic input]
@critical_section
@setter
traceback.tb_next
[clinic start generated code]*/

static int
traceback_tb_next_set_impl(PyTracebackObject *self, PyObject *value)
/*[clinic end generated code: output=d4868cbc48f2adac input=ce66367f85e3c443]*/
{
    if (!value) {
        PyErr_Format(PyExc_TypeError, "can't delete tb_next attribute");
        return -1;
    }

    /* We accept None or a traceback object, and map None -> NULL (inverse of
       tb_next_get) */
    if (value == Py_None) {
        value = NULL;
    } else if (!PyTraceBack_Check(value)) {
        PyErr_Format(PyExc_TypeError,
                     "expected traceback object, got '%s'",
                     Py_TYPE(value)->tp_name);
        return -1;
    }

    /* Check for loops */
    PyTracebackObject *cursor = (PyTracebackObject *)value;
    Py_XINCREF(cursor);
    while (cursor) {
        if (cursor == self) {
            PyErr_Format(PyExc_ValueError, "traceback loop detected");
            Py_DECREF(cursor);
            return -1;
        }
        Py_BEGIN_CRITICAL_SECTION(cursor);
        Py_XINCREF(cursor->tb_next);
        Py_SETREF(cursor, cursor->tb_next);
        Py_END_CRITICAL_SECTION();
    }

    Py_XSETREF(self->tb_next, (PyTracebackObject *)Py_XNewRef(value));

    return 0;
}


static PyMethodDef tb_methods[] = {
   {"__dir__", tb_dir, METH_NOARGS, NULL},
   {NULL, NULL, 0, NULL},
};

static PyMemberDef tb_memberlist[] = {
    {"tb_frame",        _Py_T_OBJECT,       OFF(tb_frame),  Py_READONLY|Py_AUDIT_READ},
    {"tb_lasti",        Py_T_INT,          OFF(tb_lasti),  Py_READONLY},
    {NULL}      /* Sentinel */
};

static PyGetSetDef tb_getsetters[] = {
    TRACEBACK_TB_NEXT_GETSETDEF
    {"tb_lineno", tb_lineno_get, NULL, NULL, NULL},
    {NULL}      /* Sentinel */
};

static void
tb_dealloc(PyObject *op)
{
    PyTracebackObject *tb = _PyTracebackObject_CAST(op);
    PyObject_GC_UnTrack(tb);
    Py_XDECREF(tb->tb_next);
    Py_XDECREF(tb->tb_frame);
    PyObject_GC_Del(tb);
}

static int
tb_traverse(PyObject *op, visitproc visit, void *arg)
{
    PyTracebackObject *tb = _PyTracebackObject_CAST(op);
    Py_VISIT(tb->tb_next);
    Py_VISIT(tb->tb_frame);
    return 0;
}

static int
tb_clear(PyObject *op)
{
    PyTracebackObject *tb = _PyTracebackObject_CAST(op);
    Py_CLEAR(tb->tb_next);
    Py_CLEAR(tb->tb_frame);
    return 0;
}

PyTypeObject PyTraceBack_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "traceback",
    sizeof(PyTracebackObject),
    0,
    tb_dealloc,         /*tp_dealloc*/
    0,                  /*tp_vectorcall_offset*/
    0,    /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_as_async*/
    0,                  /*tp_repr*/
    0,                  /*tp_as_number*/
    0,                  /*tp_as_sequence*/
    0,                  /*tp_as_mapping*/
    0,                  /* tp_hash */
    0,                  /* tp_call */
    0,                  /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                  /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,/* tp_flags */
    tb_new__doc__,                              /* tp_doc */
    tb_traverse,                                /* tp_traverse */
    tb_clear,                                   /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    tb_methods,         /* tp_methods */
    tb_memberlist,      /* tp_members */
    tb_getsetters,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    tb_new,                                     /* tp_new */
};


PyObject*
_PyTraceBack_FromFrame(PyObject *tb_next, PyFrameObject *frame)
{
    assert(tb_next == NULL || PyTraceBack_Check(tb_next));
    assert(frame != NULL);
    int addr = _PyInterpreterFrame_LASTI(frame->f_frame) * sizeof(_Py_CODEUNIT);
    return tb_create_raw((PyTracebackObject *)tb_next, frame, addr, -1);
}


int
PyTraceBack_Here(PyFrameObject *frame)
{
    PyObject *exc = PyErr_GetRaisedException();
    assert(PyExceptionInstance_Check(exc));
    PyObject *tb = PyException_GetTraceback(exc);
    PyObject *newtb = _PyTraceBack_FromFrame(tb, frame);
    Py_XDECREF(tb);
    if (newtb == NULL) {
        _PyErr_ChainExceptions1(exc);
        return -1;
    }
    PyException_SetTraceback(exc, newtb);
    Py_XDECREF(newtb);
    PyErr_SetRaisedException(exc);
    return 0;
}

/* Insert a frame into the traceback for (funcname, filename, lineno). */
void _PyTraceback_Add(const char *funcname, const char *filename, int lineno)
{
    PyObject *globals;
    PyCodeObject *code;
    PyFrameObject *frame;
    PyThreadState *tstate = _PyThreadState_GET();

    /* Save and clear the current exception. Python functions must not be
       called with an exception set. Calling Python functions happens when
       the codec of the filesystem encoding is implemented in pure Python. */
    PyObject *exc = _PyErr_GetRaisedException(tstate);

    globals = PyDict_New();
    if (!globals)
        goto error;
    code = PyCode_NewEmpty(filename, funcname, lineno);
    if (!code) {
        Py_DECREF(globals);
        goto error;
    }
    frame = PyFrame_New(tstate, code, globals, NULL);
    Py_DECREF(globals);
    Py_DECREF(code);
    if (!frame)
        goto error;
    frame->f_lineno = lineno;

    _PyErr_SetRaisedException(tstate, exc);
    PyTraceBack_Here(frame);
    Py_DECREF(frame);
    return;

error:
    _PyErr_ChainExceptions1(exc);
}

static PyObject *
_Py_FindSourceFile(PyObject *filename, char* namebuf, size_t namelen, PyObject *io)
{
    Py_ssize_t i;
    PyObject *binary;
    PyObject *v;
    Py_ssize_t npath;
    size_t taillen;
    PyObject *syspath;
    PyObject *path;
    const char* tail;
    PyObject *filebytes;
    const char* filepath;
    Py_ssize_t len;
    PyObject* result;
    PyObject *open = NULL;

    filebytes = PyUnicode_EncodeFSDefault(filename);
    if (filebytes == NULL) {
        PyErr_Clear();
        return NULL;
    }
    filepath = PyBytes_AS_STRING(filebytes);

    /* Search tail of filename in sys.path before giving up */
    tail = strrchr(filepath, SEP);
    if (tail == NULL)
        tail = filepath;
    else
        tail++;
    taillen = strlen(tail);

    PyThreadState *tstate = _PyThreadState_GET();
    if (PySys_GetOptionalAttr(&_Py_ID(path), &syspath) < 0) {
        PyErr_Clear();
        goto error;
    }
    if (syspath == NULL || !PyList_Check(syspath)) {
        goto error;
    }
    npath = PyList_Size(syspath);

    open = PyObject_GetAttr(io, &_Py_ID(open));
    for (i = 0; i < npath; i++) {
        v = PyList_GetItem(syspath, i);
        if (v == NULL) {
            PyErr_Clear();
            break;
        }
        if (!PyUnicode_Check(v))
            continue;
        path = PyUnicode_EncodeFSDefault(v);
        if (path == NULL) {
            PyErr_Clear();
            continue;
        }
        len = PyBytes_GET_SIZE(path);
        if (len + 1 + (Py_ssize_t)taillen >= (Py_ssize_t)namelen - 1) {
            Py_DECREF(path);
            continue; /* Too long */
        }
        strcpy(namebuf, PyBytes_AS_STRING(path));
        Py_DECREF(path);
        if (strlen(namebuf) != (size_t)len)
            continue; /* v contains '\0' */
        if (len > 0 && namebuf[len-1] != SEP)
            namebuf[len++] = SEP;
        strcpy(namebuf+len, tail);

        binary = _PyObject_CallMethodFormat(tstate, open, "ss", namebuf, "rb");
        if (binary != NULL) {
            result = binary;
            goto finally;
        }
        PyErr_Clear();
    }
    goto error;

error:
    result = NULL;
finally:
    Py_XDECREF(open);
    Py_XDECREF(syspath);
    Py_DECREF(filebytes);
    return result;
}

/* Writes indent spaces. Returns 0 on success and non-zero on failure.
 */
int
_Py_WriteIndent(int indent, PyObject *f)
{
    char buf[11] = "          ";
    assert(strlen(buf) == 10);
    while (indent > 0) {
        if (indent < 10) {
            buf[indent] = '\0';
        }
        if (PyFile_WriteString(buf, f) < 0) {
            return -1;
        }
        indent -= 10;
    }
    return 0;
}

static int
display_source_line(PyObject *f, PyObject *filename, int lineno, int indent,
                    int *truncation, PyObject **line)
{
    int fd;
    int i;
    char *found_encoding;
    const char *encoding;
    PyObject *io;
    PyObject *binary;
    PyObject *fob = NULL;
    PyObject *lineobj = NULL;
    PyObject *res;
    char buf[MAXPATHLEN+1];
    int kind;
    const void *data;

    /* open the file */
    if (filename == NULL)
        return 0;

    /* Do not attempt to open things like <string> or <stdin> */
    assert(PyUnicode_Check(filename));
    if (PyUnicode_READ_CHAR(filename, 0) == '<') {
        Py_ssize_t len = PyUnicode_GET_LENGTH(filename);
        if (len > 0 && PyUnicode_READ_CHAR(filename, len - 1) == '>') {
            return 0;
        }
    }

    io = PyImport_ImportModule("io");
    if (io == NULL) {
        return -1;
    }

    binary = _PyObject_CallMethod(io, &_Py_ID(open), "Os", filename, "rb");
    if (binary == NULL) {
        PyErr_Clear();

        binary = _Py_FindSourceFile(filename, buf, sizeof(buf), io);
        if (binary == NULL) {
            Py_DECREF(io);
            return -1;
        }
    }

    /* use the right encoding to decode the file as unicode */
    fd = PyObject_AsFileDescriptor(binary);
    if (fd < 0) {
        Py_DECREF(io);
        Py_DECREF(binary);
        return 0;
    }
    found_encoding = _PyTokenizer_FindEncodingFilename(fd, filename);
    if (found_encoding == NULL)
        PyErr_Clear();
    encoding = (found_encoding != NULL) ? found_encoding : "utf-8";
    /* Reset position */
    if (lseek(fd, 0, SEEK_SET) == (off_t)-1) {
        Py_DECREF(io);
        Py_DECREF(binary);
        PyMem_Free(found_encoding);
        return 0;
    }
    fob = _PyObject_CallMethod(io, &_Py_ID(TextIOWrapper),
                               "Os", binary, encoding);
    Py_DECREF(io);
    PyMem_Free(found_encoding);

    if (fob == NULL) {
        PyErr_Clear();

        res = PyObject_CallMethodNoArgs(binary, &_Py_ID(close));
        Py_DECREF(binary);
        if (res)
            Py_DECREF(res);
        else
            PyErr_Clear();
        return 0;
    }
    Py_DECREF(binary);

    /* get the line number lineno */
    for (i = 0; i < lineno; i++) {
        Py_XDECREF(lineobj);
        lineobj = PyFile_GetLine(fob, -1);
        if (!lineobj) {
            PyErr_Clear();
            break;
        }
    }
    res = PyObject_CallMethodNoArgs(fob, &_Py_ID(close));
    if (res) {
        Py_DECREF(res);
    }
    else {
        PyErr_Clear();
    }
    Py_DECREF(fob);
    if (!lineobj || !PyUnicode_Check(lineobj)) {
        Py_XDECREF(lineobj);
        return -1;
    }

    if (line) {
        *line = Py_NewRef(lineobj);
    }

    /* remove the indentation of the line */
    kind = PyUnicode_KIND(lineobj);
    data = PyUnicode_DATA(lineobj);
    for (i=0; i < PyUnicode_GET_LENGTH(lineobj); i++) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        if (ch != ' ' && ch != '\t' && ch != '\014')
            break;
    }
    if (i) {
        PyObject *truncated;
        truncated = PyUnicode_Substring(lineobj, i, PyUnicode_GET_LENGTH(lineobj));
        if (truncated) {
            Py_SETREF(lineobj, truncated);
        } else {
            PyErr_Clear();
        }
    }

    if (truncation != NULL) {
        *truncation = i - indent;
    }

    /* Write some spaces before the line */
    if (_Py_WriteIndent(indent, f) < 0) {
        goto error;
    }

    /* finally display the line */
    if (PyFile_WriteObject(lineobj, f, Py_PRINT_RAW) < 0) {
        goto error;
    }

    if (PyFile_WriteString("\n", f) < 0) {
        goto error;
    }

    Py_DECREF(lineobj);
    return 0;
error:
    Py_DECREF(lineobj);
    return -1;
}

int
_Py_DisplaySourceLine(PyObject *f, PyObject *filename, int lineno, int indent,
                      int *truncation, PyObject **line)
{
    return display_source_line(f, filename, lineno, indent, truncation, line);
}


#define IS_WHITESPACE(c) (((c) == ' ') || ((c) == '\t') || ((c) == '\f'))
#define _TRACEBACK_SOURCE_LINE_INDENT 4

static inline int
ignore_source_errors(void) {
    if (PyErr_Occurred()) {
        if (PyErr_ExceptionMatches(PyExc_KeyboardInterrupt)) {
            return -1;
        }
        PyErr_Clear();
    }
    return 0;
}

static int
tb_displayline(PyTracebackObject* tb, PyObject *f, PyObject *filename, int lineno,
               PyFrameObject *frame, PyObject *name)
{
    if (filename == NULL || name == NULL) {
        return -1;
    }

    PyObject *line = PyUnicode_FromFormat("  File \"%U\", line %d, in %U\n",
                                          filename, lineno, name);
    if (line == NULL) {
        return -1;
    }

    int res = PyFile_WriteObject(line, f, Py_PRINT_RAW);
    Py_DECREF(line);
    if (res < 0) {
        return -1;
    }

    int err = 0;

    int truncation = _TRACEBACK_SOURCE_LINE_INDENT;
    PyObject* source_line = NULL;
    int rc = display_source_line(
            f, filename, lineno, _TRACEBACK_SOURCE_LINE_INDENT,
            &truncation, &source_line);
    if (rc != 0 || !source_line) {
        /* ignore errors since we can't report them, can we? */
        err = ignore_source_errors();
    }
    Py_XDECREF(source_line);
    return err;
}

static const int TB_RECURSIVE_CUTOFF = 3; // Also hardcoded in traceback.py.

static int
tb_print_line_repeated(PyObject *f, long cnt)
{
    cnt -= TB_RECURSIVE_CUTOFF;
    PyObject *line = PyUnicode_FromFormat(
        (cnt > 1)
          ? "  [Previous line repeated %ld more times]\n"
          : "  [Previous line repeated %ld more time]\n",
        cnt);
    if (line == NULL) {
        return -1;
    }
    int err = PyFile_WriteObject(line, f, Py_PRINT_RAW);
    Py_DECREF(line);
    return err;
}

static int
tb_printinternal(PyTracebackObject *tb, PyObject *f, long limit)
{
    PyCodeObject *code = NULL;
    Py_ssize_t depth = 0;
    PyObject *last_file = NULL;
    int last_line = -1;
    PyObject *last_name = NULL;
    long cnt = 0;
    PyTracebackObject *tb1 = tb;
    while (tb1 != NULL) {
        depth++;
        tb1 = tb1->tb_next;
    }
    while (tb != NULL && depth > limit) {
        depth--;
        tb = tb->tb_next;
    }
    while (tb != NULL) {
        code = PyFrame_GetCode(tb->tb_frame);
        int tb_lineno = tb->tb_lineno;
        if (tb_lineno == -1) {
            tb_lineno = tb_get_lineno((PyObject *)tb);
        }
        if (last_file == NULL ||
            code->co_filename != last_file ||
            last_line == -1 || tb_lineno != last_line ||
            last_name == NULL || code->co_name != last_name) {
            if (cnt > TB_RECURSIVE_CUTOFF) {
                if (tb_print_line_repeated(f, cnt) < 0) {
                    goto error;
                }
            }
            last_file = code->co_filename;
            last_line = tb_lineno;
            last_name = code->co_name;
            cnt = 0;
        }
        cnt++;
        if (cnt <= TB_RECURSIVE_CUTOFF) {
            if (tb_displayline(tb, f, code->co_filename, tb_lineno,
                               tb->tb_frame, code->co_name) < 0) {
                goto error;
            }

            if (PyErr_CheckSignals() < 0) {
                goto error;
            }
        }
        Py_CLEAR(code);
        tb = tb->tb_next;
    }
    if (cnt > TB_RECURSIVE_CUTOFF) {
        if (tb_print_line_repeated(f, cnt) < 0) {
            goto error;
        }
    }
    return 0;
error:
    Py_XDECREF(code);
    return -1;
}

#define PyTraceBack_LIMIT 1000

int
_PyTraceBack_Print(PyObject *v, const char *header, PyObject *f)
{
    PyObject *limitv;
    long limit = PyTraceBack_LIMIT;

    if (v == NULL) {
        return 0;
    }
    if (!PyTraceBack_Check(v)) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (PySys_GetOptionalAttrString("tracebacklimit", &limitv) < 0) {
        return -1;
    }
    else if (limitv != NULL && PyLong_Check(limitv)) {
        int overflow;
        limit = PyLong_AsLongAndOverflow(limitv, &overflow);
        if (overflow > 0) {
            limit = LONG_MAX;
        }
        else if (limit <= 0) {
            Py_DECREF(limitv);
            return 0;
        }
    }
    Py_XDECREF(limitv);

    if (PyFile_WriteString(header, f) < 0) {
        return -1;
    }

    if (tb_printinternal((PyTracebackObject *)v, f, limit) < 0) {
        return -1;
    }

    return 0;
}

int
PyTraceBack_Print(PyObject *v, PyObject *f)
{
    const char *header = EXCEPTION_TB_HEADER;
    return _PyTraceBack_Print(v, header, f);
}

/* Format an integer in range [0; 0xffffffff] to decimal and write it
   into the file fd.

   This function is signal safe. */

void
_Py_DumpDecimal(int fd, size_t value)
{
    /* maximum number of characters required for output of %lld or %p.
       We need at most ceil(log10(256)*SIZEOF_LONG_LONG) digits,
       plus 1 for the null byte.  53/22 is an upper bound for log10(256). */
    char buffer[1 + (sizeof(size_t)*53-1) / 22 + 1];
    char *ptr, *end;

    end = &buffer[Py_ARRAY_LENGTH(buffer) - 1];
    ptr = end;
    *ptr = '\0';
    do {
        --ptr;
        assert(ptr >= buffer);
        *ptr = '0' + (value % 10);
        value /= 10;
    } while (value);

    (void)_Py_write_noraise(fd, ptr, end - ptr);
}

/* Format an integer as hexadecimal with width digits into fd file descriptor.
   The function is signal safe. */
static void
dump_hexadecimal(int fd, uintptr_t value, Py_ssize_t width, int strip_zeros)
{
    char buffer[sizeof(uintptr_t) * 2 + 1], *ptr, *end;
    Py_ssize_t size = Py_ARRAY_LENGTH(buffer) - 1;

    if (width > size)
        width = size;
    /* it's ok if width is negative */

    end = &buffer[size];
    ptr = end;
    *ptr = '\0';
    do {
        --ptr;
        assert(ptr >= buffer);
        *ptr = Py_hexdigits[value & 15];
        value >>= 4;
    } while ((end - ptr) < width || value);

    size = end - ptr;
    if (strip_zeros) {
        while (*ptr == '0' && size >= 2) {
            ptr++;
            size--;
        }
    }

    (void)_Py_write_noraise(fd, ptr, size);
}

void
_Py_DumpHexadecimal(int fd, uintptr_t value, Py_ssize_t width)
{
    dump_hexadecimal(fd, value, width, 0);
}

#ifdef CAN_C_BACKTRACE
static void
dump_pointer(int fd, void *ptr)
{
    PUTS(fd, "0x");
    dump_hexadecimal(fd, (uintptr_t)ptr, sizeof(void*), 1);
}
#endif

static void
dump_char(int fd, char ch)
{
    char buf[1] = {ch};
    (void)_Py_write_noraise(fd, buf, 1);
}

void
_Py_DumpASCII(int fd, PyObject *text)
{
    PyASCIIObject *ascii = _PyASCIIObject_CAST(text);
    Py_ssize_t i, size;
    int truncated;
    int kind;
    void *data = NULL;
    Py_UCS4 ch;

    if (!PyUnicode_Check(text))
        return;

    size = ascii->length;
    kind = ascii->state.kind;
    if (ascii->state.compact) {
        if (ascii->state.ascii)
            data = ascii + 1;
        else
            data = _PyCompactUnicodeObject_CAST(text) + 1;
    }
    else {
        data = _PyUnicodeObject_CAST(text)->data.any;
        if (data == NULL)
            return;
    }

    if (MAX_STRING_LENGTH < size) {
        size = MAX_STRING_LENGTH;
        truncated = 1;
    }
    else {
        truncated = 0;
    }

    // Is an ASCII string?
    if (ascii->state.ascii) {
        assert(kind == PyUnicode_1BYTE_KIND);
        char *str = data;

        int need_escape = 0;
        for (i=0; i < size; i++) {
            ch = str[i];
            if (!(' ' <= ch && ch <= 126)) {
                need_escape = 1;
                break;
            }
        }
        if (!need_escape) {
            // The string can be written with a single write() syscall
            (void)_Py_write_noraise(fd, str, size);
            goto done;
        }
    }

    for (i=0; i < size; i++) {
        ch = PyUnicode_READ(kind, data, i);
        if (' ' <= ch && ch <= 126) {
            /* printable ASCII character */
            dump_char(fd, (char)ch);
        }
        else if (ch <= 0xff) {
            PUTS(fd, "\\x");
            _Py_DumpHexadecimal(fd, ch, 2);
        }
        else if (ch <= 0xffff) {
            PUTS(fd, "\\u");
            _Py_DumpHexadecimal(fd, ch, 4);
        }
        else {
            PUTS(fd, "\\U");
            _Py_DumpHexadecimal(fd, ch, 8);
        }
    }

done:
    if (truncated) {
        PUTS(fd, "...");
    }
}


#ifdef MS_WINDOWS
static void
_Py_DumpWideString(int fd, wchar_t *str)
{
    Py_ssize_t size = wcslen(str);
    int truncated;
    if (MAX_STRING_LENGTH < size) {
        size = MAX_STRING_LENGTH;
        truncated = 1;
    }
    else {
        truncated = 0;
    }

    for (Py_ssize_t i=0; i < size; i++) {
        Py_UCS4 ch = str[i];
        if (' ' <= ch && ch <= 126) {
            /* printable ASCII character */
            dump_char(fd, (char)ch);
        }
        else if (ch <= 0xff) {
            PUTS(fd, "\\x");
            _Py_DumpHexadecimal(fd, ch, 2);
        }
        else if (Py_UNICODE_IS_HIGH_SURROGATE(ch)
                 && Py_UNICODE_IS_LOW_SURROGATE(str[i+1])) {
            ch = Py_UNICODE_JOIN_SURROGATES(ch, str[i+1]);
            i++;  // Skip the low surrogate character
            PUTS(fd, "\\U");
            _Py_DumpHexadecimal(fd, ch, 8);
        }
        else {
            Py_BUILD_ASSERT(sizeof(wchar_t) == 2);
            PUTS(fd, "\\u");
            _Py_DumpHexadecimal(fd, ch, 4);
        }
    }

    if (truncated) {
        PUTS(fd, "...");
    }
}
#endif


/* Write a frame into the file fd: "File "xxx", line xxx in xxx".

   This function is signal safe.

   Return 0 on success. Return -1 if the frame is invalid. */

static int
dump_frame(int fd, _PyInterpreterFrame *frame)
{
    if (frame->owner == FRAME_OWNED_BY_INTERPRETER) {
        /* Ignore trampoline frame */
        return 0;
    }

    PyCodeObject *code = _PyFrame_SafeGetCode(frame);
    if (code == NULL) {
        return -1;
    }

    int res = 0;
    PUTS(fd, "  File ");
    if (code->co_filename != NULL
        && PyUnicode_Check(code->co_filename))
    {
        PUTS(fd, "\"");
        _Py_DumpASCII(fd, code->co_filename);
        PUTS(fd, "\"");
    }
    else {
        PUTS(fd, "???");
        res = -1;
    }

    PUTS(fd, ", line ");
    int lasti = _PyFrame_SafeGetLasti(frame);
    int lineno = -1;
    if (lasti >= 0) {
        lineno = _PyCode_SafeAddr2Line(code, lasti);
    }
    if (lineno >= 0) {
        _Py_DumpDecimal(fd, (size_t)lineno);
    }
    else {
        PUTS(fd, "???");
        res = -1;
    }

    PUTS(fd, " in ");
    if (code->co_name != NULL && PyUnicode_Check(code->co_name)) {
        _Py_DumpASCII(fd, code->co_name);
    }
    else {
        PUTS(fd, "???");
        res = -1;
    }
    PUTS(fd, "\n");
    return res;
}

static int
tstate_is_freed(PyThreadState *tstate)
{
    if (_PyMem_IsPtrFreed(tstate)) {
        return 1;
    }
    if (_PyMem_IsPtrFreed(tstate->interp)) {
        return 1;
    }
    if (_PyMem_IsULongFreed(tstate->thread_id)) {
        return 1;
    }
    return 0;
}


static int
interp_is_freed(PyInterpreterState *interp)
{
    return _PyMem_IsPtrFreed(interp);
}


static void
dump_traceback(int fd, PyThreadState *tstate, int write_header)
{
    if (write_header) {
        PUTS(fd, "Stack (most recent call first):\n");
    }

    if (tstate_is_freed(tstate)) {
        PUTS(fd, "  <freed thread state>\n");
        return;
    }

    _PyInterpreterFrame *frame = tstate->current_frame;
    if (frame == NULL) {
        PUTS(fd, "  <no Python frame>\n");
        return;
    }

    unsigned int depth = 0;
    while (1) {
        if (MAX_FRAME_DEPTH <= depth) {
            if (MAX_FRAME_DEPTH < depth) {
                PUTS(fd, "plus ");
                _Py_DumpDecimal(fd, depth);
                PUTS(fd, " frames\n");
            }
            break;
        }

        if (_PyMem_IsPtrFreed(frame)) {
            PUTS(fd, "  <freed frame>\n");
            break;
        }
        // Read frame->previous early since memory can be freed during
        // dump_frame()
        _PyInterpreterFrame *previous = frame->previous;

        if (dump_frame(fd, frame) < 0) {
            PUTS(fd, "  <invalid frame>\n");
            break;
        }

        frame = previous;
        if (frame == NULL) {
            break;
        }
        depth++;
    }
}

/* Dump the traceback of a Python thread into fd. Use write() to write the
   traceback and retry if write() is interrupted by a signal (failed with
   EINTR), but don't call the Python signal handler.

   The caller is responsible to call PyErr_CheckSignals() to call Python signal
   handlers if signals were received. */
void
_Py_DumpTraceback(int fd, PyThreadState *tstate)
{
    dump_traceback(fd, tstate, 1);
}

#if defined(HAVE_PTHREAD_GETNAME_NP) || defined(HAVE_PTHREAD_GET_NAME_NP)
# if defined(__OpenBSD__)
    /* pthread_*_np functions, especially pthread_{get,set}_name_np().
       pthread_np.h exists on both OpenBSD and FreeBSD but the latter declares
       pthread_getname_np() and pthread_setname_np() in pthread.h as long as
       __BSD_VISIBLE remains set.
     */
#   include <pthread_np.h>
# endif
#endif


// Write the thread name
static void
write_thread_name(int fd, PyThreadState *tstate)
{
#ifndef MS_WINDOWS
#if defined(HAVE_PTHREAD_GETNAME_NP) || defined(HAVE_PTHREAD_GET_NAME_NP)
    char name[100];
    pthread_t thread = (pthread_t)tstate->thread_id;
#ifdef HAVE_PTHREAD_GETNAME_NP
    int rc = pthread_getname_np(thread, name, Py_ARRAY_LENGTH(name));
#else /* defined(HAVE_PTHREAD_GET_NAME_NP) */
    int rc = 0; /* pthread_get_name_np() returns void */
    pthread_get_name_np(thread, name, Py_ARRAY_LENGTH(name));
#endif
    if (!rc) {
        size_t len = strlen(name);
        if (len) {
            PUTS(fd, " [");
            (void)_Py_write_noraise(fd, name, len);
            PUTS(fd, "]");
        }
    }
#endif
#else
    // Windows implementation
    if (pGetThreadDescription == NULL) {
        return;
    }

    HANDLE thread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, tstate->thread_id);
    if (thread == NULL) {
        return;
    }

    wchar_t *name;
    HRESULT hr = pGetThreadDescription(thread, &name);
    if (!FAILED(hr)) {
        if (name[0] != 0) {
            PUTS(fd, " [");
            _Py_DumpWideString(fd, name);
            PUTS(fd, "]");
        }
        LocalFree(name);
    }
    CloseHandle(thread);
#endif
}


/* Write the thread identifier into the file 'fd': "Current thread 0xHHHH:\" if
   is_current is true, "Thread 0xHHHH:\n" otherwise.

   This function is signal safe (except on Windows). */

static void
write_thread_id(int fd, PyThreadState *tstate, int is_current)
{
    if (is_current)
        PUTS(fd, "Current thread 0x");
    else
        PUTS(fd, "Thread 0x");
    _Py_DumpHexadecimal(fd,
                        tstate->thread_id,
                        sizeof(unsigned long) * 2);

    if (!_PyMem_IsULongFreed(tstate->thread_id)) {
        write_thread_name(fd, tstate);
    }

    PUTS(fd, " (most recent call first):\n");
}

/* Dump the traceback of all Python threads into fd. Use write() to write the
   traceback and retry if write() is interrupted by a signal (failed with
   EINTR), but don't call the Python signal handler.

   The caller is responsible to call PyErr_CheckSignals() to call Python signal
   handlers if signals were received. */
const char*
_Py_DumpTracebackThreads(int fd, PyInterpreterState *interp,
                         PyThreadState *current_tstate)
{
    if (current_tstate == NULL) {
        /* _Py_DumpTracebackThreads() is called from signal handlers by
           faulthandler.

           SIGSEGV, SIGFPE, SIGABRT, SIGBUS and SIGILL are synchronous signals
           and are thus delivered to the thread that caused the fault. Get the
           Python thread state of the current thread.

           PyThreadState_Get() doesn't give the state of the thread that caused
           the fault if the thread released the GIL, and so
           _PyThreadState_GET() cannot be used. Read the thread specific
           storage (TSS) instead: call PyGILState_GetThisThreadState(). */
        current_tstate = PyGILState_GetThisThreadState();
    }

    if (current_tstate != NULL && tstate_is_freed(current_tstate)) {
        return "tstate is freed";
    }

    if (interp == NULL) {
        if (current_tstate == NULL) {
            interp = _PyGILState_GetInterpreterStateUnsafe();
            if (interp == NULL) {
                /* We need the interpreter state to get Python threads */
                return "unable to get the interpreter state";
            }
        }
        else {
            interp = current_tstate->interp;
        }
    }
    assert(interp != NULL);

    if (interp_is_freed(interp)) {
        return "interp is freed";
    }

    /* Get the current interpreter from the current thread */
    PyThreadState *tstate = PyInterpreterState_ThreadHead(interp);
    if (tstate == NULL)
        return "unable to get the thread head state";

    /* Dump the traceback of each thread */
    unsigned int nthreads = 0;
    _Py_BEGIN_SUPPRESS_IPH
    do
    {
        if (nthreads != 0)
            PUTS(fd, "\n");
        if (nthreads >= MAX_NTHREADS) {
            PUTS(fd, "...\n");
            break;
        }

        if (tstate_is_freed(tstate)) {
            PUTS(fd, "<freed thread state>\n");
            break;
        }

        write_thread_id(fd, tstate, tstate == current_tstate);
        if (tstate == current_tstate && tstate->interp->gc.collecting) {
            PUTS(fd, "  Garbage-collecting\n");
        }
        dump_traceback(fd, tstate, 0);

        tstate = PyThreadState_Next(tstate);
        nthreads++;
    } while (tstate != NULL);
    _Py_END_SUPPRESS_IPH

    return NULL;
}

#ifdef CAN_C_BACKTRACE
/* Based on glibc's implementation of backtrace_symbols(), but only uses stack memory. */
void
_Py_backtrace_symbols_fd(int fd, void *const *array, Py_ssize_t size)
{
    VLA(Dl_info, info, size);
    VLA(int, status, size);
    /* Fill in the information we can get from dladdr() */
    for (Py_ssize_t i = 0; i < size; ++i) {
#ifdef __APPLE__
        status[i] = dladdr(array[i], &info[i]);
#else
        struct link_map *map;
        status[i] = dladdr1(array[i], &info[i], (void **)&map, RTLD_DL_LINKMAP);
        if (status[i] != 0
            && info[i].dli_fname != NULL
            && info[i].dli_fname[0] != '\0') {
            /* The load bias is more useful to the user than the load
               address. The use of these addresses is to calculate an
               address in the ELF file, so its prelinked bias is not
               something we want to subtract out */
            info[i].dli_fbase = (void *) map->l_addr;
        }
#endif
    }
    for (Py_ssize_t i = 0; i < size; ++i) {
        if (status[i] == 0
            || info[i].dli_fname == NULL
            || info[i].dli_fname[0] == '\0'
        ) {
            PUTS(fd, "  Binary file '<unknown>' [");
            dump_pointer(fd, array[i]);
            PUTS(fd, "]\n");
            continue;
        }

        if (info[i].dli_sname == NULL) {
            /* We found no symbol name to use, so describe it as
               relative to the file. */
            info[i].dli_saddr = info[i].dli_fbase;
        }

        if (info[i].dli_sname == NULL && info[i].dli_saddr == 0) {
            PUTS(fd, "  Binary file \"");
            PUTS(fd, info[i].dli_fname);
            PUTS(fd, "\" [");
            dump_pointer(fd, array[i]);
            PUTS(fd, "]\n");
        }
        else {
            char sign;
            ptrdiff_t offset;
            if (array[i] >= (void *) info[i].dli_saddr) {
                sign = '+';
                offset = array[i] - info[i].dli_saddr;
            }
            else {
                sign = '-';
                offset = info[i].dli_saddr - array[i];
            }
            const char *symbol_name = info[i].dli_sname != NULL ? info[i].dli_sname : "";
            PUTS(fd, "  Binary file \"");
            PUTS(fd, info[i].dli_fname);
            PUTS(fd, "\", at ");
            PUTS(fd, symbol_name);
            dump_char(fd, sign);
            PUTS(fd, "0x");
            dump_hexadecimal(fd, offset, sizeof(offset), 1);
            PUTS(fd, " [");
            dump_pointer(fd, array[i]);
            PUTS(fd, "]\n");
        }
    }
}

void
_Py_DumpStack(int fd)
{
#define BACKTRACE_SIZE 32
    PUTS(fd, "Current thread's C stack trace (most recent call first):\n");
    VLA(void *, callstack, BACKTRACE_SIZE);
    int frames = backtrace(callstack, BACKTRACE_SIZE);
    if (frames == 0) {
        // Some systems won't return anything for the stack trace
        PUTS(fd, "  <system returned no stack trace>\n");
        return;
    }

    _Py_backtrace_symbols_fd(fd, callstack, frames);
    if (frames == BACKTRACE_SIZE) {
        PUTS(fd, "  <truncated rest of calls>\n");
    }

#undef BACKTRACE_SIZE
}
#else
void
_Py_DumpStack(int fd)
{
    PUTS(fd, "Current thread's C stack trace (most recent call first):\n");
    PUTS(fd, "  <cannot get C stack on this system>\n");
}
#endif

void
_Py_InitDumpStack(void)
{
#ifdef CAN_C_BACKTRACE
    // gh-137185: Call backtrace() once to force libgcc to be loaded early.
    void *callstack[1];
    (void)backtrace(callstack, 1);
#endif
}


void
_Py_DumpTraceback_Init(void)
{
#ifdef MS_WINDOWS
    if (pGetThreadDescription != NULL) {
        return;
    }

    HMODULE kernelbase = GetModuleHandleW(L"kernelbase.dll");
    if (kernelbase != NULL) {
        pGetThreadDescription = (PF_GET_THREAD_DESCRIPTION)GetProcAddress(
                                    kernelbase, "GetThreadDescription");
    }
#endif
}
