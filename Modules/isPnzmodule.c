#include <Python.h>
#include <math.h>
#include <float.h> // For DBL_MAX and DBL_MIN

// Add the docstring for the function
static char isPnz_doc[] = 
    "isPnz(x)\n"
    "------------\n"
    "Determines if a number is positive, negative, or zero.\n\n"
    "Returns True if the number is positive (non-zero),\n"
    "False if the number is negative (non-zero),\n"
    "None if the number is positive zero (+0.0),\n"
    "False if the number is negative zero (-0.0).\n\n"
    "The function uses `copysign` to detect signed zeros.\n"
    "This ensures compatibility with IEEE 754 floating-point standards.\n\n"
    "The function accepts both integers and floating-point numbers.\n"
    "It converts integers to floating-point numbers before comparison.\n";

// Function definition for isPnz
static PyObject *
isPnz(PyObject *self, PyObject *arg)
{
    double x;
    // Fast conversion for both int and float types with overflow check
    ASSIGN_NUMBER(x, arg, error);

    // Check for NaN (Not a Number)
    if (isnan(x)) {
        PyErr_SetString(PyExc_ValueError, "The value is NaN.");
        return NULL;
    }

    // Check for infinity (positive or negative)
    if (isinf(x)) {
        if (x > 0.0)
            Py_RETURN_TRUE;  // Positive infinity
        else
            Py_RETURN_FALSE; // Negative infinity
    }

    // Check for negative zero explicitly using copysign
    if (x == 0.0) {
        if (copysign(1.0, x) > 0.0) {
            Py_RETURN_NONE;  // Positive zero
        } else {
            Py_RETURN_FALSE; // Negative zero
        }
    }

    if (x > 0.0)
        Py_RETURN_TRUE;  // Positive non-zero
    else
        Py_RETURN_FALSE; // Negative non-zero

error:
    return NULL;  // Invalid input, propagate the error
}

// Define the methods table
static PyMethodDef methods[] = {
    {"isPnz", isPnz, METH_O, isPnz_doc},  // Link docstring to the function
    {NULL, NULL, 0, NULL}  // Sentinel
};

// Module definition
static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "isPnzmodule",   // Name of the module
    "Module for isPnz function",  // Module docstring
    -1,
    methods
};

// Module initialization function
PyMODINIT_FUNC
PyInit_isPnzmodule(void)
{
    return PyModule_Create(&moduledef);
}
