/***********************************************************
Copyright (C) 1997, 2002, 2003, 2007, 2008 Martin von Loewis

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies.

This software comes with no warranty. Use at your own risk.

******************************************************************/

#include "Python.h"

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

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif

#if defined(MS_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

PyDoc_STRVAR(locale__doc__, "Support for POSIX locales.");

static PyObject *Error;

/* Convert a char* to a Unicode object according to the current locale */
static PyObject*
str2uni(const char* s)
{
    size_t needed = mbstowcs(NULL, s, 0);
    size_t res1;
    wchar_t smallbuf[30];
    wchar_t *dest;
    PyObject *res2;
    if (needed == (size_t)-1) {
        PyErr_SetString(PyExc_ValueError, "Cannot convert byte to string");
        return NULL;
    }
    if (needed < sizeof(smallbuf))
        dest = smallbuf;
    else {
        dest = PyMem_Malloc(needed+1);
        if (!dest)
            return PyErr_NoMemory();
    }
    /* This shouldn't fail now */
    res1 = mbstowcs(dest, s, needed+1);
    assert(res == needed);
    res2 = PyUnicode_FromWideChar(dest, res1);
    if (dest != smallbuf)
        PyMem_Free(dest);
    return res2;
}        

/* support functions for formatting floating point numbers */

PyDoc_STRVAR(setlocale__doc__,
"(integer,string=None) -> string. Activates/queries locale processing.");

/* the grouping is terminated by either 0 or CHAR_MAX */
static PyObject*
copy_grouping(char* s)
{
    int i;
    PyObject *result, *val = NULL;

    if (s[0] == '\0')
        /* empty string: no grouping at all */
        return PyList_New(0);

    for (i = 0; s[i] != '\0' && s[i] != CHAR_MAX; i++)
        ; /* nothing */

    result = PyList_New(i+1);
    if (!result)
        return NULL;

    i = -1;
    do {
        i++;
        val = PyLong_FromLong(s[i]);
        if (!val)
            break;
        if (PyList_SetItem(result, i, val)) {
            Py_DECREF(val);
            val = NULL;
            break;
        }
    } while (s[i] != '\0' && s[i] != CHAR_MAX);

    if (!val) {
        Py_DECREF(result);
        return NULL;
    }

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

    if (locale) {
        /* set locale */
        result = setlocale(category, locale);
        if (!result) {
            /* operation failed, no setting was changed */
            PyErr_SetString(Error, "unsupported locale setting");
            return NULL;
        }
        result_object = str2uni(result);
        if (!result_object)
            return NULL;
    } else {
        /* get locale */
        result = setlocale(category, NULL);
        if (!result) {
            PyErr_SetString(Error, "locale query failed");
            return NULL;
        }
        result_object = str2uni(result);
    }
    return result_object;
}

PyDoc_STRVAR(localeconv__doc__,
"() -> dict. Returns numeric and monetary locale-specific parameters.");

static PyObject*
PyLocale_localeconv(PyObject* self)
{
    PyObject* result;
    struct lconv *l;
    PyObject *x;

    result = PyDict_New();
    if (!result)
        return NULL;

    /* if LC_NUMERIC is different in the C library, use saved value */
    l = localeconv();

    /* hopefully, the localeconv result survives the C library calls
       involved herein */

#define RESULT_STRING(s)\
    x = str2uni(l->s);   \
    if (!x) goto failed;\
    PyDict_SetItemString(result, #s, x);\
    Py_XDECREF(x)

#define RESULT_INT(i)\
    x = PyLong_FromLong(l->i);\
    if (!x) goto failed;\
    PyDict_SetItemString(result, #i, x);\
    Py_XDECREF(x)

    /* Numeric information */
    RESULT_STRING(decimal_point);
    RESULT_STRING(thousands_sep);
    x = copy_grouping(l->grouping);
    if (!x)
        goto failed;
    PyDict_SetItemString(result, "grouping", x);
    Py_XDECREF(x);

    /* Monetary information */
    RESULT_STRING(int_curr_symbol);
    RESULT_STRING(currency_symbol);
    RESULT_STRING(mon_decimal_point);
    RESULT_STRING(mon_thousands_sep);
    x = copy_grouping(l->mon_grouping);
    if (!x)
        goto failed;
    PyDict_SetItemString(result, "mon_grouping", x);
    Py_XDECREF(x);
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
    return result;

  failed:
    Py_XDECREF(result);
    Py_XDECREF(x);
    return NULL;
}

#if defined(HAVE_WCSCOLL)
PyDoc_STRVAR(strcoll__doc__,
"string,string -> int. Compares two strings according to the locale.");

static PyObject*
PyLocale_strcoll(PyObject* self, PyObject* args)
{
    PyObject *os1, *os2, *result = NULL;
    wchar_t *ws1 = NULL, *ws2 = NULL;
    Py_ssize_t len1, len2;
    
    if (!PyArg_ParseTuple(args, "UU:strcoll", &os1, &os2))
        return NULL;
    /* Convert the unicode strings to wchar[]. */
    len1 = PyUnicode_GET_SIZE(os1) + 1;
    ws1 = PyMem_MALLOC(len1 * sizeof(wchar_t));
    if (!ws1) {
        PyErr_NoMemory();
        goto done;
    }
    if (PyUnicode_AsWideChar((PyUnicodeObject*)os1, ws1, len1) == -1)
        goto done;
    ws1[len1 - 1] = 0;
    len2 = PyUnicode_GET_SIZE(os2) + 1;
    ws2 = PyMem_MALLOC(len2 * sizeof(wchar_t));
    if (!ws2) {
        PyErr_NoMemory();
        goto done;
    }
    if (PyUnicode_AsWideChar((PyUnicodeObject*)os2, ws2, len2) == -1)
        goto done;
    ws2[len2 - 1] = 0;
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
"string -> string. Returns a string that behaves for cmp locale-aware.");

static PyObject*
PyLocale_strxfrm(PyObject* self, PyObject* args)
{
    Py_UNICODE *s0;
    Py_ssize_t n0;
    wchar_t *s, *buf = NULL;
    size_t n1, n2;
    PyObject *result = NULL;
    Py_ssize_t i;

    if (!PyArg_ParseTuple(args, "u#:strxfrm", &s0, &n0))
        return NULL;

#ifdef HAVE_USABLE_WCHAR_T
    s = s0;
#else
    s = PyMem_Malloc(n0+1);
    if (!s)
        return PyErr_NoMemory();
    for (i=0; i<=n0; i++)
        s[i] = s0[i];
#endif

    /* assume no change in size, first */
    n1 = wcslen(s) + 1;
    buf = PyMem_Malloc(n1);
    if (!buf) {
        PyErr_NoMemory();
        goto exit;
    }
    n2 = wcsxfrm(buf, s, n1);
    if (n2 >= n1) {
        /* more space needed */
        buf = PyMem_Realloc(buf, n2+1);
        if (!buf) {
            PyErr_NoMemory();
            goto exit;
        }
        n2 = wcsxfrm(buf, s, n2);
    }
    result = PyUnicode_FromWideChar(buf, n2);
 exit:
    if (buf) PyMem_Free(buf);
#ifdef HAVE_USABLE_WCHAR_T
    PyMem_Free(s);
#endif
    return result;
}
#endif

#if defined(MS_WINDOWS)
static PyObject*
PyLocale_getdefaultlocale(PyObject* self)
{
    char encoding[100];
    char locale[100];

    PyOS_snprintf(encoding, sizeof(encoding), "cp%d", GetACP());

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

#if defined(__APPLE__)
/*
** Find out what the current script is.
** Donated by Fredrik Lundh.
*/
static char *mac_getscript(void)
{
    CFStringEncoding enc = CFStringGetSystemEncoding();
    static CFStringRef name = NULL;
    /* Return the code name for the encodings for which we have codecs. */
    switch(enc) {
    case kCFStringEncodingMacRoman: return "mac-roman";
    case kCFStringEncodingMacGreek: return "mac-greek";
    case kCFStringEncodingMacCyrillic: return "mac-cyrillic";
    case kCFStringEncodingMacTurkish: return "mac-turkish";
    case kCFStringEncodingMacIcelandic: return "mac-icelandic";
    /* XXX which one is mac-latin2? */
    }
    if (!name) {
        /* This leaks an object. */
        name = CFStringConvertEncodingToIANACharSetName(enc);
    }
    return (char *)CFStringGetCStringPtr(name, 0); 
}

static PyObject*
PyLocale_getdefaultlocale(PyObject* self)
{
    return Py_BuildValue("Os", Py_None, mac_getscript());
}
#endif

#ifdef HAVE_LANGINFO_H
#define LANGINFO(X) {#X, X}
struct langinfo_constant{
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
            return str2uni(result);
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
	if (!PyArg_ParseTuple(args, "z", &in))
		return 0;
	return str2uni(gettext(in));
}

PyDoc_STRVAR(dgettext__doc__,
"dgettext(domain, msg) -> string\n"
"Return translation of msg in domain.");

static PyObject*
PyIntl_dgettext(PyObject* self, PyObject *args)
{
	char *domain, *in;
	if (!PyArg_ParseTuple(args, "zz", &domain, &in))
		return 0;
	return str2uni(dgettext(domain, in));
}

PyDoc_STRVAR(dcgettext__doc__,
"dcgettext(domain, msg, category) -> string\n"
"Return translation of msg in domain and category.");

static PyObject*
PyIntl_dcgettext(PyObject *self, PyObject *args)
{
	char *domain, *msgid;
	int category;
	if (!PyArg_ParseTuple(args, "zzi", &domain, &msgid, &category))
		return 0;
	return str2uni(dcgettext(domain,msgid,category));
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
	return str2uni(domain);
}

PyDoc_STRVAR(bindtextdomain__doc__,
"bindtextdomain(domain, dir) -> string\n"
"Bind the C library's domain to dir.");

static PyObject*
PyIntl_bindtextdomain(PyObject* self,PyObject*args)
{
	char *domain,*dirname;
	if (!PyArg_ParseTuple(args, "zz", &domain, &dirname))
		return 0;
	dirname = bindtextdomain(domain, dirname);
	if (!dirname) {
		PyErr_SetFromErrno(PyExc_OSError);
		return NULL;
	}
	return str2uni(dirname);
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
	if (codeset)
		return str2uni(codeset);
	Py_RETURN_NONE;
}
#endif

#endif

static struct PyMethodDef PyLocale_Methods[] = {
  {"setlocale", (PyCFunction) PyLocale_setlocale, 
   METH_VARARGS, setlocale__doc__},
  {"localeconv", (PyCFunction) PyLocale_localeconv, 
   METH_NOARGS, localeconv__doc__},
#ifdef HAVE_WCSCOLL
  {"strcoll", (PyCFunction) PyLocale_strcoll, 
   METH_VARARGS, strcoll__doc__},
#endif
#ifdef HAVE_WCSXFRM
  {"strxfrm", (PyCFunction) PyLocale_strxfrm, 
   METH_VARARGS, strxfrm__doc__},
#endif
#if defined(MS_WINDOWS) || defined(__APPLE__)
  {"_getdefaultlocale", (PyCFunction) PyLocale_getdefaultlocale, METH_NOARGS},
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

PyMODINIT_FUNC
init_locale(void)
{
    PyObject *m, *d, *x;
#ifdef HAVE_LANGINFO_H
    int i;
#endif

    m = Py_InitModule3("_locale", PyLocale_Methods, locale__doc__);
    if (m == NULL)
    	return;

    d = PyModule_GetDict(m);

    x = PyLong_FromLong(LC_CTYPE);
    PyDict_SetItemString(d, "LC_CTYPE", x);
    Py_XDECREF(x);

    x = PyLong_FromLong(LC_TIME);
    PyDict_SetItemString(d, "LC_TIME", x);
    Py_XDECREF(x);

    x = PyLong_FromLong(LC_COLLATE);
    PyDict_SetItemString(d, "LC_COLLATE", x);
    Py_XDECREF(x);

    x = PyLong_FromLong(LC_MONETARY);
    PyDict_SetItemString(d, "LC_MONETARY", x);
    Py_XDECREF(x);

#ifdef LC_MESSAGES
    x = PyLong_FromLong(LC_MESSAGES);
    PyDict_SetItemString(d, "LC_MESSAGES", x);
    Py_XDECREF(x);
#endif /* LC_MESSAGES */

    x = PyLong_FromLong(LC_NUMERIC);
    PyDict_SetItemString(d, "LC_NUMERIC", x);
    Py_XDECREF(x);

    x = PyLong_FromLong(LC_ALL);
    PyDict_SetItemString(d, "LC_ALL", x);
    Py_XDECREF(x);

    x = PyLong_FromLong(CHAR_MAX);
    PyDict_SetItemString(d, "CHAR_MAX", x);
    Py_XDECREF(x);

    Error = PyErr_NewException("locale.Error", NULL, NULL);
    PyDict_SetItemString(d, "Error", Error);

#ifdef HAVE_LANGINFO_H
    for (i = 0; langinfo_constants[i].name; i++) {
	    PyModule_AddIntConstant(m, langinfo_constants[i].name,
				    langinfo_constants[i].value);
    }
#endif
}

/* 
Local variables:
c-basic-offset: 4
indent-tabs-mode: nil
End:
*/
