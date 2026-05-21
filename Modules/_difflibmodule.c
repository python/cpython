/*
 * _difflib accelerator module.
 *
 * Provides a fast C implementation of difflib.SequenceMatcher that is used
 * by Lib/difflib.py when available.  The algorithm mirrors the pure-Python
 * implementation exactly (output is bit-identical, including tie breaks);
 * the performance comes from operating on integer-label arrays rather than
 * Python objects in the inner DP and recursion loops.
 *
 * The implementation was built incrementally; comments throughout the file
 * use [phase N] tags to mark code that was added in each optimisation
 * step.  See the design discussion at
 *   https://discuss.python.org/t/<TODO-issue-link>
 * for benchmarks per phase.
 *
 * Phase summary:
 *   [phase 1]  C port of find_longest_match: paired j2len_val/j2len_ver
 *              int arrays + generation counter replace the per-row
 *              j2len = {} dict.  Lives in flm_core().
 *   [phase 2]  C port of chain_b: builds b2j without per-element
 *              setdefault/append.  Type-specialised iteration of b for
 *              str/list/tuple/bytes.  Lives in chain_b().
 *   [phase 3]  Full Ratcliff-Obershelp recursion in C: position-indexed
 *              int32 label arrays (a_lbl, a_dp, b_lbl, junk_mask) carry
 *              the work; DP and extension passes are pure C.  Lives in
 *              flm_core() (extension passes) and compute_matching_blocks().
 *   [phase 4]  Codepoint-keyed cp_full[] / cp_dp[] lookup tables for
 *              str.  Lives in chain_b() (table construction, str branch)
 *              and build_a_labels() (str fast path).
 *   [phase 5]  Bytes fast path (cp arrays of size 256), persistent DP
 *              scratch (j2len_val2/ver2, no per-call alloca/memset),
 *              skip the max-codepoint scan for UCS1 strings.  Lives in
 *              chain_b() (bytes branch + UCS1 shortcut) and flm_core()
 *              (persistent scratch).
 */

#define PY_SSIZE_T_CLEAN

// clinic/_difflibmodule.c.h uses internal pycore_modsupport.h API
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/*[clinic input]
module _difflib
class _difflib.SequenceMatcher "SequenceMatcherObject *" "clinic_state()->SequenceMatcher_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=ee484bc2f95ade86]*/


/* ====================================================================== */
/* Module state and per-instance state.                                    */
/* ====================================================================== */

typedef struct {
    PyTypeObject *SequenceMatcher_Type;
    PyObject *Match;       /* difflib.Match namedtuple */
} _difflib_state;

static inline _difflib_state *
get_module_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_difflib_state *)state;
}

static struct PyModuleDef _difflib_module;

typedef struct {
    PyObject_HEAD

    /* Public attributes (mirror difflib.SequenceMatcher). */
    PyObject *isjunk;        /* callable or None */
    PyObject *a;             /* current sequence a */
    PyObject *b;             /* current sequence b */
    PyObject *b2j;           /* dict: elt -> list[int] */
    PyObject *bjunk;         /* set */
    PyObject *bpopular;      /* set */
    PyObject *matching_blocks;  /* cached list[Match] or None */
    PyObject *opcodes;          /* cached list[tuple] or None */
    PyObject *fullbcount;       /* cached dict or None (quick_ratio) */
    int autojunk;

    /* Private C state. */
    Py_ssize_t la;
    Py_ssize_t lb;
    /* [phase 3] Integer-label arrays.  Every distinct element of b gets a
       "full label"; b2j-survivors also get a "DP label" (-1 otherwise).
       Position-indexed so the DP and extension passes never touch a
       PyObject in the hot loop. */
    Py_ssize_t nlbl;            /* number of DP labels */
    int32_t *a_lbl;             /* len la, full label or -1 */
    int32_t *a_dp;              /* len la, DP label or -1 */
    int32_t *b_lbl;             /* len lb, full label */
    int32_t *jbuf;              /* concatenated index lists per DP label */
    int32_t *jstart;            /* len nlbl */
    int32_t *jcount;            /* len nlbl */
    uint8_t *junk_mask;         /* len lb, 1 if b[i] in bjunk */
    PyObject *elt_to_lbl_full;
    PyObject *elt_to_lbl_dp;
    /* [phase 4/5] Codepoint-keyed lookup tables for str/bytes inputs.
       Phase 4 added them for str; phase 5 added the bytes branch.  NULL
       when neither applies. */
    int32_t *cp_full;           /* codepoint-keyed fast path (str/bytes) */
    int32_t *cp_dp;
    Py_ssize_t cp_max_plus1;
    /* [phase 1] j2len_val/ver pair: paired int + generation array
       replacing the per-row Python `j2len = {}` dict.
       [phase 5] j2len_val2/ver2: persistent second pair so flm_core
       no longer alloca()s + memset()s per call. */
    Py_ssize_t *j2len_val;
    Py_ssize_t *j2len_ver;
    Py_ssize_t *j2len_val2;
    Py_ssize_t *j2len_ver2;
    Py_ssize_t j2len_size;
    Py_ssize_t gen;

    int b_ready;                /* chain_b() has run for current b */
    int a_ready;                /* a labels built for current a */
} SequenceMatcherObject;

#include "clinic/_difflibmodule.c.h"


/* ====================================================================== */
/* Small helpers.                                                         */
/* ====================================================================== */

static void
free_b_state(SequenceMatcherObject *self)
{
    PyMem_Free(self->b_lbl);
    PyMem_Free(self->jbuf);
    PyMem_Free(self->jstart);
    PyMem_Free(self->jcount);
    PyMem_Free(self->junk_mask);
    PyMem_Free(self->cp_full);
    PyMem_Free(self->cp_dp);
    PyMem_Free(self->j2len_val);
    PyMem_Free(self->j2len_ver);
    PyMem_Free(self->j2len_val2);
    PyMem_Free(self->j2len_ver2);
    self->b_lbl = NULL;
    self->jbuf = NULL;
    self->jstart = NULL;
    self->jcount = NULL;
    self->junk_mask = NULL;
    self->cp_full = NULL;
    self->cp_dp = NULL;
    self->j2len_val = NULL;
    self->j2len_ver = NULL;
    self->j2len_val2 = NULL;
    self->j2len_ver2 = NULL;
    Py_CLEAR(self->elt_to_lbl_full);
    Py_CLEAR(self->elt_to_lbl_dp);
    self->lb = 0;
    self->nlbl = 0;
    self->cp_max_plus1 = 0;
    self->j2len_size = 0;
    self->gen = 0;
    self->b_ready = 0;
}

static void
free_a_state(SequenceMatcherObject *self)
{
    PyMem_Free(self->a_lbl);
    PyMem_Free(self->a_dp);
    self->a_lbl = NULL;
    self->a_dp = NULL;
    self->la = 0;
    self->a_ready = 0;
}

static void
invalidate_caches(SequenceMatcherObject *self)
{
    Py_CLEAR(self->matching_blocks);
    Py_CLEAR(self->opcodes);
}


/* ====================================================================== */
/* chain_b: build b2j, bjunk, bpopular, and integer-label state.           */
/*                                                                         */
/* [phase 2] This is the C port of SequenceMatcher.__chain_b().  The       */
/* per-element setdefault/append loop from Python is replaced by direct    */
/* dict access, with type-specialised reads for str/list/tuple/bytes so    */
/* we never go through PySequence_GetItem for those four common cases.     */
/*                                                                         */
/* [phase 3] After building b2j, we assign small integer labels: a "full   */
/* label" for every distinct element of b (used by the extension passes    */
/* via b_lbl[]) and a separate "DP label" for elements that survived       */
/* junk/popular pruning (used by the DP via a_dp[]/jbuf[]).                */
/*                                                                         */
/* [phase 4/5] If b is str or bytes, we also build codepoint-keyed         */
/* lookup tables cp_full[]/cp_dp[] so build_a_labels can skip dict probes  */
/* entirely.  See the cp_sz construction near the bottom of the function. */
/* ====================================================================== */

static int
chain_b(SequenceMatcherObject *self)
{
    PyObject *b2j = NULL;
    PyObject *bjunk = NULL;
    PyObject *bpopular = NULL;
    PyObject *elt_to_lbl_full = NULL;
    PyObject *elt_to_lbl_dp = NULL;
    PyObject *junk_keys = NULL;
    PyObject *pop_keys = NULL;
    PyObject *iter_keys = NULL;
    PyObject *items = NULL;
    int rc = -1;

    PyObject *b = self->b;
    Py_ssize_t lb = PyObject_Length(b);
    if (lb < 0) {
        goto done;
    }

    b2j = PyDict_New();
    bjunk = PySet_New(NULL);
    bpopular = PySet_New(NULL);
    if (b2j == NULL || bjunk == NULL || bpopular == NULL) {
        goto done;
    }

    int is_str = PyUnicode_Check(b);
    int is_list = PyList_Check(b);
    int is_tuple = PyTuple_Check(b);
    int is_bytes = PyBytes_Check(b);
    int kind = 0;
    const void *udata = NULL;
    const unsigned char *bdata = NULL;

    if (is_str) {
        if (PyUnicode_READY(b) < 0) {
            goto done;
        }
        kind = PyUnicode_KIND(b);
        udata = PyUnicode_DATA(b);
    }
    else if (is_bytes) {
        bdata = (const unsigned char *)PyBytes_AS_STRING(b);
    }

    /* Pass 1: build b2j (elt -> list of indices). */
    for (Py_ssize_t i = 0; i < lb; i++) {
        PyObject *elt;
        if (is_str) {
            elt = PyUnicode_FromOrdinal(PyUnicode_READ(kind, udata, i));
            if (elt == NULL) {
                goto done;
            }
        }
        else if (is_bytes) {
            elt = PyLong_FromLong(bdata[i]);
            if (elt == NULL) {
                goto done;
            }
        }
        else if (is_list) {
            elt = Py_NewRef(PyList_GET_ITEM(b, i));
        }
        else if (is_tuple) {
            elt = Py_NewRef(PyTuple_GET_ITEM(b, i));
        }
        else {
            elt = PySequence_GetItem(b, i);
            if (elt == NULL) {
                goto done;
            }
        }

        PyObject *lst = PyDict_GetItemWithError(b2j, elt);
        if (lst == NULL) {
            if (PyErr_Occurred()) {
                Py_DECREF(elt);
                goto done;
            }
            lst = PyList_New(0);
            if (lst == NULL) {
                Py_DECREF(elt);
                goto done;
            }
            if (PyDict_SetItem(b2j, elt, lst) < 0) {
                Py_DECREF(lst);
                Py_DECREF(elt);
                goto done;
            }
            Py_DECREF(lst);
            lst = PyDict_GetItemWithError(b2j, elt);
            if (lst == NULL) {
                Py_DECREF(elt);
                goto done;
            }
        }
        PyObject *idx = PyLong_FromSsize_t(i);
        if (idx == NULL || PyList_Append(lst, idx) < 0) {
            Py_XDECREF(idx);
            Py_DECREF(elt);
            goto done;
        }
        Py_DECREF(idx);
        Py_DECREF(elt);
    }

    /* Assign FULL labels to all distinct b elements; fill b_lbl. */
    elt_to_lbl_full = PyDict_New();
    if (elt_to_lbl_full == NULL) {
        goto done;
    }
    Py_ssize_t nfull = PyDict_GET_SIZE(b2j);
    int32_t *b_lbl = (int32_t *)PyMem_Malloc(sizeof(int32_t) * (size_t)lb);
    uint8_t *junk_mask = (uint8_t *)PyMem_Calloc((size_t)(lb > 0 ? lb : 1), 1);
    if (b_lbl == NULL || junk_mask == NULL) {
        PyMem_Free(b_lbl);
        PyMem_Free(junk_mask);
        PyErr_NoMemory();
        goto done;
    }
    {
        PyObject *k, *v;
        Py_ssize_t pos = 0;
        int32_t lbl = 0;
        while (PyDict_Next(b2j, &pos, &k, &v)) {
            PyObject *lbl_obj = PyLong_FromLong(lbl);
            if (lbl_obj == NULL) {
                PyMem_Free(b_lbl);
                PyMem_Free(junk_mask);
                goto done;
            }
            if (PyDict_SetItem(elt_to_lbl_full, k, lbl_obj) < 0) {
                Py_DECREF(lbl_obj);
                PyMem_Free(b_lbl);
                PyMem_Free(junk_mask);
                goto done;
            }
            Py_DECREF(lbl_obj);
            Py_ssize_t n = PyList_GET_SIZE(v);
            for (Py_ssize_t i = 0; i < n; i++) {
                Py_ssize_t bi = PyLong_AsSsize_t(PyList_GET_ITEM(v, i));
                if (bi == -1 && PyErr_Occurred()) {
                    PyMem_Free(b_lbl);
                    PyMem_Free(junk_mask);
                    goto done;
                }
                b_lbl[bi] = lbl;
            }
            lbl++;
        }
    }
    (void)nfull;

    /* Apply isjunk callback (if any) and mark junk_mask. */
    if (self->isjunk != Py_None) {
        junk_keys = PyList_New(0);
        if (junk_keys == NULL) {
            PyMem_Free(b_lbl);
            PyMem_Free(junk_mask);
            goto done;
        }
        iter_keys = PyDict_Keys(b2j);
        if (iter_keys == NULL) {
            PyMem_Free(b_lbl);
            PyMem_Free(junk_mask);
            goto done;
        }
        Py_ssize_t nk = PyList_GET_SIZE(iter_keys);
        for (Py_ssize_t i = 0; i < nk; i++) {
            PyObject *k = PyList_GET_ITEM(iter_keys, i);
            PyObject *res = PyObject_CallOneArg(self->isjunk, k);
            if (res == NULL) {
                PyMem_Free(b_lbl);
                PyMem_Free(junk_mask);
                goto done;
            }
            int truthy = PyObject_IsTrue(res);
            Py_DECREF(res);
            if (truthy < 0) {
                PyMem_Free(b_lbl);
                PyMem_Free(junk_mask);
                goto done;
            }
            if (truthy) {
                if (PySet_Add(bjunk, k) < 0
                    || PyList_Append(junk_keys, k) < 0)
                {
                    PyMem_Free(b_lbl);
                    PyMem_Free(junk_mask);
                    goto done;
                }
            }
        }
        Py_ssize_t njk = PyList_GET_SIZE(junk_keys);
        for (Py_ssize_t i = 0; i < njk; i++) {
            PyObject *k = PyList_GET_ITEM(junk_keys, i);
            PyObject *lbl_obj = PyDict_GetItemWithError(elt_to_lbl_full, k);
            if (lbl_obj == NULL) {
                PyMem_Free(b_lbl);
                PyMem_Free(junk_mask);
                goto done;
            }
            long lbl = PyLong_AsLong(lbl_obj);
            for (Py_ssize_t bi = 0; bi < lb; bi++) {
                if (b_lbl[bi] == (int32_t)lbl) {
                    junk_mask[bi] = 1;
                }
            }
            if (PyDict_DelItem(b2j, k) < 0) {
                PyMem_Free(b_lbl);
                PyMem_Free(junk_mask);
                goto done;
            }
        }
    }

    /* Autojunk popular elements (matches Lib/difflib.py semantics). */
    if (self->autojunk && lb >= 200) {
        Py_ssize_t ntest = lb / 100 + 1;
        pop_keys = PyList_New(0);
        items = PyDict_Items(b2j);
        if (pop_keys == NULL || items == NULL) {
            PyMem_Free(b_lbl);
            PyMem_Free(junk_mask);
            goto done;
        }
        Py_ssize_t ni = PyList_GET_SIZE(items);
        for (Py_ssize_t i = 0; i < ni; i++) {
            PyObject *kv = PyList_GET_ITEM(items, i);
            PyObject *k = PyTuple_GET_ITEM(kv, 0);
            PyObject *v = PyTuple_GET_ITEM(kv, 1);
            if (PyList_GET_SIZE(v) > ntest) {
                if (PySet_Add(bpopular, k) < 0
                    || PyList_Append(pop_keys, k) < 0)
                {
                    PyMem_Free(b_lbl);
                    PyMem_Free(junk_mask);
                    goto done;
                }
            }
        }
        Py_ssize_t npk = PyList_GET_SIZE(pop_keys);
        for (Py_ssize_t i = 0; i < npk; i++) {
            if (PyDict_DelItem(b2j, PyList_GET_ITEM(pop_keys, i)) < 0) {
                PyMem_Free(b_lbl);
                PyMem_Free(junk_mask);
                goto done;
            }
        }
    }

    /* Build DP labels + jbuf from post-junk b2j. */
    elt_to_lbl_dp = PyDict_New();
    if (elt_to_lbl_dp == NULL) {
        PyMem_Free(b_lbl);
        PyMem_Free(junk_mask);
        goto done;
    }
    Py_ssize_t nlbl = PyDict_GET_SIZE(b2j);
    Py_ssize_t total = 0;
    {
        PyObject *k, *v;
        Py_ssize_t pos = 0;
        while (PyDict_Next(b2j, &pos, &k, &v)) {
            total += PyList_GET_SIZE(v);
        }
    }
    int32_t *jbuf = (int32_t *)PyMem_Malloc(
        sizeof(int32_t) * (size_t)(total > 0 ? total : 1));
    int32_t *jstart = (int32_t *)PyMem_Malloc(
        sizeof(int32_t) * (size_t)(nlbl > 0 ? nlbl : 1));
    int32_t *jcount = (int32_t *)PyMem_Malloc(
        sizeof(int32_t) * (size_t)(nlbl > 0 ? nlbl : 1));
    if (jbuf == NULL || jstart == NULL || jcount == NULL) {
        PyMem_Free(jbuf);
        PyMem_Free(jstart);
        PyMem_Free(jcount);
        PyMem_Free(b_lbl);
        PyMem_Free(junk_mask);
        PyErr_NoMemory();
        goto done;
    }
    {
        PyObject *k, *v;
        Py_ssize_t pos = 0;
        int32_t lbl = 0;
        int32_t cursor = 0;
        while (PyDict_Next(b2j, &pos, &k, &v)) {
            Py_ssize_t n = PyList_GET_SIZE(v);
            jstart[lbl] = cursor;
            jcount[lbl] = (int32_t)n;
            for (Py_ssize_t i = 0; i < n; i++) {
                Py_ssize_t bi = PyLong_AsSsize_t(PyList_GET_ITEM(v, i));
                if (bi == -1 && PyErr_Occurred()) {
                    PyMem_Free(jbuf);
                    PyMem_Free(jstart);
                    PyMem_Free(jcount);
                    PyMem_Free(b_lbl);
                    PyMem_Free(junk_mask);
                    goto done;
                }
                jbuf[cursor + i] = (int32_t)bi;
            }
            cursor += (int32_t)n;
            PyObject *lbl_obj = PyLong_FromLong(lbl);
            if (lbl_obj == NULL) {
                PyMem_Free(jbuf);
                PyMem_Free(jstart);
                PyMem_Free(jcount);
                PyMem_Free(b_lbl);
                PyMem_Free(junk_mask);
                goto done;
            }
            if (PyDict_SetItem(elt_to_lbl_dp, k, lbl_obj) < 0) {
                Py_DECREF(lbl_obj);
                PyMem_Free(jbuf);
                PyMem_Free(jstart);
                PyMem_Free(jcount);
                PyMem_Free(b_lbl);
                PyMem_Free(junk_mask);
                goto done;
            }
            Py_DECREF(lbl_obj);
            lbl++;
        }
    }

    /* DP scratch (two versioned pairs). */
    Py_ssize_t scratch_sz = lb + 1;
    Py_ssize_t *jv = (Py_ssize_t *)PyMem_Malloc(
        sizeof(Py_ssize_t) * (size_t)scratch_sz);
    Py_ssize_t *jver = (Py_ssize_t *)PyMem_Calloc(
        (size_t)scratch_sz, sizeof(Py_ssize_t));
    Py_ssize_t *jv2 = (Py_ssize_t *)PyMem_Malloc(
        sizeof(Py_ssize_t) * (size_t)scratch_sz);
    Py_ssize_t *jver2 = (Py_ssize_t *)PyMem_Calloc(
        (size_t)scratch_sz, sizeof(Py_ssize_t));
    if (jv == NULL || jver == NULL || jv2 == NULL || jver2 == NULL) {
        PyMem_Free(jv);
        PyMem_Free(jver);
        PyMem_Free(jv2);
        PyMem_Free(jver2);
        PyMem_Free(jbuf);
        PyMem_Free(jstart);
        PyMem_Free(jcount);
        PyMem_Free(b_lbl);
        PyMem_Free(junk_mask);
        PyErr_NoMemory();
        goto done;
    }

    /* [phase 4/5] Codepoint-keyed fast path.
       For str/bytes b we build cp_full[ch] -> full label and
       cp_dp[ch] -> DP label, indexed directly by the codepoint.
       build_a_labels can then skip the PyUnicode_FromOrdinal /
       PyLong_FromLong + dict-probe per element of a.
         - [phase 4] str branch.
         - [phase 5] bytes branch (cp_sz fixed at 256) and the UCS1
           shortcut (cp_sz fixed at 256 instead of scanning to find
           the max codepoint).
    */
    int32_t *cp_full = NULL;
    int32_t *cp_dp = NULL;
    Py_ssize_t cp_sz = 0;
    if (is_str) {
        if (kind == PyUnicode_1BYTE_KIND) {
            cp_sz = 256;
        }
        else {
            Py_UCS4 maxch = 0;
            for (Py_ssize_t i = 0; i < lb; i++) {
                Py_UCS4 c = PyUnicode_READ(kind, udata, i);
                if (c > maxch) {
                    maxch = c;
                }
            }
            cp_sz = (Py_ssize_t)maxch + 1;
        }
    }
    else if (is_bytes) {
        cp_sz = 256;
    }
    if (cp_sz > 0) {
        cp_full = (int32_t *)PyMem_Malloc(sizeof(int32_t) * (size_t)cp_sz);
        cp_dp = (int32_t *)PyMem_Malloc(sizeof(int32_t) * (size_t)cp_sz);
        if (cp_full == NULL || cp_dp == NULL) {
            PyMem_Free(cp_full);
            PyMem_Free(cp_dp);
            PyMem_Free(jv);
            PyMem_Free(jver);
            PyMem_Free(jv2);
            PyMem_Free(jver2);
            PyMem_Free(jbuf);
            PyMem_Free(jstart);
            PyMem_Free(jcount);
            PyMem_Free(b_lbl);
            PyMem_Free(junk_mask);
            PyErr_NoMemory();
            goto done;
        }
        for (Py_ssize_t i = 0; i < cp_sz; i++) {
            cp_full[i] = -1;
            cp_dp[i] = -1;
        }
        if (is_str) {
            for (Py_ssize_t i = 0; i < lb; i++) {
                Py_UCS4 c = PyUnicode_READ(kind, udata, i);
                cp_full[c] = b_lbl[i];
            }
        }
        else {
            for (Py_ssize_t i = 0; i < lb; i++) {
                cp_full[bdata[i]] = b_lbl[i];
            }
        }
        PyObject *k, *v;
        Py_ssize_t pos = 0;
        while (PyDict_Next(elt_to_lbl_dp, &pos, &k, &v)) {
            long dp = PyLong_AsLong(v);
            if (dp == -1 && PyErr_Occurred()) {
                PyMem_Free(cp_full);
                PyMem_Free(cp_dp);
                PyMem_Free(jv);
                PyMem_Free(jver);
                PyMem_Free(jv2);
                PyMem_Free(jver2);
                PyMem_Free(jbuf);
                PyMem_Free(jstart);
                PyMem_Free(jcount);
                PyMem_Free(b_lbl);
                PyMem_Free(junk_mask);
                goto done;
            }
            Py_ssize_t cp_idx;
            if (is_str) {
                if (!PyUnicode_Check(k) || PyUnicode_GET_LENGTH(k) != 1) {
                    continue;
                }
                cp_idx = (Py_ssize_t)PyUnicode_READ_CHAR(k, 0);
            }
            else {
                if (!PyLong_Check(k)) {
                    continue;
                }
                long c = PyLong_AsLong(k);
                if (c < 0 || c >= cp_sz) {
                    continue;
                }
                cp_idx = c;
            }
            if (cp_idx < cp_sz) {
                cp_dp[cp_idx] = (int32_t)dp;
            }
        }
    }

    /* Commit. */
    free_b_state(self);
    self->lb = lb;
    self->nlbl = nlbl;
    self->b_lbl = b_lbl;
    self->jbuf = jbuf;
    self->jstart = jstart;
    self->jcount = jcount;
    self->junk_mask = junk_mask;
    self->cp_full = cp_full;
    self->cp_dp = cp_dp;
    self->cp_max_plus1 = cp_sz;
    self->j2len_val = jv;
    self->j2len_ver = jver;
    self->j2len_val2 = jv2;
    self->j2len_ver2 = jver2;
    self->j2len_size = scratch_sz;
    self->gen = 0;
    self->elt_to_lbl_full = elt_to_lbl_full;
    self->elt_to_lbl_dp = elt_to_lbl_dp;
    elt_to_lbl_full = NULL;
    elt_to_lbl_dp = NULL;

    Py_XSETREF(self->b2j, Py_NewRef(b2j));
    Py_XSETREF(self->bjunk, Py_NewRef(bjunk));
    Py_XSETREF(self->bpopular, Py_NewRef(bpopular));
    self->b_ready = 1;
    self->a_ready = 0;
    rc = 0;

done:
    Py_XDECREF(b2j);
    Py_XDECREF(bjunk);
    Py_XDECREF(bpopular);
    Py_XDECREF(elt_to_lbl_full);
    Py_XDECREF(elt_to_lbl_dp);
    Py_XDECREF(junk_keys);
    Py_XDECREF(pop_keys);
    Py_XDECREF(iter_keys);
    Py_XDECREF(items);
    return rc;
}


/* ====================================================================== */
/* build_a_labels: assign full and DP labels to each position of a.        */
/*                                                                         */
/* [phase 3] General path (any sequence type): walk a, look each element  */
/* up in elt_to_lbl_full / elt_to_lbl_dp, store the resulting int32       */
/* labels in a_lbl[] and a_dp[].                                           */
/*                                                                         */
/* [phase 4/5] str / bytes fast paths use the codepoint-keyed cp_full[]   */
/* and cp_dp[] tables built by chain_b(), so the per-position element     */
/* reconstruction (PyUnicode_FromOrdinal / PyLong_FromLong) and dict       */
/* probe go away entirely.                                                 */
/* ====================================================================== */

static int
build_a_labels(SequenceMatcherObject *self)
{
    PyObject *a = self->a;
    Py_ssize_t la = PyObject_Length(a);
    if (la < 0) {
        return -1;
    }

    int32_t *a_lbl = (int32_t *)PyMem_Malloc(
        sizeof(int32_t) * (size_t)(la > 0 ? la : 1));
    int32_t *a_dp = (int32_t *)PyMem_Malloc(
        sizeof(int32_t) * (size_t)(la > 0 ? la : 1));
    if (a_lbl == NULL || a_dp == NULL) {
        PyMem_Free(a_lbl);
        PyMem_Free(a_dp);
        PyErr_NoMemory();
        return -1;
    }

    int is_str = PyUnicode_Check(a);
    int is_list = PyList_Check(a);
    int is_tuple = PyTuple_Check(a);
    int is_bytes = PyBytes_Check(a);
    int kind = 0;
    const void *udata = NULL;
    const unsigned char *adata = NULL;

    if (is_str) {
        if (PyUnicode_READY(a) < 0) {
            PyMem_Free(a_lbl);
            PyMem_Free(a_dp);
            return -1;
        }
        kind = PyUnicode_KIND(a);
        udata = PyUnicode_DATA(a);
    }
    else if (is_bytes) {
        adata = (const unsigned char *)PyBytes_AS_STRING(a);
    }

    /* [phase 4] Fast path for str a paired with str b.  cp_full / cp_dp
       were built once in chain_b(); each position of a turns into two
       array indexings, no PyObject construction or dict lookup. */
    if (is_str && self->cp_full != NULL) {
        Py_ssize_t cpmax = self->cp_max_plus1;
        const int32_t *cf = self->cp_full;
        const int32_t *cd = self->cp_dp;
        for (Py_ssize_t i = 0; i < la; i++) {
            Py_UCS4 c = PyUnicode_READ(kind, udata, i);
            if ((Py_ssize_t)c < cpmax) {
                a_lbl[i] = cf[c];
                a_dp[i] = cd[c];
            }
            else {
                a_lbl[i] = -1;
                a_dp[i] = -1;
            }
        }
    }
    /* [phase 5] Fast path for bytes a paired with bytes b. */
    else if (is_bytes && self->cp_full != NULL) {
        const int32_t *cf = self->cp_full;
        const int32_t *cd = self->cp_dp;
        for (Py_ssize_t i = 0; i < la; i++) {
            a_lbl[i] = cf[adata[i]];
            a_dp[i] = cd[adata[i]];
        }
    }
    else {
        for (Py_ssize_t i = 0; i < la; i++) {
            PyObject *elt;
            if (is_str) {
                elt = PyUnicode_FromOrdinal(PyUnicode_READ(kind, udata, i));
                if (elt == NULL) {
                    goto error;
                }
            }
            else if (is_bytes) {
                elt = PyLong_FromLong(adata[i]);
                if (elt == NULL) {
                    goto error;
                }
            }
            else if (is_list) {
                elt = Py_NewRef(PyList_GET_ITEM(a, i));
            }
            else if (is_tuple) {
                elt = Py_NewRef(PyTuple_GET_ITEM(a, i));
            }
            else {
                elt = PySequence_GetItem(a, i);
                if (elt == NULL) {
                    goto error;
                }
            }

            PyObject *vf = PyDict_GetItemWithError(self->elt_to_lbl_full, elt);
            if (vf == NULL) {
                if (PyErr_Occurred()) {
                    Py_DECREF(elt);
                    goto error;
                }
                a_lbl[i] = -1;
            }
            else {
                a_lbl[i] = (int32_t)PyLong_AsLong(vf);
            }
            PyObject *vd = PyDict_GetItemWithError(self->elt_to_lbl_dp, elt);
            if (vd == NULL) {
                if (PyErr_Occurred()) {
                    Py_DECREF(elt);
                    goto error;
                }
                a_dp[i] = -1;
            }
            else {
                a_dp[i] = (int32_t)PyLong_AsLong(vd);
            }
            Py_DECREF(elt);
        }
    }

    free_a_state(self);
    self->a_lbl = a_lbl;
    self->a_dp = a_dp;
    self->la = la;
    self->a_ready = 1;
    return 0;

error:
    PyMem_Free(a_lbl);
    PyMem_Free(a_dp);
    return -1;
}


/* ====================================================================== */
/* ensure_ready: lazily run chain_b()/build_a_labels() as needed.         */
/* ====================================================================== */

static int
ensure_ready(SequenceMatcherObject *self)
{
    if (!self->b_ready) {
        if (chain_b(self) < 0) {
            return -1;
        }
    }
    if (!self->a_ready) {
        if (build_a_labels(self) < 0) {
            return -1;
        }
    }
    return 0;
}


/* ====================================================================== */
/* find_longest_match (core): pure-C DP on integer-label arrays.          */
/*                                                                         */
/* [phase 1] The j2len mapping that pure-Python builds with                */
/*     newj2len = {}                                                       */
/*     ... newj2len[j] = j2lenget(j-1, 0) + 1                              */
/* is replaced by two paired int arrays (val + ver) and a generation       */
/* counter.  `cur_ver[j-1] == cur_gen` means the previous row has a real   */
/* value at j-1, so no per-row clear is needed.                            */
/*                                                                         */
/* [phase 3] The DP itself works on integer DP labels (a_dp[]), and the    */
/* extension passes compare a_lbl[i] / b_lbl[j] (also int32) instead of    */
/* a[i] / b[j] via PyObject_RichCompareBool.                               */
/*                                                                         */
/* [phase 5] cur_val/cur_ver and nxt_val/nxt_ver live on the instance      */
/* (j2len_val/ver and j2len_val2/ver2), so no alloca/memset per call.     */
/* ====================================================================== */

static void
flm_core(SequenceMatcherObject *self,
         Py_ssize_t alo, Py_ssize_t ahi,
         Py_ssize_t blo, Py_ssize_t bhi,
         Py_ssize_t *out_i, Py_ssize_t *out_j, Py_ssize_t *out_k)
{
    Py_ssize_t besti = alo;
    Py_ssize_t bestj = blo;
    Py_ssize_t bestsize = 0;

    const int32_t *A = self->a_dp;
    const int32_t *JB = self->jbuf;
    const int32_t *JS = self->jstart;
    const int32_t *JC = self->jcount;
    const int32_t *AF = self->a_lbl;
    const int32_t *BF = self->b_lbl;
    const uint8_t *JM = self->junk_mask;
    Py_ssize_t nlbl = self->nlbl;

    Py_ssize_t gen = self->gen;
    Py_ssize_t *cur_val = self->j2len_val;
    Py_ssize_t *cur_ver = self->j2len_ver;
    Py_ssize_t *nxt_val = self->j2len_val2;
    Py_ssize_t *nxt_ver = self->j2len_ver2;
    Py_ssize_t cur_gen = ++gen;
    Py_ssize_t nxt_gen = ++gen;

    for (Py_ssize_t i = alo; i < ahi; i++) {
        int32_t lab = A[i];
        if (lab < 0 || lab >= nlbl) {
            Py_ssize_t *tv = cur_val;
            cur_val = nxt_val;
            nxt_val = tv;
            Py_ssize_t *tr = cur_ver;
            cur_ver = nxt_ver;
            nxt_ver = tr;
            cur_gen = nxt_gen;
            nxt_gen = ++gen;
            continue;
        }
        Py_ssize_t start = JS[lab];
        Py_ssize_t n = JC[lab];
        const int32_t *L = JB + start;
        Py_ssize_t k0 = 0;
        while (k0 < n && L[k0] < blo) {
            k0++;
        }
        for (Py_ssize_t idx = k0; idx < n; idx++) {
            Py_ssize_t j = L[idx];
            if (j >= bhi) {
                break;
            }
            Py_ssize_t prev = 0;
            if (j > 0 && cur_ver[j - 1] == cur_gen) {
                prev = cur_val[j - 1];
            }
            Py_ssize_t k = prev + 1;
            nxt_val[j] = k;
            nxt_ver[j] = nxt_gen;
            if (k > bestsize) {
                besti = i - k + 1;
                bestj = j - k + 1;
                bestsize = k;
            }
        }
        Py_ssize_t *tv = cur_val;
        cur_val = nxt_val;
        nxt_val = tv;
        Py_ssize_t *tr = cur_ver;
        cur_ver = nxt_ver;
        nxt_ver = tr;
        cur_gen = nxt_gen;
        nxt_gen = ++gen;
    }
    self->gen = gen;

    /* Extension passes on integer labels. */
    while (besti > alo && bestj > blo) {
        Py_ssize_t bj = bestj - 1;
        if (JM[bj] || AF[besti - 1] < 0 || AF[besti - 1] != BF[bj]) {
            break;
        }
        besti--;
        bestj--;
        bestsize++;
    }
    while (besti + bestsize < ahi && bestj + bestsize < bhi) {
        Py_ssize_t bj = bestj + bestsize;
        if (JM[bj] || AF[besti + bestsize] < 0
            || AF[besti + bestsize] != BF[bj])
        {
            break;
        }
        bestsize++;
    }
    while (besti > alo && bestj > blo) {
        Py_ssize_t bj = bestj - 1;
        if (!JM[bj] || AF[besti - 1] < 0 || AF[besti - 1] != BF[bj]) {
            break;
        }
        besti--;
        bestj--;
        bestsize++;
    }
    while (besti + bestsize < ahi && bestj + bestsize < bhi) {
        Py_ssize_t bj = bestj + bestsize;
        if (!JM[bj] || AF[besti + bestsize] < 0
            || AF[besti + bestsize] != BF[bj])
        {
            break;
        }
        bestsize++;
    }

    *out_i = besti;
    *out_j = bestj;
    *out_k = bestsize;
}


/* ====================================================================== */
/* matching_blocks recursion (in C).                                      */
/*                                                                         */
/* [phase 3] The full Ratcliff-Obershelp recursion lives here.  The        */
/* pure-Python equivalent does:                                            */
/*     queue = [(0, la, 0, lb)]                                            */
/*     while queue:                                                        */
/*         alo, ahi, blo, bhi = queue.pop()                                */
/*         i, j, k = self.find_longest_match(alo, ahi, blo, bhi)           */
/*         ...                                                             */
/* and crosses the Python/C boundary on every recursive call.  This        */
/* function runs the whole queue + flm_core() + sort + collapse loop in    */
/* one C call, so per-recursion overhead disappears.                       */
/* ====================================================================== */

typedef struct {
    Py_ssize_t alo, ahi, blo, bhi;
} range_t;

typedef struct {
    Py_ssize_t i, j, k;
} triple_t;

static int
triple_compare(const void *a, const void *b)
{
    const triple_t *x = (const triple_t *)a;
    const triple_t *y = (const triple_t *)b;
    if (x->i != y->i) {
        return x->i < y->i ? -1 : 1;
    }
    if (x->j != y->j) {
        return x->j < y->j ? -1 : 1;
    }
    if (x->k != y->k) {
        return x->k < y->k ? -1 : 1;
    }
    return 0;
}

static int
compute_matching_blocks(SequenceMatcherObject *self,
                        triple_t **out_blocks, Py_ssize_t *out_n)
{
    Py_ssize_t qcap = 64;
    range_t *queue = (range_t *)PyMem_Malloc(sizeof(range_t) * (size_t)qcap);
    Py_ssize_t mcap = 64;
    triple_t *matches = (triple_t *)PyMem_Malloc(
        sizeof(triple_t) * (size_t)mcap);
    triple_t *collapsed = NULL;
    if (queue == NULL || matches == NULL) {
        PyMem_Free(queue);
        PyMem_Free(matches);
        PyErr_NoMemory();
        return -1;
    }
    Py_ssize_t qn = 0;
    Py_ssize_t mn = 0;

    range_t r0;
    r0.alo = 0;
    r0.ahi = self->la;
    r0.blo = 0;
    r0.bhi = self->lb;
    queue[qn++] = r0;

    while (qn > 0) {
        range_t r = queue[--qn];
        Py_ssize_t i, j, k;
        flm_core(self, r.alo, r.ahi, r.blo, r.bhi, &i, &j, &k);
        if (k == 0) {
            continue;
        }
        if (mn >= mcap) {
            mcap *= 2;
            triple_t *nm = (triple_t *)PyMem_Realloc(
                matches, sizeof(triple_t) * (size_t)mcap);
            if (nm == NULL) {
                PyMem_Free(queue);
                PyMem_Free(matches);
                PyErr_NoMemory();
                return -1;
            }
            matches = nm;
        }
        triple_t t;
        t.i = i;
        t.j = j;
        t.k = k;
        matches[mn++] = t;
        if (r.alo < i && r.blo < j) {
            if (qn >= qcap) {
                qcap *= 2;
                range_t *nq = (range_t *)PyMem_Realloc(
                    queue, sizeof(range_t) * (size_t)qcap);
                if (nq == NULL) {
                    PyMem_Free(queue);
                    PyMem_Free(matches);
                    PyErr_NoMemory();
                    return -1;
                }
                queue = nq;
            }
            range_t r2;
            r2.alo = r.alo;
            r2.ahi = i;
            r2.blo = r.blo;
            r2.bhi = j;
            queue[qn++] = r2;
        }
        if (i + k < r.ahi && j + k < r.bhi) {
            if (qn >= qcap) {
                qcap *= 2;
                range_t *nq = (range_t *)PyMem_Realloc(
                    queue, sizeof(range_t) * (size_t)qcap);
                if (nq == NULL) {
                    PyMem_Free(queue);
                    PyMem_Free(matches);
                    PyErr_NoMemory();
                    return -1;
                }
                queue = nq;
            }
            range_t r2;
            r2.alo = i + k;
            r2.ahi = r.ahi;
            r2.blo = j + k;
            r2.bhi = r.bhi;
            queue[qn++] = r2;
        }
    }
    PyMem_Free(queue);

    qsort(matches, (size_t)mn, sizeof(triple_t), triple_compare);

    collapsed = (triple_t *)PyMem_Malloc(sizeof(triple_t) * (size_t)(mn + 1));
    if (collapsed == NULL) {
        PyMem_Free(matches);
        PyErr_NoMemory();
        return -1;
    }
    Py_ssize_t on = 0;
    Py_ssize_t i1 = 0, j1 = 0, k1 = 0;
    for (Py_ssize_t idx = 0; idx < mn; idx++) {
        Py_ssize_t i2 = matches[idx].i;
        Py_ssize_t j2 = matches[idx].j;
        Py_ssize_t k2 = matches[idx].k;
        if (i1 + k1 == i2 && j1 + k1 == j2) {
            k1 += k2;
        }
        else {
            if (k1) {
                triple_t t;
                t.i = i1;
                t.j = j1;
                t.k = k1;
                collapsed[on++] = t;
            }
            i1 = i2;
            j1 = j2;
            k1 = k2;
        }
    }
    if (k1) {
        triple_t t;
        t.i = i1;
        t.j = j1;
        t.k = k1;
        collapsed[on++] = t;
    }
    triple_t sentinel;
    sentinel.i = self->la;
    sentinel.j = self->lb;
    sentinel.k = 0;
    collapsed[on++] = sentinel;
    PyMem_Free(matches);

    *out_blocks = collapsed;
    *out_n = on;
    return 0;
}


/* ====================================================================== */
/* Helpers for building Match namedtuples.                                 */
/* ====================================================================== */

static PyObject *
build_match(PyObject *module, Py_ssize_t i, Py_ssize_t j, Py_ssize_t k)
{
    _difflib_state *state = get_module_state(module);
    return PyObject_CallFunction(state->Match, "nnn", i, j, k);
}

static PyObject *
module_of(SequenceMatcherObject *self)
{
    return PyType_GetModuleByDef(Py_TYPE(self), &_difflib_module);
}


/* ====================================================================== */
/* Method implementations.                                                */
/* ====================================================================== */

/*[clinic input]
_difflib.SequenceMatcher.__init__

    isjunk: object = None
    a: object(c_default="NULL") = ''
    b: object(c_default="NULL") = ''
    autojunk: bool = True

Construct a SequenceMatcher.

See difflib.py for the full documentation; output is identical to the
pure-Python SequenceMatcher class.
[clinic start generated code]*/

static int
_difflib_SequenceMatcher___init___impl(SequenceMatcherObject *self,
                                       PyObject *isjunk, PyObject *a,
                                       PyObject *b, int autojunk)
/*[clinic end generated code: output=0d5ef8814b30159b input=aab0c2f4f8a063b4]*/
{
    Py_INCREF(isjunk);
    Py_XSETREF(self->isjunk, isjunk);
    self->autojunk = autojunk;

    PyObject *empty = PyUnicode_FromStringAndSize(NULL, 0);
    if (empty == NULL) {
        return -1;
    }
    PyObject *a_val = (a == NULL) ? empty : a;
    PyObject *b_val = (b == NULL) ? empty : b;

    Py_XSETREF(self->a, Py_NewRef(a_val));
    Py_XSETREF(self->b, Py_NewRef(b_val));
    Py_DECREF(empty);

    self->b_ready = 0;
    self->a_ready = 0;
    invalidate_caches(self);
    Py_XSETREF(self->fullbcount, Py_NewRef(Py_None));

    /* Eager: match pure-Python semantics where b2j/bjunk/bpopular are
       populated immediately so that they are visible as attributes
       right after construction. */
    if (chain_b(self) < 0) {
        return -1;
    }
    return 0;
}

/*[clinic input]
_difflib.SequenceMatcher.set_seq1

    a: object
    /

Set the first sequence to be compared.

The second sequence to be compared is not changed.
[clinic start generated code]*/

static PyObject *
_difflib_SequenceMatcher_set_seq1_impl(SequenceMatcherObject *self,
                                       PyObject *a)
/*[clinic end generated code: output=d7bd77eb821dd8b8 input=9445bdbeb31d0bf2]*/
{
    if (a == self->a) {
        Py_RETURN_NONE;
    }
    Py_XSETREF(self->a, Py_NewRef(a));
    self->a_ready = 0;
    invalidate_caches(self);
    Py_RETURN_NONE;
}

/*[clinic input]
_difflib.SequenceMatcher.set_seq2

    b: object
    /

Set the second sequence to be compared.

The first sequence to be compared is not changed.
[clinic start generated code]*/

static PyObject *
_difflib_SequenceMatcher_set_seq2_impl(SequenceMatcherObject *self,
                                       PyObject *b)
/*[clinic end generated code: output=1c21f4e4b95dfad8 input=8a4295ec082859be]*/
{
    if (b == self->b) {
        Py_RETURN_NONE;
    }
    Py_XSETREF(self->b, Py_NewRef(b));
    self->b_ready = 0;
    self->a_ready = 0;
    invalidate_caches(self);
    Py_XSETREF(self->fullbcount, Py_NewRef(Py_None));
    if (chain_b(self) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
_difflib.SequenceMatcher.set_seqs

    a: object
    b: object
    /

Set the two sequences to be compared.
[clinic start generated code]*/

static PyObject *
_difflib_SequenceMatcher_set_seqs_impl(SequenceMatcherObject *self,
                                       PyObject *a, PyObject *b)
/*[clinic end generated code: output=6125de76c8b14cda input=d045d9c013583673]*/
{
    PyObject *r = _difflib_SequenceMatcher_set_seq1_impl(self, a);
    if (r == NULL) {
        return NULL;
    }
    Py_DECREF(r);
    return _difflib_SequenceMatcher_set_seq2_impl(self, b);
}

/*[clinic input]
_difflib.SequenceMatcher.find_longest_match

    alo: Py_ssize_t = 0
    ahi: object = None
    blo: Py_ssize_t = 0
    bhi: object = None

Find longest matching block in a[alo:ahi] and b[blo:bhi].

By default the entire sequences are searched.  Returns Match(i, j, k)
such that a[i:i+k] equals b[j:j+k].
[clinic start generated code]*/

static PyObject *
_difflib_SequenceMatcher_find_longest_match_impl(SequenceMatcherObject *self,
                                                 Py_ssize_t alo,
                                                 PyObject *ahi,
                                                 Py_ssize_t blo,
                                                 PyObject *bhi)
/*[clinic end generated code: output=1650f5386c4d5669 input=849e78330a319475]*/
{
    if (ensure_ready(self) < 0) {
        return NULL;
    }
    Py_ssize_t ahi_n, bhi_n;
    if (ahi == Py_None) {
        ahi_n = self->la;
    }
    else {
        ahi_n = PyNumber_AsSsize_t(ahi, PyExc_OverflowError);
        if (ahi_n == -1 && PyErr_Occurred()) {
            return NULL;
        }
    }
    if (bhi == Py_None) {
        bhi_n = self->lb;
    }
    else {
        bhi_n = PyNumber_AsSsize_t(bhi, PyExc_OverflowError);
        if (bhi_n == -1 && PyErr_Occurred()) {
            return NULL;
        }
    }
    Py_ssize_t i, j, k;
    flm_core(self, alo, ahi_n, blo, bhi_n, &i, &j, &k);
    return build_match(module_of(self), i, j, k);
}

/*[clinic input]
_difflib.SequenceMatcher.get_matching_blocks

Return list of triples describing matching subsequences.
[clinic start generated code]*/

static PyObject *
_difflib_SequenceMatcher_get_matching_blocks_impl(SequenceMatcherObject *self)
/*[clinic end generated code: output=3b59fa10d3ad4613 input=b11de093158a3d8a]*/
{
    if (self->matching_blocks != NULL) {
        return Py_NewRef(self->matching_blocks);
    }
    if (ensure_ready(self) < 0) {
        return NULL;
    }
    triple_t *blocks = NULL;
    Py_ssize_t n = 0;
    if (compute_matching_blocks(self, &blocks, &n) < 0) {
        return NULL;
    }
    PyObject *result = PyList_New(n);
    if (result == NULL) {
        PyMem_Free(blocks);
        return NULL;
    }
    PyObject *module = module_of(self);
    for (Py_ssize_t idx = 0; idx < n; idx++) {
        PyObject *m = build_match(module, blocks[idx].i,
                                  blocks[idx].j, blocks[idx].k);
        if (m == NULL) {
            Py_DECREF(result);
            PyMem_Free(blocks);
            return NULL;
        }
        PyList_SET_ITEM(result, idx, m);
    }
    PyMem_Free(blocks);
    Py_XSETREF(self->matching_blocks, Py_NewRef(result));
    return result;
}

/*[clinic input]
_difflib.SequenceMatcher.get_opcodes

Return list of 5-tuples describing how to turn a into b.
[clinic start generated code]*/

static PyObject *
_difflib_SequenceMatcher_get_opcodes_impl(SequenceMatcherObject *self)
/*[clinic end generated code: output=be7b94a026664a7d input=4d38c91ce94a560e]*/
{
    if (self->opcodes != NULL) {
        return Py_NewRef(self->opcodes);
    }
    PyObject *blocks = _difflib_SequenceMatcher_get_matching_blocks_impl(self);
    if (blocks == NULL) {
        return NULL;
    }
    PyObject *answer = PyList_New(0);
    if (answer == NULL) {
        Py_DECREF(blocks);
        return NULL;
    }
    Py_ssize_t i = 0, j = 0;
    Py_ssize_t n = PyList_GET_SIZE(blocks);
    for (Py_ssize_t bidx = 0; bidx < n; bidx++) {
        PyObject *m = PyList_GET_ITEM(blocks, bidx);
        Py_ssize_t ai = PyLong_AsSsize_t(PyTuple_GET_ITEM(m, 0));
        Py_ssize_t bj = PyLong_AsSsize_t(PyTuple_GET_ITEM(m, 1));
        Py_ssize_t size = PyLong_AsSsize_t(PyTuple_GET_ITEM(m, 2));

        const char *tag = NULL;
        if (i < ai && j < bj) {
            tag = "replace";
        }
        else if (i < ai) {
            tag = "delete";
        }
        else if (j < bj) {
            tag = "insert";
        }
        if (tag != NULL) {
            PyObject *t = Py_BuildValue("(snnnn)", tag, i, ai, j, bj);
            if (t == NULL || PyList_Append(answer, t) < 0) {
                Py_XDECREF(t);
                Py_DECREF(answer);
                Py_DECREF(blocks);
                return NULL;
            }
            Py_DECREF(t);
        }
        i = ai + size;
        j = bj + size;
        if (size > 0) {
            PyObject *t = Py_BuildValue("(snnnn)", "equal", ai, i, bj, j);
            if (t == NULL || PyList_Append(answer, t) < 0) {
                Py_XDECREF(t);
                Py_DECREF(answer);
                Py_DECREF(blocks);
                return NULL;
            }
            Py_DECREF(t);
        }
    }
    Py_DECREF(blocks);
    Py_XSETREF(self->opcodes, Py_NewRef(answer));
    return answer;
}

/*[clinic input]
_difflib.SequenceMatcher.ratio

Return a measure of the sequences' similarity (float in [0, 1]).
[clinic start generated code]*/

static PyObject *
_difflib_SequenceMatcher_ratio_impl(SequenceMatcherObject *self)
/*[clinic end generated code: output=1691c4582d293748 input=f8c99bdde6e27e60]*/
{
    if (ensure_ready(self) < 0) {
        return NULL;
    }
    triple_t *blocks = NULL;
    Py_ssize_t n = 0;
    if (compute_matching_blocks(self, &blocks, &n) < 0) {
        return NULL;
    }
    Py_ssize_t total_k = 0;
    for (Py_ssize_t i = 0; i < n; i++) {
        total_k += blocks[i].k;
    }
    PyMem_Free(blocks);
    Py_ssize_t denom = self->la + self->lb;
    double r = denom > 0 ? (2.0 * (double)total_k / (double)denom) : 1.0;
    return PyFloat_FromDouble(r);
}


/* ====================================================================== */
/* Type definition.                                                       */
/* ====================================================================== */

static PyMethodDef sequence_matcher_methods[] = {
    _DIFFLIB_SEQUENCEMATCHER_SET_SEQS_METHODDEF
    _DIFFLIB_SEQUENCEMATCHER_SET_SEQ1_METHODDEF
    _DIFFLIB_SEQUENCEMATCHER_SET_SEQ2_METHODDEF
    _DIFFLIB_SEQUENCEMATCHER_FIND_LONGEST_MATCH_METHODDEF
    _DIFFLIB_SEQUENCEMATCHER_GET_MATCHING_BLOCKS_METHODDEF
    _DIFFLIB_SEQUENCEMATCHER_GET_OPCODES_METHODDEF
    _DIFFLIB_SEQUENCEMATCHER_RATIO_METHODDEF
    {NULL, NULL, 0, NULL}
};

static PyMemberDef sequence_matcher_members[] = {
    {"isjunk", Py_T_OBJECT_EX,
     offsetof(SequenceMatcherObject, isjunk), Py_READONLY},
    {"a", Py_T_OBJECT_EX,
     offsetof(SequenceMatcherObject, a), Py_READONLY},
    {"b", Py_T_OBJECT_EX,
     offsetof(SequenceMatcherObject, b), Py_READONLY},
    {"b2j", Py_T_OBJECT_EX,
     offsetof(SequenceMatcherObject, b2j), Py_READONLY},
    {"bjunk", Py_T_OBJECT_EX,
     offsetof(SequenceMatcherObject, bjunk), Py_READONLY},
    {"bpopular", Py_T_OBJECT_EX,
     offsetof(SequenceMatcherObject, bpopular), Py_READONLY},
    {"autojunk", Py_T_BOOL,
     offsetof(SequenceMatcherObject, autojunk), Py_READONLY},
    {"fullbcount", Py_T_OBJECT_EX,
     offsetof(SequenceMatcherObject, fullbcount), 0},
    {NULL, 0, 0, 0, NULL}
};

static int
sequence_matcher_traverse(PyObject *op, visitproc visit, void *arg)
{
    SequenceMatcherObject *self = (SequenceMatcherObject *)op;
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->isjunk);
    Py_VISIT(self->a);
    Py_VISIT(self->b);
    Py_VISIT(self->b2j);
    Py_VISIT(self->bjunk);
    Py_VISIT(self->bpopular);
    Py_VISIT(self->matching_blocks);
    Py_VISIT(self->opcodes);
    Py_VISIT(self->fullbcount);
    Py_VISIT(self->elt_to_lbl_full);
    Py_VISIT(self->elt_to_lbl_dp);
    return 0;
}

static int
sequence_matcher_clear(PyObject *op)
{
    SequenceMatcherObject *self = (SequenceMatcherObject *)op;
    Py_CLEAR(self->isjunk);
    Py_CLEAR(self->a);
    Py_CLEAR(self->b);
    Py_CLEAR(self->b2j);
    Py_CLEAR(self->bjunk);
    Py_CLEAR(self->bpopular);
    Py_CLEAR(self->matching_blocks);
    Py_CLEAR(self->opcodes);
    Py_CLEAR(self->fullbcount);
    Py_CLEAR(self->elt_to_lbl_full);
    Py_CLEAR(self->elt_to_lbl_dp);
    return 0;
}

static void
sequence_matcher_dealloc(PyObject *op)
{
    SequenceMatcherObject *self = (SequenceMatcherObject *)op;
    PyTypeObject *tp = Py_TYPE(self);
    PyObject_GC_UnTrack(self);
    free_a_state(self);
    free_b_state(self);
    (void)sequence_matcher_clear(op);
    tp->tp_free((PyObject *)self);
    Py_DECREF(tp);
}

static PyType_Slot sequence_matcher_slots[] = {
    {Py_tp_doc, (void *)_difflib_SequenceMatcher___init____doc__},
    {Py_tp_init, _difflib_SequenceMatcher___init__},
    {Py_tp_dealloc, sequence_matcher_dealloc},
    {Py_tp_traverse, sequence_matcher_traverse},
    {Py_tp_clear, sequence_matcher_clear},
    {Py_tp_methods, sequence_matcher_methods},
    {Py_tp_members, sequence_matcher_members},
    {0, NULL}
};

static PyType_Spec sequence_matcher_spec = {
    .name = "_difflib.SequenceMatcher",
    .basicsize = sizeof(SequenceMatcherObject),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE
             | Py_TPFLAGS_HAVE_GC,
    .slots = sequence_matcher_slots,
};


/* ====================================================================== */
/* Module init.                                                           */
/* ====================================================================== */

static int
_difflib_exec(PyObject *module)
{
    _difflib_state *state = get_module_state(module);

    PyObject *difflib = PyImport_ImportModule("collections");
    if (difflib == NULL) {
        return -1;
    }
    PyObject *namedtuple = PyObject_GetAttrString(difflib, "namedtuple");
    Py_DECREF(difflib);
    if (namedtuple == NULL) {
        return -1;
    }
    PyObject *match_args = PyTuple_Pack(2,
        PyUnicode_FromString("Match"),
        PyUnicode_FromString("a b size"));
    if (match_args == NULL) {
        Py_DECREF(namedtuple);
        return -1;
    }
    PyObject *Match = PyObject_Call(namedtuple, match_args, NULL);
    Py_DECREF(match_args);
    Py_DECREF(namedtuple);
    if (Match == NULL) {
        return -1;
    }
    state->Match = Match;

    PyObject *type = PyType_FromModuleAndSpec(module, &sequence_matcher_spec,
                                              NULL);
    if (type == NULL) {
        return -1;
    }
    state->SequenceMatcher_Type = (PyTypeObject *)type;
    if (PyModule_AddType(module, (PyTypeObject *)type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(module, "Match", Match) < 0) {
        return -1;
    }
    return 0;
}

static int
_difflib_traverse(PyObject *module, visitproc visit, void *arg)
{
    _difflib_state *state = get_module_state(module);
    Py_VISIT(state->SequenceMatcher_Type);
    Py_VISIT(state->Match);
    return 0;
}

static int
_difflib_clear(PyObject *module)
{
    _difflib_state *state = get_module_state(module);
    Py_CLEAR(state->SequenceMatcher_Type);
    Py_CLEAR(state->Match);
    return 0;
}

static PyModuleDef_Slot _difflib_slots[] = {
    {Py_mod_exec, _difflib_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    /* The module's own state is per-interpreter (no globals); per-instance
       state on SequenceMatcherObject is unsynchronised, matching the
       pure-Python contract that callers don't share an instance across
       threads.  Both are fine under free-threading. */
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static struct PyModuleDef _difflib_module = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_difflib",
    .m_doc = PyDoc_STR(
        "C accelerator for difflib.SequenceMatcher."),
    .m_size = sizeof(_difflib_state),
    .m_slots = _difflib_slots,
    .m_traverse = _difflib_traverse,
    .m_clear = _difflib_clear,
};

PyMODINIT_FUNC
PyInit__difflib(void)
{
    return PyModuleDef_Init(&_difflib_module);
}
