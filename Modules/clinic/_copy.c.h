/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_copy_deepcopy__doc__,
   "deepcopy($module, x, memo=None, /)\n"
     "--\n"
     "\n"
     "Create a deep copy of x\n"
     "\n"
     "See the documentation for the copy module for details.");

#define _COPY_DEEPCOPY_METHODDEF    \
    {"deepcopy", (PyCFunction)_copy_deepcopy, METH_VARARGS, _copy_deepcopy__doc__},

static PyObject *
_copy_deepcopy_impl(PyObject * module, PyObject * x, PyObject * memo);

static PyObject *
_copy_deepcopy(PyObject * module, PyObject * args)
 {
    PyObject * return_value = NULL;
    PyObject * x;
    PyObject * memo = Py_None;
    
        if (!PyArg_UnpackTuple(args, "deepcopy",
            1, 2,
            &x, &memo)) {
        goto exit;
        
    }
     return_value = _copy_deepcopy_impl(module, x, memo);
    
        exit:
    return return_value;
    }
/*[clinic end generated code: output=5c6b3eb60e1908c6 input=a9049054013a1b77]*/
