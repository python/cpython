
/* Traceback implementation */

#include "Python.h"

#include "code.h"
#include "frameobject.h"
#include "structmember.h"
#include "osdefs.h"
#include "traceback.h"
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#define OFF(x) offsetof(PyTracebackObject, x)

/* Method from Parser/tokenizer.c */
extern char * PyTokenizer_FindEncoding(int);

static PyObject *
tb_dir(PyTracebackObject *self)
{
    return Py_BuildValue("[ssss]", "tb_frame", "tb_next",
                                   "tb_lasti", "tb_lineno");
}

static PyMethodDef tb_methods[] = {
   {"__dir__", (PyCFunction)tb_dir, METH_NOARGS},
   {NULL, NULL, 0, NULL},
};

static PyMemberDef tb_memberlist[] = {
    {"tb_next",         T_OBJECT,       OFF(tb_next),   READONLY},
    {"tb_frame",        T_OBJECT,       OFF(tb_frame),  READONLY},
    {"tb_lasti",        T_INT,          OFF(tb_lasti),  READONLY},
    {"tb_lineno",       T_INT,          OFF(tb_lineno), READONLY},
    {NULL}      /* Sentinel */
};

static void
tb_dealloc(PyTracebackObject *tb)
{
    PyObject_GC_UnTrack(tb);
    Py_TRASHCAN_SAFE_BEGIN(tb)
    Py_XDECREF(tb->tb_next);
    Py_XDECREF(tb->tb_frame);
    PyObject_GC_Del(tb);
    Py_TRASHCAN_SAFE_END(tb)
}

static int
tb_traverse(PyTracebackObject *tb, visitproc visit, void *arg)
{
    Py_VISIT(tb->tb_next);
    Py_VISIT(tb->tb_frame);
    return 0;
}

static void
tb_clear(PyTracebackObject *tb)
{
    Py_CLEAR(tb->tb_next);
    Py_CLEAR(tb->tb_frame);
}

PyTypeObject PyTraceBack_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "traceback",
    sizeof(PyTracebackObject),
    0,
    (destructor)tb_dealloc, /*tp_dealloc*/
    0,                  /*tp_print*/
    0,    /*tp_getattr*/
    0,                  /*tp_setattr*/
    0,                  /*tp_reserved*/
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
    0,                                          /* tp_doc */
    (traverseproc)tb_traverse,                  /* tp_traverse */
    (inquiry)tb_clear,                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    tb_methods,         /* tp_methods */
    tb_memberlist,      /* tp_members */
    0,                                          /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
};

static PyTracebackObject *
newtracebackobject(PyTracebackObject *next, PyFrameObject *frame)
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
        tb->tb_lasti = frame->f_lasti;
        tb->tb_lineno = PyFrame_GetLineNumber(frame);
        PyObject_GC_Track(tb);
    }
    return tb;
}

int
PyTraceBack_Here(PyFrameObject *frame)
{
    PyThreadState *tstate = PyThreadState_GET();
    PyTracebackObject *oldtb = (PyTracebackObject *) tstate->curexc_traceback;
    PyTracebackObject *tb = newtracebackobject(oldtb, frame);
    if (tb == NULL)
        return -1;
    tstate->curexc_traceback = (PyObject *)tb;
    Py_XDECREF(oldtb);
    return 0;
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

    syspath = PySys_GetObject("path");
    if (syspath == NULL || !PyList_Check(syspath))
        goto error;
    npath = PyList_Size(syspath);

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
        if (strlen(namebuf) != len)
            continue; /* v contains '\0' */
        if (len > 0 && namebuf[len-1] != SEP)
            namebuf[len++] = SEP;
        strcpy(namebuf+len, tail);

        binary = PyObject_CallMethod(io, "open", "ss", namebuf, "rb");
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
    Py_DECREF(filebytes);
    return result;
}

int
_Py_DisplaySourceLine(PyObject *f, PyObject *filename, int lineno, int indent)
{
    int err = 0;
    int fd;
    int i;
    char *found_encoding;
    char *encoding;
    PyObject *io;
    PyObject *binary;
    PyObject *fob = NULL;
    PyObject *lineobj = NULL;
    PyObject *res;
    char buf[MAXPATHLEN+1];
    Py_UNICODE *u, *p;
    Py_ssize_t len;

    /* open the file */
    if (filename == NULL)
        return 0;

    io = PyImport_ImportModuleNoBlock("io");
    if (io == NULL)
        return -1;
    binary = PyObject_CallMethod(io, "open", "Os", filename, "rb");

    if (binary == NULL) {
        binary = _Py_FindSourceFile(filename, buf, sizeof(buf), io);
        if (binary == NULL) {
            Py_DECREF(io);
            return 0;
        }
    }

    /* use the right encoding to decode the file as unicode */
    fd = PyObject_AsFileDescriptor(binary);
    found_encoding = PyTokenizer_FindEncoding(fd);
    encoding = (found_encoding != NULL) ? found_encoding : "utf-8";
    lseek(fd, 0, 0); /* Reset position */
    fob = PyObject_CallMethod(io, "TextIOWrapper", "Os", binary, encoding);
    Py_DECREF(io);
    Py_DECREF(binary);
    PyMem_FREE(found_encoding);

    if (fob == NULL) {
        PyErr_Clear();
        return 0;
    }

    /* get the line number lineno */
    for (i = 0; i < lineno; i++) {
        Py_XDECREF(lineobj);
        lineobj = PyFile_GetLine(fob, -1);
        if (!lineobj) {
            err = -1;
            break;
        }
    }
    res = PyObject_CallMethod(fob, "close", "");
    if (res)
        Py_DECREF(res);
    else
        PyErr_Clear();
    Py_DECREF(fob);
    if (!lineobj || !PyUnicode_Check(lineobj)) {
        Py_XDECREF(lineobj);
        return err;
    }

    /* remove the indentation of the line */
    u = PyUnicode_AS_UNICODE(lineobj);
    len = PyUnicode_GET_SIZE(lineobj);
    for (p=u; *p == ' ' || *p == '\t' || *p == '\014'; p++)
        len--;
    if (u != p) {
        PyObject *truncated;
        truncated = PyUnicode_FromUnicode(p, len);
        if (truncated) {
            Py_DECREF(lineobj);
            lineobj = truncated;
        } else {
            PyErr_Clear();
        }
    }

    /* Write some spaces before the line */
    strcpy(buf, "          ");
    assert (strlen(buf) == 10);
    while (indent > 0) {
        if(indent < 10)
            buf[indent] = '\0';
        err = PyFile_WriteString(buf, f);
        if (err != 0)
            break;
        indent -= 10;
    }

    /* finally display the line */
    if (err == 0)
        err = PyFile_WriteObject(lineobj, f, Py_PRINT_RAW);
    Py_DECREF(lineobj);
    if  (err == 0)
        err = PyFile_WriteString("\n", f);
    return err;
}

static int
tb_displayline(PyObject *f, PyObject *filename, int lineno, PyObject *name)
{
    int err;
    PyObject *line;

    if (filename == NULL || name == NULL)
        return -1;
    line = PyUnicode_FromFormat("  File \"%U\", line %d, in %U\n",
                                filename, lineno, name);
    if (line == NULL)
        return -1;
    err = PyFile_WriteObject(line, f, Py_PRINT_RAW);
    Py_DECREF(line);
    if (err != 0)
        return err;
    return _Py_DisplaySourceLine(f, filename, lineno, 4);
}

static int
tb_printinternal(PyTracebackObject *tb, PyObject *f, long limit)
{
    int err = 0;
    long depth = 0;
    PyTracebackObject *tb1 = tb;
    while (tb1 != NULL) {
        depth++;
        tb1 = tb1->tb_next;
    }
    while (tb != NULL && err == 0) {
        if (depth <= limit) {
            err = tb_displayline(f,
                                 tb->tb_frame->f_code->co_filename,
                                 tb->tb_lineno,
                                 tb->tb_frame->f_code->co_name);
        }
        depth--;
        tb = tb->tb_next;
        if (err == 0)
            err = PyErr_CheckSignals();
    }
    return err;
}

#define PyTraceBack_LIMIT 1000

int
PyTraceBack_Print(PyObject *v, PyObject *f)
{
    int err;
    PyObject *limitv;
    long limit = PyTraceBack_LIMIT;

    if (v == NULL)
        return 0;
    if (!PyTraceBack_Check(v)) {
        PyErr_BadInternalCall();
        return -1;
    }
    limitv = PySys_GetObject("tracebacklimit");
    if (limitv) {
        PyObject *exc_type, *exc_value, *exc_tb;

        PyErr_Fetch(&exc_type, &exc_value, &exc_tb);
        limit = PyLong_AsLong(limitv);
        if (limit == -1 && PyErr_Occurred()) {
            if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
                limit = PyTraceBack_LIMIT;
            }
            else {
                Py_XDECREF(exc_type);
                Py_XDECREF(exc_value);
                Py_XDECREF(exc_tb);
                return 0;
            }
        }
        else if (limit <= 0) {
            limit = PyTraceBack_LIMIT;
        }
        PyErr_Restore(exc_type, exc_value, exc_tb);
    }
    err = PyFile_WriteString("Traceback (most recent call last):\n", f);
    if (!err)
        err = tb_printinternal((PyTracebackObject *)v, f, limit);
    return err;
}
