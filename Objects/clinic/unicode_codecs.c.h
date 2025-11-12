/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(EncodingMap_size__doc__,
"size($self, /)\n"
"--\n"
"\n"
"Return the size (in bytes) of this object.");

#define ENCODINGMAP_SIZE_METHODDEF    \
    {"size", (PyCFunction)EncodingMap_size, METH_NOARGS, EncodingMap_size__doc__},

static PyObject *
EncodingMap_size_impl(struct encoding_map *self);

static PyObject *
EncodingMap_size(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return EncodingMap_size_impl((struct encoding_map *)self);
}
/*[clinic end generated code: output=0f563ba23bbdc339 input=a9049054013a1b77]*/
