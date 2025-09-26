#include "Python.h"
#include "pycore_critical_section.h" // Py_*_CRITICAL_SECTION_SEQUENCE_FAST
#include "pycore_unicodeobject.h" // _Py_ADJUST_INDICES

#include "stringlib/eq.h"      // unicode_eq()


#define _PyUnicode_LENGTH(op)                           \
    (_PyASCIIObject_CAST(op)->length)
#define PyUnicode_HASH PyUnstable_Unicode_GET_CACHED_HASH

#define ADJUST_INDICES _Py_ADJUST_INDICES
#define ensure_unicode _PyUnicode_Ensure
#define unicode_askind _PyUnicode_AsKind
#define unicode_get_empty _PyUnicode_GetEmpty


/* Fast detection of the most frequent whitespace characters */
const unsigned char _Py_ascii_whitespace[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
/*     case 0x0009: * CHARACTER TABULATION */
/*     case 0x000A: * LINE FEED */
/*     case 0x000B: * LINE TABULATION */
/*     case 0x000C: * FORM FEED */
/*     case 0x000D: * CARRIAGE RETURN */
    0, 1, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
/*     case 0x001C: * FILE SEPARATOR */
/*     case 0x001D: * GROUP SEPARATOR */
/*     case 0x001E: * RECORD SEPARATOR */
/*     case 0x001F: * UNIT SEPARATOR */
    0, 0, 0, 0, 1, 1, 1, 1,
/*     case 0x0020: * SPACE */
    1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

/* Fast detection of the most frequent linebreak characters */
static const unsigned char ascii_linebreak[] = {
    0, 0, 0, 0, 0, 0, 0, 0,
/*         0x000A, * LINE FEED */
/*         0x000B, * LINE TABULATION */
/*         0x000C, * FORM FEED */
/*         0x000D, * CARRIAGE RETURN */
    0, 0, 1, 1, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
/*         0x001C, * FILE SEPARATOR */
/*         0x001D, * GROUP SEPARATOR */
/*         0x001E, * RECORD SEPARATOR */
    0, 0, 0, 0, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};


/* --- Bloom Filters ----------------------------------------------------- */

/* stuff to implement simple "bloom filters" for Unicode characters.
   to keep things simple, we use a single bitmask, using the least 5
   bits from each unicode characters as the bit index. */

/* the linebreak mask is set up by _PyUnicode_Init() below */

#if LONG_BIT >= 128
#define BLOOM_WIDTH 128
#elif LONG_BIT >= 64
#define BLOOM_WIDTH 64
#elif LONG_BIT >= 32
#define BLOOM_WIDTH 32
#else
#error "LONG_BIT is smaller than 32"
#endif

#define BLOOM_MASK unsigned long

static BLOOM_MASK bloom_linebreak = ~(BLOOM_MASK)0;

#define BLOOM(mask, ch)     ((mask &  (1UL << ((ch) & (BLOOM_WIDTH - 1)))))

#define BLOOM_LINEBREAK(ch)                                             \
    ((ch) < 128U ? ascii_linebreak[(ch)] :                              \
     (BLOOM(bloom_linebreak, (ch)) && Py_UNICODE_ISLINEBREAK(ch)))

static inline BLOOM_MASK
make_bloom_mask(int kind, const void* ptr, Py_ssize_t len)
{
#define BLOOM_UPDATE(TYPE, MASK, PTR, LEN)             \
    do {                                               \
        TYPE *data = (TYPE *)PTR;                      \
        TYPE *end = data + LEN;                        \
        Py_UCS4 ch;                                    \
        for (; data != end; data++) {                  \
            ch = *data;                                \
            MASK |= (1UL << (ch & (BLOOM_WIDTH - 1))); \
        }                                              \
        break;                                         \
    } while (0)

    /* calculate simple bloom-style bitmask for a given unicode string */

    BLOOM_MASK mask;

    mask = 0;
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        BLOOM_UPDATE(Py_UCS1, mask, ptr, len);
        break;
    case PyUnicode_2BYTE_KIND:
        BLOOM_UPDATE(Py_UCS2, mask, ptr, len);
        break;
    case PyUnicode_4BYTE_KIND:
        BLOOM_UPDATE(Py_UCS4, mask, ptr, len);
        break;
    default:
        Py_UNREACHABLE();
    }
    return mask;

#undef BLOOM_UPDATE
}


/* Compilation of templated routines */

#define STRINGLIB_GET_EMPTY() _PyUnicode_GetEmpty()

#include "stringlib/asciilib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/find_max_char.h"
#include "stringlib/partition.h"
#include "stringlib/split.h"
#include "stringlib/undef.h"

#include "stringlib/ucs1lib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/find_max_char.h"
#include "stringlib/partition.h"
#include "stringlib/replace.h"
#include "stringlib/repr.h"
#include "stringlib/split.h"
#include "stringlib/undef.h"

#include "stringlib/ucs2lib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/find_max_char.h"
#include "stringlib/partition.h"
#include "stringlib/replace.h"
#include "stringlib/repr.h"
#include "stringlib/split.h"
#include "stringlib/undef.h"

#include "stringlib/ucs4lib.h"
#include "stringlib/fastsearch.h"
#include "stringlib/count.h"
#include "stringlib/find.h"
#include "stringlib/find_max_char.h"
#include "stringlib/partition.h"
#include "stringlib/replace.h"
#include "stringlib/repr.h"
#include "stringlib/split.h"
#include "stringlib/undef.h"

#undef STRINGLIB_GET_EMPTY


Py_ssize_t
_PyUnicode_FindChar(const void *s, int kind,
                    Py_ssize_t size, Py_UCS4 ch,
                    int direction)
{
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        if ((Py_UCS1) ch != ch)
            return -1;
        if (direction > 0)
            return ucs1lib_find_char((const Py_UCS1 *) s, size, (Py_UCS1) ch);
        else
            return ucs1lib_rfind_char((const Py_UCS1 *) s, size, (Py_UCS1) ch);
    case PyUnicode_2BYTE_KIND:
        if ((Py_UCS2) ch != ch)
            return -1;
        if (direction > 0)
            return ucs2lib_find_char((const Py_UCS2 *) s, size, (Py_UCS2) ch);
        else
            return ucs2lib_rfind_char((const Py_UCS2 *) s, size, (Py_UCS2) ch);
    case PyUnicode_4BYTE_KIND:
        if (direction > 0)
            return ucs4lib_find_char((const Py_UCS4 *) s, size, ch);
        else
            return ucs4lib_rfind_char((const Py_UCS4 *) s, size, ch);
    default:
        Py_UNREACHABLE();
    }
}


Py_ssize_t
PyUnicode_FindChar(PyObject *str, Py_UCS4 ch,
                   Py_ssize_t start, Py_ssize_t end,
                   int direction)
{
    int kind;
    Py_ssize_t len, result;
    len = PyUnicode_GET_LENGTH(str);
    ADJUST_INDICES(start, end, len);
    if (end - start < 1)
        return -1;
    kind = PyUnicode_KIND(str);
    result = _PyUnicode_FindChar(PyUnicode_1BYTE_DATA(str) + kind*start,
                                 kind, end-start, ch, direction);
    if (result == -1)
        return -1;
    else
        return start + result;
}


int
_PyUnicode_Tailmatch(PyObject *self,
                     PyObject *substring,
                     Py_ssize_t start,
                     Py_ssize_t end,
                     int direction)
{
    int kind_self;
    int kind_sub;
    const void *data_self;
    const void *data_sub;
    Py_ssize_t offset;
    Py_ssize_t i;
    Py_ssize_t end_sub;

    ADJUST_INDICES(start, end, PyUnicode_GET_LENGTH(self));
    end -= PyUnicode_GET_LENGTH(substring);
    if (end < start)
        return 0;

    if (PyUnicode_GET_LENGTH(substring) == 0)
        return 1;

    kind_self = PyUnicode_KIND(self);
    data_self = PyUnicode_DATA(self);
    kind_sub = PyUnicode_KIND(substring);
    data_sub = PyUnicode_DATA(substring);
    end_sub = PyUnicode_GET_LENGTH(substring) - 1;

    if (direction > 0)
        offset = end;
    else
        offset = start;

    if (PyUnicode_READ(kind_self, data_self, offset) ==
        PyUnicode_READ(kind_sub, data_sub, 0) &&
        PyUnicode_READ(kind_self, data_self, offset + end_sub) ==
        PyUnicode_READ(kind_sub, data_sub, end_sub)) {
        /* If both are of the same kind, memcmp is sufficient */
        if (kind_self == kind_sub) {
            return ! memcmp((char *)data_self +
                                (offset * PyUnicode_KIND(substring)),
                            data_sub,
                            PyUnicode_GET_LENGTH(substring) *
                                PyUnicode_KIND(substring));
        }
        /* otherwise we have to compare each character by first accessing it */
        else {
            /* We do not need to compare 0 and len(substring)-1 because
               the if statement above ensured already that they are equal
               when we end up here. */
            for (i = 1; i < end_sub; ++i) {
                if (PyUnicode_READ(kind_self, data_self, offset + i) !=
                    PyUnicode_READ(kind_sub, data_sub, i))
                    return 0;
            }
            return 1;
        }
    }

    return 0;
}


Py_ssize_t
PyUnicode_Tailmatch(PyObject *str,
                    PyObject *substr,
                    Py_ssize_t start,
                    Py_ssize_t end,
                    int direction)
{
    if (ensure_unicode(str) < 0 || ensure_unicode(substr) < 0)
        return -1;

    return _PyUnicode_Tailmatch(str, substr, start, end, direction);
}


static Py_ssize_t
anylib_find(int kind, PyObject *str1, const void *buf1, Py_ssize_t len1,
            PyObject *str2, const void *buf2, Py_ssize_t len2, Py_ssize_t offset)
{
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        if (PyUnicode_IS_ASCII(str1) && PyUnicode_IS_ASCII(str2))
            return asciilib_find(buf1, len1, buf2, len2, offset);
        else
            return ucs1lib_find(buf1, len1, buf2, len2, offset);
    case PyUnicode_2BYTE_KIND:
        return ucs2lib_find(buf1, len1, buf2, len2, offset);
    case PyUnicode_4BYTE_KIND:
        return ucs4lib_find(buf1, len1, buf2, len2, offset);
    }
    Py_UNREACHABLE();
}


Py_ssize_t
_PyUnicode_AnylibFindSlice(PyObject* s1, PyObject* s2,
                           Py_ssize_t start,
                           Py_ssize_t end,
                           int direction)
{
    int kind1, kind2;
    const void *buf1, *buf2;
    Py_ssize_t len1, len2, result;

    kind1 = PyUnicode_KIND(s1);
    kind2 = PyUnicode_KIND(s2);
    if (kind1 < kind2)
        return -1;

    len1 = PyUnicode_GET_LENGTH(s1);
    len2 = PyUnicode_GET_LENGTH(s2);
    ADJUST_INDICES(start, end, len1);
    if (end - start < len2)
        return -1;

    buf1 = PyUnicode_DATA(s1);
    buf2 = PyUnicode_DATA(s2);
    if (len2 == 1) {
        Py_UCS4 ch = PyUnicode_READ(kind2, buf2, 0);
        result = _PyUnicode_FindChar((const char *)buf1 + kind1*start,
                                     kind1, end - start, ch, direction);
        if (result == -1)
            return -1;
        else
            return start + result;
    }

    if (kind2 != kind1) {
        buf2 = unicode_askind(kind2, buf2, len2, kind1);
        if (!buf2)
            return -2;
    }

    if (direction > 0) {
        switch (kind1) {
        case PyUnicode_1BYTE_KIND:
            if (PyUnicode_IS_ASCII(s1) && PyUnicode_IS_ASCII(s2))
                result = asciilib_find_slice(buf1, len1, buf2, len2, start, end);
            else
                result = ucs1lib_find_slice(buf1, len1, buf2, len2, start, end);
            break;
        case PyUnicode_2BYTE_KIND:
            result = ucs2lib_find_slice(buf1, len1, buf2, len2, start, end);
            break;
        case PyUnicode_4BYTE_KIND:
            result = ucs4lib_find_slice(buf1, len1, buf2, len2, start, end);
            break;
        default:
            Py_UNREACHABLE();
        }
    }
    else {
        switch (kind1) {
        case PyUnicode_1BYTE_KIND:
            if (PyUnicode_IS_ASCII(s1) && PyUnicode_IS_ASCII(s2))
                result = asciilib_rfind_slice(buf1, len1, buf2, len2, start, end);
            else
                result = ucs1lib_rfind_slice(buf1, len1, buf2, len2, start, end);
            break;
        case PyUnicode_2BYTE_KIND:
            result = ucs2lib_rfind_slice(buf1, len1, buf2, len2, start, end);
            break;
        case PyUnicode_4BYTE_KIND:
            result = ucs4lib_rfind_slice(buf1, len1, buf2, len2, start, end);
            break;
        default:
            Py_UNREACHABLE();
        }
    }

    assert((kind2 != kind1) == (buf2 != PyUnicode_DATA(s2)));
    if (kind2 != kind1)
        PyMem_Free((void *)buf2);

    return result;
}


Py_ssize_t
PyUnicode_Find(PyObject *str,
               PyObject *substr,
               Py_ssize_t start,
               Py_ssize_t end,
               int direction)
{
    if (ensure_unicode(str) < 0 || ensure_unicode(substr) < 0)
        return -2;

    return _PyUnicode_AnylibFindSlice(str, substr, start, end, direction);
}


static Py_ssize_t
anylib_count(int kind, PyObject *sstr, const void* sbuf,
             Py_ssize_t slen, PyObject *str1, const void *buf1,
             Py_ssize_t len1, Py_ssize_t maxcount)
{
    switch (kind) {
    case PyUnicode_1BYTE_KIND:
        return ucs1lib_count(sbuf, slen, buf1, len1, maxcount);
    case PyUnicode_2BYTE_KIND:
        return ucs2lib_count(sbuf, slen, buf1, len1, maxcount);
    case PyUnicode_4BYTE_KIND:
        return ucs4lib_count(sbuf, slen, buf1, len1, maxcount);
    }
    Py_UNREACHABLE();
}


Py_ssize_t
_PyUnicode_Count(PyObject *str, PyObject *substr, Py_ssize_t start,
                 Py_ssize_t end)
{
    assert(PyUnicode_Check(str));
    assert(PyUnicode_Check(substr));

    Py_ssize_t result;
    int kind1, kind2;
    const void *buf1 = NULL, *buf2 = NULL;
    Py_ssize_t len1, len2;

    kind1 = PyUnicode_KIND(str);
    kind2 = PyUnicode_KIND(substr);
    if (kind1 < kind2)
        return 0;

    len1 = PyUnicode_GET_LENGTH(str);
    len2 = PyUnicode_GET_LENGTH(substr);
    ADJUST_INDICES(start, end, len1);
    if (end - start < len2)
        return 0;

    buf1 = PyUnicode_DATA(str);
    buf2 = PyUnicode_DATA(substr);
    if (kind2 != kind1) {
        buf2 = unicode_askind(kind2, buf2, len2, kind1);
        if (!buf2)
            goto onError;
    }

    // We don't reuse `anylib_count` here because of the explicit casts.
    switch (kind1) {
    case PyUnicode_1BYTE_KIND:
        result = ucs1lib_count(
            ((const Py_UCS1*)buf1) + start, end - start,
            buf2, len2, PY_SSIZE_T_MAX
            );
        break;
    case PyUnicode_2BYTE_KIND:
        result = ucs2lib_count(
            ((const Py_UCS2*)buf1) + start, end - start,
            buf2, len2, PY_SSIZE_T_MAX
            );
        break;
    case PyUnicode_4BYTE_KIND:
        result = ucs4lib_count(
            ((const Py_UCS4*)buf1) + start, end - start,
            buf2, len2, PY_SSIZE_T_MAX
            );
        break;
    default:
        Py_UNREACHABLE();
    }

    assert((kind2 != kind1) == (buf2 != PyUnicode_DATA(substr)));
    if (kind2 != kind1)
        PyMem_Free((void *)buf2);

    return result;
  onError:
    assert((kind2 != kind1) == (buf2 != PyUnicode_DATA(substr)));
    if (kind2 != kind1)
        PyMem_Free((void *)buf2);
    return -1;
}


Py_ssize_t
PyUnicode_Count(PyObject *str,
                PyObject *substr,
                Py_ssize_t start,
                Py_ssize_t end)
{
    if (ensure_unicode(str) < 0 || ensure_unicode(substr) < 0)
        return -1;

    return _PyUnicode_Count(str, substr, start, end);
}


static void
replace_1char_inplace(PyObject *u, Py_ssize_t pos,
                      Py_UCS4 u1, Py_UCS4 u2, Py_ssize_t maxcount)
{
    int kind = PyUnicode_KIND(u);
    void *data = PyUnicode_DATA(u);
    Py_ssize_t len = PyUnicode_GET_LENGTH(u);
    if (kind == PyUnicode_1BYTE_KIND) {
        ucs1lib_replace_1char_inplace((Py_UCS1 *)data + pos,
                                      (Py_UCS1 *)data + len,
                                      u1, u2, maxcount);
    }
    else if (kind == PyUnicode_2BYTE_KIND) {
        ucs2lib_replace_1char_inplace((Py_UCS2 *)data + pos,
                                      (Py_UCS2 *)data + len,
                                      u1, u2, maxcount);
    }
    else {
        assert(kind == PyUnicode_4BYTE_KIND);
        ucs4lib_replace_1char_inplace((Py_UCS4 *)data + pos,
                                      (Py_UCS4 *)data + len,
                                      u1, u2, maxcount);
    }
}


/* Ensure that a string uses the most efficient storage, if it is not the
   case: create a new string with of the right kind. Write NULL into *p_unicode
   on error. */
static void
unicode_adjust_maxchar(PyObject **p_unicode)
{
    PyObject *unicode, *copy;
    Py_UCS4 max_char;
    Py_ssize_t len;
    int kind;

    assert(p_unicode != NULL);
    unicode = *p_unicode;
    if (PyUnicode_IS_ASCII(unicode))
        return;

    len = PyUnicode_GET_LENGTH(unicode);
    kind = PyUnicode_KIND(unicode);
    if (kind == PyUnicode_1BYTE_KIND) {
        const Py_UCS1 *u = PyUnicode_1BYTE_DATA(unicode);
        max_char = ucs1lib_find_max_char(u, u + len);
        if (max_char >= 128)
            return;
    }
    else if (kind == PyUnicode_2BYTE_KIND) {
        const Py_UCS2 *u = PyUnicode_2BYTE_DATA(unicode);
        max_char = ucs2lib_find_max_char(u, u + len);
        if (max_char >= 256)
            return;
    }
    else if (kind == PyUnicode_4BYTE_KIND) {
        const Py_UCS4 *u = PyUnicode_4BYTE_DATA(unicode);
        max_char = ucs4lib_find_max_char(u, u + len);
        if (max_char >= 0x10000)
            return;
    }
    else
        Py_UNREACHABLE();

    copy = PyUnicode_New(len, max_char);
    if (copy != NULL)
        _PyUnicode_FastCopyCharacters(copy, 0, unicode, 0, len);
    Py_DECREF(unicode);
    *p_unicode = copy;
}


PyObject*
_PyUnicode_Replace(PyObject *self, PyObject *str1,
                   PyObject *str2, Py_ssize_t maxcount)
{
    PyObject *u;
    const char *sbuf = PyUnicode_DATA(self);
    const void *buf1 = PyUnicode_DATA(str1);
    const void *buf2 = PyUnicode_DATA(str2);
    int srelease = 0, release1 = 0, release2 = 0;
    int skind = PyUnicode_KIND(self);
    int kind1 = PyUnicode_KIND(str1);
    int kind2 = PyUnicode_KIND(str2);
    Py_ssize_t slen = PyUnicode_GET_LENGTH(self);
    Py_ssize_t len1 = PyUnicode_GET_LENGTH(str1);
    Py_ssize_t len2 = PyUnicode_GET_LENGTH(str2);
    int mayshrink;
    Py_UCS4 maxchar, maxchar_str1, maxchar_str2;

    if (slen < len1)
        goto nothing;

    if (maxcount < 0)
        maxcount = PY_SSIZE_T_MAX;
    else if (maxcount == 0)
        goto nothing;

    if (str1 == str2)
        goto nothing;

    maxchar = PyUnicode_MAX_CHAR_VALUE(self);
    maxchar_str1 = PyUnicode_MAX_CHAR_VALUE(str1);
    if (maxchar < maxchar_str1)
        /* substring too wide to be present */
        goto nothing;
    maxchar_str2 = PyUnicode_MAX_CHAR_VALUE(str2);
    /* Replacing str1 with str2 may cause a maxchar reduction in the
       result string. */
    mayshrink = (maxchar_str2 < maxchar_str1) && (maxchar == maxchar_str1);
    maxchar = Py_MAX(maxchar, maxchar_str2);

    if (len1 == len2) {
        /* same length */
        if (len1 == 0)
            goto nothing;
        if (len1 == 1) {
            /* replace characters */
            Py_UCS4 u1, u2;
            Py_ssize_t pos;

            u1 = PyUnicode_READ(kind1, buf1, 0);
            pos = _PyUnicode_FindChar(sbuf, skind, slen, u1, 1);
            if (pos < 0)
                goto nothing;
            u2 = PyUnicode_READ(kind2, buf2, 0);
            u = PyUnicode_New(slen, maxchar);
            if (!u)
                goto error;

            _PyUnicode_FastCopyCharacters(u, 0, self, 0, slen);
            replace_1char_inplace(u, pos, u1, u2, maxcount);
        }
        else {
            int rkind = skind;
            char *res;
            Py_ssize_t i;

            if (kind1 < rkind) {
                /* widen substring */
                buf1 = unicode_askind(kind1, buf1, len1, rkind);
                if (!buf1) goto error;
                release1 = 1;
            }
            i = anylib_find(rkind, self, sbuf, slen, str1, buf1, len1, 0);
            if (i < 0)
                goto nothing;
            if (rkind > kind2) {
                /* widen replacement */
                buf2 = unicode_askind(kind2, buf2, len2, rkind);
                if (!buf2) goto error;
                release2 = 1;
            }
            else if (rkind < kind2) {
                /* widen self and buf1 */
                rkind = kind2;
                if (release1) {
                    assert(buf1 != PyUnicode_DATA(str1));
                    PyMem_Free((void *)buf1);
                    buf1 = PyUnicode_DATA(str1);
                    release1 = 0;
                }
                sbuf = unicode_askind(skind, sbuf, slen, rkind);
                if (!sbuf) goto error;
                srelease = 1;
                buf1 = unicode_askind(kind1, buf1, len1, rkind);
                if (!buf1) goto error;
                release1 = 1;
            }
            u = PyUnicode_New(slen, maxchar);
            if (!u)
                goto error;
            assert(PyUnicode_KIND(u) == rkind);
            res = PyUnicode_DATA(u);

            memcpy(res, sbuf, rkind * slen);
            /* change everything in-place, starting with this one */
            memcpy(res + rkind * i,
                   buf2,
                   rkind * len2);
            i += len1;

            while ( --maxcount > 0) {
                i = anylib_find(rkind, self,
                                sbuf+rkind*i, slen-i,
                                str1, buf1, len1, i);
                if (i == -1)
                    break;
                memcpy(res + rkind * i,
                       buf2,
                       rkind * len2);
                i += len1;
            }
        }
    }
    else {
        Py_ssize_t n, i, j, ires;
        Py_ssize_t new_size;
        int rkind = skind;
        char *res;

        if (kind1 < rkind) {
            /* widen substring */
            buf1 = unicode_askind(kind1, buf1, len1, rkind);
            if (!buf1) goto error;
            release1 = 1;
        }
        n = anylib_count(rkind, self, sbuf, slen, str1,
                         buf1, len1, maxcount);
        if (n == 0)
            goto nothing;
        if (kind2 < rkind) {
            /* widen replacement */
            buf2 = unicode_askind(kind2, buf2, len2, rkind);
            if (!buf2) goto error;
            release2 = 1;
        }
        else if (kind2 > rkind) {
            /* widen self and buf1 */
            rkind = kind2;
            sbuf = unicode_askind(skind, sbuf, slen, rkind);
            if (!sbuf) goto error;
            srelease = 1;
            if (release1) {
                assert(buf1 != PyUnicode_DATA(str1));
                PyMem_Free((void *)buf1);
                buf1 = PyUnicode_DATA(str1);
                release1 = 0;
            }
            buf1 = unicode_askind(kind1, buf1, len1, rkind);
            if (!buf1) goto error;
            release1 = 1;
        }
        /* new_size = PyUnicode_GET_LENGTH(self) + n * (PyUnicode_GET_LENGTH(str2) -
           PyUnicode_GET_LENGTH(str1)); */
        if (len1 < len2 && len2 - len1 > (PY_SSIZE_T_MAX - slen) / n) {
                PyErr_SetString(PyExc_OverflowError,
                                "replace string is too long");
                goto error;
        }
        new_size = slen + n * (len2 - len1);
        if (new_size == 0) {
            u = unicode_get_empty();
            goto done;
        }
        if (new_size > (PY_SSIZE_T_MAX / rkind)) {
            PyErr_SetString(PyExc_OverflowError,
                            "replace string is too long");
            goto error;
        }
        u = PyUnicode_New(new_size, maxchar);
        if (!u)
            goto error;
        assert(PyUnicode_KIND(u) == rkind);
        res = PyUnicode_DATA(u);
        ires = i = 0;
        if (len1 > 0) {
            while (n-- > 0) {
                /* look for next match */
                j = anylib_find(rkind, self,
                                sbuf + rkind * i, slen-i,
                                str1, buf1, len1, i);
                if (j == -1)
                    break;
                else if (j > i) {
                    /* copy unchanged part [i:j] */
                    memcpy(res + rkind * ires,
                           sbuf + rkind * i,
                           rkind * (j-i));
                    ires += j - i;
                }
                /* copy substitution string */
                if (len2 > 0) {
                    memcpy(res + rkind * ires,
                           buf2,
                           rkind * len2);
                    ires += len2;
                }
                i = j + len1;
            }
            if (i < slen)
                /* copy tail [i:] */
                memcpy(res + rkind * ires,
                       sbuf + rkind * i,
                       rkind * (slen-i));
        }
        else {
            /* interleave */
            while (n > 0) {
                memcpy(res + rkind * ires,
                       buf2,
                       rkind * len2);
                ires += len2;
                if (--n <= 0)
                    break;
                memcpy(res + rkind * ires,
                       sbuf + rkind * i,
                       rkind);
                ires++;
                i++;
            }
            memcpy(res + rkind * ires,
                   sbuf + rkind * i,
                   rkind * (slen-i));
        }
    }

    if (mayshrink) {
        unicode_adjust_maxchar(&u);
        if (u == NULL)
            goto error;
    }

  done:
    assert(srelease == (sbuf != PyUnicode_DATA(self)));
    assert(release1 == (buf1 != PyUnicode_DATA(str1)));
    assert(release2 == (buf2 != PyUnicode_DATA(str2)));
    if (srelease)
        PyMem_Free((void *)sbuf);
    if (release1)
        PyMem_Free((void *)buf1);
    if (release2)
        PyMem_Free((void *)buf2);
    assert(_PyUnicode_CheckConsistency(u, 1));
    return u;

  nothing:
    /* nothing to replace; return original string (when possible) */
    assert(srelease == (sbuf != PyUnicode_DATA(self)));
    assert(release1 == (buf1 != PyUnicode_DATA(str1)));
    assert(release2 == (buf2 != PyUnicode_DATA(str2)));
    if (srelease)
        PyMem_Free((void *)sbuf);
    if (release1)
        PyMem_Free((void *)buf1);
    if (release2)
        PyMem_Free((void *)buf2);
    return _PyUnicode_ResultUnchanged(self);

  error:
    assert(srelease == (sbuf != PyUnicode_DATA(self)));
    assert(release1 == (buf1 != PyUnicode_DATA(str1)));
    assert(release2 == (buf2 != PyUnicode_DATA(str2)));
    if (srelease)
        PyMem_Free((void *)sbuf);
    if (release1)
        PyMem_Free((void *)buf1);
    if (release2)
        PyMem_Free((void *)buf2);
    return NULL;
}


Py_UCS4
_PyUnicode_FindMaxChar(PyObject *unicode, Py_ssize_t start, Py_ssize_t end)
{
    int kind;
    const void *startptr, *endptr;

    assert(0 <= start);
    assert(end <= PyUnicode_GET_LENGTH(unicode));
    assert(start <= end);

    if (start == 0 && end == PyUnicode_GET_LENGTH(unicode))
        return PyUnicode_MAX_CHAR_VALUE(unicode);

    if (start == end)
        return 127;

    if (PyUnicode_IS_ASCII(unicode))
        return 127;

    kind = PyUnicode_KIND(unicode);
    startptr = PyUnicode_DATA(unicode);
    endptr = (char *)startptr + end * kind;
    startptr = (char *)startptr + start * kind;
    switch(kind) {
    case PyUnicode_1BYTE_KIND:
        return ucs1lib_find_max_char(startptr, endptr);
    case PyUnicode_2BYTE_KIND:
        return ucs2lib_find_max_char(startptr, endptr);
    case PyUnicode_4BYTE_KIND:
        return ucs4lib_find_max_char(startptr, endptr);
    default:
        Py_UNREACHABLE();
    }
}


/*
This function searchs the longest common leading whitespace
of all lines in the [src, end).
It returns the length of the common leading whitespace and sets `output` to
point to the beginning of the common leading whitespace if length > 0.
*/
static Py_ssize_t
search_longest_common_leading_whitespace(
    const char *const src,
    const char *const end,
    const char **output)
{
    // [_start, _start + _len)
    // describes the current longest common leading whitespace
    const char *_start = NULL;
    Py_ssize_t _len = 0;

    for (const char *iter = src; iter < end; ++iter) {
        const char *line_start = iter;
        const char *leading_whitespace_end = NULL;

        // scan the whole line
        while (iter < end && *iter != '\n') {
            if (!leading_whitespace_end && *iter != ' ' && *iter != '\t') {
                /* `iter` points to the first non-whitespace character
                   in this line */
                if (iter == line_start) {
                    // some line has no indent, fast exit!
                    return 0;
                }
                leading_whitespace_end = iter;
            }
            ++iter;
        }

        // if this line has all white space, skip it
        if (!leading_whitespace_end) {
            continue;
        }

        if (!_start) {
            // update the first leading whitespace
            _start = line_start;
            _len = leading_whitespace_end - line_start;
            assert(_len > 0);
        }
        else {
            /* We then compare with the current longest leading whitespace.

               [line_start, leading_whitespace_end) is the leading
               whitespace of this line,

               [_start, _start + _len) is the leading whitespace of the
               current longest leading whitespace. */
            Py_ssize_t new_len = 0;
            const char *_iter = _start, *line_iter = line_start;

            while (_iter < _start + _len && line_iter < leading_whitespace_end
                   && *_iter == *line_iter)
            {
                ++_iter;
                ++line_iter;
                ++new_len;
            }

            _len = new_len;
            if (_len == 0) {
                // No common things now, fast exit!
                return 0;
            }
        }
    }

    assert(_len >= 0);
    if (_len > 0) {
        *output = _start;
    }
    return _len;
}


/* Dedent a string.
   Behaviour is expected to be an exact match of `textwrap.dedent`.
   Return a new reference on success, NULL with exception set on error.
   */
PyObject *
_PyUnicode_Dedent(PyObject *unicode)
{
    Py_ssize_t src_len = 0;
    const char *src = PyUnicode_AsUTF8AndSize(unicode, &src_len);
    if (!src) {
        return NULL;
    }
    assert(src_len >= 0);
    if (src_len == 0) {
        return Py_NewRef(unicode);
    }

    const char *const end = src + src_len;

    // [whitespace_start, whitespace_start + whitespace_len)
    // describes the current longest common leading whitespace
    const char *whitespace_start = NULL;
    Py_ssize_t whitespace_len = search_longest_common_leading_whitespace(
        src, end, &whitespace_start);

    if (whitespace_len == 0) {
        return Py_NewRef(unicode);
    }

    // now we should trigger a dedent
    char *dest = PyMem_Malloc(src_len);
    if (!dest) {
        PyErr_NoMemory();
        return NULL;
    }
    char *dest_iter = dest;

    for (const char *iter = src; iter < end; ++iter) {
        const char *line_start = iter;
        bool in_leading_space = true;

        // iterate over a line to find the end of a line
        while (iter < end && *iter != '\n') {
            if (in_leading_space && *iter != ' ' && *iter != '\t') {
                in_leading_space = false;
            }
            ++iter;
        }

        // invariant: *iter == '\n' or iter == end
        bool append_newline = iter < end;

        // if this line has all white space, write '\n' and continue
        if (in_leading_space && append_newline) {
            *dest_iter++ = '\n';
            continue;
        }

        /* copy [new_line_start + whitespace_len, iter) to buffer, then
            conditionally append '\n' */

        Py_ssize_t new_line_len = iter - line_start - whitespace_len;
        assert(new_line_len >= 0);
        memcpy(dest_iter, line_start + whitespace_len, new_line_len);

        dest_iter += new_line_len;

        if (append_newline) {
            *dest_iter++ = '\n';
        }
    }

    PyObject *res = PyUnicode_FromStringAndSize(dest, dest_iter - dest);
    PyMem_Free(dest);
    return res;
}


static inline void
unicode_fill(int kind, void *data, Py_UCS4 value,
             Py_ssize_t start, Py_ssize_t length)
{
    assert(0 <= start);
    switch (kind) {
    case PyUnicode_1BYTE_KIND: {
        assert(value <= 0xff);
        Py_UCS1 ch = (unsigned char)value;
        Py_UCS1 *to = (Py_UCS1 *)data + start;
        memset(to, ch, length);
        break;
    }
    case PyUnicode_2BYTE_KIND: {
        assert(value <= 0xffff);
        Py_UCS2 ch = (Py_UCS2)value;
        Py_UCS2 *to = (Py_UCS2 *)data + start;
        const Py_UCS2 *end = to + length;
        for (; to < end; ++to) *to = ch;
        break;
    }
    case PyUnicode_4BYTE_KIND: {
        assert(value <= _Py_MAX_UNICODE);
        Py_UCS4 ch = value;
        Py_UCS4 * to = (Py_UCS4 *)data + start;
        const Py_UCS4 *end = to + length;
        for (; to < end; ++to) *to = ch;
        break;
    }
    default: Py_UNREACHABLE();
    }
}


void
_PyUnicode_Fill(int kind, void *data, Py_UCS4 value,
                Py_ssize_t start, Py_ssize_t length)
{
    unicode_fill(kind, data, value, start, length);
}


void
_PyUnicode_FastFill(PyObject *unicode, Py_ssize_t start, Py_ssize_t length,
                    Py_UCS4 fill_char)
{
    const int kind = PyUnicode_KIND(unicode);
    void *data = PyUnicode_DATA(unicode);
    assert(_PyUnicode_IsModifiable(unicode));
    assert(fill_char <= PyUnicode_MAX_CHAR_VALUE(unicode));
    assert(start >= 0);
    assert(start + length <= PyUnicode_GET_LENGTH(unicode));
    unicode_fill(kind, data, fill_char, start, length);
}


Py_ssize_t
PyUnicode_Fill(PyObject *unicode, Py_ssize_t start, Py_ssize_t length,
               Py_UCS4 fill_char)
{
    Py_ssize_t maxlen;

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (_PyUnicode_CheckModifiable(unicode))
        return -1;

    if (start < 0) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return -1;
    }
    if (fill_char > PyUnicode_MAX_CHAR_VALUE(unicode)) {
        PyErr_SetString(PyExc_ValueError,
                         "fill character is bigger than "
                         "the string maximum character");
        return -1;
    }

    maxlen = PyUnicode_GET_LENGTH(unicode) - start;
    length = Py_MIN(maxlen, length);
    if (length <= 0)
        return 0;

    _PyUnicode_FastFill(unicode, start, length, fill_char);
    return length;
}


PyObject *
PyUnicode_Partition(PyObject *str_obj, PyObject *sep_obj)
{
    PyObject* out;
    int kind1, kind2;
    const void *buf1, *buf2;
    Py_ssize_t len1, len2;

    if (ensure_unicode(str_obj) < 0 || ensure_unicode(sep_obj) < 0)
        return NULL;

    kind1 = PyUnicode_KIND(str_obj);
    kind2 = PyUnicode_KIND(sep_obj);
    len1 = PyUnicode_GET_LENGTH(str_obj);
    len2 = PyUnicode_GET_LENGTH(sep_obj);
    if (kind1 < kind2 || len1 < len2) {
        PyObject *empty = unicode_get_empty();  // Borrowed reference
        return PyTuple_Pack(3, str_obj, empty, empty);
    }
    buf1 = PyUnicode_DATA(str_obj);
    buf2 = PyUnicode_DATA(sep_obj);
    if (kind2 != kind1) {
        buf2 = _PyUnicode_AsKind(kind2, buf2, len2, kind1);
        if (!buf2)
            return NULL;
    }

    switch (kind1) {
    case PyUnicode_1BYTE_KIND:
        if (PyUnicode_IS_ASCII(str_obj) && PyUnicode_IS_ASCII(sep_obj))
            out = asciilib_partition(str_obj, buf1, len1, sep_obj, buf2, len2);
        else
            out = ucs1lib_partition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    case PyUnicode_2BYTE_KIND:
        out = ucs2lib_partition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    case PyUnicode_4BYTE_KIND:
        out = ucs4lib_partition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    default:
        Py_UNREACHABLE();
    }

    assert((kind2 == kind1) == (buf2 == PyUnicode_DATA(sep_obj)));
    if (kind2 != kind1)
        PyMem_Free((void *)buf2);

    return out;
}


PyObject *
PyUnicode_RPartition(PyObject *str_obj, PyObject *sep_obj)
{
    PyObject* out;
    int kind1, kind2;
    const void *buf1, *buf2;
    Py_ssize_t len1, len2;

    if (ensure_unicode(str_obj) < 0 || ensure_unicode(sep_obj) < 0)
        return NULL;

    kind1 = PyUnicode_KIND(str_obj);
    kind2 = PyUnicode_KIND(sep_obj);
    len1 = PyUnicode_GET_LENGTH(str_obj);
    len2 = PyUnicode_GET_LENGTH(sep_obj);
    if (kind1 < kind2 || len1 < len2) {
        PyObject *empty = unicode_get_empty();  // Borrowed reference
        return PyTuple_Pack(3, empty, empty, str_obj);
    }
    buf1 = PyUnicode_DATA(str_obj);
    buf2 = PyUnicode_DATA(sep_obj);
    if (kind2 != kind1) {
        buf2 = _PyUnicode_AsKind(kind2, buf2, len2, kind1);
        if (!buf2)
            return NULL;
    }

    switch (kind1) {
    case PyUnicode_1BYTE_KIND:
        if (PyUnicode_IS_ASCII(str_obj) && PyUnicode_IS_ASCII(sep_obj))
            out = asciilib_rpartition(str_obj, buf1, len1, sep_obj, buf2, len2);
        else
            out = ucs1lib_rpartition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    case PyUnicode_2BYTE_KIND:
        out = ucs2lib_rpartition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    case PyUnicode_4BYTE_KIND:
        out = ucs4lib_rpartition(str_obj, buf1, len1, sep_obj, buf2, len2);
        break;
    default:
        Py_UNREACHABLE();
    }

    assert((kind2 == kind1) == (buf2 == PyUnicode_DATA(sep_obj)));
    if (kind2 != kind1)
        PyMem_Free((void *)buf2);

    return out;
}


static const char*
unicode_kind_name(PyObject *unicode)
{
    /* don't check consistency: unicode_kind_name() is called from
       _PyUnicode_Dump() */
    if (!PyUnicode_IS_COMPACT(unicode))
    {
        switch (PyUnicode_KIND(unicode))
        {
        case PyUnicode_1BYTE_KIND:
            if (PyUnicode_IS_ASCII(unicode))
                return "legacy ascii";
            else
                return "legacy latin1";
        case PyUnicode_2BYTE_KIND:
            return "legacy UCS2";
        case PyUnicode_4BYTE_KIND:
            return "legacy UCS4";
        default:
            return "<legacy invalid kind>";
        }
    }
    switch (PyUnicode_KIND(unicode)) {
    case PyUnicode_1BYTE_KIND:
        if (PyUnicode_IS_ASCII(unicode))
            return "ascii";
        else
            return "latin1";
    case PyUnicode_2BYTE_KIND:
        return "UCS2";
    case PyUnicode_4BYTE_KIND:
        return "UCS4";
    default:
        return "<invalid compact kind>";
    }
}


#ifdef Py_DEBUG
/* Functions wrapping macros for use in debugger */
const char *_PyUnicode_utf8(void *unicode_raw){
    PyObject *unicode = _PyObject_CAST(unicode_raw);
    return PyUnicode_UTF8(unicode);
}

const void *_PyUnicode_compact_data(void *unicode_raw) {
    PyObject *unicode = _PyObject_CAST(unicode_raw);
    return _PyUnicode_COMPACT_DATA(unicode);
}
const void *_PyUnicode_data(void *unicode_raw) {
    PyObject *unicode = _PyObject_CAST(unicode_raw);
    printf("obj %p\n", (void*)unicode);
    printf("compact %d\n", PyUnicode_IS_COMPACT(unicode));
    printf("compact ascii %d\n", PyUnicode_IS_COMPACT_ASCII(unicode));
    printf("ascii op %p\n", (void*)(_PyASCIIObject_CAST(unicode) + 1));
    printf("compact op %p\n", (void*)(_PyCompactUnicodeObject_CAST(unicode) + 1));
    printf("compact data %p\n", _PyUnicode_COMPACT_DATA(unicode));
    return PyUnicode_DATA(unicode);
}


void
_PyUnicode_Dump(PyObject *op)
{
    PyASCIIObject *ascii = _PyASCIIObject_CAST(op);
    PyCompactUnicodeObject *compact = _PyCompactUnicodeObject_CAST(op);
    PyUnicodeObject *unicode = _PyUnicodeObject_CAST(op);
    const void *data;

    if (ascii->state.compact)
    {
        if (ascii->state.ascii)
            data = (ascii + 1);
        else
            data = (compact + 1);
    }
    else
        data = unicode->data.any;
    printf("%s: len=%zu, ", unicode_kind_name(op), ascii->length);

    if (!ascii->state.ascii) {
        printf("utf8=%p (%zu)", (void *)compact->utf8, compact->utf8_length);
    }
    printf(", data=%p\n", data);
}
#endif


static int
_copy_characters(PyObject *to, Py_ssize_t to_start,
                 PyObject *from, Py_ssize_t from_start,
                 Py_ssize_t how_many, int check_maxchar)
{
    int from_kind, to_kind;
    const void *from_data;
    void *to_data;

    assert(0 <= how_many);
    assert(0 <= from_start);
    assert(0 <= to_start);
    assert(PyUnicode_Check(from));
    assert(from_start + how_many <= PyUnicode_GET_LENGTH(from));

    assert(to == NULL || PyUnicode_Check(to));

    if (how_many == 0) {
        return 0;
    }

    assert(to != NULL);
    assert(to_start + how_many <= PyUnicode_GET_LENGTH(to));

    from_kind = PyUnicode_KIND(from);
    from_data = PyUnicode_DATA(from);
    to_kind = PyUnicode_KIND(to);
    to_data = PyUnicode_DATA(to);

#ifdef Py_DEBUG
    if (!check_maxchar
        && PyUnicode_MAX_CHAR_VALUE(from) > PyUnicode_MAX_CHAR_VALUE(to))
    {
        Py_UCS4 to_maxchar = PyUnicode_MAX_CHAR_VALUE(to);
        Py_UCS4 ch;
        Py_ssize_t i;
        for (i=0; i < how_many; i++) {
            ch = PyUnicode_READ(from_kind, from_data, from_start + i);
            assert(ch <= to_maxchar);
        }
    }
#endif

    if (from_kind == to_kind) {
        if (check_maxchar
            && !PyUnicode_IS_ASCII(from) && PyUnicode_IS_ASCII(to))
        {
            /* Writing Latin-1 characters into an ASCII string requires to
               check that all written characters are pure ASCII */
            Py_UCS4 max_char;
            max_char = ucs1lib_find_max_char(from_data,
                                             (const Py_UCS1*)from_data + how_many);
            if (max_char >= 128)
                return -1;
        }
        memcpy((char*)to_data + to_kind * to_start,
                  (const char*)from_data + from_kind * from_start,
                  to_kind * how_many);
    }
    else if (from_kind == PyUnicode_1BYTE_KIND
             && to_kind == PyUnicode_2BYTE_KIND)
    {
        _PyUnicode_CONVERT_BYTES(
            Py_UCS1, Py_UCS2,
            PyUnicode_1BYTE_DATA(from) + from_start,
            PyUnicode_1BYTE_DATA(from) + from_start + how_many,
            PyUnicode_2BYTE_DATA(to) + to_start
            );
    }
    else if (from_kind == PyUnicode_1BYTE_KIND
             && to_kind == PyUnicode_4BYTE_KIND)
    {
        _PyUnicode_CONVERT_BYTES(
            Py_UCS1, Py_UCS4,
            PyUnicode_1BYTE_DATA(from) + from_start,
            PyUnicode_1BYTE_DATA(from) + from_start + how_many,
            PyUnicode_4BYTE_DATA(to) + to_start
            );
    }
    else if (from_kind == PyUnicode_2BYTE_KIND
             && to_kind == PyUnicode_4BYTE_KIND)
    {
        _PyUnicode_CONVERT_BYTES(
            Py_UCS2, Py_UCS4,
            PyUnicode_2BYTE_DATA(from) + from_start,
            PyUnicode_2BYTE_DATA(from) + from_start + how_many,
            PyUnicode_4BYTE_DATA(to) + to_start
            );
    }
    else {
        assert (PyUnicode_MAX_CHAR_VALUE(from) > PyUnicode_MAX_CHAR_VALUE(to));

        if (!check_maxchar) {
            if (from_kind == PyUnicode_2BYTE_KIND
                && to_kind == PyUnicode_1BYTE_KIND)
            {
                _PyUnicode_CONVERT_BYTES(
                    Py_UCS2, Py_UCS1,
                    PyUnicode_2BYTE_DATA(from) + from_start,
                    PyUnicode_2BYTE_DATA(from) + from_start + how_many,
                    PyUnicode_1BYTE_DATA(to) + to_start
                    );
            }
            else if (from_kind == PyUnicode_4BYTE_KIND
                     && to_kind == PyUnicode_1BYTE_KIND)
            {
                _PyUnicode_CONVERT_BYTES(
                    Py_UCS4, Py_UCS1,
                    PyUnicode_4BYTE_DATA(from) + from_start,
                    PyUnicode_4BYTE_DATA(from) + from_start + how_many,
                    PyUnicode_1BYTE_DATA(to) + to_start
                    );
            }
            else if (from_kind == PyUnicode_4BYTE_KIND
                     && to_kind == PyUnicode_2BYTE_KIND)
            {
                _PyUnicode_CONVERT_BYTES(
                    Py_UCS4, Py_UCS2,
                    PyUnicode_4BYTE_DATA(from) + from_start,
                    PyUnicode_4BYTE_DATA(from) + from_start + how_many,
                    PyUnicode_2BYTE_DATA(to) + to_start
                    );
            }
            else {
                Py_UNREACHABLE();
            }
        }
        else {
            const Py_UCS4 to_maxchar = PyUnicode_MAX_CHAR_VALUE(to);
            Py_UCS4 ch;
            Py_ssize_t i;

            for (i=0; i < how_many; i++) {
                ch = PyUnicode_READ(from_kind, from_data, from_start + i);
                if (ch > to_maxchar)
                    return -1;
                PyUnicode_WRITE(to_kind, to_data, to_start + i, ch);
            }
        }
    }
    return 0;
}

void
_PyUnicode_FastCopyCharacters(
    PyObject *to, Py_ssize_t to_start,
    PyObject *from, Py_ssize_t from_start, Py_ssize_t how_many)
{
    (void)_copy_characters(to, to_start, from, from_start, how_many, 0);
}

Py_ssize_t
PyUnicode_CopyCharacters(PyObject *to, Py_ssize_t to_start,
                         PyObject *from, Py_ssize_t from_start,
                         Py_ssize_t how_many)
{
    int err;

    if (!PyUnicode_Check(from) || !PyUnicode_Check(to)) {
        PyErr_BadInternalCall();
        return -1;
    }

    if ((size_t)from_start > (size_t)PyUnicode_GET_LENGTH(from)) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return -1;
    }
    if ((size_t)to_start > (size_t)PyUnicode_GET_LENGTH(to)) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return -1;
    }
    if (how_many < 0) {
        PyErr_SetString(PyExc_SystemError, "how_many cannot be negative");
        return -1;
    }
    how_many = Py_MIN(PyUnicode_GET_LENGTH(from)-from_start, how_many);
    if (to_start + how_many > PyUnicode_GET_LENGTH(to)) {
        PyErr_Format(PyExc_SystemError,
                     "Cannot write %zi characters at %zi "
                     "in a string of %zi characters",
                     how_many, to_start, PyUnicode_GET_LENGTH(to));
        return -1;
    }

    if (how_many == 0)
        return 0;

    if (_PyUnicode_CheckModifiable(to))
        return -1;

    err = _copy_characters(to, to_start, from, from_start, how_many, 1);
    if (err) {
        PyErr_Format(PyExc_SystemError,
                     "Cannot copy %s characters "
                     "into a string of %s characters",
                     unicode_kind_name(from),
                     unicode_kind_name(to));
        return -1;
    }
    return how_many;
}


PyObject *
_PyUnicode_Repr(PyObject *unicode)
{
    Py_ssize_t isize = PyUnicode_GET_LENGTH(unicode);
    const void *idata = PyUnicode_DATA(unicode);

    /* Compute length of output, quote characters, and
       maximum character */
    Py_ssize_t osize = 0;
    Py_UCS4 maxch = 127;
    Py_ssize_t squote = 0;
    Py_ssize_t dquote = 0;
    int ikind = PyUnicode_KIND(unicode);
    for (Py_ssize_t i = 0; i < isize; i++) {
        Py_UCS4 ch = PyUnicode_READ(ikind, idata, i);
        Py_ssize_t incr = 1;
        switch (ch) {
        case '\'': squote++; break;
        case '"':  dquote++; break;
        case '\\': case '\t': case '\r': case '\n':
            incr = 2;
            break;
        default:
            /* Fast-path ASCII */
            if (ch < ' ' || ch == 0x7f)
                incr = 4; /* \xHH */
            else if (ch < 0x7f)
                ;
            else if (Py_UNICODE_ISPRINTABLE(ch))
                maxch = (ch > maxch) ? ch : maxch;
            else if (ch < 0x100)
                incr = 4; /* \xHH */
            else if (ch < 0x10000)
                incr = 6; /* \uHHHH */
            else
                incr = 10; /* \uHHHHHHHH */
        }
        if (osize > PY_SSIZE_T_MAX - incr) {
            PyErr_SetString(PyExc_OverflowError,
                            "string is too long to generate repr");
            return NULL;
        }
        osize += incr;
    }

    Py_UCS4 quote = '\'';
    int changed = (osize != isize);
    if (squote) {
        changed = 1;
        if (dquote)
            /* Both squote and dquote present. Use squote,
               and escape them */
            osize += squote;
        else
            quote = '"';
    }
    osize += 2;   /* quotes */

    PyObject *repr = PyUnicode_New(osize, maxch);
    if (repr == NULL)
        return NULL;
    int okind = PyUnicode_KIND(repr);
    void *odata = PyUnicode_DATA(repr);

    if (!changed) {
        PyUnicode_WRITE(okind, odata, 0, quote);

        _PyUnicode_FastCopyCharacters(repr, 1,
                                      unicode, 0,
                                      isize);

        PyUnicode_WRITE(okind, odata, osize-1, quote);
    }
    else {
        switch (okind) {
        case PyUnicode_1BYTE_KIND:
            ucs1lib_repr(unicode, quote, odata);
            break;
        case PyUnicode_2BYTE_KIND:
            ucs2lib_repr(unicode, quote, odata);
            break;
        default:
            assert(okind == PyUnicode_4BYTE_KIND);
            ucs4lib_repr(unicode, quote, odata);
        }
    }

    assert(_PyUnicode_CheckConsistency(repr, 1));
    return repr;
}


Py_UCS4
PyUnicode_ReadChar(PyObject *unicode, Py_ssize_t index)
{
    const void *data;
    int kind;

    if (!PyUnicode_Check(unicode)) {
        PyErr_BadArgument();
        return (Py_UCS4)-1;
    }
    if (index < 0 || index >= PyUnicode_GET_LENGTH(unicode)) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return (Py_UCS4)-1;
    }
    data = PyUnicode_DATA(unicode);
    kind = PyUnicode_KIND(unicode);
    return PyUnicode_READ(kind, data, index);
}


int
PyUnicode_WriteChar(PyObject *unicode, Py_ssize_t index, Py_UCS4 ch)
{
    if (!PyUnicode_Check(unicode) || !PyUnicode_IS_COMPACT(unicode)) {
        PyErr_BadArgument();
        return -1;
    }
    if (index < 0 || index >= PyUnicode_GET_LENGTH(unicode)) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return -1;
    }
    if (_PyUnicode_CheckModifiable(unicode))
        return -1;
    if (ch > PyUnicode_MAX_CHAR_VALUE(unicode)) {
        PyErr_SetString(PyExc_ValueError, "character out of range");
        return -1;
    }
    PyUnicode_WRITE(PyUnicode_KIND(unicode), PyUnicode_DATA(unicode),
                    index, ch);
    return 0;
}


const char *
PyUnicode_GetDefaultEncoding(void)
{
    return "utf-8";
}


PyObject *
_PyUnicode_TransformDecimalAndSpaceToASCII(PyObject *unicode)
{
    if (!PyUnicode_Check(unicode)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    if (PyUnicode_IS_ASCII(unicode)) {
        /* If the string is already ASCII, just return the same string */
        return Py_NewRef(unicode);
    }

    Py_ssize_t len = PyUnicode_GET_LENGTH(unicode);
    PyObject *result = PyUnicode_New(len, 127);
    if (result == NULL) {
        return NULL;
    }

    Py_UCS1 *out = PyUnicode_1BYTE_DATA(result);
    int kind = PyUnicode_KIND(unicode);
    const void *data = PyUnicode_DATA(unicode);
    Py_ssize_t i;
    for (i = 0; i < len; ++i) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        if (ch < 127) {
            out[i] = ch;
        }
        else if (Py_UNICODE_ISSPACE(ch)) {
            out[i] = ' ';
        }
        else {
            int decimal = Py_UNICODE_TODECIMAL(ch);
            if (decimal < 0) {
                out[i] = '?';
                out[i+1] = '\0';
                _PyUnicode_LENGTH(result) = i + 1;
                break;
            }
            out[i] = '0' + decimal;
        }
    }

    assert(_PyUnicode_CheckConsistency(result, 1));
    return result;
}


PyObject *
PyUnicode_Join(PyObject *separator, PyObject *seq)
{
    PyObject *res;
    PyObject *fseq;
    Py_ssize_t seqlen;
    PyObject **items;

    fseq = PySequence_Fast(seq, "can only join an iterable");
    if (fseq == NULL) {
        return NULL;
    }

    Py_BEGIN_CRITICAL_SECTION_SEQUENCE_FAST(seq);

    items = PySequence_Fast_ITEMS(fseq);
    seqlen = PySequence_Fast_GET_SIZE(fseq);
    res = _PyUnicode_JoinArray(separator, items, seqlen);

    Py_END_CRITICAL_SECTION_SEQUENCE_FAST();

    Py_DECREF(fseq);
    return res;
}

PyObject *
_PyUnicode_JoinArray(PyObject *separator, PyObject *const *items, Py_ssize_t seqlen)
{
    PyObject *res = NULL; /* the result */
    PyObject *sep = NULL;
    Py_ssize_t seplen;
    PyObject *item;
    Py_ssize_t sz, i, res_offset;
    Py_UCS4 maxchar;
    Py_UCS4 item_maxchar;
    int use_memcpy;
    unsigned char *res_data = NULL, *sep_data = NULL;
    PyObject *last_obj;
    int kind = 0;

    /* If empty sequence, return u"". */
    if (seqlen == 0) {
        return _PyUnicode_GetEmpty();
    }

    /* If singleton sequence with an exact Unicode, return that. */
    last_obj = NULL;
    if (seqlen == 1) {
        if (PyUnicode_CheckExact(items[0])) {
            res = items[0];
            return Py_NewRef(res);
        }
        seplen = 0;
        maxchar = 0;
    }
    else {
        /* Set up sep and seplen */
        if (separator == NULL) {
            /* fall back to a blank space separator */
            sep = PyUnicode_FromOrdinal(' ');
            if (!sep)
                goto onError;
            seplen = 1;
            maxchar = 32;
        }
        else {
            if (!PyUnicode_Check(separator)) {
                PyErr_Format(PyExc_TypeError,
                             "separator: expected str instance,"
                             " %.80s found",
                             Py_TYPE(separator)->tp_name);
                goto onError;
            }
            sep = separator;
            seplen = PyUnicode_GET_LENGTH(separator);
            maxchar = PyUnicode_MAX_CHAR_VALUE(separator);
            /* inc refcount to keep this code path symmetric with the
               above case of a blank separator */
            Py_INCREF(sep);
        }
        last_obj = sep;
    }

    /* There are at least two things to join, or else we have a subclass
     * of str in the sequence.
     * Do a pre-pass to figure out the total amount of space we'll
     * need (sz), and see whether all argument are strings.
     */
    sz = 0;
#ifdef Py_DEBUG
    use_memcpy = 0;
#else
    use_memcpy = 1;
#endif
    for (i = 0; i < seqlen; i++) {
        size_t add_sz;
        item = items[i];
        if (!PyUnicode_Check(item)) {
            PyErr_Format(PyExc_TypeError,
                         "sequence item %zd: expected str instance,"
                         " %.80s found",
                         i, Py_TYPE(item)->tp_name);
            goto onError;
        }
        add_sz = PyUnicode_GET_LENGTH(item);
        item_maxchar = PyUnicode_MAX_CHAR_VALUE(item);
        maxchar = Py_MAX(maxchar, item_maxchar);
        if (i != 0) {
            add_sz += seplen;
        }
        if (add_sz > (size_t)(PY_SSIZE_T_MAX - sz)) {
            PyErr_SetString(PyExc_OverflowError,
                            "join() result is too long for a Python string");
            goto onError;
        }
        sz += add_sz;
        if (use_memcpy && last_obj != NULL) {
            if (PyUnicode_KIND(last_obj) != PyUnicode_KIND(item))
                use_memcpy = 0;
        }
        last_obj = item;
    }

    res = PyUnicode_New(sz, maxchar);
    if (res == NULL)
        goto onError;

    /* Catenate everything. */
#ifdef Py_DEBUG
    use_memcpy = 0;
#else
    if (use_memcpy) {
        res_data = PyUnicode_1BYTE_DATA(res);
        kind = PyUnicode_KIND(res);
        if (seplen != 0)
            sep_data = PyUnicode_1BYTE_DATA(sep);
    }
#endif
    if (use_memcpy) {
        for (i = 0; i < seqlen; ++i) {
            Py_ssize_t itemlen;
            item = items[i];

            /* Copy item, and maybe the separator. */
            if (i && seplen != 0) {
                memcpy(res_data,
                          sep_data,
                          kind * seplen);
                res_data += kind * seplen;
            }

            itemlen = PyUnicode_GET_LENGTH(item);
            if (itemlen != 0) {
                memcpy(res_data,
                          PyUnicode_DATA(item),
                          kind * itemlen);
                res_data += kind * itemlen;
            }
        }
        assert(res_data == PyUnicode_1BYTE_DATA(res)
                           + kind * PyUnicode_GET_LENGTH(res));
    }
    else {
        for (i = 0, res_offset = 0; i < seqlen; ++i) {
            Py_ssize_t itemlen;
            item = items[i];

            /* Copy item, and maybe the separator. */
            if (i && seplen != 0) {
                _PyUnicode_FastCopyCharacters(res, res_offset, sep, 0, seplen);
                res_offset += seplen;
            }

            itemlen = PyUnicode_GET_LENGTH(item);
            if (itemlen != 0) {
                _PyUnicode_FastCopyCharacters(res, res_offset, item, 0, itemlen);
                res_offset += itemlen;
            }
        }
        assert(res_offset == PyUnicode_GET_LENGTH(res));
    }

    Py_XDECREF(sep);
    assert(_PyUnicode_CheckConsistency(res, 1));
    return res;

  onError:
    Py_XDECREF(sep);
    Py_XDECREF(res);
    return NULL;
}


PyObject*
_PyUnicode_Pad(PyObject *self,
               Py_ssize_t left,
               Py_ssize_t right,
               Py_UCS4 fill)
{
    PyObject *u;
    Py_UCS4 maxchar;
    int kind;
    void *data;

    if (left < 0)
        left = 0;
    if (right < 0)
        right = 0;

    if (left == 0 && right == 0)
        return _PyUnicode_ResultUnchanged(self);

    if (left > PY_SSIZE_T_MAX - _PyUnicode_LENGTH(self) ||
        right > PY_SSIZE_T_MAX - (left + _PyUnicode_LENGTH(self))) {
        PyErr_SetString(PyExc_OverflowError, "padded string is too long");
        return NULL;
    }
    maxchar = PyUnicode_MAX_CHAR_VALUE(self);
    maxchar = Py_MAX(maxchar, fill);
    u = PyUnicode_New(left + _PyUnicode_LENGTH(self) + right, maxchar);
    if (!u)
        return NULL;

    kind = PyUnicode_KIND(u);
    data = PyUnicode_DATA(u);
    if (left)
        _PyUnicode_Fill(kind, data, fill, 0, left);
    if (right)
        _PyUnicode_Fill(kind, data, fill, left + _PyUnicode_LENGTH(self), right);
    _PyUnicode_FastCopyCharacters(u, left, self, 0, _PyUnicode_LENGTH(self));
    assert(_PyUnicode_CheckConsistency(u, 1));
    return u;
}


int
_PyUnicode_Equal(PyObject *str1, PyObject *str2)
{
    assert(PyUnicode_Check(str1));
    assert(PyUnicode_Check(str2));
    if (str1 == str2) {
        return 1;
    }
    return unicode_eq(str1, str2);
}


int
PyUnicode_Equal(PyObject *str1, PyObject *str2)
{
    if (!PyUnicode_Check(str1)) {
        PyErr_Format(PyExc_TypeError,
                     "first argument must be str, not %T", str1);
        return -1;
    }
    if (!PyUnicode_Check(str2)) {
        PyErr_Format(PyExc_TypeError,
                     "second argument must be str, not %T", str2);
        return -1;
    }

    return _PyUnicode_Equal(str1, str2);
}


static int
unicode_compare(PyObject *str1, PyObject *str2)
{
#define COMPARE(TYPE1, TYPE2) \
    do { \
        TYPE1* p1 = (TYPE1 *)data1; \
        TYPE2* p2 = (TYPE2 *)data2; \
        TYPE1* end = p1 + len; \
        Py_UCS4 c1, c2; \
        for (; p1 != end; p1++, p2++) { \
            c1 = *p1; \
            c2 = *p2; \
            if (c1 != c2) \
                return (c1 < c2) ? -1 : 1; \
        } \
    } \
    while (0)

    int kind1, kind2;
    const void *data1, *data2;
    Py_ssize_t len1, len2, len;

    kind1 = PyUnicode_KIND(str1);
    kind2 = PyUnicode_KIND(str2);
    data1 = PyUnicode_DATA(str1);
    data2 = PyUnicode_DATA(str2);
    len1 = PyUnicode_GET_LENGTH(str1);
    len2 = PyUnicode_GET_LENGTH(str2);
    len = Py_MIN(len1, len2);

    switch(kind1) {
    case PyUnicode_1BYTE_KIND:
    {
        switch(kind2) {
        case PyUnicode_1BYTE_KIND:
        {
            int cmp = memcmp(data1, data2, len);
            /* normalize result of memcmp() into the range [-1; 1] */
            if (cmp < 0)
                return -1;
            if (cmp > 0)
                return 1;
            break;
        }
        case PyUnicode_2BYTE_KIND:
            COMPARE(Py_UCS1, Py_UCS2);
            break;
        case PyUnicode_4BYTE_KIND:
            COMPARE(Py_UCS1, Py_UCS4);
            break;
        default:
            Py_UNREACHABLE();
        }
        break;
    }
    case PyUnicode_2BYTE_KIND:
    {
        switch(kind2) {
        case PyUnicode_1BYTE_KIND:
            COMPARE(Py_UCS2, Py_UCS1);
            break;
        case PyUnicode_2BYTE_KIND:
        {
            COMPARE(Py_UCS2, Py_UCS2);
            break;
        }
        case PyUnicode_4BYTE_KIND:
            COMPARE(Py_UCS2, Py_UCS4);
            break;
        default:
            Py_UNREACHABLE();
        }
        break;
    }
    case PyUnicode_4BYTE_KIND:
    {
        switch(kind2) {
        case PyUnicode_1BYTE_KIND:
            COMPARE(Py_UCS4, Py_UCS1);
            break;
        case PyUnicode_2BYTE_KIND:
            COMPARE(Py_UCS4, Py_UCS2);
            break;
        case PyUnicode_4BYTE_KIND:
        {
#if defined(HAVE_WMEMCMP) && SIZEOF_WCHAR_T == 4
            int cmp = wmemcmp((wchar_t *)data1, (wchar_t *)data2, len);
            /* normalize result of wmemcmp() into the range [-1; 1] */
            if (cmp < 0)
                return -1;
            if (cmp > 0)
                return 1;
#else
            COMPARE(Py_UCS4, Py_UCS4);
#endif
            break;
        }
        default:
            Py_UNREACHABLE();
        }
        break;
    }
    default:
        Py_UNREACHABLE();
    }

    if (len1 == len2)
        return 0;
    if (len1 < len2)
        return -1;
    else
        return 1;

#undef COMPARE
}


int
PyUnicode_Compare(PyObject *left, PyObject *right)
{
    if (PyUnicode_Check(left) && PyUnicode_Check(right)) {
        /* a string is equal to itself */
        if (left == right)
            return 0;

        return unicode_compare(left, right);
    }
    PyErr_Format(PyExc_TypeError,
                 "Can't compare %.100s and %.100s",
                 Py_TYPE(left)->tp_name,
                 Py_TYPE(right)->tp_name);
    return -1;
}

int
PyUnicode_CompareWithASCIIString(PyObject* uni, const char* str)
{
    Py_ssize_t i;
    int kind;
    Py_UCS4 chr;

    assert(_PyUnicode_CHECK(uni));
    kind = PyUnicode_KIND(uni);
    if (kind == PyUnicode_1BYTE_KIND) {
        const void *data = PyUnicode_1BYTE_DATA(uni);
        size_t len1 = (size_t)PyUnicode_GET_LENGTH(uni);
        size_t len, len2 = strlen(str);
        int cmp;

        len = Py_MIN(len1, len2);
        cmp = memcmp(data, str, len);
        if (cmp != 0) {
            if (cmp < 0)
                return -1;
            else
                return 1;
        }
        if (len1 > len2)
            return 1; /* uni is longer */
        if (len1 < len2)
            return -1; /* str is longer */
        return 0;
    }
    else {
        const void *data = PyUnicode_DATA(uni);
        /* Compare Unicode string and source character set string */
        for (i = 0; (chr = PyUnicode_READ(kind, data, i)) && str[i]; i++)
            if (chr != (unsigned char)str[i])
                return (chr < (unsigned char)(str[i])) ? -1 : 1;
        /* This check keeps Python strings that end in '\0' from comparing equal
         to C strings identical up to that point. */
        if (PyUnicode_GET_LENGTH(uni) != i || chr)
            return 1; /* uni is longer */
        if (str[i])
            return -1; /* str is longer */
        return 0;
    }
}

int
PyUnicode_EqualToUTF8(PyObject *unicode, const char *str)
{
    return PyUnicode_EqualToUTF8AndSize(unicode, str, strlen(str));
}

int
PyUnicode_EqualToUTF8AndSize(PyObject *unicode, const char *str, Py_ssize_t size)
{
    assert(_PyUnicode_CHECK(unicode));
    assert(str);

    if (PyUnicode_IS_ASCII(unicode)) {
        Py_ssize_t len = PyUnicode_GET_LENGTH(unicode);
        return size == len &&
            memcmp(PyUnicode_1BYTE_DATA(unicode), str, len) == 0;
    }
    if (PyUnicode_UTF8(unicode) != NULL) {
        Py_ssize_t len = PyUnicode_UTF8_LENGTH(unicode);
        return size == len &&
            memcmp(PyUnicode_UTF8(unicode), str, len) == 0;
    }

    Py_ssize_t len = PyUnicode_GET_LENGTH(unicode);
    if ((size_t)len >= (size_t)size || (size_t)len < (size_t)size / 4) {
        return 0;
    }
    const unsigned char *s = (const unsigned char *)str;
    const unsigned char *ends = s + (size_t)size;
    int kind = PyUnicode_KIND(unicode);
    const void *data = PyUnicode_DATA(unicode);
    /* Compare Unicode string and UTF-8 string */
    for (Py_ssize_t i = 0; i < len; i++) {
        Py_UCS4 ch = PyUnicode_READ(kind, data, i);
        if (ch < 0x80) {
            if (ends == s || s[0] != ch) {
                return 0;
            }
            s += 1;
        }
        else if (ch < 0x800) {
            if ((ends - s) < 2 ||
                s[0] != (0xc0 | (ch >> 6)) ||
                s[1] != (0x80 | (ch & 0x3f)))
            {
                return 0;
            }
            s += 2;
        }
        else if (ch < 0x10000) {
            if (Py_UNICODE_IS_SURROGATE(ch) ||
                (ends - s) < 3 ||
                s[0] != (0xe0 | (ch >> 12)) ||
                s[1] != (0x80 | ((ch >> 6) & 0x3f)) ||
                s[2] != (0x80 | (ch & 0x3f)))
            {
                return 0;
            }
            s += 3;
        }
        else {
            assert(ch <= _Py_MAX_UNICODE);
            if ((ends - s) < 4 ||
                s[0] != (0xf0 | (ch >> 18)) ||
                s[1] != (0x80 | ((ch >> 12) & 0x3f)) ||
                s[2] != (0x80 | ((ch >> 6) & 0x3f)) ||
                s[3] != (0x80 | (ch & 0x3f)))
            {
                return 0;
            }
            s += 4;
        }
    }
    return s == ends;
}

int
_PyUnicode_EqualToASCIIString(PyObject *unicode, const char *str)
{
    size_t len;
    assert(_PyUnicode_CHECK(unicode));
    assert(str);
#ifndef NDEBUG
    for (const char *p = str; *p; p++) {
        assert((unsigned char)*p < 128);
    }
#endif
    if (!PyUnicode_IS_ASCII(unicode))
        return 0;
    len = (size_t)PyUnicode_GET_LENGTH(unicode);
    return strlen(str) == len &&
           memcmp(PyUnicode_1BYTE_DATA(unicode), str, len) == 0;
}

int
_PyUnicode_EqualToASCIIId(PyObject *left, _Py_Identifier *right)
{
    PyObject *right_uni;

    assert(_PyUnicode_CHECK(left));
    assert(right->string);
#ifndef NDEBUG
    for (const char *p = right->string; *p; p++) {
        assert((unsigned char)*p < 128);
    }
#endif

    if (!PyUnicode_IS_ASCII(left))
        return 0;

    right_uni = _PyUnicode_FromId(right);       /* borrowed */
    if (right_uni == NULL) {
        /* memory error or bad data */
        PyErr_Clear();
        return _PyUnicode_EqualToASCIIString(left, right->string);
    }

    if (left == right_uni)
        return 1;

    assert(PyUnicode_CHECK_INTERNED(right_uni));
    if (PyUnicode_CHECK_INTERNED(left)) {
        return 0;
    }

    Py_hash_t right_hash = PyUnicode_HASH(right_uni);
    assert(right_hash != -1);
    Py_hash_t hash = PyUnicode_HASH(left);
    if (hash != -1 && hash != right_hash) {
        return 0;
    }

    return unicode_eq(left, right_uni);
}

PyObject *
PyUnicode_RichCompare(PyObject *left, PyObject *right, int op)
{
    int result;

    if (!PyUnicode_Check(left) || !PyUnicode_Check(right))
        Py_RETURN_NOTIMPLEMENTED;

    if (left == right) {
        switch (op) {
        case Py_EQ:
        case Py_LE:
        case Py_GE:
            /* a string is equal to itself */
            Py_RETURN_TRUE;
        case Py_NE:
        case Py_LT:
        case Py_GT:
            Py_RETURN_FALSE;
        default:
            PyErr_BadArgument();
            return NULL;
        }
    }
    else if (op == Py_EQ || op == Py_NE) {
        result = unicode_eq(left, right);
        result ^= (op == Py_NE);
        return PyBool_FromLong(result);
    }
    else {
        result = unicode_compare(left, right);
        Py_RETURN_RICHCOMPARE(result, 0, op);
    }
}

int
PyUnicode_Contains(PyObject *str, PyObject *substr)
{
    int kind1, kind2;
    const void *buf1, *buf2;
    Py_ssize_t len1, len2;
    int result;

    if (!PyUnicode_Check(substr)) {
        PyErr_Format(PyExc_TypeError,
                     "'in <string>' requires string as left operand, not %.100s",
                     Py_TYPE(substr)->tp_name);
        return -1;
    }
    if (ensure_unicode(str) < 0)
        return -1;

    kind1 = PyUnicode_KIND(str);
    kind2 = PyUnicode_KIND(substr);
    if (kind1 < kind2)
        return 0;
    len1 = PyUnicode_GET_LENGTH(str);
    len2 = PyUnicode_GET_LENGTH(substr);
    if (len1 < len2)
        return 0;
    buf1 = PyUnicode_DATA(str);
    buf2 = PyUnicode_DATA(substr);
    if (len2 == 1) {
        Py_UCS4 ch = PyUnicode_READ(kind2, buf2, 0);
        result = (_PyUnicode_FindChar((const char *)buf1, kind1, len1, ch, 1) != -1);
        return result;
    }
    if (kind2 != kind1) {
        buf2 = _PyUnicode_AsKind(kind2, buf2, len2, kind1);
        if (!buf2)
            return -1;
    }

    switch (kind1) {
    case PyUnicode_1BYTE_KIND:
        result = ucs1lib_find(buf1, len1, buf2, len2, 0) != -1;
        break;
    case PyUnicode_2BYTE_KIND:
        result = ucs2lib_find(buf1, len1, buf2, len2, 0) != -1;
        break;
    case PyUnicode_4BYTE_KIND:
        result = ucs4lib_find(buf1, len1, buf2, len2, 0) != -1;
        break;
    default:
        Py_UNREACHABLE();
    }

    assert((kind2 == kind1) == (buf2 == PyUnicode_DATA(substr)));
    if (kind2 != kind1)
        PyMem_Free((void *)buf2);

    return result;
}

/* Concat to string or Unicode object giving a new Unicode object. */

PyObject *
PyUnicode_Concat(PyObject *left, PyObject *right)
{
    PyObject *result;
    Py_UCS4 maxchar, maxchar2;
    Py_ssize_t left_len, right_len, new_len;

    if (ensure_unicode(left) < 0)
        return NULL;

    if (!PyUnicode_Check(right)) {
        PyErr_Format(PyExc_TypeError,
            "can only concatenate str (not \"%.200s\") to str",
            Py_TYPE(right)->tp_name);
        return NULL;
    }

    /* Shortcuts */
    PyObject *empty = unicode_get_empty();  // Borrowed reference
    if (left == empty) {
        return PyUnicode_FromObject(right);
    }
    if (right == empty) {
        return PyUnicode_FromObject(left);
    }

    left_len = PyUnicode_GET_LENGTH(left);
    right_len = PyUnicode_GET_LENGTH(right);
    if (left_len > PY_SSIZE_T_MAX - right_len) {
        PyErr_SetString(PyExc_OverflowError,
                        "strings are too large to concat");
        return NULL;
    }
    new_len = left_len + right_len;

    maxchar = PyUnicode_MAX_CHAR_VALUE(left);
    maxchar2 = PyUnicode_MAX_CHAR_VALUE(right);
    maxchar = Py_MAX(maxchar, maxchar2);

    /* Concat the two Unicode strings */
    result = PyUnicode_New(new_len, maxchar);
    if (result == NULL)
        return NULL;
    _PyUnicode_FastCopyCharacters(result, 0, left, 0, left_len);
    _PyUnicode_FastCopyCharacters(result, left_len, right, 0, right_len);
    assert(_PyUnicode_CheckConsistency(result, 1));
    return result;
}

void
PyUnicode_Append(PyObject **p_left, PyObject *right)
{
    PyObject *left, *res;
    Py_UCS4 maxchar, maxchar2;
    Py_ssize_t left_len, right_len, new_len;

    if (p_left == NULL) {
        if (!PyErr_Occurred())
            PyErr_BadInternalCall();
        return;
    }
    left = *p_left;
    if (right == NULL || left == NULL
        || !PyUnicode_Check(left) || !PyUnicode_Check(right)) {
        if (!PyErr_Occurred())
            PyErr_BadInternalCall();
        goto error;
    }

    /* Shortcuts */
    PyObject *empty = unicode_get_empty();  // Borrowed reference
    if (left == empty) {
        Py_DECREF(left);
        *p_left = Py_NewRef(right);
        return;
    }
    if (right == empty) {
        return;
    }

    left_len = PyUnicode_GET_LENGTH(left);
    right_len = PyUnicode_GET_LENGTH(right);
    if (left_len > PY_SSIZE_T_MAX - right_len) {
        PyErr_SetString(PyExc_OverflowError,
                        "strings are too large to concat");
        goto error;
    }
    new_len = left_len + right_len;

    if (_PyUnicode_IsModifiable(left)
        && PyUnicode_CheckExact(right)
        && PyUnicode_KIND(right) <= PyUnicode_KIND(left)
        /* Don't resize for ascii += latin1. Convert ascii to latin1 requires
           to change the structure size, but characters are stored just after
           the structure, and so it requires to move all characters which is
           not so different than duplicating the string. */
        && !(PyUnicode_IS_ASCII(left) && !PyUnicode_IS_ASCII(right)))
    {
        /* append inplace */
        if (_PyUnicode_Resize(p_left, new_len) != 0)
            goto error;

        /* copy 'right' into the newly allocated area of 'left' */
        _PyUnicode_FastCopyCharacters(*p_left, left_len, right, 0, right_len);
    }
    else {
        maxchar = PyUnicode_MAX_CHAR_VALUE(left);
        maxchar2 = PyUnicode_MAX_CHAR_VALUE(right);
        maxchar = Py_MAX(maxchar, maxchar2);

        /* Concat the two Unicode strings */
        res = PyUnicode_New(new_len, maxchar);
        if (res == NULL)
            goto error;
        _PyUnicode_FastCopyCharacters(res, 0, left, 0, left_len);
        _PyUnicode_FastCopyCharacters(res, left_len, right, 0, right_len);
        Py_DECREF(left);
        *p_left = res;
    }
    assert(_PyUnicode_CheckConsistency(*p_left, 1));
    return;

error:
    Py_CLEAR(*p_left);
}

void
PyUnicode_AppendAndDel(PyObject **pleft, PyObject *right)
{
    PyUnicode_Append(pleft, right);
    Py_XDECREF(right);
}


PyObject*
PyUnicode_Substring(PyObject *self, Py_ssize_t start, Py_ssize_t end)
{
    const unsigned char *data;
    int kind;
    Py_ssize_t length;

    length = PyUnicode_GET_LENGTH(self);
    end = Py_MIN(end, length);

    if (start == 0 && end == length)
        return _PyUnicode_ResultUnchanged(self);

    if (start < 0 || end < 0) {
        PyErr_SetString(PyExc_IndexError, "string index out of range");
        return NULL;
    }
    if (start >= length || end < start) {
        return _PyUnicode_GetEmpty();
    }

    length = end - start;
    if (PyUnicode_IS_ASCII(self)) {
        data = PyUnicode_1BYTE_DATA(self);
        return _PyUnicode_FromASCII((const char*)(data + start), length);
    }
    else {
        kind = PyUnicode_KIND(self);
        data = PyUnicode_1BYTE_DATA(self);
        return PyUnicode_FromKindAndData(kind,
                                         data + kind * start,
                                         length);
    }
}


/* _PyUnicode_InsertThousandsGrouping() helper functions */
#include "stringlib/localeutil.h"

/**
 * InsertThousandsGrouping:
 * @writer: Unicode writer.
 * @n_buffer: Number of characters in @buffer.
 * @digits: Digits we're reading from. If count is non-NULL, this is unused.
 * @d_pos: Start of digits string.
 * @n_digits: The number of digits in the string, in which we want
 *            to put the grouping chars.
 * @min_width: The minimum width of the digits in the output string.
 *             Output will be zero-padded on the left to fill.
 * @grouping: see definition in localeconv().
 * @thousands_sep: see definition in localeconv().
 *
 * There are 2 modes: counting and filling. If @writer is NULL,
 *  we are in counting mode, else filling mode.
 * If counting, the required buffer size is returned.
 * If filling, we know the buffer will be large enough, so we don't
 *  need to pass in the buffer size.
 * Inserts thousand grouping characters (as defined by grouping and
 *  thousands_sep) into @writer.
 *
 * Return value: -1 on error, number of characters otherwise.
 **/
Py_ssize_t
_PyUnicode_InsertThousandsGrouping(
    _PyUnicodeWriter *writer,
    Py_ssize_t n_buffer,
    PyObject *digits,
    Py_ssize_t d_pos,
    Py_ssize_t n_digits,
    Py_ssize_t min_width,
    const char *grouping,
    PyObject *thousands_sep,
    Py_UCS4 *maxchar,
    int forward)
{
    min_width = Py_MAX(0, min_width);
    if (writer) {
        assert(digits != NULL);
        assert(maxchar == NULL);
    }
    else {
        assert(digits == NULL);
        assert(maxchar != NULL);
    }
    assert(0 <= d_pos);
    assert(0 <= n_digits);
    assert(grouping != NULL);

    Py_ssize_t count = 0;
    Py_ssize_t n_zeros;
    int loop_broken = 0;
    int use_separator = 0; /* First time through, don't append the
                              separator. They only go between
                              groups. */
    Py_ssize_t buffer_pos;
    Py_ssize_t digits_pos;
    Py_ssize_t len;
    Py_ssize_t n_chars;
    Py_ssize_t remaining = n_digits; /* Number of chars remaining to
                                        be looked at */
    /* A generator that returns all of the grouping widths, until it
       returns 0. */
    GroupGenerator groupgen;
    GroupGenerator_init(&groupgen, grouping);
    const Py_ssize_t thousands_sep_len = PyUnicode_GET_LENGTH(thousands_sep);

    /* if digits are not grouped, thousands separator
       should be an empty string */
    assert(!(grouping[0] == CHAR_MAX && thousands_sep_len != 0));

    digits_pos = d_pos + (forward ? 0 : n_digits);
    if (writer) {
        buffer_pos = writer->pos + (forward ? 0 : n_buffer);
        assert(buffer_pos <= PyUnicode_GET_LENGTH(writer->buffer));
        assert(digits_pos <= PyUnicode_GET_LENGTH(digits));
    }
    else {
        buffer_pos = forward ? 0 : n_buffer;
    }

    if (!writer) {
        *maxchar = 127;
    }

    while ((len = GroupGenerator_next(&groupgen)) > 0) {
        len = Py_MIN(len, Py_MAX(Py_MAX(remaining, min_width), 1));
        n_zeros = Py_MAX(0, len - remaining);
        n_chars = Py_MAX(0, Py_MIN(remaining, len));

        /* Use n_zero zero's and n_chars chars */

        /* Count only, don't do anything. */
        count += (use_separator ? thousands_sep_len : 0) + n_zeros + n_chars;

        /* Copy into the writer. */
        InsertThousandsGrouping_fill(writer, &buffer_pos,
                                     digits, &digits_pos,
                                     n_chars, n_zeros,
                                     use_separator ? thousands_sep : NULL,
                                     thousands_sep_len, maxchar, forward);

        /* Use a separator next time. */
        use_separator = 1;

        remaining -= n_chars;
        min_width -= len;

        if (remaining <= 0 && min_width <= 0) {
            loop_broken = 1;
            break;
        }
        min_width -= thousands_sep_len;
    }
    if (!loop_broken) {
        /* We left the loop without using a break statement. */

        len = Py_MAX(Py_MAX(remaining, min_width), 1);
        n_zeros = Py_MAX(0, len - remaining);
        n_chars = Py_MAX(0, Py_MIN(remaining, len));

        /* Use n_zero zero's and n_chars chars */
        count += (use_separator ? thousands_sep_len : 0) + n_zeros + n_chars;

        /* Copy into the writer. */
        InsertThousandsGrouping_fill(writer, &buffer_pos,
                                     digits, &digits_pos,
                                     n_chars, n_zeros,
                                     use_separator ? thousands_sep : NULL,
                                     thousands_sep_len, maxchar, forward);
    }
    return count;
}


/* externally visible for str.strip(unicode) */
PyObject *
_PyUnicode_XStrip(PyObject *self, int striptype, PyObject *sepobj)
{
    const void *data;
    int kind;
    Py_ssize_t i, j, len;
    BLOOM_MASK sepmask;
    Py_ssize_t seplen;

    kind = PyUnicode_KIND(self);
    data = PyUnicode_DATA(self);
    len = PyUnicode_GET_LENGTH(self);
    seplen = PyUnicode_GET_LENGTH(sepobj);
    sepmask = make_bloom_mask(PyUnicode_KIND(sepobj),
                              PyUnicode_DATA(sepobj),
                              seplen);

    i = 0;
    if (striptype != _Py_RIGHTSTRIP) {
        while (i < len) {
            Py_UCS4 ch = PyUnicode_READ(kind, data, i);
            if (!BLOOM(sepmask, ch))
                break;
            if (PyUnicode_FindChar(sepobj, ch, 0, seplen, 1) < 0)
                break;
            i++;
        }
    }

    j = len;
    if (striptype != _Py_LEFTSTRIP) {
        j--;
        while (j >= i) {
            Py_UCS4 ch = PyUnicode_READ(kind, data, j);
            if (!BLOOM(sepmask, ch))
                break;
            if (PyUnicode_FindChar(sepobj, ch, 0, seplen, 1) < 0)
                break;
            j--;
        }

        j++;
    }

    return PyUnicode_Substring(self, i, j);
}


PyObject *
_PyUnicode_do_strip(PyObject *self, int striptype)
{
    Py_ssize_t len, i, j;

    len = PyUnicode_GET_LENGTH(self);

    if (PyUnicode_IS_ASCII(self)) {
        const Py_UCS1 *data = PyUnicode_1BYTE_DATA(self);

        i = 0;
        if (striptype != _Py_RIGHTSTRIP) {
            while (i < len) {
                Py_UCS1 ch = data[i];
                if (!_Py_ascii_whitespace[ch])
                    break;
                i++;
            }
        }

        j = len;
        if (striptype != _Py_LEFTSTRIP) {
            j--;
            while (j >= i) {
                Py_UCS1 ch = data[j];
                if (!_Py_ascii_whitespace[ch])
                    break;
                j--;
            }
            j++;
        }
    }
    else {
        int kind = PyUnicode_KIND(self);
        const void *data = PyUnicode_DATA(self);

        i = 0;
        if (striptype != _Py_RIGHTSTRIP) {
            while (i < len) {
                Py_UCS4 ch = PyUnicode_READ(kind, data, i);
                if (!Py_UNICODE_ISSPACE(ch))
                    break;
                i++;
            }
        }

        j = len;
        if (striptype != _Py_LEFTSTRIP) {
            j--;
            while (j >= i) {
                Py_UCS4 ch = PyUnicode_READ(kind, data, j);
                if (!Py_UNICODE_ISSPACE(ch))
                    break;
                j--;
            }
            j++;
        }
    }

    return PyUnicode_Substring(self, i, j);
}


PyObject *
PyUnicode_Splitlines(PyObject *string, int keepends)
{
    PyObject *list;

    if (ensure_unicode(string) < 0)
        return NULL;

    switch (PyUnicode_KIND(string)) {
    case PyUnicode_1BYTE_KIND:
        if (PyUnicode_IS_ASCII(string))
            list = asciilib_splitlines(
                string, PyUnicode_1BYTE_DATA(string),
                PyUnicode_GET_LENGTH(string), keepends);
        else
            list = ucs1lib_splitlines(
                string, PyUnicode_1BYTE_DATA(string),
                PyUnicode_GET_LENGTH(string), keepends);
        break;
    case PyUnicode_2BYTE_KIND:
        list = ucs2lib_splitlines(
            string, PyUnicode_2BYTE_DATA(string),
            PyUnicode_GET_LENGTH(string), keepends);
        break;
    case PyUnicode_4BYTE_KIND:
        list = ucs4lib_splitlines(
            string, PyUnicode_4BYTE_DATA(string),
            PyUnicode_GET_LENGTH(string), keepends);
        break;
    default:
        Py_UNREACHABLE();
    }
    return list;
}


PyObject *
_PyUnicode_Split(PyObject *self,
                 PyObject *substring,
                 Py_ssize_t maxcount)
{
    int kind1, kind2;
    const void *buf1, *buf2;
    Py_ssize_t len1, len2;
    PyObject* out;
    len1 = PyUnicode_GET_LENGTH(self);
    kind1 = PyUnicode_KIND(self);

    if (substring == NULL) {
        if (maxcount < 0) {
            maxcount = (len1 - 1) / 2 + 1;
        }
        switch (kind1) {
        case PyUnicode_1BYTE_KIND:
            if (PyUnicode_IS_ASCII(self))
                return asciilib_split_whitespace(
                    self,  PyUnicode_1BYTE_DATA(self),
                    len1, maxcount
                    );
            else
                return ucs1lib_split_whitespace(
                    self,  PyUnicode_1BYTE_DATA(self),
                    len1, maxcount
                    );
        case PyUnicode_2BYTE_KIND:
            return ucs2lib_split_whitespace(
                self,  PyUnicode_2BYTE_DATA(self),
                len1, maxcount
                );
        case PyUnicode_4BYTE_KIND:
            return ucs4lib_split_whitespace(
                self,  PyUnicode_4BYTE_DATA(self),
                len1, maxcount
                );
        default:
            Py_UNREACHABLE();
        }
    }

    kind2 = PyUnicode_KIND(substring);
    len2 = PyUnicode_GET_LENGTH(substring);
    if (maxcount < 0) {
        // if len2 == 0, it will raise ValueError.
        maxcount = len2 == 0 ? 0 : (len1 / len2) + 1;
        // handle expected overflow case: (Py_SSIZE_T_MAX / 1) + 1
        maxcount = maxcount < 0 ? len1 : maxcount;
    }
    if (kind1 < kind2 || len1 < len2) {
        out = PyList_New(1);
        if (out == NULL)
            return NULL;
        PyList_SET_ITEM(out, 0, Py_NewRef(self));
        return out;
    }
    buf1 = PyUnicode_DATA(self);
    buf2 = PyUnicode_DATA(substring);
    if (kind2 != kind1) {
        buf2 = _PyUnicode_AsKind(kind2, buf2, len2, kind1);
        if (!buf2)
            return NULL;
    }

    switch (kind1) {
    case PyUnicode_1BYTE_KIND:
        if (PyUnicode_IS_ASCII(self) && PyUnicode_IS_ASCII(substring))
            out = asciilib_split(
                self,  buf1, len1, buf2, len2, maxcount);
        else
            out = ucs1lib_split(
                self,  buf1, len1, buf2, len2, maxcount);
        break;
    case PyUnicode_2BYTE_KIND:
        out = ucs2lib_split(
            self,  buf1, len1, buf2, len2, maxcount);
        break;
    case PyUnicode_4BYTE_KIND:
        out = ucs4lib_split(
            self,  buf1, len1, buf2, len2, maxcount);
        break;
    default:
        out = NULL;
    }
    assert((kind2 != kind1) == (buf2 != PyUnicode_DATA(substring)));
    if (kind2 != kind1)
        PyMem_Free((void *)buf2);
    return out;
}


PyObject*
_PyUnicode_RSplit(PyObject *self,
                  PyObject *substring,
                  Py_ssize_t maxcount)
{
    int kind1, kind2;
    const void *buf1, *buf2;
    Py_ssize_t len1, len2;
    PyObject* out;

    len1 = PyUnicode_GET_LENGTH(self);
    kind1 = PyUnicode_KIND(self);

    if (substring == NULL) {
        if (maxcount < 0) {
            maxcount = (len1 - 1) / 2 + 1;
        }
        switch (kind1) {
        case PyUnicode_1BYTE_KIND:
            if (PyUnicode_IS_ASCII(self))
                return asciilib_rsplit_whitespace(
                    self,  PyUnicode_1BYTE_DATA(self),
                    len1, maxcount
                    );
            else
                return ucs1lib_rsplit_whitespace(
                    self,  PyUnicode_1BYTE_DATA(self),
                    len1, maxcount
                    );
        case PyUnicode_2BYTE_KIND:
            return ucs2lib_rsplit_whitespace(
                self,  PyUnicode_2BYTE_DATA(self),
                len1, maxcount
                );
        case PyUnicode_4BYTE_KIND:
            return ucs4lib_rsplit_whitespace(
                self,  PyUnicode_4BYTE_DATA(self),
                len1, maxcount
                );
        default:
            Py_UNREACHABLE();
        }
    }
    kind2 = PyUnicode_KIND(substring);
    len2 = PyUnicode_GET_LENGTH(substring);
    if (maxcount < 0) {
        // if len2 == 0, it will raise ValueError.
        maxcount = len2 == 0 ? 0 : (len1 / len2) + 1;
        // handle expected overflow case: (Py_SSIZE_T_MAX / 1) + 1
        maxcount = maxcount < 0 ? len1 : maxcount;
    }
    if (kind1 < kind2 || len1 < len2) {
        out = PyList_New(1);
        if (out == NULL)
            return NULL;
        PyList_SET_ITEM(out, 0, Py_NewRef(self));
        return out;
    }
    buf1 = PyUnicode_DATA(self);
    buf2 = PyUnicode_DATA(substring);
    if (kind2 != kind1) {
        buf2 = _PyUnicode_AsKind(kind2, buf2, len2, kind1);
        if (!buf2)
            return NULL;
    }

    switch (kind1) {
    case PyUnicode_1BYTE_KIND:
        if (PyUnicode_IS_ASCII(self) && PyUnicode_IS_ASCII(substring))
            out = asciilib_rsplit(
                self,  buf1, len1, buf2, len2, maxcount);
        else
            out = ucs1lib_rsplit(
                self,  buf1, len1, buf2, len2, maxcount);
        break;
    case PyUnicode_2BYTE_KIND:
        out = ucs2lib_rsplit(
            self,  buf1, len1, buf2, len2, maxcount);
        break;
    case PyUnicode_4BYTE_KIND:
        out = ucs4lib_rsplit(
            self,  buf1, len1, buf2, len2, maxcount);
        break;
    default:
        out = NULL;
    }
    assert((kind2 != kind1) == (buf2 != PyUnicode_DATA(substring)));
    if (kind2 != kind1)
        PyMem_Free((void *)buf2);
    return out;
}


void
_PyUnicode_InitGlobalState(void)
{
    static int initialized = 0;
    if (initialized) {
        return;
    }
    initialized = 1;

    /* initialize the linebreak bloom filter */
    const Py_UCS2 linebreak[] = {
        0x000A, /* LINE FEED */
        0x000D, /* CARRIAGE RETURN */
        0x001C, /* FILE SEPARATOR */
        0x001D, /* GROUP SEPARATOR */
        0x001E, /* RECORD SEPARATOR */
        0x0085, /* NEXT LINE */
        0x2028, /* LINE SEPARATOR */
        0x2029, /* PARAGRAPH SEPARATOR */
    };
    bloom_linebreak = make_bloom_mask(
        PyUnicode_2BYTE_KIND, linebreak,
        Py_ARRAY_LENGTH(linebreak));
}


PyObject*
_PyUnicode_Expandtabs(PyObject *self, int tabsize)
{
    Py_ssize_t i, j, line_pos, src_len, incr;
    Py_UCS4 ch;
    PyObject *u;
    const void *src_data;
    void *dest_data;
    int kind;
    int found;

    /* First pass: determine size of output string */
    src_len = PyUnicode_GET_LENGTH(self);
    i = j = line_pos = 0;
    kind = PyUnicode_KIND(self);
    src_data = PyUnicode_DATA(self);
    found = 0;
    for (; i < src_len; i++) {
        ch = PyUnicode_READ(kind, src_data, i);
        if (ch == '\t') {
            found = 1;
            if (tabsize > 0) {
                incr = tabsize - (line_pos % tabsize); /* cannot overflow */
                if (j > PY_SSIZE_T_MAX - incr)
                    goto overflow;
                line_pos += incr;
                j += incr;
            }
        }
        else {
            if (j > PY_SSIZE_T_MAX - 1)
                goto overflow;
            line_pos++;
            j++;
            if (ch == '\n' || ch == '\r')
                line_pos = 0;
        }
    }
    if (!found)
        return _PyUnicode_ResultUnchanged(self);

    /* Second pass: create output string and fill it */
    u = PyUnicode_New(j, PyUnicode_MAX_CHAR_VALUE(self));
    if (!u)
        return NULL;
    dest_data = PyUnicode_DATA(u);

    i = j = line_pos = 0;

    for (; i < src_len; i++) {
        ch = PyUnicode_READ(kind, src_data, i);
        if (ch == '\t') {
            if (tabsize > 0) {
                incr = tabsize - (line_pos % tabsize);
                line_pos += incr;
                _PyUnicode_Fill(kind, dest_data, ' ', j, incr);
                j += incr;
            }
        }
        else {
            line_pos++;
            PyUnicode_WRITE(kind, dest_data, j, ch);
            j++;
            if (ch == '\n' || ch == '\r')
                line_pos = 0;
        }
    }
    assert (j == PyUnicode_GET_LENGTH(u));
    return _PyUnicode_Result(u);

  overflow:
    PyErr_SetString(PyExc_OverflowError, "new string is too long");
    return NULL;
}

