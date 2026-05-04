/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_long.h"          // _PyLong_UInt64_Converter()
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(_random_Random_random__doc__,
"random($self, /)\n"
"--\n"
"\n"
"random() -> x in the interval [0, 1).");

#define _RANDOM_RANDOM_RANDOM_METHODDEF    \
    {"random", (PyCFunction)_random_Random_random, METH_NOARGS, _random_Random_random__doc__},

static PyObject *
_random_Random_random_impl(RandomObject *self);

static PyObject *
_random_Random_random(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _random_Random_random_impl((RandomObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_random_Random_seed__doc__,
"seed($self, n=None, /)\n"
"--\n"
"\n"
"seed([n]) -> None.\n"
"\n"
"Defaults to use urandom and falls back to a combination\n"
"of the current time and the process identifier.");

#define _RANDOM_RANDOM_SEED_METHODDEF    \
    {"seed", _PyCFunction_CAST(_random_Random_seed), METH_FASTCALL, _random_Random_seed__doc__},

static PyObject *
_random_Random_seed_impl(RandomObject *self, PyObject *n);

static PyObject *
_random_Random_seed(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *n = Py_None;

    if (!_PyArg_CheckPositional("seed", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    n = args[0];
skip_optional:
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _random_Random_seed_impl((RandomObject *)self, n);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}

PyDoc_STRVAR(_random_Random_getstate__doc__,
"getstate($self, /)\n"
"--\n"
"\n"
"getstate() -> tuple containing the current state.");

#define _RANDOM_RANDOM_GETSTATE_METHODDEF    \
    {"getstate", (PyCFunction)_random_Random_getstate, METH_NOARGS, _random_Random_getstate__doc__},

static PyObject *
_random_Random_getstate_impl(RandomObject *self);

static PyObject *
_random_Random_getstate(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _random_Random_getstate_impl((RandomObject *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_random_Random_setstate__doc__,
"setstate($self, state, /)\n"
"--\n"
"\n"
"setstate(state) -> None.  Restores generator state.");

#define _RANDOM_RANDOM_SETSTATE_METHODDEF    \
    {"setstate", (PyCFunction)_random_Random_setstate, METH_O, _random_Random_setstate__doc__},

static PyObject *
_random_Random_setstate_impl(RandomObject *self, PyObject *state);

static PyObject *
_random_Random_setstate(PyObject *self, PyObject *state)
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _random_Random_setstate_impl((RandomObject *)self, state);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_random_Random_getrandbits__doc__,
"getrandbits($self, k, /)\n"
"--\n"
"\n"
"getrandbits(k) -> x.  Generates an int with k random bits.");

#define _RANDOM_RANDOM_GETRANDBITS_METHODDEF    \
    {"getrandbits", (PyCFunction)_random_Random_getrandbits, METH_O, _random_Random_getrandbits__doc__},

static PyObject *
_random_Random_getrandbits_impl(RandomObject *self, uint64_t k);

static PyObject *
_random_Random_getrandbits(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    uint64_t k;

    if (!_PyLong_UInt64_Converter(arg, &k)) {
        goto exit;
    }
    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _random_Random_getrandbits_impl((RandomObject *)self, k);
    Py_END_CRITICAL_SECTION();

exit:
    return return_value;
}
/*[clinic end generated code: output=7ce97b2194eecaf7 input=a9049054013a1b77]*/
