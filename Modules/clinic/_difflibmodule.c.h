/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_difflib_SequenceMatcher___init____doc__,
"SequenceMatcher(isjunk=None, a=\'\', b=\'\', autojunk=True)\n"
"--\n"
"\n"
"Construct a SequenceMatcher.\n"
"\n"
"See difflib.py for the full documentation; output is identical to the\n"
"pure-Python SequenceMatcher class.");

static int
_difflib_SequenceMatcher___init___impl(SequenceMatcherObject *self,
                                       PyObject *isjunk, PyObject *a,
                                       PyObject *b, int autojunk);

static int
_difflib_SequenceMatcher___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(isjunk), _Py_LATIN1_CHR('a'), _Py_LATIN1_CHR('b'), &_Py_ID(autojunk), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"isjunk", "a", "b", "autojunk", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "SequenceMatcher",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *isjunk = Py_None;
    PyObject *a = NULL;
    PyObject *b = NULL;
    int autojunk = 1;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        isjunk = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[1]) {
        a = fastargs[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[2]) {
        b = fastargs[2];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    autojunk = PyObject_IsTrue(fastargs[3]);
    if (autojunk < 0) {
        goto exit;
    }
skip_optional_pos:
    return_value = _difflib_SequenceMatcher___init___impl((SequenceMatcherObject *)self, isjunk, a, b, autojunk);

exit:
    return return_value;
}

PyDoc_STRVAR(_difflib_SequenceMatcher_set_seq1__doc__,
"set_seq1($self, a, /)\n"
"--\n"
"\n"
"Set the first sequence to be compared.\n"
"\n"
"The second sequence to be compared is not changed.");

#define _DIFFLIB_SEQUENCEMATCHER_SET_SEQ1_METHODDEF    \
    {"set_seq1", (PyCFunction)_difflib_SequenceMatcher_set_seq1, METH_O, _difflib_SequenceMatcher_set_seq1__doc__},

static PyObject *
_difflib_SequenceMatcher_set_seq1_impl(SequenceMatcherObject *self,
                                       PyObject *a);

static PyObject *
_difflib_SequenceMatcher_set_seq1(PyObject *self, PyObject *a)
{
    PyObject *return_value = NULL;

    return_value = _difflib_SequenceMatcher_set_seq1_impl((SequenceMatcherObject *)self, a);

    return return_value;
}

PyDoc_STRVAR(_difflib_SequenceMatcher_set_seq2__doc__,
"set_seq2($self, b, /)\n"
"--\n"
"\n"
"Set the second sequence to be compared.\n"
"\n"
"The first sequence to be compared is not changed.");

#define _DIFFLIB_SEQUENCEMATCHER_SET_SEQ2_METHODDEF    \
    {"set_seq2", (PyCFunction)_difflib_SequenceMatcher_set_seq2, METH_O, _difflib_SequenceMatcher_set_seq2__doc__},

static PyObject *
_difflib_SequenceMatcher_set_seq2_impl(SequenceMatcherObject *self,
                                       PyObject *b);

static PyObject *
_difflib_SequenceMatcher_set_seq2(PyObject *self, PyObject *b)
{
    PyObject *return_value = NULL;

    return_value = _difflib_SequenceMatcher_set_seq2_impl((SequenceMatcherObject *)self, b);

    return return_value;
}

PyDoc_STRVAR(_difflib_SequenceMatcher_set_seqs__doc__,
"set_seqs($self, a, b, /)\n"
"--\n"
"\n"
"Set the two sequences to be compared.");

#define _DIFFLIB_SEQUENCEMATCHER_SET_SEQS_METHODDEF    \
    {"set_seqs", _PyCFunction_CAST(_difflib_SequenceMatcher_set_seqs), METH_FASTCALL, _difflib_SequenceMatcher_set_seqs__doc__},

static PyObject *
_difflib_SequenceMatcher_set_seqs_impl(SequenceMatcherObject *self,
                                       PyObject *a, PyObject *b);

static PyObject *
_difflib_SequenceMatcher_set_seqs(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *a;
    PyObject *b;

    if (!_PyArg_CheckPositional("set_seqs", nargs, 2, 2)) {
        goto exit;
    }
    a = args[0];
    b = args[1];
    return_value = _difflib_SequenceMatcher_set_seqs_impl((SequenceMatcherObject *)self, a, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_difflib_SequenceMatcher_find_longest_match__doc__,
"find_longest_match($self, /, alo=0, ahi=None, blo=0, bhi=None)\n"
"--\n"
"\n"
"Find longest matching block in a[alo:ahi] and b[blo:bhi].\n"
"\n"
"By default the entire sequences are searched.  Returns Match(i, j, k)\n"
"such that a[i:i+k] equals b[j:j+k].");

#define _DIFFLIB_SEQUENCEMATCHER_FIND_LONGEST_MATCH_METHODDEF    \
    {"find_longest_match", _PyCFunction_CAST(_difflib_SequenceMatcher_find_longest_match), METH_FASTCALL|METH_KEYWORDS, _difflib_SequenceMatcher_find_longest_match__doc__},

static PyObject *
_difflib_SequenceMatcher_find_longest_match_impl(SequenceMatcherObject *self,
                                                 Py_ssize_t alo,
                                                 PyObject *ahi,
                                                 Py_ssize_t blo,
                                                 PyObject *bhi);

static PyObject *
_difflib_SequenceMatcher_find_longest_match(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(alo), &_Py_ID(ahi), &_Py_ID(blo), &_Py_ID(bhi), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"alo", "ahi", "blo", "bhi", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "find_longest_match",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    Py_ssize_t alo = 0;
    PyObject *ahi = Py_None;
    Py_ssize_t blo = 0;
    PyObject *bhi = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[0]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            alo = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[1]) {
        ahi = args[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        {
            Py_ssize_t ival = -1;
            PyObject *iobj = _PyNumber_Index(args[2]);
            if (iobj != NULL) {
                ival = PyLong_AsSsize_t(iobj);
                Py_DECREF(iobj);
            }
            if (ival == -1 && PyErr_Occurred()) {
                goto exit;
            }
            blo = ival;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    bhi = args[3];
skip_optional_pos:
    return_value = _difflib_SequenceMatcher_find_longest_match_impl((SequenceMatcherObject *)self, alo, ahi, blo, bhi);

exit:
    return return_value;
}

PyDoc_STRVAR(_difflib_SequenceMatcher_get_matching_blocks__doc__,
"get_matching_blocks($self, /)\n"
"--\n"
"\n"
"Return list of triples describing matching subsequences.");

#define _DIFFLIB_SEQUENCEMATCHER_GET_MATCHING_BLOCKS_METHODDEF    \
    {"get_matching_blocks", (PyCFunction)_difflib_SequenceMatcher_get_matching_blocks, METH_NOARGS, _difflib_SequenceMatcher_get_matching_blocks__doc__},

static PyObject *
_difflib_SequenceMatcher_get_matching_blocks_impl(SequenceMatcherObject *self);

static PyObject *
_difflib_SequenceMatcher_get_matching_blocks(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _difflib_SequenceMatcher_get_matching_blocks_impl((SequenceMatcherObject *)self);
}

PyDoc_STRVAR(_difflib_SequenceMatcher_get_opcodes__doc__,
"get_opcodes($self, /)\n"
"--\n"
"\n"
"Return list of 5-tuples describing how to turn a into b.");

#define _DIFFLIB_SEQUENCEMATCHER_GET_OPCODES_METHODDEF    \
    {"get_opcodes", (PyCFunction)_difflib_SequenceMatcher_get_opcodes, METH_NOARGS, _difflib_SequenceMatcher_get_opcodes__doc__},

static PyObject *
_difflib_SequenceMatcher_get_opcodes_impl(SequenceMatcherObject *self);

static PyObject *
_difflib_SequenceMatcher_get_opcodes(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _difflib_SequenceMatcher_get_opcodes_impl((SequenceMatcherObject *)self);
}

PyDoc_STRVAR(_difflib_SequenceMatcher_ratio__doc__,
"ratio($self, /)\n"
"--\n"
"\n"
"Return a measure of the sequences\' similarity (float in [0, 1]).");

#define _DIFFLIB_SEQUENCEMATCHER_RATIO_METHODDEF    \
    {"ratio", (PyCFunction)_difflib_SequenceMatcher_ratio, METH_NOARGS, _difflib_SequenceMatcher_ratio__doc__},

static PyObject *
_difflib_SequenceMatcher_ratio_impl(SequenceMatcherObject *self);

static PyObject *
_difflib_SequenceMatcher_ratio(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _difflib_SequenceMatcher_ratio_impl((SequenceMatcherObject *)self);
}
/*[clinic end generated code: output=359d12bb49bcc3bd input=a9049054013a1b77]*/
