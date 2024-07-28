/*
 * C accelerator for the translation function from UNIX shell patterns
 * to RE patterns.
 */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "macros.h"
#include "util.h"           // for get_fnmatchmodulestate_state()

#include "pycore_runtime.h" // for _Py_ID()

// ==== Helper declarations ===================================================

/*
 * Write re.escape(pattern[start:stop]).
 *
 * This returns the number of written characters, or -1 if an error occurred.
 *
 * @pre     0 <= start < stop <= len(pattern)
 */
static inline Py_ssize_t
escape_block(PyUnicodeWriter *writer,
             PyObject *pattern, Py_ssize_t start, Py_ssize_t stop,
             PyObject *re_escape_func);

/*
 * Construct a regular expression out of a UNIX-style expression.
 *
 * The expression to translate is the content of an '[(BLOCK)]' expression,
 * which contains single unicode characters or character ranges (e.g., 'a-z').
 *
 * By convention, 'start' and 'stop' represent the INCLUSIVE start index
 * and EXCLUSIVE stop index of BLOCK in 'pattern'. Stated otherwise:
 *
 *      pattern[start] == BLOCK[0]
 *      pattern[stop] == ']'
 *
 * For instance, for "ab[c-f]g[!1-5]", the values of 'start' and 'stop'
 * for the sub-pattern '[c-f]' are 3 and 6 respectively, while their
 * values for '[!1-5]' are 9 and 13 respectively.
 *
 * The 'pattern_str_find_meth' argument is a reference to pattern.find().
 */
static PyObject *
translate_expression(fnmatchmodule_state *state,
                     PyObject *pattern, Py_ssize_t start, Py_ssize_t stop,
                     PyObject *pattern_str_find_meth);

/*
 * Write the translated pattern obtained by translate_expression().
 *
 * This returns the number of written characters, or -1 if an error occurred.
 */
static Py_ssize_t
write_expression(fnmatchmodule_state *state,
                 PyUnicodeWriter *writer, PyObject *expression,
                 PyObject *setops_re_sub_meth);

/*
 * Build the final regular expression by processing the wildcards.
 *
 * The position of each wildcard in 'pattern' is given by 'indices'.
 */
static PyObject *
process_wildcards(PyObject *pattern, PyObject *indices);

// ==== API implementation ====================================================

static inline PyObject *
get_setops_re_sub_method(fnmatchmodule_state *state)
{
    PyObject *compiled = PyObject_CallMethodOneArg(state->re_module,
                                                   &_Py_ID(compile),
                                                   state->setops_str);
    PyObject *method = PyObject_GetAttr(compiled, &_Py_ID(sub));
    Py_DECREF(compiled);
    return method;
}

PyObject *
_Py_fnmatch_translate(PyObject *module, PyObject *pattern)
{
    assert(PyUnicode_Check(pattern));
    fnmatchmodule_state *state = get_fnmatchmodule_state(module);
    const Py_ssize_t maxind = PyUnicode_GET_LENGTH(pattern);

    // We would write less data if there are successive '*',
    // which usually happens once or twice in the pattern.
    // Otherwise, we write >= maxind characters since escaping
    // them always add more characters.
    //
    // Note that only '()[]{}?*+-|^$\\.&~# \t\n\r\v\f' need to
    // be escaped when translated to RE patterns and '*' and '?'
    // are already handled without being escaped.
    //
    // In general, UNIX style patterns are more likely to contain
    // wildcards than characters to be escaped, with the exception
    // of '-', '\' and '~' (we usually want to match filenmaes),
    // and there is a sparse number of them. Therefore, we only
    // estimate the number of characters to be written to be the
    // same as the number of characters in the pattern.
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(maxind);
    if (writer == NULL) {
        return NULL;
    }

    // ---- decl local objects ------------------------------------------------
    // list containing the indices where '*' has a special meaning
    PyObject *wildcard_indices = NULL;
    // cached functions (cache is local to the call)
    PyObject *re_escape_func = NULL;        // re.escape()
    PyObject *setops_re_subfn = NULL;       // re.compile('([&~|])').sub()
    PyObject *pattern_str_find_meth = NULL; // pattern.find()
    // ---- def local objects -------------------------------------------------
    wildcard_indices = PyList_New(0);
    CHECK_NOT_NULL_OR_ABORT(wildcard_indices);
    // The Python implementation always takes queries re.escape() and re.sub()
    // inside translate() and thus we should at least allow external users to
    // mock those functions (thus, we cannot cache them in the module's state).
    re_escape_func = PyObject_GetAttr(state->re_module, &_Py_ID(escape));
    CHECK_NOT_NULL_OR_ABORT(re_escape_func);
    setops_re_subfn = get_setops_re_sub_method(state);
    CHECK_NOT_NULL_OR_ABORT(setops_re_subfn);
    pattern_str_find_meth = PyObject_GetAttr(pattern, &_Py_ID(find));
    CHECK_NOT_NULL_OR_ABORT(pattern_str_find_meth);
    // ------------------------------------------------------------------------
    const int pattern_kind = PyUnicode_KIND(pattern);
    const void *const pattern_data = PyUnicode_DATA(pattern);
    // ---- def local macros --------------------------------------------------
#define READ_CHAR(IND)  PyUnicode_READ(pattern_kind, pattern_data, IND)
    /* advance IND if the character is CHAR */
#define ADVANCE_IF_CHAR_IS(CHAR, IND, MAXIND)               \
    do {                                                    \
        if ((IND) < (MAXIND) && READ_CHAR(IND) == (CHAR)) { \
            ++IND;                                          \
        }                                                   \
    } while (0)
#define WRITE_PENDING(ESCSTOP)                                  \
    do {                                                        \
        if (escstart != -1) {                                   \
            Py_ssize_t t = escape_block(writer, pattern,        \
                                        escstart, (ESCSTOP),    \
                                        re_escape_func);        \
            if (t < 0) {                                        \
                goto abort;                                     \
            }                                                   \
            written += t;                                       \
            escstart = -1;                                      \
        }                                                       \
    } while (0)
    // ------------------------------------------------------------------------
    Py_ssize_t i = 0;                       // current index
    Py_ssize_t written = 0;                 // number of characters written
    Py_ssize_t escstart = -1, escstop = -1; // start/stop escaping indices
    while ((escstop = i) < maxind) {
        Py_UCS4 chr = READ_CHAR(i++);
        switch (chr) {
            case '*': {
                WRITE_PENDING(escstop);
                // translate wildcard '*' (fnmatch) into optional '.' (regex)
                WRITE_CHAR_OR_ABORT(writer, '*');
                // skip duplicated '*'
                for (; i < maxind && READ_CHAR(i) == '*'; ++i);
                // store the position of the wildcard
                PyObject *wildcard_index = PyLong_FromSsize_t(written++);
                CHECK_NOT_NULL_OR_ABORT(wildcard_index);
                int rc = PyList_Append(wildcard_indices, wildcard_index);
                Py_DECREF(wildcard_index);
                CHECK_RET_CODE_OR_ABORT(rc);
                break;
            }
            case '?': {
                WRITE_PENDING(escstop);
                // translate optional '?' (fnmatch) into optional '.' (regex)
                WRITE_CHAR_OR_ABORT(writer, '.');
                ++written; // increase the expected result's length
                break;
            }
            case '[': {
                WRITE_PENDING(escstop);
                assert(READ_CHAR(escstop) == '[');
                Py_ssize_t j = i;
                ADVANCE_IF_CHAR_IS('!', j, maxind);             // [!
                ADVANCE_IF_CHAR_IS(']', j, maxind);             // [!] or []
                for (; j < maxind && READ_CHAR(j) != ']'; ++j); // locate ']'
                if (j >= maxind) {
                    WRITE_ASCII_OR_ABORT(writer, "\\[", 2);
                    written += 2;   // we just wrote 2 characters
                    break;          // explicit early break for clarity
                }
                else {
                    assert(READ_CHAR(j) == ']');
                    Py_ssize_t pos = PyUnicode_FindChar(pattern, '-', i, j, 1);
                    if (pos == -2) {
                        goto abort;
                    }
                    PyObject *expr = NULL;
                    if (pos == -1) {
                        PyObject *tmp = PyUnicode_Substring(pattern, i, j);
                        CHECK_NOT_NULL_OR_ABORT(tmp);
                        expr = BACKSLASH_REPLACE(state, tmp);
                        Py_DECREF(tmp);
                    }
                    else {
                        expr = translate_expression(state, pattern, i, j,
                                                    pattern_str_find_meth);
                    }
                    CHECK_NOT_NULL_OR_ABORT(expr);
                    Py_ssize_t expr_len = write_expression(state, writer, expr,
                                                           setops_re_subfn);
                    Py_DECREF(expr);
                    if (expr_len < 0) {
                        goto abort;
                    }
                    written += expr_len;
                    i = j + 1;  // jump to the character after ']'
                    break;      // explicit early break for clarity
                }
            }
            default: {
                if (escstart == -1) {
                    assert(i >= 1);
                    escstart = i - 1;
                }
                break;
            }
        }
    }
    WRITE_PENDING(maxind);
#undef WRITE_PENDING
#undef ADVANCE_IF_CHAR_IS
#undef READ_CHAR
    Py_DECREF(pattern_str_find_meth);
    Py_DECREF(setops_re_subfn);
    Py_DECREF(re_escape_func);
    PyObject *translated = PyUnicodeWriter_Finish(writer);
    if (translated == NULL) {
        Py_DECREF(wildcard_indices);
        return NULL;
    }
    PyObject *res = process_wildcards(translated, wildcard_indices);
    Py_DECREF(translated);
    Py_DECREF(wildcard_indices);
    return res;
abort:
    Py_XDECREF(pattern_str_find_meth);
    Py_XDECREF(setops_re_subfn);
    Py_XDECREF(re_escape_func);
    Py_XDECREF(wildcard_indices);
    PyUnicodeWriter_Discard(writer);
    return NULL;
}

// ==== Helper implementations ================================================

static inline Py_ssize_t
escape_block(PyUnicodeWriter *writer,
             PyObject *pattern, Py_ssize_t start, Py_ssize_t stop,
             PyObject *re_escape_func)
{
#ifdef Py_DEBUG
    if (start < 0 || start >= stop || stop > PyUnicode_GET_LENGTH(pattern)) {
        PyErr_BadInternalCall();
        return -1;
    }
#endif
    PyObject *str = PyUnicode_Substring(pattern, start, stop);
    CHECK_NOT_NULL_OR_ABORT(str);
    PyObject *escaped = PyObject_CallOneArg(re_escape_func, str);
    Py_DECREF(str);
    CHECK_NOT_NULL_OR_ABORT(escaped);
    Py_ssize_t written = PyUnicode_GET_LENGTH(escaped);
    int rc = _WRITE_STRING(writer, escaped);
    Py_DECREF(escaped);
    CHECK_RET_CODE_OR_ABORT(rc);
    return written;
abort:
    return -1;
}

/*
 * Extract a list of chunks from the pattern group described by start and stop.
 *
 * For instance, the chunks for [a-z0-9] or [!a-z0-9] are ['a', 'z0', '9'].
 *
 * See translate_expression() for its usage.
 */
static PyObject *
split_expression(fnmatchmodule_state *state,
                 PyObject *pattern, Py_ssize_t start, Py_ssize_t stop,
                 PyObject *str_find_func)
{
    // ---- decl local objects ------------------------------------------------
    PyObject *chunks = NULL, *maxind = NULL;
    PyObject *hyphen = state->hyphen_str;
    // ---- def local objects -------------------------------------------------
    chunks = PyList_New(0);
    CHECK_NOT_NULL_OR_ABORT(chunks);
    maxind = PyLong_FromSsize_t(stop);
    CHECK_NOT_NULL_OR_ABORT(maxind);
    // ---- def local macros --------------------------------------------------
    /* add pattern[START:STOP] to the list of chunks */
#define ADD_CHUNK(START, STOP)                                              \
    do {                                                                    \
        PyObject *chunk = PyUnicode_Substring(pattern, (START), (STOP));    \
        CHECK_NOT_NULL_OR_ABORT(chunk);                                     \
        int rc = PyList_Append(chunks, chunk);                              \
        Py_DECREF(chunk);                                                   \
        CHECK_RET_CODE_OR_ABORT(rc);                                        \
    } while (0)
    // ------------------------------------------------------------------------
    Py_ssize_t chunk_start = start;
    bool is_complement = PyUnicode_READ_CHAR(pattern, start) == '!';
    // skip '!' character (it is handled separately in write_expression())
    Py_ssize_t ind = is_complement ? start + 2 : start + 1;
    while (ind < stop) {
        PyObject *p_chunk_stop = PyObject_CallFunction(str_find_func, "OnO",
                                                       hyphen, ind, maxind);
        CHECK_NOT_NULL_OR_ABORT(p_chunk_stop);
        Py_ssize_t chunk_stop = PyLong_AsSsize_t(p_chunk_stop);
        Py_DECREF(p_chunk_stop);
        if (chunk_stop < 0) {
            if (PyErr_Occurred()) {
                goto abort;
            }
            // -1 here means that '-' was not found
            assert(chunk_stop == -1);
            break;
        }
        ADD_CHUNK(chunk_start, chunk_stop);
        chunk_start = chunk_stop + 1;    // jump after '-'
        ind = chunk_stop + 3;            // ensure a non-empty next chunk
    }
    if (chunk_start < stop) {
        ADD_CHUNK(chunk_start, stop);
    }
    else {
        Py_ssize_t chunkscount = PyList_GET_SIZE(chunks);
        assert(chunkscount > 0);
        PyObject *chunk = PyList_GET_ITEM(chunks, chunkscount - 1);
        assert(chunk != NULL);
        PyObject *str = PyUnicode_Concat(chunk, hyphen);
        // PyList_SetItem() does not create a new reference on 'str'
        // so we should not decref 'str' after the call, unless there
        // is an issue while setting the item.
        if (str == NULL || PyList_SetItem(chunks, chunkscount - 1, str) < 0) {
            Py_XDECREF(str);
            goto abort;
        }
    }
#undef ADD_CHUNK
    Py_DECREF(maxind);
    return chunks;
abort:
    Py_XDECREF(maxind);
    Py_XDECREF(chunks);
    return NULL;
}

/*
 * Remove empty ranges (they are invalid in RE).
 *
 * See translate_expression() for its usage.
 */
static int
simplify_expression(PyObject *chunks)
{
    // for k in range(len(chunks) - 1, 0, -1):
    for (Py_ssize_t k = PyList_GET_SIZE(chunks) - 1; k > 0; --k) {
        PyObject *c1 = PyList_GET_ITEM(chunks, k - 1);
        assert(c1 != NULL);
        Py_ssize_t c1len = PyUnicode_GET_LENGTH(c1);
        assert(c1len > 0);

        PyObject *c2 = PyList_GET_ITEM(chunks, k);
        assert(c2 != NULL);
        Py_ssize_t c2len = PyUnicode_GET_LENGTH(c2);
        assert(c2len > 0);

        if (PyUnicode_READ_CHAR(c1, c1len - 1) > PyUnicode_READ_CHAR(c2, 0)) {
            Py_ssize_t olen = c1len + c2len - 2;
            assert(olen >= 0);
            PyObject *str = NULL;
            if (olen == 0) {        // c1[:1] + c2[1:] == ''
                str = Py_GetConstant(Py_CONSTANT_EMPTY_STR);
                assert(_Py_IsImmortal(str));
            }
            else if (c1len == 1) {  // c1[:1] + c2[1:] == c2[1:]
                assert(c2len > 1);
                str = PyUnicode_Substring(c2, 1, c2len);
            }
            else if (c2len == 1) {  // c1[:1] + c2[1:] == c1[:1]
                assert(c1len > 1);
                str = PyUnicode_Substring(c1, 0, c1len - 1);
            }
            else {
                assert(c1len > 1);
                assert(c2len > 1);
                PyUnicodeWriter *writer = PyUnicodeWriter_Create(olen);
                CHECK_NOT_NULL_OR_ABORT(writer);
                // all but the last character in the first chunk
                if (_WRITE_SUBSTRING(writer, c1, 0, c1len - 1) < 0) {
                    PyUnicodeWriter_Discard(writer);
                    goto abort;
                }
                // all but the first character in the second chunk
                if (_WRITE_SUBSTRING(writer, c2, 1, c2len) < 0) {
                    PyUnicodeWriter_Discard(writer);
                    goto abort;
                }
                str = PyUnicodeWriter_Finish(writer);
            }
            // PyList_SetItem() does not create a new reference on 'str'
            // so we should not decref 'str' after the call, unless there
            // is an issue while setting the item.
            if (str == NULL || PyList_SetItem(chunks, k - 1, str) < 0) {
                Py_XDECREF(str);
                goto abort;
            }
            CHECK_RET_CODE_OR_ABORT(PySequence_DelItem(chunks, k));
        }
    }
    return 0;
abort:
    return -1;
}

/*
 * Escape backslashes and hyphens for set difference (--),
 * but hyphens that create ranges should not be escaped.
 *
 * See translate_expression() for its usage.
 */
static int
escape_expression(fnmatchmodule_state *state, PyObject *chunks)
{
    for (Py_ssize_t c = 0; c < PyList_GET_SIZE(chunks); ++c) {
        PyObject *s0 = PyList_GET_ITEM(chunks, c);
        assert(s0 != NULL);
        PyObject *s1 = BACKSLASH_REPLACE(state, s0);
        CHECK_NOT_NULL_OR_ABORT(s1);
        PyObject *s2 = HYPHEN_REPLACE(state, s1);
        Py_DECREF(s1);
        // PyList_SetItem() does not create a new reference on 's2'
        // so we should not decref 's2' after the call, unless there
        // is an issue while setting the item.
        if (s2 == NULL || PyList_SetItem(chunks, c, s2) < 0) {
            Py_XDECREF(s2);
            goto abort;
        }
    }
    return 0;
abort:
    return -1;
}

static PyObject *
translate_expression(fnmatchmodule_state *state,
                     PyObject *pattern, Py_ssize_t start, Py_ssize_t stop,
                     PyObject *pattern_str_find_meth)
{
    PyObject *chunks = split_expression(state, pattern, start, stop,
                                        pattern_str_find_meth);
    CHECK_NOT_NULL_OR_ABORT(chunks);
    // remove empty ranges
    CHECK_RET_CODE_OR_ABORT(simplify_expression(chunks));
    // escape backslashes and set differences
    CHECK_RET_CODE_OR_ABORT(escape_expression(state, chunks));
    PyObject *res = PyUnicode_Join(state->hyphen_str, chunks);
    Py_DECREF(chunks);
    return res;
abort:
    Py_XDECREF(chunks);
    return NULL;
}

static Py_ssize_t
write_expression(fnmatchmodule_state *state,
                 PyUnicodeWriter *writer, PyObject *expression,
                 PyObject *setops_re_sub_meth)
{
    PyObject *safe_expression = NULL;  // for the 'goto abort' statements
    Py_ssize_t grouplen = PyUnicode_GET_LENGTH(expression);
    if (grouplen == 0) {
        // empty range: never match
        WRITE_ASCII_OR_ABORT(writer, "(?!)", 4);
        return 4;
    }
    Py_UCS4 token = PyUnicode_READ_CHAR(expression, 0);
    if (grouplen == 1 && token == '!') {
        // negated empty range: match any character
        WRITE_CHAR_OR_ABORT(writer, '.');
        return 1;
    }
    Py_ssize_t extra = 2; // '[' and ']'
    WRITE_CHAR_OR_ABORT(writer, '[');
    // escape set operations as late as possible
    safe_expression = SETOPS_REPLACE(state, expression, setops_re_sub_meth);
    CHECK_NOT_NULL_OR_ABORT(safe_expression);
    switch (token) {
        case '!': {
            WRITE_CHAR_OR_ABORT(writer, '^'); // replace '!' by '^'
            WRITE_SUBSTRING_OR_ABORT(writer, safe_expression, 1, grouplen);
            break;
        }
        case '^':
        case '[': {
            WRITE_CHAR_OR_ABORT(writer, '\\');
            ++extra; // because we wrote '\\'
            WRITE_STRING_OR_ABORT(writer, safe_expression);
            break;
        }
        default: {
            WRITE_STRING_OR_ABORT(writer, safe_expression);
            break;
        }
    }
    Py_DECREF(safe_expression);
    WRITE_CHAR_OR_ABORT(writer, ']');
    return grouplen + extra;
abort:
    Py_XDECREF(safe_expression);
    return -1;
}

static PyObject *
process_wildcards(PyObject *pattern, PyObject *indices)
{
    const Py_ssize_t n = PyUnicode_GET_LENGTH(pattern);
    const Py_ssize_t m = PyList_GET_SIZE(indices);
    // Let m = len(indices) and n = len(pattern). By construction,
    //
    //      pattern = [PREFIX] [[(* INNER) ... (* INNER)] (*) [OUTER]]
    //
    // where [...] is an optional group and (...) is a required group.
    //
    // The algorithm is as follows:
    //
    //  - Write "(?s:".
    //  - Write the optional PREFIX.
    //  - Write an INNER group (* INNER) as "(?>.*?" + INNER + ")".
    //  - Write ".*" instead of the last wildcard.
    //  - Write an optional OUTER string normally.
    //  - Write ")\\Z".
    //
    // If m = 0, the writer needs n + 7 characters. Otherwise, it requires
    // exactly n + 6(m-1) + 1 + 7 = n + 6m + 2 characters, where the "+1"
    // is due to the fact that writing ".*" instead of "*" only increases
    // the total length of the pattern by 1 (and not by 2).
    const Py_ssize_t reslen = m == 0 ? (n + 7) : (n + 6 * m + 2);
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(reslen);
    if (writer == NULL) {
        return NULL;
    }
    // ---- def local macros --------------------------------------------------
#define LOAD_WILDCARD_INDEX(VAR, IND)                               \
    do {                                                            \
        VAR = PyLong_AsSsize_t(PyList_GET_ITEM(indices, (IND)));    \
        if ((VAR) < 0 && PyErr_Occurred())  {                       \
            goto abort;                                             \
        }                                                           \
    } while (0)
    // ------------------------------------------------------------------------
    WRITE_ASCII_OR_ABORT(writer, "(?s:", 4);
    if (m == 0) {
        WRITE_STRING_OR_ABORT(writer, pattern);
    }
    else {
        Py_ssize_t i = 0, j = -1;
        // process the optional PREFIX
        LOAD_WILDCARD_INDEX(j, 0);
        WRITE_SUBSTRING_OR_ABORT(writer, pattern, i, j);
        i = j + 1;
        for (Py_ssize_t k = 1; k < m; ++k) {
            // process the (* INNER) groups
            LOAD_WILDCARD_INDEX(j, k);
            assert(i < j);
            // write the atomic RE group '(?>.*?' + INNER + ')'
            WRITE_ASCII_OR_ABORT(writer, "(?>.*?", 6);
            WRITE_SUBSTRING_OR_ABORT(writer, pattern, i, j);
            WRITE_CHAR_OR_ABORT(writer, ')');
            i = j + 1;
        }
        // handle the (*) [OUTER] part
        WRITE_ASCII_OR_ABORT(writer, ".*", 2);
        WRITE_SUBSTRING_OR_ABORT(writer, pattern, i, n);
    }
    WRITE_ASCII_OR_ABORT(writer, ")\\Z", 3);
    PyObject *res = PyUnicodeWriter_Finish(writer);
    assert(res == NULL || PyUnicode_GET_LENGTH(res) == reslen);
    return res;
abort:
    PyUnicodeWriter_Discard(writer);
    return NULL;
#undef LOAD_WILDCARD_INDEX
}
