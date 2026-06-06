/* C accelerator for the html module (html.escape and html.unescape).

   escape() scans 1-byte strings word-at-a-time (SWAR) to skip runs with no
   special character eight bytes at a time, using the same broadcast/haszero
   masks as Objects/unicodeobject.c, and returns the input unchanged when there
   is nothing to escape.  unescape() replaces the regex + Python callback with a
   single C pass: it bulk-copies the text between references and binary-searches
   the generated HTML5 named-reference and numeric-charref tables.

   The module has no mutable state: inputs are immutable str objects and the
   lookup tables are read-only, so it supports free-threading and a
   per-interpreter GIL. */

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include <stdint.h>
#include <string.h>

#include "html_entities.h"

#include "clinic/_htmlmodule.c.h"

/*[clinic input]
module _html
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=95e66f9a73b6c8ba]*/

#define SWAR_ONES  0x0101010101010101ULL
#define SWAR_HIGHS 0x8080808080808080ULL

static inline uint64_t
swar_haszero(uint64_t v)
{
    return (v - SWAR_ONES) & ~v & SWAR_HIGHS;
}

static inline uint64_t
swar_hasbyte(uint64_t w, uint8_t c)
{
    return swar_haszero(w ^ (SWAR_ONES * c));
}

static inline uint64_t
swar_specials(uint64_t w, int quote)
{
    uint64_t m = swar_hasbyte(w, '&') | swar_hasbyte(w, '<') | swar_hasbyte(w, '>');
    if (quote) {
        m |= swar_hasbyte(w, '"') | swar_hasbyte(w, '\'');
    }
    return m;
}

static inline Py_ssize_t
escape_extra(Py_UCS4 ch, int quote)
{
    switch (ch) {
        case '&': return 4;               /* "&amp;"  */
        case '<': case '>': return 3;     /* "&lt;"   */
        case '"': return quote ? 5 : 0;   /* "&quot;" */
        case '\'': return quote ? 5 : 0;  /* "&#x27;" */
        default: return 0;
    }
}

static inline Py_ssize_t
write_escaped(int kind, void *data, Py_ssize_t o, Py_UCS4 ch, int quote)
{
    const char *rep = NULL;
    int rlen = 0;
    switch (ch) {
        case '&': rep = "&amp;"; rlen = 5; break;
        case '<': rep = "&lt;"; rlen = 4; break;
        case '>': rep = "&gt;"; rlen = 4; break;
        case '"': if (quote) { rep = "&quot;"; rlen = 6; } break;
        case '\'': if (quote) { rep = "&#x27;"; rlen = 6; } break;
        default: break;
    }
    if (rep != NULL) {
        for (int k = 0; k < rlen; k++) {
            PyUnicode_WRITE(kind, data, o + k, (Py_UCS4)rep[k]);
        }
        return rlen;
    }
    PyUnicode_WRITE(kind, data, o, ch);
    return 1;
}

/*[clinic input]
_html.escape

    s: unicode
    quote: bool = True

Replace special characters "&", "<" and ">" to HTML-safe sequences.

If the optional flag quote is true (the default), the quotation mark
characters, both double quote (") and single quote ('), are also
translated.
[clinic start generated code]*/

static PyObject *
_html_escape_impl(PyObject *module, PyObject *s, int quote)
/*[clinic end generated code: output=7e6916b020ab13bd input=04fd630fd061e3c5]*/
{
    int kind = PyUnicode_KIND(s);
    Py_ssize_t n = PyUnicode_GET_LENGTH(s);
    const void *data = PyUnicode_DATA(s);

    Py_ssize_t extra = 0;
    if (kind == PyUnicode_1BYTE_KIND) {
        const uint8_t *p = (const uint8_t *)data;
        Py_ssize_t i = 0;
        while (i + 8 <= n) {
            uint64_t w;
            memcpy(&w, p + i, 8);
            if (swar_specials(w, quote) == 0) {
                i += 8;
                continue;
            }
            for (int j = 0; j < 8; j++) {
                extra += escape_extra(p[i + j], quote);
            }
            i += 8;
        }
        for (; i < n; i++) {
            extra += escape_extra(p[i], quote);
        }
    }
    else {
        for (Py_ssize_t i = 0; i < n; i++) {
            extra += escape_extra(PyUnicode_READ(kind, data, i), quote);
        }
    }

    if (extra == 0) {
        /* Nothing to escape.  Match the pure-Python escape(), which returns a
           true str (str.replace() normalises subclasses); for an exact str this
           just returns a new reference to the input. */
        return PyUnicode_FromObject(s);
    }

    Py_UCS4 maxchar = PyUnicode_MAX_CHAR_VALUE(s);
    PyObject *out = PyUnicode_New(n + extra, maxchar);
    if (out == NULL) {
        return NULL;
    }
    int okind = PyUnicode_KIND(out);
    void *odata = PyUnicode_DATA(out);
    Py_ssize_t o = 0;

    if (kind == PyUnicode_1BYTE_KIND && okind == PyUnicode_1BYTE_KIND) {
        const uint8_t *p = (const uint8_t *)data;
        uint8_t *q = (uint8_t *)odata;
        Py_ssize_t i = 0;
        while (i + 8 <= n) {
            uint64_t w;
            memcpy(&w, p + i, 8);
            if (swar_specials(w, quote) == 0) {
                memcpy(q + o, p + i, 8);
                o += 8;
                i += 8;
                continue;
            }
            for (int j = 0; j < 8; j++) {
                o += write_escaped(okind, odata, o, p[i + j], quote);
            }
            i += 8;
        }
        for (; i < n; i++) {
            o += write_escaped(okind, odata, o, p[i], quote);
        }
    }
    else {
        for (Py_ssize_t i = 0; i < n; i++) {
            o += write_escaped(okind, odata, o, PyUnicode_READ(kind, data, i), quote);
        }
    }
    return out;
}

static int
cmp_name(const char *a, Py_ssize_t alen, const char *b, unsigned blen)
{
    Py_ssize_t m = alen < (Py_ssize_t)blen ? alen : (Py_ssize_t)blen;
    int c = memcmp(a, b, (size_t)m);
    if (c != 0) {
        return c < 0 ? -1 : 1;
    }
    if (alen == (Py_ssize_t)blen) {
        return 0;
    }
    return alen < (Py_ssize_t)blen ? -1 : 1;
}

static const html5_entity *
find_entity(const char *name, Py_ssize_t len)
{
    int lo = 0, hi = html5_count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) >> 1;
        const html5_entity *e = &html5_entities[mid];
        int c = cmp_name(name, len, e->name, e->name_len);
        if (c == 0) {
            return e;
        }
        if (c < 0) {
            hi = mid - 1;
        }
        else {
            lo = mid + 1;
        }
    }
    return NULL;
}

static int
find_invalid_charref(Py_UCS4 num, Py_UCS4 *cp)
{
    int lo = 0, hi = invalid_charref_count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) >> 1;
        Py_UCS4 v = invalid_charrefs[mid].num;
        if (v == num) {
            *cp = invalid_charrefs[mid].cp;
            return 1;
        }
        if (num < v) {
            hi = mid - 1;
        }
        else {
            lo = mid + 1;
        }
    }
    return 0;
}

static int
is_invalid_codepoint(Py_UCS4 num)
{
    int lo = 0, hi = invalid_codepoint_count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) >> 1;
        Py_UCS4 v = invalid_codepoints[mid];
        if (v == num) {
            return 1;
        }
        if (num < v) {
            hi = mid - 1;
        }
        else {
            lo = mid + 1;
        }
    }
    return 0;
}

static inline int
is_name_char(Py_UCS4 c)
{
    /* [^\t\n\f <&#;] from the _charref regex in Lib/html/__init__.py. */
    switch (c) {
        case '\t': case '\n': case '\x0c': case ' ':
        case '<': case '&': case '#': case ';':
            return 0;
        default:
            return 1;
    }
}

static inline int
hex_value(Py_UCS4 c)
{
    if (c >= '0' && c <= '9') return (int)(c - '0');
    if (c >= 'a' && c <= 'f') return (int)(c - 'a') + 10;
    if (c >= 'A' && c <= 'F') return (int)(c - 'A') + 10;
    return -1;
}

/* Parse a character reference that starts with '&' at index i.  On a match,
   write the replacement to the writer, set *consumed to the number of input
   characters used (including '&'), and return 1.  Return 0 when no reference
   matches (the caller emits '&' literally) and -1 on a writer error. */
static int
parse_charref(int kind, const void *data, Py_ssize_t n, Py_ssize_t i,
              PyUnicodeWriter *writer, Py_ssize_t *consumed)
{
    Py_ssize_t p = i + 1;
    if (p >= n) {
        return 0;
    }
    Py_UCS4 c = PyUnicode_READ(kind, data, p);

    if (c == '#') {
        Py_ssize_t d = p + 1;
        int hex = 0;
        if (d < n) {
            Py_UCS4 x = PyUnicode_READ(kind, data, d);
            if (x == 'x' || x == 'X') {
                hex = 1;
                d++;
            }
        }
        Py_UCS4 num = 0;
        int overflow = 0;
        Py_ssize_t start = d;
        while (d < n) {
            Py_UCS4 x = PyUnicode_READ(kind, data, d);
            if (hex) {
                int v = hex_value(x);
                if (v < 0) {
                    break;
                }
                num = num * 16 + (Py_UCS4)v;
            }
            else {
                if (x < '0' || x > '9') {
                    break;
                }
                num = num * 10 + (x - '0');
            }
            if (num > 0x110000) {
                num = 0x110000;  /* cap to trigger the > 0x10FFFF branch below */
                overflow = 1;
            }
            d++;
        }
        if (d == start) {
            return 0;  /* no digits: the regex does not match */
        }
        if (d < n && PyUnicode_READ(kind, data, d) == ';') {
            d++;  /* optional trailing ';' */
        }

        Py_UCS4 repl;
        if (!overflow && find_invalid_charref(num, &repl)) {
            if (PyUnicodeWriter_WriteChar(writer, repl) < 0) {
                return -1;
            }
        }
        else if ((num >= 0xD800 && num <= 0xDFFF) || num > 0x10FFFF) {
            if (PyUnicodeWriter_WriteChar(writer, 0xFFFD) < 0) {
                return -1;
            }
        }
        else if (is_invalid_codepoint(num)) {
            /* maps to the empty string */
        }
        else if (PyUnicodeWriter_WriteChar(writer, num) < 0) {
            return -1;
        }
        *consumed = d - i;
        return 1;
    }

    if (!is_name_char(c)) {
        return 0;  /* e.g. "&;", "& ", "&&": the regex does not match */
    }

    /* Named reference: read up to 32 name characters, then an optional ';'. */
    Py_UCS4 ucs[HTML5_MAX_NAME_LEN];
    char ascii[HTML5_MAX_NAME_LEN + 1];
    int nlen = 0;
    Py_ssize_t d = p;
    while (d < n && nlen < HTML5_MAX_NAME_LEN) {
        Py_UCS4 x = PyUnicode_READ(kind, data, d);
        if (!is_name_char(x)) {
            break;
        }
        ucs[nlen] = x;
        ascii[nlen] = (x < 128) ? (char)x : (char)0x01;  /* 0x01 never matches */
        nlen++;
        d++;
    }
    int semi = 0;
    if (d < n && PyUnicode_READ(kind, data, d) == ';') {
        ascii[nlen] = ';';
        semi = 1;
        d++;
    }
    int toklen = nlen + semi;

    /* Whole token first, then shorter prefixes (HTML5 longest match). */
    const html5_entity *e = find_entity(ascii, toklen);
    int matchlen = toklen;
    if (e == NULL) {
        for (int x = toklen - 1; x >= 2; x--) {
            e = find_entity(ascii, x);
            if (e != NULL) {
                matchlen = x;
                break;
            }
        }
    }

    if (e == NULL) {
        /* No match: the callback returns '&' + group, i.e. unchanged. */
        if (PyUnicodeWriter_WriteChar(writer, '&') < 0) {
            return -1;
        }
        for (int k = 0; k < nlen; k++) {
            if (PyUnicodeWriter_WriteChar(writer, ucs[k]) < 0) {
                return -1;
            }
        }
        if (semi && PyUnicodeWriter_WriteChar(writer, ';') < 0) {
            return -1;
        }
        *consumed = d - i;
        return 1;
    }

    if (PyUnicodeWriter_WriteChar(writer, e->cp0) < 0) {
        return -1;
    }
    if (e->cp1 && PyUnicodeWriter_WriteChar(writer, e->cp1) < 0) {
        return -1;
    }
    /* Emit the unmatched tail of the token verbatim. */
    for (int k = matchlen; k < toklen; k++) {
        Py_UCS4 ch = (k < nlen) ? ucs[k] : (Py_UCS4)';';
        if (PyUnicodeWriter_WriteChar(writer, ch) < 0) {
            return -1;
        }
    }
    *consumed = d - i;
    return 1;
}

/*[clinic input]
_html.unescape

    s: unicode
    /

Convert named and numeric character references to Unicode characters.

This function uses the rules defined by the HTML 5 standard for both
valid and invalid character references, and the list of HTML 5 named
character references defined in html.entities.html5.
[clinic start generated code]*/

static PyObject *
_html_unescape_impl(PyObject *module, PyObject *s)
/*[clinic end generated code: output=36781d63ddc15dd9 input=8a45dd7fcf275d12]*/
{
    Py_ssize_t n = PyUnicode_GET_LENGTH(s);
    int kind = PyUnicode_KIND(s);
    const void *data = PyUnicode_DATA(s);

    if (PyUnicode_FindChar(s, '&', 0, n, 1) < 0) {
        return Py_NewRef(s);
    }

    PyUnicodeWriter *writer = PyUnicodeWriter_Create(n);
    if (writer == NULL) {
        return NULL;
    }

    Py_ssize_t i = 0;
    while (i < n) {
        /* Bulk-copy the run of non-'&' text, then handle the reference. */
        Py_ssize_t j = PyUnicode_FindChar(s, '&', i, n, 1);
        if (j < 0) {
            if (PyUnicodeWriter_WriteSubstring(writer, s, i, n) < 0) {
                goto error;
            }
            break;
        }
        if (j > i && PyUnicodeWriter_WriteSubstring(writer, s, i, j) < 0) {
            goto error;
        }
        Py_ssize_t consumed;
        int r = parse_charref(kind, data, n, j, writer, &consumed);
        if (r < 0) {
            goto error;
        }
        if (r == 0) {
            if (PyUnicodeWriter_WriteChar(writer, '&') < 0) {
                goto error;
            }
            i = j + 1;
        }
        else {
            i = j + consumed;
        }
    }
    return PyUnicodeWriter_Finish(writer);

error:
    PyUnicodeWriter_Discard(writer);
    return NULL;
}

static PyMethodDef html_methods[] = {
    _HTML_ESCAPE_METHODDEF
    _HTML_UNESCAPE_METHODDEF
    {NULL, NULL}
};

static struct PyModuleDef_Slot html_slots[] = {
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL},
};

static struct PyModuleDef htmlmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_html",
    .m_doc = "C accelerator for the html module.",
    .m_size = 0,
    .m_methods = html_methods,
    .m_slots = html_slots,
};

PyMODINIT_FUNC
PyInit__html(void)
{
    return PyModuleDef_Init(&htmlmodule);
}
