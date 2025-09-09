/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_BadArgument()

PyDoc_STRVAR(BaseException___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define BASEEXCEPTION___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)BaseException___reduce__, METH_NOARGS, BaseException___reduce____doc__},

static PyObject *
BaseException___reduce___impl(PyBaseExceptionObject *self);

static PyObject *
BaseException___reduce__(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___reduce___impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(BaseException___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n");

#define BASEEXCEPTION___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)BaseException___setstate__, METH_O, BaseException___setstate____doc__},

static PyObject *
BaseException___setstate___impl(PyBaseExceptionObject *self, PyObject *state);

static PyObject *
BaseException___setstate__(PyObject *self, PyObject *state)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___setstate___impl((PyBaseExceptionObject *)self, state);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(BaseException_with_traceback__doc__,
"with_traceback($self, tb, /)\n"
"--\n"
"\n"
"Set self.__traceback__ to tb and return self.");

#define BASEEXCEPTION_WITH_TRACEBACK_METHODDEF    \
    {"with_traceback", (PyCFunction)BaseException_with_traceback, METH_O, BaseException_with_traceback__doc__},

static PyObject *
BaseException_with_traceback_impl(PyBaseExceptionObject *self, PyObject *tb);

static PyObject *
BaseException_with_traceback(PyObject *self, PyObject *tb)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException_with_traceback_impl((PyBaseExceptionObject *)self, tb);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(BaseException_add_note__doc__,
"add_note($self, note, /)\n"
"--\n"
"\n"
"Add a note to the exception");

#define BASEEXCEPTION_ADD_NOTE_METHODDEF    \
    {"add_note", (PyCFunction)BaseException_add_note, METH_O, BaseException_add_note__doc__},

static PyObject *
BaseException_add_note_impl(PyBaseExceptionObject *self, PyObject *note);

static PyObject *
BaseException_add_note(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *note;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("add_note", "argument", "str", arg);
        goto exit;
    }
    note = arg;
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException_add_note_impl((PyBaseExceptionObject *)self, note);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

#if !defined(BaseException_args_DOCSTR)
#  define BaseException_args_DOCSTR NULL
#endif
#if defined(BASEEXCEPTION_ARGS_GETSETDEF)
#  undef BASEEXCEPTION_ARGS_GETSETDEF
#  define BASEEXCEPTION_ARGS_GETSETDEF {"args", (getter)BaseException_args_get, (setter)BaseException_args_set, BaseException_args_DOCSTR},
#else
#  define BASEEXCEPTION_ARGS_GETSETDEF {"args", (getter)BaseException_args_get, NULL, BaseException_args_DOCSTR},
#endif

static PyObject *
BaseException_args_get_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException_args_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException_args_get_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(BaseException_args_DOCSTR)
#  define BaseException_args_DOCSTR NULL
#endif
#if defined(BASEEXCEPTION_ARGS_GETSETDEF)
#  undef BASEEXCEPTION_ARGS_GETSETDEF
#  define BASEEXCEPTION_ARGS_GETSETDEF {"args", (getter)BaseException_args_get, (setter)BaseException_args_set, BaseException_args_DOCSTR},
#else
#  define BASEEXCEPTION_ARGS_GETSETDEF {"args", NULL, (setter)BaseException_args_set, NULL},
#endif

static int
BaseException_args_set_impl(PyBaseExceptionObject *self, PyObject *value);

static int
BaseException_args_set(PyObject *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException_args_set_impl((PyBaseExceptionObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(BaseException___traceback___DOCSTR)
#  define BaseException___traceback___DOCSTR NULL
#endif
#if defined(BASEEXCEPTION___TRACEBACK___GETSETDEF)
#  undef BASEEXCEPTION___TRACEBACK___GETSETDEF
#  define BASEEXCEPTION___TRACEBACK___GETSETDEF {"__traceback__", (getter)BaseException___traceback___get, (setter)BaseException___traceback___set, BaseException___traceback___DOCSTR},
#else
#  define BASEEXCEPTION___TRACEBACK___GETSETDEF {"__traceback__", (getter)BaseException___traceback___get, NULL, BaseException___traceback___DOCSTR},
#endif

static PyObject *
BaseException___traceback___get_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException___traceback___get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___traceback___get_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(BaseException___traceback___DOCSTR)
#  define BaseException___traceback___DOCSTR NULL
#endif
#if defined(BASEEXCEPTION___TRACEBACK___GETSETDEF)
#  undef BASEEXCEPTION___TRACEBACK___GETSETDEF
#  define BASEEXCEPTION___TRACEBACK___GETSETDEF {"__traceback__", (getter)BaseException___traceback___get, (setter)BaseException___traceback___set, BaseException___traceback___DOCSTR},
#else
#  define BASEEXCEPTION___TRACEBACK___GETSETDEF {"__traceback__", NULL, (setter)BaseException___traceback___set, NULL},
#endif

static int
BaseException___traceback___set_impl(PyBaseExceptionObject *self,
                                     PyObject *value);

static int
BaseException___traceback___set(PyObject *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___traceback___set_impl((PyBaseExceptionObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(BaseException___context___DOCSTR)
#  define BaseException___context___DOCSTR NULL
#endif
#if defined(BASEEXCEPTION___CONTEXT___GETSETDEF)
#  undef BASEEXCEPTION___CONTEXT___GETSETDEF
#  define BASEEXCEPTION___CONTEXT___GETSETDEF {"__context__", (getter)BaseException___context___get, (setter)BaseException___context___set, BaseException___context___DOCSTR},
#else
#  define BASEEXCEPTION___CONTEXT___GETSETDEF {"__context__", (getter)BaseException___context___get, NULL, BaseException___context___DOCSTR},
#endif

static PyObject *
BaseException___context___get_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException___context___get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___context___get_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(BaseException___context___DOCSTR)
#  define BaseException___context___DOCSTR NULL
#endif
#if defined(BASEEXCEPTION___CONTEXT___GETSETDEF)
#  undef BASEEXCEPTION___CONTEXT___GETSETDEF
#  define BASEEXCEPTION___CONTEXT___GETSETDEF {"__context__", (getter)BaseException___context___get, (setter)BaseException___context___set, BaseException___context___DOCSTR},
#else
#  define BASEEXCEPTION___CONTEXT___GETSETDEF {"__context__", NULL, (setter)BaseException___context___set, NULL},
#endif

static int
BaseException___context___set_impl(PyBaseExceptionObject *self,
                                   PyObject *value);

static int
BaseException___context___set(PyObject *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___context___set_impl((PyBaseExceptionObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(BaseException___cause___DOCSTR)
#  define BaseException___cause___DOCSTR NULL
#endif
#if defined(BASEEXCEPTION___CAUSE___GETSETDEF)
#  undef BASEEXCEPTION___CAUSE___GETSETDEF
#  define BASEEXCEPTION___CAUSE___GETSETDEF {"__cause__", (getter)BaseException___cause___get, (setter)BaseException___cause___set, BaseException___cause___DOCSTR},
#else
#  define BASEEXCEPTION___CAUSE___GETSETDEF {"__cause__", (getter)BaseException___cause___get, NULL, BaseException___cause___DOCSTR},
#endif

static PyObject *
BaseException___cause___get_impl(PyBaseExceptionObject *self);

static PyObject *
BaseException___cause___get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___cause___get_impl((PyBaseExceptionObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

#if !defined(BaseException___cause___DOCSTR)
#  define BaseException___cause___DOCSTR NULL
#endif
#if defined(BASEEXCEPTION___CAUSE___GETSETDEF)
#  undef BASEEXCEPTION___CAUSE___GETSETDEF
#  define BASEEXCEPTION___CAUSE___GETSETDEF {"__cause__", (getter)BaseException___cause___get, (setter)BaseException___cause___set, BaseException___cause___DOCSTR},
#else
#  define BASEEXCEPTION___CAUSE___GETSETDEF {"__cause__", NULL, (setter)BaseException___cause___set, NULL},
#endif

static int
BaseException___cause___set_impl(PyBaseExceptionObject *self,
                                 PyObject *value);

static int
BaseException___cause___set(PyObject *self, PyObject *value, void *Py_UNUSED(context))
{
    int return_value;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseException___cause___set_impl((PyBaseExceptionObject *)self, value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(BaseExceptionGroup_derive__doc__,
"derive($self, excs, /)\n"
"--\n"
"\n");

#define BASEEXCEPTIONGROUP_DERIVE_METHODDEF    \
    {"derive", (PyCFunction)BaseExceptionGroup_derive, METH_O, BaseExceptionGroup_derive__doc__},

static PyObject *
BaseExceptionGroup_derive_impl(PyBaseExceptionGroupObject *self,
                               PyObject *excs);

static PyObject *
BaseExceptionGroup_derive(PyObject *self, PyObject *excs)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseExceptionGroup_derive_impl((PyBaseExceptionGroupObject *)self, excs);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(BaseExceptionGroup_split__doc__,
"split($self, matcher_value, /)\n"
"--\n"
"\n");

#define BASEEXCEPTIONGROUP_SPLIT_METHODDEF    \
    {"split", (PyCFunction)BaseExceptionGroup_split, METH_O, BaseExceptionGroup_split__doc__},

static PyObject *
BaseExceptionGroup_split_impl(PyBaseExceptionGroupObject *self,
                              PyObject *matcher_value);

static PyObject *
BaseExceptionGroup_split(PyObject *self, PyObject *matcher_value)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseExceptionGroup_split_impl((PyBaseExceptionGroupObject *)self, matcher_value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(BaseExceptionGroup_subgroup__doc__,
"subgroup($self, matcher_value, /)\n"
"--\n"
"\n");

#define BASEEXCEPTIONGROUP_SUBGROUP_METHODDEF    \
    {"subgroup", (PyCFunction)BaseExceptionGroup_subgroup, METH_O, BaseExceptionGroup_subgroup__doc__},

static PyObject *
BaseExceptionGroup_subgroup_impl(PyBaseExceptionGroupObject *self,
                                 PyObject *matcher_value);

static PyObject *
BaseExceptionGroup_subgroup(PyObject *self, PyObject *matcher_value)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = BaseExceptionGroup_subgroup_impl((PyBaseExceptionGroupObject *)self, matcher_value);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=fcf70b3b71f3d14a input=a9049054013a1b77]*/
