
/* Traceback implementation */

#include "Python.h"

#include "pycore_ast.h"           // asdl_seq_*
#include "pycore_call.h"          // _PyObject_CallMethodFormat()
#include "pycore_compile.h"       // _PyAST_Optimize
#include "pycore_fileutils.h"     // _Py_BEGIN_SUPPRESS_IPH
#include "pycore_frame.h"         // _PyFrame_GetCode()
#include "pycore_interp.h"        // PyInterpreterState.gc
#include "pycore_parser.h"        // _PyParser_ASTFromString
#include "pycore_pyarena.h"       // _PyArena_Free()
#include "pycore_pyerrors.h"      // _PyErr_Fetch()
#include "pycore_pystate.h"       // _PyThreadState_GET()
#include "pycore_traceback.h"     // EXCEPTION_TB_HEADER

#include "../Parser/pegen.h"      // _PyPegen_byte_offset_to_character_offset()
#include "frameobject.h"          // PyFrame_New()
#include "structmember.h"         // PyMemberDef
#include "osdefs.h"               // SEP
#ifdef HAVE_FCNTL_H
#  include <fcntl.h>
#endif

#define OFF(x) offsetof(PyTracebackObject, x)

#define PUTS(fd, str) _Py_write_noraise(fd, str, (int)strlen(str))
#define MAX_STRING_LENGTH 500
#define MAX_FRAME_DEPTH 100
#define MAX_NTHREADS 100

/* Function from Parser/tokenizer.c */
extern char* _PyTokenizer_FindEncodingFilename(int, PyObject *);

/*[clinic input]
class TracebackType "PyTracebackObject *" "&PyTraceback_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=928fa06c10151120]*/

#include "clinic/traceback.c.h"

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
        Py_XINCREF(next);
        tb->tb_next = next;
        Py_XINCREF(frame);
        tb->tb_frame = frame;
        tb->tb_lasti = lasti;
        tb->tb_lineno = lineno;
        PyObject_GC_Track(tb);
    }
    return (PyObject *)tb;
}

/*[clinic input]
@classmethod
TracebackType.__new__ as tb_new

  tb_next: object
  tb_frame: object(type='PyFrameObject *', subclass_of='&PyFrame_Type')
  tb_lasti: int
  tb_lineno: int

Create a new traceback object.
[clinic start generated code]*/

static PyObject *
tb_new_impl(PyTypeObject *type, PyObject *tb_next, PyFrameObject *tb_frame,
            int tb_lasti, int tb_lineno)
/*[clinic end generated code: output=fa077debd72d861a input=01cbe8ec8783fca7]*/
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
tb_dir(PyTracebackObject *self, PyObject *Py_UNUSED(ignored))
{
    return Py_BuildValue("[ssss]", "tb_frame", "tb_next",
                                   "tb_lasti", "tb_lineno");
}

static PyObject *
tb_next_get(PyTracebackObject *self, void *Py_UNUSED(_))
{
    PyObject* ret = (PyObject*)self->tb_next;
    if (!ret) {
        ret = Py_None;
    }
    Py_INCREF(ret);
    return ret;
}

static int
tb_next_set(PyTracebackObject *self, PyObject *new_next, void *Py_UNUSED(_))
{
    if (!new_next) {
        PyErr_Format(PyExc_TypeError, "can't delete tb_next attribute");
        return -1;
    }

    /* We accept None or a traceback object, and map None -> NULL (inverse of
       tb_next_get) */
    if (new_next == Py_None) {
        new_next = NULL;
    } else if (!PyTraceBack_Check(new_next)) {
        PyErr_Format(PyExc_TypeError,
                     "expected traceback object, got '%s'",
                     Py_TYPE(new_next)->tp_name);
        return -1;
    }

    /* Check for loops */
    PyTracebackObject *cursor = (PyTracebackObject *)new_next;
    while (cursor) {
        if (cursor == self) {
            PyErr_Format(PyExc_ValueError, "traceback loop detected");
            return -1;
        }
        cursor = cursor->tb_next;
    }

    PyObject *old_next = (PyObject*)self->tb_next;
    Py_XINCREF(new_next);
    self->tb_next = (PyTracebackObject *)new_next;
    Py_XDECREF(old_next);

    return 0;
}


static PyMethodDef tb_methods[] = {
   {"__dir__", _PyCFunction_CAST(tb_dir), METH_NOARGS},
   {NULL, NULL, 0, NULL},
};

static PyMemberDef tb_memberlist[] = {
    {"tb_frame",        T_OBJECT,       OFF(tb_frame),  READONLY|PY_AUDIT_READ},
    {"tb_lasti",        T_INT,          OFF(tb_lasti),  READONLY},
    {"tb_lineno",       T_INT,          OFF(tb_lineno), READONLY},
    {NULL}      /* Sentinel */
};

static PyGetSetDef tb_getsetters[] = {
    {"tb_next", (getter)tb_next_get, (setter)tb_next_set, NULL, NULL},
    {NULL}      /* Sentinel */
};

static void
tb_dealloc(PyTracebackObject *tb)
{
    PyObject_GC_UnTrack(tb);
    Py_TRASHCAN_BEGIN(tb, tb_dealloc)
    Py_XDECREF(tb->tb_next);
    Py_XDECREF(tb->tb_frame);
    PyObject_GC_Del(tb);
    Py_TRASHCAN_END
}

static int
tb_traverse(PyTracebackObject *tb, visitproc visit, void *arg)
{
    Py_VISIT(tb->tb_next);
    Py_VISIT(tb->tb_frame);
    return 0;
}

static int
tb_clear(PyTracebackObject *tb)
{
    Py_CLEAR(tb->tb_next);
    Py_CLEAR(tb->tb_frame);
    return 0;
}

PyTypeObject PyTraceBack_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "traceback",
    sizeof(PyTracebackObject),
    0,
    (destructor)tb_dealloc, /*tp_dealloc*/
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
    (traverseproc)tb_traverse,                  /* tp_traverse */
    (inquiry)tb_clear,                          /* tp_clear */
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
    return tb_create_raw((PyTracebackObject *)tb_next, frame, addr,
                         PyFrame_GetLineNumber(frame));
}


int
PyTraceBack_Here(PyFrameObject *frame)
{
    PyObject *exc, *val, *tb, *newtb;
    PyErr_Fetch(&exc, &val, &tb);
    newtb = _PyTraceBack_FromFrame(tb, frame);
    if (newtb == NULL) {
        _PyErr_ChainExceptions(exc, val, tb);
        return -1;
    }
    PyErr_Restore(exc, val, newtb);
    Py_XDECREF(tb);
    return 0;
}

/* Insert a frame into the traceback for (funcname, filename, lineno). */
void _PyTraceback_Add(const char *funcname, const char *filename, int lineno)
{
    PyObject *globals;
    PyCodeObject *code;
    PyFrameObject *frame;
    PyObject *exc, *val, *tb;
    PyThreadState *tstate = _PyThreadState_GET();

    /* Save and clear the current exception. Python functions must not be
       called with an exception set. Calling Python functions happens when
       the codec of the filesystem encoding is implemented in pure Python. */
    _PyErr_Fetch(tstate, &exc, &val, &tb);

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

    _PyErr_Restore(tstate, exc, val, tb);
    PyTraceBack_Here(frame);
    Py_DECREF(frame);
    return;

error:
    _PyErr_ChainExceptions(exc, val, tb);
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
    syspath = _PySys_GetAttr(tstate, &_Py_ID(path));
    if (syspath == NULL || !PyList_Check(syspath))
        goto error;
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

/* Writes indent spaces, followed by the margin if it is not `\0`.
   Returns 0 on success and non-zero on failure.
 */
int
_Py_WriteIndentedMargin(int indent, const char *margin, PyObject *f)
{
    if (_Py_WriteIndent(indent, f) < 0) {
        return -1;
    }
    if (margin) {
        if (PyFile_WriteString(margin, f) < 0) {
            return -1;
        }
    }
    return 0;
}

static int
display_source_line_with_margin(PyObject *f, PyObject *filename, int lineno, int indent,
                                int margin_indent, const char *margin,
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
        Py_INCREF(lineobj);
        *line = lineobj;
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
            Py_DECREF(lineobj);
            lineobj = truncated;
        } else {
            PyErr_Clear();
        }
    }

    if (truncation != NULL) {
        *truncation = i - indent;
    }

    if (_Py_WriteIndentedMargin(margin_indent, margin, f) < 0) {
        goto error;
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
    return display_source_line_with_margin(f, filename, lineno, indent, 0,
                                           NULL, truncation, line);
}

/* AST based Traceback Specialization
 *
 * When displaying a new traceback line, for certain syntactical constructs
 * (e.g a subscript, an arithmetic operation) we try to create a representation
 * that separates the primary source of error from the rest.
 *
 * Example specialization of BinOp nodes:
 *  Traceback (most recent call last):
 *    File "/home/isidentical/cpython/cpython/t.py", line 10, in <module>
 *      add_values(1, 2, 'x', 3, 4)
 *    File "/home/isidentical/cpython/cpython/t.py", line 2, in add_values
 *      return a + b + c + d + e
 *             ~~~~~~^~~
 *  TypeError: 'NoneType' object is not subscriptable
 */

#define IS_WHITESPACE(c) (((c) == ' ') || ((c) == '\t') || ((c) == '\f'))

static int
extract_anchors_from_expr(const char *segment_str, expr_ty expr, Py_ssize_t *left_anchor, Py_ssize_t *right_anchor,
                          char** primary_error_char, char** secondary_error_char)
{
    switch (expr->kind) {
        case BinOp_kind: {
            expr_ty left = expr->v.BinOp.left;
            expr_ty right = expr->v.BinOp.right;
            for (int i = left->end_col_offset; i < right->col_offset; i++) {
                if (IS_WHITESPACE(segment_str[i])) {
                    continue;
                }

                *left_anchor = i;
                *right_anchor = i + 1;

                // Check whether if this a two-character operator (e.g //)
                if (i + 1 < right->col_offset && !IS_WHITESPACE(segment_str[i + 1])) {
                    ++*right_anchor;
                }

                // Set the error characters
                *primary_error_char = "~";
                *secondary_error_char = "^";
                break;
            }
            return 1;
        }
        case Subscript_kind: {
            *left_anchor = expr->v.Subscript.value->end_col_offset;
            *right_anchor = expr->v.Subscript.slice->end_col_offset + 1;

            // Set the error characters
            *primary_error_char = "~";
            *secondary_error_char = "^";
            return 1;
        }
        default:
            return 0;
    }
}

static int
extract_anchors_from_stmt(const char *segment_str, stmt_ty statement, Py_ssize_t *left_anchor, Py_ssize_t *right_anchor,
                          char** primary_error_char, char** secondary_error_char)
{
    switch (statement->kind) {
        case Expr_kind: {
            return extract_anchors_from_expr(segment_str, statement->v.Expr.value, left_anchor, right_anchor,
                                             primary_error_char, secondary_error_char);
        }
        default:
            return 0;
    }
}

static int
extract_anchors_from_line(PyObject *filename, PyObject *line,
                          Py_ssize_t start_offset, Py_ssize_t end_offset,
                          Py_ssize_t *left_anchor, Py_ssize_t *right_anchor,
                          char** primary_error_char, char** secondary_error_char)
{
    int res = -1;
    PyArena *arena = NULL;
    PyObject *segment = PyUnicode_Substring(line, start_offset, end_offset);
    if (!segment) {
        goto done;
    }

    const char *segment_str = PyUnicode_AsUTF8(segment);
    if (!segment_str) {
        goto done;
    }

    arena = _PyArena_New();
    if (!arena) {
        goto done;
    }

    PyCompilerFlags flags = _PyCompilerFlags_INIT;

    _PyASTOptimizeState state;
    state.optimize = _Py_GetConfig()->optimization_level;
    state.ff_features = 0;

    mod_ty module = _PyParser_ASTFromString(segment_str, filename, Py_file_input,
                                            &flags, arena);
    if (!module) {
        goto done;
    }
    if (!_PyAST_Optimize(module, arena, &state)) {
        goto done;
    }

    assert(module->kind == Module_kind);
    if (asdl_seq_LEN(module->v.Module.body) == 1) {
        stmt_ty statement = asdl_seq_GET(module->v.Module.body, 0);
        res = extract_anchors_from_stmt(segment_str, statement, left_anchor, right_anchor,
                                        primary_error_char, secondary_error_char);
    } else {
        res = 0;
    }

done:
    if (res > 0) {
        // Normalize the AST offsets to byte offsets and adjust them with the
        // start of the actual line (instead of the source code segment).
        assert(segment != NULL);
        assert(*left_anchor >= 0);
        assert(*right_anchor >= 0);
        *left_anchor = _PyPegen_byte_offset_to_character_offset(segment, *left_anchor) + start_offset;
        *right_anchor = _PyPegen_byte_offset_to_character_offset(segment, *right_anchor) + start_offset;
    }
    Py_XDECREF(segment);
    if (arena) {
        _PyArena_Free(arena);
    }
    return res;
}

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

static inline int
print_error_location_carets(PyObject *f, int offset, Py_ssize_t start_offset, Py_ssize_t end_offset,
                            Py_ssize_t right_start_offset, Py_ssize_t left_end_offset,
                            const char *primary, const char *secondary) {
    int special_chars = (left_end_offset != -1 || right_start_offset != -1);
    const char *str;
    while (++offset <= end_offset) {
        if (offset <= start_offset) {
            str = " ";
        } else if (special_chars && left_end_offset < offset && offset <= right_start_offset) {
            str = secondary;
        } else {
            str = primary;
        }
        if (PyFile_WriteString(str, f) < 0) {
            return -1;
        }
    }
    if (PyFile_WriteString("\n", f) < 0) {
        return -1;
    }
    return 0;
}

static int
tb_displayline(PyTracebackObject* tb, PyObject *f, PyObject *filename, int lineno,
               PyFrameObject *frame, PyObject *name, int margin_indent, const char *margin)
{
    if (filename == NULL || name == NULL) {
        return -1;
    }

    if (_Py_WriteIndentedMargin(margin_indent, margin, f) < 0) {
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
    int rc = display_source_line_with_margin(
            f, filename, lineno, _TRACEBACK_SOURCE_LINE_INDENT,
            margin_indent, margin, &truncation, &source_line);
    if (rc != 0 || !source_line) {
        /* ignore errors since we can't report them, can we? */
        err = ignore_source_errors();
        goto done;
    }

    int code_offset = tb->tb_lasti;
    PyCodeObject* code = frame->f_frame->f_code;
    const Py_ssize_t source_line_len = PyUnicode_GET_LENGTH(source_line);

    int start_line;
    int end_line;
    int start_col_byte_offset;
    int end_col_byte_offset;
    if (!PyCode_Addr2Location(code, code_offset, &start_line, &start_col_byte_offset,
                              &end_line, &end_col_byte_offset)) {
        goto done;
    }

    if (start_line < 0 || end_line < 0
        || start_col_byte_offset < 0
        || end_col_byte_offset < 0)
    {
        goto done;
    }

    // When displaying errors, we will use the following generic structure:
    //
    //  ERROR LINE ERROR LINE ERROR LINE ERROR LINE ERROR LINE ERROR LINE ERROR LINE
    //        ~~~~~~~~~~~~~~~^^^^^^^^^^^^^^^^^^^^^^^^^~~~~~~~~~~~~~~~~~~~
    //        |              |-> left_end_offset     |                  |-> end_offset
    //        |-> start_offset                       |-> right_start_offset
    //
    // In general we will only have (start_offset, end_offset) but we can gather more information
    // by analyzing the AST of the text between *start_offset* and *end_offset*. If this succeeds
    // we could get *left_end_offset* and *right_start_offset* and some selection of characters for
    // the different ranges (primary_error_char and secondary_error_char). If we cannot obtain the
    // AST information or we cannot identify special ranges within it, then left_end_offset and
    // right_end_offset will be set to -1.
    //
    // To keep the column indicators pertinent, they are not shown when the primary character
    // spans the whole line.

    // Convert the utf-8 byte offset to the actual character offset so we print the right number of carets.
    assert(source_line);
    Py_ssize_t start_offset = _PyPegen_byte_offset_to_character_offset(source_line, start_col_byte_offset);
    if (start_offset < 0) {
        err = ignore_source_errors() < 0;
        goto done;
    }

    Py_ssize_t end_offset = _PyPegen_byte_offset_to_character_offset(source_line, end_col_byte_offset);
    if (end_offset < 0) {
        err = ignore_source_errors() < 0;
        goto done;
    }

    Py_ssize_t left_end_offset = -1;
    Py_ssize_t right_start_offset = -1;

    char *primary_error_char = "^";
    char *secondary_error_char = primary_error_char;

    if (start_line == end_line) {
        int res = extract_anchors_from_line(filename, source_line, start_offset, end_offset,
                                            &left_end_offset, &right_start_offset,
                                            &primary_error_char, &secondary_error_char);
        if (res < 0 && ignore_source_errors() < 0) {
            goto done;
        }
    }
    else {
        // If this is a multi-line expression, then we will highlight until
        // the last non-whitespace character.
        const char *source_line_str = PyUnicode_AsUTF8(source_line);
        if (!source_line_str) {
            goto done;
        }

        Py_ssize_t i = source_line_len;
        while (--i >= 0) {
            if (!IS_WHITESPACE(source_line_str[i])) {
                break;
            }
        }

        end_offset = i + 1;
    }

    // Elide indicators if primary char spans the frame line
    Py_ssize_t stripped_line_len = source_line_len - truncation - _TRACEBACK_SOURCE_LINE_INDENT;
    bool has_secondary_ranges = (left_end_offset != -1 || right_start_offset != -1);
    if (end_offset - start_offset == stripped_line_len && !has_secondary_ranges) {
        goto done;
    }

    if (_Py_WriteIndentedMargin(margin_indent, margin, f) < 0) {
        err = -1;
        goto done;
    }

    if (print_error_location_carets(f, truncation, start_offset, end_offset,
                                    right_start_offset, left_end_offset,
                                    primary_error_char, secondary_error_char) < 0) {
        err = -1;
        goto done;
    }

done:
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
tb_printinternal(PyTracebackObject *tb, PyObject *f, long limit,
                 int indent, const char *margin)
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
        if (last_file == NULL ||
            code->co_filename != last_file ||
            last_line == -1 || tb->tb_lineno != last_line ||
            last_name == NULL || code->co_name != last_name) {
            if (cnt > TB_RECURSIVE_CUTOFF) {
                if (tb_print_line_repeated(f, cnt) < 0) {
                    goto error;
                }
            }
            last_file = code->co_filename;
            last_line = tb->tb_lineno;
            last_name = code->co_name;
            cnt = 0;
        }
        cnt++;
        if (cnt <= TB_RECURSIVE_CUTOFF) {
            if (tb_displayline(tb, f, code->co_filename, tb->tb_lineno,
                               tb->tb_frame, code->co_name, indent, margin) < 0) {
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
_PyTraceBack_Print_Indented(PyObject *v, int indent, const char *margin,
                            const char *header_margin, const char *header, PyObject *f)
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
    limitv = PySys_GetObject("tracebacklimit");
    if (limitv && PyLong_Check(limitv)) {
        int overflow;
        limit = PyLong_AsLongAndOverflow(limitv, &overflow);
        if (overflow > 0) {
            limit = LONG_MAX;
        }
        else if (limit <= 0) {
            return 0;
        }
    }
    if (_Py_WriteIndentedMargin(indent, header_margin, f) < 0) {
        return -1;
    }

    if (PyFile_WriteString(header, f) < 0) {
        return -1;
    }

    if (tb_printinternal((PyTracebackObject *)v, f, limit, indent, margin) < 0) {
        return -1;
    }

    return 0;
}

int
PyTraceBack_Print(PyObject *v, PyObject *f)
{
    int indent = 0;
    const char *margin = NULL;
    const char *header_margin = NULL;
    const char *header = EXCEPTION_TB_HEADER;

    return _PyTraceBack_Print_Indented(v, indent, margin, header_margin, header, f);
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

    _Py_write_noraise(fd, ptr, end - ptr);
}

/* Format an integer as hexadecimal with width digits into fd file descriptor.
   The function is signal safe. */
void
_Py_DumpHexadecimal(int fd, uintptr_t value, Py_ssize_t width)
{
    char buffer[sizeof(uintptr_t) * 2 + 1], *ptr, *end;
    const Py_ssize_t size = Py_ARRAY_LENGTH(buffer) - 1;

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

    _Py_write_noraise(fd, ptr, end - ptr);
}

void
_Py_DumpASCII(int fd, PyObject *text)
{
    PyASCIIObject *ascii = _PyASCIIObject_CAST(text);
    Py_ssize_t i, size;
    int truncated;
    int kind;
    void *data = NULL;
    wchar_t *wstr = NULL;
    Py_UCS4 ch;

    if (!PyUnicode_Check(text))
        return;

    size = ascii->length;
    kind = ascii->state.kind;
    if (kind == PyUnicode_WCHAR_KIND) {
        wstr = ascii->wstr;
        if (wstr == NULL)
            return;
        size = _PyCompactUnicodeObject_CAST(text)->wstr_length;
    }
    else if (ascii->state.compact) {
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
            _Py_write_noraise(fd, str, size);
            goto done;
        }
    }

    for (i=0; i < size; i++) {
        if (kind != PyUnicode_WCHAR_KIND)
            ch = PyUnicode_READ(kind, data, i);
        else
            ch = wstr[i];
        if (' ' <= ch && ch <= 126) {
            /* printable ASCII character */
            char c = (char)ch;
            _Py_write_noraise(fd, &c, 1);
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

/* Write a frame into the file fd: "File "xxx", line xxx in xxx".

   This function is signal safe. */

static void
dump_frame(int fd, _PyInterpreterFrame *frame)
{
    PyCodeObject *code = frame->f_code;
    PUTS(fd, "  File ");
    if (code->co_filename != NULL
        && PyUnicode_Check(code->co_filename))
    {
        PUTS(fd, "\"");
        _Py_DumpASCII(fd, code->co_filename);
        PUTS(fd, "\"");
    } else {
        PUTS(fd, "???");
    }

    int lineno = _PyInterpreterFrame_GetLine(frame);
    PUTS(fd, ", line ");
    if (lineno >= 0) {
        _Py_DumpDecimal(fd, (size_t)lineno);
    }
    else {
        PUTS(fd, "???");
    }
    PUTS(fd, " in ");

    if (code->co_name != NULL
       && PyUnicode_Check(code->co_name)) {
        _Py_DumpASCII(fd, code->co_name);
    }
    else {
        PUTS(fd, "???");
    }

    PUTS(fd, "\n");
}

static void
dump_traceback(int fd, PyThreadState *tstate, int write_header)
{
    _PyInterpreterFrame *frame;
    unsigned int depth;

    if (write_header) {
        PUTS(fd, "Stack (most recent call first):\n");
    }

    frame = tstate->cframe->current_frame;
    if (frame == NULL) {
        PUTS(fd, "  <no Python frame>\n");
        return;
    }

    depth = 0;
    while (1) {
        if (MAX_FRAME_DEPTH <= depth) {
            PUTS(fd, "  ...\n");
            break;
        }
        dump_frame(fd, frame);
        frame = frame->previous;
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

/* Write the thread identifier into the file 'fd': "Current thread 0xHHHH:\" if
   is_current is true, "Thread 0xHHHH:\n" otherwise.

   This function is signal safe. */

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
    PyThreadState *tstate;
    unsigned int nthreads;

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

    /* Get the current interpreter from the current thread */
    tstate = PyInterpreterState_ThreadHead(interp);
    if (tstate == NULL)
        return "unable to get the thread head state";

    /* Dump the traceback of each thread */
    tstate = PyInterpreterState_ThreadHead(interp);
    nthreads = 0;
    _Py_BEGIN_SUPPRESS_IPH
    do
    {
        if (nthreads != 0)
            PUTS(fd, "\n");
        if (nthreads >= MAX_NTHREADS) {
            PUTS(fd, "...\n");
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

