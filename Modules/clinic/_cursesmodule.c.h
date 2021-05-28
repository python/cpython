/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_curses_window_addch__doc__,
"addch([y, x,] ch, [attr=_curses.A_NORMAL])\n"
"Paint the character.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  ch\n"
"    Character to add.\n"
"  attr\n"
"    Attributes for the character.\n"
"\n"
"Paint character ch at (y, x) with attributes attr,\n"
"overwriting any character previously painted at that location.\n"
"By default, the character position and attributes are the\n"
"current settings for the window object.");

#define _CURSES_WINDOW_ADDCH_METHODDEF    \
    {"addch", (PyCFunction)_curses_window_addch, METH_VARARGS, _curses_window_addch__doc__},

static PyObject *
_curses_window_addch_impl(PyCursesWindowObject *self, int group_left_1,
                          int y, int x, PyObject *ch, int group_right_1,
                          long attr);

static PyObject *
_curses_window_addch(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *ch;
    int group_right_1 = 0;
    long attr = A_NORMAL;

    switch (PyTuple_GET_SIZE(args)) {
        case 1:
            if (!PyArg_ParseTuple(args, "O:addch", &ch)) {
                goto exit;
            }
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "Ol:addch", &ch, &attr)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "iiO:addch", &y, &x, &ch)) {
                goto exit;
            }
            group_left_1 = 1;
            break;
        case 4:
            if (!PyArg_ParseTuple(args, "iiOl:addch", &y, &x, &ch, &attr)) {
                goto exit;
            }
            group_right_1 = 1;
            group_left_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.addch requires 1 to 4 arguments");
            goto exit;
    }
    return_value = _curses_window_addch_impl(self, group_left_1, y, x, ch, group_right_1, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_addstr__doc__,
"addstr([y, x,] str, [attr])\n"
"Paint the string.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  str\n"
"    String to add.\n"
"  attr\n"
"    Attributes for characters.\n"
"\n"
"Paint the string str at (y, x) with attributes attr,\n"
"overwriting anything previously on the display.\n"
"By default, the character position and attributes are the\n"
"current settings for the window object.");

#define _CURSES_WINDOW_ADDSTR_METHODDEF    \
    {"addstr", (PyCFunction)_curses_window_addstr, METH_VARARGS, _curses_window_addstr__doc__},

static PyObject *
_curses_window_addstr_impl(PyCursesWindowObject *self, int group_left_1,
                           int y, int x, PyObject *str, int group_right_1,
                           long attr);

static PyObject *
_curses_window_addstr(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *str;
    int group_right_1 = 0;
    long attr = 0;

    switch (PyTuple_GET_SIZE(args)) {
        case 1:
            if (!PyArg_ParseTuple(args, "O:addstr", &str)) {
                goto exit;
            }
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "Ol:addstr", &str, &attr)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "iiO:addstr", &y, &x, &str)) {
                goto exit;
            }
            group_left_1 = 1;
            break;
        case 4:
            if (!PyArg_ParseTuple(args, "iiOl:addstr", &y, &x, &str, &attr)) {
                goto exit;
            }
            group_right_1 = 1;
            group_left_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.addstr requires 1 to 4 arguments");
            goto exit;
    }
    return_value = _curses_window_addstr_impl(self, group_left_1, y, x, str, group_right_1, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_addnstr__doc__,
"addnstr([y, x,] str, n, [attr])\n"
"Paint at most n characters of the string.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  str\n"
"    String to add.\n"
"  n\n"
"    Maximal number of characters.\n"
"  attr\n"
"    Attributes for characters.\n"
"\n"
"Paint at most n characters of the string str at (y, x) with\n"
"attributes attr, overwriting anything previously on the display.\n"
"By default, the character position and attributes are the\n"
"current settings for the window object.");

#define _CURSES_WINDOW_ADDNSTR_METHODDEF    \
    {"addnstr", (PyCFunction)_curses_window_addnstr, METH_VARARGS, _curses_window_addnstr__doc__},

static PyObject *
_curses_window_addnstr_impl(PyCursesWindowObject *self, int group_left_1,
                            int y, int x, PyObject *str, int n,
                            int group_right_1, long attr);

static PyObject *
_curses_window_addnstr(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *str;
    int n;
    int group_right_1 = 0;
    long attr = 0;

    switch (PyTuple_GET_SIZE(args)) {
        case 2:
            if (!PyArg_ParseTuple(args, "Oi:addnstr", &str, &n)) {
                goto exit;
            }
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "Oil:addnstr", &str, &n, &attr)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        case 4:
            if (!PyArg_ParseTuple(args, "iiOi:addnstr", &y, &x, &str, &n)) {
                goto exit;
            }
            group_left_1 = 1;
            break;
        case 5:
            if (!PyArg_ParseTuple(args, "iiOil:addnstr", &y, &x, &str, &n, &attr)) {
                goto exit;
            }
            group_right_1 = 1;
            group_left_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.addnstr requires 2 to 5 arguments");
            goto exit;
    }
    return_value = _curses_window_addnstr_impl(self, group_left_1, y, x, str, n, group_right_1, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_bkgd__doc__,
"bkgd($self, ch, attr=_curses.A_NORMAL, /)\n"
"--\n"
"\n"
"Set the background property of the window.\n"
"\n"
"  ch\n"
"    Background character.\n"
"  attr\n"
"    Background attributes.");

#define _CURSES_WINDOW_BKGD_METHODDEF    \
    {"bkgd", (PyCFunction)(void(*)(void))_curses_window_bkgd, METH_FASTCALL, _curses_window_bkgd__doc__},

static PyObject *
_curses_window_bkgd_impl(PyCursesWindowObject *self, PyObject *ch, long attr);

static PyObject *
_curses_window_bkgd(PyCursesWindowObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *ch;
    long attr = A_NORMAL;

    if (!_PyArg_CheckPositional("bkgd", nargs, 1, 2)) {
        goto exit;
    }
    ch = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    attr = PyLong_AsLong(args[1]);
    if (attr == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _curses_window_bkgd_impl(self, ch, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_attroff__doc__,
"attroff($self, attr, /)\n"
"--\n"
"\n"
"Remove attribute attr from the \"background\" set.");

#define _CURSES_WINDOW_ATTROFF_METHODDEF    \
    {"attroff", (PyCFunction)_curses_window_attroff, METH_O, _curses_window_attroff__doc__},

static PyObject *
_curses_window_attroff_impl(PyCursesWindowObject *self, long attr);

static PyObject *
_curses_window_attroff(PyCursesWindowObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    long attr;

    attr = PyLong_AsLong(arg);
    if (attr == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_window_attroff_impl(self, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_attron__doc__,
"attron($self, attr, /)\n"
"--\n"
"\n"
"Add attribute attr from the \"background\" set.");

#define _CURSES_WINDOW_ATTRON_METHODDEF    \
    {"attron", (PyCFunction)_curses_window_attron, METH_O, _curses_window_attron__doc__},

static PyObject *
_curses_window_attron_impl(PyCursesWindowObject *self, long attr);

static PyObject *
_curses_window_attron(PyCursesWindowObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    long attr;

    attr = PyLong_AsLong(arg);
    if (attr == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_window_attron_impl(self, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_attrset__doc__,
"attrset($self, attr, /)\n"
"--\n"
"\n"
"Set the \"background\" set of attributes.");

#define _CURSES_WINDOW_ATTRSET_METHODDEF    \
    {"attrset", (PyCFunction)_curses_window_attrset, METH_O, _curses_window_attrset__doc__},

static PyObject *
_curses_window_attrset_impl(PyCursesWindowObject *self, long attr);

static PyObject *
_curses_window_attrset(PyCursesWindowObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    long attr;

    attr = PyLong_AsLong(arg);
    if (attr == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_window_attrset_impl(self, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_bkgdset__doc__,
"bkgdset($self, ch, attr=_curses.A_NORMAL, /)\n"
"--\n"
"\n"
"Set the window\'s background.\n"
"\n"
"  ch\n"
"    Background character.\n"
"  attr\n"
"    Background attributes.");

#define _CURSES_WINDOW_BKGDSET_METHODDEF    \
    {"bkgdset", (PyCFunction)(void(*)(void))_curses_window_bkgdset, METH_FASTCALL, _curses_window_bkgdset__doc__},

static PyObject *
_curses_window_bkgdset_impl(PyCursesWindowObject *self, PyObject *ch,
                            long attr);

static PyObject *
_curses_window_bkgdset(PyCursesWindowObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *ch;
    long attr = A_NORMAL;

    if (!_PyArg_CheckPositional("bkgdset", nargs, 1, 2)) {
        goto exit;
    }
    ch = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    attr = PyLong_AsLong(args[1]);
    if (attr == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _curses_window_bkgdset_impl(self, ch, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_border__doc__,
"border($self, ls=_curses.ACS_VLINE, rs=_curses.ACS_VLINE,\n"
"       ts=_curses.ACS_HLINE, bs=_curses.ACS_HLINE,\n"
"       tl=_curses.ACS_ULCORNER, tr=_curses.ACS_URCORNER,\n"
"       bl=_curses.ACS_LLCORNER, br=_curses.ACS_LRCORNER, /)\n"
"--\n"
"\n"
"Draw a border around the edges of the window.\n"
"\n"
"  ls\n"
"    Left side.\n"
"  rs\n"
"    Right side.\n"
"  ts\n"
"    Top side.\n"
"  bs\n"
"    Bottom side.\n"
"  tl\n"
"    Upper-left corner.\n"
"  tr\n"
"    Upper-right corner.\n"
"  bl\n"
"    Bottom-left corner.\n"
"  br\n"
"    Bottom-right corner.\n"
"\n"
"Each parameter specifies the character to use for a specific part of the\n"
"border.  The characters can be specified as integers or as one-character\n"
"strings.  A 0 value for any parameter will cause the default character to be\n"
"used for that parameter.");

#define _CURSES_WINDOW_BORDER_METHODDEF    \
    {"border", (PyCFunction)(void(*)(void))_curses_window_border, METH_FASTCALL, _curses_window_border__doc__},

static PyObject *
_curses_window_border_impl(PyCursesWindowObject *self, PyObject *ls,
                           PyObject *rs, PyObject *ts, PyObject *bs,
                           PyObject *tl, PyObject *tr, PyObject *bl,
                           PyObject *br);

static PyObject *
_curses_window_border(PyCursesWindowObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *ls = NULL;
    PyObject *rs = NULL;
    PyObject *ts = NULL;
    PyObject *bs = NULL;
    PyObject *tl = NULL;
    PyObject *tr = NULL;
    PyObject *bl = NULL;
    PyObject *br = NULL;

    if (!_PyArg_CheckPositional("border", nargs, 0, 8)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    ls = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    rs = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    ts = args[2];
    if (nargs < 4) {
        goto skip_optional;
    }
    bs = args[3];
    if (nargs < 5) {
        goto skip_optional;
    }
    tl = args[4];
    if (nargs < 6) {
        goto skip_optional;
    }
    tr = args[5];
    if (nargs < 7) {
        goto skip_optional;
    }
    bl = args[6];
    if (nargs < 8) {
        goto skip_optional;
    }
    br = args[7];
skip_optional:
    return_value = _curses_window_border_impl(self, ls, rs, ts, bs, tl, tr, bl, br);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_box__doc__,
"box([verch=0, horch=0])\n"
"Draw a border around the edges of the window.\n"
"\n"
"  verch\n"
"    Left and right side.\n"
"  horch\n"
"    Top and bottom side.\n"
"\n"
"Similar to border(), but both ls and rs are verch and both ts and bs are\n"
"horch.  The default corner characters are always used by this function.");

#define _CURSES_WINDOW_BOX_METHODDEF    \
    {"box", (PyCFunction)_curses_window_box, METH_VARARGS, _curses_window_box__doc__},

static PyObject *
_curses_window_box_impl(PyCursesWindowObject *self, int group_right_1,
                        PyObject *verch, PyObject *horch);

static PyObject *
_curses_window_box(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    PyObject *verch = _PyLong_GetZero();
    PyObject *horch = _PyLong_GetZero();

    switch (PyTuple_GET_SIZE(args)) {
        case 0:
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "OO:box", &verch, &horch)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.box requires 0 to 2 arguments");
            goto exit;
    }
    return_value = _curses_window_box_impl(self, group_right_1, verch, horch);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_delch__doc__,
"delch([y, x])\n"
"Delete any character at (y, x).\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.");

#define _CURSES_WINDOW_DELCH_METHODDEF    \
    {"delch", (PyCFunction)_curses_window_delch, METH_VARARGS, _curses_window_delch__doc__},

static PyObject *
_curses_window_delch_impl(PyCursesWindowObject *self, int group_right_1,
                          int y, int x);

static PyObject *
_curses_window_delch(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int y = 0;
    int x = 0;

    switch (PyTuple_GET_SIZE(args)) {
        case 0:
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "ii:delch", &y, &x)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.delch requires 0 to 2 arguments");
            goto exit;
    }
    return_value = _curses_window_delch_impl(self, group_right_1, y, x);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_derwin__doc__,
"derwin([nlines=0, ncols=0,] begin_y, begin_x)\n"
"Create a sub-window (window-relative coordinates).\n"
"\n"
"  nlines\n"
"    Height.\n"
"  ncols\n"
"    Width.\n"
"  begin_y\n"
"    Top side y-coordinate.\n"
"  begin_x\n"
"    Left side x-coordinate.\n"
"\n"
"derwin() is the same as calling subwin(), except that begin_y and begin_x\n"
"are relative to the origin of the window, rather than relative to the entire\n"
"screen.");

#define _CURSES_WINDOW_DERWIN_METHODDEF    \
    {"derwin", (PyCFunction)_curses_window_derwin, METH_VARARGS, _curses_window_derwin__doc__},

static PyObject *
_curses_window_derwin_impl(PyCursesWindowObject *self, int group_left_1,
                           int nlines, int ncols, int begin_y, int begin_x);

static PyObject *
_curses_window_derwin(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int nlines = 0;
    int ncols = 0;
    int begin_y;
    int begin_x;

    switch (PyTuple_GET_SIZE(args)) {
        case 2:
            if (!PyArg_ParseTuple(args, "ii:derwin", &begin_y, &begin_x)) {
                goto exit;
            }
            break;
        case 4:
            if (!PyArg_ParseTuple(args, "iiii:derwin", &nlines, &ncols, &begin_y, &begin_x)) {
                goto exit;
            }
            group_left_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.derwin requires 2 to 4 arguments");
            goto exit;
    }
    return_value = _curses_window_derwin_impl(self, group_left_1, nlines, ncols, begin_y, begin_x);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_echochar__doc__,
"echochar($self, ch, attr=_curses.A_NORMAL, /)\n"
"--\n"
"\n"
"Add character ch with attribute attr, and refresh.\n"
"\n"
"  ch\n"
"    Character to add.\n"
"  attr\n"
"    Attributes for the character.");

#define _CURSES_WINDOW_ECHOCHAR_METHODDEF    \
    {"echochar", (PyCFunction)(void(*)(void))_curses_window_echochar, METH_FASTCALL, _curses_window_echochar__doc__},

static PyObject *
_curses_window_echochar_impl(PyCursesWindowObject *self, PyObject *ch,
                             long attr);

static PyObject *
_curses_window_echochar(PyCursesWindowObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *ch;
    long attr = A_NORMAL;

    if (!_PyArg_CheckPositional("echochar", nargs, 1, 2)) {
        goto exit;
    }
    ch = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    attr = PyLong_AsLong(args[1]);
    if (attr == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _curses_window_echochar_impl(self, ch, attr);

exit:
    return return_value;
}

#if defined(NCURSES_MOUSE_VERSION)

PyDoc_STRVAR(_curses_window_enclose__doc__,
"enclose($self, y, x, /)\n"
"--\n"
"\n"
"Return True if the screen-relative coordinates are enclosed by the window.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.");

#define _CURSES_WINDOW_ENCLOSE_METHODDEF    \
    {"enclose", (PyCFunction)(void(*)(void))_curses_window_enclose, METH_FASTCALL, _curses_window_enclose__doc__},

static PyObject *
_curses_window_enclose_impl(PyCursesWindowObject *self, int y, int x);

static PyObject *
_curses_window_enclose(PyCursesWindowObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int y;
    int x;

    if (!_PyArg_CheckPositional("enclose", nargs, 2, 2)) {
        goto exit;
    }
    y = _PyLong_AsInt(args[0]);
    if (y == -1 && PyErr_Occurred()) {
        goto exit;
    }
    x = _PyLong_AsInt(args[1]);
    if (x == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_window_enclose_impl(self, y, x);

exit:
    return return_value;
}

#endif /* defined(NCURSES_MOUSE_VERSION) */

PyDoc_STRVAR(_curses_window_getbkgd__doc__,
"getbkgd($self, /)\n"
"--\n"
"\n"
"Return the window\'s current background character/attribute pair.");

#define _CURSES_WINDOW_GETBKGD_METHODDEF    \
    {"getbkgd", (PyCFunction)_curses_window_getbkgd, METH_NOARGS, _curses_window_getbkgd__doc__},

static long
_curses_window_getbkgd_impl(PyCursesWindowObject *self);

static PyObject *
_curses_window_getbkgd(PyCursesWindowObject *self, PyObject *Py_UNUSED(ignored))
{
    PyObject *return_value = NULL;
    long _return_value;

    _return_value = _curses_window_getbkgd_impl(self);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_getch__doc__,
"getch([y, x])\n"
"Get a character code from terminal keyboard.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"\n"
"The integer returned does not have to be in ASCII range: function keys,\n"
"keypad keys and so on return numbers higher than 256.  In no-delay mode, -1\n"
"is returned if there is no input, else getch() waits until a key is pressed.");

#define _CURSES_WINDOW_GETCH_METHODDEF    \
    {"getch", (PyCFunction)_curses_window_getch, METH_VARARGS, _curses_window_getch__doc__},

static int
_curses_window_getch_impl(PyCursesWindowObject *self, int group_right_1,
                          int y, int x);

static PyObject *
_curses_window_getch(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int y = 0;
    int x = 0;
    int _return_value;

    switch (PyTuple_GET_SIZE(args)) {
        case 0:
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "ii:getch", &y, &x)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.getch requires 0 to 2 arguments");
            goto exit;
    }
    _return_value = _curses_window_getch_impl(self, group_right_1, y, x);
    if ((_return_value == -1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromLong((long)_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_getkey__doc__,
"getkey([y, x])\n"
"Get a character (string) from terminal keyboard.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"\n"
"Returning a string instead of an integer, as getch() does.  Function keys,\n"
"keypad keys and other special keys return a multibyte string containing the\n"
"key name.  In no-delay mode, an exception is raised if there is no input.");

#define _CURSES_WINDOW_GETKEY_METHODDEF    \
    {"getkey", (PyCFunction)_curses_window_getkey, METH_VARARGS, _curses_window_getkey__doc__},

static PyObject *
_curses_window_getkey_impl(PyCursesWindowObject *self, int group_right_1,
                           int y, int x);

static PyObject *
_curses_window_getkey(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int y = 0;
    int x = 0;

    switch (PyTuple_GET_SIZE(args)) {
        case 0:
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "ii:getkey", &y, &x)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.getkey requires 0 to 2 arguments");
            goto exit;
    }
    return_value = _curses_window_getkey_impl(self, group_right_1, y, x);

exit:
    return return_value;
}

#if defined(HAVE_NCURSESW)

PyDoc_STRVAR(_curses_window_get_wch__doc__,
"get_wch([y, x])\n"
"Get a wide character from terminal keyboard.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"\n"
"Return a character for most keys, or an integer for function keys,\n"
"keypad keys, and other special keys.");

#define _CURSES_WINDOW_GET_WCH_METHODDEF    \
    {"get_wch", (PyCFunction)_curses_window_get_wch, METH_VARARGS, _curses_window_get_wch__doc__},

static PyObject *
_curses_window_get_wch_impl(PyCursesWindowObject *self, int group_right_1,
                            int y, int x);

static PyObject *
_curses_window_get_wch(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int y = 0;
    int x = 0;

    switch (PyTuple_GET_SIZE(args)) {
        case 0:
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "ii:get_wch", &y, &x)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.get_wch requires 0 to 2 arguments");
            goto exit;
    }
    return_value = _curses_window_get_wch_impl(self, group_right_1, y, x);

exit:
    return return_value;
}

#endif /* defined(HAVE_NCURSESW) */

PyDoc_STRVAR(_curses_window_hline__doc__,
"hline([y, x,] ch, n, [attr=_curses.A_NORMAL])\n"
"Display a horizontal line.\n"
"\n"
"  y\n"
"    Starting Y-coordinate.\n"
"  x\n"
"    Starting X-coordinate.\n"
"  ch\n"
"    Character to draw.\n"
"  n\n"
"    Line length.\n"
"  attr\n"
"    Attributes for the characters.");

#define _CURSES_WINDOW_HLINE_METHODDEF    \
    {"hline", (PyCFunction)_curses_window_hline, METH_VARARGS, _curses_window_hline__doc__},

static PyObject *
_curses_window_hline_impl(PyCursesWindowObject *self, int group_left_1,
                          int y, int x, PyObject *ch, int n,
                          int group_right_1, long attr);

static PyObject *
_curses_window_hline(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *ch;
    int n;
    int group_right_1 = 0;
    long attr = A_NORMAL;

    switch (PyTuple_GET_SIZE(args)) {
        case 2:
            if (!PyArg_ParseTuple(args, "Oi:hline", &ch, &n)) {
                goto exit;
            }
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "Oil:hline", &ch, &n, &attr)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        case 4:
            if (!PyArg_ParseTuple(args, "iiOi:hline", &y, &x, &ch, &n)) {
                goto exit;
            }
            group_left_1 = 1;
            break;
        case 5:
            if (!PyArg_ParseTuple(args, "iiOil:hline", &y, &x, &ch, &n, &attr)) {
                goto exit;
            }
            group_right_1 = 1;
            group_left_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.hline requires 2 to 5 arguments");
            goto exit;
    }
    return_value = _curses_window_hline_impl(self, group_left_1, y, x, ch, n, group_right_1, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_insch__doc__,
"insch([y, x,] ch, [attr=_curses.A_NORMAL])\n"
"Insert a character before the current or specified position.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  ch\n"
"    Character to insert.\n"
"  attr\n"
"    Attributes for the character.\n"
"\n"
"All characters to the right of the cursor are shifted one position right, with\n"
"the rightmost characters on the line being lost.");

#define _CURSES_WINDOW_INSCH_METHODDEF    \
    {"insch", (PyCFunction)_curses_window_insch, METH_VARARGS, _curses_window_insch__doc__},

static PyObject *
_curses_window_insch_impl(PyCursesWindowObject *self, int group_left_1,
                          int y, int x, PyObject *ch, int group_right_1,
                          long attr);

static PyObject *
_curses_window_insch(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *ch;
    int group_right_1 = 0;
    long attr = A_NORMAL;

    switch (PyTuple_GET_SIZE(args)) {
        case 1:
            if (!PyArg_ParseTuple(args, "O:insch", &ch)) {
                goto exit;
            }
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "Ol:insch", &ch, &attr)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "iiO:insch", &y, &x, &ch)) {
                goto exit;
            }
            group_left_1 = 1;
            break;
        case 4:
            if (!PyArg_ParseTuple(args, "iiOl:insch", &y, &x, &ch, &attr)) {
                goto exit;
            }
            group_right_1 = 1;
            group_left_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.insch requires 1 to 4 arguments");
            goto exit;
    }
    return_value = _curses_window_insch_impl(self, group_left_1, y, x, ch, group_right_1, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_inch__doc__,
"inch([y, x])\n"
"Return the character at the given position in the window.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"\n"
"The bottom 8 bits are the character proper, and upper bits are the attributes.");

#define _CURSES_WINDOW_INCH_METHODDEF    \
    {"inch", (PyCFunction)_curses_window_inch, METH_VARARGS, _curses_window_inch__doc__},

static unsigned long
_curses_window_inch_impl(PyCursesWindowObject *self, int group_right_1,
                         int y, int x);

static PyObject *
_curses_window_inch(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int y = 0;
    int x = 0;
    unsigned long _return_value;

    switch (PyTuple_GET_SIZE(args)) {
        case 0:
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "ii:inch", &y, &x)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.inch requires 0 to 2 arguments");
            goto exit;
    }
    _return_value = _curses_window_inch_impl(self, group_right_1, y, x);
    if ((_return_value == (unsigned long)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromUnsignedLong(_return_value);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_insstr__doc__,
"insstr([y, x,] str, [attr])\n"
"Insert the string before the current or specified position.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  str\n"
"    String to insert.\n"
"  attr\n"
"    Attributes for characters.\n"
"\n"
"Insert a character string (as many characters as will fit on the line)\n"
"before the character under the cursor.  All characters to the right of\n"
"the cursor are shifted right, with the rightmost characters on the line\n"
"being lost.  The cursor position does not change (after moving to y, x,\n"
"if specified).");

#define _CURSES_WINDOW_INSSTR_METHODDEF    \
    {"insstr", (PyCFunction)_curses_window_insstr, METH_VARARGS, _curses_window_insstr__doc__},

static PyObject *
_curses_window_insstr_impl(PyCursesWindowObject *self, int group_left_1,
                           int y, int x, PyObject *str, int group_right_1,
                           long attr);

static PyObject *
_curses_window_insstr(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *str;
    int group_right_1 = 0;
    long attr = 0;

    switch (PyTuple_GET_SIZE(args)) {
        case 1:
            if (!PyArg_ParseTuple(args, "O:insstr", &str)) {
                goto exit;
            }
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "Ol:insstr", &str, &attr)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "iiO:insstr", &y, &x, &str)) {
                goto exit;
            }
            group_left_1 = 1;
            break;
        case 4:
            if (!PyArg_ParseTuple(args, "iiOl:insstr", &y, &x, &str, &attr)) {
                goto exit;
            }
            group_right_1 = 1;
            group_left_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.insstr requires 1 to 4 arguments");
            goto exit;
    }
    return_value = _curses_window_insstr_impl(self, group_left_1, y, x, str, group_right_1, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_insnstr__doc__,
"insnstr([y, x,] str, n, [attr])\n"
"Insert at most n characters of the string.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"  str\n"
"    String to insert.\n"
"  n\n"
"    Maximal number of characters.\n"
"  attr\n"
"    Attributes for characters.\n"
"\n"
"Insert a character string (as many characters as will fit on the line)\n"
"before the character under the cursor, up to n characters.  If n is zero\n"
"or negative, the entire string is inserted.  All characters to the right\n"
"of the cursor are shifted right, with the rightmost characters on the line\n"
"being lost.  The cursor position does not change (after moving to y, x, if\n"
"specified).");

#define _CURSES_WINDOW_INSNSTR_METHODDEF    \
    {"insnstr", (PyCFunction)_curses_window_insnstr, METH_VARARGS, _curses_window_insnstr__doc__},

static PyObject *
_curses_window_insnstr_impl(PyCursesWindowObject *self, int group_left_1,
                            int y, int x, PyObject *str, int n,
                            int group_right_1, long attr);

static PyObject *
_curses_window_insnstr(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *str;
    int n;
    int group_right_1 = 0;
    long attr = 0;

    switch (PyTuple_GET_SIZE(args)) {
        case 2:
            if (!PyArg_ParseTuple(args, "Oi:insnstr", &str, &n)) {
                goto exit;
            }
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "Oil:insnstr", &str, &n, &attr)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        case 4:
            if (!PyArg_ParseTuple(args, "iiOi:insnstr", &y, &x, &str, &n)) {
                goto exit;
            }
            group_left_1 = 1;
            break;
        case 5:
            if (!PyArg_ParseTuple(args, "iiOil:insnstr", &y, &x, &str, &n, &attr)) {
                goto exit;
            }
            group_right_1 = 1;
            group_left_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.insnstr requires 2 to 5 arguments");
            goto exit;
    }
    return_value = _curses_window_insnstr_impl(self, group_left_1, y, x, str, n, group_right_1, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_is_linetouched__doc__,
"is_linetouched($self, line, /)\n"
"--\n"
"\n"
"Return True if the specified line was modified, otherwise return False.\n"
"\n"
"  line\n"
"    Line number.\n"
"\n"
"Raise a curses.error exception if line is not valid for the given window.");

#define _CURSES_WINDOW_IS_LINETOUCHED_METHODDEF    \
    {"is_linetouched", (PyCFunction)_curses_window_is_linetouched, METH_O, _curses_window_is_linetouched__doc__},

static PyObject *
_curses_window_is_linetouched_impl(PyCursesWindowObject *self, int line);

static PyObject *
_curses_window_is_linetouched(PyCursesWindowObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int line;

    line = _PyLong_AsInt(arg);
    if (line == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_window_is_linetouched_impl(self, line);

exit:
    return return_value;
}

#if defined(py_is_pad)

PyDoc_STRVAR(_curses_window_noutrefresh__doc__,
"noutrefresh([pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol])\n"
"Mark for refresh but wait.\n"
"\n"
"This function updates the data structure representing the desired state of the\n"
"window, but does not force an update of the physical screen.  To accomplish\n"
"that, call doupdate().");

#define _CURSES_WINDOW_NOUTREFRESH_METHODDEF    \
    {"noutrefresh", (PyCFunction)_curses_window_noutrefresh, METH_VARARGS, _curses_window_noutrefresh__doc__},

static PyObject *
_curses_window_noutrefresh_impl(PyCursesWindowObject *self,
                                int group_right_1, int pminrow, int pmincol,
                                int sminrow, int smincol, int smaxrow,
                                int smaxcol);

static PyObject *
_curses_window_noutrefresh(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int pminrow = 0;
    int pmincol = 0;
    int sminrow = 0;
    int smincol = 0;
    int smaxrow = 0;
    int smaxcol = 0;

    switch (PyTuple_GET_SIZE(args)) {
        case 0:
            break;
        case 6:
            if (!PyArg_ParseTuple(args, "iiiiii:noutrefresh", &pminrow, &pmincol, &sminrow, &smincol, &smaxrow, &smaxcol)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.noutrefresh requires 0 to 6 arguments");
            goto exit;
    }
    return_value = _curses_window_noutrefresh_impl(self, group_right_1, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol);

exit:
    return return_value;
}

#endif /* defined(py_is_pad) */

#if !defined(py_is_pad)

PyDoc_STRVAR(_curses_window_noutrefresh__doc__,
"noutrefresh($self, /)\n"
"--\n"
"\n"
"Mark for refresh but wait.\n"
"\n"
"This function updates the data structure representing the desired state of the\n"
"window, but does not force an update of the physical screen.  To accomplish\n"
"that, call doupdate().");

#define _CURSES_WINDOW_NOUTREFRESH_METHODDEF    \
    {"noutrefresh", (PyCFunction)_curses_window_noutrefresh, METH_NOARGS, _curses_window_noutrefresh__doc__},

static PyObject *
_curses_window_noutrefresh_impl(PyCursesWindowObject *self);

static PyObject *
_curses_window_noutrefresh(PyCursesWindowObject *self, PyObject *Py_UNUSED(ignored))
{
    return _curses_window_noutrefresh_impl(self);
}

#endif /* !defined(py_is_pad) */

PyDoc_STRVAR(_curses_window_overlay__doc__,
"overlay(destwin, [sminrow, smincol, dminrow, dmincol, dmaxrow, dmaxcol])\n"
"Overlay the window on top of destwin.\n"
"\n"
"The windows need not be the same size, only the overlapping region is copied.\n"
"This copy is non-destructive, which means that the current background\n"
"character does not overwrite the old contents of destwin.\n"
"\n"
"To get fine-grained control over the copied region, the second form of\n"
"overlay() can be used.  sminrow and smincol are the upper-left coordinates\n"
"of the source window, and the other variables mark a rectangle in the\n"
"destination window.");

#define _CURSES_WINDOW_OVERLAY_METHODDEF    \
    {"overlay", (PyCFunction)_curses_window_overlay, METH_VARARGS, _curses_window_overlay__doc__},

static PyObject *
_curses_window_overlay_impl(PyCursesWindowObject *self,
                            PyCursesWindowObject *destwin, int group_right_1,
                            int sminrow, int smincol, int dminrow,
                            int dmincol, int dmaxrow, int dmaxcol);

static PyObject *
_curses_window_overlay(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyCursesWindowObject *destwin;
    int group_right_1 = 0;
    int sminrow = 0;
    int smincol = 0;
    int dminrow = 0;
    int dmincol = 0;
    int dmaxrow = 0;
    int dmaxcol = 0;

    switch (PyTuple_GET_SIZE(args)) {
        case 1:
            if (!PyArg_ParseTuple(args, "O!:overlay", &PyCursesWindow_Type, &destwin)) {
                goto exit;
            }
            break;
        case 7:
            if (!PyArg_ParseTuple(args, "O!iiiiii:overlay", &PyCursesWindow_Type, &destwin, &sminrow, &smincol, &dminrow, &dmincol, &dmaxrow, &dmaxcol)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.overlay requires 1 to 7 arguments");
            goto exit;
    }
    return_value = _curses_window_overlay_impl(self, destwin, group_right_1, sminrow, smincol, dminrow, dmincol, dmaxrow, dmaxcol);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_overwrite__doc__,
"overwrite(destwin, [sminrow, smincol, dminrow, dmincol, dmaxrow,\n"
"          dmaxcol])\n"
"Overwrite the window on top of destwin.\n"
"\n"
"The windows need not be the same size, in which case only the overlapping\n"
"region is copied.  This copy is destructive, which means that the current\n"
"background character overwrites the old contents of destwin.\n"
"\n"
"To get fine-grained control over the copied region, the second form of\n"
"overwrite() can be used. sminrow and smincol are the upper-left coordinates\n"
"of the source window, the other variables mark a rectangle in the destination\n"
"window.");

#define _CURSES_WINDOW_OVERWRITE_METHODDEF    \
    {"overwrite", (PyCFunction)_curses_window_overwrite, METH_VARARGS, _curses_window_overwrite__doc__},

static PyObject *
_curses_window_overwrite_impl(PyCursesWindowObject *self,
                              PyCursesWindowObject *destwin,
                              int group_right_1, int sminrow, int smincol,
                              int dminrow, int dmincol, int dmaxrow,
                              int dmaxcol);

static PyObject *
_curses_window_overwrite(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyCursesWindowObject *destwin;
    int group_right_1 = 0;
    int sminrow = 0;
    int smincol = 0;
    int dminrow = 0;
    int dmincol = 0;
    int dmaxrow = 0;
    int dmaxcol = 0;

    switch (PyTuple_GET_SIZE(args)) {
        case 1:
            if (!PyArg_ParseTuple(args, "O!:overwrite", &PyCursesWindow_Type, &destwin)) {
                goto exit;
            }
            break;
        case 7:
            if (!PyArg_ParseTuple(args, "O!iiiiii:overwrite", &PyCursesWindow_Type, &destwin, &sminrow, &smincol, &dminrow, &dmincol, &dmaxrow, &dmaxcol)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.overwrite requires 1 to 7 arguments");
            goto exit;
    }
    return_value = _curses_window_overwrite_impl(self, destwin, group_right_1, sminrow, smincol, dminrow, dmincol, dmaxrow, dmaxcol);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_putwin__doc__,
"putwin($self, file, /)\n"
"--\n"
"\n"
"Write all data associated with the window into the provided file object.\n"
"\n"
"This information can be later retrieved using the getwin() function.");

#define _CURSES_WINDOW_PUTWIN_METHODDEF    \
    {"putwin", (PyCFunction)_curses_window_putwin, METH_O, _curses_window_putwin__doc__},

PyDoc_STRVAR(_curses_window_redrawln__doc__,
"redrawln($self, beg, num, /)\n"
"--\n"
"\n"
"Mark the specified lines corrupted.\n"
"\n"
"  beg\n"
"    Starting line number.\n"
"  num\n"
"    The number of lines.\n"
"\n"
"They should be completely redrawn on the next refresh() call.");

#define _CURSES_WINDOW_REDRAWLN_METHODDEF    \
    {"redrawln", (PyCFunction)(void(*)(void))_curses_window_redrawln, METH_FASTCALL, _curses_window_redrawln__doc__},

static PyObject *
_curses_window_redrawln_impl(PyCursesWindowObject *self, int beg, int num);

static PyObject *
_curses_window_redrawln(PyCursesWindowObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int beg;
    int num;

    if (!_PyArg_CheckPositional("redrawln", nargs, 2, 2)) {
        goto exit;
    }
    beg = _PyLong_AsInt(args[0]);
    if (beg == -1 && PyErr_Occurred()) {
        goto exit;
    }
    num = _PyLong_AsInt(args[1]);
    if (num == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_window_redrawln_impl(self, beg, num);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_refresh__doc__,
"refresh([pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol])\n"
"Update the display immediately.\n"
"\n"
"Synchronize actual screen with previous drawing/deleting methods.\n"
"The 6 optional arguments can only be specified when the window is a pad\n"
"created with newpad().  The additional parameters are needed to indicate\n"
"what part of the pad and screen are involved.  pminrow and pmincol specify\n"
"the upper left-hand corner of the rectangle to be displayed in the pad.\n"
"sminrow, smincol, smaxrow, and smaxcol specify the edges of the rectangle to\n"
"be displayed on the screen.  The lower right-hand corner of the rectangle to\n"
"be displayed in the pad is calculated from the screen coordinates, since the\n"
"rectangles must be the same size.  Both rectangles must be entirely contained\n"
"within their respective structures.  Negative values of pminrow, pmincol,\n"
"sminrow, or smincol are treated as if they were zero.");

#define _CURSES_WINDOW_REFRESH_METHODDEF    \
    {"refresh", (PyCFunction)_curses_window_refresh, METH_VARARGS, _curses_window_refresh__doc__},

static PyObject *
_curses_window_refresh_impl(PyCursesWindowObject *self, int group_right_1,
                            int pminrow, int pmincol, int sminrow,
                            int smincol, int smaxrow, int smaxcol);

static PyObject *
_curses_window_refresh(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int pminrow = 0;
    int pmincol = 0;
    int sminrow = 0;
    int smincol = 0;
    int smaxrow = 0;
    int smaxcol = 0;

    switch (PyTuple_GET_SIZE(args)) {
        case 0:
            break;
        case 6:
            if (!PyArg_ParseTuple(args, "iiiiii:refresh", &pminrow, &pmincol, &sminrow, &smincol, &smaxrow, &smaxcol)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.refresh requires 0 to 6 arguments");
            goto exit;
    }
    return_value = _curses_window_refresh_impl(self, group_right_1, pminrow, pmincol, sminrow, smincol, smaxrow, smaxcol);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_setscrreg__doc__,
"setscrreg($self, top, bottom, /)\n"
"--\n"
"\n"
"Define a software scrolling region.\n"
"\n"
"  top\n"
"    First line number.\n"
"  bottom\n"
"    Last line number.\n"
"\n"
"All scrolling actions will take place in this region.");

#define _CURSES_WINDOW_SETSCRREG_METHODDEF    \
    {"setscrreg", (PyCFunction)(void(*)(void))_curses_window_setscrreg, METH_FASTCALL, _curses_window_setscrreg__doc__},

static PyObject *
_curses_window_setscrreg_impl(PyCursesWindowObject *self, int top,
                              int bottom);

static PyObject *
_curses_window_setscrreg(PyCursesWindowObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int top;
    int bottom;

    if (!_PyArg_CheckPositional("setscrreg", nargs, 2, 2)) {
        goto exit;
    }
    top = _PyLong_AsInt(args[0]);
    if (top == -1 && PyErr_Occurred()) {
        goto exit;
    }
    bottom = _PyLong_AsInt(args[1]);
    if (bottom == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_window_setscrreg_impl(self, top, bottom);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_subwin__doc__,
"subwin([nlines=0, ncols=0,] begin_y, begin_x)\n"
"Create a sub-window (screen-relative coordinates).\n"
"\n"
"  nlines\n"
"    Height.\n"
"  ncols\n"
"    Width.\n"
"  begin_y\n"
"    Top side y-coordinate.\n"
"  begin_x\n"
"    Left side x-coordinate.\n"
"\n"
"By default, the sub-window will extend from the specified position to the\n"
"lower right corner of the window.");

#define _CURSES_WINDOW_SUBWIN_METHODDEF    \
    {"subwin", (PyCFunction)_curses_window_subwin, METH_VARARGS, _curses_window_subwin__doc__},

static PyObject *
_curses_window_subwin_impl(PyCursesWindowObject *self, int group_left_1,
                           int nlines, int ncols, int begin_y, int begin_x);

static PyObject *
_curses_window_subwin(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int nlines = 0;
    int ncols = 0;
    int begin_y;
    int begin_x;

    switch (PyTuple_GET_SIZE(args)) {
        case 2:
            if (!PyArg_ParseTuple(args, "ii:subwin", &begin_y, &begin_x)) {
                goto exit;
            }
            break;
        case 4:
            if (!PyArg_ParseTuple(args, "iiii:subwin", &nlines, &ncols, &begin_y, &begin_x)) {
                goto exit;
            }
            group_left_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.subwin requires 2 to 4 arguments");
            goto exit;
    }
    return_value = _curses_window_subwin_impl(self, group_left_1, nlines, ncols, begin_y, begin_x);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_scroll__doc__,
"scroll([lines=1])\n"
"Scroll the screen or scrolling region.\n"
"\n"
"  lines\n"
"    Number of lines to scroll.\n"
"\n"
"Scroll upward if the argument is positive and downward if it is negative.");

#define _CURSES_WINDOW_SCROLL_METHODDEF    \
    {"scroll", (PyCFunction)_curses_window_scroll, METH_VARARGS, _curses_window_scroll__doc__},

static PyObject *
_curses_window_scroll_impl(PyCursesWindowObject *self, int group_right_1,
                           int lines);

static PyObject *
_curses_window_scroll(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_right_1 = 0;
    int lines = 1;

    switch (PyTuple_GET_SIZE(args)) {
        case 0:
            break;
        case 1:
            if (!PyArg_ParseTuple(args, "i:scroll", &lines)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.scroll requires 0 to 1 arguments");
            goto exit;
    }
    return_value = _curses_window_scroll_impl(self, group_right_1, lines);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_touchline__doc__,
"touchline(start, count, [changed=True])\n"
"Pretend count lines have been changed, starting with line start.\n"
"\n"
"If changed is supplied, it specifies whether the affected lines are marked\n"
"as having been changed (changed=True) or unchanged (changed=False).");

#define _CURSES_WINDOW_TOUCHLINE_METHODDEF    \
    {"touchline", (PyCFunction)_curses_window_touchline, METH_VARARGS, _curses_window_touchline__doc__},

static PyObject *
_curses_window_touchline_impl(PyCursesWindowObject *self, int start,
                              int count, int group_right_1, int changed);

static PyObject *
_curses_window_touchline(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int start;
    int count;
    int group_right_1 = 0;
    int changed = 1;

    switch (PyTuple_GET_SIZE(args)) {
        case 2:
            if (!PyArg_ParseTuple(args, "ii:touchline", &start, &count)) {
                goto exit;
            }
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "iii:touchline", &start, &count, &changed)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.touchline requires 2 to 3 arguments");
            goto exit;
    }
    return_value = _curses_window_touchline_impl(self, start, count, group_right_1, changed);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_window_vline__doc__,
"vline([y, x,] ch, n, [attr=_curses.A_NORMAL])\n"
"Display a vertical line.\n"
"\n"
"  y\n"
"    Starting Y-coordinate.\n"
"  x\n"
"    Starting X-coordinate.\n"
"  ch\n"
"    Character to draw.\n"
"  n\n"
"    Line length.\n"
"  attr\n"
"    Attributes for the character.");

#define _CURSES_WINDOW_VLINE_METHODDEF    \
    {"vline", (PyCFunction)_curses_window_vline, METH_VARARGS, _curses_window_vline__doc__},

static PyObject *
_curses_window_vline_impl(PyCursesWindowObject *self, int group_left_1,
                          int y, int x, PyObject *ch, int n,
                          int group_right_1, long attr);

static PyObject *
_curses_window_vline(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *ch;
    int n;
    int group_right_1 = 0;
    long attr = A_NORMAL;

    switch (PyTuple_GET_SIZE(args)) {
        case 2:
            if (!PyArg_ParseTuple(args, "Oi:vline", &ch, &n)) {
                goto exit;
            }
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "Oil:vline", &ch, &n, &attr)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        case 4:
            if (!PyArg_ParseTuple(args, "iiOi:vline", &y, &x, &ch, &n)) {
                goto exit;
            }
            group_left_1 = 1;
            break;
        case 5:
            if (!PyArg_ParseTuple(args, "iiOil:vline", &y, &x, &ch, &n, &attr)) {
                goto exit;
            }
            group_right_1 = 1;
            group_left_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.window.vline requires 2 to 5 arguments");
            goto exit;
    }
    return_value = _curses_window_vline_impl(self, group_left_1, y, x, ch, n, group_right_1, attr);

exit:
    return return_value;
}

#if defined(HAVE_CURSES_FILTER)

PyDoc_STRVAR(_curses_filter__doc__,
"filter($module, /)\n"
"--\n"
"\n");

#define _CURSES_FILTER_METHODDEF    \
    {"filter", (PyCFunction)_curses_filter, METH_NOARGS, _curses_filter__doc__},

static PyObject *
_curses_filter_impl(PyObject *module);

static PyObject *
_curses_filter(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_filter_impl(module);
}

#endif /* defined(HAVE_CURSES_FILTER) */

PyDoc_STRVAR(_curses_baudrate__doc__,
"baudrate($module, /)\n"
"--\n"
"\n"
"Return the output speed of the terminal in bits per second.");

#define _CURSES_BAUDRATE_METHODDEF    \
    {"baudrate", (PyCFunction)_curses_baudrate, METH_NOARGS, _curses_baudrate__doc__},

static PyObject *
_curses_baudrate_impl(PyObject *module);

static PyObject *
_curses_baudrate(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_baudrate_impl(module);
}

PyDoc_STRVAR(_curses_beep__doc__,
"beep($module, /)\n"
"--\n"
"\n"
"Emit a short attention sound.");

#define _CURSES_BEEP_METHODDEF    \
    {"beep", (PyCFunction)_curses_beep, METH_NOARGS, _curses_beep__doc__},

static PyObject *
_curses_beep_impl(PyObject *module);

static PyObject *
_curses_beep(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_beep_impl(module);
}

PyDoc_STRVAR(_curses_can_change_color__doc__,
"can_change_color($module, /)\n"
"--\n"
"\n"
"Return True if the programmer can change the colors displayed by the terminal.");

#define _CURSES_CAN_CHANGE_COLOR_METHODDEF    \
    {"can_change_color", (PyCFunction)_curses_can_change_color, METH_NOARGS, _curses_can_change_color__doc__},

static PyObject *
_curses_can_change_color_impl(PyObject *module);

static PyObject *
_curses_can_change_color(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_can_change_color_impl(module);
}

PyDoc_STRVAR(_curses_cbreak__doc__,
"cbreak($module, flag=True, /)\n"
"--\n"
"\n"
"Enter cbreak mode.\n"
"\n"
"  flag\n"
"    If false, the effect is the same as calling nocbreak().\n"
"\n"
"In cbreak mode (sometimes called \"rare\" mode) normal tty line buffering is\n"
"turned off and characters are available to be read one by one.  However,\n"
"unlike raw mode, special characters (interrupt, quit, suspend, and flow\n"
"control) retain their effects on the tty driver and calling program.\n"
"Calling first raw() then cbreak() leaves the terminal in cbreak mode.");

#define _CURSES_CBREAK_METHODDEF    \
    {"cbreak", (PyCFunction)(void(*)(void))_curses_cbreak, METH_FASTCALL, _curses_cbreak__doc__},

static PyObject *
_curses_cbreak_impl(PyObject *module, int flag);

static PyObject *
_curses_cbreak(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int flag = 1;

    if (!_PyArg_CheckPositional("cbreak", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    flag = _PyLong_AsInt(args[0]);
    if (flag == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _curses_cbreak_impl(module, flag);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_color_content__doc__,
"color_content($module, color_number, /)\n"
"--\n"
"\n"
"Return the red, green, and blue (RGB) components of the specified color.\n"
"\n"
"  color_number\n"
"    The number of the color (0 - (COLORS-1)).\n"
"\n"
"A 3-tuple is returned, containing the R, G, B values for the given color,\n"
"which will be between 0 (no component) and 1000 (maximum amount of component).");

#define _CURSES_COLOR_CONTENT_METHODDEF    \
    {"color_content", (PyCFunction)_curses_color_content, METH_O, _curses_color_content__doc__},

static PyObject *
_curses_color_content_impl(PyObject *module, int color_number);

static PyObject *
_curses_color_content(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int color_number;

    if (!color_converter(arg, &color_number)) {
        goto exit;
    }
    return_value = _curses_color_content_impl(module, color_number);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_color_pair__doc__,
"color_pair($module, pair_number, /)\n"
"--\n"
"\n"
"Return the attribute value for displaying text in the specified color.\n"
"\n"
"  pair_number\n"
"    The number of the color pair.\n"
"\n"
"This attribute value can be combined with A_STANDOUT, A_REVERSE, and the\n"
"other A_* attributes.  pair_number() is the counterpart to this function.");

#define _CURSES_COLOR_PAIR_METHODDEF    \
    {"color_pair", (PyCFunction)_curses_color_pair, METH_O, _curses_color_pair__doc__},

static PyObject *
_curses_color_pair_impl(PyObject *module, int pair_number);

static PyObject *
_curses_color_pair(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int pair_number;

    pair_number = _PyLong_AsInt(arg);
    if (pair_number == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_color_pair_impl(module, pair_number);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_curs_set__doc__,
"curs_set($module, visibility, /)\n"
"--\n"
"\n"
"Set the cursor state.\n"
"\n"
"  visibility\n"
"    0 for invisible, 1 for normal visible, or 2 for very visible.\n"
"\n"
"If the terminal supports the visibility requested, the previous cursor\n"
"state is returned; otherwise, an exception is raised.  On many terminals,\n"
"the \"visible\" mode is an underline cursor and the \"very visible\" mode is\n"
"a block cursor.");

#define _CURSES_CURS_SET_METHODDEF    \
    {"curs_set", (PyCFunction)_curses_curs_set, METH_O, _curses_curs_set__doc__},

static PyObject *
_curses_curs_set_impl(PyObject *module, int visibility);

static PyObject *
_curses_curs_set(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int visibility;

    visibility = _PyLong_AsInt(arg);
    if (visibility == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_curs_set_impl(module, visibility);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_def_prog_mode__doc__,
"def_prog_mode($module, /)\n"
"--\n"
"\n"
"Save the current terminal mode as the \"program\" mode.\n"
"\n"
"The \"program\" mode is the mode when the running program is using curses.\n"
"\n"
"Subsequent calls to reset_prog_mode() will restore this mode.");

#define _CURSES_DEF_PROG_MODE_METHODDEF    \
    {"def_prog_mode", (PyCFunction)_curses_def_prog_mode, METH_NOARGS, _curses_def_prog_mode__doc__},

static PyObject *
_curses_def_prog_mode_impl(PyObject *module);

static PyObject *
_curses_def_prog_mode(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_def_prog_mode_impl(module);
}

PyDoc_STRVAR(_curses_def_shell_mode__doc__,
"def_shell_mode($module, /)\n"
"--\n"
"\n"
"Save the current terminal mode as the \"shell\" mode.\n"
"\n"
"The \"shell\" mode is the mode when the running program is not using curses.\n"
"\n"
"Subsequent calls to reset_shell_mode() will restore this mode.");

#define _CURSES_DEF_SHELL_MODE_METHODDEF    \
    {"def_shell_mode", (PyCFunction)_curses_def_shell_mode, METH_NOARGS, _curses_def_shell_mode__doc__},

static PyObject *
_curses_def_shell_mode_impl(PyObject *module);

static PyObject *
_curses_def_shell_mode(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_def_shell_mode_impl(module);
}

PyDoc_STRVAR(_curses_delay_output__doc__,
"delay_output($module, ms, /)\n"
"--\n"
"\n"
"Insert a pause in output.\n"
"\n"
"  ms\n"
"    Duration in milliseconds.");

#define _CURSES_DELAY_OUTPUT_METHODDEF    \
    {"delay_output", (PyCFunction)_curses_delay_output, METH_O, _curses_delay_output__doc__},

static PyObject *
_curses_delay_output_impl(PyObject *module, int ms);

static PyObject *
_curses_delay_output(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int ms;

    ms = _PyLong_AsInt(arg);
    if (ms == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_delay_output_impl(module, ms);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_doupdate__doc__,
"doupdate($module, /)\n"
"--\n"
"\n"
"Update the physical screen to match the virtual screen.");

#define _CURSES_DOUPDATE_METHODDEF    \
    {"doupdate", (PyCFunction)_curses_doupdate, METH_NOARGS, _curses_doupdate__doc__},

static PyObject *
_curses_doupdate_impl(PyObject *module);

static PyObject *
_curses_doupdate(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_doupdate_impl(module);
}

PyDoc_STRVAR(_curses_echo__doc__,
"echo($module, flag=True, /)\n"
"--\n"
"\n"
"Enter echo mode.\n"
"\n"
"  flag\n"
"    If false, the effect is the same as calling noecho().\n"
"\n"
"In echo mode, each character input is echoed to the screen as it is entered.");

#define _CURSES_ECHO_METHODDEF    \
    {"echo", (PyCFunction)(void(*)(void))_curses_echo, METH_FASTCALL, _curses_echo__doc__},

static PyObject *
_curses_echo_impl(PyObject *module, int flag);

static PyObject *
_curses_echo(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int flag = 1;

    if (!_PyArg_CheckPositional("echo", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    flag = _PyLong_AsInt(args[0]);
    if (flag == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _curses_echo_impl(module, flag);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_endwin__doc__,
"endwin($module, /)\n"
"--\n"
"\n"
"De-initialize the library, and return terminal to normal status.");

#define _CURSES_ENDWIN_METHODDEF    \
    {"endwin", (PyCFunction)_curses_endwin, METH_NOARGS, _curses_endwin__doc__},

static PyObject *
_curses_endwin_impl(PyObject *module);

static PyObject *
_curses_endwin(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_endwin_impl(module);
}

PyDoc_STRVAR(_curses_erasechar__doc__,
"erasechar($module, /)\n"
"--\n"
"\n"
"Return the user\'s current erase character.");

#define _CURSES_ERASECHAR_METHODDEF    \
    {"erasechar", (PyCFunction)_curses_erasechar, METH_NOARGS, _curses_erasechar__doc__},

static PyObject *
_curses_erasechar_impl(PyObject *module);

static PyObject *
_curses_erasechar(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_erasechar_impl(module);
}

PyDoc_STRVAR(_curses_flash__doc__,
"flash($module, /)\n"
"--\n"
"\n"
"Flash the screen.\n"
"\n"
"That is, change it to reverse-video and then change it back in a short interval.");

#define _CURSES_FLASH_METHODDEF    \
    {"flash", (PyCFunction)_curses_flash, METH_NOARGS, _curses_flash__doc__},

static PyObject *
_curses_flash_impl(PyObject *module);

static PyObject *
_curses_flash(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_flash_impl(module);
}

PyDoc_STRVAR(_curses_flushinp__doc__,
"flushinp($module, /)\n"
"--\n"
"\n"
"Flush all input buffers.\n"
"\n"
"This throws away any typeahead that has been typed by the user and has not\n"
"yet been processed by the program.");

#define _CURSES_FLUSHINP_METHODDEF    \
    {"flushinp", (PyCFunction)_curses_flushinp, METH_NOARGS, _curses_flushinp__doc__},

static PyObject *
_curses_flushinp_impl(PyObject *module);

static PyObject *
_curses_flushinp(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_flushinp_impl(module);
}

#if defined(getsyx)

PyDoc_STRVAR(_curses_getsyx__doc__,
"getsyx($module, /)\n"
"--\n"
"\n"
"Return the current coordinates of the virtual screen cursor.\n"
"\n"
"Return a (y, x) tuple.  If leaveok is currently true, return (-1, -1).");

#define _CURSES_GETSYX_METHODDEF    \
    {"getsyx", (PyCFunction)_curses_getsyx, METH_NOARGS, _curses_getsyx__doc__},

static PyObject *
_curses_getsyx_impl(PyObject *module);

static PyObject *
_curses_getsyx(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_getsyx_impl(module);
}

#endif /* defined(getsyx) */

#if defined(NCURSES_MOUSE_VERSION)

PyDoc_STRVAR(_curses_getmouse__doc__,
"getmouse($module, /)\n"
"--\n"
"\n"
"Retrieve the queued mouse event.\n"
"\n"
"After getch() returns KEY_MOUSE to signal a mouse event, this function\n"
"returns a 5-tuple (id, x, y, z, bstate).");

#define _CURSES_GETMOUSE_METHODDEF    \
    {"getmouse", (PyCFunction)_curses_getmouse, METH_NOARGS, _curses_getmouse__doc__},

static PyObject *
_curses_getmouse_impl(PyObject *module);

static PyObject *
_curses_getmouse(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_getmouse_impl(module);
}

#endif /* defined(NCURSES_MOUSE_VERSION) */

#if defined(NCURSES_MOUSE_VERSION)

PyDoc_STRVAR(_curses_ungetmouse__doc__,
"ungetmouse($module, id, x, y, z, bstate, /)\n"
"--\n"
"\n"
"Push a KEY_MOUSE event onto the input queue.\n"
"\n"
"The following getmouse() will return the given state data.");

#define _CURSES_UNGETMOUSE_METHODDEF    \
    {"ungetmouse", (PyCFunction)(void(*)(void))_curses_ungetmouse, METH_FASTCALL, _curses_ungetmouse__doc__},

static PyObject *
_curses_ungetmouse_impl(PyObject *module, short id, int x, int y, int z,
                        unsigned long bstate);

static PyObject *
_curses_ungetmouse(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    short id;
    int x;
    int y;
    int z;
    unsigned long bstate;

    if (!_PyArg_CheckPositional("ungetmouse", nargs, 5, 5)) {
        goto exit;
    }
    {
        long ival = PyLong_AsLong(args[0]);
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        else if (ival < SHRT_MIN) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed short integer is less than minimum");
            goto exit;
        }
        else if (ival > SHRT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "signed short integer is greater than maximum");
            goto exit;
        }
        else {
            id = (short) ival;
        }
    }
    x = _PyLong_AsInt(args[1]);
    if (x == -1 && PyErr_Occurred()) {
        goto exit;
    }
    y = _PyLong_AsInt(args[2]);
    if (y == -1 && PyErr_Occurred()) {
        goto exit;
    }
    z = _PyLong_AsInt(args[3]);
    if (z == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (!PyLong_Check(args[4])) {
        _PyArg_BadArgument("ungetmouse", "argument 5", "int", args[4]);
        goto exit;
    }
    bstate = PyLong_AsUnsignedLongMask(args[4]);
    return_value = _curses_ungetmouse_impl(module, id, x, y, z, bstate);

exit:
    return return_value;
}

#endif /* defined(NCURSES_MOUSE_VERSION) */

PyDoc_STRVAR(_curses_getwin__doc__,
"getwin($module, file, /)\n"
"--\n"
"\n"
"Read window related data stored in the file by an earlier putwin() call.\n"
"\n"
"The routine then creates and initializes a new window using that data,\n"
"returning the new window object.");

#define _CURSES_GETWIN_METHODDEF    \
    {"getwin", (PyCFunction)_curses_getwin, METH_O, _curses_getwin__doc__},

PyDoc_STRVAR(_curses_halfdelay__doc__,
"halfdelay($module, tenths, /)\n"
"--\n"
"\n"
"Enter half-delay mode.\n"
"\n"
"  tenths\n"
"    Maximal blocking delay in tenths of seconds (1 - 255).\n"
"\n"
"Use nocbreak() to leave half-delay mode.");

#define _CURSES_HALFDELAY_METHODDEF    \
    {"halfdelay", (PyCFunction)_curses_halfdelay, METH_O, _curses_halfdelay__doc__},

static PyObject *
_curses_halfdelay_impl(PyObject *module, unsigned char tenths);

static PyObject *
_curses_halfdelay(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    unsigned char tenths;

    {
        long ival = PyLong_AsLong(arg);
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        else if (ival < 0) {
            PyErr_SetString(PyExc_OverflowError,
                            "unsigned byte integer is less than minimum");
            goto exit;
        }
        else if (ival > UCHAR_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "unsigned byte integer is greater than maximum");
            goto exit;
        }
        else {
            tenths = (unsigned char) ival;
        }
    }
    return_value = _curses_halfdelay_impl(module, tenths);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_has_colors__doc__,
"has_colors($module, /)\n"
"--\n"
"\n"
"Return True if the terminal can display colors; otherwise, return False.");

#define _CURSES_HAS_COLORS_METHODDEF    \
    {"has_colors", (PyCFunction)_curses_has_colors, METH_NOARGS, _curses_has_colors__doc__},

static PyObject *
_curses_has_colors_impl(PyObject *module);

static PyObject *
_curses_has_colors(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_has_colors_impl(module);
}

PyDoc_STRVAR(_curses_has_ic__doc__,
"has_ic($module, /)\n"
"--\n"
"\n"
"Return True if the terminal has insert- and delete-character capabilities.");

#define _CURSES_HAS_IC_METHODDEF    \
    {"has_ic", (PyCFunction)_curses_has_ic, METH_NOARGS, _curses_has_ic__doc__},

static PyObject *
_curses_has_ic_impl(PyObject *module);

static PyObject *
_curses_has_ic(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_has_ic_impl(module);
}

PyDoc_STRVAR(_curses_has_il__doc__,
"has_il($module, /)\n"
"--\n"
"\n"
"Return True if the terminal has insert- and delete-line capabilities.");

#define _CURSES_HAS_IL_METHODDEF    \
    {"has_il", (PyCFunction)_curses_has_il, METH_NOARGS, _curses_has_il__doc__},

static PyObject *
_curses_has_il_impl(PyObject *module);

static PyObject *
_curses_has_il(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_has_il_impl(module);
}

#if defined(HAVE_CURSES_HAS_KEY)

PyDoc_STRVAR(_curses_has_key__doc__,
"has_key($module, key, /)\n"
"--\n"
"\n"
"Return True if the current terminal type recognizes a key with that value.\n"
"\n"
"  key\n"
"    Key number.");

#define _CURSES_HAS_KEY_METHODDEF    \
    {"has_key", (PyCFunction)_curses_has_key, METH_O, _curses_has_key__doc__},

static PyObject *
_curses_has_key_impl(PyObject *module, int key);

static PyObject *
_curses_has_key(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int key;

    key = _PyLong_AsInt(arg);
    if (key == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_has_key_impl(module, key);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_HAS_KEY) */

PyDoc_STRVAR(_curses_init_color__doc__,
"init_color($module, color_number, r, g, b, /)\n"
"--\n"
"\n"
"Change the definition of a color.\n"
"\n"
"  color_number\n"
"    The number of the color to be changed (0 - (COLORS-1)).\n"
"  r\n"
"    Red component (0 - 1000).\n"
"  g\n"
"    Green component (0 - 1000).\n"
"  b\n"
"    Blue component (0 - 1000).\n"
"\n"
"When init_color() is used, all occurrences of that color on the screen\n"
"immediately change to the new definition.  This function is a no-op on\n"
"most terminals; it is active only if can_change_color() returns true.");

#define _CURSES_INIT_COLOR_METHODDEF    \
    {"init_color", (PyCFunction)(void(*)(void))_curses_init_color, METH_FASTCALL, _curses_init_color__doc__},

static PyObject *
_curses_init_color_impl(PyObject *module, int color_number, short r, short g,
                        short b);

static PyObject *
_curses_init_color(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int color_number;
    short r;
    short g;
    short b;

    if (!_PyArg_CheckPositional("init_color", nargs, 4, 4)) {
        goto exit;
    }
    if (!color_converter(args[0], &color_number)) {
        goto exit;
    }
    if (!component_converter(args[1], &r)) {
        goto exit;
    }
    if (!component_converter(args[2], &g)) {
        goto exit;
    }
    if (!component_converter(args[3], &b)) {
        goto exit;
    }
    return_value = _curses_init_color_impl(module, color_number, r, g, b);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_init_pair__doc__,
"init_pair($module, pair_number, fg, bg, /)\n"
"--\n"
"\n"
"Change the definition of a color-pair.\n"
"\n"
"  pair_number\n"
"    The number of the color-pair to be changed (1 - (COLOR_PAIRS-1)).\n"
"  fg\n"
"    Foreground color number (-1 - (COLORS-1)).\n"
"  bg\n"
"    Background color number (-1 - (COLORS-1)).\n"
"\n"
"If the color-pair was previously initialized, the screen is refreshed and\n"
"all occurrences of that color-pair are changed to the new definition.");

#define _CURSES_INIT_PAIR_METHODDEF    \
    {"init_pair", (PyCFunction)(void(*)(void))_curses_init_pair, METH_FASTCALL, _curses_init_pair__doc__},

static PyObject *
_curses_init_pair_impl(PyObject *module, int pair_number, int fg, int bg);

static PyObject *
_curses_init_pair(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int pair_number;
    int fg;
    int bg;

    if (!_PyArg_CheckPositional("init_pair", nargs, 3, 3)) {
        goto exit;
    }
    if (!pair_converter(args[0], &pair_number)) {
        goto exit;
    }
    if (!color_allow_default_converter(args[1], &fg)) {
        goto exit;
    }
    if (!color_allow_default_converter(args[2], &bg)) {
        goto exit;
    }
    return_value = _curses_init_pair_impl(module, pair_number, fg, bg);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_initscr__doc__,
"initscr($module, /)\n"
"--\n"
"\n"
"Initialize the library.\n"
"\n"
"Return a WindowObject which represents the whole screen.");

#define _CURSES_INITSCR_METHODDEF    \
    {"initscr", (PyCFunction)_curses_initscr, METH_NOARGS, _curses_initscr__doc__},

static PyObject *
_curses_initscr_impl(PyObject *module);

static PyObject *
_curses_initscr(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_initscr_impl(module);
}

PyDoc_STRVAR(_curses_setupterm__doc__,
"setupterm($module, /, term=None, fd=-1)\n"
"--\n"
"\n"
"Initialize the terminal.\n"
"\n"
"  term\n"
"    Terminal name.\n"
"    If omitted, the value of the TERM environment variable will be used.\n"
"  fd\n"
"    File descriptor to which any initialization sequences will be sent.\n"
"    If not supplied, the file descriptor for sys.stdout will be used.");

#define _CURSES_SETUPTERM_METHODDEF    \
    {"setupterm", (PyCFunction)(void(*)(void))_curses_setupterm, METH_FASTCALL|METH_KEYWORDS, _curses_setupterm__doc__},

static PyObject *
_curses_setupterm_impl(PyObject *module, const char *term, int fd);

static PyObject *
_curses_setupterm(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"term", "fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "setupterm", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    const char *term = NULL;
    int fd = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        if (args[0] == Py_None) {
            term = NULL;
        }
        else if (PyUnicode_Check(args[0])) {
            Py_ssize_t term_length;
            term = PyUnicode_AsUTF8AndSize(args[0], &term_length);
            if (term == NULL) {
                goto exit;
            }
            if (strlen(term) != (size_t)term_length) {
                PyErr_SetString(PyExc_ValueError, "embedded null character");
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("setupterm", "argument 'term'", "str or None", args[0]);
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    fd = _PyLong_AsInt(args[1]);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = _curses_setupterm_impl(module, term, fd);

exit:
    return return_value;
}

#if (defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20081102)

PyDoc_STRVAR(_curses_get_escdelay__doc__,
"get_escdelay($module, /)\n"
"--\n"
"\n"
"Gets the curses ESCDELAY setting.\n"
"\n"
"Gets the number of milliseconds to wait after reading an escape character,\n"
"to distinguish between an individual escape character entered on the\n"
"keyboard from escape sequences sent by cursor and function keys.");

#define _CURSES_GET_ESCDELAY_METHODDEF    \
    {"get_escdelay", (PyCFunction)_curses_get_escdelay, METH_NOARGS, _curses_get_escdelay__doc__},

static PyObject *
_curses_get_escdelay_impl(PyObject *module);

static PyObject *
_curses_get_escdelay(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_get_escdelay_impl(module);
}

#endif /* (defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20081102) */

#if (defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20081102)

PyDoc_STRVAR(_curses_set_escdelay__doc__,
"set_escdelay($module, ms, /)\n"
"--\n"
"\n"
"Sets the curses ESCDELAY setting.\n"
"\n"
"  ms\n"
"    length of the delay in milliseconds.\n"
"\n"
"Sets the number of milliseconds to wait after reading an escape character,\n"
"to distinguish between an individual escape character entered on the\n"
"keyboard from escape sequences sent by cursor and function keys.");

#define _CURSES_SET_ESCDELAY_METHODDEF    \
    {"set_escdelay", (PyCFunction)_curses_set_escdelay, METH_O, _curses_set_escdelay__doc__},

static PyObject *
_curses_set_escdelay_impl(PyObject *module, int ms);

static PyObject *
_curses_set_escdelay(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int ms;

    ms = _PyLong_AsInt(arg);
    if (ms == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_set_escdelay_impl(module, ms);

exit:
    return return_value;
}

#endif /* (defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20081102) */

#if (defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20081102)

PyDoc_STRVAR(_curses_get_tabsize__doc__,
"get_tabsize($module, /)\n"
"--\n"
"\n"
"Gets the curses TABSIZE setting.\n"
"\n"
"Gets the number of columns used by the curses library when converting a tab\n"
"character to spaces as it adds the tab to a window.");

#define _CURSES_GET_TABSIZE_METHODDEF    \
    {"get_tabsize", (PyCFunction)_curses_get_tabsize, METH_NOARGS, _curses_get_tabsize__doc__},

static PyObject *
_curses_get_tabsize_impl(PyObject *module);

static PyObject *
_curses_get_tabsize(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_get_tabsize_impl(module);
}

#endif /* (defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20081102) */

#if (defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20081102)

PyDoc_STRVAR(_curses_set_tabsize__doc__,
"set_tabsize($module, size, /)\n"
"--\n"
"\n"
"Sets the curses TABSIZE setting.\n"
"\n"
"  size\n"
"    rendered cell width of a tab character.\n"
"\n"
"Sets the number of columns used by the curses library when converting a tab\n"
"character to spaces as it adds the tab to a window.");

#define _CURSES_SET_TABSIZE_METHODDEF    \
    {"set_tabsize", (PyCFunction)_curses_set_tabsize, METH_O, _curses_set_tabsize__doc__},

static PyObject *
_curses_set_tabsize_impl(PyObject *module, int size);

static PyObject *
_curses_set_tabsize(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int size;

    size = _PyLong_AsInt(arg);
    if (size == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_set_tabsize_impl(module, size);

exit:
    return return_value;
}

#endif /* (defined(NCURSES_EXT_FUNCS) && NCURSES_EXT_FUNCS >= 20081102) */

PyDoc_STRVAR(_curses_intrflush__doc__,
"intrflush($module, flag, /)\n"
"--\n"
"\n");

#define _CURSES_INTRFLUSH_METHODDEF    \
    {"intrflush", (PyCFunction)_curses_intrflush, METH_O, _curses_intrflush__doc__},

static PyObject *
_curses_intrflush_impl(PyObject *module, int flag);

static PyObject *
_curses_intrflush(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int flag;

    flag = _PyLong_AsInt(arg);
    if (flag == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_intrflush_impl(module, flag);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_isendwin__doc__,
"isendwin($module, /)\n"
"--\n"
"\n"
"Return True if endwin() has been called.");

#define _CURSES_ISENDWIN_METHODDEF    \
    {"isendwin", (PyCFunction)_curses_isendwin, METH_NOARGS, _curses_isendwin__doc__},

static PyObject *
_curses_isendwin_impl(PyObject *module);

static PyObject *
_curses_isendwin(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_isendwin_impl(module);
}

#if defined(HAVE_CURSES_IS_TERM_RESIZED)

PyDoc_STRVAR(_curses_is_term_resized__doc__,
"is_term_resized($module, nlines, ncols, /)\n"
"--\n"
"\n"
"Return True if resize_term() would modify the window structure, False otherwise.\n"
"\n"
"  nlines\n"
"    Height.\n"
"  ncols\n"
"    Width.");

#define _CURSES_IS_TERM_RESIZED_METHODDEF    \
    {"is_term_resized", (PyCFunction)(void(*)(void))_curses_is_term_resized, METH_FASTCALL, _curses_is_term_resized__doc__},

static PyObject *
_curses_is_term_resized_impl(PyObject *module, int nlines, int ncols);

static PyObject *
_curses_is_term_resized(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int nlines;
    int ncols;

    if (!_PyArg_CheckPositional("is_term_resized", nargs, 2, 2)) {
        goto exit;
    }
    nlines = _PyLong_AsInt(args[0]);
    if (nlines == -1 && PyErr_Occurred()) {
        goto exit;
    }
    ncols = _PyLong_AsInt(args[1]);
    if (ncols == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_is_term_resized_impl(module, nlines, ncols);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_IS_TERM_RESIZED) */

PyDoc_STRVAR(_curses_keyname__doc__,
"keyname($module, key, /)\n"
"--\n"
"\n"
"Return the name of specified key.\n"
"\n"
"  key\n"
"    Key number.");

#define _CURSES_KEYNAME_METHODDEF    \
    {"keyname", (PyCFunction)_curses_keyname, METH_O, _curses_keyname__doc__},

static PyObject *
_curses_keyname_impl(PyObject *module, int key);

static PyObject *
_curses_keyname(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int key;

    key = _PyLong_AsInt(arg);
    if (key == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_keyname_impl(module, key);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_killchar__doc__,
"killchar($module, /)\n"
"--\n"
"\n"
"Return the user\'s current line kill character.");

#define _CURSES_KILLCHAR_METHODDEF    \
    {"killchar", (PyCFunction)_curses_killchar, METH_NOARGS, _curses_killchar__doc__},

static PyObject *
_curses_killchar_impl(PyObject *module);

static PyObject *
_curses_killchar(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_killchar_impl(module);
}

PyDoc_STRVAR(_curses_longname__doc__,
"longname($module, /)\n"
"--\n"
"\n"
"Return the terminfo long name field describing the current terminal.\n"
"\n"
"The maximum length of a verbose description is 128 characters.  It is defined\n"
"only after the call to initscr().");

#define _CURSES_LONGNAME_METHODDEF    \
    {"longname", (PyCFunction)_curses_longname, METH_NOARGS, _curses_longname__doc__},

static PyObject *
_curses_longname_impl(PyObject *module);

static PyObject *
_curses_longname(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_longname_impl(module);
}

PyDoc_STRVAR(_curses_meta__doc__,
"meta($module, yes, /)\n"
"--\n"
"\n"
"Enable/disable meta keys.\n"
"\n"
"If yes is True, allow 8-bit characters to be input.  If yes is False,\n"
"allow only 7-bit characters.");

#define _CURSES_META_METHODDEF    \
    {"meta", (PyCFunction)_curses_meta, METH_O, _curses_meta__doc__},

static PyObject *
_curses_meta_impl(PyObject *module, int yes);

static PyObject *
_curses_meta(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int yes;

    yes = _PyLong_AsInt(arg);
    if (yes == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_meta_impl(module, yes);

exit:
    return return_value;
}

#if defined(NCURSES_MOUSE_VERSION)

PyDoc_STRVAR(_curses_mouseinterval__doc__,
"mouseinterval($module, interval, /)\n"
"--\n"
"\n"
"Set and retrieve the maximum time between press and release in a click.\n"
"\n"
"  interval\n"
"    Time in milliseconds.\n"
"\n"
"Set the maximum time that can elapse between press and release events in\n"
"order for them to be recognized as a click, and return the previous interval\n"
"value.");

#define _CURSES_MOUSEINTERVAL_METHODDEF    \
    {"mouseinterval", (PyCFunction)_curses_mouseinterval, METH_O, _curses_mouseinterval__doc__},

static PyObject *
_curses_mouseinterval_impl(PyObject *module, int interval);

static PyObject *
_curses_mouseinterval(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int interval;

    interval = _PyLong_AsInt(arg);
    if (interval == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_mouseinterval_impl(module, interval);

exit:
    return return_value;
}

#endif /* defined(NCURSES_MOUSE_VERSION) */

#if defined(NCURSES_MOUSE_VERSION)

PyDoc_STRVAR(_curses_mousemask__doc__,
"mousemask($module, newmask, /)\n"
"--\n"
"\n"
"Set the mouse events to be reported, and return a tuple (availmask, oldmask).\n"
"\n"
"Return a tuple (availmask, oldmask).  availmask indicates which of the\n"
"specified mouse events can be reported; on complete failure it returns 0.\n"
"oldmask is the previous value of the given window\'s mouse event mask.\n"
"If this function is never called, no mouse events are ever reported.");

#define _CURSES_MOUSEMASK_METHODDEF    \
    {"mousemask", (PyCFunction)_curses_mousemask, METH_O, _curses_mousemask__doc__},

static PyObject *
_curses_mousemask_impl(PyObject *module, unsigned long newmask);

static PyObject *
_curses_mousemask(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    unsigned long newmask;

    if (!PyLong_Check(arg)) {
        _PyArg_BadArgument("mousemask", "argument", "int", arg);
        goto exit;
    }
    newmask = PyLong_AsUnsignedLongMask(arg);
    return_value = _curses_mousemask_impl(module, newmask);

exit:
    return return_value;
}

#endif /* defined(NCURSES_MOUSE_VERSION) */

PyDoc_STRVAR(_curses_napms__doc__,
"napms($module, ms, /)\n"
"--\n"
"\n"
"Sleep for specified time.\n"
"\n"
"  ms\n"
"    Duration in milliseconds.");

#define _CURSES_NAPMS_METHODDEF    \
    {"napms", (PyCFunction)_curses_napms, METH_O, _curses_napms__doc__},

static PyObject *
_curses_napms_impl(PyObject *module, int ms);

static PyObject *
_curses_napms(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int ms;

    ms = _PyLong_AsInt(arg);
    if (ms == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_napms_impl(module, ms);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_newpad__doc__,
"newpad($module, nlines, ncols, /)\n"
"--\n"
"\n"
"Create and return a pointer to a new pad data structure.\n"
"\n"
"  nlines\n"
"    Height.\n"
"  ncols\n"
"    Width.");

#define _CURSES_NEWPAD_METHODDEF    \
    {"newpad", (PyCFunction)(void(*)(void))_curses_newpad, METH_FASTCALL, _curses_newpad__doc__},

static PyObject *
_curses_newpad_impl(PyObject *module, int nlines, int ncols);

static PyObject *
_curses_newpad(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int nlines;
    int ncols;

    if (!_PyArg_CheckPositional("newpad", nargs, 2, 2)) {
        goto exit;
    }
    nlines = _PyLong_AsInt(args[0]);
    if (nlines == -1 && PyErr_Occurred()) {
        goto exit;
    }
    ncols = _PyLong_AsInt(args[1]);
    if (ncols == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_newpad_impl(module, nlines, ncols);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_newwin__doc__,
"newwin(nlines, ncols, [begin_y=0, begin_x=0])\n"
"Return a new window.\n"
"\n"
"  nlines\n"
"    Height.\n"
"  ncols\n"
"    Width.\n"
"  begin_y\n"
"    Top side y-coordinate.\n"
"  begin_x\n"
"    Left side x-coordinate.\n"
"\n"
"By default, the window will extend from the specified position to the lower\n"
"right corner of the screen.");

#define _CURSES_NEWWIN_METHODDEF    \
    {"newwin", (PyCFunction)_curses_newwin, METH_VARARGS, _curses_newwin__doc__},

static PyObject *
_curses_newwin_impl(PyObject *module, int nlines, int ncols,
                    int group_right_1, int begin_y, int begin_x);

static PyObject *
_curses_newwin(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int nlines;
    int ncols;
    int group_right_1 = 0;
    int begin_y = 0;
    int begin_x = 0;

    switch (PyTuple_GET_SIZE(args)) {
        case 2:
            if (!PyArg_ParseTuple(args, "ii:newwin", &nlines, &ncols)) {
                goto exit;
            }
            break;
        case 4:
            if (!PyArg_ParseTuple(args, "iiii:newwin", &nlines, &ncols, &begin_y, &begin_x)) {
                goto exit;
            }
            group_right_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "_curses.newwin requires 2 to 4 arguments");
            goto exit;
    }
    return_value = _curses_newwin_impl(module, nlines, ncols, group_right_1, begin_y, begin_x);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_nl__doc__,
"nl($module, flag=True, /)\n"
"--\n"
"\n"
"Enter newline mode.\n"
"\n"
"  flag\n"
"    If false, the effect is the same as calling nonl().\n"
"\n"
"This mode translates the return key into newline on input, and translates\n"
"newline into return and line-feed on output.  Newline mode is initially on.");

#define _CURSES_NL_METHODDEF    \
    {"nl", (PyCFunction)(void(*)(void))_curses_nl, METH_FASTCALL, _curses_nl__doc__},

static PyObject *
_curses_nl_impl(PyObject *module, int flag);

static PyObject *
_curses_nl(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int flag = 1;

    if (!_PyArg_CheckPositional("nl", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    flag = _PyLong_AsInt(args[0]);
    if (flag == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _curses_nl_impl(module, flag);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_nocbreak__doc__,
"nocbreak($module, /)\n"
"--\n"
"\n"
"Leave cbreak mode.\n"
"\n"
"Return to normal \"cooked\" mode with line buffering.");

#define _CURSES_NOCBREAK_METHODDEF    \
    {"nocbreak", (PyCFunction)_curses_nocbreak, METH_NOARGS, _curses_nocbreak__doc__},

static PyObject *
_curses_nocbreak_impl(PyObject *module);

static PyObject *
_curses_nocbreak(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_nocbreak_impl(module);
}

PyDoc_STRVAR(_curses_noecho__doc__,
"noecho($module, /)\n"
"--\n"
"\n"
"Leave echo mode.\n"
"\n"
"Echoing of input characters is turned off.");

#define _CURSES_NOECHO_METHODDEF    \
    {"noecho", (PyCFunction)_curses_noecho, METH_NOARGS, _curses_noecho__doc__},

static PyObject *
_curses_noecho_impl(PyObject *module);

static PyObject *
_curses_noecho(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_noecho_impl(module);
}

PyDoc_STRVAR(_curses_nonl__doc__,
"nonl($module, /)\n"
"--\n"
"\n"
"Leave newline mode.\n"
"\n"
"Disable translation of return into newline on input, and disable low-level\n"
"translation of newline into newline/return on output.");

#define _CURSES_NONL_METHODDEF    \
    {"nonl", (PyCFunction)_curses_nonl, METH_NOARGS, _curses_nonl__doc__},

static PyObject *
_curses_nonl_impl(PyObject *module);

static PyObject *
_curses_nonl(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_nonl_impl(module);
}

PyDoc_STRVAR(_curses_noqiflush__doc__,
"noqiflush($module, /)\n"
"--\n"
"\n"
"Disable queue flushing.\n"
"\n"
"When queue flushing is disabled, normal flush of input and output queues\n"
"associated with the INTR, QUIT and SUSP characters will not be done.");

#define _CURSES_NOQIFLUSH_METHODDEF    \
    {"noqiflush", (PyCFunction)_curses_noqiflush, METH_NOARGS, _curses_noqiflush__doc__},

static PyObject *
_curses_noqiflush_impl(PyObject *module);

static PyObject *
_curses_noqiflush(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_noqiflush_impl(module);
}

PyDoc_STRVAR(_curses_noraw__doc__,
"noraw($module, /)\n"
"--\n"
"\n"
"Leave raw mode.\n"
"\n"
"Return to normal \"cooked\" mode with line buffering.");

#define _CURSES_NORAW_METHODDEF    \
    {"noraw", (PyCFunction)_curses_noraw, METH_NOARGS, _curses_noraw__doc__},

static PyObject *
_curses_noraw_impl(PyObject *module);

static PyObject *
_curses_noraw(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_noraw_impl(module);
}

PyDoc_STRVAR(_curses_pair_content__doc__,
"pair_content($module, pair_number, /)\n"
"--\n"
"\n"
"Return a tuple (fg, bg) containing the colors for the requested color pair.\n"
"\n"
"  pair_number\n"
"    The number of the color pair (0 - (COLOR_PAIRS-1)).");

#define _CURSES_PAIR_CONTENT_METHODDEF    \
    {"pair_content", (PyCFunction)_curses_pair_content, METH_O, _curses_pair_content__doc__},

static PyObject *
_curses_pair_content_impl(PyObject *module, int pair_number);

static PyObject *
_curses_pair_content(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int pair_number;

    if (!pair_converter(arg, &pair_number)) {
        goto exit;
    }
    return_value = _curses_pair_content_impl(module, pair_number);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_pair_number__doc__,
"pair_number($module, attr, /)\n"
"--\n"
"\n"
"Return the number of the color-pair set by the specified attribute value.\n"
"\n"
"color_pair() is the counterpart to this function.");

#define _CURSES_PAIR_NUMBER_METHODDEF    \
    {"pair_number", (PyCFunction)_curses_pair_number, METH_O, _curses_pair_number__doc__},

static PyObject *
_curses_pair_number_impl(PyObject *module, int attr);

static PyObject *
_curses_pair_number(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int attr;

    attr = _PyLong_AsInt(arg);
    if (attr == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_pair_number_impl(module, attr);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_putp__doc__,
"putp($module, string, /)\n"
"--\n"
"\n"
"Emit the value of a specified terminfo capability for the current terminal.\n"
"\n"
"Note that the output of putp() always goes to standard output.");

#define _CURSES_PUTP_METHODDEF    \
    {"putp", (PyCFunction)_curses_putp, METH_O, _curses_putp__doc__},

static PyObject *
_curses_putp_impl(PyObject *module, const char *string);

static PyObject *
_curses_putp(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *string;

    if (!PyArg_Parse(arg, "y:putp", &string)) {
        goto exit;
    }
    return_value = _curses_putp_impl(module, string);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_qiflush__doc__,
"qiflush($module, flag=True, /)\n"
"--\n"
"\n"
"Enable queue flushing.\n"
"\n"
"  flag\n"
"    If false, the effect is the same as calling noqiflush().\n"
"\n"
"If queue flushing is enabled, all output in the display driver queue\n"
"will be flushed when the INTR, QUIT and SUSP characters are read.");

#define _CURSES_QIFLUSH_METHODDEF    \
    {"qiflush", (PyCFunction)(void(*)(void))_curses_qiflush, METH_FASTCALL, _curses_qiflush__doc__},

static PyObject *
_curses_qiflush_impl(PyObject *module, int flag);

static PyObject *
_curses_qiflush(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int flag = 1;

    if (!_PyArg_CheckPositional("qiflush", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    flag = _PyLong_AsInt(args[0]);
    if (flag == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _curses_qiflush_impl(module, flag);

exit:
    return return_value;
}

#if (defined(HAVE_CURSES_RESIZETERM) || defined(HAVE_CURSES_RESIZE_TERM))

PyDoc_STRVAR(_curses_update_lines_cols__doc__,
"update_lines_cols($module, /)\n"
"--\n"
"\n");

#define _CURSES_UPDATE_LINES_COLS_METHODDEF    \
    {"update_lines_cols", (PyCFunction)_curses_update_lines_cols, METH_NOARGS, _curses_update_lines_cols__doc__},

static PyObject *
_curses_update_lines_cols_impl(PyObject *module);

static PyObject *
_curses_update_lines_cols(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_update_lines_cols_impl(module);
}

#endif /* (defined(HAVE_CURSES_RESIZETERM) || defined(HAVE_CURSES_RESIZE_TERM)) */

PyDoc_STRVAR(_curses_raw__doc__,
"raw($module, flag=True, /)\n"
"--\n"
"\n"
"Enter raw mode.\n"
"\n"
"  flag\n"
"    If false, the effect is the same as calling noraw().\n"
"\n"
"In raw mode, normal line buffering and processing of interrupt, quit,\n"
"suspend, and flow control keys are turned off; characters are presented to\n"
"curses input functions one by one.");

#define _CURSES_RAW_METHODDEF    \
    {"raw", (PyCFunction)(void(*)(void))_curses_raw, METH_FASTCALL, _curses_raw__doc__},

static PyObject *
_curses_raw_impl(PyObject *module, int flag);

static PyObject *
_curses_raw(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int flag = 1;

    if (!_PyArg_CheckPositional("raw", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    flag = _PyLong_AsInt(args[0]);
    if (flag == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _curses_raw_impl(module, flag);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_reset_prog_mode__doc__,
"reset_prog_mode($module, /)\n"
"--\n"
"\n"
"Restore the terminal to \"program\" mode, as previously saved by def_prog_mode().");

#define _CURSES_RESET_PROG_MODE_METHODDEF    \
    {"reset_prog_mode", (PyCFunction)_curses_reset_prog_mode, METH_NOARGS, _curses_reset_prog_mode__doc__},

static PyObject *
_curses_reset_prog_mode_impl(PyObject *module);

static PyObject *
_curses_reset_prog_mode(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_reset_prog_mode_impl(module);
}

PyDoc_STRVAR(_curses_reset_shell_mode__doc__,
"reset_shell_mode($module, /)\n"
"--\n"
"\n"
"Restore the terminal to \"shell\" mode, as previously saved by def_shell_mode().");

#define _CURSES_RESET_SHELL_MODE_METHODDEF    \
    {"reset_shell_mode", (PyCFunction)_curses_reset_shell_mode, METH_NOARGS, _curses_reset_shell_mode__doc__},

static PyObject *
_curses_reset_shell_mode_impl(PyObject *module);

static PyObject *
_curses_reset_shell_mode(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_reset_shell_mode_impl(module);
}

PyDoc_STRVAR(_curses_resetty__doc__,
"resetty($module, /)\n"
"--\n"
"\n"
"Restore terminal mode.");

#define _CURSES_RESETTY_METHODDEF    \
    {"resetty", (PyCFunction)_curses_resetty, METH_NOARGS, _curses_resetty__doc__},

static PyObject *
_curses_resetty_impl(PyObject *module);

static PyObject *
_curses_resetty(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_resetty_impl(module);
}

#if defined(HAVE_CURSES_RESIZETERM)

PyDoc_STRVAR(_curses_resizeterm__doc__,
"resizeterm($module, nlines, ncols, /)\n"
"--\n"
"\n"
"Resize the standard and current windows to the specified dimensions.\n"
"\n"
"  nlines\n"
"    Height.\n"
"  ncols\n"
"    Width.\n"
"\n"
"Adjusts other bookkeeping data used by the curses library that record the\n"
"window dimensions (in particular the SIGWINCH handler).");

#define _CURSES_RESIZETERM_METHODDEF    \
    {"resizeterm", (PyCFunction)(void(*)(void))_curses_resizeterm, METH_FASTCALL, _curses_resizeterm__doc__},

static PyObject *
_curses_resizeterm_impl(PyObject *module, int nlines, int ncols);

static PyObject *
_curses_resizeterm(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int nlines;
    int ncols;

    if (!_PyArg_CheckPositional("resizeterm", nargs, 2, 2)) {
        goto exit;
    }
    nlines = _PyLong_AsInt(args[0]);
    if (nlines == -1 && PyErr_Occurred()) {
        goto exit;
    }
    ncols = _PyLong_AsInt(args[1]);
    if (ncols == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_resizeterm_impl(module, nlines, ncols);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_RESIZETERM) */

#if defined(HAVE_CURSES_RESIZE_TERM)

PyDoc_STRVAR(_curses_resize_term__doc__,
"resize_term($module, nlines, ncols, /)\n"
"--\n"
"\n"
"Backend function used by resizeterm(), performing most of the work.\n"
"\n"
"  nlines\n"
"    Height.\n"
"  ncols\n"
"    Width.\n"
"\n"
"When resizing the windows, resize_term() blank-fills the areas that are\n"
"extended.  The calling application should fill in these areas with appropriate\n"
"data.  The resize_term() function attempts to resize all windows.  However,\n"
"due to the calling convention of pads, it is not possible to resize these\n"
"without additional interaction with the application.");

#define _CURSES_RESIZE_TERM_METHODDEF    \
    {"resize_term", (PyCFunction)(void(*)(void))_curses_resize_term, METH_FASTCALL, _curses_resize_term__doc__},

static PyObject *
_curses_resize_term_impl(PyObject *module, int nlines, int ncols);

static PyObject *
_curses_resize_term(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int nlines;
    int ncols;

    if (!_PyArg_CheckPositional("resize_term", nargs, 2, 2)) {
        goto exit;
    }
    nlines = _PyLong_AsInt(args[0]);
    if (nlines == -1 && PyErr_Occurred()) {
        goto exit;
    }
    ncols = _PyLong_AsInt(args[1]);
    if (ncols == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_resize_term_impl(module, nlines, ncols);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_RESIZE_TERM) */

PyDoc_STRVAR(_curses_savetty__doc__,
"savetty($module, /)\n"
"--\n"
"\n"
"Save terminal mode.");

#define _CURSES_SAVETTY_METHODDEF    \
    {"savetty", (PyCFunction)_curses_savetty, METH_NOARGS, _curses_savetty__doc__},

static PyObject *
_curses_savetty_impl(PyObject *module);

static PyObject *
_curses_savetty(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_savetty_impl(module);
}

#if defined(getsyx)

PyDoc_STRVAR(_curses_setsyx__doc__,
"setsyx($module, y, x, /)\n"
"--\n"
"\n"
"Set the virtual screen cursor.\n"
"\n"
"  y\n"
"    Y-coordinate.\n"
"  x\n"
"    X-coordinate.\n"
"\n"
"If y and x are both -1, then leaveok is set.");

#define _CURSES_SETSYX_METHODDEF    \
    {"setsyx", (PyCFunction)(void(*)(void))_curses_setsyx, METH_FASTCALL, _curses_setsyx__doc__},

static PyObject *
_curses_setsyx_impl(PyObject *module, int y, int x);

static PyObject *
_curses_setsyx(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int y;
    int x;

    if (!_PyArg_CheckPositional("setsyx", nargs, 2, 2)) {
        goto exit;
    }
    y = _PyLong_AsInt(args[0]);
    if (y == -1 && PyErr_Occurred()) {
        goto exit;
    }
    x = _PyLong_AsInt(args[1]);
    if (x == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_setsyx_impl(module, y, x);

exit:
    return return_value;
}

#endif /* defined(getsyx) */

PyDoc_STRVAR(_curses_start_color__doc__,
"start_color($module, /)\n"
"--\n"
"\n"
"Initializes eight basic colors and global variables COLORS and COLOR_PAIRS.\n"
"\n"
"Must be called if the programmer wants to use colors, and before any other\n"
"color manipulation routine is called.  It is good practice to call this\n"
"routine right after initscr().\n"
"\n"
"It also restores the colors on the terminal to the values they had when the\n"
"terminal was just turned on.");

#define _CURSES_START_COLOR_METHODDEF    \
    {"start_color", (PyCFunction)_curses_start_color, METH_NOARGS, _curses_start_color__doc__},

static PyObject *
_curses_start_color_impl(PyObject *module);

static PyObject *
_curses_start_color(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_start_color_impl(module);
}

PyDoc_STRVAR(_curses_termattrs__doc__,
"termattrs($module, /)\n"
"--\n"
"\n"
"Return a logical OR of all video attributes supported by the terminal.");

#define _CURSES_TERMATTRS_METHODDEF    \
    {"termattrs", (PyCFunction)_curses_termattrs, METH_NOARGS, _curses_termattrs__doc__},

static PyObject *
_curses_termattrs_impl(PyObject *module);

static PyObject *
_curses_termattrs(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_termattrs_impl(module);
}

PyDoc_STRVAR(_curses_termname__doc__,
"termname($module, /)\n"
"--\n"
"\n"
"Return the value of the environment variable TERM, truncated to 14 characters.");

#define _CURSES_TERMNAME_METHODDEF    \
    {"termname", (PyCFunction)_curses_termname, METH_NOARGS, _curses_termname__doc__},

static PyObject *
_curses_termname_impl(PyObject *module);

static PyObject *
_curses_termname(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_termname_impl(module);
}

PyDoc_STRVAR(_curses_tigetflag__doc__,
"tigetflag($module, capname, /)\n"
"--\n"
"\n"
"Return the value of the Boolean capability.\n"
"\n"
"  capname\n"
"    The terminfo capability name.\n"
"\n"
"The value -1 is returned if capname is not a Boolean capability, or 0 if\n"
"it is canceled or absent from the terminal description.");

#define _CURSES_TIGETFLAG_METHODDEF    \
    {"tigetflag", (PyCFunction)_curses_tigetflag, METH_O, _curses_tigetflag__doc__},

static PyObject *
_curses_tigetflag_impl(PyObject *module, const char *capname);

static PyObject *
_curses_tigetflag(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *capname;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("tigetflag", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t capname_length;
    capname = PyUnicode_AsUTF8AndSize(arg, &capname_length);
    if (capname == NULL) {
        goto exit;
    }
    if (strlen(capname) != (size_t)capname_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _curses_tigetflag_impl(module, capname);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_tigetnum__doc__,
"tigetnum($module, capname, /)\n"
"--\n"
"\n"
"Return the value of the numeric capability.\n"
"\n"
"  capname\n"
"    The terminfo capability name.\n"
"\n"
"The value -2 is returned if capname is not a numeric capability, or -1 if\n"
"it is canceled or absent from the terminal description.");

#define _CURSES_TIGETNUM_METHODDEF    \
    {"tigetnum", (PyCFunction)_curses_tigetnum, METH_O, _curses_tigetnum__doc__},

static PyObject *
_curses_tigetnum_impl(PyObject *module, const char *capname);

static PyObject *
_curses_tigetnum(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *capname;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("tigetnum", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t capname_length;
    capname = PyUnicode_AsUTF8AndSize(arg, &capname_length);
    if (capname == NULL) {
        goto exit;
    }
    if (strlen(capname) != (size_t)capname_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _curses_tigetnum_impl(module, capname);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_tigetstr__doc__,
"tigetstr($module, capname, /)\n"
"--\n"
"\n"
"Return the value of the string capability.\n"
"\n"
"  capname\n"
"    The terminfo capability name.\n"
"\n"
"None is returned if capname is not a string capability, or is canceled or\n"
"absent from the terminal description.");

#define _CURSES_TIGETSTR_METHODDEF    \
    {"tigetstr", (PyCFunction)_curses_tigetstr, METH_O, _curses_tigetstr__doc__},

static PyObject *
_curses_tigetstr_impl(PyObject *module, const char *capname);

static PyObject *
_curses_tigetstr(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *capname;

    if (!PyUnicode_Check(arg)) {
        _PyArg_BadArgument("tigetstr", "argument", "str", arg);
        goto exit;
    }
    Py_ssize_t capname_length;
    capname = PyUnicode_AsUTF8AndSize(arg, &capname_length);
    if (capname == NULL) {
        goto exit;
    }
    if (strlen(capname) != (size_t)capname_length) {
        PyErr_SetString(PyExc_ValueError, "embedded null character");
        goto exit;
    }
    return_value = _curses_tigetstr_impl(module, capname);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_tparm__doc__,
"tparm($module, str, i1=0, i2=0, i3=0, i4=0, i5=0, i6=0, i7=0, i8=0,\n"
"      i9=0, /)\n"
"--\n"
"\n"
"Instantiate the specified byte string with the supplied parameters.\n"
"\n"
"  str\n"
"    Parameterized byte string obtained from the terminfo database.");

#define _CURSES_TPARM_METHODDEF    \
    {"tparm", (PyCFunction)(void(*)(void))_curses_tparm, METH_FASTCALL, _curses_tparm__doc__},

static PyObject *
_curses_tparm_impl(PyObject *module, const char *str, int i1, int i2, int i3,
                   int i4, int i5, int i6, int i7, int i8, int i9);

static PyObject *
_curses_tparm(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    const char *str;
    int i1 = 0;
    int i2 = 0;
    int i3 = 0;
    int i4 = 0;
    int i5 = 0;
    int i6 = 0;
    int i7 = 0;
    int i8 = 0;
    int i9 = 0;

    if (!_PyArg_ParseStack(args, nargs, "y|iiiiiiiii:tparm",
        &str, &i1, &i2, &i3, &i4, &i5, &i6, &i7, &i8, &i9)) {
        goto exit;
    }
    return_value = _curses_tparm_impl(module, str, i1, i2, i3, i4, i5, i6, i7, i8, i9);

exit:
    return return_value;
}

#if defined(HAVE_CURSES_TYPEAHEAD)

PyDoc_STRVAR(_curses_typeahead__doc__,
"typeahead($module, fd, /)\n"
"--\n"
"\n"
"Specify that the file descriptor fd be used for typeahead checking.\n"
"\n"
"  fd\n"
"    File descriptor.\n"
"\n"
"If fd is -1, then no typeahead checking is done.");

#define _CURSES_TYPEAHEAD_METHODDEF    \
    {"typeahead", (PyCFunction)_curses_typeahead, METH_O, _curses_typeahead__doc__},

static PyObject *
_curses_typeahead_impl(PyObject *module, int fd);

static PyObject *
_curses_typeahead(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;

    fd = _PyLong_AsInt(arg);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_typeahead_impl(module, fd);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_TYPEAHEAD) */

PyDoc_STRVAR(_curses_unctrl__doc__,
"unctrl($module, ch, /)\n"
"--\n"
"\n"
"Return a string which is a printable representation of the character ch.\n"
"\n"
"Control characters are displayed as a caret followed by the character,\n"
"for example as ^C.  Printing characters are left as they are.");

#define _CURSES_UNCTRL_METHODDEF    \
    {"unctrl", (PyCFunction)_curses_unctrl, METH_O, _curses_unctrl__doc__},

PyDoc_STRVAR(_curses_ungetch__doc__,
"ungetch($module, ch, /)\n"
"--\n"
"\n"
"Push ch so the next getch() will return it.");

#define _CURSES_UNGETCH_METHODDEF    \
    {"ungetch", (PyCFunction)_curses_ungetch, METH_O, _curses_ungetch__doc__},

#if defined(HAVE_NCURSESW)

PyDoc_STRVAR(_curses_unget_wch__doc__,
"unget_wch($module, ch, /)\n"
"--\n"
"\n"
"Push ch so the next get_wch() will return it.");

#define _CURSES_UNGET_WCH_METHODDEF    \
    {"unget_wch", (PyCFunction)_curses_unget_wch, METH_O, _curses_unget_wch__doc__},

#endif /* defined(HAVE_NCURSESW) */

#if defined(HAVE_CURSES_USE_ENV)

PyDoc_STRVAR(_curses_use_env__doc__,
"use_env($module, flag, /)\n"
"--\n"
"\n"
"Use environment variables LINES and COLUMNS.\n"
"\n"
"If used, this function should be called before initscr() or newterm() are\n"
"called.\n"
"\n"
"When flag is False, the values of lines and columns specified in the terminfo\n"
"database will be used, even if environment variables LINES and COLUMNS (used\n"
"by default) are set, or if curses is running in a window (in which case\n"
"default behavior would be to use the window size if LINES and COLUMNS are\n"
"not set).");

#define _CURSES_USE_ENV_METHODDEF    \
    {"use_env", (PyCFunction)_curses_use_env, METH_O, _curses_use_env__doc__},

static PyObject *
_curses_use_env_impl(PyObject *module, int flag);

static PyObject *
_curses_use_env(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int flag;

    flag = _PyLong_AsInt(arg);
    if (flag == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _curses_use_env_impl(module, flag);

exit:
    return return_value;
}

#endif /* defined(HAVE_CURSES_USE_ENV) */

#if !defined(STRICT_SYSV_CURSES)

PyDoc_STRVAR(_curses_use_default_colors__doc__,
"use_default_colors($module, /)\n"
"--\n"
"\n"
"Allow use of default values for colors on terminals supporting this feature.\n"
"\n"
"Use this to support transparency in your application.  The default color\n"
"is assigned to the color number -1.");

#define _CURSES_USE_DEFAULT_COLORS_METHODDEF    \
    {"use_default_colors", (PyCFunction)_curses_use_default_colors, METH_NOARGS, _curses_use_default_colors__doc__},

static PyObject *
_curses_use_default_colors_impl(PyObject *module);

static PyObject *
_curses_use_default_colors(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_use_default_colors_impl(module);
}

#endif /* !defined(STRICT_SYSV_CURSES) */

PyDoc_STRVAR(_curses_has_extended_color_support__doc__,
"has_extended_color_support($module, /)\n"
"--\n"
"\n"
"Return True if the module supports extended colors; otherwise, return False.\n"
"\n"
"Extended color support allows more than 256 color-pairs for terminals\n"
"that support more than 16 colors (e.g. xterm-256color).");

#define _CURSES_HAS_EXTENDED_COLOR_SUPPORT_METHODDEF    \
    {"has_extended_color_support", (PyCFunction)_curses_has_extended_color_support, METH_NOARGS, _curses_has_extended_color_support__doc__},

static PyObject *
_curses_has_extended_color_support_impl(PyObject *module);

static PyObject *
_curses_has_extended_color_support(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_has_extended_color_support_impl(module);
}

#ifndef _CURSES_WINDOW_ENCLOSE_METHODDEF
    #define _CURSES_WINDOW_ENCLOSE_METHODDEF
#endif /* !defined(_CURSES_WINDOW_ENCLOSE_METHODDEF) */

#ifndef _CURSES_WINDOW_GET_WCH_METHODDEF
    #define _CURSES_WINDOW_GET_WCH_METHODDEF
#endif /* !defined(_CURSES_WINDOW_GET_WCH_METHODDEF) */

#ifndef _CURSES_WINDOW_NOUTREFRESH_METHODDEF
    #define _CURSES_WINDOW_NOUTREFRESH_METHODDEF
#endif /* !defined(_CURSES_WINDOW_NOUTREFRESH_METHODDEF) */

#ifndef _CURSES_FILTER_METHODDEF
    #define _CURSES_FILTER_METHODDEF
#endif /* !defined(_CURSES_FILTER_METHODDEF) */

#ifndef _CURSES_GETSYX_METHODDEF
    #define _CURSES_GETSYX_METHODDEF
#endif /* !defined(_CURSES_GETSYX_METHODDEF) */

#ifndef _CURSES_GETMOUSE_METHODDEF
    #define _CURSES_GETMOUSE_METHODDEF
#endif /* !defined(_CURSES_GETMOUSE_METHODDEF) */

#ifndef _CURSES_UNGETMOUSE_METHODDEF
    #define _CURSES_UNGETMOUSE_METHODDEF
#endif /* !defined(_CURSES_UNGETMOUSE_METHODDEF) */

#ifndef _CURSES_HAS_KEY_METHODDEF
    #define _CURSES_HAS_KEY_METHODDEF
#endif /* !defined(_CURSES_HAS_KEY_METHODDEF) */

#ifndef _CURSES_GET_ESCDELAY_METHODDEF
    #define _CURSES_GET_ESCDELAY_METHODDEF
#endif /* !defined(_CURSES_GET_ESCDELAY_METHODDEF) */

#ifndef _CURSES_SET_ESCDELAY_METHODDEF
    #define _CURSES_SET_ESCDELAY_METHODDEF
#endif /* !defined(_CURSES_SET_ESCDELAY_METHODDEF) */

#ifndef _CURSES_GET_TABSIZE_METHODDEF
    #define _CURSES_GET_TABSIZE_METHODDEF
#endif /* !defined(_CURSES_GET_TABSIZE_METHODDEF) */

#ifndef _CURSES_SET_TABSIZE_METHODDEF
    #define _CURSES_SET_TABSIZE_METHODDEF
#endif /* !defined(_CURSES_SET_TABSIZE_METHODDEF) */

#ifndef _CURSES_IS_TERM_RESIZED_METHODDEF
    #define _CURSES_IS_TERM_RESIZED_METHODDEF
#endif /* !defined(_CURSES_IS_TERM_RESIZED_METHODDEF) */

#ifndef _CURSES_MOUSEINTERVAL_METHODDEF
    #define _CURSES_MOUSEINTERVAL_METHODDEF
#endif /* !defined(_CURSES_MOUSEINTERVAL_METHODDEF) */

#ifndef _CURSES_MOUSEMASK_METHODDEF
    #define _CURSES_MOUSEMASK_METHODDEF
#endif /* !defined(_CURSES_MOUSEMASK_METHODDEF) */

#ifndef _CURSES_UPDATE_LINES_COLS_METHODDEF
    #define _CURSES_UPDATE_LINES_COLS_METHODDEF
#endif /* !defined(_CURSES_UPDATE_LINES_COLS_METHODDEF) */

#ifndef _CURSES_RESIZETERM_METHODDEF
    #define _CURSES_RESIZETERM_METHODDEF
#endif /* !defined(_CURSES_RESIZETERM_METHODDEF) */

#ifndef _CURSES_RESIZE_TERM_METHODDEF
    #define _CURSES_RESIZE_TERM_METHODDEF
#endif /* !defined(_CURSES_RESIZE_TERM_METHODDEF) */

#ifndef _CURSES_SETSYX_METHODDEF
    #define _CURSES_SETSYX_METHODDEF
#endif /* !defined(_CURSES_SETSYX_METHODDEF) */

#ifndef _CURSES_TYPEAHEAD_METHODDEF
    #define _CURSES_TYPEAHEAD_METHODDEF
#endif /* !defined(_CURSES_TYPEAHEAD_METHODDEF) */

#ifndef _CURSES_UNGET_WCH_METHODDEF
    #define _CURSES_UNGET_WCH_METHODDEF
#endif /* !defined(_CURSES_UNGET_WCH_METHODDEF) */

#ifndef _CURSES_USE_ENV_METHODDEF
    #define _CURSES_USE_ENV_METHODDEF
#endif /* !defined(_CURSES_USE_ENV_METHODDEF) */

#ifndef _CURSES_USE_DEFAULT_COLORS_METHODDEF
    #define _CURSES_USE_DEFAULT_COLORS_METHODDEF
#endif /* !defined(_CURSES_USE_DEFAULT_COLORS_METHODDEF) */
/*[clinic end generated code: output=9efc9943a3ac3741 input=a9049054013a1b77]*/
