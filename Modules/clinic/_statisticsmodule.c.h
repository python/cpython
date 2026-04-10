/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_statistics__normal_dist_inv_cdf__doc__,
"_normal_dist_inv_cdf($module, p, mu, sigma, /)\n"
"--\n"
"\n");

#define _STATISTICS__NORMAL_DIST_INV_CDF_METHODDEF    \
    {"_normal_dist_inv_cdf", (PyCFunction)(void(*)(void))_statistics__normal_dist_inv_cdf, METH_FASTCALL, _statistics__normal_dist_inv_cdf__doc__},

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

    if (nargs != 3) {
        PyErr_Format(PyExc_TypeError, "_normal_dist_inv_cdf expected 3 arguments, got %zd", nargs);
        goto exit;
    }
    p = PyFloat_AsDouble(args[0]);
    if (p == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    mu = PyFloat_AsDouble(args[1]);
    if (mu == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    sigma = PyFloat_AsDouble(args[2]);
    if (sigma == -1.0 && PyErr_Occurred()) {
        goto exit;
    }
    _return_value = _statistics__normal_dist_inv_cdf_impl(module, p, mu, sigma);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyFloat_FromDouble(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=0f0c849d51f16f1b input=a9049054013a1b77]*/
