/*
 * C accelerator for the translation function from UNIX shell patterns
 * to RE patterns.
 */

#include "_fnmatchmodule.h" // for get_fnmatchmodulestate_state()

#include "pycore_call.h"

// ==== Macro definitions =====================================================
//
// The following _WRITE_* and _WRITE_*_OR macros do NOT check their inputs
// since they directly delegate to the _PyUnicodeWriter_Write* underlying
// function. In particular, the caller is responsible for type safety.

#define _WRITE_OR_FAIL(WRITE_OPERATION, ON_ERROR)   \
    do {                                            \
        if ((WRITE_OPERATION) < 0) {                \
            ON_ERROR;                               \
        }                                           \
    } while (0)

/* write a character CHAR */
#define _WRITE_CHAR(WRITER, CHAR) \
    _PyUnicodeWriter_WriteChar((_PyUnicodeWriter *)(WRITER), (CHAR))
/* write a character CHAR or execute the ON_ERROR statements if it fails */
#define _WRITE_CHAR_OR(WRITER, CHAR, ON_ERROR) \
    _WRITE_OR_FAIL(_WRITE_CHAR((WRITER), (CHAR)), ON_ERROR)

/* write an ASCII string STRING of given length LENGTH */
#define _WRITE_ASCII(WRITER, ASCII, LENGTH)                         \
    _PyUnicodeWriter_WriteASCIIString((_PyUnicodeWriter *)(WRITER), \
                                      (ASCII), (LENGTH))
/*
 * Write an ASCII string STRING of given length LENGTH,
 * or execute the ON_ERROR statements if it fails.
 */
#define _WRITE_ASCII_OR(WRITER, ASCII, LENGTH, ON_ERROR) \
    _WRITE_OR_FAIL(_WRITE_ASCII((WRITER), (ASCII), (LENGTH)), ON_ERROR)

/* write the string STRING */
#define _WRITE_STRING(WRITER, STRING) \
    _PyUnicodeWriter_WriteStr((_PyUnicodeWriter *)(WRITER), (STRING))
/* write the string STRING or execute the ON_ERROR statements if it fails */
#define _WRITE_STRING_OR(WRITER, STRING, ON_ERROR) \
    _WRITE_OR_FAIL(_WRITE_STRING((WRITER), (STRING)), ON_ERROR)

/* write the substring STRING[START:STOP] */
#define _WRITE_BLOCK(WRITER, STRING, START, STOP)                   \
    _PyUnicodeWriter_WriteSubstring((_PyUnicodeWriter *)(WRITER),   \
                                    (STRING), (START), (STOP))
/*
 * Write the substring STRING[START:STOP] if START < STOP,
 * or execute the ON_ERROR statements if it fails.
 */
#define _WRITE_BLOCK_OR(WRITER, STRING, START, STOP, ON_ERROR)          \
    do {                                                                \
        /* intermediate variables to allow in-place operations */       \
        Py_ssize_t _i = (START), _j = (STOP);                           \
        if (_i < _j && _WRITE_BLOCK((WRITER), (STRING), _i, _j) < 0) {  \
            ON_ERROR;                                                   \
        }                                                               \
    } while (0)

// ==== Helper declarations ===================================================

/*
 * Creates a new Unicode object from a Py_UCS4 character.
 *
 * Note: this is 'unicode_char' taken from Objects/unicodeobject.c.
 */
static PyObject *
get_unicode_character(Py_UCS4 ch);

/*
 * Construct a regular expression out of a UNIX-style expression.
 *
 * The expression to translate is the content of an '[(BLOCK)]' expression
 * or '[!(BLOCK)]' expression. The BLOCK contains single unicode characters
 * or character ranges (e.g., 'a-z').
 *
 * By convention 'start' and 'stop' represent the INCLUSIVE start index
 * and EXCLUSIVE stop index of BLOCK in the full 'pattern'. Note that
 * we always have pattern[stop] == ']' and pattern[start] == BLOCK[0].
 *
 * For instance, for "ab[c-f]g[!1-5]", the values of 'start' and 'stop'
 * for the sub-pattern '[c-f]' are 3 and 6 respectively, whereas their
 * values for '[!1-5]' are 10 (not 9) and 13 respectively.
 */
static PyObject *
translate_expression(fnmatchmodule_state *state,
                     PyObject *pattern, Py_ssize_t start, Py_ssize_t stop);

/*
 * Write the translated pattern obtained by translate_expression().
 *
 * This returns the number of written characters, or -1 if an error occurred.
 */
static Py_ssize_t
write_expression(PyUnicodeWriter *writer, PyObject *expression);

/*
 * Build the final regular expression by processing the wildcards.
 *
 * The position of each wildcard in 'pattern' is given by 'indices'.
 */
static PyObject *
process_wildcards(PyObject *pattern, PyObject *indices);

// ==== API implementation ====================================================

PyObject *
_Py_fnmatch_translate(PyObject *module, PyObject *pattern)
{
    assert(PyUnicode_Check(pattern));
    fnmatchmodule_state *state = get_fnmatchmodule_state(module);
    const Py_ssize_t n = PyUnicode_GET_LENGTH(pattern);

    // We would write less data if there are successive '*',
    // which should not be the case in general. Otherwise,
    // we write >= n characters since escaping them always
    // add more characters.
    //
    // Note that only b'()[]{}?*+-|^$\\.&~# \t\n\r\v\f' need to
    // be escaped when translated to RE patterns and '*' and '?'
    // are already handled without being escaped.
    //
    // In general, UNIX style patterns are more likely to contain
    // wildcards than characters to be escaped, with the exception
    // of '-', '\' and '~' (we usually want to match filenmaes),
    // and there is a sparse number of them. Therefore, we only
    // estimate the number of characters to be written to be the
    // same as the number of characters in the pattern.
    //
    // TODO(picnixz): should we limit the estimation?
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(n);
    if (writer == NULL) {
        return NULL;
    }

    // list containing the indices where '*' has a special meaning
    PyObject *indices = NULL;
    // cached functions (cache is local to the call)
    PyObject *re_escape_func = NULL, *re_sub_func = NULL;

    indices = PyList_New(0);
    if (indices == NULL) {
        goto abort;
    }
    re_escape_func = PyObject_GetAttr(state->re_module, &_Py_ID(escape));
    if (re_escape_func == NULL) {
        goto abort;
    }
    re_sub_func = PyObject_GetAttr(state->re_module, &_Py_ID(sub));
    if (re_sub_func == NULL) {
        goto abort;
    }

    const int unicode_kind = PyUnicode_KIND(pattern);
    const void *const unicode_data = PyUnicode_DATA(pattern);
    /* declaration of some local helping macros */
#define READ(IND) PyUnicode_READ(unicode_kind, unicode_data, (IND))
    /* advance IND if the character is CHAR */
#define ADVANCE_IF_NEXT_CHAR_IS(CHAR, IND, MAXIND)              \
    do {                                                        \
        /* the following forces IND to be a variable name */    \
        void *Py_UNUSED(_ind) = &IND;                           \
        if ((IND) < (MAXIND) && READ(IND) == (CHAR)) {          \
            ++IND;                                              \
        }                                                       \
    } while (0)

    Py_ssize_t i = 0;   // current index
    Py_ssize_t wi = 0;  // number of characters written
    while (i < n) {
        // read and advance to the next character
        Py_UCS4 chr = READ(i++);
        switch (chr) {
            case '*': {
                _WRITE_CHAR_OR(writer, '*', goto abort);
                // skip duplicated '*'
                for (; i < n && READ(i) == '*'; ++i);
                PyObject *index = PyLong_FromSsize_t(wi++);
                if (index == NULL) {
                    goto abort;
                }
                int rc = PyList_Append(indices, index);
                Py_DECREF(index);
                if (rc < 0) {
                    goto abort;
                }
                break;
            }
            case '?': {
                // translate optional '?' (fnmatch) into optional '.' (regex)
                _WRITE_CHAR_OR(writer, '.', goto abort);
                ++wi; // increase the expected result's length
                break;
            }
            case '[': {
                Py_ssize_t j = i;
                ADVANCE_IF_NEXT_CHAR_IS('!', j, n);     // [!
                ADVANCE_IF_NEXT_CHAR_IS(']', j, n);     // [!] or []
                for (; j < n && READ(j) != ']'; ++j);   // locate closing ']'
                if (j >= n) {
                    _WRITE_ASCII_OR(writer, "\\[", 2, goto abort);
                    wi += 2; // we just wrote 2 characters
                    break;  // early break for clarity
                }
                else {
                    //              v--- pattern[j] (exclusive)
                    // '[' * ... * ']'
                    //     ^----- pattern[i] (inclusive)
                    Py_ssize_t pos = PyUnicode_FindChar(pattern, '-', i, j, 1);
                    if (pos == -2) {
                        goto abort;
                    }
                    PyObject *s1 = NULL, *s2 = NULL;
                    if (pos == -1) {
                        PyObject *s0 = PyUnicode_Substring(pattern, i, j);
                        if (s0 == NULL) {
                            goto abort;
                        }
                        s1 = PyObject_CallMethodObjArgs(
                            s0,
                            &_Py_ID(replace),
                            state->backslash_str,
                            state->backslash_esc_str,
                            NULL
                        );
                        Py_DECREF(s0);
                    }
                    else {
                        assert(pos >= 0);
                        assert(READ(j) == ']');
                        s1 = translate_expression(state, pattern, i, j);
                    }
                    if (s1 == NULL) {
                        goto abort;
                    }
                    s2 = PyObject_CallFunctionObjArgs(
                        re_sub_func,
                        state->inactive_toks_str,
                        state->inactive_toks_repl_str,
                        s1,
                        NULL
                    );
                    Py_DECREF(s1);
                    if (s2 == NULL) {
                        goto abort;
                    }
                    Py_ssize_t difflen = write_expression(writer, s2);
                    Py_DECREF(s2);
                    if (difflen < 0) {
                        goto abort;
                    }
                    wi += difflen;
                    i = j + 1;  // jump to the character after ']'
                    break;      // early break for clarity
                }
            }
            default: {
                PyObject *str = get_unicode_character(chr);
                if (str == NULL) {
                    goto abort;
                }
                PyObject *escchr = PyObject_CallOneArg(re_escape_func, str);
                Py_DECREF(str);
                if (escchr == NULL) {
                    goto abort;
                }
                Py_ssize_t difflen = PyUnicode_GET_LENGTH(escchr);
                int rc = _WRITE_STRING(writer, escchr);
                Py_DECREF(escchr);
                if (rc < 0) {
                    goto abort;
                }
                wi += difflen;
                break;
            }
        }
    }
#undef ADVANCE_IF_NEXT_CHAR_IS
#undef READ
    Py_DECREF(re_sub_func);
    Py_DECREF(re_escape_func);
    PyObject *translated = PyUnicodeWriter_Finish(writer);
    if (translated == NULL) {
        Py_DECREF(indices);
        return NULL;
    }
    PyObject *res = process_wildcards(translated, indices);
    Py_DECREF(translated);
    Py_DECREF(indices);
    return res;
abort:
    Py_XDECREF(re_sub_func);
    Py_XDECREF(re_escape_func);
    Py_XDECREF(indices);
    PyUnicodeWriter_Discard(writer);
    return NULL;
}

// ==== Helper implementations ================================================

static PyObject *
get_unicode_character(Py_UCS4 ch)
{
    assert(ch <= 0x10ffff);
    if (ch < 256) {
        PyObject *o = _Py_LATIN1_CHR(ch);
        assert(_Py_IsImmortal(o));
        return o;
    }
    PyObject *unicode = PyUnicode_New(1, ch);
    if (unicode == NULL) {
        return NULL;
    }
    assert(PyUnicode_KIND(unicode) != PyUnicode_1BYTE_KIND);
    if (PyUnicode_KIND(unicode) == PyUnicode_2BYTE_KIND) {
        PyUnicode_2BYTE_DATA(unicode)[0] = (Py_UCS2)ch;
    }
    else {
        assert(PyUnicode_KIND(unicode) == PyUnicode_4BYTE_KIND);
        PyUnicode_4BYTE_DATA(unicode)[0] = ch;
    }
    assert(_PyUnicode_CheckConsistency(unicode, 1));
    return unicode;
}

/*
 * Extract a list of chunks from the pattern group described by i and j.
 *
 * See translate_expression() for its usage.
 */
static PyObject *
translate_expression_split(fnmatchmodule_state *state,
                           PyObject *pattern, Py_ssize_t i, Py_ssize_t j)
{
    PyObject *chunks = NULL;
    // local cache for some objects
    PyObject *str_find_func = NULL, *max_find_index = NULL;

    chunks = PyList_New(0);
    if (chunks == NULL) {
        goto abort;
    }
    str_find_func = PyObject_GetAttr(pattern, &_Py_ID(find));
    if (str_find_func == NULL) {
        goto abort;
    }
    max_find_index = PyLong_FromSsize_t(j);
    if (max_find_index == NULL) {
        goto abort;
    }

    Py_ssize_t k = (PyUnicode_READ_CHAR(pattern, i) == '!') ? i + 2 : i + 1;
    while (k < j) {
        PyObject *eobj = PyObject_CallFunction(
            str_find_func, "OnO", state->hyphen_str, k, max_find_index);
        if (eobj == NULL) {
            goto abort;
        }
        Py_ssize_t t = PyLong_AsSsize_t(eobj);
        Py_DECREF(eobj);
        if (t < 0) {
            if (PyErr_Occurred()) {
                goto abort;
            }
            // -1 here means that '-' was not found
            assert(t == -1);
            break;
        }
        PyObject *sub = PyUnicode_Substring(pattern, i, t);
        if (sub == NULL) {
            goto abort;
        }
        int rc = PyList_Append(chunks, sub);
        Py_DECREF(sub);
        if (rc < 0) {
            goto abort;
        }
        i = t + 1;
        k = t + 3;
    }
    // handle the last group
    if (i >= j) {
        Py_ssize_t chunkscount = PyList_GET_SIZE(chunks);
        assert(chunkscount > 0);
        PyObject *chunk = PyList_GET_ITEM(chunks, chunkscount - 1);
        assert(chunk != NULL);
        PyObject *str = PyUnicode_Concat(chunk, state->hyphen_str);
        // PyList_SetItem() does not create a new reference on 'str'
        // so we should not decref 'str' after the call, unless there
        // is an issue while setting the item.
        if (str == NULL || PyList_SetItem(chunks, chunkscount - 1, str) < 0) {
            Py_XDECREF(str);
            goto abort;
        }
    }
    else {
        // add the remaining sub-pattern
        PyObject *sub = PyUnicode_Substring(pattern, i, j);
        if (sub == NULL) {
            goto abort;
        }
        int rc = PyList_Append(chunks, sub);
        Py_DECREF(sub);
        if (rc < 0) {
            goto abort;
        }
    }
    Py_DECREF(max_find_index);
    Py_DECREF(str_find_func);
    return chunks;
abort:
    Py_XDECREF(max_find_index);
    Py_XDECREF(str_find_func);
    Py_XDECREF(chunks);
    return NULL;
}

/*
 * Remove empty ranges (they are invalid in RE).
 *
 * See translate_expression() for its usage.
 */
static int
translate_expression_simplify(fnmatchmodule_state *st, PyObject *chunks)
{
    Py_ssize_t c = PyList_GET_SIZE(chunks);
    while (--c) {
        PyObject *c1 = PyList_GET_ITEM(chunks, c - 1);
        assert(c1 != NULL);
        Py_ssize_t c1len = PyUnicode_GET_LENGTH(c1);
        assert(c1len > 0);

        PyObject *c2 = PyList_GET_ITEM(chunks, c);
        assert(c2 != NULL);
        Py_ssize_t c2len = PyUnicode_GET_LENGTH(c2);
        assert(c2len > 0);

        if (PyUnicode_READ_CHAR(c1, c1len - 1) > PyUnicode_READ_CHAR(c2, 0)) {
            Py_ssize_t olen = c1len + c2len - 2;
            assert(olen >= 0);
            // see https://github.com/python/cpython/issues/114917 for
            // why we need olen + 1 and not olen currently
            PyUnicodeWriter *writer = PyUnicodeWriter_Create(olen + 1);
            if (writer == NULL) {
                return -1;
            }
            // all but the last character in the first chunk
            if (_WRITE_BLOCK(writer, c1, 0, c1len - 1) < 0) {
                PyUnicodeWriter_Discard(writer);
                return -1;
            }
            // all but the first character in the second chunk
            if (_WRITE_BLOCK(writer, c2, 1, c2len) < 0) {
                PyUnicodeWriter_Discard(writer);
                return -1;
            }
            // PyList_SetItem() does not create a new reference on 'merged'
            // so we should not decref 'merged' after the call, unless there
            // is an issue while setting the item.
            PyObject *merged = PyUnicodeWriter_Finish(writer);
            if (merged == NULL || PyList_SetItem(chunks, c - 1, merged) < 0) {
                Py_XDECREF(merged);
                return -1;
            }
            if (PySequence_DelItem(chunks, c) < 0) {
                return -1;
            }
        }
    }
    return 0;
}

/*
 * Escape backslashes and hyphens for set difference (--),
 * but hyphens that create ranges should not be escaped.
 *
 * See translate_expression() for its usage.
 */
static int
translate_expression_escape(fnmatchmodule_state *st, PyObject *chunks)
{
    for (Py_ssize_t c = 0; c < PyList_GET_SIZE(chunks); ++c) {
        PyObject *s0 = PyList_GET_ITEM(chunks, c);
        assert(s0 != NULL);
        PyObject *s1 = PyObject_CallMethodObjArgs(s0,
                                                  &_Py_ID(replace),
                                                  st->backslash_str,
                                                  st->backslash_esc_str,
                                                  NULL);
        if (s1 == NULL) {
            return -1;
        }
        PyObject *s2 = PyObject_CallMethodObjArgs(s1,
                                                  &_Py_ID(replace),
                                                  st->hyphen_str,
                                                  st->hyphen_esc_str,
                                                  NULL);
        Py_DECREF(s1);
        // PyList_SetItem() does not create a new reference on 's2'
        // so we should not decref 's2' after the call, unless there
        // is an issue while setting the item.
        if (s2 == NULL || PyList_SetItem(chunks, c, s2) < 0) {
            Py_XDECREF(s2);
            return -1;
        }
    }
    return 0;
}

static PyObject *
translate_expression(fnmatchmodule_state *state,
                     PyObject *pattern, Py_ssize_t i, Py_ssize_t j)
{
    PyObject *chunks = translate_expression_split(state, pattern, i, j);
    if (chunks == NULL) {
        goto abort;
    }
    // remove empty ranges
    if (translate_expression_simplify(state, chunks) < 0) {
        goto abort;
    }
    // escape backslashes and set differences
    if (translate_expression_escape(state, chunks) < 0) {
        goto abort;
    }
    PyObject *res = PyUnicode_Join(state->hyphen_str, chunks);
    Py_DECREF(chunks);
    return res;
abort:
    Py_XDECREF(chunks);
    return NULL;
}

static Py_ssize_t
write_expression(PyUnicodeWriter *writer, PyObject *expression)
{
#define WRITE_CHAR(c)           _WRITE_CHAR_OR(writer, (c), return -1)
#define WRITE_STRING(s)         _WRITE_STRING_OR(writer, (s), return -1)
    Py_ssize_t grouplen = PyUnicode_GET_LENGTH(expression);
    if (grouplen == 0) {
        /* empty range: never match */
        _WRITE_ASCII_OR(writer, "(?!)", 4, return -1);
        return 4;
    }
    Py_UCS4 token = PyUnicode_READ_CHAR(expression, 0);
    if (grouplen == 1 && token == '!') {
        /* negated empty range: match any character */
        WRITE_CHAR('.');
        return 1;
    }
    Py_ssize_t extra = 2; // '[' and ']'
    WRITE_CHAR('[');
    switch (token) {
        case '!': {
            WRITE_CHAR('^'); // replace '!' by '^'
            _WRITE_BLOCK_OR(writer, expression, 1, grouplen, return -1);
            break;
        }
        case '^':
        case '[': {
            WRITE_CHAR('\\');
            ++extra; // because we wrote '\\'
            WRITE_STRING(expression);
            break;
        }
        default: {
            WRITE_STRING(expression);
            break;
        }
    }
    WRITE_CHAR(']');
    return grouplen + extra;
#undef WRITE_STRING
#undef WRITE_CHAR
}

static PyObject *
process_wildcards(PyObject *pattern, PyObject *indices)
{
    const Py_ssize_t m = PyList_GET_SIZE(indices);
    if (m == 0) {
        // "(?s:" + pattern + ")\Z"
        return PyUnicode_FromFormat("(?s:%U)\\Z", pattern);
    }
    /*
     * Special cases: indices[0] == 0 or indices[-1] + 1 == n
     *
     * If indices[0] == 0       write (?>.*?abcdef) instead of abcdef
     * If indices[-1] == n - 1  write '.*' instead of empty string
     */
    Py_ssize_t i = 0, j, n = PyUnicode_GET_LENGTH(pattern);
    /*
     * If the pattern starts with '*', we will write everything
     * before it. So we will write at least indices[0] characters.
     *
     * For the inner groups 'STAR STRING ...' we always surround
     * the STRING by "(?>.*?" and ")", and thus we will write at
     * least 7 + len(STRING) characters.
     *
     * We write one additional '.*' if indices[-1] + 1 == n.
     *
     * Since the result is surrounded by "(?s:" and ")\Z", we
     * write at least "indices[0] + 7*m + n + 6" characters,
     * where 'm' is the number of stars and 'n' the length
     * of the /translated) pattern.
     */
    PyObject *jobj = PyList_GET_ITEM(indices, 0);
    assert(jobj != NULL);
    j = PyLong_AsSsize_t(jobj);  // get the first position of '*'
    if (j < 0) {
        return NULL;
    }
    Py_ssize_t estimate = j + 7 * m + n + 6;
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(estimate);
    if (writer == NULL) {
        return NULL;
    }
    _WRITE_BLOCK_OR(writer, pattern, i, j, goto abort);
    i = j + 1; // jump after the '*'
    for (Py_ssize_t k = 1; k < m; ++k) {
        // process all but the last wildcard.
        PyObject *ind = PyList_GET_ITEM(indices, k);
        assert(ind != NULL);
        j = PyLong_AsSsize_t(ind);
        if (j < 0) {
            goto abort;
        }
        assert(i < j);
        // write the atomic RE group '(?>.*?' + BLOCK + ')'
        _WRITE_ASCII_OR(writer, "(?>.*?", 6, goto abort);
        _WRITE_BLOCK_OR(writer, pattern, i, j, goto abort);
        _WRITE_CHAR_OR(writer, ')', goto abort);
        i = j + 1;
    }
    // handle the remaining wildcard
    _WRITE_ASCII_OR(writer, ".*", 2, goto abort);
    // write the remaining substring (if non-empty)
    _WRITE_BLOCK_OR(writer, pattern, i, n, goto abort);
    PyObject *processed = PyUnicodeWriter_Finish(writer);
    if (processed == NULL) {
        return NULL;
    }
    // "(?s:" + processed + ")\Z"
    PyObject *res = PyUnicode_FromFormat("(?s:%U)\\Z", processed);
    Py_DECREF(processed);
    return res;
abort:
    PyUnicodeWriter_Discard(writer);
    return NULL;
}

#undef _WRITE_BLOCK_OR
#undef _WRITE_BLOCK
#undef _WRITE_STRING_OR
#undef _WRITE_STRING
#undef _WRITE_ASCII_OR
#undef _WRITE_ASCII
#undef _WRITE_CHAR_OR
#undef _WRITE_CHAR
#undef _WRITE_OR_FAIL
