/* stringlib: split implementation */

#ifndef STRINGLIB_FASTSEARCH_H
#error must include "stringlib/fastsearch.h" before including this module
#endif

/* Overallocate the initial list to reduce the number of reallocs for small
   split sizes.  Eg, "A A A A A A A A A A".split() (10 elements) has three
   resizes, to sizes 4, 8, then 16.  Most observed string splits are for human
   text (roughly 11 words per line) and field delimited data (usually 1-10
   fields).  For large strings the split algorithms are bandwidth limited
   so increasing the preallocation likely will not improve things.*/

#define MAX_PREALLOC 12

/* 5 splits gives 6 elements */
#define PREALLOC_SIZE(maxsplit) \
    (maxsplit >= MAX_PREALLOC ? MAX_PREALLOC : maxsplit+1)

#define SPLIT_APPEND(data, left, right)         \
    sub = STRINGLIB_NEW((data) + (left),        \
                        (right) - (left));      \
    if (sub == NULL)                            \
        goto onError;                           \
    if (PyList_Append(list, sub)) {             \
        Py_DECREF(sub);                         \
        goto onError;                           \
    }                                           \
    else                                        \
        Py_DECREF(sub);

#define SPLIT_ADD(data, left, right) {          \
    sub = STRINGLIB_NEW((data) + (left),        \
                        (right) - (left));      \
    if (sub == NULL)                            \
        goto onError;                           \
    if (count < MAX_PREALLOC) {                 \
        PyList_SET_ITEM(list, count, sub);      \
    } else {                                    \
        if (PyList_Append(list, sub)) {         \
            Py_DECREF(sub);                     \
            goto onError;                       \
        }                                       \
        else                                    \
            Py_DECREF(sub);                     \
    }                                           \
    count++; }


/* Always force the list to the expected size. */
#define FIX_PREALLOC_SIZE(list) Py_SIZE(list) = count

Py_LOCAL_INLINE(PyObject *)
STRINGLIB(split_whitespace)(PyObject* str_obj,
                           const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                           Py_ssize_t maxcount)
{
    Py_ssize_t i, j, count=0;
    PyObject *list = PyList_New(PREALLOC_SIZE(maxcount));
    PyObject *sub;

    if (list == NULL)
        return NULL;

    i = j = 0;
    while (maxcount-- > 0) {
        while (i < str_len && STRINGLIB_ISSPACE(str[i]))
            i++;
        if (i == str_len) break;
        j = i; i++;
        while (i < str_len && !STRINGLIB_ISSPACE(str[i]))
            i++;
#ifndef STRINGLIB_MUTABLE
        if (j == 0 && i == str_len && STRINGLIB_CHECK_EXACT(str_obj)) {
            /* No whitespace in str_obj, so just use it as list[0] */
            Py_INCREF(str_obj);
            PyList_SET_ITEM(list, 0, (PyObject *)str_obj);
            count++;
            break;
        }
#endif
        SPLIT_ADD(str, j, i);
    }

    if (i < str_len) {
        /* Only occurs when maxcount was reached */
        /* Skip any remaining whitespace and copy to end of string */
        while (i < str_len && STRINGLIB_ISSPACE(str[i]))
            i++;
        if (i != str_len)
            SPLIT_ADD(str, i, str_len);
    }
    FIX_PREALLOC_SIZE(list);
    return list;

  onError:
    Py_DECREF(list);
    return NULL;
}

Py_LOCAL_INLINE(PyObject *)
STRINGLIB(split_char)(PyObject* str_obj,
                     const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                     const STRINGLIB_CHAR ch,
                     Py_ssize_t maxcount)
{
    Py_ssize_t i, j, count=0;
    PyObject *list = PyList_New(PREALLOC_SIZE(maxcount));
    PyObject *sub;

    if (list == NULL)
        return NULL;

    i = j = 0;
    while ((j < str_len) && (maxcount-- > 0)) {
        for(; j < str_len; j++) {
            /* I found that using memchr makes no difference */
            if (str[j] == ch) {
                SPLIT_ADD(str, i, j);
                i = j = j + 1;
                break;
            }
        }
    }
#ifndef STRINGLIB_MUTABLE
    if (count == 0 && STRINGLIB_CHECK_EXACT(str_obj)) {
        /* ch not in str_obj, so just use str_obj as list[0] */
        Py_INCREF(str_obj);
        PyList_SET_ITEM(list, 0, (PyObject *)str_obj);
        count++;
    } else
#endif
    if (i <= str_len) {
        SPLIT_ADD(str, i, str_len);
    }
    FIX_PREALLOC_SIZE(list);
    return list;

  onError:
    Py_DECREF(list);
    return NULL;
}

Py_LOCAL_INLINE(PyObject *)
STRINGLIB(split)(PyObject* str_obj,
                const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                const STRINGLIB_CHAR* sep, Py_ssize_t sep_len,
                Py_ssize_t maxcount)
{
    Py_ssize_t i, j, pos, count=0;
    PyObject *list, *sub;

    if (sep_len == 0) {
        PyErr_SetString(PyExc_ValueError, "empty separator");
        return NULL;
    }
    else if (sep_len == 1)
        return STRINGLIB(split_char)(str_obj, str, str_len, sep[0], maxcount);

    list = PyList_New(PREALLOC_SIZE(maxcount));
    if (list == NULL)
        return NULL;

    i = j = 0;
    while (maxcount-- > 0) {
        pos = FASTSEARCH(str+i, str_len-i, sep, sep_len, -1, FAST_SEARCH);
        if (pos < 0)
            break;
        j = i + pos;
        SPLIT_ADD(str, i, j);
        i = j + sep_len;
    }
#ifndef STRINGLIB_MUTABLE
    if (count == 0 && STRINGLIB_CHECK_EXACT(str_obj)) {
        /* No match in str_obj, so just use it as list[0] */
        Py_INCREF(str_obj);
        PyList_SET_ITEM(list, 0, (PyObject *)str_obj);
        count++;
    } else
#endif
    {
        SPLIT_ADD(str, i, str_len);
    }
    FIX_PREALLOC_SIZE(list);
    return list;

  onError:
    Py_DECREF(list);
    return NULL;
}

Py_LOCAL_INLINE(PyObject *)
STRINGLIB(rsplit_whitespace)(PyObject* str_obj,
                            const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                            Py_ssize_t maxcount)
{
    Py_ssize_t i, j, count=0;
    PyObject *list = PyList_New(PREALLOC_SIZE(maxcount));
    PyObject *sub;

    if (list == NULL)
        return NULL;

    i = j = str_len - 1;
    while (maxcount-- > 0) {
        while (i >= 0 && STRINGLIB_ISSPACE(str[i]))
            i--;
        if (i < 0) break;
        j = i; i--;
        while (i >= 0 && !STRINGLIB_ISSPACE(str[i]))
            i--;
#ifndef STRINGLIB_MUTABLE
        if (j == str_len - 1 && i < 0 && STRINGLIB_CHECK_EXACT(str_obj)) {
            /* No whitespace in str_obj, so just use it as list[0] */
            Py_INCREF(str_obj);
            PyList_SET_ITEM(list, 0, (PyObject *)str_obj);
            count++;
            break;
        }
#endif
        SPLIT_ADD(str, i + 1, j + 1);
    }

    if (i >= 0) {
        /* Only occurs when maxcount was reached */
        /* Skip any remaining whitespace and copy to beginning of string */
        while (i >= 0 && STRINGLIB_ISSPACE(str[i]))
            i--;
        if (i >= 0)
            SPLIT_ADD(str, 0, i + 1);
    }
    FIX_PREALLOC_SIZE(list);
    if (PyList_Reverse(list) < 0)
        goto onError;
    return list;

  onError:
    Py_DECREF(list);
    return NULL;
}

Py_LOCAL_INLINE(PyObject *)
STRINGLIB(rsplit_char)(PyObject* str_obj,
                      const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                      const STRINGLIB_CHAR ch,
                      Py_ssize_t maxcount)
{
    Py_ssize_t i, j, count=0;
    PyObject *list = PyList_New(PREALLOC_SIZE(maxcount));
    PyObject *sub;

    if (list == NULL)
        return NULL;

    i = j = str_len - 1;
    while ((i >= 0) && (maxcount-- > 0)) {
        for(; i >= 0; i--) {
            if (str[i] == ch) {
                SPLIT_ADD(str, i + 1, j + 1);
                j = i = i - 1;
                break;
            }
        }
    }
#ifndef STRINGLIB_MUTABLE
    if (count == 0 && STRINGLIB_CHECK_EXACT(str_obj)) {
        /* ch not in str_obj, so just use str_obj as list[0] */
        Py_INCREF(str_obj);
        PyList_SET_ITEM(list, 0, (PyObject *)str_obj);
        count++;
    } else
#endif
    if (j >= -1) {
        SPLIT_ADD(str, 0, j + 1);
    }
    FIX_PREALLOC_SIZE(list);
    if (PyList_Reverse(list) < 0)
        goto onError;
    return list;

  onError:
    Py_DECREF(list);
    return NULL;
}

Py_LOCAL_INLINE(PyObject *)
STRINGLIB(rsplit)(PyObject* str_obj,
                 const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                 const STRINGLIB_CHAR* sep, Py_ssize_t sep_len,
                 Py_ssize_t maxcount)
{
    Py_ssize_t j, pos, count=0;
    PyObject *list, *sub;

    if (sep_len == 0) {
        PyErr_SetString(PyExc_ValueError, "empty separator");
        return NULL;
    }
    else if (sep_len == 1)
        return STRINGLIB(rsplit_char)(str_obj, str, str_len, sep[0], maxcount);

    list = PyList_New(PREALLOC_SIZE(maxcount));
    if (list == NULL)
        return NULL;

    j = str_len;
    while (maxcount-- > 0) {
        pos = FASTSEARCH(str, j, sep, sep_len, -1, FAST_RSEARCH);
        if (pos < 0)
            break;
        SPLIT_ADD(str, pos + sep_len, j);
        j = pos;
    }
#ifndef STRINGLIB_MUTABLE
    if (count == 0 && STRINGLIB_CHECK_EXACT(str_obj)) {
        /* No match in str_obj, so just use it as list[0] */
        Py_INCREF(str_obj);
        PyList_SET_ITEM(list, 0, (PyObject *)str_obj);
        count++;
    } else
#endif
    {
        SPLIT_ADD(str, 0, j);
    }
    FIX_PREALLOC_SIZE(list);
    if (PyList_Reverse(list) < 0)
        goto onError;
    return list;

  onError:
    Py_DECREF(list);
    return NULL;
}

Py_LOCAL_INLINE(PyObject *)
STRINGLIB(splitlines)(PyObject* str_obj,
                     const STRINGLIB_CHAR* str, Py_ssize_t str_len,
                     int keepends)
{
    /* This does not use the preallocated list because splitlines is
       usually run with hundreds of newlines.  The overhead of
       switching between PyList_SET_ITEM and append causes about a
       2-3% slowdown for that common case.  A smarter implementation
       could move the if check out, so the SET_ITEMs are done first
       and the appends only done when the prealloc buffer is full.
       That's too much work for little gain.*/

    Py_ssize_t i;
    Py_ssize_t j;
    PyObject *list = PyList_New(0);
    PyObject *sub;

    if (list == NULL)
        return NULL;

    for (i = j = 0; i < str_len; ) {
        Py_ssize_t eol;

        /* Find a line and append it */
        while (i < str_len && !STRINGLIB_ISLINEBREAK(str[i]))
            i++;

        /* Skip the line break reading CRLF as one line break */
        eol = i;
        if (i < str_len) {
            if (str[i] == '\r' && i + 1 < str_len && str[i+1] == '\n')
                i += 2;
            else
                i++;
            if (keepends)
                eol = i;
        }
#ifndef STRINGLIB_MUTABLE
        if (j == 0 && eol == str_len && STRINGLIB_CHECK_EXACT(str_obj)) {
            /* No linebreak in str_obj, so just use it as list[0] */
            if (PyList_Append(list, str_obj))
                goto onError;
            break;
        }
#endif
        SPLIT_APPEND(str, j, eol);
        j = i;
    }
    return list;

  onError:
    Py_DECREF(list);
    return NULL;
}

