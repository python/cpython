/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(SHA256Type_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define SHA256TYPE_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(SHA256Type_copy), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, SHA256Type_copy__doc__},

static PyObject *
SHA256Type_copy_impl(SHA256object *self, PyTypeObject *cls);

static PyObject *
SHA256Type_copy(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        return NULL;
    }
    return SHA256Type_copy_impl((SHA256object *)self, cls);
}

PyDoc_STRVAR(SHA512Type_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define SHA512TYPE_COPY_METHODDEF    \
    {"copy", _PyCFunction_CAST(SHA512Type_copy), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, SHA512Type_copy__doc__},

static PyObject *
SHA512Type_copy_impl(SHA512object *self, PyTypeObject *cls);

static PyObject *
SHA512Type_copy(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        return NULL;
    }
    return SHA512Type_copy_impl((SHA512object *)self, cls);
}

PyDoc_STRVAR(SHA256Type_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define SHA256TYPE_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)SHA256Type_digest, METH_NOARGS, SHA256Type_digest__doc__},

static PyObject *
SHA256Type_digest_impl(SHA256object *self);

static PyObject *
SHA256Type_digest(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return SHA256Type_digest_impl((SHA256object *)self);
}

PyDoc_STRVAR(SHA512Type_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define SHA512TYPE_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)SHA512Type_digest, METH_NOARGS, SHA512Type_digest__doc__},

static PyObject *
SHA512Type_digest_impl(SHA512object *self);

static PyObject *
SHA512Type_digest(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return SHA512Type_digest_impl((SHA512object *)self);
}

PyDoc_STRVAR(SHA256Type_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define SHA256TYPE_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)SHA256Type_hexdigest, METH_NOARGS, SHA256Type_hexdigest__doc__},

static PyObject *
SHA256Type_hexdigest_impl(SHA256object *self);

static PyObject *
SHA256Type_hexdigest(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return SHA256Type_hexdigest_impl((SHA256object *)self);
}

PyDoc_STRVAR(SHA512Type_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define SHA512TYPE_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)SHA512Type_hexdigest, METH_NOARGS, SHA512Type_hexdigest__doc__},

static PyObject *
SHA512Type_hexdigest_impl(SHA512object *self);

static PyObject *
SHA512Type_hexdigest(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return SHA512Type_hexdigest_impl((SHA512object *)self);
}

PyDoc_STRVAR(SHA256Type_update__doc__,
"update($self, obj, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define SHA256TYPE_UPDATE_METHODDEF    \
    {"update", (PyCFunction)SHA256Type_update, METH_O, SHA256Type_update__doc__},

static PyObject *
SHA256Type_update_impl(SHA256object *self, PyObject *obj);

static PyObject *
SHA256Type_update(PyObject *self, PyObject *obj)
{
    PyObject *return_value = NULL;

    return_value = SHA256Type_update_impl((SHA256object *)self, obj);

    return return_value;
}

PyDoc_STRVAR(SHA512Type_update__doc__,
"update($self, obj, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define SHA512TYPE_UPDATE_METHODDEF    \
    {"update", (PyCFunction)SHA512Type_update, METH_O, SHA512Type_update__doc__},

static PyObject *
SHA512Type_update_impl(SHA512object *self, PyObject *obj);

static PyObject *
SHA512Type_update(PyObject *self, PyObject *obj)
{
    PyObject *return_value = NULL;

    return_value = SHA512Type_update_impl((SHA512object *)self, obj);

    return return_value;
}

PyDoc_STRVAR(_sha2_sha256__doc__,
"sha256($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Return a new SHA-256 hash object; optionally initialized with a string.");

#define _SHA2_SHA256_METHODDEF    \
    {"sha256", _PyCFunction_CAST(_sha2_sha256), METH_FASTCALL|METH_KEYWORDS, _sha2_sha256__doc__},

static PyObject *
_sha2_sha256_impl(PyObject *module, PyObject *string, int usedforsecurity);

static PyObject *
_sha2_sha256(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(string), &_Py_ID(usedforsecurity), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "sha256",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *string = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        string = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[1]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _sha2_sha256_impl(module, string, usedforsecurity);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha2_sha224__doc__,
"sha224($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Return a new SHA-224 hash object; optionally initialized with a string.");

#define _SHA2_SHA224_METHODDEF    \
    {"sha224", _PyCFunction_CAST(_sha2_sha224), METH_FASTCALL|METH_KEYWORDS, _sha2_sha224__doc__},

static PyObject *
_sha2_sha224_impl(PyObject *module, PyObject *string, int usedforsecurity);

static PyObject *
_sha2_sha224(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(string), &_Py_ID(usedforsecurity), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "sha224",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *string = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        string = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[1]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _sha2_sha224_impl(module, string, usedforsecurity);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha2_sha512__doc__,
"sha512($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Return a new SHA-512 hash object; optionally initialized with a string.");

#define _SHA2_SHA512_METHODDEF    \
    {"sha512", _PyCFunction_CAST(_sha2_sha512), METH_FASTCALL|METH_KEYWORDS, _sha2_sha512__doc__},

static PyObject *
_sha2_sha512_impl(PyObject *module, PyObject *string, int usedforsecurity);

static PyObject *
_sha2_sha512(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(string), &_Py_ID(usedforsecurity), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "sha512",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *string = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        string = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[1]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _sha2_sha512_impl(module, string, usedforsecurity);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha2_sha384__doc__,
"sha384($module, /, string=b\'\', *, usedforsecurity=True)\n"
"--\n"
"\n"
"Return a new SHA-384 hash object; optionally initialized with a string.");

#define _SHA2_SHA384_METHODDEF    \
    {"sha384", _PyCFunction_CAST(_sha2_sha384), METH_FASTCALL|METH_KEYWORDS, _sha2_sha384__doc__},

static PyObject *
_sha2_sha384_impl(PyObject *module, PyObject *string, int usedforsecurity);

static PyObject *
_sha2_sha384(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(string), &_Py_ID(usedforsecurity), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"string", "usedforsecurity", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "sha384",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *string = NULL;
    int usedforsecurity = 1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        string = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
skip_optional_pos:
    if (!noptargs) {
        goto skip_optional_kwonly;
    }
    usedforsecurity = PyObject_IsTrue(args[1]);
    if (usedforsecurity < 0) {
        goto exit;
    }
skip_optional_kwonly:
    return_value = _sha2_sha384_impl(module, string, usedforsecurity);

exit:
    return return_value;
}
/*[clinic end generated code: output=af11090855b7c85a input=a9049054013a1b77]*/
