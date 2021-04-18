/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_locale_setlocale__doc__,
"setlocale($module, category, locale=<unrepresentable>, /)\n"
"--\n"
"\n"
"Activates/queries locale processing.");

#define _LOCALE_SETLOCALE_METHODDEF    \
    {"setlocale", (PyCFunction)(void(*)(void))_locale_setlocale, METH_FASTCALL, _locale_setlocale__doc__},

static PyObject *
_locale_setlocale_impl(PyObject *module, int category, const char *locale);

static PyObject *
_locale_setlocale(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int category;
    const char *locale = NULL;

    if (!_PyArg_CheckPositional("setlocale", nargs, 1, 2)) {
        goto exit;
    }
    category = _PyLong_AsInt(args[0]);
    if (category == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (args[1] == Py_None) {
        locale = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t locale_length;
        locale = PyUnicode_AsUTF8AndSize(args[1], &locale_length);
        if (locale == NULL) {
            goto exit;
        }
        if (strlen(locale) != (size_t)locale_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("setlocale", "argument 2", "str or None", args[1]);
        goto exit;
    }
skip_optional:
    return_value = _locale_setlocale_impl(module, category, locale);

exit:
    return return_value;
}

PyDoc_STRVAR(_locale_localeconv__doc__,
"localeconv($module, /)\n"
"--\n"
"\n"
"Returns numeric and monetary locale-specific parameters.");

#define _LOCALE_LOCALECONV_METHODDEF    \
    {"localeconv", (PyCFunction)_locale_localeconv, METH_NOARGS, _locale_localeconv__doc__},

static PyObject *
_locale_localeconv_impl(PyObject *module);

static PyObject *
_locale_localeconv(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _locale_localeconv_impl(module);
}

#if defined(HAVE_WCSCOLL)

PyDoc_STRVAR(_locale_strcoll__doc__,
"strcoll($module, os1, os2, /)\n"
"--\n"
"\n"
"Compares two strings according to the locale.");

#define _LOCALE_STRCOLL_METHODDEF    \
    {"strcoll", (PyCFunction)(void(*)(void))_locale_strcoll, METH_FASTCALL, _locale_strcoll__doc__},

static PyObject *
_locale_strcoll_impl(PyObject *module, PyObject *os1, PyObject *os2);

static PyObject *
_locale_strcoll(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *os1;
    PyObject *os2;

    if (!_PyArg_CheckPositional("strcoll", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("strcoll", "argument 1", "str", args[0]);
        goto exit;
    }
    if (PyUnicode_READY(args[0]) == -1) {
        goto exit;
    }
    os1 = args[0];
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("strcoll", "argument 2", "str", args[1]);
        goto exit;
    }
    if (PyUnicode_READY(args[1]) == -1) {
        goto exit;
    }
    os2 = args[1];
    return_value = _locale_strcoll_impl(module, os1, os2);

exit:
    return return_value;
}

#endif /* defined(HAVE_WCSCOLL) */

#if defined(HAVE_WCSXFRM)

PyDoc_STRVAR(_locale_strxfrm__doc__,
"strxfrm($module, string, /)\n"
"--\n"
"\n"
"Return a string that can be used as a key for locale-aware comparisons.");

#define _LOCALE_STRXFRM_METHODDEF    \
    {"strxfrm", (PyCFunction)_locale_strxfrm, METH_O, _locale_strxfrm__doc__},

static PyObject *
_locale_strxfrm_impl(PyObject *module, PyObject *str);

static PyObject *
_locale_strxfrm(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *str;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("strxfrm", "argument", "str", arg);
        goto exit;
    }
    if (PyUnicode_READY(arg) == -1) {
        goto exit;
    }
    str = arg;
    return_value = _locale_strxfrm_impl(module, str);

exit:
    return return_value;
}

#endif /* defined(HAVE_WCSXFRM) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_locale__getdefaultlocale__doc__,
"_getdefaultlocale($module, /)\n"
"--\n"
"\n");

#define _LOCALE__GETDEFAULTLOCALE_METHODDEF    \
    {"_getdefaultlocale", (PyCFunction)_locale__getdefaultlocale, METH_NOARGS, _locale__getdefaultlocale__doc__},

static PyObject *
_locale__getdefaultlocale_impl(PyObject *module);

static PyObject *
_locale__getdefaultlocale(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _locale__getdefaultlocale_impl(module);
}

#endif /* defined(MS_WINDOWS) */

#if defined(HAVE_LANGINFO_H)

PyDoc_STRVAR(_locale_nl_langinfo__doc__,
"nl_langinfo($module, key, /)\n"
"--\n"
"\n"
"Return the value for the locale information associated with key.");

#define _LOCALE_NL_LANGINFO_METHODDEF    \
    {"nl_langinfo", (PyCFunction)_locale_nl_langinfo, METH_O, _locale_nl_langinfo__doc__},

static PyObject *
_locale_nl_langinfo_impl(PyObject *module, int item);

static PyObject *
_locale_nl_langinfo(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int item;

    item = _PyLong_AsInt(arg);
    if (item == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _locale_nl_langinfo_impl(module, item);

exit:
    return return_value;
}

#endif /* defined(HAVE_LANGINFO_H) */

#if defined(HAVE_LIBINTL_H)

PyDoc_STRVAR(_locale_gettext__doc__,
"gettext($module, msg, /)\n"
"--\n"
"\n"
"gettext(msg) -> string\n"
"\n"
"Return translation of msg.");

#define _LOCALE_GETTEXT_METHODDEF    \
    {"gettext", (PyCFunction)_locale_gettext, METH_O, _locale_gettext__doc__},

static PyObject *
_locale_gettext_impl(PyObject *module, const char *in);

static PyObject *
_locale_gettext(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *in;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("gettext", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t in_length;
    in = PyUnicode_AsUTF8AndSize(arg, &in_length);
    if (in == NULL) {
        goto exit;
    }
    if (strlen(in) != (size_t)in_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _locale_gettext_impl(module, in);

exit:
    return return_value;
}

#endif /* defined(HAVE_LIBINTL_H) */

#if defined(HAVE_LIBINTL_H)

PyDoc_STRVAR(_locale_dgettext__doc__,
"dgettext($module, domain, msg, /)\n"
"--\n"
"\n"
"dgettext(domain, msg) -> string\n"
"\n"
"Return translation of msg in domain.");

#define _LOCALE_DGETTEXT_METHODDEF    \
    {"dgettext", (PyCFunction)(void(*)(void))_locale_dgettext, METH_FASTCALL, _locale_dgettext__doc__},

static PyObject *
_locale_dgettext_impl(PyObject *module, const char *domain, const char *in);

static PyObject *
_locale_dgettext(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *domain;
    const char *in;

    if (!_PyArg_CheckPositional("dgettext", nargs, 2, 2)) {
        goto exit;
    }
    if (args[0] == Py_None) {
        domain = NULL;
    }
    else if (PyUnicode_Check(args[0])) {
        Py_ssize_t domain_length;
        domain = PyUnicode_AsUTF8AndSize(args[0], &domain_length);
        if (domain == NULL) {
            goto exit;
        }
        if (strlen(domain) != (size_t)domain_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("dgettext", "argument 1", "str or None", args[0]);
        goto exit;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("dgettext", "argument 2", "str", args[1]);
        goto exit;
    }
    Py_ssize_t in_length;
    in = PyUnicode_AsUTF8AndSize(args[1], &in_length);
    if (in == NULL) {
        goto exit;
    }
    if (strlen(in) != (size_t)in_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _locale_dgettext_impl(module, domain, in);

exit:
    return return_value;
}

#endif /* defined(HAVE_LIBINTL_H) */

#if defined(HAVE_LIBINTL_H)

PyDoc_STRVAR(_locale_dcgettext__doc__,
"dcgettext($module, domain, msg, category, /)\n"
"--\n"
"\n"
"Return translation of msg in domain and category.");

#define _LOCALE_DCGETTEXT_METHODDEF    \
    {"dcgettext", (PyCFunction)(void(*)(void))_locale_dcgettext, METH_FASTCALL, _locale_dcgettext__doc__},

static PyObject *
_locale_dcgettext_impl(PyObject *module, const char *domain,
                       const char *msgid, int category);

static PyObject *
_locale_dcgettext(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *domain;
    const char *msgid;
    int category;

    if (!_PyArg_CheckPositional("dcgettext", nargs, 3, 3)) {
        goto exit;
    }
    if (args[0] == Py_None) {
        domain = NULL;
    }
    else if (PyUnicode_Check(args[0])) {
        Py_ssize_t domain_length;
        domain = PyUnicode_AsUTF8AndSize(args[0], &domain_length);
        if (domain == NULL) {
            goto exit;
        }
        if (strlen(domain) != (size_t)domain_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("dcgettext", "argument 1", "str or None", args[0]);
        goto exit;
    }
    if (!PyUnicode_Check(args[1])) {
        _PyArg_BadArgument("dcgettext", "argument 2", "str", args[1]);
        goto exit;
    }
    Py_ssize_t msgid_length;
    msgid = PyUnicode_AsUTF8AndSize(args[1], &msgid_length);
    if (msgid == NULL) {
        goto exit;
    }
    if (strlen(msgid) != (size_t)msgid_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    category = _PyLong_AsInt(args[2]);
    if (category == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _locale_dcgettext_impl(module, domain, msgid, category);

exit:
    return return_value;
}

#endif /* defined(HAVE_LIBINTL_H) */

#if defined(HAVE_LIBINTL_H)

PyDoc_STRVAR(_locale_textdomain__doc__,
"textdomain($module, domain, /)\n"
"--\n"
"\n"
"Set the C library\'s textdmain to domain, returning the new domain.");

#define _LOCALE_TEXTDOMAIN_METHODDEF    \
    {"textdomain", (PyCFunction)_locale_textdomain, METH_O, _locale_textdomain__doc__},

static PyObject *
_locale_textdomain_impl(PyObject *module, const char *domain);

static PyObject *
_locale_textdomain(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *domain;

    if (arg == Py_None) {
        domain = NULL;
    }
    else if (PyUnicode_Check(arg)) {
        Py_ssize_t domain_length;
        domain = PyUnicode_AsUTF8AndSize(arg, &domain_length);
        if (domain == NULL) {
            goto exit;
        }
        if (strlen(domain) != (size_t)domain_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("textdomain", "argument", "str or None", arg);
        goto exit;
    }
    return_value = _locale_textdomain_impl(module, domain);

exit:
    return return_value;
}

#endif /* defined(HAVE_LIBINTL_H) */

#if defined(HAVE_LIBINTL_H)

PyDoc_STRVAR(_locale_bindtextdomain__doc__,
"bindtextdomain($module, domain, dir, /)\n"
"--\n"
"\n"
"Bind the C library\'s domain to dir.");

#define _LOCALE_BINDTEXTDOMAIN_METHODDEF    \
    {"bindtextdomain", (PyCFunction)(void(*)(void))_locale_bindtextdomain, METH_FASTCALL, _locale_bindtextdomain__doc__},

static PyObject *
_locale_bindtextdomain_impl(PyObject *module, const char *domain,
                            PyObject *dirname_obj);

static PyObject *
_locale_bindtextdomain(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *domain;
    PyObject *dirname_obj;

    if (!_PyArg_CheckPositional("bindtextdomain", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("bindtextdomain", "argument 1", "str", args[0]);
        goto exit;
    }
    Py_ssize_t domain_length;
    domain = PyUnicode_AsUTF8AndSize(args[0], &domain_length);
    if (domain == NULL) {
        goto exit;
    }
    if (strlen(domain) != (size_t)domain_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    dirname_obj = args[1];
    return_value = _locale_bindtextdomain_impl(module, domain, dirname_obj);

exit:
    return return_value;
}

#endif /* defined(HAVE_LIBINTL_H) */

#if defined(HAVE_LIBINTL_H) && defined(HAVE_BIND_TEXTDOMAIN_CODESET)

PyDoc_STRVAR(_locale_bind_textdomain_codeset__doc__,
"bind_textdomain_codeset($module, domain, codeset, /)\n"
"--\n"
"\n"
"Bind the C library\'s domain to codeset.");

#define _LOCALE_BIND_TEXTDOMAIN_CODESET_METHODDEF    \
    {"bind_textdomain_codeset", (PyCFunction)(void(*)(void))_locale_bind_textdomain_codeset, METH_FASTCALL, _locale_bind_textdomain_codeset__doc__},

static PyObject *
_locale_bind_textdomain_codeset_impl(PyObject *module, const char *domain,
                                     const char *codeset);

static PyObject *
_locale_bind_textdomain_codeset(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *domain;
    const char *codeset;

    if (!_PyArg_CheckPositional("bind_textdomain_codeset", nargs, 2, 2)) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("bind_textdomain_codeset", "argument 1", "str", args[0]);
        goto exit;
    }
    Py_ssize_t domain_length;
    domain = PyUnicode_AsUTF8AndSize(args[0], &domain_length);
    if (domain == NULL) {
        goto exit;
    }
    if (strlen(domain) != (size_t)domain_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    if (args[1] == Py_None) {
        codeset = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t codeset_length;
        codeset = PyUnicode_AsUTF8AndSize(args[1], &codeset_length);
        if (codeset == NULL) {
            goto exit;
        }
        if (strlen(codeset) != (size_t)codeset_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("bind_textdomain_codeset", "argument 2", "str or None", args[1]);
        goto exit;
    }
    return_value = _locale_bind_textdomain_codeset_impl(module, domain, codeset);

exit:
    return return_value;
}

#endif /* defined(HAVE_LIBINTL_H) && defined(HAVE_BIND_TEXTDOMAIN_CODESET) */

PyDoc_STRVAR(_locale__get_locale_encoding__doc__,
"_get_locale_encoding($module, /)\n"
"--\n"
"\n"
"Get the current locale encoding.");

#define _LOCALE__GET_LOCALE_ENCODING_METHODDEF    \
    {"_get_locale_encoding", (PyCFunction)_locale__get_locale_encoding, METH_NOARGS, _locale__get_locale_encoding__doc__},

static PyObject *
_locale__get_locale_encoding_impl(PyObject *module);

static PyObject *
_locale__get_locale_encoding(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _locale__get_locale_encoding_impl(module);
}

#ifndef _LOCALE_STRCOLL_METHODDEF
    #define _LOCALE_STRCOLL_METHODDEF
#endif /* !defined(_LOCALE_STRCOLL_METHODDEF) */

#ifndef _LOCALE_STRXFRM_METHODDEF
    #define _LOCALE_STRXFRM_METHODDEF
#endif /* !defined(_LOCALE_STRXFRM_METHODDEF) */

#ifndef _LOCALE__GETDEFAULTLOCALE_METHODDEF
    #define _LOCALE__GETDEFAULTLOCALE_METHODDEF
#endif /* !defined(_LOCALE__GETDEFAULTLOCALE_METHODDEF) */

#ifndef _LOCALE_NL_LANGINFO_METHODDEF
    #define _LOCALE_NL_LANGINFO_METHODDEF
#endif /* !defined(_LOCALE_NL_LANGINFO_METHODDEF) */

#ifndef _LOCALE_GETTEXT_METHODDEF
    #define _LOCALE_GETTEXT_METHODDEF
#endif /* !defined(_LOCALE_GETTEXT_METHODDEF) */

#ifndef _LOCALE_DGETTEXT_METHODDEF
    #define _LOCALE_DGETTEXT_METHODDEF
#endif /* !defined(_LOCALE_DGETTEXT_METHODDEF) */

#ifndef _LOCALE_DCGETTEXT_METHODDEF
    #define _LOCALE_DCGETTEXT_METHODDEF
#endif /* !defined(_LOCALE_DCGETTEXT_METHODDEF) */

#ifndef _LOCALE_TEXTDOMAIN_METHODDEF
    #define _LOCALE_TEXTDOMAIN_METHODDEF
#endif /* !defined(_LOCALE_TEXTDOMAIN_METHODDEF) */

#ifndef _LOCALE_BINDTEXTDOMAIN_METHODDEF
    #define _LOCALE_BINDTEXTDOMAIN_METHODDEF
#endif /* !defined(_LOCALE_BINDTEXTDOMAIN_METHODDEF) */

#ifndef _LOCALE_BIND_TEXTDOMAIN_CODESET_METHODDEF
    #define _LOCALE_BIND_TEXTDOMAIN_CODESET_METHODDEF
#endif /* !defined(_LOCALE_BIND_TEXTDOMAIN_CODESET_METHODDEF) */
/*[clinic end generated code: output=cd703c8a3a75fcf4 input=a9049054013a1b77]*/
