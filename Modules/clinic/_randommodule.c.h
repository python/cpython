/*[clinic input]
preserve
[clinic start generated code]*/

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
_random_Random_random(RandomObject *self, PyObject *Py_UNUSED(ignored))
{
    return _random_Random_random_impl(self);
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
    {"seed", (PyCFunction)(void(*)(void))_random_Random_seed, METH_FASTCALL, _random_Random_seed__doc__},

static PyObject *
_random_Random_seed_impl(RandomObject *self, PyObject *n);

static PyObject *
_random_Random_seed(RandomObject *self, PyObject *const *args, Py_ssize_t nargs)
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
    return_value = _random_Random_seed_impl(self, n);

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
_random_Random_getstate(RandomObject *self, PyObject *Py_UNUSED(ignored))
{
    return _random_Random_getstate_impl(self);
}

PyDoc_STRVAR(_random_Random_setstate__doc__,
"setstate($self, state, /)\n"
"--\n"
"\n"
"setstate(state) -> None.  Restores generator state.");

#define _RANDOM_RANDOM_SETSTATE_METHODDEF    \
    {"setstate", (PyCFunction)_random_Random_setstate, METH_O, _random_Random_setstate__doc__},

PyDoc_STRVAR(_random_Random_getrandbits__doc__,
"getrandbits($self, k, /)\n"
"--\n"
"\n"
"getrandbits(k) -> x.  Generates an int with k random bits.");

#define _RANDOM_RANDOM_GETRANDBITS_METHODDEF    \
    {"getrandbits", (PyCFunction)_random_Random_getrandbits, METH_O, _random_Random_getrandbits__doc__},

static PyObject *
_random_Random_getrandbits_impl(RandomObject *self, int k);

static PyObject *
_random_Random_getrandbits(RandomObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int k;

    k = _PyLong_AsInt(arg);
    if (k == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _random_Random_getrandbits_impl(self, k);

exit:
    return return_value;
}

PyDoc_STRVAR(_random_Random___reduce____doc__,
"__reduce__($self, /)\n"
"--\n"
"\n");

#define _RANDOM_RANDOM___REDUCE___METHODDEF    \
    {"__reduce__", (PyCFunction)_random_Random___reduce__, METH_NOARGS, _random_Random___reduce____doc__},

static PyObject *
_random_Random___reduce___impl(RandomObject *self);

static PyObject *
_random_Random___reduce__(RandomObject *self, PyObject *Py_UNUSED(ignored))
{
    return _random_Random___reduce___impl(self);
}
/*[clinic end generated code: output=450f0961c2c92389 input=a9049054013a1b77]*/
