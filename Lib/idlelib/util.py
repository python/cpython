"""
Idlelib objects with no external idlelib dependencies
which are needed in more than one idlelib module.

They are included here because
    a) they don't particularly belong elsewhere; or
    b) because inclusion here simplifies the idlelib dependency graph.

TODO:
    * Python versions (editor and help_about),
    * tk version and patchlevel (pyshell, help_about, maxos?, editor?),
    * std streams (pyshell, run),
    * warning stuff (pyshell, run).
"""
import sys

# .pyw is for Windows; .pyi is for typing stub files.
# The extension order is needed for iomenu open/save dialogs.
py_extensions = ('.py', '.pyw', '.pyi')


# fix_x functions seem only needed once per process.

def fix_scaling(root):  # Called in filelist _test, pyshell, and run.
    """Scale fonts on HiDPI displays, once per process."""
    import tkinter.font
    scaling = float(root.tk.call('tk', 'scaling'))
    if scaling > 1.4:
        for name in tkinter.font.names(root):
            font = tkinter.font.Font(root=root, name=name, exists=True)
            size = int(font['size'])
            if size < 0:
                font['size'] = round(-0.75*size)


# Fix for HiDPI screens on Windows.  CALL BEFORE ANY TK OPERATIONS!
# URL for arguments for the ...Awareness call below.
# https://msdn.microsoft.com/en-us/library/windows/desktop/dn280512(v=vs.85).aspx
if sys.platform == 'win32':  # pragma: no cover
    def fix_win_hidpi():  # Called in pyshell and turtledemo.
        try:
            import ctypes
            PROCESS_SYSTEM_DPI_AWARE = 1  # Int required.
            ctypes.OleDLL('shcore').SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE)
        except (ImportError, AttributeError, OSError):
            pass

def fix_word_breaks(root):  # Called in editor htest, filelist _test, pyshell.
    # On Windows, tcl/tk breaks 'words' only on spaces, as in Command Prompt.
    # We want Motif style everywhere. See #21474, msg218992 and followup.
    tk = root.tk
    tk.call('tcl_wordBreakAfter', 'a b', 0) # make sure word.tcl is loaded
    tk.call('set', 'tcl_wordchars', r'\w')
    tk.call('set', 'tcl_nonwordchars', r'\W')


def fix_x11_paste(root):
    "Make paste replace selection on x11.  See issue #5124."
    if root._windowingsystem == 'x11':
        for cls in 'Text', 'Entry', 'Spinbox':
            root.bind_class(
                cls,
                '<<Paste>>',
                'catch {%W delete sel.first sel.last}\n' +
                        root.bind_class(cls, '<<Paste>>'))


# Mouse wheel handling.

def x11_buttons(widget):
    """Return whether Tk reports wheel rotations to widget as button events.

    On X11, Tk 8.6 and older report a mouse wheel rotation as a
    <Button-4> or <Button-5> event.  Tk 8.7 and newer report it as a
    <MouseWheel> event, as Tk always did on Windows and macOS.  Which of
    the two a widget gets depends on its windowing system, which is a
    property of its display, so a widget is needed, not just the version.
    """
    from tkinter import TkVersion
    return TkVersion < 8.7 and widget._windowingsystem == 'x11'


def bind_wheel(widget, func):  # Called in editor and tree.
    "Bind func to the events that Tk sends widget for a wheel rotation."
    if x11_buttons(widget):
        widget.bind('<Button-4>', func)
        widget.bind('<Button-5>', func)
    else:
        widget.bind('<MouseWheel>', func)


def wheel_event(event, widget=None):
    """Handle a scrollwheel event by scrolling 5 lines.

    For a <MouseWheel> event, event.delta is 120*n on Windows and X11,
    and -1*n on macOS, where n can be > 1 if one scrolls fast.  Flicking
    the wheel generates up to maybe 20 events with n up to 10 or more.
    Macs use wheel down (delta = 1*n) to scroll up, so positive delta
    means to scroll up on all systems.

    A <Button-4> or <Button-5> event (see x11_buttons) says up or down
    by its number, and has no delta; a wheel event has no number.

    The widget parameter is needed so tree label bindings can pass the
    underlying canvas.  If tree is replaced by ttk.Treeview, it can go.

    This function depends on widget.yview to not be overridden by
    a subclass.
    """
    up = event.num == 4 if event.num in (4, 5) else event.delta > 0
    lines = -5 if up else 5
    widget = event.widget if widget is None else widget
    widget.yview('scroll', lines, 'units')
    return 'break'


if __name__ == '__main__':
    from unittest import main
    main('idlelib.idle_test.test_util', verbosity=2)
