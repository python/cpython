/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(curses_window_addch__doc__,
"addch([y, x,] ch, [attr])\n"
"Paint character ch at (y, x) with attributes attr.\n"
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

#define CURSES_WINDOW_ADDCH_METHODDEF    \
    {"addch", (PyCFunction)curses_window_addch, METH_VARARGS, curses_window_addch__doc__},

static PyObject *
curses_window_addch_impl(PyCursesWindowObject *self, int group_left_1, int y,
                         int x, PyObject *ch, int group_right_1, long attr);

static PyObject *
curses_window_addch(PyCursesWindowObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int group_left_1 = 0;
    int y = 0;
    int x = 0;
    PyObject *ch;
    int group_right_1 = 0;
    long attr = 0;

    switch (PyTuple_GET_SIZE(args)) {
        case 1:
            if (!PyArg_ParseTuple(args, "O:addch", &ch))
                goto exit;
            break;
        case 2:
            if (!PyArg_ParseTuple(args, "Ol:addch", &ch, &attr))
                goto exit;
            group_right_1 = 1;
            break;
        case 3:
            if (!PyArg_ParseTuple(args, "iiO:addch", &y, &x, &ch))
                goto exit;
            group_left_1 = 1;
            break;
        case 4:
            if (!PyArg_ParseTuple(args, "iiOl:addch", &y, &x, &ch, &attr))
                goto exit;
            group_right_1 = 1;
            group_left_1 = 1;
            break;
        default:
            PyErr_SetString(PyExc_TypeError, "curses.window.addch requires 1 to 4 arguments");
            goto exit;
    }
    return_value = curses_window_addch_impl(self, group_left_1, y, x, ch, group_right_1, attr);

exit:
    return return_value;
}
/*[clinic end generated code: output=982b1e709577f3ec input=a9049054013a1b77]*/
