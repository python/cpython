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

#if !defined(gen_getframe_DOCSTR)
#  define gen_getframe_DOCSTR NULL
#endif
#if defined(GEN_GETFRAME_GETSETDEF)
#  undef GEN_GETFRAME_GETSETDEF
#  define GEN_GETFRAME_GETSETDEF {"gi_frame", (getter)gen_getframe_get, (setter)gen_getframe_set, gen_getframe_DOCSTR},
#else
#  define GEN_GETFRAME_GETSETDEF {"gi_frame", (getter)gen_getframe_get, NULL, gen_getframe_DOCSTR},
#endif

static PyObject *
gen_getframe_get_impl(PyGenObject *self);

static PyObject *
gen_getframe_get(PyGenObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = gen_getframe_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(cr_getsuspended_DOCSTR)
#  define cr_getsuspended_DOCSTR NULL
#endif
#if defined(CR_GETSUSPENDED_GETSETDEF)
#  undef CR_GETSUSPENDED_GETSETDEF
#  define CR_GETSUSPENDED_GETSETDEF {"cr_suspended", (getter)cr_getsuspended_get, (setter)cr_getsuspended_set, cr_getsuspended_DOCSTR},
#else
#  define CR_GETSUSPENDED_GETSETDEF {"cr_suspended", (getter)cr_getsuspended_get, NULL, cr_getsuspended_DOCSTR},
#endif

static PyObject *
cr_getsuspended_get_impl(PyCoroObject *self);

static PyObject *
cr_getsuspended_get(PyCoroObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = cr_getsuspended_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(cr_getrunning_DOCSTR)
#  define cr_getrunning_DOCSTR NULL
#endif
#if defined(CR_GETRUNNING_GETSETDEF)
#  undef CR_GETRUNNING_GETSETDEF
#  define CR_GETRUNNING_GETSETDEF {"cr_running", (getter)cr_getrunning_get, (setter)cr_getrunning_set, cr_getrunning_DOCSTR},
#else
#  define CR_GETRUNNING_GETSETDEF {"cr_running", (getter)cr_getrunning_get, NULL, cr_getrunning_DOCSTR},
#endif

static PyObject *
cr_getrunning_get_impl(PyCoroObject *self);

static PyObject *
cr_getrunning_get(PyCoroObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = cr_getrunning_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(cr_getframe_DOCSTR)
#  define cr_getframe_DOCSTR NULL
#endif
#if defined(CR_GETFRAME_GETSETDEF)
#  undef CR_GETFRAME_GETSETDEF
#  define CR_GETFRAME_GETSETDEF {"cr_frame", (getter)cr_getframe_get, (setter)cr_getframe_set, cr_getframe_DOCSTR},
#else
#  define CR_GETFRAME_GETSETDEF {"cr_frame", (getter)cr_getframe_get, NULL, cr_getframe_DOCSTR},
#endif

static PyObject *
cr_getframe_get_impl(PyCoroObject *self);

static PyObject *
cr_getframe_get(PyCoroObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = cr_getframe_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(ag_getframe_DOCSTR)
#  define ag_getframe_DOCSTR NULL
#endif
#if defined(AG_GETFRAME_GETSETDEF)
#  undef AG_GETFRAME_GETSETDEF
#  define AG_GETFRAME_GETSETDEF {"ag_frame", (getter)ag_getframe_get, (setter)ag_getframe_set, ag_getframe_DOCSTR},
#else
#  define AG_GETFRAME_GETSETDEF {"ag_frame", (getter)ag_getframe_get, NULL, ag_getframe_DOCSTR},
#endif

static PyObject *
ag_getframe_get_impl(PyAsyncGenObject *self);

static PyObject *
ag_getframe_get(PyAsyncGenObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = ag_getframe_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(ag_getsuspended_DOCSTR)
#  define ag_getsuspended_DOCSTR NULL
#endif
#if defined(AG_GETSUSPENDED_GETSETDEF)
#  undef AG_GETSUSPENDED_GETSETDEF
#  define AG_GETSUSPENDED_GETSETDEF {"ag_suspended", (getter)ag_getsuspended_get, (setter)ag_getsuspended_set, ag_getsuspended_DOCSTR},
#else
#  define AG_GETSUSPENDED_GETSETDEF {"ag_suspended", (getter)ag_getsuspended_get, NULL, ag_getsuspended_DOCSTR},
#endif

static PyObject *
ag_getsuspended_get_impl(PyAsyncGenObject *self);

static PyObject *
ag_getsuspended_get(PyAsyncGenObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = ag_getsuspended_get_impl(self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=fb2d54f4bfdabe4f input=a9049054013a1b77]*/
