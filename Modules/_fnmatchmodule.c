/*
 * C accelerator for the 'fnmatch' module (POSIX only).
 *
 * Most functions expect string or bytes instances, and thus the Python
 * implementation should first pre-process path-like objects, possibly
 * applying normalizations depending on the platform if needed.
 */

#include "Python.h"
#include "pycore_call.h" // for _PyObject_CallMethod

#include "clinic/_fnmatchmodule.c.h"

#define INVALID_PATTERN_TYPE "pattern must be a string or a bytes object"

// module state functions

typedef struct {
    PyObject *re_module; // 're' module
    PyObject *os_module; // 'os' module

    PyObject *lru_cache; // optional cache for regex patterns, if needed

    PyObject *str_atomic_bgroup;    // (?>.*?
    PyObject *str_atomic_egroup;    // )
    PyObject *str_wildcard;         // *
} fnmatchmodule_state;

static inline fnmatchmodule_state *
get_fnmatchmodulestate_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (fnmatchmodule_state *) state;
}

static int
fnmatchmodule_clear(PyObject *m)
{
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(m);
    Py_CLEAR(st->os_module);
    Py_CLEAR(st->re_module);
    Py_CLEAR(st->lru_cache);

    Py_CLEAR(st->str_atomic_bgroup);
    Py_CLEAR(st->str_atomic_egroup);
    Py_CLEAR(st->str_wildcard);
    return 0;
}

static int
fnmatchmodule_traverse(PyObject *m, visitproc visit, void *arg)
{
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(m);
    Py_VISIT(st->os_module);
    Py_VISIT(st->re_module);
    Py_VISIT(st->lru_cache);

    Py_VISIT(st->str_atomic_bgroup);
    Py_VISIT(st->str_atomic_egroup);
    Py_VISIT(st->str_wildcard);
    return 0;
}

static void
fnmatchmodule_free(void *m)
{
    fnmatchmodule_clear((PyObject *) m);
}

static int
fnmatchmodule_exec(PyObject *m)
{
#define IMPORT_MODULE(attr, name) \
    do { \
        state->attr = PyImport_ImportModule((name)); \
        if (state->attr == NULL) { \
            return -1; \
        } \
    } while (0)

#define INTERN_STRING(attr, str) \
    do { \
        state->attr = PyUnicode_InternFromString((str)); \
        if (state->attr == NULL) { \
            return -1; \
        } \
    } while (0)

    fnmatchmodule_state *state = get_fnmatchmodulestate_state(m);

    // imports
    IMPORT_MODULE(os_module, "os");
    IMPORT_MODULE(re_module, "re");

    // helpers
    state->lru_cache = _PyImport_GetModuleAttrString("functools", "lru_cache");
    if (state->lru_cache == NULL) {
        return -1;
    }
    // todo: handle LRU cache

    // interned strings
    INTERN_STRING(str_atomic_bgroup, "(?>.*?");
    INTERN_STRING(str_atomic_egroup, ")");
    INTERN_STRING(str_wildcard, "*");

#undef INTERN_STRING
#undef IMPORT_MODULE

    return 0;
}

/*[clinic input]
module _fnmatch
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=356e324d57d93f08]*/

#ifdef Py_HAVE_FNMATCH
#include <fnmatch.h>

#define VERIFY_NAME_ARG_TYPE(name, check, expecting) \
    do { \
        if (!check) { \
            PyErr_Format(PyExc_TypeError, \
                         "name must be a %s object, got %.200s", \
                         expecting, Py_TYPE(name)->tp_name); \
            return -1; \
        } \
    } while (0)

#define PROCESS_MATCH_RESULT(r) \
    do { \
        int res = (r); /* avoid variable capture */ \
        if (res < 0) { \
            return res; \
        } \
        return res != FNM_NOMATCH; \
    } while (0)

/*
 * Perform a case-sensitive match using fnmatch(3).
 *
 * Parameters
 *
 *      pattern     A UNIX shell pattern.
 *      name        The filename to match (bytes object).
 *
 * Returns 1 if the 'name' matches the 'pattern' and 0 otherwise.
 *
 * Returns -1 if (1) 'name' is not a `bytes` object, and
 * sets a TypeError exception, or (2) something went wrong.
 */
static inline int
posix_fnmatch_encoded(const char *pattern, PyObject *name)
{
    VERIFY_NAME_ARG_TYPE(name, PyBytes_Check(name), "bytes");
    PROCESS_MATCH_RESULT(fnmatch(pattern, PyBytes_AS_STRING(name), 0));
}

/* Same as `posix_fnmatch_encoded` but for string-like objects. */
static inline int
posix_fnmatch_unicode(const char *pattern, PyObject *name)
{
    VERIFY_NAME_ARG_TYPE(name, PyUnicode_Check(name), "string");
    PROCESS_MATCH_RESULT(fnmatch(pattern, PyUnicode_AsUTF8(name), 0));
}

static PyObject *
posix_fnmatch_filter(const char *pattern, PyObject *names,
                     int (*match)(const char *, PyObject *))
{
    PyObject *iter = PyObject_GetIter(names);
    if (iter == NULL) {
        return NULL;
    }

    PyObject *res = PyList_New(0);
    if (res == NULL) {
        Py_DECREF(iter);
        return NULL;
    }

    PyObject *name = NULL;
    while ((name = PyIter_Next(iter))) {
        int rc = match(pattern, name);
        if (rc < 0) {
            goto abort;
        }
        if (rc == 1) {
            if (PyList_Append(res, name) < 0) {
                goto abort;
            }
        }
        Py_DECREF(name);
        if (PyErr_Occurred()) {
            Py_DECREF(res);
            Py_DECREF(iter);
            return NULL;
        }
    }
    Py_DECREF(iter);
    return res;
abort:
    Py_XDECREF(name);
    Py_DECREF(iter);
    Py_DECREF(res);
    return NULL;
}
#else

static PyObject *
get_match_function(PyObject *module, PyObject *pattern)
{
    PyObject *expr = _fnmatch_translate_impl(module, pattern);
    if (expr == NULL) {
        return NULL;
    }
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(module);
    PyObject *compiled = PyObject_CallMethod(st->re_module, "compile", "O", expr);
    Py_DECREF(expr);
    if (compiled == NULL) {
        return NULL;
    }
    PyObject *matcher = PyObject_GetAttr(compiled, &_Py_ID(match));
    Py_DECREF(compiled);
    return matcher;
}

static PyMethodDef get_match_function_method_def = {
    "get_match_function",
    _PyCFunction_CAST(get_match_function),
    METH_O,
    NULL
};

/*
 * Perform a case-sensitive match using regular expressions.
 *
 * Parameters
 *
 *      pattern     A translated regular expression.
 *      name        The filename to match.
 *
 * Returns 1 if the 'name' matches the 'pattern' and 0 otherwise.
 * Returns -1 if something went wrong.
 */
static inline int
regex_fnmatch_generic(PyObject *matcher, PyObject *name)
{
    // If 'name' is of incorrect type, it will be detected when calling
    // the matcher function (we emulate 're.compile(...).match(name)').
    PyObject *match = PyObject_CallFunction(matcher, "O", name);
    if (match == NULL) {
        return -1;
    }
    int matching = match != Py_None;
    Py_DECREF(match);
    return matching;
}

static PyObject *
regex_fnmatch_filter(PyObject *matcher, PyObject *names)
{
    PyObject *iter = PyObject_GetIter(names);
    if (iter == NULL) {
        return NULL;
    }

    PyObject *res = PyList_New(0);
    if (res == NULL) {
        Py_DECREF(iter);
        return NULL;
    }

    PyObject *name = NULL;
    while ((name = PyIter_Next(iter))) {
        int rc = regex_fnmatch_generic(matcher, name);
        if (rc < 0) {
            goto abort;
        }
        if (rc == 1) {
            if (PyList_Append(res, name) < 0) {
                goto abort;
            }
        }
        Py_DECREF(name);
        if (PyErr_Occurred()) {
            Py_DECREF(res);
            Py_DECREF(iter);
            return NULL;
        }
    }
    Py_DECREF(iter);
    return res;
abort:
    Py_XDECREF(name);
    Py_DECREF(iter);
    Py_DECREF(res);
    return NULL;
}
#endif

/*[clinic input]
_fnmatch.filter -> object

    names: object
    pat: object

[clinic start generated code]*/

static PyObject *
_fnmatch_filter_impl(PyObject *module, PyObject *names, PyObject *pat)
/*[clinic end generated code: output=7f11aa68436d05fc input=1d233174e1c4157a]*/
{
#ifndef Py_HAVE_FNMATCH
    PyObject *matcher = get_match_function(module, pat);
    if (matcher == NULL) {
        return NULL;
    }
    PyObject *result = regex_fnmatch_filter(matcher, names);
    Py_DECREF(matcher);
    return result;
#else
    // Note that the Python implementation of fnmatch.filter() does not
    // call os.fspath() on the names being matched, whereas it does on NT.
    if (PyBytes_Check(pat)) {
        const char *pattern = PyBytes_AS_STRING(pat);
        return posix_fnmatch_filter(pattern, names, &posix_fnmatch_encoded);
    }
    if (PyUnicode_Check(pat)) {
        const char *pattern = PyUnicode_AsUTF8(pat);
        return posix_fnmatch_filter(pattern, names, &posix_fnmatch_unicode);
    }
    PyErr_SetString(PyExc_TypeError, INVALID_PATTERN_TYPE);
    return NULL;
#endif
}

/*[clinic input]
_fnmatch.fnmatchcase -> bool

    name: object
    pat: object

Test whether `name` matches `pattern`, including case.

This is a version of fnmatch() which doesn't case-normalize
its arguments.

[clinic start generated code]*/

static int
_fnmatch_fnmatchcase_impl(PyObject *module, PyObject *name, PyObject *pat)
/*[clinic end generated code: output=4d1283b1b1fc7cb8 input=b02a6a5c8c5a46e2]*/
{
#ifndef Py_HAVE_FNMATCH
    PyObject *matcher = get_match_function(module, pat);
    if (matcher == NULL) {
        return -1;
    }
    int res = regex_fnmatch_generic(matcher, name);
    Py_DECREF(matcher);
    return res;
#else
    // This function does not transform path-like objects, nor does it
    // case-normalize 'name' or 'pattern' (whether it is the Python or
    // the C implementation).
    if (PyBytes_Check(pat)) {
        const char *pattern = PyBytes_AS_STRING(pat);
        return posix_fnmatch_encoded(pattern, name);
    }
    if (PyUnicode_Check(pat)) {
        const char *pattern = PyUnicode_AsUTF8(pat);
        return posix_fnmatch_unicode(pattern, name);
    }
    PyErr_SetString(PyExc_TypeError, INVALID_PATTERN_TYPE);
    return -1;
#endif
}

/*
 * Convert Py_UCS4 to (PyObject *).
 *
 * This creates a new reference.
 *
 * Note: this is 'unicode_char' taken from Objects/unicodeobject.c.
 */
static PyObject *
get_unicode_character(Py_UCS4 ch)
{
    assert(ch <= MAX_UNICODE);
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
        PyUnicode_2BYTE_DATA(unicode)[0] = (Py_UCS2) ch;
    }
    else {
        assert(PyUnicode_KIND(unicode) == PyUnicode_4BYTE_KIND);
        PyUnicode_4BYTE_DATA(unicode)[0] = ch;
    }
    assert(_PyUnicode_CheckConsistency(unicode, 1));
    return unicode;
}

static Py_ssize_t /* number of written characters or -1 on error */
write_escaped_string(PyObject *re, _PyUnicodeWriter *writer, PyObject *str)
{
    PyObject *escaped = PyObject_CallMethodOneArg(re, &_Py_ID(escape), str);
    if (escaped == NULL) {
        return -1;
    }
    Py_ssize_t written = PyUnicode_GET_LENGTH(escaped);
    int rc = _PyUnicodeWriter_WriteStr(writer, escaped);
    Py_DECREF(escaped);
    if (rc < 0) {
        return -1;
    }
    assert(written > 0);
    return written;
}

static Py_ssize_t /* number of written characters or -1 on error */
write_translated_group(_PyUnicodeWriter *writer, PyObject *group)
{
#define WRITE_ASCII(str, len) \
    do { \
        if (_PyUnicodeWriter_WriteASCIIString(writer, (str), (len)) < 0) { \
            return -1; \
        } \
    } while (0)

#define WRITE_CHAR(c) \
    do { \
        if (_PyUnicodeWriter_WriteChar(writer, (c)) < 0) { \
            return -1; \
        } \
    } while (0)

    Py_ssize_t grouplen;
    const char *buffer = PyUnicode_AsUTF8AndSize(group, &grouplen);
    if (grouplen == 0) {
        /* empty range: never match */
        WRITE_ASCII("(?!)", 4);
        return 4;
    }
    else if (grouplen == 1 && buffer[0] == '!') {
        /* negated empty range: match any character */
        WRITE_CHAR('.');
        return 1;
    }
    else {
        Py_ssize_t extra = 2; // '[' and ']'
        WRITE_CHAR('[');
        switch (buffer[0]) {
            case '!': {
                WRITE_CHAR('^');
                if (_PyUnicodeWriter_WriteSubstring(writer, group, 1, grouplen) < 0) {
                    return -1;
                }
                break;
            }
            case '^':
            case '[': {
                WRITE_CHAR('\\');
                extra++;
                break;
            }
            default:
                if (_PyUnicodeWriter_WriteStr(writer, group) < 0) {
                    return -1;
                }
                break;
        }
        WRITE_CHAR(']');
        return grouplen + extra;
    }
#undef WRITE_CHAR
#undef WRITE_ASCII
}

static PyObject *
get_translated_group(PyObject *pattern,
                     Py_ssize_t i /* pattern[i-1] == '[' (incl.) */,
                     Py_ssize_t j /* pattern[j]   == ']' (excl.) */)
{
    PyObject *chunks = PyList_New(0);
    if (chunks == NULL) {
        return NULL;
    }
    Py_ssize_t k = (PyUnicode_READ_CHAR(pattern, i) == '!') ? i + 2 : i + 1;
    Py_ssize_t chunkscount = 0;
    while (k < j) {
        PyObject *eobj = _PyObject_CallMethod(pattern, &_Py_ID(find), "ii", k, j);
        if (eobj == NULL) {
            goto error;
        }
        Py_ssize_t t = PyLong_AsSsize_t(eobj);
        Py_DECREF(eobj);
        if (t < 0) {
            goto error;
        }
        PyObject *sub = PyUnicode_Substring(pattern, i, t);
        if (sub == NULL) {
            goto error;
        }
        int rc = PyList_Append(chunks, sub);
        Py_DECREF(sub);
        if (rc < 0) {
            goto error;
        }
        chunkscount += 1;
        i = t + 1;
        k = t + 3;
    }
    if (i >= j) {
        assert(chunkscount > 0);
        PyObject *chunk = PyList_GET_ITEM(chunks, chunkscount - 1);
        PyObject *hyphen = PyUnicode_FromOrdinal('-');
        if (hyphen == NULL) {
            goto error;
        }
        PyObject *repl = PyUnicode_Concat(chunk, hyphen);
        Py_DECREF(hyphen);
        int rc = PyList_SetItem(chunks, chunkscount - 1, repl);
        Py_DECREF(repl);
        if (rc < 0) {
            goto error;
        }
    }
    else {
        PyObject *sub = PyUnicode_Substring(pattern, i, j);
        if (sub == NULL) {
            goto error;
        }
        int rc = PyList_Append(chunks, sub);
        Py_DECREF(sub);
        if (rc < 0) {
            goto error;
        }
        chunkscount += 1;
    }
    // remove empty ranges (they are not valid in RE)
    Py_ssize_t c = chunkscount;
    while (--c) {
        PyObject *c1 = PyList_GET_ITEM(chunks, c - 1);
        assert(c1len > 0);
        Py_ssize_t c1len = PyUnicode_GET_LENGTH(c1);
        assert(c1 != NULL);

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
                goto error;
            }
            PyObject *merged = PyUnicode_Concat(c1sub, c2sub);
            Py_DECREF(c1sub);
            Py_DECREF(c2sub);
            if (merged == NULL) {
                goto error;
            }
            int rc = PyList_SetItem(chunks, c - 1, merged);
            Py_DECREF(merged);
            if (rc < 0) {
                goto error;
            }
            if (PySequence_DelItem(chunks, c) < 0) {
                goto error;
            }
            chunkscount--;
        }
    }
    // Escape backslashes and hyphens for set difference (--),
    // but hyphens that create ranges should not be escaped.
    for (c = 0; c < chunkscount; ++c) {
        PyObject *s0 = PyList_GetItem(chunks, c);
        if (s0 == NULL) {
            goto error;
        }
        PyObject *s1 = PyObject_CallMethod(s0, "replace", "ss", "\\", "\\\\");
        if (s1 == NULL) {
            goto error;
        }
        PyObject *s2 = PyObject_CallMethod(s1, "replace", "ss", "-", "\\-");
        Py_DECREF(s1);
        if (s2 == NULL) {
            goto error;
        }
        if (PyList_SetItem(chunks, c, s2) < 0) {
            goto error;
        }
    }
    PyObject *hyphen = PyUnicode_FromOrdinal('-');
    if (hyphen == NULL) {
        goto error;
    }
    PyObject *res = PyUnicode_Join(hyphen, chunks);
    Py_DECREF(hyphen);
    if (res == NULL) {
        goto error;
    }
    Py_DECREF(chunks);
    return res;
error:
    Py_XDECREF(chunks);
    return NULL;
}

static PyObject *
join_translated_parts(PyObject *module, PyObject *strings, PyObject *indices)
{
#define WRITE_SUBSTRING(i, j) \
    do { \
        if ((i) < (j)) { \
            if (_PyUnicodeWriter_WriteSubstring(_writer, strings, (i), (j)) < 0) { \
                goto abort; \
            } \
        } \
    } while (0)

    const Py_ssize_t m = PyList_GET_SIZE(indices);
    if (m == 0) {
        // just write fr'(?s:{parts} + ")\Z"
        return PyUnicode_FromFormat("(?s:%S)\\Z", strings);
    }
    /*
     * Special cases: indices[0] == 0 or indices[-1] + 1 == n
     *
     * If indices[0] == 0       write (?>.*?abcdef) instead of abcdef
     * If indices[-1] == n - 1  write '.*' instead of empty string
     */
    PyObject *ind;
    Py_ssize_t i = 0, j, n = PyUnicode_GET_LENGTH(strings);
    /*
     * If the pattern starts with '*', we will write everything
     * before it. So we will write at least indices[0] characters.
     *
     * For the inner groups 'STAR STRING ...' we always surround
     * the STRING by "(?>.*?" and ")", and thus we will write at
     * least 7 + len(STRING) characters.
     *
     * We write one additional '.*' if indices[-1] + 1 = n.
     *
     * Since the result is surrounded by "(?s:" and ")\Z", we
     * write at least "indices[0] + 7m + n + 6" characters,
     * where 'm' is the number of stars and 'n' the length
     * of the translated pattern.
     */
    PyObject *jobj = PyList_GET_ITEM(indices, 0);
    j = PyLong_AsSsize_t(jobj);  // get the first position of '*'
    if (j < 0) {
        return NULL;
    }
    Py_ssize_t estimate = j + 7 * m + n + 6;
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(estimate);
    if (writer == NULL) {
        return NULL;
    }
    _PyUnicodeWriter *_writer = (_PyUnicodeWriter *) (writer);

    WRITE_SUBSTRING(i, j);  // write stuff before '*' if needed
    i = j + 1;              // jump after the star
    for (Py_ssize_t k = 1; k < m; ++k) {
        ind = PyList_GET_ITEM(indices, k);
        j = PyLong_AsSsize_t(ind);
        assert(j < 0 || i > j);
        if (j < 0 ||
            (_PyUnicodeWriter_WriteASCIIString(_writer, "(?>.*?", 6) < 0) ||
            (_PyUnicodeWriter_WriteSubstring(_writer, strings, i, j) < 0) ||
            (_PyUnicodeWriter_WriteChar(_writer, ')') < 0)) {
            goto abort;
        }
        i = j + 1;
    }
    // handle the last group
    if (_PyUnicodeWriter_WriteASCIIString(_writer, ".*", 2) < 0) {
        goto abort;
    }
    WRITE_SUBSTRING(i, n); // write TAIL part

#undef WRITE_SUBSTRING

    PyObject *res = PyUnicodeWriter_Finish(writer);
    if (res == NULL) {
        return NULL;
    }
    return PyUnicode_FromFormat("(?s:%S)\\Z", res);
abort:
    PyUnicodeWriter_Discard(writer);
    return NULL;
}

static PyObject *
translate(PyObject *module, PyObject *pattern)
/* new reference */
{
#define READ(ind) PyUnicode_READ(kind, data, (ind))

#define ADVANCE_IF_CHAR(ch, ind, maxind) \
    do { \
        if ((ind) < (maxind) && READ(ind) == (ch)) { \
            ++(ind); \
        } \
    } while (0)

#define _WHILE_READ_CMP(ch, ind, maxind, cmp) \
    do { \
        while ((ind) < (maxind) && READ(ind) cmp (ch)) { \
            ++(ind); \
        } \
    } while (0)

#define ADVANCE_TO_NEXT(ch, from, maxind) _WHILE_READ_CMP(ch, from, maxind, !=)
#define DROP_DUPLICATES(ch, from, maxind) _WHILE_READ_CMP(ch, from, maxind, ==)

    fnmatchmodule_state *state = get_fnmatchmodulestate_state(module);
    PyObject *re = state->re_module;
    const Py_ssize_t n = PyUnicode_GET_LENGTH(pattern);
    // We would write less data if there are successive '*', which should
    // not be the case in general. Otherwise, we write >= n characters
    // since escaping them would always add more characters so we will
    // overestimate a bit the number of characters to write.
    //
    // TODO(picnixz): should we limit the estimation or not?
    PyUnicodeWriter *writer = PyUnicodeWriter_Create((Py_ssize_t) (1.05 * n));
    if (writer == NULL) {
        return NULL;
    }
    _PyUnicodeWriter *_writer = (_PyUnicodeWriter *) (writer);
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
                if (_PyUnicodeWriter_WriteChar(_writer, chr) < 0) {
                    goto abort;
                }
                DROP_DUPLICATES('*', i, n);
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
                if (_PyUnicodeWriter_WriteChar(_writer, '.') < 0) {
                    goto abort;
                }
                ++h; // increase the expected result's length
                break;
            }
            case '[': {
                Py_ssize_t j = i;           // 'i' is already at next char
                ADVANCE_IF_CHAR('!', j, n); // [!
                ADVANCE_IF_CHAR(']', j, n); // [!] or []
                ADVANCE_TO_NEXT(']', j, n); // locate closing ']'
                if (j >= n) {
                    if (_PyUnicodeWriter_WriteASCIIString(_writer, "\\[", 2) < 0) {
                        goto abort;
                    }
                    h += 2; // we just wrote 2 characters
                    break;  // early break for clarity
                }
                else {
                    //              v--- pattern[j] (exclusive)
                    // '[' * ... * ']'
                    //     ^----- pattern[i] (inclusive)
                    PyObject *s1 = NULL, *s2 = NULL;
                    int rc = PyUnicode_FindChar(pattern, '-', i, j, 1);
                    if (rc == -2) {
                        goto abort;
                    }
                    if (rc == -1) {
                        PyObject *group = PyUnicode_Substring(pattern, i, j);
                        if (group == NULL) {
                            goto abort;
                        }
                        s1 = _PyObject_CallMethod(group, &_Py_ID(replace), "ss", "\\", "\\\\");
                        Py_DECREF(group);
                    }
                    else {
                        assert(rc >= 0);
                        s1 = get_translated_group(pattern, i, j);
                    }
                    if (s1 == NULL) {
                        goto abort;
                    }
                    s2 = _PyObject_CallMethod(re, &_Py_ID(sub), "ssO", "([&~|])", "\\\\\\1", s1);
                    Py_DECREF(s1);
                    if (s2 == NULL) {
                        goto abort;
                    }
                    int difflen = write_translated_group(_writer, s2);
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
                int difflen = write_escaped_string(re, _writer, str);
                Py_DECREF(str);
                if (difflen < 0) {
                    goto abort;
                }
                h += difflen;
                break;
            }
        }
    }
#undef DROP_DUPLICATES
#undef ADVANCE_TO_NEXT
#undef _WHILE_READ_CMP
#undef ADVANCE_IF_CHAR
#undef READ
    PyObject *parts = PyUnicodeWriter_Finish(writer);
    if (parts == NULL) {
        Py_DECREF(indices);
        return NULL;
    }
    assert(h == PyUnicode_GET_LENGTH(parts));
    PyObject *res = join_translated_parts(module, parts, indices);
    Py_DECREF(parts);
    Py_DECREF(indices);
    return res;
abort:
    Py_XDECREF(indices);
    PyUnicodeWriter_Discard(writer);
    return NULL;
}

/*[clinic input]
_fnmatch.translate -> object

    pat as pattern: object

[clinic start generated code]*/

static PyObject *
_fnmatch_translate_impl(PyObject *module, PyObject *pattern)
/*[clinic end generated code: output=2d9e3bbcbcc6e90e input=56e39f7beea97810]*/
{
    if (PyBytes_Check(pattern)) {
        PyObject *unicode = PyUnicode_DecodeLatin1(PyBytes_AS_STRING(pattern),
                                                   PyBytes_GET_SIZE(pattern),
                                                   "strict");
        if (unicode == NULL) {
            return NULL;
        }
        // translated regular expression as a str object
        PyObject *str_expr = translate(module, unicode);
        Py_DECREF(unicode);
        if (str_expr == NULL) {
            return NULL;
        }
        PyObject *expr = PyUnicode_AsLatin1String(str_expr);
        Py_DECREF(str_expr);
        return expr;
    }
    else if (PyUnicode_Check(pattern)) {
        return translate(module, pattern);
    }
    else {
        PyErr_SetString(PyExc_TypeError, INVALID_PATTERN_TYPE);
        return NULL;
    }
}

static PyMethodDef fnmatchmodule_methods[] = {
    _FNMATCH_FILTER_METHODDEF
    _FNMATCH_FNMATCHCASE_METHODDEF
    _FNMATCH_TRANSLATE_METHODDEF
    {NULL, NULL}
};

static struct PyModuleDef_Slot fnmatchmodule_slots[] = {
    {Py_mod_exec, fnmatchmodule_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL},
};

static struct PyModuleDef _fnmatchmodule = {
    PyModuleDef_HEAD_INIT,
    "_fnmatch",
    NULL,
    .m_size = sizeof(fnmatchmodule_state),
    .m_methods = fnmatchmodule_methods,
    .m_slots = fnmatchmodule_slots,
    .m_traverse = fnmatchmodule_traverse,
    .m_clear = fnmatchmodule_clear,
    .m_free = fnmatchmodule_free,
};

PyMODINIT_FUNC
PyInit__fnmatch(void)
{
    return PyModuleDef_Init(&_fnmatchmodule);
}
