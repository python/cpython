/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_sha2_sha224_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define _SHA2_SHA224_COPY_METHODDEF    \
    {"copy", (PyCFunction)_sha2_sha224_copy, METH_NOARGS, _sha2_sha224_copy__doc__},

static PyObject *
_sha2_sha224_copy_impl(SHA2N_O(224) *self);

static PyObject *
_sha2_sha224_copy(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sha2_sha224_copy_impl((SHA2N_O(224) *)self);
}

PyDoc_STRVAR(_sha2_sha256_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define _SHA2_SHA256_COPY_METHODDEF    \
    {"copy", (PyCFunction)_sha2_sha256_copy, METH_NOARGS, _sha2_sha256_copy__doc__},

static PyObject *
_sha2_sha256_copy_impl(SHA2N_O(256) *self);

static PyObject *
_sha2_sha256_copy(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sha2_sha256_copy_impl((SHA2N_O(256) *)self);
}

PyDoc_STRVAR(_sha2_sha384_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define _SHA2_SHA384_COPY_METHODDEF    \
    {"copy", (PyCFunction)_sha2_sha384_copy, METH_NOARGS, _sha2_sha384_copy__doc__},

static PyObject *
_sha2_sha384_copy_impl(SHA2N_O(384) *self);

static PyObject *
_sha2_sha384_copy(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sha2_sha384_copy_impl((SHA2N_O(384) *)self);
}

PyDoc_STRVAR(_sha2_sha512_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the hash object.");

#define _SHA2_SHA512_COPY_METHODDEF    \
    {"copy", (PyCFunction)_sha2_sha512_copy, METH_NOARGS, _sha2_sha512_copy__doc__},

static PyObject *
_sha2_sha512_copy_impl(SHA2N_O(512) *self);

static PyObject *
_sha2_sha512_copy(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sha2_sha512_copy_impl((SHA2N_O(512) *)self);
}

PyDoc_STRVAR(_sha2_sha224_update__doc__,
"update($self, data, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define _SHA2_SHA224_UPDATE_METHODDEF    \
    {"update", (PyCFunction)_sha2_sha224_update, METH_O, _sha2_sha224_update__doc__},

PyDoc_STRVAR(_sha2_sha256_update__doc__,
"update($self, data, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define _SHA2_SHA256_UPDATE_METHODDEF    \
    {"update", (PyCFunction)_sha2_sha256_update, METH_O, _sha2_sha256_update__doc__},

PyDoc_STRVAR(_sha2_sha384_update__doc__,
"update($self, data, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define _SHA2_SHA384_UPDATE_METHODDEF    \
    {"update", (PyCFunction)_sha2_sha384_update, METH_O, _sha2_sha384_update__doc__},

PyDoc_STRVAR(_sha2_sha512_update__doc__,
"update($self, data, /)\n"
"--\n"
"\n"
"Update this hash object\'s state with the provided string.");

#define _SHA2_SHA512_UPDATE_METHODDEF    \
    {"update", (PyCFunction)_sha2_sha512_update, METH_O, _sha2_sha512_update__doc__},

PyDoc_STRVAR(_sha2_sha224_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define _SHA2_SHA224_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)_sha2_sha224_digest, METH_NOARGS, _sha2_sha224_digest__doc__},

static PyObject *
_sha2_sha224_digest_impl(SHA2N_O(224) *self);

static PyObject *
_sha2_sha224_digest(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sha2_sha224_digest_impl((SHA2N_O(224) *)self);
}

PyDoc_STRVAR(_sha2_sha256_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define _SHA2_SHA256_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)_sha2_sha256_digest, METH_NOARGS, _sha2_sha256_digest__doc__},

static PyObject *
_sha2_sha256_digest_impl(SHA2N_O(256) *self);

static PyObject *
_sha2_sha256_digest(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sha2_sha256_digest_impl((SHA2N_O(256) *)self);
}

PyDoc_STRVAR(_sha2_sha384_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define _SHA2_SHA384_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)_sha2_sha384_digest, METH_NOARGS, _sha2_sha384_digest__doc__},

static PyObject *
_sha2_sha384_digest_impl(SHA2N_O(384) *self);

static PyObject *
_sha2_sha384_digest(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sha2_sha384_digest_impl((SHA2N_O(384) *)self);
}

PyDoc_STRVAR(_sha2_sha512_digest__doc__,
"digest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a bytes object.");

#define _SHA2_SHA512_DIGEST_METHODDEF    \
    {"digest", (PyCFunction)_sha2_sha512_digest, METH_NOARGS, _sha2_sha512_digest__doc__},

static PyObject *
_sha2_sha512_digest_impl(SHA2N_O(512) *self);

static PyObject *
_sha2_sha512_digest(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sha2_sha512_digest_impl((SHA2N_O(512) *)self);
}

PyDoc_STRVAR(_sha2_sha224_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _SHA2_SHA224_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)_sha2_sha224_hexdigest, METH_NOARGS, _sha2_sha224_hexdigest__doc__},

static PyObject *
_sha2_sha224_hexdigest_impl(SHA2N_O(224) *self);

static PyObject *
_sha2_sha224_hexdigest(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sha2_sha224_hexdigest_impl((SHA2N_O(224) *)self);
}

PyDoc_STRVAR(_sha2_sha256_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _SHA2_SHA256_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)_sha2_sha256_hexdigest, METH_NOARGS, _sha2_sha256_hexdigest__doc__},

static PyObject *
_sha2_sha256_hexdigest_impl(SHA2N_O(256) *self);

static PyObject *
_sha2_sha256_hexdigest(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sha2_sha256_hexdigest_impl((SHA2N_O(256) *)self);
}

PyDoc_STRVAR(_sha2_sha384_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _SHA2_SHA384_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)_sha2_sha384_hexdigest, METH_NOARGS, _sha2_sha384_hexdigest__doc__},

static PyObject *
_sha2_sha384_hexdigest_impl(SHA2N_O(384) *self);

static PyObject *
_sha2_sha384_hexdigest(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sha2_sha384_hexdigest_impl((SHA2N_O(384) *)self);
}

PyDoc_STRVAR(_sha2_sha512_hexdigest__doc__,
"hexdigest($self, /)\n"
"--\n"
"\n"
"Return the digest value as a string of hexadecimal digits.");

#define _SHA2_SHA512_HEXDIGEST_METHODDEF    \
    {"hexdigest", (PyCFunction)_sha2_sha512_hexdigest, METH_NOARGS, _sha2_sha512_hexdigest__doc__},

static PyObject *
_sha2_sha512_hexdigest_impl(SHA2N_O(512) *self);

static PyObject *
_sha2_sha512_hexdigest(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sha2_sha512_hexdigest_impl((SHA2N_O(512) *)self);
}

PyDoc_STRVAR(_sha2_sha224__doc__,
"sha224($module, /, data=b\'\', *, usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Return a new SHA-224 hash object; optionally initialized with a string.");

#define _SHA2_SHA224_METHODDEF    \
    {"sha224", _PyCFunction_CAST(_sha2_sha224), METH_FASTCALL|METH_KEYWORDS, _sha2_sha224__doc__},

static PyObject *
_sha2_sha224_impl(PyObject *module, PyObject *data, int usedforsecurity,
                  PyObject *string);

static PyObject *
_sha2_sha224(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "sha224",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data = NULL;
    int usedforsecurity = 1;
    PyObject *string = NULL;

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
    string = args[2];
skip_optional_kwonly:
    return_value = _sha2_sha224_impl(module, data, usedforsecurity, string);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha2_sha256__doc__,
"sha256($module, /, data=b\'\', *, usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Return a new SHA-256 hash object; optionally initialized with a string.");

#define _SHA2_SHA256_METHODDEF    \
    {"sha256", _PyCFunction_CAST(_sha2_sha256), METH_FASTCALL|METH_KEYWORDS, _sha2_sha256__doc__},

static PyObject *
_sha2_sha256_impl(PyObject *module, PyObject *data, int usedforsecurity,
                  PyObject *string);

static PyObject *
_sha2_sha256(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "sha256",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data = NULL;
    int usedforsecurity = 1;
    PyObject *string = NULL;

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
    string = args[2];
skip_optional_kwonly:
    return_value = _sha2_sha256_impl(module, data, usedforsecurity, string);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha2_sha384__doc__,
"sha384($module, /, data=b\'\', *, usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Return a new SHA-384 hash object; optionally initialized with a string.");

#define _SHA2_SHA384_METHODDEF    \
    {"sha384", _PyCFunction_CAST(_sha2_sha384), METH_FASTCALL|METH_KEYWORDS, _sha2_sha384__doc__},

static PyObject *
_sha2_sha384_impl(PyObject *module, PyObject *data, int usedforsecurity,
                  PyObject *string);

static PyObject *
_sha2_sha384(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "sha384",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data = NULL;
    int usedforsecurity = 1;
    PyObject *string = NULL;

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
    string = args[2];
skip_optional_kwonly:
    return_value = _sha2_sha384_impl(module, data, usedforsecurity, string);

exit:
    return return_value;
}

PyDoc_STRVAR(_sha2_sha512__doc__,
"sha512($module, /, data=b\'\', *, usedforsecurity=True, string=None)\n"
"--\n"
"\n"
"Return a new SHA-512 hash object; optionally initialized with a string.");

#define _SHA2_SHA512_METHODDEF    \
    {"sha512", _PyCFunction_CAST(_sha2_sha512), METH_FASTCALL|METH_KEYWORDS, _sha2_sha512__doc__},

static PyObject *
_sha2_sha512_impl(PyObject *module, PyObject *data, int usedforsecurity,
                  PyObject *string);

static PyObject *
_sha2_sha512(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "sha512",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *data = NULL;
    int usedforsecurity = 1;
    PyObject *string = NULL;

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
    string = args[2];
skip_optional_kwonly:
    return_value = _sha2_sha512_impl(module, data, usedforsecurity, string);

exit:
    return return_value;
}
/*[clinic end generated code: output=c44fd7727002844e input=a9049054013a1b77]*/
