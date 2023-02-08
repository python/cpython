/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(_copy_deepcopy__doc__,
"deepcopy($module, x, memo=None, /)\n"
"--\n"
"\n"
"Create a deep copy of x\n"
"\n"
"See the documentation for the copy module for details.");


static PyObject *
_copy_deepcopy_impl(PyObject *module, PyObject *x, PyObject *memo);

static PyObject *
_copy_deepcopy(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *memo = Py_None;

    if (!_PyArg_CheckPositional("deepcopy", nargs, 1, 2)) {
        return NULL;
    }
    if (nargs == 2) {
        memo = args[1];
    }
    return _copy_deepcopy_impl(module, args[0], memo);

}
/*[clinic end generated code: output=ab88e7f79337ebab input=a9049054013a1b77]*/
