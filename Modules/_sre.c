/*
 * Secret Labs' Regular Expression Engine
 *
 * regular expression matching engine
 *
 * partial history:
 * 1999-10-24 fl   created (based on existing template matcher code)
 * 2000-03-06 fl   first alpha, sort of
 * 2000-08-01 fl   fixes for 1.6b1
 * 2000-08-07 fl   use PyOS_CheckStack() if available
 * 2000-09-20 fl   added expand method
 * 2001-03-20 fl   lots of fixes for 2.1b2
 * 2001-04-15 fl   export copyright as Python attribute, not global
 * 2001-04-28 fl   added __copy__ methods (work in progress)
 * 2001-05-14 fl   fixes for 1.5.2 compatibility
 * 2001-07-01 fl   added BIGCHARSET support (from Martin von Loewis)
 * 2001-10-18 fl   fixed group reset issue (from Matthew Mueller)
 * 2001-10-20 fl   added split primitive; reenable unicode for 1.6/2.0/2.1
 * 2001-10-21 fl   added sub/subn primitive
 * 2001-10-24 fl   added finditer primitive (for 2.2 only)
 * 2001-12-07 fl   fixed memory leak in sub/subn (Guido van Rossum)
 * 2002-11-09 fl   fixed empty sub/subn return type
 * 2003-04-18 mvl  fully support 4-byte codes
 * 2003-10-17 gn   implemented non recursive scheme
 * 2013-02-04 mrab added fullmatch primitive
 *
 * Copyright (c) 1997-2001 by Secret Labs AB.  All rights reserved.
 *
 * This version of the SRE library can be redistributed under CNRI's
 * Python 1.6 license.  For any other use, please contact Secret Labs
 * AB (info@pythonware.com).
 *
 * Portions of this engine have been developed in cooperation with
 * CNRI.  Hewlett-Packard provided funding for 1.6 integration and
 * other compatibility work.
 */

static char copyright[] =
    " SRE 2.2.2 Copyright (c) 1997-2002 by Secret Labs AB ";

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "structmember.h" /* offsetof */

#include "sre.h"

#define SRE_CODE_BITS (8 * sizeof(SRE_CODE))

#include <ctype.h>

/* name of this module, minus the leading underscore */
#if !defined(SRE_MODULE)
#define SRE_MODULE "sre"
#endif

#define SRE_PY_MODULE "re"

/* defining this one enables tracing */
#undef VERBOSE

/* -------------------------------------------------------------------- */
/* optional features */

/* enables fast searching */
#define USE_FAST_SEARCH

/* enables copy/deepcopy handling (work in progress) */
#undef USE_BUILTIN_COPY

/* -------------------------------------------------------------------- */

#if defined(_MSC_VER)
#pragma optimize("agtw", on) /* doesn't seem to make much difference... */
#pragma warning(disable: 4710) /* who cares if functions are not inlined ;-) */
/* fastest possible local call under MSVC */
#define LOCAL(type) static __inline type __fastcall
#elif defined(USE_INLINE)
#define LOCAL(type) static inline type
#else
#define LOCAL(type) static type
#endif

/* error codes */
#define SRE_ERROR_ILLEGAL -1 /* illegal opcode */
#define SRE_ERROR_STATE -2 /* illegal state */
#define SRE_ERROR_RECURSION_LIMIT -3 /* runaway recursion */
#define SRE_ERROR_MEMORY -9 /* out of memory */
#define SRE_ERROR_INTERRUPTED -10 /* signal handler raised exception */

#if defined(VERBOSE)
#define TRACE(v) printf v
#else
#define TRACE(v)
#endif

/* -------------------------------------------------------------------- */
/* search engine state */

/* default character predicates (run sre_chars.py to regenerate tables) */

#define SRE_DIGIT_MASK 1
#define SRE_SPACE_MASK 2
#define SRE_LINEBREAK_MASK 4
#define SRE_ALNUM_MASK 8
#define SRE_WORD_MASK 16

/* FIXME: this assumes ASCII.  create tables in init_sre() instead */

static char sre_char_info[128] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 6, 2,
2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 25, 25, 25, 25, 25, 25, 25, 25,
25, 25, 0, 0, 0, 0, 0, 0, 0, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 0, 0,
0, 0, 16, 0, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24,
24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 0, 0, 0, 0, 0 };

static char sre_char_lower[128] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
61, 62, 63, 64, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
122, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105,
106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
120, 121, 122, 123, 124, 125, 126, 127 };

#define SRE_IS_DIGIT(ch)\
    ((ch) < 128 ? (sre_char_info[(ch)] & SRE_DIGIT_MASK) : 0)
#define SRE_IS_SPACE(ch)\
    ((ch) < 128 ? (sre_char_info[(ch)] & SRE_SPACE_MASK) : 0)
#define SRE_IS_LINEBREAK(ch)\
    ((ch) < 128 ? (sre_char_info[(ch)] & SRE_LINEBREAK_MASK) : 0)
#define SRE_IS_ALNUM(ch)\
    ((ch) < 128 ? (sre_char_info[(ch)] & SRE_ALNUM_MASK) : 0)
#define SRE_IS_WORD(ch)\
    ((ch) < 128 ? (sre_char_info[(ch)] & SRE_WORD_MASK) : 0)

static unsigned int sre_lower(unsigned int ch)
{
    return ((ch) < 128 ? (unsigned int)sre_char_lower[ch] : ch);
}

/* locale-specific character predicates */
/* !(c & ~N) == (c < N+1) for any unsigned c, this avoids
 * warnings when c's type supports only numbers < N+1 */
#define SRE_LOC_IS_ALNUM(ch) (!((ch) & ~255) ? isalnum((ch)) : 0)
#define SRE_LOC_IS_WORD(ch) (SRE_LOC_IS_ALNUM((ch)) || (ch) == '_')

static unsigned int sre_lower_locale(unsigned int ch)
{
    return ((ch) < 256 ? (unsigned int)tolower((ch)) : ch);
}

/* unicode-specific character predicates */

#define SRE_UNI_IS_DIGIT(ch) Py_UNICODE_ISDECIMAL(ch)
#define SRE_UNI_IS_SPACE(ch) Py_UNICODE_ISSPACE(ch)
#define SRE_UNI_IS_LINEBREAK(ch) Py_UNICODE_ISLINEBREAK(ch)
#define SRE_UNI_IS_ALNUM(ch) Py_UNICODE_ISALNUM(ch)
#define SRE_UNI_IS_WORD(ch) (SRE_UNI_IS_ALNUM(ch) || (ch) == '_')

static unsigned int sre_lower_unicode(unsigned int ch)
{
    return (unsigned int) Py_UNICODE_TOLOWER(ch);
}

LOCAL(int)
sre_category(SRE_CODE category, unsigned int ch)
{
    switch (category) {

    case SRE_CATEGORY_DIGIT:
        return SRE_IS_DIGIT(ch);
    case SRE_CATEGORY_NOT_DIGIT:
        return !SRE_IS_DIGIT(ch);
    case SRE_CATEGORY_SPACE:
        return SRE_IS_SPACE(ch);
    case SRE_CATEGORY_NOT_SPACE:
        return !SRE_IS_SPACE(ch);
    case SRE_CATEGORY_WORD:
        return SRE_IS_WORD(ch);
    case SRE_CATEGORY_NOT_WORD:
        return !SRE_IS_WORD(ch);
    case SRE_CATEGORY_LINEBREAK:
        return SRE_IS_LINEBREAK(ch);
    case SRE_CATEGORY_NOT_LINEBREAK:
        return !SRE_IS_LINEBREAK(ch);

    case SRE_CATEGORY_LOC_WORD:
        return SRE_LOC_IS_WORD(ch);
    case SRE_CATEGORY_LOC_NOT_WORD:
        return !SRE_LOC_IS_WORD(ch);

    case SRE_CATEGORY_UNI_DIGIT:
        return SRE_UNI_IS_DIGIT(ch);
    case SRE_CATEGORY_UNI_NOT_DIGIT:
        return !SRE_UNI_IS_DIGIT(ch);
    case SRE_CATEGORY_UNI_SPACE:
        return SRE_UNI_IS_SPACE(ch);
    case SRE_CATEGORY_UNI_NOT_SPACE:
        return !SRE_UNI_IS_SPACE(ch);
    case SRE_CATEGORY_UNI_WORD:
        return SRE_UNI_IS_WORD(ch);
    case SRE_CATEGORY_UNI_NOT_WORD:
        return !SRE_UNI_IS_WORD(ch);
    case SRE_CATEGORY_UNI_LINEBREAK:
        return SRE_UNI_IS_LINEBREAK(ch);
    case SRE_CATEGORY_UNI_NOT_LINEBREAK:
        return !SRE_UNI_IS_LINEBREAK(ch);
    }
    return 0;
}

/* helpers */

static void
data_stack_dealloc(SRE_STATE* state)
{
    if (state->data_stack) {
        PyMem_FREE(state->data_stack);
        state->data_stack = NULL;
    }
    state->data_stack_size = state->data_stack_base = 0;
}

static int
data_stack_grow(SRE_STATE* state, Py_ssize_t size)
{
    Py_ssize_t minsize, cursize;
    minsize = state->data_stack_base+size;
    cursize = state->data_stack_size;
    if (cursize < minsize) {
        void* stack;
        cursize = minsize+minsize/4+1024;
        TRACE(("allocate/grow stack %" PY_FORMAT_SIZE_T "d\n", cursize));
        stack = PyMem_REALLOC(state->data_stack, cursize);
        if (!stack) {
            data_stack_dealloc(state);
            return SRE_ERROR_MEMORY;
        }
        state->data_stack = (char *)stack;
        state->data_stack_size = cursize;
    }
    return 0;
}

/* generate 8-bit version */

#define SRE_CHAR Py_UCS1
#define SIZEOF_SRE_CHAR 1
#define SRE(F) sre_ucs1_##F
#include "sre_lib.h"

/* generate 16-bit unicode version */

#define SRE_CHAR Py_UCS2
#define SIZEOF_SRE_CHAR 2
#define SRE(F) sre_ucs2_##F
#include "sre_lib.h"

/* generate 32-bit unicode version */

#define SRE_CHAR Py_UCS4
#define SIZEOF_SRE_CHAR 4
#define SRE(F) sre_ucs4_##F
#include "sre_lib.h"

/* -------------------------------------------------------------------- */
/* factories and destructors */

/* see sre.h for object declarations */
static PyObject*pattern_new_match(PatternObject*, SRE_STATE*, Py_ssize_t);
static PyObject*pattern_scanner(PatternObject*, PyObject*, PyObject* kw);

static PyObject *
sre_codesize(PyObject* self, PyObject *unused)
{
    return PyLong_FromSize_t(sizeof(SRE_CODE));
}

static PyObject *
sre_getlower(PyObject* self, PyObject* args)
{
    int character, flags;
    if (!PyArg_ParseTuple(args, "ii", &character, &flags))
        return NULL;
    if (flags & SRE_FLAG_LOCALE)
        return Py_BuildValue("i", sre_lower_locale(character));
    if (flags & SRE_FLAG_UNICODE)
        return Py_BuildValue("i", sre_lower_unicode(character));
    return Py_BuildValue("i", sre_lower(character));
}

LOCAL(void)
state_reset(SRE_STATE* state)
{
    /* FIXME: dynamic! */
    /*memset(state->mark, 0, sizeof(*state->mark) * SRE_MARK_SIZE);*/

    state->lastmark = -1;
    state->lastindex = -1;

    state->repeat = NULL;

    data_stack_dealloc(state);
}

static void*
getstring(PyObject* string, Py_ssize_t* p_length,
          int* p_isbytes, int* p_charsize,
          Py_buffer *view)
{
    /* given a python object, return a data pointer, a length (in
       characters), and a character size.  return NULL if the object
       is not a string (or not compatible) */

    /* Unicode objects do not support the buffer API. So, get the data
       directly instead. */
    if (PyUnicode_Check(string)) {
        if (PyUnicode_READY(string) == -1)
            return NULL;
        *p_length = PyUnicode_GET_LENGTH(string);
        *p_charsize = PyUnicode_KIND(string);
        *p_isbytes = 0;
        return PyUnicode_DATA(string);
    }

    /* get pointer to byte string buffer */
    if (PyObject_GetBuffer(string, view, PyBUF_SIMPLE) != 0) {
        PyErr_SetString(PyExc_TypeError, "expected string or buffer");
        return NULL;
    }

    *p_length = view->len;
    *p_charsize = 1;
    *p_isbytes = 1;

    if (view->buf == NULL) {
        PyErr_SetString(PyExc_ValueError, "Buffer is NULL");
        PyBuffer_Release(view);
        view->buf = NULL;
        return NULL;
    }
    return view->buf;
}

LOCAL(PyObject*)
state_init(SRE_STATE* state, PatternObject* pattern, PyObject* string,
           Py_ssize_t start, Py_ssize_t end)
{
    /* prepare state object */

    Py_ssize_t length;
    int isbytes, charsize;
    void* ptr;

    memset(state, 0, sizeof(SRE_STATE));

    state->lastmark = -1;
    state->lastindex = -1;

    state->buffer.buf = NULL;
    ptr = getstring(string, &length, &isbytes, &charsize, &state->buffer);
    if (!ptr)
        goto err;

    if (isbytes && pattern->isbytes == 0) {
        PyErr_SetString(PyExc_TypeError,
                        "can't use a string pattern on a bytes-like object");
        goto err;
    }
    if (!isbytes && pattern->isbytes > 0) {
        PyErr_SetString(PyExc_TypeError,
                        "can't use a bytes pattern on a string-like object");
        goto err;
    }

    /* adjust boundaries */
    if (start < 0)
        start = 0;
    else if (start > length)
        start = length;

    if (end < 0)
        end = 0;
    else if (end > length)
        end = length;

    state->isbytes = isbytes;
    state->charsize = charsize;

    state->beginning = ptr;

    state->start = (void*) ((char*) ptr + start * state->charsize);
    state->end = (void*) ((char*) ptr + end * state->charsize);

    Py_INCREF(string);
    state->string = string;
    state->pos = start;
    state->endpos = end;

    if (pattern->flags & SRE_FLAG_LOCALE)
        state->lower = sre_lower_locale;
    else if (pattern->flags & SRE_FLAG_UNICODE)
        state->lower = sre_lower_unicode;
    else
        state->lower = sre_lower;

    return string;
  err:
    if (state->buffer.buf)
        PyBuffer_Release(&state->buffer);
    return NULL;
}

LOCAL(void)
state_fini(SRE_STATE* state)
{
    if (state->buffer.buf)
        PyBuffer_Release(&state->buffer);
    Py_XDECREF(state->string);
    data_stack_dealloc(state);
}

/* calculate offset from start of string */
#define STATE_OFFSET(state, member)\
    (((char*)(member) - (char*)(state)->beginning) / (state)->charsize)

LOCAL(PyObject*)
getslice(int isbytes, const void *ptr,
         PyObject* string, Py_ssize_t start, Py_ssize_t end)
{
    if (isbytes) {
        if (PyBytes_CheckExact(string) &&
            start == 0 && end == PyBytes_GET_SIZE(string)) {
            Py_INCREF(string);
            return string;
        }
        return PyBytes_FromStringAndSize(
                (const char *)ptr + start, end - start);
    }
    else {
        return PyUnicode_Substring(string, start, end);
    }
}

LOCAL(PyObject*)
state_getslice(SRE_STATE* state, Py_ssize_t index, PyObject* string, int empty)
{
    Py_ssize_t i, j;

    index = (index - 1) * 2;

    if (string == Py_None || index >= state->lastmark || !state->mark[index] || !state->mark[index+1]) {
        if (empty)
            /* want empty string */
            i = j = 0;
        else {
            Py_INCREF(Py_None);
            return Py_None;
        }
    } else {
        i = STATE_OFFSET(state, state->mark[index]);
        j = STATE_OFFSET(state, state->mark[index+1]);
    }

    return getslice(state->isbytes, state->beginning, string, i, j);
}

static void
pattern_error(Py_ssize_t status)
{
    switch (status) {
    case SRE_ERROR_RECURSION_LIMIT:
        PyErr_SetString(
            PyExc_RuntimeError,
            "maximum recursion limit exceeded"
            );
        break;
    case SRE_ERROR_MEMORY:
        PyErr_NoMemory();
        break;
    case SRE_ERROR_INTERRUPTED:
    /* An exception has already been raised, so let it fly */
        break;
    default:
        /* other error codes indicate compiler/engine bugs */
        PyErr_SetString(
            PyExc_RuntimeError,
            "internal error in regular expression engine"
            );
    }
}

static void
pattern_dealloc(PatternObject* self)
{
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *) self);
    Py_XDECREF(self->pattern);
    Py_XDECREF(self->groupindex);
    Py_XDECREF(self->indexgroup);
    PyObject_DEL(self);
}

LOCAL(Py_ssize_t)
sre_match(SRE_STATE* state, SRE_CODE* pattern, int match_all)
{
    if (state->charsize == 1)
        return sre_ucs1_match(state, pattern, match_all);
    if (state->charsize == 2)
        return sre_ucs2_match(state, pattern, match_all);
    assert(state->charsize == 4);
    return sre_ucs4_match(state, pattern, match_all);
}

LOCAL(Py_ssize_t)
sre_search(SRE_STATE* state, SRE_CODE* pattern)
{
    if (state->charsize == 1)
        return sre_ucs1_search(state, pattern);
    if (state->charsize == 2)
        return sre_ucs2_search(state, pattern);
    assert(state->charsize == 4);
    return sre_ucs4_search(state, pattern);
}

static PyObject *
fix_string_param(PyObject *string, PyObject *string2, const char *oldname)
{
    if (string2 != NULL) {
        if (string != NULL) {
            PyErr_Format(PyExc_TypeError,
                         "Argument given by name ('%s') and position (1)",
                         oldname);
            return NULL;
        }
        if (PyErr_WarnFormat(PyExc_DeprecationWarning, 1,
                             "The '%s' keyword parameter name is deprecated.  "
                             "Use 'string' instead.", oldname) < 0)
            return NULL;
        return string2;
    }
    if (string == NULL) {
        PyErr_SetString(PyExc_TypeError,
                        "Required argument 'string' (pos 1) not found");
        return NULL;
    }
    return string;
}

static PyObject *
pattern_match(PatternObject *self, PyObject *args, PyObject *kwargs)
{
    static char *_keywords[] = {"string", "pos", "endpos", "pattern", NULL};
    PyObject *string = NULL;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;
    PyObject *pattern = NULL;
    SRE_STATE state;
    Py_ssize_t status;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "|Onn$O:match", _keywords,
        &string, &pos, &endpos, &pattern))
        return NULL;
    string = fix_string_param(string, pattern, "pattern");
    if (!string)
        return NULL;
    string = state_init(&state, (PatternObject *)self, string, pos, endpos);
    if (!string)
        return NULL;

    state.ptr = state.start;

    TRACE(("|%p|%p|MATCH\n", PatternObject_GetCode(self), state.ptr));

    status = sre_match(&state, PatternObject_GetCode(self), 0);

    TRACE(("|%p|%p|END\n", PatternObject_GetCode(self), state.ptr));
    if (PyErr_Occurred())
        return NULL;

    state_fini(&state);

    return (PyObject *)pattern_new_match(self, &state, status);
}

static PyObject*
pattern_fullmatch(PatternObject* self, PyObject* args, PyObject* kw)
{
    SRE_STATE state;
    Py_ssize_t status;

    PyObject *string = NULL, *string2 = NULL;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    static char* kwlist[] = { "string", "pos", "endpos", "pattern", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|Onn$O:fullmatch", kwlist,
                                     &string, &start, &end, &string2))
        return NULL;

    string = fix_string_param(string, string2, "pattern");
    if (!string)
        return NULL;

    string = state_init(&state, self, string, start, end);
    if (!string)
        return NULL;

    state.ptr = state.start;

    TRACE(("|%p|%p|FULLMATCH\n", PatternObject_GetCode(self), state.ptr));

    status = sre_match(&state, PatternObject_GetCode(self), 1);

    TRACE(("|%p|%p|END\n", PatternObject_GetCode(self), state.ptr));
    if (PyErr_Occurred())
        return NULL;

    state_fini(&state);

    return pattern_new_match(self, &state, status);
}

static PyObject*
pattern_search(PatternObject* self, PyObject* args, PyObject* kw)
{
    SRE_STATE state;
    Py_ssize_t status;

    PyObject *string = NULL, *string2 = NULL;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    static char* kwlist[] = { "string", "pos", "endpos", "pattern", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|Onn$O:search", kwlist,
                                     &string, &start, &end, &string2))
        return NULL;

    string = fix_string_param(string, string2, "pattern");
    if (!string)
        return NULL;

    string = state_init(&state, self, string, start, end);
    if (!string)
        return NULL;

    TRACE(("|%p|%p|SEARCH\n", PatternObject_GetCode(self), state.ptr));

    status = sre_search(&state, PatternObject_GetCode(self));

    TRACE(("|%p|%p|END\n", PatternObject_GetCode(self), state.ptr));

    state_fini(&state);

    if (PyErr_Occurred())
        return NULL;

    return pattern_new_match(self, &state, status);
}

static PyObject*
call(char* module, char* function, PyObject* args)
{
    PyObject* name;
    PyObject* mod;
    PyObject* func;
    PyObject* result;

    if (!args)
        return NULL;
    name = PyUnicode_FromString(module);
    if (!name)
        return NULL;
    mod = PyImport_Import(name);
    Py_DECREF(name);
    if (!mod)
        return NULL;
    func = PyObject_GetAttrString(mod, function);
    Py_DECREF(mod);
    if (!func)
        return NULL;
    result = PyObject_CallObject(func, args);
    Py_DECREF(func);
    Py_DECREF(args);
    return result;
}

#ifdef USE_BUILTIN_COPY
static int
deepcopy(PyObject** object, PyObject* memo)
{
    PyObject* copy;

    copy = call(
        "copy", "deepcopy",
        PyTuple_Pack(2, *object, memo)
        );
    if (!copy)
        return 0;

    Py_DECREF(*object);
    *object = copy;

    return 1; /* success */
}
#endif

static PyObject*
pattern_findall(PatternObject* self, PyObject* args, PyObject* kw)
{
    SRE_STATE state;
    PyObject* list;
    Py_ssize_t status;
    Py_ssize_t i, b, e;

    PyObject *string = NULL, *string2 = NULL;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    static char* kwlist[] = { "string", "pos", "endpos", "source", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|Onn$O:findall", kwlist,
                                     &string, &start, &end, &string2))
        return NULL;

    string = fix_string_param(string, string2, "source");
    if (!string)
        return NULL;

    string = state_init(&state, self, string, start, end);
    if (!string)
        return NULL;

    list = PyList_New(0);
    if (!list) {
        state_fini(&state);
        return NULL;
    }

    while (state.start <= state.end) {

        PyObject* item;

        state_reset(&state);

        state.ptr = state.start;

        status = sre_search(&state, PatternObject_GetCode(self));
        if (PyErr_Occurred())
            goto error;

        if (status <= 0) {
            if (status == 0)
                break;
            pattern_error(status);
            goto error;
        }

        /* don't bother to build a match object */
        switch (self->groups) {
        case 0:
            b = STATE_OFFSET(&state, state.start);
            e = STATE_OFFSET(&state, state.ptr);
            item = getslice(state.isbytes, state.beginning,
                            string, b, e);
            if (!item)
                goto error;
            break;
        case 1:
            item = state_getslice(&state, 1, string, 1);
            if (!item)
                goto error;
            break;
        default:
            item = PyTuple_New(self->groups);
            if (!item)
                goto error;
            for (i = 0; i < self->groups; i++) {
                PyObject* o = state_getslice(&state, i+1, string, 1);
                if (!o) {
                    Py_DECREF(item);
                    goto error;
                }
                PyTuple_SET_ITEM(item, i, o);
            }
            break;
        }

        status = PyList_Append(list, item);
        Py_DECREF(item);
        if (status < 0)
            goto error;

        if (state.ptr == state.start)
            state.start = (void*) ((char*) state.ptr + state.charsize);
        else
            state.start = state.ptr;

    }

    state_fini(&state);
    return list;

error:
    Py_DECREF(list);
    state_fini(&state);
    return NULL;

}

static PyObject*
pattern_finditer(PatternObject* pattern, PyObject* args, PyObject* kw)
{
    PyObject* scanner;
    PyObject* search;
    PyObject* iterator;

    scanner = pattern_scanner(pattern, args, kw);
    if (!scanner)
        return NULL;

    search = PyObject_GetAttrString(scanner, "search");
    Py_DECREF(scanner);
    if (!search)
        return NULL;

    iterator = PyCallIter_New(search, Py_None);
    Py_DECREF(search);

    return iterator;
}

static PyObject*
pattern_split(PatternObject* self, PyObject* args, PyObject* kw)
{
    SRE_STATE state;
    PyObject* list;
    PyObject* item;
    Py_ssize_t status;
    Py_ssize_t n;
    Py_ssize_t i;
    void* last;

    PyObject *string = NULL, *string2 = NULL;
    Py_ssize_t maxsplit = 0;
    static char* kwlist[] = { "string", "maxsplit", "source", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|On$O:split", kwlist,
                                     &string, &maxsplit, &string2))
        return NULL;

    string = fix_string_param(string, string2, "source");
    if (!string)
        return NULL;

    string = state_init(&state, self, string, 0, PY_SSIZE_T_MAX);
    if (!string)
        return NULL;

    list = PyList_New(0);
    if (!list) {
        state_fini(&state);
        return NULL;
    }

    n = 0;
    last = state.start;

    while (!maxsplit || n < maxsplit) {

        state_reset(&state);

        state.ptr = state.start;

        status = sre_search(&state, PatternObject_GetCode(self));
        if (PyErr_Occurred())
            goto error;

        if (status <= 0) {
            if (status == 0)
                break;
            pattern_error(status);
            goto error;
        }

        if (state.start == state.ptr) {
            if (last == state.end)
                break;
            /* skip one character */
            state.start = (void*) ((char*) state.ptr + state.charsize);
            continue;
        }

        /* get segment before this match */
        item = getslice(state.isbytes, state.beginning,
            string, STATE_OFFSET(&state, last),
            STATE_OFFSET(&state, state.start)
            );
        if (!item)
            goto error;
        status = PyList_Append(list, item);
        Py_DECREF(item);
        if (status < 0)
            goto error;

        /* add groups (if any) */
        for (i = 0; i < self->groups; i++) {
            item = state_getslice(&state, i+1, string, 0);
            if (!item)
                goto error;
            status = PyList_Append(list, item);
            Py_DECREF(item);
            if (status < 0)
                goto error;
        }

        n = n + 1;

        last = state.start = state.ptr;

    }

    /* get segment following last match (even if empty) */
    item = getslice(state.isbytes, state.beginning,
        string, STATE_OFFSET(&state, last), state.endpos
        );
    if (!item)
        goto error;
    status = PyList_Append(list, item);
    Py_DECREF(item);
    if (status < 0)
        goto error;

    state_fini(&state);
    return list;

error:
    Py_DECREF(list);
    state_fini(&state);
    return NULL;

}

static PyObject*
pattern_subx(PatternObject* self, PyObject* ptemplate, PyObject* string,
             Py_ssize_t count, Py_ssize_t subn)
{
    SRE_STATE state;
    PyObject* list;
    PyObject* joiner;
    PyObject* item;
    PyObject* filter;
    PyObject* args;
    PyObject* match;
    void* ptr;
    Py_ssize_t status;
    Py_ssize_t n;
    Py_ssize_t i, b, e;
    int isbytes, charsize;
    int filter_is_callable;
    Py_buffer view;

    if (PyCallable_Check(ptemplate)) {
        /* sub/subn takes either a function or a template */
        filter = ptemplate;
        Py_INCREF(filter);
        filter_is_callable = 1;
    } else {
        /* if not callable, check if it's a literal string */
        int literal;
        view.buf = NULL;
        ptr = getstring(ptemplate, &n, &isbytes, &charsize, &view);
        b = charsize;
        if (ptr) {
            if (charsize == 1)
                literal = memchr(ptr, '\\', n) == NULL;
            else
                literal = PyUnicode_FindChar(ptemplate, '\\', 0, n, 1) == -1;
        } else {
            PyErr_Clear();
            literal = 0;
        }
        if (view.buf)
            PyBuffer_Release(&view);
        if (literal) {
            filter = ptemplate;
            Py_INCREF(filter);
            filter_is_callable = 0;
        } else {
            /* not a literal; hand it over to the template compiler */
            filter = call(
                SRE_PY_MODULE, "_subx",
                PyTuple_Pack(2, self, ptemplate)
                );
            if (!filter)
                return NULL;
            filter_is_callable = PyCallable_Check(filter);
        }
    }

    string = state_init(&state, self, string, 0, PY_SSIZE_T_MAX);
    if (!string) {
        Py_DECREF(filter);
        return NULL;
    }

    list = PyList_New(0);
    if (!list) {
        Py_DECREF(filter);
        state_fini(&state);
        return NULL;
    }

    n = i = 0;

    while (!count || n < count) {

        state_reset(&state);

        state.ptr = state.start;

        status = sre_search(&state, PatternObject_GetCode(self));
        if (PyErr_Occurred())
            goto error;

        if (status <= 0) {
            if (status == 0)
                break;
            pattern_error(status);
            goto error;
        }

        b = STATE_OFFSET(&state, state.start);
        e = STATE_OFFSET(&state, state.ptr);

        if (i < b) {
            /* get segment before this match */
            item = getslice(state.isbytes, state.beginning,
                string, i, b);
            if (!item)
                goto error;
            status = PyList_Append(list, item);
            Py_DECREF(item);
            if (status < 0)
                goto error;

        } else if (i == b && i == e && n > 0)
            /* ignore empty match on latest position */
            goto next;

        if (filter_is_callable) {
            /* pass match object through filter */
            match = pattern_new_match(self, &state, 1);
            if (!match)
                goto error;
            args = PyTuple_Pack(1, match);
            if (!args) {
                Py_DECREF(match);
                goto error;
            }
            item = PyObject_CallObject(filter, args);
            Py_DECREF(args);
            Py_DECREF(match);
            if (!item)
                goto error;
        } else {
            /* filter is literal string */
            item = filter;
            Py_INCREF(item);
        }

        /* add to list */
        if (item != Py_None) {
            status = PyList_Append(list, item);
            Py_DECREF(item);
            if (status < 0)
                goto error;
        }

        i = e;
        n = n + 1;

next:
        /* move on */
        if (state.ptr == state.start)
            state.start = (void*) ((char*) state.ptr + state.charsize);
        else
            state.start = state.ptr;

    }

    /* get segment following last match */
    if (i < state.endpos) {
        item = getslice(state.isbytes, state.beginning,
                        string, i, state.endpos);
        if (!item)
            goto error;
        status = PyList_Append(list, item);
        Py_DECREF(item);
        if (status < 0)
            goto error;
    }

    state_fini(&state);

    Py_DECREF(filter);

    /* convert list to single string (also removes list) */
    joiner = getslice(state.isbytes, state.beginning, string, 0, 0);
    if (!joiner) {
        Py_DECREF(list);
        return NULL;
    }
    if (PyList_GET_SIZE(list) == 0) {
        Py_DECREF(list);
        item = joiner;
    }
    else {
        if (state.isbytes)
            item = _PyBytes_Join(joiner, list);
        else
            item = PyUnicode_Join(joiner, list);
        Py_DECREF(joiner);
        Py_DECREF(list);
        if (!item)
            return NULL;
    }

    if (subn)
        return Py_BuildValue("Nn", item, n);

    return item;

error:
    Py_DECREF(list);
    state_fini(&state);
    Py_DECREF(filter);
    return NULL;

}

static PyObject*
pattern_sub(PatternObject* self, PyObject* args, PyObject* kw)
{
    PyObject* ptemplate;
    PyObject* string;
    Py_ssize_t count = 0;
    static char* kwlist[] = { "repl", "string", "count", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO|n:sub", kwlist,
                                     &ptemplate, &string, &count))
        return NULL;

    return pattern_subx(self, ptemplate, string, count, 0);
}

static PyObject*
pattern_subn(PatternObject* self, PyObject* args, PyObject* kw)
{
    PyObject* ptemplate;
    PyObject* string;
    Py_ssize_t count = 0;
    static char* kwlist[] = { "repl", "string", "count", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "OO|n:subn", kwlist,
                                     &ptemplate, &string, &count))
        return NULL;

    return pattern_subx(self, ptemplate, string, count, 1);
}

static PyObject*
pattern_copy(PatternObject* self, PyObject *unused)
{
#ifdef USE_BUILTIN_COPY
    PatternObject* copy;
    int offset;

    copy = PyObject_NEW_VAR(PatternObject, &Pattern_Type, self->codesize);
    if (!copy)
        return NULL;

    offset = offsetof(PatternObject, groups);

    Py_XINCREF(self->groupindex);
    Py_XINCREF(self->indexgroup);
    Py_XINCREF(self->pattern);

    memcpy((char*) copy + offset, (char*) self + offset,
           sizeof(PatternObject) + self->codesize * sizeof(SRE_CODE) - offset);
    copy->weakreflist = NULL;

    return (PyObject*) copy;
#else
    PyErr_SetString(PyExc_TypeError, "cannot copy this pattern object");
    return NULL;
#endif
}

static PyObject*
pattern_deepcopy(PatternObject* self, PyObject* memo)
{
#ifdef USE_BUILTIN_COPY
    PatternObject* copy;

    copy = (PatternObject*) pattern_copy(self);
    if (!copy)
        return NULL;

    if (!deepcopy(&copy->groupindex, memo) ||
        !deepcopy(&copy->indexgroup, memo) ||
        !deepcopy(&copy->pattern, memo)) {
        Py_DECREF(copy);
        return NULL;
    }

#else
    PyErr_SetString(PyExc_TypeError, "cannot deepcopy this pattern object");
    return NULL;
#endif
}

static PyObject *
pattern_repr(PatternObject *obj)
{
    static const struct {
        const char *name;
        int value;
    } flag_names[] = {
        {"re.TEMPLATE", SRE_FLAG_TEMPLATE},
        {"re.IGNORECASE", SRE_FLAG_IGNORECASE},
        {"re.LOCALE", SRE_FLAG_LOCALE},
        {"re.MULTILINE", SRE_FLAG_MULTILINE},
        {"re.DOTALL", SRE_FLAG_DOTALL},
        {"re.UNICODE", SRE_FLAG_UNICODE},
        {"re.VERBOSE", SRE_FLAG_VERBOSE},
        {"re.DEBUG", SRE_FLAG_DEBUG},
        {"re.ASCII", SRE_FLAG_ASCII},
    };
    PyObject *result = NULL;
    PyObject *flag_items;
    int i;
    int flags = obj->flags;

    /* Omit re.UNICODE for valid string patterns. */
    if (obj->isbytes == 0 &&
        (flags & (SRE_FLAG_LOCALE|SRE_FLAG_UNICODE|SRE_FLAG_ASCII)) ==
         SRE_FLAG_UNICODE)
        flags &= ~SRE_FLAG_UNICODE;

    flag_items = PyList_New(0);
    if (!flag_items)
        return NULL;

    for (i = 0; i < Py_ARRAY_LENGTH(flag_names); i++) {
        if (flags & flag_names[i].value) {
            PyObject *item = PyUnicode_FromString(flag_names[i].name);
            if (!item)
                goto done;

            if (PyList_Append(flag_items, item) < 0) {
                Py_DECREF(item);
                goto done;
            }
            Py_DECREF(item);
            flags &= ~flag_names[i].value;
        }
    }
    if (flags) {
        PyObject *item = PyUnicode_FromFormat("0x%x", flags);
        if (!item)
            goto done;

        if (PyList_Append(flag_items, item) < 0) {
            Py_DECREF(item);
            goto done;
        }
        Py_DECREF(item);
    }

    if (PyList_Size(flag_items) > 0) {
        PyObject *flags_result;
        PyObject *sep = PyUnicode_FromString("|");
        if (!sep)
            goto done;
        flags_result = PyUnicode_Join(sep, flag_items);
        Py_DECREF(sep);
        if (!flags_result)
            goto done;
        result = PyUnicode_FromFormat("re.compile(%.200R, %S)",
                                      obj->pattern, flags_result);
        Py_DECREF(flags_result);
    }
    else {
        result = PyUnicode_FromFormat("re.compile(%.200R)", obj->pattern);
    }

done:
    Py_DECREF(flag_items);
    return result;
}

PyDoc_STRVAR(pattern_match_doc,
"match(string[, pos[, endpos]]) -> match object or None.\n\
    Matches zero or more characters at the beginning of the string");

PyDoc_STRVAR(pattern_fullmatch_doc,
"fullmatch(string[, pos[, endpos]]) -> match object or None.\n\
    Matches against all of the string");

PyDoc_STRVAR(pattern_search_doc,
"search(string[, pos[, endpos]]) -> match object or None.\n\
    Scan through string looking for a match, and return a corresponding\n\
    match object instance. Return None if no position in the string matches.");

PyDoc_STRVAR(pattern_split_doc,
"split(string[, maxsplit = 0])  -> list.\n\
    Split string by the occurrences of pattern.");

PyDoc_STRVAR(pattern_findall_doc,
"findall(string[, pos[, endpos]]) -> list.\n\
   Return a list of all non-overlapping matches of pattern in string.");

PyDoc_STRVAR(pattern_finditer_doc,
"finditer(string[, pos[, endpos]]) -> iterator.\n\
    Return an iterator over all non-overlapping matches for the \n\
    RE pattern in string. For each match, the iterator returns a\n\
    match object.");

PyDoc_STRVAR(pattern_sub_doc,
"sub(repl, string[, count = 0]) -> newstring.\n\
    Return the string obtained by replacing the leftmost non-overlapping\n\
    occurrences of pattern in string by the replacement repl.");

PyDoc_STRVAR(pattern_subn_doc,
"subn(repl, string[, count = 0]) -> (newstring, number of subs)\n\
    Return the tuple (new_string, number_of_subs_made) found by replacing\n\
    the leftmost non-overlapping occurrences of pattern with the\n\
    replacement repl.");

PyDoc_STRVAR(pattern_doc, "Compiled regular expression objects");

static PyMethodDef pattern_methods[] = {
    {"match", (PyCFunction) pattern_match, METH_VARARGS|METH_KEYWORDS,
        pattern_match_doc},
    {"fullmatch", (PyCFunction) pattern_fullmatch, METH_VARARGS|METH_KEYWORDS,
        pattern_fullmatch_doc},
    {"search", (PyCFunction) pattern_search, METH_VARARGS|METH_KEYWORDS,
        pattern_search_doc},
    {"sub", (PyCFunction) pattern_sub, METH_VARARGS|METH_KEYWORDS,
        pattern_sub_doc},
    {"subn", (PyCFunction) pattern_subn, METH_VARARGS|METH_KEYWORDS,
        pattern_subn_doc},
    {"split", (PyCFunction) pattern_split, METH_VARARGS|METH_KEYWORDS,
        pattern_split_doc},
    {"findall", (PyCFunction) pattern_findall, METH_VARARGS|METH_KEYWORDS,
        pattern_findall_doc},
    {"finditer", (PyCFunction) pattern_finditer, METH_VARARGS|METH_KEYWORDS,
        pattern_finditer_doc},
    {"scanner", (PyCFunction) pattern_scanner, METH_VARARGS|METH_KEYWORDS},
    {"__copy__", (PyCFunction) pattern_copy, METH_NOARGS},
    {"__deepcopy__", (PyCFunction) pattern_deepcopy, METH_O},
    {NULL, NULL}
};

#define PAT_OFF(x) offsetof(PatternObject, x)
static PyMemberDef pattern_members[] = {
    {"pattern",    T_OBJECT,    PAT_OFF(pattern),       READONLY},
    {"flags",      T_INT,       PAT_OFF(flags),         READONLY},
    {"groups",     T_PYSSIZET,  PAT_OFF(groups),        READONLY},
    {"groupindex", T_OBJECT,    PAT_OFF(groupindex),    READONLY},
    {NULL}  /* Sentinel */
};

static PyTypeObject Pattern_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_" SRE_MODULE ".SRE_Pattern",
    sizeof(PatternObject), sizeof(SRE_CODE),
    (destructor)pattern_dealloc,        /* tp_dealloc */
    0,                                  /* tp_print */
    0,                                  /* tp_getattr */
    0,                                  /* tp_setattr */
    0,                                  /* tp_reserved */
    (reprfunc)pattern_repr,             /* tp_repr */
    0,                                  /* tp_as_number */
    0,                                  /* tp_as_sequence */
    0,                                  /* tp_as_mapping */
    0,                                  /* tp_hash */
    0,                                  /* tp_call */
    0,                                  /* tp_str */
    0,                                  /* tp_getattro */
    0,                                  /* tp_setattro */
    0,                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    pattern_doc,                        /* tp_doc */
    0,                                  /* tp_traverse */
    0,                                  /* tp_clear */
    0,                                  /* tp_richcompare */
    offsetof(PatternObject, weakreflist),       /* tp_weaklistoffset */
    0,                                  /* tp_iter */
    0,                                  /* tp_iternext */
    pattern_methods,                    /* tp_methods */
    pattern_members,                    /* tp_members */
};

static int _validate(PatternObject *self); /* Forward */

static PyObject *
_compile(PyObject* self_, PyObject* args)
{
    /* "compile" pattern descriptor to pattern object */

    PatternObject* self;
    Py_ssize_t i, n;

    PyObject* pattern;
    int flags = 0;
    PyObject* code;
    Py_ssize_t groups = 0;
    PyObject* groupindex = NULL;
    PyObject* indexgroup = NULL;

    if (!PyArg_ParseTuple(args, "OiO!|nOO", &pattern, &flags,
                          &PyList_Type, &code, &groups,
                          &groupindex, &indexgroup))
        return NULL;

    n = PyList_GET_SIZE(code);
    /* coverity[ampersand_in_size] */
    self = PyObject_NEW_VAR(PatternObject, &Pattern_Type, n);
    if (!self)
        return NULL;
    self->weakreflist = NULL;
    self->pattern = NULL;
    self->groupindex = NULL;
    self->indexgroup = NULL;

    self->codesize = n;

    for (i = 0; i < n; i++) {
        PyObject *o = PyList_GET_ITEM(code, i);
        unsigned long value = PyLong_AsUnsignedLong(o);
        self->code[i] = (SRE_CODE) value;
        if ((unsigned long) self->code[i] != value) {
            PyErr_SetString(PyExc_OverflowError,
                            "regular expression code size limit exceeded");
            break;
        }
    }

    if (PyErr_Occurred()) {
        Py_DECREF(self);
        return NULL;
    }

    if (pattern == Py_None) {
        self->isbytes = -1;
    }
    else {
        Py_ssize_t p_length;
        int charsize;
        Py_buffer view;
        view.buf = NULL;
        if (!getstring(pattern, &p_length, &self->isbytes,
                       &charsize, &view)) {
            Py_DECREF(self);
            return NULL;
        }
        if (view.buf)
            PyBuffer_Release(&view);
    }

    Py_INCREF(pattern);
    self->pattern = pattern;

    self->flags = flags;

    self->groups = groups;

    Py_XINCREF(groupindex);
    self->groupindex = groupindex;

    Py_XINCREF(indexgroup);
    self->indexgroup = indexgroup;

    self->weakreflist = NULL;

    if (!_validate(self)) {
        Py_DECREF(self);
        return NULL;
    }

    return (PyObject*) self;
}

/* -------------------------------------------------------------------- */
/* Code validation */

/* To learn more about this code, have a look at the _compile() function in
   Lib/sre_compile.py.  The validation functions below checks the code array
   for conformance with the code patterns generated there.

   The nice thing about the generated code is that it is position-independent:
   all jumps are relative jumps forward.  Also, jumps don't cross each other:
   the target of a later jump is always earlier than the target of an earlier
   jump.  IOW, this is okay:

   J---------J-------T--------T
    \         \_____/        /
     \______________________/

   but this is not:

   J---------J-------T--------T
    \_________\_____/        /
               \____________/

   It also helps that SRE_CODE is always an unsigned type.
*/

/* Defining this one enables tracing of the validator */
#undef VVERBOSE

/* Trace macro for the validator */
#if defined(VVERBOSE)
#define VTRACE(v) printf v
#else
#define VTRACE(v) do {} while(0)  /* do nothing */
#endif

/* Report failure */
#define FAIL do { VTRACE(("FAIL: %d\n", __LINE__)); return 0; } while (0)

/* Extract opcode, argument, or skip count from code array */
#define GET_OP                                          \
    do {                                                \
        VTRACE(("%p: ", code));                         \
        if (code >= end) FAIL;                          \
        op = *code++;                                   \
        VTRACE(("%lu (op)\n", (unsigned long)op));      \
    } while (0)
#define GET_ARG                                         \
    do {                                                \
        VTRACE(("%p= ", code));                         \
        if (code >= end) FAIL;                          \
        arg = *code++;                                  \
        VTRACE(("%lu (arg)\n", (unsigned long)arg));    \
    } while (0)
#define GET_SKIP_ADJ(adj)                               \
    do {                                                \
        VTRACE(("%p= ", code));                         \
        if (code >= end) FAIL;                          \
        skip = *code;                                   \
        VTRACE(("%lu (skip to %p)\n",                   \
               (unsigned long)skip, code+skip));        \
        if (skip-adj > (Py_uintptr_t)(end - code))      \
            FAIL;                                       \
        code++;                                         \
    } while (0)
#define GET_SKIP GET_SKIP_ADJ(0)

static int
_validate_charset(SRE_CODE *code, SRE_CODE *end)
{
    /* Some variables are manipulated by the macros above */
    SRE_CODE op;
    SRE_CODE arg;
    SRE_CODE offset;
    int i;

    while (code < end) {
        GET_OP;
        switch (op) {

        case SRE_OP_NEGATE:
            break;

        case SRE_OP_LITERAL:
            GET_ARG;
            break;

        case SRE_OP_RANGE:
            GET_ARG;
            GET_ARG;
            break;

        case SRE_OP_CHARSET:
            offset = 256/SRE_CODE_BITS; /* 256-bit bitmap */
            if (offset > (Py_uintptr_t)(end - code))
                FAIL;
            code += offset;
            break;

        case SRE_OP_BIGCHARSET:
            GET_ARG; /* Number of blocks */
            offset = 256/sizeof(SRE_CODE); /* 256-byte table */
            if (offset > (Py_uintptr_t)(end - code))
                FAIL;
            /* Make sure that each byte points to a valid block */
            for (i = 0; i < 256; i++) {
                if (((unsigned char *)code)[i] >= arg)
                    FAIL;
            }
            code += offset;
            offset = arg * (256/SRE_CODE_BITS); /* 256-bit bitmap times arg */
            if (offset > (Py_uintptr_t)(end - code))
                FAIL;
            code += offset;
            break;

        case SRE_OP_CATEGORY:
            GET_ARG;
            switch (arg) {
            case SRE_CATEGORY_DIGIT:
            case SRE_CATEGORY_NOT_DIGIT:
            case SRE_CATEGORY_SPACE:
            case SRE_CATEGORY_NOT_SPACE:
            case SRE_CATEGORY_WORD:
            case SRE_CATEGORY_NOT_WORD:
            case SRE_CATEGORY_LINEBREAK:
            case SRE_CATEGORY_NOT_LINEBREAK:
            case SRE_CATEGORY_LOC_WORD:
            case SRE_CATEGORY_LOC_NOT_WORD:
            case SRE_CATEGORY_UNI_DIGIT:
            case SRE_CATEGORY_UNI_NOT_DIGIT:
            case SRE_CATEGORY_UNI_SPACE:
            case SRE_CATEGORY_UNI_NOT_SPACE:
            case SRE_CATEGORY_UNI_WORD:
            case SRE_CATEGORY_UNI_NOT_WORD:
            case SRE_CATEGORY_UNI_LINEBREAK:
            case SRE_CATEGORY_UNI_NOT_LINEBREAK:
                break;
            default:
                FAIL;
            }
            break;

        default:
            FAIL;

        }
    }

    return 1;
}

static int
_validate_inner(SRE_CODE *code, SRE_CODE *end, Py_ssize_t groups)
{
    /* Some variables are manipulated by the macros above */
    SRE_CODE op;
    SRE_CODE arg;
    SRE_CODE skip;

    VTRACE(("code=%p, end=%p\n", code, end));

    if (code > end)
        FAIL;

    while (code < end) {
        GET_OP;
        switch (op) {

        case SRE_OP_MARK:
            /* We don't check whether marks are properly nested; the
               sre_match() code is robust even if they don't, and the worst
               you can get is nonsensical match results. */
            GET_ARG;
            if (arg > 2 * (size_t)groups + 1) {
                VTRACE(("arg=%d, groups=%d\n", (int)arg, (int)groups));
                FAIL;
            }
            break;

        case SRE_OP_LITERAL:
        case SRE_OP_NOT_LITERAL:
        case SRE_OP_LITERAL_IGNORE:
        case SRE_OP_NOT_LITERAL_IGNORE:
            GET_ARG;
            /* The arg is just a character, nothing to check */
            break;

        case SRE_OP_SUCCESS:
        case SRE_OP_FAILURE:
            /* Nothing to check; these normally end the matching process */
            break;

        case SRE_OP_AT:
            GET_ARG;
            switch (arg) {
            case SRE_AT_BEGINNING:
            case SRE_AT_BEGINNING_STRING:
            case SRE_AT_BEGINNING_LINE:
            case SRE_AT_END:
            case SRE_AT_END_LINE:
            case SRE_AT_END_STRING:
            case SRE_AT_BOUNDARY:
            case SRE_AT_NON_BOUNDARY:
            case SRE_AT_LOC_BOUNDARY:
            case SRE_AT_LOC_NON_BOUNDARY:
            case SRE_AT_UNI_BOUNDARY:
            case SRE_AT_UNI_NON_BOUNDARY:
                break;
            default:
                FAIL;
            }
            break;

        case SRE_OP_ANY:
        case SRE_OP_ANY_ALL:
            /* These have no operands */
            break;

        case SRE_OP_IN:
        case SRE_OP_IN_IGNORE:
            GET_SKIP;
            /* Stop 1 before the end; we check the FAILURE below */
            if (!_validate_charset(code, code+skip-2))
                FAIL;
            if (code[skip-2] != SRE_OP_FAILURE)
                FAIL;
            code += skip-1;
            break;

        case SRE_OP_INFO:
            {
                /* A minimal info field is
                   <INFO> <1=skip> <2=flags> <3=min> <4=max>;
                   If SRE_INFO_PREFIX or SRE_INFO_CHARSET is in the flags,
                   more follows. */
                SRE_CODE flags, i;
                SRE_CODE *newcode;
                GET_SKIP;
                newcode = code+skip-1;
                GET_ARG; flags = arg;
                GET_ARG;
                GET_ARG;
                /* Check that only valid flags are present */
                if ((flags & ~(SRE_INFO_PREFIX |
                               SRE_INFO_LITERAL |
                               SRE_INFO_CHARSET)) != 0)
                    FAIL;
                /* PREFIX and CHARSET are mutually exclusive */
                if ((flags & SRE_INFO_PREFIX) &&
                    (flags & SRE_INFO_CHARSET))
                    FAIL;
                /* LITERAL implies PREFIX */
                if ((flags & SRE_INFO_LITERAL) &&
                    !(flags & SRE_INFO_PREFIX))
                    FAIL;
                /* Validate the prefix */
                if (flags & SRE_INFO_PREFIX) {
                    SRE_CODE prefix_len;
                    GET_ARG; prefix_len = arg;
                    GET_ARG;
                    /* Here comes the prefix string */
                    if (prefix_len > (Py_uintptr_t)(newcode - code))
                        FAIL;
                    code += prefix_len;
                    /* And here comes the overlap table */
                    if (prefix_len > (Py_uintptr_t)(newcode - code))
                        FAIL;
                    /* Each overlap value should be < prefix_len */
                    for (i = 0; i < prefix_len; i++) {
                        if (code[i] >= prefix_len)
                            FAIL;
                    }
                    code += prefix_len;
                }
                /* Validate the charset */
                if (flags & SRE_INFO_CHARSET) {
                    if (!_validate_charset(code, newcode-1))
                        FAIL;
                    if (newcode[-1] != SRE_OP_FAILURE)
                        FAIL;
                    code = newcode;
                }
                else if (code != newcode) {
                  VTRACE(("code=%p, newcode=%p\n", code, newcode));
                    FAIL;
                }
            }
            break;

        case SRE_OP_BRANCH:
            {
                SRE_CODE *target = NULL;
                for (;;) {
                    GET_SKIP;
                    if (skip == 0)
                        break;
                    /* Stop 2 before the end; we check the JUMP below */
                    if (!_validate_inner(code, code+skip-3, groups))
                        FAIL;
                    code += skip-3;
                    /* Check that it ends with a JUMP, and that each JUMP
                       has the same target */
                    GET_OP;
                    if (op != SRE_OP_JUMP)
                        FAIL;
                    GET_SKIP;
                    if (target == NULL)
                        target = code+skip-1;
                    else if (code+skip-1 != target)
                        FAIL;
                }
            }
            break;

        case SRE_OP_REPEAT_ONE:
        case SRE_OP_MIN_REPEAT_ONE:
            {
                SRE_CODE min, max;
                GET_SKIP;
                GET_ARG; min = arg;
                GET_ARG; max = arg;
                if (min > max)
                    FAIL;
                if (max > SRE_MAXREPEAT)
                    FAIL;
                if (!_validate_inner(code, code+skip-4, groups))
                    FAIL;
                code += skip-4;
                GET_OP;
                if (op != SRE_OP_SUCCESS)
                    FAIL;
            }
            break;

        case SRE_OP_REPEAT:
            {
                SRE_CODE min, max;
                GET_SKIP;
                GET_ARG; min = arg;
                GET_ARG; max = arg;
                if (min > max)
                    FAIL;
                if (max > SRE_MAXREPEAT)
                    FAIL;
                if (!_validate_inner(code, code+skip-3, groups))
                    FAIL;
                code += skip-3;
                GET_OP;
                if (op != SRE_OP_MAX_UNTIL && op != SRE_OP_MIN_UNTIL)
                    FAIL;
            }
            break;

        case SRE_OP_GROUPREF:
        case SRE_OP_GROUPREF_IGNORE:
            GET_ARG;
            if (arg >= (size_t)groups)
                FAIL;
            break;

        case SRE_OP_GROUPREF_EXISTS:
            /* The regex syntax for this is: '(?(group)then|else)', where
               'group' is either an integer group number or a group name,
               'then' and 'else' are sub-regexes, and 'else' is optional. */
            GET_ARG;
            if (arg >= (size_t)groups)
                FAIL;
            GET_SKIP_ADJ(1);
            code--; /* The skip is relative to the first arg! */
            /* There are two possibilities here: if there is both a 'then'
               part and an 'else' part, the generated code looks like:

               GROUPREF_EXISTS
               <group>
               <skipyes>
               ...then part...
               JUMP
               <skipno>
               (<skipyes> jumps here)
               ...else part...
               (<skipno> jumps here)

               If there is only a 'then' part, it looks like:

               GROUPREF_EXISTS
               <group>
               <skip>
               ...then part...
               (<skip> jumps here)

               There is no direct way to decide which it is, and we don't want
               to allow arbitrary jumps anywhere in the code; so we just look
               for a JUMP opcode preceding our skip target.
            */
            if (skip >= 3 && skip-3 < (Py_uintptr_t)(end - code) &&
                code[skip-3] == SRE_OP_JUMP)
            {
                VTRACE(("both then and else parts present\n"));
                if (!_validate_inner(code+1, code+skip-3, groups))
                    FAIL;
                code += skip-2; /* Position after JUMP, at <skipno> */
                GET_SKIP;
                if (!_validate_inner(code, code+skip-1, groups))
                    FAIL;
                code += skip-1;
            }
            else {
                VTRACE(("only a then part present\n"));
                if (!_validate_inner(code+1, code+skip-1, groups))
                    FAIL;
                code += skip-1;
            }
            break;

        case SRE_OP_ASSERT:
        case SRE_OP_ASSERT_NOT:
            GET_SKIP;
            GET_ARG; /* 0 for lookahead, width for lookbehind */
            code--; /* Back up over arg to simplify math below */
            if (arg & 0x80000000)
                FAIL; /* Width too large */
            /* Stop 1 before the end; we check the SUCCESS below */
            if (!_validate_inner(code+1, code+skip-2, groups))
                FAIL;
            code += skip-2;
            GET_OP;
            if (op != SRE_OP_SUCCESS)
                FAIL;
            break;

        default:
            FAIL;

        }
    }

    VTRACE(("okay\n"));
    return 1;
}

static int
_validate_outer(SRE_CODE *code, SRE_CODE *end, Py_ssize_t groups)
{
    if (groups < 0 || groups > 100 || code >= end || end[-1] != SRE_OP_SUCCESS)
        FAIL;
    if (groups == 0)  /* fix for simplejson */
        groups = 100; /* 100 groups should always be safe */
    return _validate_inner(code, end-1, groups);
}

static int
_validate(PatternObject *self)
{
    if (!_validate_outer(self->code, self->code+self->codesize, self->groups))
    {
        PyErr_SetString(PyExc_RuntimeError, "invalid SRE code");
        return 0;
    }
    else
        VTRACE(("Success!\n"));
    return 1;
}

/* -------------------------------------------------------------------- */
/* match methods */

static void
match_dealloc(MatchObject* self)
{
    Py_XDECREF(self->regs);
    Py_XDECREF(self->string);
    Py_DECREF(self->pattern);
    PyObject_DEL(self);
}

static PyObject*
match_getslice_by_index(MatchObject* self, Py_ssize_t index, PyObject* def)
{
    Py_ssize_t length;
    int isbytes, charsize;
    Py_buffer view;
    PyObject *result;
    void* ptr;

    if (index < 0 || index >= self->groups) {
        /* raise IndexError if we were given a bad group number */
        PyErr_SetString(
            PyExc_IndexError,
            "no such group"
            );
        return NULL;
    }

    index *= 2;

    if (self->string == Py_None || self->mark[index] < 0) {
        /* return default value if the string or group is undefined */
        Py_INCREF(def);
        return def;
    }

    ptr = getstring(self->string, &length, &isbytes, &charsize, &view);
    if (ptr == NULL)
        return NULL;
    result = getslice(isbytes, ptr,
                      self->string, self->mark[index], self->mark[index+1]);
    if (isbytes && view.buf != NULL)
        PyBuffer_Release(&view);
    return result;
}

static Py_ssize_t
match_getindex(MatchObject* self, PyObject* index)
{
    Py_ssize_t i;

    if (index == NULL)
        /* Default value */
        return 0;

    if (PyLong_Check(index))
        return PyLong_AsSsize_t(index);

    i = -1;

    if (self->pattern->groupindex) {
        index = PyObject_GetItem(self->pattern->groupindex, index);
        if (index) {
            if (PyLong_Check(index))
                i = PyLong_AsSsize_t(index);
            Py_DECREF(index);
        } else
            PyErr_Clear();
    }

    return i;
}

static PyObject*
match_getslice(MatchObject* self, PyObject* index, PyObject* def)
{
    return match_getslice_by_index(self, match_getindex(self, index), def);
}

static PyObject*
match_expand(MatchObject* self, PyObject* ptemplate)
{
    /* delegate to Python code */
    return call(
        SRE_PY_MODULE, "_expand",
        PyTuple_Pack(3, self->pattern, self, ptemplate)
        );
}

static PyObject*
match_group(MatchObject* self, PyObject* args)
{
    PyObject* result;
    Py_ssize_t i, size;

    size = PyTuple_GET_SIZE(args);

    switch (size) {
    case 0:
        result = match_getslice(self, Py_False, Py_None);
        break;
    case 1:
        result = match_getslice(self, PyTuple_GET_ITEM(args, 0), Py_None);
        break;
    default:
        /* fetch multiple items */
        result = PyTuple_New(size);
        if (!result)
            return NULL;
        for (i = 0; i < size; i++) {
            PyObject* item = match_getslice(
                self, PyTuple_GET_ITEM(args, i), Py_None
                );
            if (!item) {
                Py_DECREF(result);
                return NULL;
            }
            PyTuple_SET_ITEM(result, i, item);
        }
        break;
    }
    return result;
}

static PyObject*
match_groups(MatchObject* self, PyObject* args, PyObject* kw)
{
    PyObject* result;
    Py_ssize_t index;

    PyObject* def = Py_None;
    static char* kwlist[] = { "default", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O:groups", kwlist, &def))
        return NULL;

    result = PyTuple_New(self->groups-1);
    if (!result)
        return NULL;

    for (index = 1; index < self->groups; index++) {
        PyObject* item;
        item = match_getslice_by_index(self, index, def);
        if (!item) {
            Py_DECREF(result);
            return NULL;
        }
        PyTuple_SET_ITEM(result, index-1, item);
    }

    return result;
}

static PyObject*
match_groupdict(MatchObject* self, PyObject* args, PyObject* kw)
{
    PyObject* result;
    PyObject* keys;
    Py_ssize_t index;

    PyObject* def = Py_None;
    static char* kwlist[] = { "default", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|O:groupdict", kwlist, &def))
        return NULL;

    result = PyDict_New();
    if (!result || !self->pattern->groupindex)
        return result;

    keys = PyMapping_Keys(self->pattern->groupindex);
    if (!keys)
        goto failed;

    for (index = 0; index < PyList_GET_SIZE(keys); index++) {
        int status;
        PyObject* key;
        PyObject* value;
        key = PyList_GET_ITEM(keys, index);
        if (!key)
            goto failed;
        value = match_getslice(self, key, def);
        if (!value) {
            Py_DECREF(key);
            goto failed;
        }
        status = PyDict_SetItem(result, key, value);
        Py_DECREF(value);
        if (status < 0)
            goto failed;
    }

    Py_DECREF(keys);

    return result;

failed:
    Py_XDECREF(keys);
    Py_DECREF(result);
    return NULL;
}

static PyObject*
match_start(MatchObject* self, PyObject* args)
{
    Py_ssize_t index;

    PyObject* index_ = NULL;
    if (!PyArg_UnpackTuple(args, "start", 0, 1, &index_))
        return NULL;

    index = match_getindex(self, index_);

    if (index < 0 || index >= self->groups) {
        PyErr_SetString(
            PyExc_IndexError,
            "no such group"
            );
        return NULL;
    }

    /* mark is -1 if group is undefined */
    return PyLong_FromSsize_t(self->mark[index*2]);
}

static PyObject*
match_end(MatchObject* self, PyObject* args)
{
    Py_ssize_t index;

    PyObject* index_ = NULL;
    if (!PyArg_UnpackTuple(args, "end", 0, 1, &index_))
        return NULL;

    index = match_getindex(self, index_);

    if (index < 0 || index >= self->groups) {
        PyErr_SetString(
            PyExc_IndexError,
            "no such group"
            );
        return NULL;
    }

    /* mark is -1 if group is undefined */
    return PyLong_FromSsize_t(self->mark[index*2+1]);
}

LOCAL(PyObject*)
_pair(Py_ssize_t i1, Py_ssize_t i2)
{
    PyObject* pair;
    PyObject* item;

    pair = PyTuple_New(2);
    if (!pair)
        return NULL;

    item = PyLong_FromSsize_t(i1);
    if (!item)
        goto error;
    PyTuple_SET_ITEM(pair, 0, item);

    item = PyLong_FromSsize_t(i2);
    if (!item)
        goto error;
    PyTuple_SET_ITEM(pair, 1, item);

    return pair;

  error:
    Py_DECREF(pair);
    return NULL;
}

static PyObject*
match_span(MatchObject* self, PyObject* args)
{
    Py_ssize_t index;

    PyObject* index_ = NULL;
    if (!PyArg_UnpackTuple(args, "span", 0, 1, &index_))
        return NULL;

    index = match_getindex(self, index_);

    if (index < 0 || index >= self->groups) {
        PyErr_SetString(
            PyExc_IndexError,
            "no such group"
            );
        return NULL;
    }

    /* marks are -1 if group is undefined */
    return _pair(self->mark[index*2], self->mark[index*2+1]);
}

static PyObject*
match_regs(MatchObject* self)
{
    PyObject* regs;
    PyObject* item;
    Py_ssize_t index;

    regs = PyTuple_New(self->groups);
    if (!regs)
        return NULL;

    for (index = 0; index < self->groups; index++) {
        item = _pair(self->mark[index*2], self->mark[index*2+1]);
        if (!item) {
            Py_DECREF(regs);
            return NULL;
        }
        PyTuple_SET_ITEM(regs, index, item);
    }

    Py_INCREF(regs);
    self->regs = regs;

    return regs;
}

static PyObject*
match_copy(MatchObject* self, PyObject *unused)
{
#ifdef USE_BUILTIN_COPY
    MatchObject* copy;
    Py_ssize_t slots, offset;

    slots = 2 * (self->pattern->groups+1);

    copy = PyObject_NEW_VAR(MatchObject, &Match_Type, slots);
    if (!copy)
        return NULL;

    /* this value a constant, but any compiler should be able to
       figure that out all by itself */
    offset = offsetof(MatchObject, string);

    Py_XINCREF(self->pattern);
    Py_XINCREF(self->string);
    Py_XINCREF(self->regs);

    memcpy((char*) copy + offset, (char*) self + offset,
           sizeof(MatchObject) + slots * sizeof(Py_ssize_t) - offset);

    return (PyObject*) copy;
#else
    PyErr_SetString(PyExc_TypeError, "cannot copy this match object");
    return NULL;
#endif
}

static PyObject*
match_deepcopy(MatchObject* self, PyObject* memo)
{
#ifdef USE_BUILTIN_COPY
    MatchObject* copy;

    copy = (MatchObject*) match_copy(self);
    if (!copy)
        return NULL;

    if (!deepcopy((PyObject**) &copy->pattern, memo) ||
        !deepcopy(&copy->string, memo) ||
        !deepcopy(&copy->regs, memo)) {
        Py_DECREF(copy);
        return NULL;
    }

#else
    PyErr_SetString(PyExc_TypeError, "cannot deepcopy this match object");
    return NULL;
#endif
}

PyDoc_STRVAR(match_doc,
"The result of re.match() and re.search().\n\
Match objects always have a boolean value of True.");

PyDoc_STRVAR(match_group_doc,
"group([group1, ...]) -> str or tuple.\n\
    Return subgroup(s) of the match by indices or names.\n\
    For 0 returns the entire match.");

PyDoc_STRVAR(match_start_doc,
"start([group=0]) -> int.\n\
    Return index of the start of the substring matched by group.");

PyDoc_STRVAR(match_end_doc,
"end([group=0]) -> int.\n\
    Return index of the end of the substring matched by group.");

PyDoc_STRVAR(match_span_doc,
"span([group]) -> tuple.\n\
    For MatchObject m, return the 2-tuple (m.start(group), m.end(group)).");

PyDoc_STRVAR(match_groups_doc,
"groups([default=None]) -> tuple.\n\
    Return a tuple containing all the subgroups of the match, from 1.\n\
    The default argument is used for groups\n\
    that did not participate in the match");

PyDoc_STRVAR(match_groupdict_doc,
"groupdict([default=None]) -> dict.\n\
    Return a dictionary containing all the named subgroups of the match,\n\
    keyed by the subgroup name. The default argument is used for groups\n\
    that did not participate in the match");

PyDoc_STRVAR(match_expand_doc,
"expand(template) -> str.\n\
    Return the string obtained by doing backslash substitution\n\
    on the string template, as done by the sub() method.");

static PyMethodDef match_methods[] = {
    {"group", (PyCFunction) match_group, METH_VARARGS, match_group_doc},
    {"start", (PyCFunction) match_start, METH_VARARGS, match_start_doc},
    {"end", (PyCFunction) match_end, METH_VARARGS, match_end_doc},
    {"span", (PyCFunction) match_span, METH_VARARGS, match_span_doc},
    {"groups", (PyCFunction) match_groups, METH_VARARGS|METH_KEYWORDS,
        match_groups_doc},
    {"groupdict", (PyCFunction) match_groupdict, METH_VARARGS|METH_KEYWORDS,
        match_groupdict_doc},
    {"expand", (PyCFunction) match_expand, METH_O, match_expand_doc},
    {"__copy__", (PyCFunction) match_copy, METH_NOARGS},
    {"__deepcopy__", (PyCFunction) match_deepcopy, METH_O},
    {NULL, NULL}
};

static PyObject *
match_lastindex_get(MatchObject *self)
{
    if (self->lastindex >= 0)
        return PyLong_FromSsize_t(self->lastindex);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
match_lastgroup_get(MatchObject *self)
{
    if (self->pattern->indexgroup && self->lastindex >= 0) {
        PyObject* result = PySequence_GetItem(
            self->pattern->indexgroup, self->lastindex
            );
        if (result)
            return result;
        PyErr_Clear();
    }
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
match_regs_get(MatchObject *self)
{
    if (self->regs) {
        Py_INCREF(self->regs);
        return self->regs;
    } else
        return match_regs(self);
}

static PyObject *
match_repr(MatchObject *self)
{
    PyObject *result;
    PyObject *group0 = match_getslice_by_index(self, 0, Py_None);
    if (group0 == NULL)
        return NULL;
    result = PyUnicode_FromFormat(
            "<%s object; span=(%d, %d), match=%.50R>",
            Py_TYPE(self)->tp_name,
            self->mark[0], self->mark[1], group0);
    Py_DECREF(group0);
    return result;
}


static PyGetSetDef match_getset[] = {
    {"lastindex", (getter)match_lastindex_get, (setter)NULL},
    {"lastgroup", (getter)match_lastgroup_get, (setter)NULL},
    {"regs",      (getter)match_regs_get,      (setter)NULL},
    {NULL}
};

#define MATCH_OFF(x) offsetof(MatchObject, x)
static PyMemberDef match_members[] = {
    {"string",  T_OBJECT,   MATCH_OFF(string),  READONLY},
    {"re",      T_OBJECT,   MATCH_OFF(pattern), READONLY},
    {"pos",     T_PYSSIZET, MATCH_OFF(pos),     READONLY},
    {"endpos",  T_PYSSIZET, MATCH_OFF(endpos),  READONLY},
    {NULL}
};

/* FIXME: implement setattr("string", None) as a special case (to
   detach the associated string, if any */

static PyTypeObject Match_Type = {
    PyVarObject_HEAD_INIT(NULL,0)
    "_" SRE_MODULE ".SRE_Match",
    sizeof(MatchObject), sizeof(Py_ssize_t),
    (destructor)match_dealloc,  /* tp_dealloc */
    0,                          /* tp_print */
    0,                          /* tp_getattr */
    0,                          /* tp_setattr */
    0,                          /* tp_reserved */
    (reprfunc)match_repr,       /* tp_repr */
    0,                          /* tp_as_number */
    0,                          /* tp_as_sequence */
    0,                          /* tp_as_mapping */
    0,                          /* tp_hash */
    0,                          /* tp_call */
    0,                          /* tp_str */
    0,                          /* tp_getattro */
    0,                          /* tp_setattro */
    0,                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,         /* tp_flags */
    match_doc,                  /* tp_doc */
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    match_methods,              /* tp_methods */
    match_members,              /* tp_members */
    match_getset,               /* tp_getset */
};

static PyObject*
pattern_new_match(PatternObject* pattern, SRE_STATE* state, Py_ssize_t status)
{
    /* create match object (from state object) */

    MatchObject* match;
    Py_ssize_t i, j;
    char* base;
    int n;

    if (status > 0) {

        /* create match object (with room for extra group marks) */
        /* coverity[ampersand_in_size] */
        match = PyObject_NEW_VAR(MatchObject, &Match_Type,
                                 2*(pattern->groups+1));
        if (!match)
            return NULL;

        Py_INCREF(pattern);
        match->pattern = pattern;

        Py_INCREF(state->string);
        match->string = state->string;

        match->regs = NULL;
        match->groups = pattern->groups+1;

        /* fill in group slices */

        base = (char*) state->beginning;
        n = state->charsize;

        match->mark[0] = ((char*) state->start - base) / n;
        match->mark[1] = ((char*) state->ptr - base) / n;

        for (i = j = 0; i < pattern->groups; i++, j+=2)
            if (j+1 <= state->lastmark && state->mark[j] && state->mark[j+1]) {
                match->mark[j+2] = ((char*) state->mark[j] - base) / n;
                match->mark[j+3] = ((char*) state->mark[j+1] - base) / n;
            } else
                match->mark[j+2] = match->mark[j+3] = -1; /* undefined */

        match->pos = state->pos;
        match->endpos = state->endpos;

        match->lastindex = state->lastindex;

        return (PyObject*) match;

    } else if (status == 0) {

        /* no match */
        Py_INCREF(Py_None);
        return Py_None;

    }

    /* internal error */
    pattern_error(status);
    return NULL;
}


/* -------------------------------------------------------------------- */
/* scanner methods (experimental) */

static void
scanner_dealloc(ScannerObject* self)
{
    state_fini(&self->state);
    Py_XDECREF(self->pattern);
    PyObject_DEL(self);
}

static PyObject*
scanner_match(ScannerObject* self, PyObject *unused)
{
    SRE_STATE* state = &self->state;
    PyObject* match;
    Py_ssize_t status;

    state_reset(state);

    state->ptr = state->start;

    status = sre_match(state, PatternObject_GetCode(self->pattern), 0);
    if (PyErr_Occurred())
        return NULL;

    match = pattern_new_match((PatternObject*) self->pattern,
                               state, status);

    if (status == 0 || state->ptr == state->start)
        state->start = (void*) ((char*) state->ptr + state->charsize);
    else
        state->start = state->ptr;

    return match;
}


static PyObject*
scanner_search(ScannerObject* self, PyObject *unused)
{
    SRE_STATE* state = &self->state;
    PyObject* match;
    Py_ssize_t status;

    state_reset(state);

    state->ptr = state->start;

    status = sre_search(state, PatternObject_GetCode(self->pattern));
    if (PyErr_Occurred())
        return NULL;

    match = pattern_new_match((PatternObject*) self->pattern,
                               state, status);

    if (status == 0 || state->ptr == state->start)
        state->start = (void*) ((char*) state->ptr + state->charsize);
    else
        state->start = state->ptr;

    return match;
}

static PyMethodDef scanner_methods[] = {
    {"match", (PyCFunction) scanner_match, METH_NOARGS},
    {"search", (PyCFunction) scanner_search, METH_NOARGS},
    {NULL, NULL}
};

#define SCAN_OFF(x) offsetof(ScannerObject, x)
static PyMemberDef scanner_members[] = {
    {"pattern", T_OBJECT, SCAN_OFF(pattern), READONLY},
    {NULL}  /* Sentinel */
};

static PyTypeObject Scanner_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_" SRE_MODULE ".SRE_Scanner",
    sizeof(ScannerObject), 0,
    (destructor)scanner_dealloc,/* tp_dealloc */
    0,                          /* tp_print */
    0,                          /* tp_getattr */
    0,                          /* tp_setattr */
    0,                          /* tp_reserved */
    0,                          /* tp_repr */
    0,                          /* tp_as_number */
    0,                          /* tp_as_sequence */
    0,                          /* tp_as_mapping */
    0,                          /* tp_hash */
    0,                          /* tp_call */
    0,                          /* tp_str */
    0,                          /* tp_getattro */
    0,                          /* tp_setattro */
    0,                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,         /* tp_flags */
    0,                          /* tp_doc */
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    scanner_methods,            /* tp_methods */
    scanner_members,            /* tp_members */
    0,                          /* tp_getset */
};

static PyObject*
pattern_scanner(PatternObject* pattern, PyObject* args, PyObject* kw)
{
    /* create search state object */

    ScannerObject* self;

    PyObject *string = NULL, *string2 = NULL;
    Py_ssize_t start = 0;
    Py_ssize_t end = PY_SSIZE_T_MAX;
    static char* kwlist[] = { "string", "pos", "endpos", "source", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kw, "|Onn$O:scanner", kwlist,
                                     &string, &start, &end, &string2))
        return NULL;

    string = fix_string_param(string, string2, "source");
    if (!string)
        return NULL;

    /* create scanner object */
    self = PyObject_NEW(ScannerObject, &Scanner_Type);
    if (!self)
        return NULL;
    self->pattern = NULL;

    string = state_init(&self->state, pattern, string, start, end);
    if (!string) {
        Py_DECREF(self);
        return NULL;
    }

    Py_INCREF(pattern);
    self->pattern = (PyObject*) pattern;

    return (PyObject*) self;
}

static PyMethodDef _functions[] = {
    {"compile", _compile, METH_VARARGS},
    {"getcodesize", sre_codesize, METH_NOARGS},
    {"getlower", sre_getlower, METH_VARARGS},
    {NULL, NULL}
};

static struct PyModuleDef sremodule = {
        PyModuleDef_HEAD_INIT,
        "_" SRE_MODULE,
        NULL,
        -1,
        _functions,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC PyInit__sre(void)
{
    PyObject* m;
    PyObject* d;
    PyObject* x;

    /* Patch object types */
    if (PyType_Ready(&Pattern_Type) || PyType_Ready(&Match_Type) ||
        PyType_Ready(&Scanner_Type))
        return NULL;

    m = PyModule_Create(&sremodule);
    if (m == NULL)
        return NULL;
    d = PyModule_GetDict(m);

    x = PyLong_FromLong(SRE_MAGIC);
    if (x) {
        PyDict_SetItemString(d, "MAGIC", x);
        Py_DECREF(x);
    }

    x = PyLong_FromLong(sizeof(SRE_CODE));
    if (x) {
        PyDict_SetItemString(d, "CODESIZE", x);
        Py_DECREF(x);
    }

    x = PyLong_FromUnsignedLong(SRE_MAXREPEAT);
    if (x) {
        PyDict_SetItemString(d, "MAXREPEAT", x);
        Py_DECREF(x);
    }

    x = PyUnicode_FromString(copyright);
    if (x) {
        PyDict_SetItemString(d, "copyright", x);
        Py_DECREF(x);
    }
    return m;
}

/* vim:ts=4:sw=4:et
*/
