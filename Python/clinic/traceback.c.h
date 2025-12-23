/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(tb_new__doc__,
"traceback(tb_next, tb_frame, tb_lasti, tb_lineno)\n"
"--\n"
"\n"
"Create a new traceback object.");

static PyObject *
tb_new_impl(PyTypeObject *type, PyObject *tb_next, PyFrameObject *tb_frame,
            int tb_lasti, int tb_lineno);

static PyObject *
tb_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(tb_next), &_Py_ID(tb_frame), &_Py_ID(tb_lasti), &_Py_ID(tb_lineno), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"tb_next", "tb_frame", "tb_lasti", "tb_lineno", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "traceback",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    PyObject *tb_next;
    PyFrameObject *tb_frame;
    int tb_lasti;
    int tb_lineno;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 4, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    tb_next = fastargs[0];
    if (!PyObject_TypeCheck(fastargs[1], &PyFrame_Type)) {
        _PyArg_BadArgument("traceback", "argument 'tb_frame'", (&PyFrame_Type)->tp_name, fastargs[1]);
        goto exit;
    }
    tb_frame = (PyFrameObject *)fastargs[1];
    tb_lasti = PyLong_AsInt(fastargs[2]);
    if (tb_lasti == -1 && PyErr_Occurred()) {
        goto exit;
    }
    tb_lineno = PyLong_AsInt(fastargs[3]);
    if (tb_lineno == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = tb_new_impl(type, tb_next, tb_frame, tb_lasti, tb_lineno);

exit:
    return return_value;
}

#if !defined(traceback_tb_next_DOCSTR)
#  define traceback_tb_next_DOCSTR NULL
#endif
#if defined(TRACEBACK_TB_NEXT_GETSETDEF)
#  undef TRACEBACK_TB_NEXT_GETSETDEF
#  define TRACEBACK_TB_NEXT_GETSETDEF {"tb_next", (getter)traceback_tb_next_get, (setter)traceback_tb_next_set, traceback_tb_next_DOCSTR},
#else
#  define TRACEBACK_TB_NEXT_GETSETDEF {"tb_next", (getter)traceback_tb_next_get, NULL, traceback_tb_next_DOCSTR},
#endif

static PyObject *
traceback_tb_next_get_impl(PyTracebackObject *self);

static PyObject *
traceback_tb_next_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = traceback_tb_next_get_impl((PyTracebackObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(traceback_tb_next_DOCSTR)
#  define traceback_tb_next_DOCSTR NULL
#endif
#if defined(TRACEBACK_TB_NEXT_GETSETDEF)
#  undef TRACEBACK_TB_NEXT_GETSETDEF
#  define TRACEBACK_TB_NEXT_GETSETDEF {"tb_next", (getter)traceback_tb_next_get, (setter)traceback_tb_next_set, traceback_tb_next_DOCSTR},
#else
#  define TRACEBACK_TB_NEXT_GETSETDEF {"tb_next", NULL, (setter)traceback_tb_next_set, NULL},
#endif

static int
traceback_tb_next_set_impl(PyTracebackObject *self, PyObject *value);

static int
traceback_tb_next_set(PyObject *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = traceback_tb_next_set_impl((PyTracebackObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=5361141395da963e input=a9049054013a1b77]*/
