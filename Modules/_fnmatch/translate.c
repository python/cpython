/*
 * C accelerator for the translation function from UNIX shell patterns
 * to RE patterns. This accelerator is platform-independent but can be
 * disabled on demand.
 */

#include "Python.h"
#include "pycore_call.h"    // for _PyObject_CallMethod()

#include "_fnmatchmodule.h" // for get_fnmatchmodulestate_state()

// ==== Helper declarations ==================================================

typedef fnmatchmodule_state State;

#define _WRITE_OR_FAIL(writeop, onerror) \
    do { \
        if ((writeop) < 0) { \
            onerror; \
        } \
    } while (0)

#define _WRITE_CHAR(writer, ch) \
    _PyUnicodeWriter_WriteChar((_PyUnicodeWriter *)(writer), (ch))
#define _WRITE_CHAR_OR(writer, ch, onerror) \
    _WRITE_OR_FAIL(_WRITE_CHAR((writer), (ch)), onerror)

#define _WRITE_ASCII(writer, ascii, length) \
    _PyUnicodeWriter_WriteASCIIString((_PyUnicodeWriter *)(writer), (ascii), (length))
#define _WRITE_ASCII_OR(writer, ascii, length, onerror) \
    _WRITE_OR_FAIL(_WRITE_ASCII((writer), (ascii), (length)), onerror)

#define _WRITE_STRING(writer, string) \
    _PyUnicodeWriter_WriteStr((_PyUnicodeWriter *)(writer), (string))
#define _WRITE_STRING_OR(writer, string, onerror) \
    _WRITE_OR_FAIL(_WRITE_STRING((writer), (string)), onerror)

#define _WRITE_BLOCK(writer, string, i, j) \
    _PyUnicodeWriter_WriteSubstring((_PyUnicodeWriter *)(writer), (string), (i), (j))
#define _WRITE_BLOCK_OR(writer, string, i, j, onerror) \
    do { \
        Py_ssize_t _i = (i), _j = (j); /* to allow in-place operators on i or j */ \
        if (_i < _j && _WRITE_BLOCK((writer), (string), _i, _j) < 0) { \
            onerror; \
        } \
    } while (0)

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
translate_expression(PyObject *pattern, Py_ssize_t start, Py_ssize_t stop);

/*
 * Write an escaped string using re.escape().
 *
 * This returns the number of written characters, or -1 if an error occurred.
 */
static Py_ssize_t
write_literal(State *state, PyUnicodeWriter *writer, PyObject *unicode);

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
#define READ(ind) PyUnicode_READ(kind, data, (ind))
#define ADVANCE_IF_CHAR(ch, ind, maxind) \
    do { \
        /* the following forces ind to be a variable name */ \
        Py_ssize_t *Py_UNUSED(_ind) = &ind; \
        if ((ind) < (maxind) && READ(ind) == (ch)) { \
            ++ind; \
        } \
    } while (0)
#define _WHILE_READ_CMP(ch, ind, maxind, cmp) \
    do { \
        /* the following forces ind to be a variable name */ \
        Py_ssize_t *Py_UNUSED(_ind) = &ind; \
        while ((ind) < (maxind) && READ(ind) cmp (ch)) { \
            ++ind; \
        } \
    } while (0)
#define ADVANCE_TO_NEXT(ch, from, maxind) _WHILE_READ_CMP((ch), (from), (maxind), !=)
#define SKIP_DUPLICATES(ch, from, maxind) _WHILE_READ_CMP((ch), (from), (maxind), ==)

    State *state = get_fnmatchmodulestate_state(module);
    PyObject *re = state->re_module;
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
    // TODO: (picnixz): should we limit the estimation in case of a failure?
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(n);
    if (writer == NULL) {
        return NULL;
    }
    // list containing the indices where '*' has a special meaning
    PyObject *indices = PyList_New(0);
    if (indices == NULL) {
        goto abort;
    }
    const int kind = PyUnicode_KIND(pattern);
    const void *data = PyUnicode_DATA(pattern);
    Py_ssize_t h = 0, i = 0;
    while (i < n) {
        // read and advance to the next character
        Py_UCS4 chr = READ(i++);
        switch (chr) {
            case '*': {
                _WRITE_CHAR_OR(writer, chr, goto abort);
                SKIP_DUPLICATES('*', i, n);
                PyObject *index = PyLong_FromSsize_t(h++);
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
                ++h; // increase the expected result's length
                break;
            }
            case '[': {
                Py_ssize_t j = i;           // 'i' is already at next char
                ADVANCE_IF_CHAR('!', j, n); // [!
                ADVANCE_IF_CHAR(']', j, n); // [!] or []
                ADVANCE_TO_NEXT(']', j, n); // locate closing ']'
                if (j >= n) {
                    _WRITE_ASCII_OR(writer, "\\[", 2, goto abort);
                    h += 2; // we just wrote 2 characters
                    break;  // early break for clarity
                }
                else {
                    //              v--- pattern[j] (exclusive)
                    // '[' * ... * ']'
                    //     ^----- pattern[i] (inclusive)
                    int pos = PyUnicode_FindChar(pattern, '-', i, j, 1);
                    if (pos == -2) {
                        goto abort;
                    }
                    PyObject *s1 = NULL, *s2 = NULL;
                    if (pos == -1) {
                        PyObject *s0 = PyUnicode_Substring(pattern, i, j);
                        if (s0 == NULL) {
                            goto abort;
                        }
                        s1 = _PyObject_CallMethod(s0, &_Py_ID(replace), "ss", "\\", "\\\\");
                        Py_DECREF(s0);
                    }
                    else {
                        assert(pos >= 0);
                        assert(READ(j) == ']');
                        s1 = translate_expression(pattern, i, j);
                    }
                    if (s1 == NULL) {
                        goto abort;
                    }
                    s2 = _PyObject_CallMethod(re, &_Py_ID(sub), "ssO", "([&~|])", "\\\\\\1", s1);
                    Py_DECREF(s1);
                    if (s2 == NULL) {
                        goto abort;
                    }
                    int difflen = write_expression(writer, s2);
                    Py_DECREF(s2);
                    if (difflen < 0) {
                        goto abort;
                    }
                    h += difflen;
                    i = j + 1;  // jump to the character after ']'
                    break;      // early break for clarity
                }
            }
            default: {
                PyObject *str = get_unicode_character(chr);
                if (str == NULL) {
                    goto abort;
                }
                int difflen = write_literal(state, writer, str);
                Py_DECREF(str);
                if (difflen < 0) {
                    goto abort;
                }
                h += difflen;
                break;
            }
        }
    }
#undef SKIP_DUPLICATES
#undef ADVANCE_TO_NEXT
#undef _WHILE_READ_CMP
#undef ADVANCE_IF_CHAR
#undef READ
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
    PyUnicodeWriter_Discard(writer);
    Py_XDECREF(indices);
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

static PyObject *
translate_expression(PyObject *pattern, Py_ssize_t i, Py_ssize_t j)
{
    PyObject *chunks = PyList_New(0);
    if (chunks == NULL) {
        return NULL;
    }
    Py_ssize_t k = (PyUnicode_READ_CHAR(pattern, i) == '!') ? i + 2 : i + 1;
    Py_ssize_t chunkscount = 0;
    while (k < j) {
        PyObject *eobj = _PyObject_CallMethod(pattern, &_Py_ID(find), "sii", "-", k, j);
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
        chunkscount += 1;
        i = t + 1;
        k = t + 3;
    }
    if (i >= j) {
        assert(chunkscount > 0);
        PyObject *chunk = PyList_GET_ITEM(chunks, chunkscount - 1);
        assert(chunk != NULL);
        PyObject *hyphen = PyUnicode_FromOrdinal('-');
        if (hyphen == NULL) {
            goto abort;
        }
        PyObject *repl = PyUnicode_Concat(chunk, hyphen);
        Py_DECREF(hyphen);
        // PyList_SetItem() does not create a new reference on 'repl'
        // so we should not decref 'repl' after the call, unless there
        // is an issue while setting the item.
        if (repl == NULL || PyList_SetItem(chunks, chunkscount - 1, repl) < 0) {
            Py_XDECREF(repl);
            goto abort;
        }
    }
    else {
        PyObject *sub = PyUnicode_Substring(pattern, i, j);
        if (sub == NULL) {
            goto abort;
        }
        int rc = PyList_Append(chunks, sub);
        Py_DECREF(sub);
        if (rc < 0) {
            goto abort;
        }
        chunkscount += 1;
    }
    // remove empty ranges (they are not valid in RE)
    Py_ssize_t c = chunkscount;
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
            // all but the last character in the chunk
            PyObject *c1sub = PyUnicode_Substring(c1, 0, c1len - 1);
            // all but the first character in the chunk
            PyObject *c2sub = PyUnicode_Substring(c2, 1, c2len);
            if (c1sub == NULL || c2sub == NULL) {
                Py_XDECREF(c1sub);
                Py_XDECREF(c2sub);
                goto abort;
            }
            PyObject *merged = PyUnicode_Concat(c1sub, c2sub);
            Py_DECREF(c1sub);
            Py_DECREF(c2sub);
            // PyList_SetItem() does not create a new reference on 'merged'
            // so we should not decref 'merged' after the call, unless there
            // is an issue while setting the item.
            if (merged == NULL || PyList_SetItem(chunks, c - 1, merged) < 0) {
                Py_XDECREF(merged);
                goto abort;
            }
            if (PySequence_DelItem(chunks, c) < 0) {
                goto abort;
            }
            chunkscount--;
        }
    }
    assert(chunkscount == PyList_GET_SIZE(chunks));
    // Escape backslashes and hyphens for set difference (--),
    // but hyphens that create ranges should not be escaped.
    for (c = 0; c < chunkscount; ++c) {
        PyObject *s0 = PyList_GET_ITEM(chunks, c);
        assert(s0 != NULL);
        PyObject *s1 = _PyObject_CallMethod(s0, &_Py_ID(replace), "ss", "\\", "\\\\");
        if (s1 == NULL) {
            goto abort;
        }
        PyObject *s2 = _PyObject_CallMethod(s1, &_Py_ID(replace), "ss", "-", "\\-");
        Py_DECREF(s1);
        // PyList_SetItem() does not create a new reference on 's2'
        // so we should not decref 's2' after the call, unless there
        // is an issue while setting the item.
        if (s2 == NULL || PyList_SetItem(chunks, c, s2) < 0) {
            Py_XDECREF(s2);
            goto abort;
        }
    }
    PyObject *hyphen = PyUnicode_FromOrdinal('-');
    if (hyphen == NULL) {
        goto abort;
    }
    PyObject *res = PyUnicode_Join(hyphen, chunks);
    Py_DECREF(hyphen);
    if (res == NULL) {
        goto abort;
    }
    Py_DECREF(chunks);
    return res;
abort:
    Py_XDECREF(chunks);
    return NULL;
}

static Py_ssize_t
write_literal(State *state, PyUnicodeWriter *writer, PyObject *unicode)
{
    PyObject *escaped = PyObject_CallMethodOneArg(state->re_module,
                                                  &_Py_ID(escape),
                                                  unicode);
    if (escaped == NULL) {
        return -1;
    }
    Py_ssize_t written = PyUnicode_GET_LENGTH(escaped);
    assert(written >= 0);
    int rc = _WRITE_STRING(writer, escaped);
    Py_DECREF(escaped);
    if (rc < 0) {
        return -1;
    }
    assert(written > 0);
    return written;
}

static Py_ssize_t
write_expression(PyUnicodeWriter *writer, PyObject *expression)
{
#define WRITE_CHAR(c)           _WRITE_CHAR_OR(writer, (c), return -1)
#define WRITE_ASCII(s, n)       _WRITE_ASCII_OR(writer, (s), (n), return -1)
#define WRITE_BLOCK(s, i, j)    _WRITE_BLOCK_OR(writer, (s), (i), (j), return -1)
#define WRITE_STRING(s)         _WRITE_STRING_OR(writer, (s), return -1)
    Py_ssize_t grouplen = PyUnicode_GET_LENGTH(expression);
    if (grouplen == 0) {
        /* empty range: never match */
        WRITE_ASCII("(?!)", 4);
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
            WRITE_BLOCK(expression, 1, grouplen);
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
#undef WRITE_BLOCK
#undef WRITE_ASCII
#undef WRITE_CHAR
}

static PyObject *
process_wildcards(PyObject *pattern, PyObject *indices)
{
    const Py_ssize_t m = PyList_GET_SIZE(indices);
    if (m == 0) {
        // just write fr'(?s:{parts} + ")\Z"
        return PyUnicode_FromFormat("(?s:%S)\\Z", pattern);
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
     * write at least "indices[0] + 7m + n + 6" characters,
     * where 'm' is the number of stars and 'n' the length
     * of the translated pattern.
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
#define WRITE_BLOCK(i, j)       _WRITE_BLOCK_OR(writer, pattern, (i), (j), goto abort)
#define WRITE_ATOMIC_BEGIN()    _WRITE_ASCII_OR(writer, "(?>.*?", 6, goto abort)
#define WRITE_ATOMIC_END()      _WRITE_CHAR_OR(writer, ')', goto abort)
    WRITE_BLOCK(i, j);  // write stuff before '*' if needed
    i = j + 1;              // jump after the '*'
    for (Py_ssize_t k = 1; k < m; ++k) {
        PyObject *ind = PyList_GET_ITEM(indices, k);
        assert(ind != NULL);
        j = PyLong_AsSsize_t(ind);
        if (j < 0) {
            goto abort;
        }
        assert(i < j);
        // write the atomic RE group
        WRITE_ATOMIC_BEGIN();
        WRITE_BLOCK(i, j);
        WRITE_ATOMIC_END();
        i = j + 1;
    }
    // handle the last group
    _WRITE_ASCII_OR(writer, ".*", 2, goto abort);
    WRITE_BLOCK(i, n); // write the remaining substring (if non-empty)
#undef WRITE_BLOCK
#undef WRITE_ATOMIC_END
#undef WRITE_ATOMIC_BEGIN
    PyObject *res = PyUnicodeWriter_Finish(writer);
    if (res == NULL) {
        return NULL;
    }
    PyObject *formatted = PyUnicode_FromFormat("(?s:%S)\\Z", res);
    Py_DECREF(res);
    return formatted;
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
