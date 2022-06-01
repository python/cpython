/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(_statistics__normal_dist_inv_cdf__doc__,
"_normal_dist_inv_cdf($module, p, mu, sigma, /)\n"
"--\n"
"\n");

#define _STATISTICS__NORMAL_DIST_INV_CDF_METHODDEF    \
    {"_normal_dist_inv_cdf", _PyCFunction_CAST(_statistics__normal_dist_inv_cdf), METH_FASTCALL, _statistics__normal_dist_inv_cdf__doc__},

static double
_statistics__normal_dist_inv_cdf_impl(PyObject *module, double p, double mu,
                                      double sigma);

static PyObject *
_statistics__normal_dist_inv_cdf(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    double p;
    double mu;
    double sigma;
    double _return_value;

    if (!_PyArg_CheckPositional("_normal_dist_inv_cdf", nargs, 3, 3)) {
        goto exit;
    }
    if (PyFloat_CheckExact(args[0])) {
        p = PyFloat_AS_DOUBLE(args[0]);
    }
    else
    {
        p = PyFloat_AsDouble(args[0]);
        if (p == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (PyFloat_CheckExact(args[1])) {
        mu = PyFloat_AS_DOUBLE(args[1]);
    }
    else
    {
        mu = PyFloat_AsDouble(args[1]);
        if (mu == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    if (PyFloat_CheckExact(args[2])) {
        sigma = PyFloat_AS_DOUBLE(args[2]);
    }
    else
    {
        sigma = PyFloat_AsDouble(args[2]);
        if (sigma == -1.0 && PyErr_Occurred()) {
            goto exit;
        }
    }
    _return_value = _statistics__normal_dist_inv_cdf_impl(module, p, mu, sigma);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyFloat_FromDouble(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=6899dc752cc6b457 input=a9049054013a1b77]*/
