#if STRINGLIB_IS_UNICODE
# error "transmogrify.h only compatible with byte-wise strings"
#endif

/* the more complicated methods.  parts of these should be pulled out into the
   shared code in bytes_methods.c to cut down on duplicate code bloat.  */

/*[clinic input]
class B "PyObject *" "&PyType_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=2935558188d97c76]*/

#include "clinic/transmogrify.h.h"

static inline PyObject *
return_self(PyObject *self)
{
#if !STRINGLIB_MUTABLE
    if (STRINGLIB_CHECK_EXACT(self)) {
        Py_INCREF(self);
        return self;
    }
#endif
    return STRINGLIB_NEW(STRINGLIB_STR(self), STRINGLIB_LEN(self));
}

/*[clinic input]
B.expandtabs as stringlib_expandtabs

    tabsize: int = 8

Return a copy where all tab characters are expanded using spaces.

If tabsize is not given, a tab size of 8 characters is assumed.
[clinic start generated code]*/

static PyObject *
stringlib_expandtabs_impl(PyObject *self, int tabsize)
/*[clinic end generated code: output=069cb7fae72e4c2b input=3c6d3b12aa3ccbea]*/
{
    const char *e, *p;
    char *q;
    Py_ssize_t i, j;
    PyObject *u;

    /* First pass: determine size of output string */
    i = j = 0;
    e = STRINGLIB_STR(self) + STRINGLIB_LEN(self);
    for (p = STRINGLIB_STR(self); p < e; p++) {
        if (*p == '\t') {
            if (tabsize > 0) {
                Py_ssize_t incr = tabsize - (j % tabsize);
                if (j > PY_SSIZE_T_MAX - incr)
                    goto overflow;
                j += incr;
            }
        }
        else {
            if (j > PY_SSIZE_T_MAX - 1)
                goto overflow;
            j++;
            if (*p == '\n' || *p == '\r') {
                if (i > PY_SSIZE_T_MAX - j)
                    goto overflow;
                i += j;
                j = 0;
            }
        }
    }

    if (i > PY_SSIZE_T_MAX - j)
        goto overflow;

    /* Second pass: create output string and fill it */
    u = STRINGLIB_NEW(NULL, i + j);
    if (!u)
        return NULL;

    j = 0;
    q = STRINGLIB_STR(u);

    for (p = STRINGLIB_STR(self); p < e; p++) {
        if (*p == '\t') {
            if (tabsize > 0) {
                i = tabsize - (j % tabsize);
                j += i;
                while (i--)
                    *q++ = ' ';
            }
        }
        else {
            j++;
            *q++ = *p;
            if (*p == '\n' || *p == '\r')
                j = 0;
        }
    }

    return u;
  overflow:
    PyErr_SetString(PyExc_OverflowError, "result too long");
    return NULL;
}

static inline PyObject *
pad(PyObject *self, Py_ssize_t left, Py_ssize_t right, char fill)
{
    PyObject *u;

    if (left < 0)
        left = 0;
    if (right < 0)
        right = 0;

    if (left == 0 && right == 0) {
        return return_self(self);
    }

    u = STRINGLIB_NEW(NULL, left + STRINGLIB_LEN(self) + right);
    if (u) {
        if (left)
            memset(STRINGLIB_STR(u), fill, left);
        memcpy(STRINGLIB_STR(u) + left,
               STRINGLIB_STR(self),
               STRINGLIB_LEN(self));
        if (right)
            memset(STRINGLIB_STR(u) + left + STRINGLIB_LEN(self),
                   fill, right);
    }

    return u;
}

/*[clinic input]
B.ljust as stringlib_ljust

    width: Py_ssize_t
    fillchar: char = b' '
    /

Return a left-justified string of length width.

Padding is done using the specified fill character.
[clinic start generated code]*/

static PyObject *
stringlib_ljust_impl(PyObject *self, Py_ssize_t width, char fillchar)
/*[clinic end generated code: output=c79ca173c5ff8337 input=eff2d014bc7d80df]*/
{
    if (STRINGLIB_LEN(self) >= width) {
        return return_self(self);
    }

    return pad(self, 0, width - STRINGLIB_LEN(self), fillchar);
}


/*[clinic input]
B.rjust as stringlib_rjust

    width: Py_ssize_t
    fillchar: char = b' '
    /

Return a right-justified string of length width.

Padding is done using the specified fill character.
[clinic start generated code]*/

static PyObject *
stringlib_rjust_impl(PyObject *self, Py_ssize_t width, char fillchar)
/*[clinic end generated code: output=7df5d728a5439570 input=218b0bd31308955d]*/
{
    if (STRINGLIB_LEN(self) >= width) {
        return return_self(self);
    }

    return pad(self, width - STRINGLIB_LEN(self), 0, fillchar);
}


/*[clinic input]
B.center as stringlib_center

    width: Py_ssize_t
    fillchar: char = b' '
    /

Return a centered string of length width.

Padding is done using the specified fill character.
[clinic start generated code]*/

static PyObject *
stringlib_center_impl(PyObject *self, Py_ssize_t width, char fillchar)
/*[clinic end generated code: output=d8da2e055288b4c2 input=3776fd278765d89b]*/
{
    Py_ssize_t marg, left;

    if (STRINGLIB_LEN(self) >= width) {
        return return_self(self);
    }

    marg = width - STRINGLIB_LEN(self);
    left = marg / 2 + (marg & width & 1);

    return pad(self, left, marg - left, fillchar);
}

/*[clinic input]
B.zfill as stringlib_zfill

    width: Py_ssize_t
    /

Pad a numeric string with zeros on the left, to fill a field of the given width.

The original string is never truncated.
[clinic start generated code]*/

static PyObject *
stringlib_zfill_impl(PyObject *self, Py_ssize_t width)
/*[clinic end generated code: output=0b3c684a7f1b2319 input=2da6d7b8e9bcb19a]*/
{
    Py_ssize_t fill;
    PyObject *s;
    char *p;

    if (STRINGLIB_LEN(self) >= width) {
        return return_self(self);
    }

    fill = width - STRINGLIB_LEN(self);

    s = pad(self, fill, 0, '0');

    if (s == NULL)
        return NULL;

    p = STRINGLIB_STR(s);
    if (p[fill] == '+' || p[fill] == '-') {
        /* move sign to beginning of string */
        p[0] = p[fill];
        p[fill] = '0';
    }

    return s;
}


/* find and count characters and substrings */

#define findchar(target, target_len, c)                         \
  ((char *)memchr((const void *)(target), c, target_len))


static Py_ssize_t
countchar(const char *target, Py_ssize_t target_len, char c,
          Py_ssize_t maxcount)
{
    Py_ssize_t count = 0;
    const char *start = target;
    const char *end = target + target_len;

    while ((start = findchar(start, end - start, c)) != NULL) {
        count++;
        if (count >= maxcount)
            break;
        start += 1;
    }
    return count;
}


/* Algorithms for different cases of string replacement */

/* len(self)>=1, from="", len(to)>=1, maxcount>=1 */
static PyObject *
stringlib_replace_interleave(PyObject *self,
                             const char *to_s, Py_ssize_t to_len,
                             Py_ssize_t maxcount)
{
    const char *self_s;
    char *result_s;
    Py_ssize_t self_len, result_len;
    Py_ssize_t count, i;
    PyObject *result;

    self_len = STRINGLIB_LEN(self);

    /* 1 at the end plus 1 after every character;
       count = min(maxcount, self_len + 1) */
    if (maxcount <= self_len) {
        count = maxcount;
    }
    else {
        /* Can't overflow: self_len + 1 <= maxcount <= PY_SSIZE_T_MAX. */
        count = self_len + 1;
    }

    /* Check for overflow */
    /*   result_len = count * to_len + self_len; */
    assert(count > 0);
    if (to_len > (PY_SSIZE_T_MAX - self_len) / count) {
        PyErr_SetString(PyExc_OverflowError,
                        "replace bytes is too long");
        return NULL;
    }
    result_len = count * to_len + self_len;
    result = STRINGLIB_NEW(NULL, result_len);
    if (result == NULL) {
        return NULL;
    }

    self_s = STRINGLIB_STR(self);
    result_s = STRINGLIB_STR(result);

    if (to_len > 1) {
        /* Lay the first one down (guaranteed this will occur) */
        memcpy(result_s, to_s, to_len);
        result_s += to_len;
        count -= 1;

        for (i = 0; i < count; i++) {
            *result_s++ = *self_s++;
            memcpy(result_s, to_s, to_len);
            result_s += to_len;
        }
    }
    else {
        result_s[0] = to_s[0];
        result_s += to_len;
        count -= 1;
        for (i = 0; i < count; i++) {
            *result_s++ = *self_s++;
            result_s[0] = to_s[0];
            result_s += to_len;
        }
    }

    /* Copy the rest of the original string */
    memcpy(result_s, self_s, self_len - i);

    return result;
}

/* Special case for deleting a single character */
/* len(self)>=1, len(from)==1, to="", maxcount>=1 */
static PyObject *
stringlib_replace_delete_single_character(PyObject *self,
                                          char from_c, Py_ssize_t maxcount)
{
    const char *self_s, *start, *next, *end;
    char *result_s;
    Py_ssize_t self_len, result_len;
    Py_ssize_t count;
    PyObject *result;

    self_len = STRINGLIB_LEN(self);
    self_s = STRINGLIB_STR(self);

    count = countchar(self_s, self_len, from_c, maxcount);
    if (count == 0) {
        return return_self(self);
    }

    result_len = self_len - count;  /* from_len == 1 */
    assert(result_len>=0);

    result = STRINGLIB_NEW(NULL, result_len);
    if (result == NULL) {
        return NULL;
    }
    result_s = STRINGLIB_STR(result);

    start = self_s;
    end = self_s + self_len;
    while (count-- > 0) {
        next = findchar(start, end - start, from_c);
        if (next == NULL)
            break;
        memcpy(result_s, start, next - start);
        result_s += (next - start);
        start = next + 1;
    }
    memcpy(result_s, start, end - start);

    return result;
}

/* len(self)>=1, len(from)>=2, to="", maxcount>=1 */

static PyObject *
stringlib_replace_delete_substring(PyObject *self,
                                   const char *from_s, Py_ssize_t from_len,
                                   Py_ssize_t maxcount)
{
    const char *self_s, *start, *next, *end;
    char *result_s;
    Py_ssize_t self_len, result_len;
    Py_ssize_t count, offset;
    PyObject *result;

    self_len = STRINGLIB_LEN(self);
    self_s = STRINGLIB_STR(self);

    count = stringlib_count(self_s, self_len,
                            from_s, from_len,
                            maxcount);

    if (count == 0) {
        /* no matches */
        return return_self(self);
    }

    result_len = self_len - (count * from_len);
    assert (result_len>=0);

    result = STRINGLIB_NEW(NULL, result_len);
    if (result == NULL) {
        return NULL;
    }
    result_s = STRINGLIB_STR(result);

    start = self_s;
    end = self_s + self_len;
    while (count-- > 0) {
        offset = stringlib_find(start, end - start,
                                from_s, from_len,
                                0);
        if (offset == -1)
            break;
        next = start + offset;

        memcpy(result_s, start, next - start);

        result_s += (next - start);
        start = next + from_len;
    }
    memcpy(result_s, start, end - start);
    return result;
}

/* len(self)>=1, len(from)==len(to)==1, maxcount>=1 */
static PyObject *
stringlib_replace_single_character_in_place(PyObject *self,
                                            char from_c, char to_c,
                                            Py_ssize_t maxcount)
{
    const char *self_s, *end;
    char *result_s, *start, *next;
    Py_ssize_t self_len;
    PyObject *result;

    /* The result string will be the same size */
    self_s = STRINGLIB_STR(self);
    self_len = STRINGLIB_LEN(self);

    next = findchar(self_s, self_len, from_c);

    if (next == NULL) {
        /* No matches; return the original bytes */
        return return_self(self);
    }

    /* Need to make a new bytes */
    result = STRINGLIB_NEW(NULL, self_len);
    if (result == NULL) {
        return NULL;
    }
    result_s = STRINGLIB_STR(result);
    memcpy(result_s, self_s, self_len);

    /* change everything in-place, starting with this one */
    start =  result_s + (next - self_s);
    *start = to_c;
    start++;
    end = result_s + self_len;

    while (--maxcount > 0) {
        next = findchar(start, end - start, from_c);
        if (next == NULL)
            break;
        *next = to_c;
        start = next + 1;
    }

    return result;
}

/* len(self)>=1, len(from)==len(to)>=2, maxcount>=1 */
static PyObject *
stringlib_replace_substring_in_place(PyObject *self,
                                     const char *from_s, Py_ssize_t from_len,
                                     const char *to_s, Py_ssize_t to_len,
                                     Py_ssize_t maxcount)
{
    const char *self_s, *end;
    char *result_s, *start;
    Py_ssize_t self_len, offset;
    PyObject *result;

    /* The result bytes will be the same size */

    self_s = STRINGLIB_STR(self);
    self_len = STRINGLIB_LEN(self);

    offset = stringlib_find(self_s, self_len,
                            from_s, from_len,
                            0);
    if (offset == -1) {
        /* No matches; return the original bytes */
        return return_self(self);
    }

    /* Need to make a new bytes */
    result = STRINGLIB_NEW(NULL, self_len);
    if (result == NULL) {
        return NULL;
    }
    result_s = STRINGLIB_STR(result);
    memcpy(result_s, self_s, self_len);

    /* change everything in-place, starting with this one */
    start =  result_s + offset;
    memcpy(start, to_s, from_len);
    start += from_len;
    end = result_s + self_len;

    while ( --maxcount > 0) {
        offset = stringlib_find(start, end - start,
                                from_s, from_len,
                                0);
        if (offset == -1)
            break;
        memcpy(start + offset, to_s, from_len);
        start += offset + from_len;
    }

    return result;
}

/* len(self)>=1, len(from)==1, len(to)>=2, maxcount>=1 */
static PyObject *
stringlib_replace_single_character(PyObject *self,
                                   char from_c,
                                   const char *to_s, Py_ssize_t to_len,
                                   Py_ssize_t maxcount)
{
    const char *self_s, *start, *next, *end;
    char *result_s;
    Py_ssize_t self_len, result_len;
    Py_ssize_t count;
    PyObject *result;

    self_s = STRINGLIB_STR(self);
    self_len = STRINGLIB_LEN(self);

    count = countchar(self_s, self_len, from_c, maxcount);
    if (count == 0) {
        /* no matches, return unchanged */
        return return_self(self);
    }

    /* use the difference between current and new, hence the "-1" */
    /*   result_len = self_len + count * (to_len-1)  */
    assert(count > 0);
    if (to_len - 1 > (PY_SSIZE_T_MAX - self_len) / count) {
        PyErr_SetString(PyExc_OverflowError, "replace bytes is too long");
        return NULL;
    }
    result_len = self_len + count * (to_len - 1);

    result = STRINGLIB_NEW(NULL, result_len);
    if (result == NULL) {
        return NULL;
    }
    result_s = STRINGLIB_STR(result);

    start = self_s;
    end = self_s + self_len;
    while (count-- > 0) {
        next = findchar(start, end - start, from_c);
        if (next == NULL)
            break;

        if (next == start) {
            /* replace with the 'to' */
            memcpy(result_s, to_s, to_len);
            result_s += to_len;
            start += 1;
        } else {
            /* copy the unchanged old then the 'to' */
            memcpy(result_s, start, next - start);
            result_s += (next - start);
            memcpy(result_s, to_s, to_len);
            result_s += to_len;
            start = next + 1;
        }
    }
    /* Copy the remainder of the remaining bytes */
    memcpy(result_s, start, end - start);

    return result;
}

/* len(self)>=1, len(from)>=2, len(to)>=2, maxcount>=1 */
static PyObject *
stringlib_replace_substring(PyObject *self,
                            const char *from_s, Py_ssize_t from_len,
                            const char *to_s, Py_ssize_t to_len,
                            Py_ssize_t maxcount)
{
    const char *self_s, *start, *next, *end;
    char *result_s;
    Py_ssize_t self_len, result_len;
    Py_ssize_t count, offset;
    PyObject *result;

    self_s = STRINGLIB_STR(self);
    self_len = STRINGLIB_LEN(self);

    count = stringlib_count(self_s, self_len,
                            from_s, from_len,
                            maxcount);

    if (count == 0) {
        /* no matches, return unchanged */
        return return_self(self);
    }

    /* Check for overflow */
    /*    result_len = self_len + count * (to_len-from_len) */
    assert(count > 0);
    if (to_len - from_len > (PY_SSIZE_T_MAX - self_len) / count) {
        PyErr_SetString(PyExc_OverflowError, "replace bytes is too long");
        return NULL;
    }
    result_len = self_len + count * (to_len - from_len);

    result = STRINGLIB_NEW(NULL, result_len);
    if (result == NULL) {
        return NULL;
    }
    result_s = STRINGLIB_STR(result);

    start = self_s;
    end = self_s + self_len;
    while (count-- > 0) {
        offset = stringlib_find(start, end - start,
                                from_s, from_len,
                                0);
        if (offset == -1)
            break;
        next = start + offset;
        if (next == start) {
            /* replace with the 'to' */
            memcpy(result_s, to_s, to_len);
            result_s += to_len;
            start += from_len;
        } else {
            /* copy the unchanged old then the 'to' */
            memcpy(result_s, start, next - start);
            result_s += (next - start);
            memcpy(result_s, to_s, to_len);
            result_s += to_len;
            start = next + from_len;
        }
    }
    /* Copy the remainder of the remaining bytes */
    memcpy(result_s, start, end - start);

    return result;
}


static PyObject *
stringlib_replace(PyObject *self,
                  const char *from_s, Py_ssize_t from_len,
                  const char *to_s, Py_ssize_t to_len,
                  Py_ssize_t maxcount)
{
    if (STRINGLIB_LEN(self) < from_len) {
        /* nothing to do; return the original bytes */
        return return_self(self);
    }
    if (maxcount < 0) {
        maxcount = PY_SSIZE_T_MAX;
    } else if (maxcount == 0) {
        /* nothing to do; return the original bytes */
        return return_self(self);
    }

    /* Handle zero-length special cases */
    if (from_len == 0) {
        if (to_len == 0) {
            /* nothing to do; return the original bytes */
            return return_self(self);
        }
        /* insert the 'to' bytes everywhere.    */
        /*    >>> b"Python".replace(b"", b".")  */
        /*    b'.P.y.t.h.o.n.'                  */
        return stringlib_replace_interleave(self, to_s, to_len, maxcount);
    }

    if (to_len == 0) {
        /* delete all occurrences of 'from' bytes */
        if (from_len == 1) {
            return stringlib_replace_delete_single_character(
                self, from_s[0], maxcount);
        } else {
            return stringlib_replace_delete_substring(
                self, from_s, from_len, maxcount);
        }
    }

    /* Handle special case where both bytes have the same length */

    if (from_len == to_len) {
        if (from_len == 1) {
            return stringlib_replace_single_character_in_place(
                self, from_s[0], to_s[0], maxcount);
        } else {
            return stringlib_replace_substring_in_place(
                self, from_s, from_len, to_s, to_len, maxcount);
        }
    }

    /* Otherwise use the more generic algorithms */
    if (from_len == 1) {
        return stringlib_replace_single_character(
            self, from_s[0], to_s, to_len, maxcount);
    } else {
        /* len('from')>=2, len('to')>=1 */
        return stringlib_replace_substring(
            self, from_s, from_len, to_s, to_len, maxcount);
    }
}

#undef findchar
