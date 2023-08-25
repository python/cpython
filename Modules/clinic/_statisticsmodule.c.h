/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_statistics__normal_dist_inv_cdf__doc__,
"_normal_dist_inv_cdf($module, p, mu, sigma, /)\n"
"--\n"
"\n");

#define _STATISTICS__NORMAL_DIST_INV_CDF_METHODDEF    \
    {"_normal_dist_inv_cdf", (PyCFunction)_statistics__normal_dist_inv_cdf, METH_VARARGS, _statistics__normal_dist_inv_cdf__doc__},

static double
_statistics__normal_dist_inv_cdf_impl(PyObject *module, double p, double mu,
                                      double sigma);

static PyObject *
_statistics__normal_dist_inv_cdf(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    double p;
    double mu;
    double sigma;
    double _return_value;

    if (!PyArg_ParseTuple(args, "ddd:_normal_dist_inv_cdf",
        &p, &mu, &sigma))
        goto exit;
    _return_value = _statistics__normal_dist_inv_cdf_impl(module, p, mu, sigma);
    if ((_return_value == -1.0) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyFloat_FromDouble(_return_value);

exit:
    return return_value;
}
/*[clinic end generated code: output=63371081183774c9 input=a9049054013a1b77]*/
