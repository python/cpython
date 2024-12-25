/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()

PyDoc_STRVAR(gen_close_meth__doc__,
"close($self, /)\n"
"--\n"
"\n"
"raise GeneratorExit inside generator.");

#define GEN_CLOSE_METH_METHODDEF    \
    {"close", (PyCFunction)gen_close_meth, METH_NOARGS, gen_close_meth__doc__},

static PyObject *
gen_close_meth_impl(PyGenObject *self);

static PyObject *
gen_close_meth(PyGenObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = gen_close_meth_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(gen_getrunning_DOCSTR)
#  define gen_getrunning_DOCSTR NULL
#endif
#if defined(GEN_GETRUNNING_GETSETDEF)
#  undef GEN_GETRUNNING_GETSETDEF
#  define GEN_GETRUNNING_GETSETDEF {"gi_running", (getter)gen_getrunning_get, (setter)gen_getrunning_set, gen_getrunning_DOCSTR},
#else
#  define GEN_GETRUNNING_GETSETDEF {"gi_running", (getter)gen_getrunning_get, NULL, gen_getrunning_DOCSTR},
#endif

static PyObject *
gen_getrunning_get_impl(PyGenObject *self);

static PyObject *
gen_getrunning_get(PyGenObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = gen_getrunning_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(gen_getsuspended_DOCSTR)
#  define gen_getsuspended_DOCSTR NULL
#endif
#if defined(GEN_GETSUSPENDED_GETSETDEF)
#  undef GEN_GETSUSPENDED_GETSETDEF
#  define GEN_GETSUSPENDED_GETSETDEF {"gi_suspended", (getter)gen_getsuspended_get, (setter)gen_getsuspended_set, gen_getsuspended_DOCSTR},
#else
#  define GEN_GETSUSPENDED_GETSETDEF {"gi_suspended", (getter)gen_getsuspended_get, NULL, gen_getsuspended_DOCSTR},
#endif

static PyObject *
gen_getsuspended_get_impl(PyGenObject *self);

static PyObject *
gen_getsuspended_get(PyGenObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = gen_getsuspended_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=e949b15b48a76555 input=a9049054013a1b77]*/
