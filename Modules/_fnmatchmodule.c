/*
 * C accelerator for the 'fnmatch' module (POSIX only).
 *
 * Most functions expect string or bytes instances, and thus the Python
 * implementation should first pre-process path-like objects, possibly
 * applying normalizations depending on the platform if needed.
 */

#include "Python.h"

#include "clinic/_fnmatchmodule.c.h"

#define INVALID_PATTERN_TYPE "pattern must be a string or a bytes object"

// module state functions

typedef struct {
    PyObject *re_module; // 're' module
    PyObject *os_module; // 'os' module

    PyObject *lru_cache; // optional cache for regex patterns, if needed
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
    return 0;
}

static int
fnmatchmodule_traverse(PyObject *m, visitproc visit, void *arg)
{
    fnmatchmodule_state *st = get_fnmatchmodulestate_state(m);
    Py_VISIT(st->os_module);
    Py_VISIT(st->re_module);
    Py_VISIT(st->lru_cache);
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
    fnmatchmodule_state *state = get_fnmatchmodulestate_state(m);

    // imports
    state->os_module = PyImport_ImportModule("os");
    if (state->os_module == NULL) {
        return -1;
    }
    state->re_module = PyImport_ImportModule("re");
    if (state->re_module == NULL) {
        return -1;
    }

    // helpers
    state->lru_cache = _PyImport_GetModuleAttrString("functools", "lru_cache");
    if (state->lru_cache == NULL) {
        return -1;
    }
    // todo: handle LRU cache
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


static inline int /* number of written characters or -1 on error */
write_normal_character(PyObject *re, _PyUnicodeWriter *writer, PyObject *cp)
{
    PyObject *ch = PyObject_CallMethodOneArg(re, &_Py_ID(escape), cp);
    if (ch == NULL) {
        return -1;
    }
    int written = PyUnicode_GetLength(ch);
    int rc = _PyUnicodeWriter_WriteStr(writer, ch);
    Py_DECREF(ch);
    if (rc < 0) {
        return -1;
    }
    assert(written > 0);
    return written;
}

static inline int /* number of written characters or -1 on error */
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
        int extra = 0;
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
                extra = 1;
                break;
            }
            default:
                if (_PyUnicodeWriter_WriteStr(writer, group) < 0) {
                    return -1;
                }
                break;
        }
        WRITE_CHAR(']');
        return 2 + grouplen + extra;
    }
#undef WRITE_CHAR
#undef WRITE_ASCII
}

static PyObject *
get_translated_group(PyObject *unicode,
                     Py_ssize_t i /* unicode[i-1] == '[' (incl.) */,
                     Py_ssize_t j /* unicode[j]   == ']' (excl.) */)
{
    PyObject *chunks = PyList_New(0);
    if (chunks == NULL) {
        return NULL;
    }
    PyObject *chr = PySequence_GetItem(unicode, i);
    if (chr == NULL) {
        goto error;
    }
    Py_ssize_t k = PyUnicode_CompareWithASCIIString(chr, "!") == 0 ? i + 2 : i + 1;
    Py_DECREF(chr);
    Py_ssize_t chunkscount = 0;
    while (k < j) {
        PyObject *eobj = PyObject_CallMethod(unicode, "find", "ii", k, j);
        if (eobj == NULL) {
            goto error;
        }
        Py_ssize_t t = PyLong_AsSsize_t(eobj);
        Py_DECREF(eobj);
        if (t < 0) {
            goto error;
        }
        PyObject *sub = PyUnicode_Substring(unicode, i, t);
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
        PyObject *sub = PyUnicode_Substring(unicode, i, j);
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
        assert(c1 != NULL);
        Py_ssize_t c1len = 0;
        const char *c1buf = PyUnicode_AsUTF8AndSize(c1, &c1len);
        if (c1buf == NULL) {
            goto error;
        }
        assert(c1len > 0);

        PyObject *c2 = PyList_GET_ITEM(chunks, c);
        assert(c2 != NULL);
        Py_ssize_t c2len = 0;
        const char *c2buf = PyUnicode_AsUTF8AndSize(c2, &c2len);
        if (c2buf == NULL) {
            goto error;
        }
        assert(c2len > 0);

        if (c1buf[c1len - 1] > c2buf[0]) {
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
    PyObject *hyphen = PyUnicode_FromString("-");
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
join_translated_parts(PyObject *parts, PyObject *indices)
{
#define LOAD_STAR_INDEX(var, k) \
    do { \
        ind = PyList_GET_ITEM(indices, (k)); \
        var = PyLong_AsSsize_t(ind); \
        if (var < 0) { \
            goto abort; \
        } \
    } while (0)

#define WRITE_SUBSTRING(i, j) \
    do { \
        if ((i) < (j)) { \
            if (_PyUnicodeWriter_WriteSubstring(_writer, parts, (i), (j)) < 0) { \
                goto abort; \
            } \
        } \
    } while (0)

#define WRITE_WILDCARD() \
    do { \
        if (_PyUnicodeWriter_WriteASCIIString(_writer, ".*", 2) < 0) { \
            goto abort; \
        } \
    } while (0)

#define WRITE_ATOMIC_SUBSTRING(i, j) \
    do { \
        if ((_PyUnicodeWriter_WriteASCIIString(_writer, "(?>.*?", 6) < 0) || \
            (_PyUnicodeWriter_WriteSubstring(_writer, parts, (i), (j)) < 0) || \
            (_PyUnicodeWriter_WriteChar(_writer, ')') < 0)) \
        { \
            goto abort; \
        } \
    } while (0)

    const Py_ssize_t m = PyList_GET_SIZE(indices);
    if (m == 0) {
        // just write fr'(?s:{parts} + ")\Z"
        return PyUnicode_FromFormat("(?s:%S)\\Z", parts);
    }

    PyUnicodeWriter *writer = PyUnicodeWriter_Create(0);
    if (writer == NULL) {
        return NULL;
    }
    _PyUnicodeWriter *_writer = (_PyUnicodeWriter *)(writer);

    /*
     * Special cases: indices[0] == 0 or indices[-1] + 1 == n
     *
     * If indices[0] == 0       write (?>.*?group_1) instead of abcdef
     * If indices[-1] == n - 1  write '.*' instead of empty string
     */
    PyObject *ind;
    Py_ssize_t i, j, n = PyUnicode_GetLength(parts);
    // handle the first group
    LOAD_STAR_INDEX(i, 0);
    if (i == 0) {
        if (m == 1) { // pattern = '*TAIL'
            WRITE_WILDCARD();
            WRITE_SUBSTRING(1, n); // write TAIL part
            goto finalize;
        }
        else { // pattern = '*BODY*...'
            LOAD_STAR_INDEX(j, 1);
            WRITE_ATOMIC_SUBSTRING(i + 1, j);
            i = j + 1;
        }
    }
    else {
        if (m == 1) { // pattern = 'HEAD*' or 'HEAD*TAIL'
            WRITE_SUBSTRING(0, i); // write HEAD part
            WRITE_WILDCARD();
            WRITE_SUBSTRING(i + 1, n); // write TAIL part (if any)
            goto finalize;
        }
        else { // pattern = 'HEAD*STRING*...'
            WRITE_SUBSTRING(0, i);  // write HEAD part
            i++;
        }
    }
    // handle the inner groups
    for (Py_ssize_t k = 1; k < m - 1; ++k) {
        LOAD_STAR_INDEX(j, k + 1);
        assert(i < j);
        WRITE_ATOMIC_SUBSTRING(i, j);
        i = j + 1;
    }
    // handle the last group
    WRITE_WILDCARD();
    WRITE_SUBSTRING(i, n); // write TAIL part (
finalize:
    ; // empty statement for allowing a label before a declaration
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
translate(PyObject *module, PyObject *unicode)
/* new reference */
{
    fnmatchmodule_state *state = get_fnmatchmodulestate_state(module);
    PyObject *re = state->re_module;

    Py_ssize_t estimate = 0;
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(estimate);
    if (writer == NULL) {
        return NULL;
    }
    _PyUnicodeWriter *_writer = (_PyUnicodeWriter *) (writer);

    // list containing the indices where '*' has a special meaning
    PyObject *indices = PyList_New(0);
    if (indices == NULL) {
        goto abort;
    }

    Py_ssize_t n = PyUnicode_GetLength(unicode);
    if (n < 0) {
        goto abort;
    }
    Py_ssize_t h = 0, i = 0;
    PyObject *peek = NULL;
    while (i < n) {
        PyObject *chr = PySequence_GetItem(unicode, i);
        if (chr == NULL) {
            goto abort;
        }
        if (PyUnicode_CompareWithASCIIString(chr, "*") == 0) {
            Py_DECREF(chr);
            if (_PyUnicodeWriter_WriteChar(_writer, '*') < 0) {
                goto abort;
            }
            // drop all other '*' that can be found afterwards
            while (++i < n) {
                peek = PySequence_GetItem(unicode, i);
                if (peek == NULL) {
                    goto abort;
                }
                if (PyUnicode_CompareWithASCIIString(peek, "*") != 0) {
                    Py_DECREF(peek);
                    break;
                }
                Py_DECREF(peek);
            }
            PyObject *index = PyLong_FromLong(h++);
            if (index == NULL) {
                goto abort;
            }
            int rc = PyList_Append(indices, index);
            Py_DECREF(index);
            if (rc < 0) {
                goto abort;
            }
        }
        else if (PyUnicode_CompareWithASCIIString(chr, "?") == 0)  {
            Py_DECREF(chr);
            // translate optional '?' (fnmatch) into optional '.' (regex)
            if (_PyUnicodeWriter_WriteChar(_writer, '.') < 0) {
                goto abort;
            }
            ++i; // advance for the next iteration
            ++h; // increase the expected result's length
        }
        else if (PyUnicode_CompareWithASCIIString(chr, "[") == 0)  {
            Py_DECREF(chr);
            // check the next characters (peek)
            Py_ssize_t j = ++i;
            if (j < n) {
                peek = PySequence_GetItem(unicode, j);
                if (peek == NULL) {
                    goto abort;
                }
                if (PyUnicode_CompareWithASCIIString(peek, "!") == 0) {// [!
                    ++j;
                }
                Py_DECREF(peek);
            }
            if (j < n) {
                peek = PySequence_GetItem(unicode, j);
                if (peek == NULL) {
                    goto abort;
                }
                if (PyUnicode_CompareWithASCIIString(peek, "]") == 0) { // [!] or []
                    ++j;
                }
                Py_DECREF(peek);
            }
            while (j < n) {
                peek = PySequence_GetItem(unicode, j);
                if (peek == NULL) {
                    goto abort;
                }
                // locate the closing ']'
                if (PyUnicode_CompareWithASCIIString(peek, "]") != 0) {
                    ++j;
                }
                Py_DECREF(peek);
            }
            if (j >= n) {
                if (_PyUnicodeWriter_WriteASCIIString(_writer, "\\[", 2) < 0) {
                    goto abort;
                }
                h += 2; // we just wrote 2 characters
            }
            else {
                //              v--- pattern[j] (exclusive)
                // '[' * ... * ']'
                //     ^----- pattern[i] (inclusive)
                PyObject *s1 = NULL, *s2 = NULL;
                if (PyUnicode_FindChar(unicode, '-', i, j, 1) >= 0) {
                    PyObject *group = PyUnicode_Substring(unicode, i, j);
                    if (group == NULL) {
                        goto abort;
                    }
                    s1 = PyObject_CallMethod(group, "replace", "ss", "\\", "\\\\");
                    Py_DECREF(group);
                }
                else {
                    s1 = get_translated_group(unicode, i, j);
                }
                if (s1 == NULL) {
                    goto abort;
                }
                s2 = PyObject_CallMethod(re, "sub", "ssO", "([&~|])", "\\\\\\1", s1);
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
                i = j + 1; // jump to the character after ']'
            }
        }
        else {
            int difflen = write_normal_character(re, _writer, chr);
            Py_DECREF(chr);
            if (difflen < 0) {
                goto abort;
            }
            h += difflen;
            ++i;
        }
    }
    PyObject *parts = PyUnicodeWriter_Finish(writer);
    if (parts == NULL) {
        Py_DECREF(indices);
        return NULL;
    }
    assert(h == PyUnicode_GET_LENGTH(parts));
    PyObject *res = join_translated_parts(parts, indices);
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
