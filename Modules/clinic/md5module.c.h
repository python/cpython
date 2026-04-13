/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(MD5Type_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define MD5TYPE_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(MD5Type_copy), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, MD5Type_copy__doc__},

static PyObject *
MD5Type_copy_impl(MD5object *self, PyTypeObject *cls);

static PyObject *
MD5Type_copy(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        return NULL;
    }
    return MD5Type_copy_impl((MD5object *)self, cls);
}

PyDoc_STRVAR(MD5Type_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define MD5TYPE_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)MD5Type_digest, METH_NOARGS, MD5Type_digest__doc__},

static PyObject *
MD5Type_digest_impl(MD5object *self);

static PyObject *
MD5Type_digest(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return MD5Type_digest_impl((MD5object *)self);
}

PyDoc_STRVAR(MD5Type_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define MD5TYPE_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)MD5Type_hexdigest, METH_NOARGS, MD5Type_hexdigest__doc__},

static PyObject *
MD5Type_hexdigest_impl(MD5object *self);

static PyObject *
MD5Type_hexdigest(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return MD5Type_hexdigest_impl((MD5object *)self);
}

PyDoc_STRVAR(MD5Type_update__doc__,
"update($self, obj, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define MD5TYPE_UPDATE_METHODDEF    \
    {"update", (PyCFunction)MD5Type_update, METH_O, MD5Type_update__doc__},

static PyObject *
MD5Type_update_impl(MD5object *self, PyObject *obj);

static PyObject *
MD5Type_update(PyObject *self, PyObject *obj)
{
    PyObject *return_value = NULL;

    return_value = MD5Type_update_impl((MD5object *)self, obj);

    return return_value;
}

PyDoc_STRVAR(_md5_md5__doc__,
"md5($module, /, data=b\'\', *, usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Return a new MD5 hash object; optionally initialized with a string.");

#define _MD5_MD5_METHODDEF    \
    {"md5", _PyCFunction_CAST(_md5_md5), METH_FASTCALL|METH_KEYWORDS, _md5_md5__doc__},

static PyObject *
_md5_md5_impl(PyObject *module, PyObject *data, int usedforsecurity,
              PyObject *string_obj);

static PyObject *
_md5_md5(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(data), &_Py_ID(usedforsecurity), &_Py_ID(string), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"data", "usedforsecurity", "string", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "md5",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data = NULL;
    int usedforsecurity = 1;
    PyObject *string_obj = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        data = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    if (args[1]) {
        usedforsecurity = PyObject_IsTrue(args[1]);
        if (usedforsecurity < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_kwonly;
        }
    }
    string_obj = args[2];
skip_optional_kwonly:
    return_value = _md5_md5_impl(module, data, usedforsecurity, string_obj);

exit:
    return return_value;
}
/*[clinic end generated code: output=920fe54b9ed06f92 input=a9049054013a1b77]*/
