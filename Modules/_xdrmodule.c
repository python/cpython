/* This module exports part of the C API to the XDR routines into Python.
 * XDR is Sun's eXternal Data Representation, as described in RFC 1014.
 * This module is used by xdrlib.py to support the float and double data
 * types which are too much of a pain to support in Python directly.  It is
 * not required by xdrlib.py -- when not available, these types aren't
 * supported at the Python layer.  Note that representations that can be
 * implemented solely in Python, are *not* reproduced here.
 *
 * Module version number: 1.0
 *
 * See xdrlib.py for usage notes.
 *
 * Note: this has so far, only been tested on Solaris 2.5 and IRIX 5.3.  On
 * these systems, you will need to link with -lnsl for these symbols to be
 * defined.
 */

#include "Python.h"

#include <rpc/rpc.h>
#include <rpc/xdr.h>

static PyObject* xdr_error;



static PyObject*
pack_float(self, args)
        PyObject* self;
        PyObject* args;
{
        XDR xdr;
        float value;
        union {                              /* guarantees proper alignment */
                long dummy;
                char buffer[4];
        } addr;
        PyObject* rtn = NULL;

        if (!PyArg_ParseTuple(args, "f", &value))
                return NULL;

        xdr.x_ops = NULL;
        xdrmem_create(&xdr, addr.buffer, 4, XDR_ENCODE);
        if (xdr.x_ops == NULL)
                PyErr_SetString(xdr_error,
                                "XDR stream initialization failed.");
        else if (xdr_float(&xdr, &value))
                rtn = PyString_FromStringAndSize(addr.buffer, 4);
        else
                PyErr_SetString(xdr_error, "conversion from float failed");

        xdr_destroy(&xdr);
        return rtn;
}
    


static PyObject*
pack_double(self, args)
        PyObject* self;
        PyObject* args;
{
        XDR xdr;
        double value;
        union {                              /* guarantees proper alignment */
                long dummy;
                char buffer[8];
        } addr;
        PyObject* rtn = NULL;

        if (!PyArg_ParseTuple(args, "d", &value))
                return NULL;

        xdr.x_ops = NULL;
        xdrmem_create(&xdr, addr.buffer, 8, XDR_ENCODE);
        if (xdr.x_ops == NULL)
                PyErr_SetString(xdr_error,
                                "XDR stream initialization failed.");
        else if (xdr_double(&xdr, &value))
                rtn = PyString_FromStringAndSize(addr.buffer, 8);
        else
                PyErr_SetString(xdr_error, "conversion from double failed");

        xdr_destroy(&xdr);
        return rtn;
}



static PyObject*
unpack_float(self, args)
        PyObject* self;
        PyObject* args;
{
        XDR xdr;
        float value;
        char* string;
        int strlen;
        PyObject* rtn = NULL;

        if (!PyArg_ParseTuple(args, "s#", &string, &strlen))
                return NULL;

        if (strlen != 4) {
                PyErr_SetString(PyExc_ValueError, "4 byte string expected");
                return NULL;
        }

        /* Python guarantees that the string is 4 byte aligned */
        xdr.x_ops = NULL;
        xdrmem_create(&xdr, (caddr_t)string, 4, XDR_DECODE);
        if (xdr.x_ops == NULL)
                PyErr_SetString(xdr_error,
                                "XDR stream initialization failed.");
        else if (xdr_float(&xdr, &value))
                rtn = Py_BuildValue("f", value);
        else
                PyErr_SetString(xdr_error, "conversion to float failed");

        xdr_destroy(&xdr);
        return rtn;
}

    

static PyObject*
unpack_double(self, args)
        PyObject* self;
        PyObject* args;
{
        XDR xdr;
        double value;
        char* string;
        int strlen;
        PyObject* rtn = NULL;

        if (!PyArg_ParseTuple(args, "s#", &string, &strlen))
                return NULL;

        if (strlen != 8) {
                PyErr_SetString(PyExc_ValueError, "8 byte string expected");
                return NULL;
        }

        /* Python guarantees that the string is 4 byte aligned */
        xdr.x_ops = NULL;
        xdrmem_create(&xdr, (caddr_t)string, 8, XDR_DECODE);
        if (xdr.x_ops == NULL)
                PyErr_SetString(xdr_error,
                                "XDR stream initialization failed.");
        else if (xdr_double(&xdr, &value))
                rtn = Py_BuildValue("d", value);
        else
                PyErr_SetString(xdr_error, "conversion to double failed");

        xdr_destroy(&xdr);
        return rtn;
}



static struct PyMethodDef
xdr_methods[] = {
        {"pack_float",    pack_float,    1},
        {"pack_double",   pack_double,   1},
        {"unpack_float",  unpack_float,  1},
        {"unpack_double", unpack_double, 1},
        {NULL,          NULL,       0}           /* sentinel */
};



void
init_xdr()
{
        PyObject* module;
        PyObject* dict;

        module = Py_InitModule("_xdr", xdr_methods);
        dict = PyModule_GetDict(module);

        xdr_error = PyString_FromString("_xdr.error");
        PyDict_SetItemString(dict, "error", xdr_error);
        if (PyErr_Occurred())
                Py_FatalError("can't initialize module _xdr");
}
