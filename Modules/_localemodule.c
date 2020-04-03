/***********************************************************
Copyright (C) 1997, 2002, 2003, 2007, 2008 Martin von Loewis

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies.

This software comes with no warranty. Use at your own risk.

******************************************************************/

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "pycore_fileutils.h"

#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_LANGINFO_H
#include <langinfo.h>
#endif

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#endif

#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif

#if defined(MS_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

PyDoc_STRVAR(locale__doc__, "Support for POSIX locales.");

typedef struct _locale_state {
    PyObject *Error;
} _locale_state;

static inline _locale_state*
get_locale_state(PyObject *m)
{
    void *state = PyModule_GetState(m);
    assert(state != NULL);
    return (_locale_state *)state;
}

/* support functions for formatting floating point numbers */

PyDoc_STRVAR(setlocale__doc__,
"(integer,string=None) -> string. Activates/queries locale processing.");

/* the grouping is terminated by either 0 or CHAR_MAX */
static PyObject*
copy_grouping(const char* s)
{
    int i;
    PyObject *result, *val = NULL;

    if (s[0] == '\0') {
        /* empty string: no grouping at all */
        return PyList_New(0);
    }

    for (i = 0; s[i] != '\0' && s[i] != CHAR_MAX; i++)
        ; /* nothing */

    result = PyList_New(i+1);
    if (!result)
        return NULL;

    i = -1;
    do {
        i++;
        val = PyLong_FromLong(s[i]);
        if (val == NULL) {
            Py_DECREF(result);
            return NULL;
        }
        PyList_SET_ITEM(result, i, val);
    } while (s[i] != '\0' && s[i] != CHAR_MAX);

    return result;
}

static PyObject*
PyLocale_setlocale(PyObject* self, PyObject* args)
{
    int category;
    char *locale = NULL, *result;
    PyObject *result_object;

    if (!PyArg_ParseTuple(args, "i|z:setlocale", &category, &locale))
        return NULL;

#if defined(MS_WINDOWS)
    if (category < LC_MIN || category > LC_MAX)
    {
        PyErr_SetString(get_locale_state(self)->Error,
                        "invalid locale category");
        return NULL;
    }
#endif

    if (locale) {
        /* set locale */
        result = setlocale(category, locale);
        if (!result) {
            /* operation failed, no setting was changed */
            PyErr_SetString(get_locale_state(self)->Error,
                            "unsupported locale setting");
            return NULL;
        }
        result_object = PyUnicode_DecodeLocale(result, NULL);
        if (!result_object)
            return NULL;
    } else {
        /* get locale */
        result = setlocale(category, NULL);
        if (!result) {
            PyErr_SetString(get_locale_state(self)->Error,
                            "locale query failed");
            return NULL;
        }
        result_object = PyUnicode_DecodeLocale(result, NULL);
    }
    return result_object;
}

static int
locale_is_ascii(const char *str)
{
    return (strlen(str) == 1 && ((unsigned char)str[0]) <= 127);
}

static int
locale_decode_monetary(PyObject *dict, struct lconv *lc)
{
    int change_locale;
    change_locale = (!locale_is_ascii(lc->int_curr_symbol)
                     || !locale_is_ascii(lc->currency_symbol)
                     || !locale_is_ascii(lc->mon_decimal_point)
                     || !locale_is_ascii(lc->mon_thousands_sep));

    /* Keep a copy of the LC_CTYPE locale */
    char *oldloc = NULL, *loc = NULL;
    if (change_locale) {
        oldloc = setlocale(LC_CTYPE, NULL);
        if (!oldloc) {
            PyErr_SetString(PyExc_RuntimeWarning,
                            "failed to get LC_CTYPE locale");
            return -1;
        }

        oldloc = _PyMem_Strdup(oldloc);
        if (!oldloc) {
            PyErr_NoMemory();
            return -1;
        }

        loc = setlocale(LC_MONETARY, NULL);
        if (loc != NULL && strcmp(loc, oldloc) == 0) {
            loc = NULL;
        }

        if (loc != NULL) {
            /* Only set the locale temporarily the LC_CTYPE locale
               to the LC_MONETARY locale if the two locales are different and
               at least one string is non-ASCII. */
            setlocale(LC_CTYPE, loc);
        }
    }

    int res = -1;

#define RESULT_STRING(ATTR) \
    do { \
        PyObject *obj; \
        obj = PyUnicode_DecodeLocale(lc->ATTR, NULL); \
        if (obj == NULL) { \
            goto done; \
        } \
        if (PyDict_SetItemString(dict, Py_STRINGIFY(ATTR), obj) < 0) { \
            Py_DECREF(obj); \
            goto done; \
        } \
        Py_DECREF(obj); \
    } while (0)

    RESULT_STRING(int_curr_symbol);
    RESULT_STRING(currency_symbol);
    RESULT_STRING(mon_decimal_point);
    RESULT_STRING(mon_thousands_sep);
#undef RESULT_STRING

    res = 0;

done:
    if (loc != NULL) {
        setlocale(LC_CTYPE, oldloc);
    }
    PyMem_Free(oldloc);
    return res;
}

PyDoc_STRVAR(localeconv__doc__,
"() -> dict. Returns numeric and monetary locale-specific parameters.");

static PyObject*
PyLocale_localeconv(PyObject* self, PyObject *Py_UNUSED(ignored))
{
    PyObject* result;
    struct lconv *lc;
    PyObject *x;

    result = PyDict_New();
    if (!result) {
        return NULL;
    }

    /* if LC_NUMERIC is different in the C library, use saved value */
    lc = localeconv();

    /* hopefully, the localeconv result survives the C library calls
       involved herein */

#define RESULT(key, obj)\
    do { \
        if (obj == NULL) \
            goto failed; \
        if (PyDict_SetItemString(result, key, obj) < 0) { \
            Py_DECREF(obj); \
            goto failed; \
        } \
        Py_DECREF(obj); \
    } while (0)

#define RESULT_STRING(s)\
    do { \
        x = PyUnicode_DecodeLocale(lc->s, NULL); \
        RESULT(#s, x); \
    } while (0)

#define RESULT_INT(i)\
    do { \
        x = PyLong_FromLong(lc->i); \
        RESULT(#i, x); \
    } while (0)

    /* Monetary information: LC_MONETARY encoding */
    if (locale_decode_monetary(result, lc) < 0) {
        goto failed;
    }
    x = copy_grouping(lc->mon_grouping);
    RESULT("mon_grouping", x);

    RESULT_STRING(positive_sign);
    RESULT_STRING(negative_sign);
    RESULT_INT(int_frac_digits);
    RESULT_INT(frac_digits);
    RESULT_INT(p_cs_precedes);
    RESULT_INT(p_sep_by_space);
    RESULT_INT(n_cs_precedes);
    RESULT_INT(n_sep_by_space);
    RESULT_INT(p_sign_posn);
    RESULT_INT(n_sign_posn);

    /* Numeric information: LC_NUMERIC encoding */
    PyObject *decimal_point, *thousands_sep;
    if (_Py_GetLocaleconvNumeric(lc, &decimal_point, &thousands_sep) < 0) {
        goto failed;
    }

    if (PyDict_SetItemString(result, "decimal_point", decimal_point) < 0) {
        Py_DECREF(decimal_point);
        Py_DECREF(thousands_sep);
        goto failed;
    }
    Py_DECREF(decimal_point);

    if (PyDict_SetItemString(result, "thousands_sep", thousands_sep) < 0) {
        Py_DECREF(thousands_sep);
        goto failed;
    }
    Py_DECREF(thousands_sep);

    x = copy_grouping(lc->grouping);
    RESULT("grouping", x);

    return result;

  failed:
    Py_DECREF(result);
    return NULL;

#undef RESULT
#undef RESULT_STRING
#undef RESULT_INT
}

#if defined(HAVE_WCSCOLL)
PyDoc_STRVAR(strcoll__doc__,
"string,string -> int. Compares two strings according to the locale.");

static PyObject*
PyLocale_strcoll(PyObject* self, PyObject* args)
{
    PyObject *os1, *os2, *result = NULL;
    wchar_t *ws1 = NULL, *ws2 = NULL;

    if (!PyArg_ParseTuple(args, "UU:strcoll", &os1, &os2))
        return NULL;
    /* Convert the unicode strings to wchar[]. */
    ws1 = PyUnicode_AsWideCharString(os1, NULL);
    if (ws1 == NULL)
        goto done;
    ws2 = PyUnicode_AsWideCharString(os2, NULL);
    if (ws2 == NULL)
        goto done;
    /* Collate the strings. */
    result = PyLong_FromLong(wcscoll(ws1, ws2));
  done:
    /* Deallocate everything. */
    if (ws1) PyMem_FREE(ws1);
    if (ws2) PyMem_FREE(ws2);
    return result;
}
#endif

#ifdef HAVE_WCSXFRM
PyDoc_STRVAR(strxfrm__doc__,
"strxfrm(string) -> string.\n\
\n\
Return a string that can be used as a key for locale-aware comparisons.");

static PyObject*
PyLocale_strxfrm(PyObject* self, PyObject* args)
{
    PyObject *str;
    Py_ssize_t n1;
    wchar_t *s = NULL, *buf = NULL;
    size_t n2;
    PyObject *result = NULL;

    if (!PyArg_ParseTuple(args, "U:strxfrm", &str))
        return NULL;

    s = PyUnicode_AsWideCharString(str, &n1);
    if (s == NULL)
        goto exit;
    if (wcslen(s) != (size_t)n1) {
        PyErr_SetString(PyExc_ValueError,
                        "embedded null character");
        goto exit;
    }

    /* assume no change in size, first */
    n1 = n1 + 1;
    buf = PyMem_New(wchar_t, n1);
    if (!buf) {
        PyErr_NoMemory();
        goto exit;
    }
    errno = 0;
    n2 = wcsxfrm(buf, s, n1);
    if (errno && errno != ERANGE) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto exit;
    }
    if (n2 >= (size_t)n1) {
        /* more space needed */
        wchar_t * new_buf = PyMem_Realloc(buf, (n2+1)*sizeof(wchar_t));
        if (!new_buf) {
            PyErr_NoMemory();
            goto exit;
        }
        buf = new_buf;
        errno = 0;
        n2 = wcsxfrm(buf, s, n2+1);
        if (errno) {
            PyErr_SetFromErrno(PyExc_OSError);
            goto exit;
        }
    }
    result = PyUnicode_FromWideChar(buf, n2);
exit:
    PyMem_Free(buf);
    PyMem_Free(s);
    return result;
}
#endif

#if defined(MS_WINDOWS)
static PyObject*
PyLocale_getdefaultlocale(PyObject* self, PyObject *Py_UNUSED(ignored))
{
    char encoding[20];
    char locale[100];

    PyOS_snprintf(encoding, sizeof(encoding), "cp%u", GetACP());

    if (GetLocaleInfo(LOCALE_USER_DEFAULT,
                      LOCALE_SISO639LANGNAME,
                      locale, sizeof(locale))) {
        Py_ssize_t i = strlen(locale);
        locale[i++] = '_';
        if (GetLocaleInfo(LOCALE_USER_DEFAULT,
                          LOCALE_SISO3166CTRYNAME,
                          locale+i, (int)(sizeof(locale)-i)))
            return Py_BuildValue("ss", locale, encoding);
    }

    /* If we end up here, this windows version didn't know about
       ISO639/ISO3166 names (it's probably Windows 95).  Return the
       Windows language identifier instead (a hexadecimal number) */

    locale[0] = '0';
    locale[1] = 'x';
    if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_IDEFAULTLANGUAGE,
                      locale+2, sizeof(locale)-2)) {
        return Py_BuildValue("ss", locale, encoding);
    }

    /* cannot determine the language code (very unlikely) */
    Py_INCREF(Py_None);
    return Py_BuildValue("Os", Py_None, encoding);
}
#endif

#ifdef HAVE_LANGINFO_H
#define LANGINFO(X) {#X, X}
static struct langinfo_constant{
    char* name;
    int value;
} langinfo_constants[] =
{
    /* These constants should exist on any langinfo implementation */
    LANGINFO(DAY_1),
    LANGINFO(DAY_2),
    LANGINFO(DAY_3),
    LANGINFO(DAY_4),
    LANGINFO(DAY_5),
    LANGINFO(DAY_6),
    LANGINFO(DAY_7),

    LANGINFO(ABDAY_1),
    LANGINFO(ABDAY_2),
    LANGINFO(ABDAY_3),
    LANGINFO(ABDAY_4),
    LANGINFO(ABDAY_5),
    LANGINFO(ABDAY_6),
    LANGINFO(ABDAY_7),

    LANGINFO(MON_1),
    LANGINFO(MON_2),
    LANGINFO(MON_3),
    LANGINFO(MON_4),
    LANGINFO(MON_5),
    LANGINFO(MON_6),
    LANGINFO(MON_7),
    LANGINFO(MON_8),
    LANGINFO(MON_9),
    LANGINFO(MON_10),
    LANGINFO(MON_11),
    LANGINFO(MON_12),

    LANGINFO(ABMON_1),
    LANGINFO(ABMON_2),
    LANGINFO(ABMON_3),
    LANGINFO(ABMON_4),
    LANGINFO(ABMON_5),
    LANGINFO(ABMON_6),
    LANGINFO(ABMON_7),
    LANGINFO(ABMON_8),
    LANGINFO(ABMON_9),
    LANGINFO(ABMON_10),
    LANGINFO(ABMON_11),
    LANGINFO(ABMON_12),

#ifdef RADIXCHAR
    /* The following are not available with glibc 2.0 */
    LANGINFO(RADIXCHAR),
    LANGINFO(THOUSEP),
    /* YESSTR and NOSTR are deprecated in glibc, since they are
       a special case of message translation, which should be rather
       done using gettext. So we don't expose it to Python in the
       first place.
    LANGINFO(YESSTR),
    LANGINFO(NOSTR),
    */
    LANGINFO(CRNCYSTR),
#endif

    LANGINFO(D_T_FMT),
    LANGINFO(D_FMT),
    LANGINFO(T_FMT),
    LANGINFO(AM_STR),
    LANGINFO(PM_STR),

    /* The following constants are available only with XPG4, but...
       AIX 3.2. only has CODESET.
       OpenBSD doesn't have CODESET but has T_FMT_AMPM, and doesn't have
       a few of the others.
       Solution: ifdef-test them all. */
#ifdef CODESET
    LANGINFO(CODESET),
#endif
#ifdef T_FMT_AMPM
    LANGINFO(T_FMT_AMPM),
#endif
#ifdef ERA
    LANGINFO(ERA),
#endif
#ifdef ERA_D_FMT
    LANGINFO(ERA_D_FMT),
#endif
#ifdef ERA_D_T_FMT
    LANGINFO(ERA_D_T_FMT),
#endif
#ifdef ERA_T_FMT
    LANGINFO(ERA_T_FMT),
#endif
#ifdef ALT_DIGITS
    LANGINFO(ALT_DIGITS),
#endif
#ifdef YESEXPR
    LANGINFO(YESEXPR),
#endif
#ifdef NOEXPR
    LANGINFO(NOEXPR),
#endif
#ifdef _DATE_FMT
    /* This is not available in all glibc versions that have CODESET. */
    LANGINFO(_DATE_FMT),
#endif
    {0, 0}
};

PyDoc_STRVAR(nl_langinfo__doc__,
"nl_langinfo(key) -> string\n"
"Return the value for the locale information associated with key.");

static PyObject*
PyLocale_nl_langinfo(PyObject* self, PyObject* args)
{
    int item, i;
    if (!PyArg_ParseTuple(args, "i:nl_langinfo", &item))
        return NULL;
    /* Check whether this is a supported constant. GNU libc sometimes
       returns numeric values in the char* return value, which would
       crash PyUnicode_FromString.  */
    for (i = 0; langinfo_constants[i].name; i++)
        if (langinfo_constants[i].value == item) {
            /* Check NULL as a workaround for GNU libc's returning NULL
               instead of an empty string for nl_langinfo(ERA).  */
            const char *result = nl_langinfo(item);
            result = result != NULL ? result : "";
            return PyUnicode_DecodeLocale(result, NULL);
        }
    PyErr_SetString(PyExc_ValueError, "unsupported langinfo constant");
    return NULL;
}
#endif /* HAVE_LANGINFO_H */

#ifdef HAVE_LIBINTL_H

PyDoc_STRVAR(gettext__doc__,
"gettext(msg) -> string\n"
"Return translation of msg.");

static PyObject*
PyIntl_gettext(PyObject* self, PyObject *args)
{
    char *in;
    if (!PyArg_ParseTuple(args, "s", &in))
        return 0;
    return PyUnicode_DecodeLocale(gettext(in), NULL);
}

PyDoc_STRVAR(dgettext__doc__,
"dgettext(domain, msg) -> string\n"
"Return translation of msg in domain.");

static PyObject*
PyIntl_dgettext(PyObject* self, PyObject *args)
{
    char *domain, *in;
    if (!PyArg_ParseTuple(args, "zs", &domain, &in))
        return 0;
    return PyUnicode_DecodeLocale(dgettext(domain, in), NULL);
}

PyDoc_STRVAR(dcgettext__doc__,
"dcgettext(domain, msg, category) -> string\n"
"Return translation of msg in domain and category.");

static PyObject*
PyIntl_dcgettext(PyObject *self, PyObject *args)
{
    char *domain, *msgid;
    int category;
    if (!PyArg_ParseTuple(args, "zsi", &domain, &msgid, &category))
        return 0;
    return PyUnicode_DecodeLocale(dcgettext(domain,msgid,category), NULL);
}

PyDoc_STRVAR(textdomain__doc__,
"textdomain(domain) -> string\n"
"Set the C library's textdmain to domain, returning the new domain.");

static PyObject*
PyIntl_textdomain(PyObject* self, PyObject* args)
{
    char *domain;
    if (!PyArg_ParseTuple(args, "z", &domain))
        return 0;
    domain = textdomain(domain);
    if (!domain) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    return PyUnicode_DecodeLocale(domain, NULL);
}

PyDoc_STRVAR(bindtextdomain__doc__,
"bindtextdomain(domain, dir) -> string\n"
"Bind the C library's domain to dir.");

static PyObject*
PyIntl_bindtextdomain(PyObject* self, PyObject*args)
{
    char *domain, *dirname, *current_dirname;
    PyObject *dirname_obj, *dirname_bytes = NULL, *result;

    if (!PyArg_ParseTuple(args, "sO", &domain, &dirname_obj))
        return 0;
    if (!strlen(domain)) {
        PyErr_SetString(get_locale_state(self)->Error,
                        "domain must be a non-empty string");
        return 0;
    }
    if (dirname_obj != Py_None) {
        if (!PyUnicode_FSConverter(dirname_obj, &dirname_bytes))
            return NULL;
        dirname = PyBytes_AsString(dirname_bytes);
    } else {
        dirname_bytes = NULL;
        dirname = NULL;
    }
    current_dirname = bindtextdomain(domain, dirname);
    if (current_dirname == NULL) {
        Py_XDECREF(dirname_bytes);
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    result = PyUnicode_DecodeLocale(current_dirname, NULL);
    Py_XDECREF(dirname_bytes);
    return result;
}

#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
PyDoc_STRVAR(bind_textdomain_codeset__doc__,
"bind_textdomain_codeset(domain, codeset) -> string\n"
"Bind the C library's domain to codeset.");

static PyObject*
PyIntl_bind_textdomain_codeset(PyObject* self,PyObject*args)
{
    char *domain,*codeset;
    if (!PyArg_ParseTuple(args, "sz", &domain, &codeset))
        return NULL;
    codeset = bind_textdomain_codeset(domain, codeset);
    if (codeset) {
        return PyUnicode_DecodeLocale(codeset, NULL);
    }
    Py_RETURN_NONE;
}
#endif

#endif

static struct PyMethodDef PyLocale_Methods[] = {
  {"setlocale", (PyCFunction) PyLocale_setlocale,
   METH_VARARGS, setlocale__doc__},
  {"localeconv", PyLocale_localeconv, METH_NOARGS, localeconv__doc__},
#ifdef HAVE_WCSCOLL
  {"strcoll", (PyCFunction) PyLocale_strcoll,
   METH_VARARGS, strcoll__doc__},
#endif
#ifdef HAVE_WCSXFRM
  {"strxfrm", (PyCFunction) PyLocale_strxfrm,
   METH_VARARGS, strxfrm__doc__},
#endif
#if defined(MS_WINDOWS)
  {"_getdefaultlocale", PyLocale_getdefaultlocale, METH_NOARGS},
#endif
#ifdef HAVE_LANGINFO_H
  {"nl_langinfo", (PyCFunction) PyLocale_nl_langinfo,
   METH_VARARGS, nl_langinfo__doc__},
#endif
#ifdef HAVE_LIBINTL_H
  {"gettext",(PyCFunction)PyIntl_gettext,METH_VARARGS,
    gettext__doc__},
  {"dgettext",(PyCFunction)PyIntl_dgettext,METH_VARARGS,
   dgettext__doc__},
  {"dcgettext",(PyCFunction)PyIntl_dcgettext,METH_VARARGS,
    dcgettext__doc__},
  {"textdomain",(PyCFunction)PyIntl_textdomain,METH_VARARGS,
   textdomain__doc__},
  {"bindtextdomain",(PyCFunction)PyIntl_bindtextdomain,METH_VARARGS,
   bindtextdomain__doc__},
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  {"bind_textdomain_codeset",(PyCFunction)PyIntl_bind_textdomain_codeset,
   METH_VARARGS, bind_textdomain_codeset__doc__},
#endif
#endif
  {NULL, NULL}
};

static int
_locale_exec(PyObject *module)
{
#ifdef HAVE_LANGINFO_H
    int i;
#endif
#define ADD_INT(module, value)                                    \
    do {                                                          \
        if (PyModule_AddIntConstant(module, #value, value) < 0) { \
            return -1;                                            \
        }                                                         \
    } while (0)

    ADD_INT(module, LC_CTYPE);
    ADD_INT(module, LC_TIME);
    ADD_INT(module, LC_COLLATE);
    ADD_INT(module, LC_MONETARY);

#ifdef LC_MESSAGES
    ADD_INT(module, LC_MESSAGES);
#endif /* LC_MESSAGES */

    ADD_INT(module, LC_NUMERIC);
    ADD_INT(module, LC_ALL);
    ADD_INT(module, CHAR_MAX);

    _locale_state *state = get_locale_state(module);
    state->Error = PyErr_NewException("locale.Error", NULL, NULL);
    if (state->Error == NULL) {
        return -1;
    }
    Py_INCREF(get_locale_state(module)->Error);
    if (PyModule_AddObject(module, "Error", get_locale_state(module)->Error) < 0) {
        Py_DECREF(get_locale_state(module)->Error);
        return -1;
    }

#ifdef HAVE_LANGINFO_H
    for (i = 0; langinfo_constants[i].name; i++) {
        if (PyModule_AddIntConstant(module,
                                    langinfo_constants[i].name,
                                    langinfo_constants[i].value) < 0) {
            return -1;
        }
    }
#endif

    if (PyErr_Occurred()) {
        return -1;
    }
    return 0;

#undef ADD_INT
}

static struct PyModuleDef_Slot _locale_slots[] = {
    {Py_mod_exec, _locale_exec},
    {0, NULL}
};

static int
locale_traverse(PyObject *module, visitproc visit, void *arg)
{
    _locale_state *state = get_locale_state(module);
    Py_VISIT(state->Error);
    return 0;
}

static int
locale_clear(PyObject *module)
{
    _locale_state *state = get_locale_state(module);
    Py_CLEAR(state->Error);
    return 0;
}

static void
locale_free(PyObject *module)
{
    locale_clear(module);
}

static struct PyModuleDef _localemodule = {
    PyModuleDef_HEAD_INIT,
    "_locale",
    locale__doc__,
    sizeof(_locale_state),
    PyLocale_Methods,
    _locale_slots,
    locale_traverse,
    locale_clear,
    (freefunc)locale_free,
};

PyMODINIT_FUNC
PyInit__locale(void)
{
    return PyModuleDef_Init(&_localemodule);
}

/*
Local variables:
c-basic-offset: 4
indent-tabs-mode: nil
End:
*/
