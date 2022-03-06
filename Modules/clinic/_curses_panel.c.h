/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_curses_panel_panel_bottom__doc__,
"bottom($self, /)\n"
"--\n"
"\n"
"Push the panel to the bottom of the stack.");

#define _CURSES_PANEL_PANEL_BOTTOM_METHODDEF    \
    {"bottom", (PyCFunction)_curses_panel_panel_bottom, METH_NOARGS, _curses_panel_panel_bottom__doc__},

static PyObject *
_curses_panel_panel_bottom_impl(PyCursesPanelObject *self);

static PyObject *
_curses_panel_panel_bottom(PyCursesPanelObject *self, PyObject *Py_UNUSED(ignored))
{
    return _curses_panel_panel_bottom_impl(self);
}

PyDoc_STRVAR(_curses_panel_panel_hide__doc__,
"hide($self, /)\n"
"--\n"
"\n"
"Hide the panel.\n"
"\n"
"This does not delete the object, it just makes the window on screen invisible.");

#define _CURSES_PANEL_PANEL_HIDE_METHODDEF    \
    {"hide", (PyCFunction)_curses_panel_panel_hide, METH_NOARGS, _curses_panel_panel_hide__doc__},

static PyObject *
_curses_panel_panel_hide_impl(PyCursesPanelObject *self);

static PyObject *
_curses_panel_panel_hide(PyCursesPanelObject *self, PyObject *Py_UNUSED(ignored))
{
    return _curses_panel_panel_hide_impl(self);
}

PyDoc_STRVAR(_curses_panel_panel_show__doc__,
"show($self, /)\n"
"--\n"
"\n"
"Display the panel (which might have been hidden).");

#define _CURSES_PANEL_PANEL_SHOW_METHODDEF    \
    {"show", (PyCFunction)_curses_panel_panel_show, METH_NOARGS, _curses_panel_panel_show__doc__},

static PyObject *
_curses_panel_panel_show_impl(PyCursesPanelObject *self);

static PyObject *
_curses_panel_panel_show(PyCursesPanelObject *self, PyObject *Py_UNUSED(ignored))
{
    return _curses_panel_panel_show_impl(self);
}

PyDoc_STRVAR(_curses_panel_panel_top__doc__,
"top($self, /)\n"
"--\n"
"\n"
"Push panel to the top of the stack.");

#define _CURSES_PANEL_PANEL_TOP_METHODDEF    \
    {"top", (PyCFunction)_curses_panel_panel_top, METH_NOARGS, _curses_panel_panel_top__doc__},

static PyObject *
_curses_panel_panel_top_impl(PyCursesPanelObject *self);

static PyObject *
_curses_panel_panel_top(PyCursesPanelObject *self, PyObject *Py_UNUSED(ignored))
{
    return _curses_panel_panel_top_impl(self);
}

PyDoc_STRVAR(_curses_panel_panel_above__doc__,
"above($self, /)\n"
"--\n"
"\n"
"Return the panel above the current panel.");

#define _CURSES_PANEL_PANEL_ABOVE_METHODDEF    \
    {"above", (PyCFunction)_curses_panel_panel_above, METH_NOARGS, _curses_panel_panel_above__doc__},

static PyObject *
_curses_panel_panel_above_impl(PyCursesPanelObject *self);

static PyObject *
_curses_panel_panel_above(PyCursesPanelObject *self, PyObject *Py_UNUSED(ignored))
{
    return _curses_panel_panel_above_impl(self);
}

PyDoc_STRVAR(_curses_panel_panel_below__doc__,
"below($self, /)\n"
"--\n"
"\n"
"Return the panel below the current panel.");

#define _CURSES_PANEL_PANEL_BELOW_METHODDEF    \
    {"below", (PyCFunction)_curses_panel_panel_below, METH_NOARGS, _curses_panel_panel_below__doc__},

static PyObject *
_curses_panel_panel_below_impl(PyCursesPanelObject *self);

static PyObject *
_curses_panel_panel_below(PyCursesPanelObject *self, PyObject *Py_UNUSED(ignored))
{
    return _curses_panel_panel_below_impl(self);
}

PyDoc_STRVAR(_curses_panel_panel_hidden__doc__,
"hidden($self, /)\n"
"--\n"
"\n"
"Return True if the panel is hidden (not visible), False otherwise.");

#define _CURSES_PANEL_PANEL_HIDDEN_METHODDEF    \
    {"hidden", (PyCFunction)_curses_panel_panel_hidden, METH_NOARGS, _curses_panel_panel_hidden__doc__},

static PyObject *
_curses_panel_panel_hidden_impl(PyCursesPanelObject *self);

static PyObject *
_curses_panel_panel_hidden(PyCursesPanelObject *self, PyObject *Py_UNUSED(ignored))
{
    return _curses_panel_panel_hidden_impl(self);
}

PyDoc_STRVAR(_curses_panel_panel_move__doc__,
"move($self, y, x, /)\n"
"--\n"
"\n"
"Move the panel to the screen coordinates (y, x).");

#define _CURSES_PANEL_PANEL_MOVE_METHODDEF    \
    {"move", (PyCFunction)_curses_panel_panel_move, METH_FASTCALL, _curses_panel_panel_move__doc__},

static PyObject *
_curses_panel_panel_move_impl(PyCursesPanelObject *self, int y, int x);

static PyObject *
_curses_panel_panel_move(PyCursesPanelObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int y;
    int x;

    if (!_PyArg_ParseStack(args, nargs, "ii:move",
        &y, &x)) {
        goto exit;
    }
    return_value = _curses_panel_panel_move_impl(self, y, x);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_panel_panel_window__doc__,
"window($self, /)\n"
"--\n"
"\n"
"Return the window object associated with the panel.");

#define _CURSES_PANEL_PANEL_WINDOW_METHODDEF    \
    {"window", (PyCFunction)_curses_panel_panel_window, METH_NOARGS, _curses_panel_panel_window__doc__},

static PyObject *
_curses_panel_panel_window_impl(PyCursesPanelObject *self);

static PyObject *
_curses_panel_panel_window(PyCursesPanelObject *self, PyObject *Py_UNUSED(ignored))
{
    return _curses_panel_panel_window_impl(self);
}

PyDoc_STRVAR(_curses_panel_panel_replace__doc__,
"replace($self, win, /)\n"
"--\n"
"\n"
"Change the window associated with the panel to the window win.");

#define _CURSES_PANEL_PANEL_REPLACE_METHODDEF    \
    {"replace", (PyCFunction)_curses_panel_panel_replace, METH_O, _curses_panel_panel_replace__doc__},

static PyObject *
_curses_panel_panel_replace_impl(PyCursesPanelObject *self,
                                 PyCursesWindowObject *win);

static PyObject *
_curses_panel_panel_replace(PyCursesPanelObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyCursesWindowObject *win;

    if (!PyArg_Parse(arg, "O!:replace", &PyCursesWindow_Type, &win)) {
        goto exit;
    }
    return_value = _curses_panel_panel_replace_impl(self, win);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_panel_panel_set_userptr__doc__,
"set_userptr($self, obj, /)\n"
"--\n"
"\n"
"Set the panel’s user pointer to obj.");

#define _CURSES_PANEL_PANEL_SET_USERPTR_METHODDEF    \
    {"set_userptr", (PyCFunction)_curses_panel_panel_set_userptr, METH_O, _curses_panel_panel_set_userptr__doc__},

PyDoc_STRVAR(_curses_panel_panel_userptr__doc__,
"userptr($self, /)\n"
"--\n"
"\n"
"Return the user pointer for the panel.");

#define _CURSES_PANEL_PANEL_USERPTR_METHODDEF    \
    {"userptr", (PyCFunction)_curses_panel_panel_userptr, METH_NOARGS, _curses_panel_panel_userptr__doc__},

static PyObject *
_curses_panel_panel_userptr_impl(PyCursesPanelObject *self);

static PyObject *
_curses_panel_panel_userptr(PyCursesPanelObject *self, PyObject *Py_UNUSED(ignored))
{
    return _curses_panel_panel_userptr_impl(self);
}

PyDoc_STRVAR(_curses_panel_bottom_panel__doc__,
"bottom_panel($module, /)\n"
"--\n"
"\n"
"Return the bottom panel in the panel stack.");

#define _CURSES_PANEL_BOTTOM_PANEL_METHODDEF    \
    {"bottom_panel", (PyCFunction)_curses_panel_bottom_panel, METH_NOARGS, _curses_panel_bottom_panel__doc__},

static PyObject *
_curses_panel_bottom_panel_impl(PyObject *module);

static PyObject *
_curses_panel_bottom_panel(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_panel_bottom_panel_impl(module);
}

PyDoc_STRVAR(_curses_panel_new_panel__doc__,
"new_panel($module, win, /)\n"
"--\n"
"\n"
"Return a panel object, associating it with the given window win.");

#define _CURSES_PANEL_NEW_PANEL_METHODDEF    \
    {"new_panel", (PyCFunction)_curses_panel_new_panel, METH_O, _curses_panel_new_panel__doc__},

static PyObject *
_curses_panel_new_panel_impl(PyObject *module, PyCursesWindowObject *win);

static PyObject *
_curses_panel_new_panel(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyCursesWindowObject *win;

    if (!PyArg_Parse(arg, "O!:new_panel", &PyCursesWindow_Type, &win)) {
        goto exit;
    }
    return_value = _curses_panel_new_panel_impl(module, win);

exit:
    return return_value;
}

PyDoc_STRVAR(_curses_panel_top_panel__doc__,
"top_panel($module, /)\n"
"--\n"
"\n"
"Return the top panel in the panel stack.");

#define _CURSES_PANEL_TOP_PANEL_METHODDEF    \
    {"top_panel", (PyCFunction)_curses_panel_top_panel, METH_NOARGS, _curses_panel_top_panel__doc__},

static PyObject *
_curses_panel_top_panel_impl(PyObject *module);

static PyObject *
_curses_panel_top_panel(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_panel_top_panel_impl(module);
}

PyDoc_STRVAR(_curses_panel_update_panels__doc__,
"update_panels($module, /)\n"
"--\n"
"\n"
"Updates the virtual screen after changes in the panel stack.\n"
"\n"
"This does not call curses.doupdate(), so you’ll have to do this yourself.");

#define _CURSES_PANEL_UPDATE_PANELS_METHODDEF    \
    {"update_panels", (PyCFunction)_curses_panel_update_panels, METH_NOARGS, _curses_panel_update_panels__doc__},

static PyObject *
_curses_panel_update_panels_impl(PyObject *module);

static PyObject *
_curses_panel_update_panels(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return _curses_panel_update_panels_impl(module);
}
/*[clinic end generated code: output=96f627ca0b08b96d input=a9049054013a1b77]*/
