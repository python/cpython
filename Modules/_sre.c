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
 * 2001-10-20 fl   added split primitive; re-enable unicode for 1.6/2.0/2.1
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

static const char copyright[] =
    " SRE 2.2.2 Copyright (c) 1997-2002 by Secret Labs AB ";

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "pycore_long.h"          // _PyLong_GetZero()
#include "pycore_moduleobject.h"  // _PyModule_GetState()
#include "structmember.h"         // PyMemberDef

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

#if defined(_MSC_VER)
#pragma optimize("agtw", on) /* doesn't seem to make much difference... */
#pragma warning(disable: 4710) /* who cares if functions are not inlined ;-) */
/* fastest possible local call under MSVC */
#define LOCAL(type) static __inline type __fastcall
#else
#define LOCAL(type) static inline type
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

#define SRE_IS_DIGIT(ch)\
    ((ch) <= '9' && Py_ISDIGIT(ch))
#define SRE_IS_SPACE(ch)\
    ((ch) <= ' ' && Py_ISSPACE(ch))
#define SRE_IS_LINEBREAK(ch)\
    ((ch) == '\n')
#define SRE_IS_WORD(ch)\
    ((ch) <= 'z' && (Py_ISALNUM(ch) || (ch) == '_'))

static unsigned int sre_lower_ascii(unsigned int ch)
{
    return ((ch) < 128 ? Py_TOLOWER(ch) : ch);
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

static unsigned int sre_upper_locale(unsigned int ch)
{
    return ((ch) < 256 ? (unsigned int)toupper((ch)) : ch);
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

static unsigned int sre_upper_unicode(unsigned int ch)
{
    return (unsigned int) Py_UNICODE_TOUPPER(ch);
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

LOCAL(int)
char_loc_ignore(SRE_CODE pattern, SRE_CODE ch)
{
    return ch == pattern
        || (SRE_CODE) sre_lower_locale(ch) == pattern
        || (SRE_CODE) sre_upper_locale(ch) == pattern;
}


/* helpers */

static void
data_stack_dealloc(SRE_STATE* state)
{
    if (state->data_stack) {
        PyMem_Free(state->data_stack);
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
        TRACE(("allocate/grow stack %zd\n", cursize));
        stack = PyMem_Realloc(state->data_stack, cursize);
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

/* module state */
typedef struct {
    PyTypeObject *Pattern_Type;
    PyTypeObject *Match_Type;
    PyTypeObject *Scanner_Type;
} _sremodulestate;

static _sremodulestate *
get_sre_module_state(PyObject *m)
{
    _sremodulestate *state = (_sremodulestate *)_PyModule_GetState(m);
    assert(state);
    return state;
}

static struct PyModuleDef sremodule;
#define get_sre_module_state_by_class(cls) \
    (get_sre_module_state(PyType_GetModule(cls)))

/* see sre.h for object declarations */
static PyObject*pattern_new_match(_sremodulestate *, PatternObject*, SRE_STATE*, Py_ssize_t);
static PyObject *pattern_scanner(_sremodulestate *, PatternObject *, PyObject *, Py_ssize_t, Py_ssize_t);

/*[clinic input]
module _sre
class _sre.SRE_Pattern "PatternObject *" "get_sre_module_state_by_class(tp)->Pattern_Type"
class _sre.SRE_Match "MatchObject *" "get_sre_module_state_by_class(tp)->Match_Type"
class _sre.SRE_Scanner "ScannerObject *" "get_sre_module_state_by_class(tp)->Scanner_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=fe2966e32b66a231]*/

/*[clinic input]
_sre.getcodesize -> int
[clinic start generated code]*/

static int
_sre_getcodesize_impl(PyObject *module)
/*[clinic end generated code: output=e0db7ce34a6dd7b1 input=bd6f6ecf4916bb2b]*/
{
    return sizeof(SRE_CODE);
}

/*[clinic input]
_sre.ascii_iscased -> bool

    character: int
    /

[clinic start generated code]*/

static int
_sre_ascii_iscased_impl(PyObject *module, int character)
/*[clinic end generated code: output=4f454b630fbd19a2 input=9f0bd952812c7ed3]*/
{
    unsigned int ch = (unsigned int)character;
    return ch < 128 && Py_ISALPHA(ch);
}

/*[clinic input]
_sre.unicode_iscased -> bool

    character: int
    /

[clinic start generated code]*/

static int
_sre_unicode_iscased_impl(PyObject *module, int character)
/*[clinic end generated code: output=9c5ddee0dc2bc258 input=51e42c3b8dddb78e]*/
{
    unsigned int ch = (unsigned int)character;
    return ch != sre_lower_unicode(ch) || ch != sre_upper_unicode(ch);
}

/*[clinic input]
_sre.ascii_tolower -> int

    character: int
    /

[clinic start generated code]*/

static int
_sre_ascii_tolower_impl(PyObject *module, int character)
/*[clinic end generated code: output=228294ed6ff2a612 input=272c609b5b61f136]*/
{
    return sre_lower_ascii(character);
}

/*[clinic input]
_sre.unicode_tolower -> int

    character: int
    /

[clinic start generated code]*/

static int
_sre_unicode_tolower_impl(PyObject *module, int character)
/*[clinic end generated code: output=6422272d7d7fee65 input=91d708c5f3c2045a]*/
{
    return sre_lower_unicode(character);
}

LOCAL(void)
state_reset(SRE_STATE* state)
{
    /* state->mark will be set to 0 in SRE_OP_MARK dynamically. */
    /*memset(state->mark, 0, sizeof(*state->mark) * SRE_MARK_SIZE);*/

    state->lastmark = -1;
    state->lastindex = -1;

    state->repeat = NULL;

    data_stack_dealloc(state);
}

static const void*
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
        PyErr_SetString(PyExc_TypeError, "expected string or bytes-like object");
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
    const void* ptr;

    memset(state, 0, sizeof(SRE_STATE));

    state->mark = PyMem_New(const void *, pattern->groups * 2);
    if (!state->mark) {
        PyErr_NoMemory();
        goto err;
    }
    state->lastmark = -1;
    state->lastindex = -1;

    state->buffer.buf = NULL;
    ptr = getstring(string, &length, &isbytes, &charsize, &state->buffer);
    if (!ptr)
        goto err;

    if (isbytes && pattern->isbytes == 0) {
        PyErr_SetString(PyExc_TypeError,
                        "cannot use a string pattern on a bytes-like object");
        goto err;
    }
    if (!isbytes && pattern->isbytes > 0) {
        PyErr_SetString(PyExc_TypeError,
                        "cannot use a bytes pattern on a string-like object");
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
    state->match_all = 0;
    state->must_advance = 0;

    state->beginning = ptr;

    state->start = (void*) ((char*) ptr + start * state->charsize);
    state->end = (void*) ((char*) ptr + end * state->charsize);

    Py_INCREF(string);
    state->string = string;
    state->pos = start;
    state->endpos = end;

    return string;
  err:
    /* We add an explicit cast here because MSVC has a bug when
       compiling C code where it believes that `const void**` cannot be
       safely casted to `void*`, see bpo-39943 for details. */
    PyMem_Free((void*) state->mark);
    state->mark = NULL;
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
    /* See above PyMem_Del for why we explicitly cast here. */
    PyMem_Free((void*) state->mark);
    state->mark = NULL;
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
            Py_RETURN_NONE;
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
        /* This error code seems to be unused. */
        PyErr_SetString(
            PyExc_RecursionError,
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

static int
pattern_traverse(PatternObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->groupindex);
    Py_VISIT(self->indexgroup);
    Py_VISIT(self->pattern);
    return 0;
}

static int
pattern_clear(PatternObject *self)
{
    Py_CLEAR(self->groupindex);
    Py_CLEAR(self->indexgroup);
    Py_CLEAR(self->pattern);
    return 0;
}

static void
pattern_dealloc(PatternObject* self)
{
    PyTypeObject *tp = Py_TYPE(self);

    PyObject_GC_UnTrack(self);
    if (self->weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *) self);
    }
    (void)pattern_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

LOCAL(Py_ssize_t)
sre_match(SRE_STATE* state, SRE_CODE* pattern)
{
    if (state->charsize == 1)
        return sre_ucs1_match(state, pattern, 1);
    if (state->charsize == 2)
        return sre_ucs2_match(state, pattern, 1);
    assert(state->charsize == 4);
    return sre_ucs4_match(state, pattern, 1);
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

/*[clinic input]
_sre.SRE_Pattern.match

    cls: defining_class
    /
    string: object
    pos: Py_ssize_t = 0
    endpos: Py_ssize_t(c_default="PY_SSIZE_T_MAX") = sys.maxsize

Matches zero or more characters at the beginning of the string.
[clinic start generated code]*/

static PyObject *
_sre_SRE_Pattern_match_impl(PatternObject *self, PyTypeObject *cls,
                            PyObject *string, Py_ssize_t pos,
                            Py_ssize_t endpos)
/*[clinic end generated code: output=ec6208ea58a0cca0 input=4bdb9c3e564d13ac]*/
{
    _sremodulestate *module_state = get_sre_module_state_by_class(cls);
    SRE_STATE state;
    Py_ssize_t status;
    PyObject *match;

    if (!state_init(&state, (PatternObject *)self, string, pos, endpos))
        return NULL;

    state.ptr = state.start;

    TRACE(("|%p|%p|MATCH\n", PatternObject_GetCode(self), state.ptr));

    status = sre_match(&state, PatternObject_GetCode(self));

    TRACE(("|%p|%p|END\n", PatternObject_GetCode(self), state.ptr));
    if (PyErr_Occurred()) {
        state_fini(&state);
        return NULL;
    }

    match = pattern_new_match(module_state, self, &state, status);
    state_fini(&state);
    return match;
}

/*[clinic input]
_sre.SRE_Pattern.fullmatch

    cls: defining_class
    /
    string: object
    pos: Py_ssize_t = 0
    endpos: Py_ssize_t(c_default="PY_SSIZE_T_MAX") = sys.maxsize

Matches against all of the string.
[clinic start generated code]*/

static PyObject *
_sre_SRE_Pattern_fullmatch_impl(PatternObject *self, PyTypeObject *cls,
                                PyObject *string, Py_ssize_t pos,
                                Py_ssize_t endpos)
/*[clinic end generated code: output=625b75b027ef94da input=50981172ab0fcfdd]*/
{
    _sremodulestate *module_state = get_sre_module_state_by_class(cls);
    SRE_STATE state;
    Py_ssize_t status;
    PyObject *match;

    if (!state_init(&state, self, string, pos, endpos))
        return NULL;

    state.ptr = state.start;

    TRACE(("|%p|%p|FULLMATCH\n", PatternObject_GetCode(self), state.ptr));

    state.match_all = 1;
    status = sre_match(&state, PatternObject_GetCode(self));

    TRACE(("|%p|%p|END\n", PatternObject_GetCode(self), state.ptr));
    if (PyErr_Occurred()) {
        state_fini(&state);
        return NULL;
    }

    match = pattern_new_match(module_state, self, &state, status);
    state_fini(&state);
    return match;
}

/*[clinic input]
_sre.SRE_Pattern.search

    cls: defining_class
    /
    string: object
    pos: Py_ssize_t = 0
    endpos: Py_ssize_t(c_default="PY_SSIZE_T_MAX") = sys.maxsize

Scan through string looking for a match, and return a corresponding match object instance.

Return None if no position in the string matches.
[clinic start generated code]*/

static PyObject *
_sre_SRE_Pattern_search_impl(PatternObject *self, PyTypeObject *cls,
                             PyObject *string, Py_ssize_t pos,
                             Py_ssize_t endpos)
/*[clinic end generated code: output=bd7f2d9d583e1463 input=afa9afb66a74a4b3]*/
{
    _sremodulestate *module_state = get_sre_module_state_by_class(cls);
    SRE_STATE state;
    Py_ssize_t status;
    PyObject *match;

    if (!state_init(&state, self, string, pos, endpos))
        return NULL;

    TRACE(("|%p|%p|SEARCH\n", PatternObject_GetCode(self), state.ptr));

    status = sre_search(&state, PatternObject_GetCode(self));

    TRACE(("|%p|%p|END\n", PatternObject_GetCode(self), state.ptr));

    if (PyErr_Occurred()) {
        state_fini(&state);
        return NULL;
    }

    match = pattern_new_match(module_state, self, &state, status);
    state_fini(&state);
    return match;
}

static PyObject*
call(const char* module, const char* function, PyObject* args)
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

/*[clinic input]
_sre.SRE_Pattern.findall

    string: object
    pos: Py_ssize_t = 0
    endpos: Py_ssize_t(c_default="PY_SSIZE_T_MAX") = sys.maxsize

Return a list of all non-overlapping matches of pattern in string.
[clinic start generated code]*/

static PyObject *
_sre_SRE_Pattern_findall_impl(PatternObject *self, PyObject *string,
                              Py_ssize_t pos, Py_ssize_t endpos)
/*[clinic end generated code: output=f4966baceea60aca input=5b6a4ee799741563]*/
{
    SRE_STATE state;
    PyObject* list;
    Py_ssize_t status;
    Py_ssize_t i, b, e;

    if (!state_init(&state, self, string, pos, endpos))
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

        state.must_advance = (state.ptr == state.start);
        state.start = state.ptr;
    }

    state_fini(&state);
    return list;

error:
    Py_DECREF(list);
    state_fini(&state);
    return NULL;

}

/*[clinic input]
_sre.SRE_Pattern.finditer

    cls: defining_class
    /
    string: object
    pos: Py_ssize_t = 0
    endpos: Py_ssize_t(c_default="PY_SSIZE_T_MAX") = sys.maxsize

Return an iterator over all non-overlapping matches for the RE pattern in string.

For each match, the iterator returns a match object.
[clinic start generated code]*/

static PyObject *
_sre_SRE_Pattern_finditer_impl(PatternObject *self, PyTypeObject *cls,
                               PyObject *string, Py_ssize_t pos,
                               Py_ssize_t endpos)
/*[clinic end generated code: output=1791dbf3618ade56 input=812e332a4848cbaf]*/
{
    _sremodulestate *module_state = get_sre_module_state_by_class(cls);
    PyObject* scanner;
    PyObject* search;
    PyObject* iterator;

    scanner = pattern_scanner(module_state, self, string, pos, endpos);
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

/*[clinic input]
_sre.SRE_Pattern.scanner

    cls: defining_class
    /
    string: object
    pos: Py_ssize_t = 0
    endpos: Py_ssize_t(c_default="PY_SSIZE_T_MAX") = sys.maxsize

[clinic start generated code]*/

static PyObject *
_sre_SRE_Pattern_scanner_impl(PatternObject *self, PyTypeObject *cls,
                              PyObject *string, Py_ssize_t pos,
                              Py_ssize_t endpos)
/*[clinic end generated code: output=f70cd506112f1bd9 input=2e487e5151bcee4c]*/
{
    _sremodulestate *module_state = get_sre_module_state_by_class(cls);

    return pattern_scanner(module_state, self, string, pos, endpos);
}

/*[clinic input]
_sre.SRE_Pattern.split

    string: object
    maxsplit: Py_ssize_t = 0

Split string by the occurrences of pattern.
[clinic start generated code]*/

static PyObject *
_sre_SRE_Pattern_split_impl(PatternObject *self, PyObject *string,
                            Py_ssize_t maxsplit)
/*[clinic end generated code: output=7ac66f381c45e0be input=1eeeb10dafc9947a]*/
{
    SRE_STATE state;
    PyObject* list;
    PyObject* item;
    Py_ssize_t status;
    Py_ssize_t n;
    Py_ssize_t i;
    const void* last;

    assert(self->codesize != 0);

    if (!state_init(&state, self, string, 0, PY_SSIZE_T_MAX))
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
        state.must_advance = (state.ptr == state.start);
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
pattern_subx(_sremodulestate* module_state,
             PatternObject* self,
             PyObject* ptemplate,
             PyObject* string,
             Py_ssize_t count,
             Py_ssize_t subn)
{
    SRE_STATE state;
    PyObject* list;
    PyObject* joiner;
    PyObject* item;
    PyObject* filter;
    PyObject* match;
    const void* ptr;
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

    if (!state_init(&state, self, string, 0, PY_SSIZE_T_MAX)) {
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

        }

        if (filter_is_callable) {
            /* pass match object through filter */
            match = pattern_new_match(module_state, self, &state, 1);
            if (!match)
                goto error;
            item = PyObject_CallOneArg(filter, match);
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
        state.must_advance = (state.ptr == state.start);
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

/*[clinic input]
_sre.SRE_Pattern.sub

    cls: defining_class
    /
    repl: object
    string: object
    count: Py_ssize_t = 0

Return the string obtained by replacing the leftmost non-overlapping occurrences of pattern in string by the replacement repl.
[clinic start generated code]*/

static PyObject *
_sre_SRE_Pattern_sub_impl(PatternObject *self, PyTypeObject *cls,
                          PyObject *repl, PyObject *string, Py_ssize_t count)
/*[clinic end generated code: output=4be141ab04bca60d input=d8d1d4ac2311a07c]*/
{
    _sremodulestate *module_state = get_sre_module_state_by_class(cls);

    return pattern_subx(module_state, self, repl, string, count, 0);
}

/*[clinic input]
_sre.SRE_Pattern.subn

    cls: defining_class
    /
    repl: object
    string: object
    count: Py_ssize_t = 0

Return the tuple (new_string, number_of_subs_made) found by replacing the leftmost non-overlapping occurrences of pattern with the replacement repl.
[clinic start generated code]*/

static PyObject *
_sre_SRE_Pattern_subn_impl(PatternObject *self, PyTypeObject *cls,
                           PyObject *repl, PyObject *string,
                           Py_ssize_t count)
/*[clinic end generated code: output=da02fd85258b1e1f input=8b78a65b8302e58d]*/
{
    _sremodulestate *module_state = get_sre_module_state_by_class(cls);

    return pattern_subx(module_state, self, repl, string, count, 1);
}

/*[clinic input]
_sre.SRE_Pattern.__copy__

[clinic start generated code]*/

static PyObject *
_sre_SRE_Pattern___copy___impl(PatternObject *self)
/*[clinic end generated code: output=85dedc2db1bd8694 input=a730a59d863bc9f5]*/
{
    Py_INCREF(self);
    return (PyObject *)self;
}

/*[clinic input]
_sre.SRE_Pattern.__deepcopy__

    memo: object
    /

[clinic start generated code]*/

static PyObject *
_sre_SRE_Pattern___deepcopy__(PatternObject *self, PyObject *memo)
/*[clinic end generated code: output=2ad25679c1f1204a input=a465b1602f997bed]*/
{
    Py_INCREF(self);
    return (PyObject *)self;
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
    size_t i;
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

PyDoc_STRVAR(pattern_doc, "Compiled regular expression object.");

/* PatternObject's 'groupindex' method. */
static PyObject *
pattern_groupindex(PatternObject *self, void *Py_UNUSED(ignored))
{
    if (self->groupindex == NULL)
        return PyDict_New();
    return PyDictProxy_New(self->groupindex);
}

static int _validate(PatternObject *self); /* Forward */

/*[clinic input]
_sre.compile

    pattern: object
    flags: int
    code: object(subclass_of='&PyList_Type')
    groups: Py_ssize_t
    groupindex: object(subclass_of='&PyDict_Type')
    indexgroup: object(subclass_of='&PyTuple_Type')

[clinic start generated code]*/

static PyObject *
_sre_compile_impl(PyObject *module, PyObject *pattern, int flags,
                  PyObject *code, Py_ssize_t groups, PyObject *groupindex,
                  PyObject *indexgroup)
/*[clinic end generated code: output=ef9c2b3693776404 input=0a68476dbbe5db30]*/
{
    /* "compile" pattern descriptor to pattern object */

    _sremodulestate *module_state = get_sre_module_state(module);
    PatternObject* self;
    Py_ssize_t i, n;

    n = PyList_GET_SIZE(code);
    /* coverity[ampersand_in_size] */
    self = PyObject_GC_NewVar(PatternObject, module_state->Pattern_Type, n);
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
    PyObject_GC_Track(self);

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

    if (PyDict_GET_SIZE(groupindex) > 0) {
        Py_INCREF(groupindex);
        self->groupindex = groupindex;
        if (PyTuple_GET_SIZE(indexgroup) > 0) {
            Py_INCREF(indexgroup);
            self->indexgroup = indexgroup;
        }
    }

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
#define FAIL do { VTRACE(("FAIL: %d\n", __LINE__)); return -1; } while (0)

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
        if (skip-adj > (uintptr_t)(end - code))         \
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
        case SRE_OP_RANGE_UNI_IGNORE:
            GET_ARG;
            GET_ARG;
            break;

        case SRE_OP_CHARSET:
            offset = 256/SRE_CODE_BITS; /* 256-bit bitmap */
            if (offset > (uintptr_t)(end - code))
                FAIL;
            code += offset;
            break;

        case SRE_OP_BIGCHARSET:
            GET_ARG; /* Number of blocks */
            offset = 256/sizeof(SRE_CODE); /* 256-byte table */
            if (offset > (uintptr_t)(end - code))
                FAIL;
            /* Make sure that each byte points to a valid block */
            for (i = 0; i < 256; i++) {
                if (((unsigned char *)code)[i] >= arg)
                    FAIL;
            }
            code += offset;
            offset = arg * (256/SRE_CODE_BITS); /* 256-bit bitmap times arg */
            if (offset > (uintptr_t)(end - code))
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

    return 0;
}

/* Returns 0 on success, -1 on failure, and 1 if the last op is JUMP. */
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
        case SRE_OP_LITERAL_UNI_IGNORE:
        case SRE_OP_NOT_LITERAL_UNI_IGNORE:
        case SRE_OP_LITERAL_LOC_IGNORE:
        case SRE_OP_NOT_LITERAL_LOC_IGNORE:
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
        case SRE_OP_IN_UNI_IGNORE:
        case SRE_OP_IN_LOC_IGNORE:
            GET_SKIP;
            /* Stop 1 before the end; we check the FAILURE below */
            if (_validate_charset(code, code+skip-2))
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
                    if (prefix_len > (uintptr_t)(newcode - code))
                        FAIL;
                    code += prefix_len;
                    /* And here comes the overlap table */
                    if (prefix_len > (uintptr_t)(newcode - code))
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
                    if (_validate_charset(code, newcode-1))
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
                    if (_validate_inner(code, code+skip-3, groups))
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
                if (code != target)
                    FAIL;
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
                if (_validate_inner(code, code+skip-4, groups))
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
                if (_validate_inner(code, code+skip-3, groups))
                    FAIL;
                code += skip-3;
                GET_OP;
                if (op != SRE_OP_MAX_UNTIL && op != SRE_OP_MIN_UNTIL)
                    FAIL;
            }
            break;

        case SRE_OP_GROUPREF:
        case SRE_OP_GROUPREF_IGNORE:
        case SRE_OP_GROUPREF_UNI_IGNORE:
        case SRE_OP_GROUPREF_LOC_IGNORE:
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
            VTRACE(("then part:\n"));
            int rc = _validate_inner(code+1, code+skip-1, groups);
            if (rc == 1) {
                VTRACE(("else part:\n"));
                code += skip-2; /* Position after JUMP, at <skipno> */
                GET_SKIP;
                rc = _validate_inner(code, code+skip-1, groups);
            }
            if (rc)
                FAIL;
            code += skip-1;
            break;

        case SRE_OP_ASSERT:
        case SRE_OP_ASSERT_NOT:
            GET_SKIP;
            GET_ARG; /* 0 for lookahead, width for lookbehind */
            code--; /* Back up over arg to simplify math below */
            if (arg & 0x80000000)
                FAIL; /* Width too large */
            /* Stop 1 before the end; we check the SUCCESS below */
            if (_validate_inner(code+1, code+skip-2, groups))
                FAIL;
            code += skip-2;
            GET_OP;
            if (op != SRE_OP_SUCCESS)
                FAIL;
            break;

        case SRE_OP_JUMP:
            if (code + 1 != end)
                FAIL;
            VTRACE(("JUMP: %d\n", __LINE__));
            return 1;

        default:
            FAIL;

        }
    }

    VTRACE(("okay\n"));
    return 0;
}

static int
_validate_outer(SRE_CODE *code, SRE_CODE *end, Py_ssize_t groups)
{
    if (groups < 0 || (size_t)groups > SRE_MAXGROUPS ||
        code >= end || end[-1] != SRE_OP_SUCCESS)
        FAIL;
    return _validate_inner(code, end-1, groups);
}

static int
_validate(PatternObject *self)
{
    if (_validate_outer(self->code, self->code+self->codesize, self->groups))
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

static int
match_traverse(MatchObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->string);
    Py_VISIT(self->regs);
    Py_VISIT(self->pattern);
    return 0;
}

static int
match_clear(MatchObject *self)
{
    Py_CLEAR(self->string);
    Py_CLEAR(self->regs);
    Py_CLEAR(self->pattern);
    return 0;
}

static void
match_dealloc(MatchObject* self)
{
    PyTypeObject *tp = Py_TYPE(self);

    PyObject_GC_UnTrack(self);
    (void)match_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyObject*
match_getslice_by_index(MatchObject* self, Py_ssize_t index, PyObject* def)
{
    Py_ssize_t length;
    int isbytes, charsize;
    Py_buffer view;
    PyObject *result;
    const void* ptr;
    Py_ssize_t i, j;

    assert(0 <= index && index < self->groups);
    index *= 2;

    if (self->string == Py_None || self->mark[index] < 0) {
        /* return default value if the string or group is undefined */
        Py_INCREF(def);
        return def;
    }

    ptr = getstring(self->string, &length, &isbytes, &charsize, &view);
    if (ptr == NULL)
        return NULL;

    i = self->mark[index];
    j = self->mark[index+1];
    i = Py_MIN(i, length);
    j = Py_MIN(j, length);
    result = getslice(isbytes, ptr, self->string, i, j);
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

    if (PyIndex_Check(index)) {
        i = PyNumber_AsSsize_t(index, NULL);
    }
    else {
        i = -1;

        if (self->pattern->groupindex) {
            index = PyDict_GetItemWithError(self->pattern->groupindex, index);
            if (index && PyLong_Check(index)) {
                i = PyLong_AsSsize_t(index);
            }
        }
    }
    if (i < 0 || i >= self->groups) {
        /* raise IndexError if we were given a bad group number */
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_IndexError, "no such group");
        }
        return -1;
    }

    return i;
}

static PyObject*
match_getslice(MatchObject* self, PyObject* index, PyObject* def)
{
    Py_ssize_t i = match_getindex(self, index);

    if (i < 0) {
        return NULL;
    }

    return match_getslice_by_index(self, i, def);
}

/*[clinic input]
_sre.SRE_Match.expand

    template: object

Return the string obtained by doing backslash substitution on the string template, as done by the sub() method.
[clinic start generated code]*/

static PyObject *
_sre_SRE_Match_expand_impl(MatchObject *self, PyObject *template)
/*[clinic end generated code: output=931b58ccc323c3a1 input=4bfdb22c2f8b146a]*/
{
    /* delegate to Python code */
    return call(
        SRE_PY_MODULE, "_expand",
        PyTuple_Pack(3, self->pattern, self, template)
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
        result = match_getslice(self, _PyLong_GetZero(), Py_None);
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
match_getitem(MatchObject* self, PyObject* name)
{
    return match_getslice(self, name, Py_None);
}

/*[clinic input]
_sre.SRE_Match.groups

    default: object = None
        Is used for groups that did not participate in the match.

Return a tuple containing all the subgroups of the match, from 1.
[clinic start generated code]*/

static PyObject *
_sre_SRE_Match_groups_impl(MatchObject *self, PyObject *default_value)
/*[clinic end generated code: output=daf8e2641537238a input=bb069ef55dabca91]*/
{
    PyObject* result;
    Py_ssize_t index;

    result = PyTuple_New(self->groups-1);
    if (!result)
        return NULL;

    for (index = 1; index < self->groups; index++) {
        PyObject* item;
        item = match_getslice_by_index(self, index, default_value);
        if (!item) {
            Py_DECREF(result);
            return NULL;
        }
        PyTuple_SET_ITEM(result, index-1, item);
    }

    return result;
}

/*[clinic input]
_sre.SRE_Match.groupdict

    default: object = None
        Is used for groups that did not participate in the match.

Return a dictionary containing all the named subgroups of the match, keyed by the subgroup name.
[clinic start generated code]*/

static PyObject *
_sre_SRE_Match_groupdict_impl(MatchObject *self, PyObject *default_value)
/*[clinic end generated code: output=29917c9073e41757 input=0ded7960b23780aa]*/
{
    PyObject *result;
    PyObject *key;
    PyObject *value;
    Py_ssize_t pos = 0;
    Py_hash_t hash;

    result = PyDict_New();
    if (!result || !self->pattern->groupindex)
        return result;

    while (_PyDict_Next(self->pattern->groupindex, &pos, &key, &value, &hash)) {
        int status;
        Py_INCREF(key);
        value = match_getslice(self, key, default_value);
        if (!value) {
            Py_DECREF(key);
            goto failed;
        }
        status = _PyDict_SetItem_KnownHash(result, key, value, hash);
        Py_DECREF(value);
        Py_DECREF(key);
        if (status < 0)
            goto failed;
    }

    return result;

failed:
    Py_DECREF(result);
    return NULL;
}

/*[clinic input]
_sre.SRE_Match.start -> Py_ssize_t

    group: object(c_default="NULL") = 0
    /

Return index of the start of the substring matched by group.
[clinic start generated code]*/

static Py_ssize_t
_sre_SRE_Match_start_impl(MatchObject *self, PyObject *group)
/*[clinic end generated code: output=3f6e7f9df2fb5201 input=ced8e4ed4b33ee6c]*/
{
    Py_ssize_t index = match_getindex(self, group);

    if (index < 0) {
        return -1;
    }

    /* mark is -1 if group is undefined */
    return self->mark[index*2];
}

/*[clinic input]
_sre.SRE_Match.end -> Py_ssize_t

    group: object(c_default="NULL") = 0
    /

Return index of the end of the substring matched by group.
[clinic start generated code]*/

static Py_ssize_t
_sre_SRE_Match_end_impl(MatchObject *self, PyObject *group)
/*[clinic end generated code: output=f4240b09911f7692 input=1b799560c7f3d7e6]*/
{
    Py_ssize_t index = match_getindex(self, group);

    if (index < 0) {
        return -1;
    }

    /* mark is -1 if group is undefined */
    return self->mark[index*2+1];
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

/*[clinic input]
_sre.SRE_Match.span

    group: object(c_default="NULL") = 0
    /

For match object m, return the 2-tuple (m.start(group), m.end(group)).
[clinic start generated code]*/

static PyObject *
_sre_SRE_Match_span_impl(MatchObject *self, PyObject *group)
/*[clinic end generated code: output=f02ae40594d14fe6 input=8fa6014e982d71d4]*/
{
    Py_ssize_t index = match_getindex(self, group);

    if (index < 0) {
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

/*[clinic input]
_sre.SRE_Match.__copy__

[clinic start generated code]*/

static PyObject *
_sre_SRE_Match___copy___impl(MatchObject *self)
/*[clinic end generated code: output=a779c5fc8b5b4eb4 input=3bb4d30b6baddb5b]*/
{
    Py_INCREF(self);
    return (PyObject *)self;
}

/*[clinic input]
_sre.SRE_Match.__deepcopy__

    memo: object
    /

[clinic start generated code]*/

static PyObject *
_sre_SRE_Match___deepcopy__(MatchObject *self, PyObject *memo)
/*[clinic end generated code: output=ba7cb46d655e4ee2 input=779d12a31c2c325e]*/
{
    Py_INCREF(self);
    return (PyObject *)self;
}

PyDoc_STRVAR(match_doc,
"The result of re.match() and re.search().\n\
Match objects always have a boolean value of True.");

PyDoc_STRVAR(match_group_doc,
"group([group1, ...]) -> str or tuple.\n\
    Return subgroup(s) of the match by indices or names.\n\
    For 0 returns the entire match.");

static PyObject *
match_lastindex_get(MatchObject *self, void *Py_UNUSED(ignored))
{
    if (self->lastindex >= 0)
        return PyLong_FromSsize_t(self->lastindex);
    Py_RETURN_NONE;
}

static PyObject *
match_lastgroup_get(MatchObject *self, void *Py_UNUSED(ignored))
{
    if (self->pattern->indexgroup &&
        self->lastindex >= 0 &&
        self->lastindex < PyTuple_GET_SIZE(self->pattern->indexgroup))
    {
        PyObject *result = PyTuple_GET_ITEM(self->pattern->indexgroup,
                                            self->lastindex);
        Py_INCREF(result);
        return result;
    }
    Py_RETURN_NONE;
}

static PyObject *
match_regs_get(MatchObject *self, void *Py_UNUSED(ignored))
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
            "<%s object; span=(%zd, %zd), match=%.50R>",
            Py_TYPE(self)->tp_name,
            self->mark[0], self->mark[1], group0);
    Py_DECREF(group0);
    return result;
}


static PyObject*
pattern_new_match(_sremodulestate* module_state,
                  PatternObject* pattern,
                  SRE_STATE* state,
                  Py_ssize_t status)
{
    /* create match object (from state object) */

    MatchObject* match;
    Py_ssize_t i, j;
    char* base;
    int n;

    if (status > 0) {

        /* create match object (with room for extra group marks) */
        /* coverity[ampersand_in_size] */
        match = PyObject_GC_NewVar(MatchObject,
                                   module_state->Match_Type,
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

        PyObject_GC_Track(match);
        return (PyObject*) match;

    } else if (status == 0) {

        /* no match */
        Py_RETURN_NONE;

    }

    /* internal error */
    pattern_error(status);
    return NULL;
}


/* -------------------------------------------------------------------- */
/* scanner methods (experimental) */

static int
scanner_traverse(ScannerObject *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->pattern);
    return 0;
}

static int
scanner_clear(ScannerObject *self)
{
    Py_CLEAR(self->pattern);
    return 0;
}

static void
scanner_dealloc(ScannerObject* self)
{
    PyTypeObject *tp = Py_TYPE(self);

    PyObject_GC_UnTrack(self);
    state_fini(&self->state);
    (void)scanner_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static int
scanner_begin(ScannerObject* self)
{
    if (self->executing) {
        PyErr_SetString(PyExc_ValueError,
                        "regular expression scanner already executing");
        return 0;
    }
    self->executing = 1;
    return 1;
}

static void
scanner_end(ScannerObject* self)
{
    assert(self->executing);
    self->executing = 0;
}

/*[clinic input]
_sre.SRE_Scanner.match

    cls: defining_class
    /

[clinic start generated code]*/

static PyObject *
_sre_SRE_Scanner_match_impl(ScannerObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=6e22c149dc0f0325 input=b5146e1f30278cb7]*/
{
    _sremodulestate *module_state = get_sre_module_state_by_class(cls);
    SRE_STATE* state = &self->state;
    PyObject* match;
    Py_ssize_t status;

    if (!scanner_begin(self)) {
        return NULL;
    }
    if (state->start == NULL) {
        scanner_end(self);
        Py_RETURN_NONE;
    }

    state_reset(state);

    state->ptr = state->start;

    status = sre_match(state, PatternObject_GetCode(self->pattern));
    if (PyErr_Occurred()) {
        scanner_end(self);
        return NULL;
    }

    match = pattern_new_match(module_state, (PatternObject*) self->pattern,
                              state, status);

    if (status == 0)
        state->start = NULL;
    else {
        state->must_advance = (state->ptr == state->start);
        state->start = state->ptr;
    }

    scanner_end(self);
    return match;
}


/*[clinic input]
_sre.SRE_Scanner.search

    cls: defining_class
    /

[clinic start generated code]*/

static PyObject *
_sre_SRE_Scanner_search_impl(ScannerObject *self, PyTypeObject *cls)
/*[clinic end generated code: output=23e8fc78013f9161 input=056c2d37171d0bf2]*/
{
    _sremodulestate *module_state = get_sre_module_state_by_class(cls);
    SRE_STATE* state = &self->state;
    PyObject* match;
    Py_ssize_t status;

    if (!scanner_begin(self)) {
        return NULL;
    }
    if (state->start == NULL) {
        scanner_end(self);
        Py_RETURN_NONE;
    }

    state_reset(state);

    state->ptr = state->start;

    status = sre_search(state, PatternObject_GetCode(self->pattern));
    if (PyErr_Occurred()) {
        scanner_end(self);
        return NULL;
    }

    match = pattern_new_match(module_state, (PatternObject*) self->pattern,
                              state, status);

    if (status == 0)
        state->start = NULL;
    else {
        state->must_advance = (state->ptr == state->start);
        state->start = state->ptr;
    }

    scanner_end(self);
    return match;
}

static PyObject *
pattern_scanner(_sremodulestate *module_state,
                PatternObject *self,
                PyObject *string,
                Py_ssize_t pos,
                Py_ssize_t endpos)
{
    ScannerObject* scanner;

    /* create scanner object */
    scanner = PyObject_GC_New(ScannerObject, module_state->Scanner_Type);
    if (!scanner)
        return NULL;
    scanner->pattern = NULL;
    scanner->executing = 0;

    /* create search state object */
    if (!state_init(&scanner->state, self, string, pos, endpos)) {
        Py_DECREF(scanner);
        return NULL;
    }

    Py_INCREF(self);
    scanner->pattern = (PyObject*) self;

    PyObject_GC_Track(scanner);
    return (PyObject*) scanner;
}

static Py_hash_t
pattern_hash(PatternObject *self)
{
    Py_hash_t hash, hash2;

    hash = PyObject_Hash(self->pattern);
    if (hash == -1) {
        return -1;
    }

    hash2 = _Py_HashBytes(self->code, sizeof(self->code[0]) * self->codesize);
    hash ^= hash2;

    hash ^= self->flags;
    hash ^= self->isbytes;
    hash ^= self->codesize;

    if (hash == -1) {
        hash = -2;
    }
    return hash;
}

static PyObject*
pattern_richcompare(PyObject *lefto, PyObject *righto, int op)
{
    PyTypeObject *tp = Py_TYPE(lefto);
    _sremodulestate *module_state = get_sre_module_state_by_class(tp);
    PatternObject *left, *right;
    int cmp;

    if (op != Py_EQ && op != Py_NE) {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (!Py_IS_TYPE(righto, module_state->Pattern_Type))
    {
        Py_RETURN_NOTIMPLEMENTED;
    }

    if (lefto == righto) {
        /* a pattern is equal to itself */
        return PyBool_FromLong(op == Py_EQ);
    }

    left = (PatternObject *)lefto;
    right = (PatternObject *)righto;

    cmp = (left->flags == right->flags
           && left->isbytes == right->isbytes
           && left->codesize == right->codesize);
    if (cmp) {
        /* Compare the code and the pattern because the same pattern can
           produce different codes depending on the locale used to compile the
           pattern when the re.LOCALE flag is used. Don't compare groups,
           indexgroup nor groupindex: they are derivated from the pattern. */
        cmp = (memcmp(left->code, right->code,
                      sizeof(left->code[0]) * left->codesize) == 0);
    }
    if (cmp) {
        cmp = PyObject_RichCompareBool(left->pattern, right->pattern,
                                       Py_EQ);
        if (cmp < 0) {
            return NULL;
        }
    }
    if (op == Py_NE) {
        cmp = !cmp;
    }
    return PyBool_FromLong(cmp);
}

#include "clinic/_sre.c.h"

static PyMethodDef pattern_methods[] = {
    _SRE_SRE_PATTERN_MATCH_METHODDEF
    _SRE_SRE_PATTERN_FULLMATCH_METHODDEF
    _SRE_SRE_PATTERN_SEARCH_METHODDEF
    _SRE_SRE_PATTERN_SUB_METHODDEF
    _SRE_SRE_PATTERN_SUBN_METHODDEF
    _SRE_SRE_PATTERN_FINDALL_METHODDEF
    _SRE_SRE_PATTERN_SPLIT_METHODDEF
    _SRE_SRE_PATTERN_FINDITER_METHODDEF
    _SRE_SRE_PATTERN_SCANNER_METHODDEF
    _SRE_SRE_PATTERN___COPY___METHODDEF
    _SRE_SRE_PATTERN___DEEPCOPY___METHODDEF
    {"__class_getitem__", (PyCFunction)Py_GenericAlias, METH_O|METH_CLASS,
     PyDoc_STR("See PEP 585")},
    {NULL, NULL}
};

static PyGetSetDef pattern_getset[] = {
    {"groupindex", (getter)pattern_groupindex, (setter)NULL,
      "A dictionary mapping group names to group numbers."},
    {NULL}  /* Sentinel */
};

#define PAT_OFF(x) offsetof(PatternObject, x)
static PyMemberDef pattern_members[] = {
    {"pattern",    T_OBJECT,    PAT_OFF(pattern),       READONLY,
     "The pattern string from which the RE object was compiled."},
    {"flags",      T_INT,       PAT_OFF(flags),         READONLY,
     "The regex matching flags."},
    {"groups",     T_PYSSIZET,  PAT_OFF(groups),        READONLY,
     "The number of capturing groups in the pattern."},
    {"__weaklistoffset__", T_PYSSIZET, offsetof(PatternObject, weakreflist), READONLY},
    {NULL}  /* Sentinel */
};

static PyType_Slot pattern_slots[] = {
    {Py_tp_dealloc, (destructor)pattern_dealloc},
    {Py_tp_repr, (reprfunc)pattern_repr},
    {Py_tp_hash, (hashfunc)pattern_hash},
    {Py_tp_doc, (void *)pattern_doc},
    {Py_tp_richcompare, pattern_richcompare},
    {Py_tp_methods, pattern_methods},
    {Py_tp_members, pattern_members},
    {Py_tp_getset, pattern_getset},
    {Py_tp_traverse, pattern_traverse},
    {Py_tp_clear, pattern_clear},
    {0, NULL},
};

static PyType_Spec pattern_spec = {
    .name = "re.Pattern",
    .basicsize = sizeof(PatternObject),
    .itemsize = sizeof(SRE_CODE),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE |
              Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC),
    .slots = pattern_slots,
};

static PyMethodDef match_methods[] = {
    {"group", (PyCFunction) match_group, METH_VARARGS, match_group_doc},
    _SRE_SRE_MATCH_START_METHODDEF
    _SRE_SRE_MATCH_END_METHODDEF
    _SRE_SRE_MATCH_SPAN_METHODDEF
    _SRE_SRE_MATCH_GROUPS_METHODDEF
    _SRE_SRE_MATCH_GROUPDICT_METHODDEF
    _SRE_SRE_MATCH_EXPAND_METHODDEF
    _SRE_SRE_MATCH___COPY___METHODDEF
    _SRE_SRE_MATCH___DEEPCOPY___METHODDEF
    {"__class_getitem__", (PyCFunction)Py_GenericAlias, METH_O|METH_CLASS,
     PyDoc_STR("See PEP 585")},
    {NULL, NULL}
};

static PyGetSetDef match_getset[] = {
    {"lastindex", (getter)match_lastindex_get, (setter)NULL,
     "The integer index of the last matched capturing group."},
    {"lastgroup", (getter)match_lastgroup_get, (setter)NULL,
     "The name of the last matched capturing group."},
    {"regs",      (getter)match_regs_get,      (setter)NULL},
    {NULL}
};

#define MATCH_OFF(x) offsetof(MatchObject, x)
static PyMemberDef match_members[] = {
    {"string",  T_OBJECT,   MATCH_OFF(string),  READONLY,
     "The string passed to match() or search()."},
    {"re",      T_OBJECT,   MATCH_OFF(pattern), READONLY,
     "The regular expression object."},
    {"pos",     T_PYSSIZET, MATCH_OFF(pos),     READONLY,
     "The index into the string at which the RE engine started looking for a match."},
    {"endpos",  T_PYSSIZET, MATCH_OFF(endpos),  READONLY,
     "The index into the string beyond which the RE engine will not go."},
    {NULL}
};

/* FIXME: implement setattr("string", None) as a special case (to
   detach the associated string, if any */
static PyType_Slot match_slots[] = {
    {Py_tp_dealloc, match_dealloc},
    {Py_tp_repr, match_repr},
    {Py_tp_doc, (void *)match_doc},
    {Py_tp_methods, match_methods},
    {Py_tp_members, match_members},
    {Py_tp_getset, match_getset},
    {Py_tp_traverse, match_traverse},
    {Py_tp_clear, match_clear},

    /* As mapping.
     *
     * Match objects do not support length or assignment, but do support
     * __getitem__.
     */
    {Py_mp_subscript, match_getitem},

    {0, NULL},
};

static PyType_Spec match_spec = {
    .name = "re.Match",
    .basicsize = sizeof(MatchObject),
    .itemsize = sizeof(Py_ssize_t),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE |
              Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC),
    .slots = match_slots,
};

static PyMethodDef scanner_methods[] = {
    _SRE_SRE_SCANNER_MATCH_METHODDEF
    _SRE_SRE_SCANNER_SEARCH_METHODDEF
    {NULL, NULL}
};

#define SCAN_OFF(x) offsetof(ScannerObject, x)
static PyMemberDef scanner_members[] = {
    {"pattern", T_OBJECT, SCAN_OFF(pattern), READONLY},
    {NULL}  /* Sentinel */
};

static PyType_Slot scanner_slots[] = {
    {Py_tp_dealloc, scanner_dealloc},
    {Py_tp_methods, scanner_methods},
    {Py_tp_members, scanner_members},
    {Py_tp_traverse, scanner_traverse},
    {Py_tp_clear, scanner_clear},
    {0, NULL},
};

static PyType_Spec scanner_spec = {
    .name = "_" SRE_MODULE ".SRE_Scanner",
    .basicsize = sizeof(ScannerObject),
    .flags = (Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE |
              Py_TPFLAGS_DISALLOW_INSTANTIATION | Py_TPFLAGS_HAVE_GC),
    .slots = scanner_slots,
};

static PyMethodDef _functions[] = {
    _SRE_COMPILE_METHODDEF
    _SRE_GETCODESIZE_METHODDEF
    _SRE_ASCII_ISCASED_METHODDEF
    _SRE_UNICODE_ISCASED_METHODDEF
    _SRE_ASCII_TOLOWER_METHODDEF
    _SRE_UNICODE_TOLOWER_METHODDEF
    {NULL, NULL}
};

static int
sre_traverse(PyObject *module, visitproc visit, void *arg)
{
    _sremodulestate *state = get_sre_module_state(module);

    Py_VISIT(state->Pattern_Type);
    Py_VISIT(state->Match_Type);
    Py_VISIT(state->Scanner_Type);

    return 0;
}

static int
sre_clear(PyObject *module)
{
    _sremodulestate *state = get_sre_module_state(module);

    Py_CLEAR(state->Pattern_Type);
    Py_CLEAR(state->Match_Type);
    Py_CLEAR(state->Scanner_Type);

    return 0;
}

static void
sre_free(void *module)
{
    sre_clear((PyObject *)module);
}

#define CREATE_TYPE(m, type, spec)                                  \
do {                                                                \
    type = (PyTypeObject *)PyType_FromModuleAndSpec(m, spec, NULL); \
    if (type == NULL) {                                             \
        goto error;                                                 \
    }                                                               \
} while (0)

#define ADD_ULONG_CONSTANT(module, name, value)           \
    do {                                                  \
        PyObject *o = PyLong_FromUnsignedLong(value);     \
        if (!o)                                           \
            goto error;                                   \
        int res = PyModule_AddObjectRef(module, name, o); \
        Py_DECREF(o);                                     \
        if (res < 0) {                                    \
            goto error;                                   \
        }                                                 \
} while (0)

static int
sre_exec(PyObject *m)
{
    _sremodulestate *state;

    /* Create heap types */
    state = get_sre_module_state(m);
    CREATE_TYPE(m, state->Pattern_Type, &pattern_spec);
    CREATE_TYPE(m, state->Match_Type, &match_spec);
    CREATE_TYPE(m, state->Scanner_Type, &scanner_spec);

    if (PyModule_AddIntConstant(m, "MAGIC", SRE_MAGIC) < 0) {
        goto error;
    }

    if (PyModule_AddIntConstant(m, "CODESIZE", sizeof(SRE_CODE)) < 0) {
        goto error;
    }

    ADD_ULONG_CONSTANT(m, "MAXREPEAT", SRE_MAXREPEAT);
    ADD_ULONG_CONSTANT(m, "MAXGROUPS", SRE_MAXGROUPS);

    if (PyModule_AddStringConstant(m, "copyright", copyright) < 0) {
        goto error;
    }

    return 0;

error:
    return -1;
}

static PyModuleDef_Slot sre_slots[] = {
    {Py_mod_exec, sre_exec},
    {0, NULL},
};

static struct PyModuleDef sremodule = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "_" SRE_MODULE,
    .m_size = sizeof(_sremodulestate),
    .m_methods = _functions,
    .m_slots = sre_slots,
    .m_traverse = sre_traverse,
    .m_free = sre_free,
    .m_clear = sre_clear,
};

PyMODINIT_FUNC
PyInit__sre(void)
{
    return PyModuleDef_Init(&sremodule);
}

/* vim:ts=4:sw=4:et
*/
