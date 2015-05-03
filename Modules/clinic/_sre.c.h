/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_sre_getcodesize__doc__,
"getcodesize($module, /)\n"
"--\n"
"\n");

#define _SRE_GETCODESIZE_METHODDEF    \
    {"getcodesize", (PyCFunction)_sre_getcodesize, METH_NOARGS, _sre_getcodesize__doc__},

static int
_sre_getcodesize_impl(PyModuleDef *module);

static PyObject *
_sre_getcodesize(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    int _return_value;

    _return_value = _sre_getcodesize_impl(module);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_getlower__doc__,
"getlower($module, character, flags, /)\n"
"--\n"
"\n");

#define _SRE_GETLOWER_METHODDEF    \
    {"getlower", (PyCFunction)_sre_getlower, METH_VARARGS, _sre_getlower__doc__},

static int
_sre_getlower_impl(PyModuleDef *module, int character, int flags);

static PyObject *
_sre_getlower(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int character;
    int flags;
    int _return_value;

    if (!PyArg_ParseTuple(args, "ii:getlower",
        &character, &flags))
        goto exit;
    _return_value = _sre_getlower_impl(module, character, flags);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_match__doc__,
"match($self, /, string=None, pos=0, endpos=sys.maxsize, *, pattern=None)\n"
"--\n"
"\n"
"Matches zero or more characters at the beginning of the string.");

#define _SRE_SRE_PATTERN_MATCH_METHODDEF    \
    {"match", (PyCFunction)_sre_SRE_Pattern_match, METH_VARARGS|METH_KEYWORDS, _sre_SRE_Pattern_match__doc__},

static PyObject *
_sre_SRE_Pattern_match_impl(PatternObject *self, PyObject *string,
                            Py_ssize_t pos, Py_ssize_t endpos,
                            PyObject *pattern);

static PyObject *
_sre_SRE_Pattern_match(PatternObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"string", "pos", "endpos", "pattern", NULL};
    PyObject *string = NULL;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;
    PyObject *pattern = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Onn$O:match", _keywords,
        &string, &pos, &endpos, &pattern))
        goto exit;
    return_value = _sre_SRE_Pattern_match_impl(self, string, pos, endpos, pattern);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_fullmatch__doc__,
"fullmatch($self, /, string=None, pos=0, endpos=sys.maxsize, *,\n"
"          pattern=None)\n"
"--\n"
"\n"
"Matches against all of the string");

#define _SRE_SRE_PATTERN_FULLMATCH_METHODDEF    \
    {"fullmatch", (PyCFunction)_sre_SRE_Pattern_fullmatch, METH_VARARGS|METH_KEYWORDS, _sre_SRE_Pattern_fullmatch__doc__},

static PyObject *
_sre_SRE_Pattern_fullmatch_impl(PatternObject *self, PyObject *string,
                                Py_ssize_t pos, Py_ssize_t endpos,
                                PyObject *pattern);

static PyObject *
_sre_SRE_Pattern_fullmatch(PatternObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"string", "pos", "endpos", "pattern", NULL};
    PyObject *string = NULL;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;
    PyObject *pattern = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Onn$O:fullmatch", _keywords,
        &string, &pos, &endpos, &pattern))
        goto exit;
    return_value = _sre_SRE_Pattern_fullmatch_impl(self, string, pos, endpos, pattern);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_search__doc__,
"search($self, /, string=None, pos=0, endpos=sys.maxsize, *,\n"
"       pattern=None)\n"
"--\n"
"\n"
"Scan through string looking for a match, and return a corresponding match object instance.\n"
"\n"
"Return None if no position in the string matches.");

#define _SRE_SRE_PATTERN_SEARCH_METHODDEF    \
    {"search", (PyCFunction)_sre_SRE_Pattern_search, METH_VARARGS|METH_KEYWORDS, _sre_SRE_Pattern_search__doc__},

static PyObject *
_sre_SRE_Pattern_search_impl(PatternObject *self, PyObject *string,
                             Py_ssize_t pos, Py_ssize_t endpos,
                             PyObject *pattern);

static PyObject *
_sre_SRE_Pattern_search(PatternObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"string", "pos", "endpos", "pattern", NULL};
    PyObject *string = NULL;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;
    PyObject *pattern = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Onn$O:search", _keywords,
        &string, &pos, &endpos, &pattern))
        goto exit;
    return_value = _sre_SRE_Pattern_search_impl(self, string, pos, endpos, pattern);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_findall__doc__,
"findall($self, /, string=None, pos=0, endpos=sys.maxsize, *,\n"
"        source=None)\n"
"--\n"
"\n"
"Return a list of all non-overlapping matches of pattern in string.");

#define _SRE_SRE_PATTERN_FINDALL_METHODDEF    \
    {"findall", (PyCFunction)_sre_SRE_Pattern_findall, METH_VARARGS|METH_KEYWORDS, _sre_SRE_Pattern_findall__doc__},

static PyObject *
_sre_SRE_Pattern_findall_impl(PatternObject *self, PyObject *string,
                              Py_ssize_t pos, Py_ssize_t endpos,
                              PyObject *source);

static PyObject *
_sre_SRE_Pattern_findall(PatternObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"string", "pos", "endpos", "source", NULL};
    PyObject *string = NULL;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;
    PyObject *source = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Onn$O:findall", _keywords,
        &string, &pos, &endpos, &source))
        goto exit;
    return_value = _sre_SRE_Pattern_findall_impl(self, string, pos, endpos, source);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_finditer__doc__,
"finditer($self, /, string, pos=0, endpos=sys.maxsize)\n"
"--\n"
"\n"
"Return an iterator over all non-overlapping matches for the RE pattern in string.\n"
"\n"
"For each match, the iterator returns a match object.");

#define _SRE_SRE_PATTERN_FINDITER_METHODDEF    \
    {"finditer", (PyCFunction)_sre_SRE_Pattern_finditer, METH_VARARGS|METH_KEYWORDS, _sre_SRE_Pattern_finditer__doc__},

static PyObject *
_sre_SRE_Pattern_finditer_impl(PatternObject *self, PyObject *string,
                               Py_ssize_t pos, Py_ssize_t endpos);

static PyObject *
_sre_SRE_Pattern_finditer(PatternObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"string", "pos", "endpos", NULL};
    PyObject *string;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|nn:finditer", _keywords,
        &string, &pos, &endpos))
        goto exit;
    return_value = _sre_SRE_Pattern_finditer_impl(self, string, pos, endpos);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_scanner__doc__,
"scanner($self, /, string, pos=0, endpos=sys.maxsize)\n"
"--\n"
"\n");

#define _SRE_SRE_PATTERN_SCANNER_METHODDEF    \
    {"scanner", (PyCFunction)_sre_SRE_Pattern_scanner, METH_VARARGS|METH_KEYWORDS, _sre_SRE_Pattern_scanner__doc__},

static PyObject *
_sre_SRE_Pattern_scanner_impl(PatternObject *self, PyObject *string,
                              Py_ssize_t pos, Py_ssize_t endpos);

static PyObject *
_sre_SRE_Pattern_scanner(PatternObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"string", "pos", "endpos", NULL};
    PyObject *string;
    Py_ssize_t pos = 0;
    Py_ssize_t endpos = PY_SSIZE_T_MAX;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|nn:scanner", _keywords,
        &string, &pos, &endpos))
        goto exit;
    return_value = _sre_SRE_Pattern_scanner_impl(self, string, pos, endpos);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_split__doc__,
"split($self, /, string=None, maxsplit=0, *, source=None)\n"
"--\n"
"\n"
"Split string by the occurrences of pattern.");

#define _SRE_SRE_PATTERN_SPLIT_METHODDEF    \
    {"split", (PyCFunction)_sre_SRE_Pattern_split, METH_VARARGS|METH_KEYWORDS, _sre_SRE_Pattern_split__doc__},

static PyObject *
_sre_SRE_Pattern_split_impl(PatternObject *self, PyObject *string,
                            Py_ssize_t maxsplit, PyObject *source);

static PyObject *
_sre_SRE_Pattern_split(PatternObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"string", "maxsplit", "source", NULL};
    PyObject *string = NULL;
    Py_ssize_t maxsplit = 0;
    PyObject *source = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|On$O:split", _keywords,
        &string, &maxsplit, &source))
        goto exit;
    return_value = _sre_SRE_Pattern_split_impl(self, string, maxsplit, source);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_sub__doc__,
"sub($self, /, repl, string, count=0)\n"
"--\n"
"\n"
"Return the string obtained by replacing the leftmost non-overlapping occurrences of pattern in string by the replacement repl.");

#define _SRE_SRE_PATTERN_SUB_METHODDEF    \
    {"sub", (PyCFunction)_sre_SRE_Pattern_sub, METH_VARARGS|METH_KEYWORDS, _sre_SRE_Pattern_sub__doc__},

static PyObject *
_sre_SRE_Pattern_sub_impl(PatternObject *self, PyObject *repl,
                          PyObject *string, Py_ssize_t count);

static PyObject *
_sre_SRE_Pattern_sub(PatternObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"repl", "string", "count", NULL};
    PyObject *repl;
    PyObject *string;
    Py_ssize_t count = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|n:sub", _keywords,
        &repl, &string, &count))
        goto exit;
    return_value = _sre_SRE_Pattern_sub_impl(self, repl, string, count);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern_subn__doc__,
"subn($self, /, repl, string, count=0)\n"
"--\n"
"\n"
"Return the tuple (new_string, number_of_subs_made) found by replacing the leftmost non-overlapping occurrences of pattern with the replacement repl.");

#define _SRE_SRE_PATTERN_SUBN_METHODDEF    \
    {"subn", (PyCFunction)_sre_SRE_Pattern_subn, METH_VARARGS|METH_KEYWORDS, _sre_SRE_Pattern_subn__doc__},

static PyObject *
_sre_SRE_Pattern_subn_impl(PatternObject *self, PyObject *repl,
                           PyObject *string, Py_ssize_t count);

static PyObject *
_sre_SRE_Pattern_subn(PatternObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"repl", "string", "count", NULL};
    PyObject *repl;
    PyObject *string;
    Py_ssize_t count = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO|n:subn", _keywords,
        &repl, &string, &count))
        goto exit;
    return_value = _sre_SRE_Pattern_subn_impl(self, repl, string, count);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Pattern___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define _SRE_SRE_PATTERN___COPY___METHODDEF    \
    {"__copy__", (PyCFunction)_sre_SRE_Pattern___copy__, METH_NOARGS, _sre_SRE_Pattern___copy____doc__},

static PyObject *
_sre_SRE_Pattern___copy___impl(PatternObject *self);

static PyObject *
_sre_SRE_Pattern___copy__(PatternObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sre_SRE_Pattern___copy___impl(self);
}

PyDoc_STRVAR(_sre_SRE_Pattern___deepcopy____doc__,
"__deepcopy__($self, /, memo)\n"
"--\n"
"\n");

#define _SRE_SRE_PATTERN___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", (PyCFunction)_sre_SRE_Pattern___deepcopy__, METH_VARARGS|METH_KEYWORDS, _sre_SRE_Pattern___deepcopy____doc__},

static PyObject *
_sre_SRE_Pattern___deepcopy___impl(PatternObject *self, PyObject *memo);

static PyObject *
_sre_SRE_Pattern___deepcopy__(PatternObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"memo", NULL};
    PyObject *memo;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:__deepcopy__", _keywords,
        &memo))
        goto exit;
    return_value = _sre_SRE_Pattern___deepcopy___impl(self, memo);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_compile__doc__,
"compile($module, /, pattern, flags, code, groups, groupindex,\n"
"        indexgroup)\n"
"--\n"
"\n");

#define _SRE_COMPILE_METHODDEF    \
    {"compile", (PyCFunction)_sre_compile, METH_VARARGS|METH_KEYWORDS, _sre_compile__doc__},

static PyObject *
_sre_compile_impl(PyModuleDef *module, PyObject *pattern, int flags,
                  PyObject *code, Py_ssize_t groups, PyObject *groupindex,
                  PyObject *indexgroup);

static PyObject *
_sre_compile(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"pattern", "flags", "code", "groups", "groupindex", "indexgroup", NULL};
    PyObject *pattern;
    int flags;
    PyObject *code;
    Py_ssize_t groups;
    PyObject *groupindex;
    PyObject *indexgroup;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OiO!nOO:compile", _keywords,
        &pattern, &flags, &PyList_Type, &code, &groups, &groupindex, &indexgroup))
        goto exit;
    return_value = _sre_compile_impl(module, pattern, flags, code, groups, groupindex, indexgroup);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_expand__doc__,
"expand($self, /, template)\n"
"--\n"
"\n"
"Return the string obtained by doing backslash substitution on the string template, as done by the sub() method.");

#define _SRE_SRE_MATCH_EXPAND_METHODDEF    \
    {"expand", (PyCFunction)_sre_SRE_Match_expand, METH_VARARGS|METH_KEYWORDS, _sre_SRE_Match_expand__doc__},

static PyObject *
_sre_SRE_Match_expand_impl(MatchObject *self, PyObject *template);

static PyObject *
_sre_SRE_Match_expand(MatchObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"template", NULL};
    PyObject *template;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:expand", _keywords,
        &template))
        goto exit;
    return_value = _sre_SRE_Match_expand_impl(self, template);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_groups__doc__,
"groups($self, /, default=None)\n"
"--\n"
"\n"
"Return a tuple containing all the subgroups of the match, from 1.\n"
"\n"
"  default\n"
"    Is used for groups that did not participate in the match.");

#define _SRE_SRE_MATCH_GROUPS_METHODDEF    \
    {"groups", (PyCFunction)_sre_SRE_Match_groups, METH_VARARGS|METH_KEYWORDS, _sre_SRE_Match_groups__doc__},

static PyObject *
_sre_SRE_Match_groups_impl(MatchObject *self, PyObject *default_value);

static PyObject *
_sre_SRE_Match_groups(MatchObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"default", NULL};
    PyObject *default_value = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:groups", _keywords,
        &default_value))
        goto exit;
    return_value = _sre_SRE_Match_groups_impl(self, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_groupdict__doc__,
"groupdict($self, /, default=None)\n"
"--\n"
"\n"
"Return a dictionary containing all the named subgroups of the match, keyed by the subgroup name.\n"
"\n"
"  default\n"
"    Is used for groups that did not participate in the match.");

#define _SRE_SRE_MATCH_GROUPDICT_METHODDEF    \
    {"groupdict", (PyCFunction)_sre_SRE_Match_groupdict, METH_VARARGS|METH_KEYWORDS, _sre_SRE_Match_groupdict__doc__},

static PyObject *
_sre_SRE_Match_groupdict_impl(MatchObject *self, PyObject *default_value);

static PyObject *
_sre_SRE_Match_groupdict(MatchObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"default", NULL};
    PyObject *default_value = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O:groupdict", _keywords,
        &default_value))
        goto exit;
    return_value = _sre_SRE_Match_groupdict_impl(self, default_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_start__doc__,
"start($self, group=0, /)\n"
"--\n"
"\n"
"Return index of the start of the substring matched by group.");

#define _SRE_SRE_MATCH_START_METHODDEF    \
    {"start", (PyCFunction)_sre_SRE_Match_start, METH_VARARGS, _sre_SRE_Match_start__doc__},

static Py_ssize_t
_sre_SRE_Match_start_impl(MatchObject *self, PyObject *group);

static PyObject *
_sre_SRE_Match_start(MatchObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *group = NULL;
    Py_ssize_t _return_value;

    if (!PyArg_UnpackTuple(args, "start",
        0, 1,
        &group))
        goto exit;
    _return_value = _sre_SRE_Match_start_impl(self, group);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_end__doc__,
"end($self, group=0, /)\n"
"--\n"
"\n"
"Return index of the end of the substring matched by group.");

#define _SRE_SRE_MATCH_END_METHODDEF    \
    {"end", (PyCFunction)_sre_SRE_Match_end, METH_VARARGS, _sre_SRE_Match_end__doc__},

static Py_ssize_t
_sre_SRE_Match_end_impl(MatchObject *self, PyObject *group);

static PyObject *
_sre_SRE_Match_end(MatchObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *group = NULL;
    Py_ssize_t _return_value;

    if (!PyArg_UnpackTuple(args, "end",
        0, 1,
        &group))
        goto exit;
    _return_value = _sre_SRE_Match_end_impl(self, group);
    if ((_return_value == -1) && PyErr_Occurred())
        goto exit;
    return_value = PyLong_FromSsize_t(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match_span__doc__,
"span($self, group=0, /)\n"
"--\n"
"\n"
"For MatchObject m, return the 2-tuple (m.start(group), m.end(group)).");

#define _SRE_SRE_MATCH_SPAN_METHODDEF    \
    {"span", (PyCFunction)_sre_SRE_Match_span, METH_VARARGS, _sre_SRE_Match_span__doc__},

static PyObject *
_sre_SRE_Match_span_impl(MatchObject *self, PyObject *group);

static PyObject *
_sre_SRE_Match_span(MatchObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *group = NULL;

    if (!PyArg_UnpackTuple(args, "span",
        0, 1,
        &group))
        goto exit;
    return_value = _sre_SRE_Match_span_impl(self, group);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Match___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define _SRE_SRE_MATCH___COPY___METHODDEF    \
    {"__copy__", (PyCFunction)_sre_SRE_Match___copy__, METH_NOARGS, _sre_SRE_Match___copy____doc__},

static PyObject *
_sre_SRE_Match___copy___impl(MatchObject *self);

static PyObject *
_sre_SRE_Match___copy__(MatchObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sre_SRE_Match___copy___impl(self);
}

PyDoc_STRVAR(_sre_SRE_Match___deepcopy____doc__,
"__deepcopy__($self, /, memo)\n"
"--\n"
"\n");

#define _SRE_SRE_MATCH___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", (PyCFunction)_sre_SRE_Match___deepcopy__, METH_VARARGS|METH_KEYWORDS, _sre_SRE_Match___deepcopy____doc__},

static PyObject *
_sre_SRE_Match___deepcopy___impl(MatchObject *self, PyObject *memo);

static PyObject *
_sre_SRE_Match___deepcopy__(MatchObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"memo", NULL};
    PyObject *memo;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:__deepcopy__", _keywords,
        &memo))
        goto exit;
    return_value = _sre_SRE_Match___deepcopy___impl(self, memo);

exit:
    return return_value;
}

PyDoc_STRVAR(_sre_SRE_Scanner_match__doc__,
"match($self, /)\n"
"--\n"
"\n");

#define _SRE_SRE_SCANNER_MATCH_METHODDEF    \
    {"match", (PyCFunction)_sre_SRE_Scanner_match, METH_NOARGS, _sre_SRE_Scanner_match__doc__},

static PyObject *
_sre_SRE_Scanner_match_impl(ScannerObject *self);

static PyObject *
_sre_SRE_Scanner_match(ScannerObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sre_SRE_Scanner_match_impl(self);
}

PyDoc_STRVAR(_sre_SRE_Scanner_search__doc__,
"search($self, /)\n"
"--\n"
"\n");

#define _SRE_SRE_SCANNER_SEARCH_METHODDEF    \
    {"search", (PyCFunction)_sre_SRE_Scanner_search, METH_NOARGS, _sre_SRE_Scanner_search__doc__},

static PyObject *
_sre_SRE_Scanner_search_impl(ScannerObject *self);

static PyObject *
_sre_SRE_Scanner_search(ScannerObject *self, PyObject *Py_UNUSED(ignored))
{
    return _sre_SRE_Scanner_search_impl(self);
}
/*[clinic end generated code: output=d1d73ab2c5008bd4 input=a9049054013a1b77]*/
