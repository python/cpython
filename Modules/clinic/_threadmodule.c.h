/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

#if (defined(HAVE_PTHREAD_GETNAME_NP) || defined(HAVE_PTHREAD_GET_NAME_NP) || defined(MS_WINDOWS))

PyDoc_STRVAR(_thread__get_name__doc__,
"_get_name($module, /)\n"
"--\n"
"\n"
"Get the name of the current thread.");

#define _THREAD__GET_NAME_METHODDEF    \
    {"_get_name", (PyCFunction)_thread__get_name, METH_NOARGS, _thread__get_name__doc__},

static PyObject *
_thread__get_name_impl(PyObject *module);

static PyObject *
_thread__get_name(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _thread__get_name_impl(module);
}

#endif /* (defined(HAVE_PTHREAD_GETNAME_NP) || defined(HAVE_PTHREAD_GET_NAME_NP) || defined(MS_WINDOWS)) */

#if (defined(HAVE_PTHREAD_SETNAME_NP) || defined(HAVE_PTHREAD_SET_NAME_NP) || defined(MS_WINDOWS))

PyDoc_STRVAR(_thread_set_name__doc__,
"set_name($module, /, name)\n"
"--\n"
"\n"
"Set the name of the current thread.");

#define _THREAD_SET_NAME_METHODDEF    \
    {"set_name", _PyCFunction_CAST(_thread_set_name), METH_FASTCALL|METH_KEYWORDS, _thread_set_name__doc__},

static PyObject *
_thread_set_name_impl(PyObject *module, PyObject *name_obj);

static PyObject *
_thread_set_name(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(name), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"name", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "set_name",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *name_obj;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("set_name", "argument 'name'", "str", args[0]);
        goto exit;
    }
    name_obj = args[0];
    return_value = _thread_set_name_impl(module, name_obj);

exit:
    return return_value;
}

#endif /* (defined(HAVE_PTHREAD_SETNAME_NP) || defined(HAVE_PTHREAD_SET_NAME_NP) || defined(MS_WINDOWS)) */

#ifndef _THREAD__GET_NAME_METHODDEF
    #define _THREAD__GET_NAME_METHODDEF
#endif /* !defined(_THREAD__GET_NAME_METHODDEF) */

#ifndef _THREAD_SET_NAME_METHODDEF
    #define _THREAD_SET_NAME_METHODDEF
#endif /* !defined(_THREAD_SET_NAME_METHODDEF) */
/*[clinic end generated code: output=e978dc4615b9bc35 input=a9049054013a1b77]*/
