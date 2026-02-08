/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()

PyDoc_STRVAR(range_iterator___length_hint____doc__,
"__length_hint__($self, /)\n"
"--\n"
"\n"
"Private method returning an estimate of len(list(it)).");

#define RANGE_ITERATOR___LENGTH_HINT___METHODDEF    \
    {"__length_hint__", (PyCFunction)range_iterator___length_hint__, METH_NOARGS, range_iterator___length_hint____doc__},

static PyObject *
range_iterator___length_hint___impl(_PyRangeIterObject *r);

static PyObject *
range_iterator___length_hint__(PyObject *r, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(r);
    return_value = range_iterator___length_hint___impl((_PyRangeIterObject *)r);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(range_iterator___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define RANGE_ITERATOR___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)range_iterator___reduce__, METH_NOARGS, range_iterator___reduce____doc__},

static PyObject *
range_iterator___reduce___impl(_PyRangeIterObject *r);

static PyObject *
range_iterator___reduce__(PyObject *r, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(r);
    return_value = range_iterator___reduce___impl((_PyRangeIterObject *)r);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(range_iterator___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define RANGE_ITERATOR___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)range_iterator___setstate__, METH_O, range_iterator___setstate____doc__},

static PyObject *
range_iterator___setstate___impl(_PyRangeIterObject *r, PyObject *state);

static PyObject *
range_iterator___setstate__(PyObject *r, PyObject *state)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(r);
    return_value = range_iterator___setstate___impl((_PyRangeIterObject *)r, state);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(longrange_iterator___length_hint____doc__,
"__length_hint__($self, /)\n"
"--\n"
"\n"
"Private method returning an estimate of len(list(it)).");

#define LONGRANGE_ITERATOR___LENGTH_HINT___METHODDEF    \
    {"__length_hint__", (PyCFunction)longrange_iterator___length_hint__, METH_NOARGS, longrange_iterator___length_hint____doc__},

static PyObject *
longrange_iterator___length_hint___impl(longrangeiterobject *r);

static PyObject *
longrange_iterator___length_hint__(PyObject *r, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(r);
    return_value = longrange_iterator___length_hint___impl((longrangeiterobject *)r);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(longrange_iterator___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n"
"Return state information for pickling.");

#define LONGRANGE_ITERATOR___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)longrange_iterator___reduce__, METH_NOARGS, longrange_iterator___reduce____doc__},

static PyObject *
longrange_iterator___reduce___impl(longrangeiterobject *r);

static PyObject *
longrange_iterator___reduce__(PyObject *r, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(r);
    return_value = longrange_iterator___reduce___impl((longrangeiterobject *)r);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(longrange_iterator___setstate____doc__,
"__setstate__($self, state, /)\n"
"--\n"
"\n"
"Set state information for unpickling.");

#define LONGRANGE_ITERATOR___SETSTATE___METHODDEF    \
    {"__setstate__", (PyCFunction)longrange_iterator___setstate__, METH_O, longrange_iterator___setstate____doc__},

static PyObject *
longrange_iterator___setstate___impl(longrangeiterobject *r, PyObject *state);

static PyObject *
longrange_iterator___setstate__(PyObject *r, PyObject *state)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(r);
    return_value = longrange_iterator___setstate___impl((longrangeiterobject *)r, state);
    Py_END_CRITICAL_SECTION();

    return return_value;
}
/*[clinic end generated code: output=719c0e4c81fe0f4a input=a9049054013a1b77]*/
