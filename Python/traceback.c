
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
        tb->tb_lineno = PyCode_Addr2Line(frame->f_code,
                                         frame->f_lasti);
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

static int
_Py_FindSourceFile(const char* filename, char* namebuf, size_t namelen, int open_flags)
{
    int i;
    int fd = -1;
    PyObject *v;
    Py_ssize_t _npath;
    int npath;
    size_t taillen;
    PyObject *syspath;
    const char* path;
    const char* tail;
    Py_ssize_t len;

    /* Search tail of filename in sys.path before giving up */
    tail = strrchr(filename, SEP);
    if (tail == NULL)
        tail = filename;
    else
        tail++;
    taillen = strlen(tail);

    syspath = PySys_GetObject("path");
    if (syspath == NULL || !PyList_Check(syspath))
        return -1;
    _npath = PyList_Size(syspath);
    npath = Py_SAFE_DOWNCAST(_npath, Py_ssize_t, int);

    for (i = 0; i < npath; i++) {
        v = PyList_GetItem(syspath, i);
        if (v == NULL) {
            PyErr_Clear();
            break;
        }
        if (!PyUnicode_Check(v))
            continue;
        path = _PyUnicode_AsStringAndSize(v, &len);
        if (len + 1 + (Py_ssize_t)taillen >= (Py_ssize_t)namelen - 1)
            continue; /* Too long */
        strcpy(namebuf, path);
        if (strlen(namebuf) != len)
            continue; /* v contains '\0' */
        if (len > 0 && namebuf[len-1] != SEP)
            namebuf[len++] = SEP;
        strcpy(namebuf+len, tail);
        Py_BEGIN_ALLOW_THREADS
        fd = open(namebuf, open_flags);
        Py_END_ALLOW_THREADS
        if (0 <= fd) {
            return fd;
        }
    }
    return -1;
}

int
_Py_DisplaySourceLine(PyObject *f, const char *filename, int lineno, int indent)
{
    int err = 0;
    int fd;
    int i;
    char *found_encoding;
    char *encoding;
    PyObject *fob = NULL;
    PyObject *lineobj = NULL;
#ifdef O_BINARY
    const int open_flags = O_RDONLY | O_BINARY;   /* necessary for Windows */
#else
    const int open_flags = O_RDONLY;
#endif
    char buf[MAXPATHLEN+1];
    Py_UNICODE *u, *p;
    Py_ssize_t len;

    /* open the file */
    if (filename == NULL)
        return 0;
    Py_BEGIN_ALLOW_THREADS
    fd = open(filename, open_flags);
    Py_END_ALLOW_THREADS
    if (fd < 0) {
        fd = _Py_FindSourceFile(filename, buf, sizeof(buf), open_flags);
        if (fd < 0)
            return 0;
        filename = buf;
    }

    /* use the right encoding to decode the file as unicode */
    found_encoding = PyTokenizer_FindEncoding(fd);
    encoding = (found_encoding != NULL) ? found_encoding :
        (char*)PyUnicode_GetDefaultEncoding();
    lseek(fd, 0, 0); /* Reset position */
    fob = PyFile_FromFd(fd, (char*)filename, "r", -1, (char*)encoding,
        NULL, NULL, 1);
    PyMem_FREE(found_encoding);
    if (fob == NULL) {
        PyErr_Clear();
        close(fd);
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
tb_displayline(PyObject *f, const char *filename, int lineno, const char *name)
{
    int err = 0;
    char linebuf[2000];

    if (filename == NULL || name == NULL)
        return -1;
    /* This is needed by Emacs' compile command */
#define FMT "  File \"%.500s\", line %d, in %.500s\n"
    PyOS_snprintf(linebuf, sizeof(linebuf), FMT, filename, lineno, name);
    err = PyFile_WriteString(linebuf, f);
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
                _PyUnicode_AsString(
                    tb->tb_frame->f_code->co_filename),
                tb->tb_lineno,
                _PyUnicode_AsString(tb->tb_frame->f_code->co_name));
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
