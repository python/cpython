/***********************************************************
Copyright (C) 1997 Martin von Loewis

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies.

This software comes with no warranty. Use at your own risk.

******************************************************************/

#include "Python.h"

#include <stdio.h>
#include <errno.h>
#include <locale.h>
#include <string.h>
#include <ctype.h>

#if defined(MS_WIN32)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef macintosh
#include "macglue.h"
#endif

static char locale__doc__[] = "Support for POSIX locales.";

static PyObject *Error;

/* support functions for formatting floating point numbers */

static char setlocale__doc__[] =
"(integer,string=None) -> string. Activates/queries locale processing."
;

/* to record the LC_NUMERIC settings */
static PyObject* grouping = NULL;
static PyObject* thousands_sep = NULL;
static PyObject* decimal_point = NULL;
/* if non-null, indicates that LC_NUMERIC is different from "C" */
static char* saved_numeric = NULL;

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
        val = PyInt_FromLong(s[i]);
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

static void
fixup_ulcase(void)
{
    PyObject *mods, *strop, *string, *ulo;
    unsigned char ul[256];
    int n, c;

    /* find the string and strop modules */
    mods = PyImport_GetModuleDict();
    if (!mods)
        return;
    string = PyDict_GetItemString(mods, "string");
    if (string)
        string = PyModule_GetDict(string);
    strop=PyDict_GetItemString(mods, "strop");
    if (strop)
        strop = PyModule_GetDict(strop);
    if (!string && !strop)
        return;

    /* create uppercase map string */
    n = 0;
    for (c = 0; c < 256; c++) {
        if (isupper(c))
            ul[n++] = c;
    }
    ulo = PyString_FromStringAndSize((const char *)ul, n);
    if (!ulo)
        return;
    if (string)
        PyDict_SetItemString(string, "uppercase", ulo);
    if (strop)
        PyDict_SetItemString(strop, "uppercase", ulo);
    Py_DECREF(ulo);

    /* create lowercase string */
    n = 0;
    for (c = 0; c < 256; c++) {
        if (islower(c))
            ul[n++] = c;
    }
    ulo = PyString_FromStringAndSize((const char *)ul, n);
    if (!ulo)
        return;
    if (string)
        PyDict_SetItemString(string, "lowercase", ulo);
    if (strop)
        PyDict_SetItemString(strop, "lowercase", ulo);
    Py_DECREF(ulo);

    /* create letters string */
    n = 0;
    for (c = 0; c < 256; c++) {
        if (isalpha(c))
            ul[n++] = c;
    }
    ulo = PyString_FromStringAndSize((const char *)ul, n);
    if (!ulo)
        return;
    if (string)
        PyDict_SetItemString(string, "letters", ulo);
    Py_DECREF(ulo);
}
  

static PyObject*
PyLocale_setlocale(PyObject* self, PyObject* args)
{
    int category;
    char *locale = NULL, *result;
    PyObject *result_object;
    struct lconv *lc;

    if (!PyArg_ParseTuple(args, "i|z:setlocale", &category, &locale))
        return NULL;

    if (locale) {
        /* set locale */
        result = setlocale(category, locale);
        if (!result) {
            /* operation failed, no setting was changed */
            PyErr_SetString(Error, "locale setting not supported");
            return NULL;
        }
        result_object = PyString_FromString(result);
        if (!result)
            return NULL;
        /* record changes to LC_NUMERIC */
        if (category == LC_NUMERIC || category == LC_ALL) {
            if (strcmp(locale, "C") == 0 || strcmp(locale, "POSIX") == 0) {
                /* user just asked for default numeric locale */
                if (saved_numeric)
                    free(saved_numeric);
                saved_numeric = NULL;
            } else {
                /* remember values */
                lc = localeconv();
                Py_XDECREF(grouping);
                grouping = copy_grouping(lc->grouping);
                Py_XDECREF(thousands_sep);
                thousands_sep = PyString_FromString(lc->thousands_sep);
                Py_XDECREF(decimal_point);
                decimal_point = PyString_FromString(lc->decimal_point);
                saved_numeric = strdup(locale);
                /* restore to "C" */
                setlocale(LC_NUMERIC, "C");
            }
        }
        /* record changes to LC_CTYPE */
        if (category == LC_CTYPE || category == LC_ALL)
            fixup_ulcase();
        /* things that got wrong up to here are ignored */
        PyErr_Clear();
    } else {
        /* get locale */
        /* restore LC_NUMERIC first, if appropriate */
        if (saved_numeric)
            setlocale(LC_NUMERIC, saved_numeric);
        result = setlocale(category, NULL);
        if (!result) {
            PyErr_SetString(Error, "locale query failed");
            return NULL;
        }
        result_object = PyString_FromString(result);
        /* restore back to "C" */
        if (saved_numeric)
            setlocale(LC_NUMERIC, "C");
    }
    return result_object;
}

static char localeconv__doc__[] =
"() -> dict. Returns numeric and monetary locale-specific parameters."
;

static PyObject*
PyLocale_localeconv(PyObject* self, PyObject* args)
{
    PyObject* result;
    struct lconv *l;
    PyObject *x;

    if (!PyArg_NoArgs(args))
        return NULL;

    result = PyDict_New();
    if (!result)
        return NULL;

    /* if LC_NUMERIC is different in the C library, use saved value */
    l = localeconv();

    /* hopefully, the localeconv result survives the C library calls
       involved herein */

#define RESULT_STRING(s)\
    x = PyString_FromString(l->s);\
    if (!x) goto failed;\
    PyDict_SetItemString(result, #s, x);\
    Py_XDECREF(x)

#define RESULT_INT(i)\
    x = PyInt_FromLong(l->i);\
    if (!x) goto failed;\
    PyDict_SetItemString(result, #i, x);\
    Py_XDECREF(x)

    /* Numeric information */
    if (saved_numeric){
        /* cannot use localeconv results */
        PyDict_SetItemString(result, "decimal_point", decimal_point);
        PyDict_SetItemString(result, "grouping", grouping);
        PyDict_SetItemString(result, "thousands_sep", thousands_sep);
    } else {
        RESULT_STRING(decimal_point);
        RESULT_STRING(thousands_sep);
        x = copy_grouping(l->grouping);
        if (!x)
            goto failed;
        PyDict_SetItemString(result, "grouping", x);
        Py_XDECREF(x);
    }

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

static char strcoll__doc__[] =
"string,string -> int. Compares two strings according to the locale."
;

static PyObject*
PyLocale_strcoll(PyObject* self, PyObject* args)
{
  char *s1,*s2;

  if (!PyArg_ParseTuple(args, "ss:strcoll", &s1, &s2))
      return NULL;
  return PyInt_FromLong(strcoll(s1, s2));
}

static char strxfrm__doc__[] =
"string -> string. Returns a string that behaves for cmp locale-aware."
;

static PyObject*
PyLocale_strxfrm(PyObject* self, PyObject* args)
{
    char *s, *buf;
    size_t n1, n2;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "s:strxfrm", &s))
        return NULL;

    /* assume no change in size, first */
    n1 = strlen(s) + 1;
    buf = PyMem_Malloc(n1);
    if (!buf)
        return PyErr_NoMemory();
    n2 = strxfrm(buf, s, n1);
    if (n2 > n1) {
        /* more space needed */
        buf = PyMem_Realloc(buf, n2);
        if (!buf)
            return PyErr_NoMemory();
        strxfrm(buf, s, n2);
    }
    result = PyString_FromString(buf);
    PyMem_Free(buf);
    return result;
}

#if defined(MS_WIN32)
static PyObject*
PyLocale_getdefaultlocale(PyObject* self, PyObject* args)
{
    char encoding[100];
    char locale[100];

    if (!PyArg_NoArgs(args))
        return NULL;

    sprintf(encoding, "cp%d", GetACP());

    if (GetLocaleInfo(LOCALE_USER_DEFAULT,
                      LOCALE_SISO639LANGNAME,
                      locale, sizeof(locale))) {
        int i = strlen(locale);
        locale[i++] = '_';
        if (GetLocaleInfo(LOCALE_USER_DEFAULT,
                          LOCALE_SISO3166CTRYNAME,
                          locale+i, sizeof(locale)-i))
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

#if defined(macintosh)
static PyObject*
PyLocale_getdefaultlocale(PyObject* self, PyObject* args)
{
    return Py_BuildValue("Os", Py_None, PyMac_getscript());
}
#endif

static struct PyMethodDef PyLocale_Methods[] = {
  {"setlocale", (PyCFunction) PyLocale_setlocale, 
   METH_VARARGS, setlocale__doc__},
  {"localeconv", (PyCFunction) PyLocale_localeconv, 
   0, localeconv__doc__},
  {"strcoll", (PyCFunction) PyLocale_strcoll, 
   METH_VARARGS, strcoll__doc__},
  {"strxfrm", (PyCFunction) PyLocale_strxfrm, 
   METH_VARARGS, strxfrm__doc__},
#if defined(MS_WIN32) || defined(macintosh)
  {"_getdefaultlocale", (PyCFunction) PyLocale_getdefaultlocale, 0},
#endif
  {NULL, NULL}
};

DL_EXPORT(void)
init_locale(void)
{
    PyObject *m, *d, *x;

    m = Py_InitModule("_locale", PyLocale_Methods);

    d = PyModule_GetDict(m);

    x = PyInt_FromLong(LC_CTYPE);
    PyDict_SetItemString(d, "LC_CTYPE", x);
    Py_XDECREF(x);

    x = PyInt_FromLong(LC_TIME);
    PyDict_SetItemString(d, "LC_TIME", x);
    Py_XDECREF(x);

    x = PyInt_FromLong(LC_COLLATE);
    PyDict_SetItemString(d, "LC_COLLATE", x);
    Py_XDECREF(x);

    x = PyInt_FromLong(LC_MONETARY);
    PyDict_SetItemString(d, "LC_MONETARY", x);
    Py_XDECREF(x);

#ifdef LC_MESSAGES
    x = PyInt_FromLong(LC_MESSAGES);
    PyDict_SetItemString(d, "LC_MESSAGES", x);
    Py_XDECREF(x);
#endif /* LC_MESSAGES */

    x = PyInt_FromLong(LC_NUMERIC);
    PyDict_SetItemString(d, "LC_NUMERIC", x);
    Py_XDECREF(x);

    x = PyInt_FromLong(LC_ALL);
    PyDict_SetItemString(d, "LC_ALL", x);
    Py_XDECREF(x);

    x = PyInt_FromLong(CHAR_MAX);
    PyDict_SetItemString(d, "CHAR_MAX", x);
    Py_XDECREF(x);

    Error = PyErr_NewException("locale.Error", NULL, NULL);
    PyDict_SetItemString(d, "Error", Error);

    x = PyString_FromString(locale__doc__);
    PyDict_SetItemString(d, "__doc__", x);
    Py_XDECREF(x);
}
