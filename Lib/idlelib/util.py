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
import tkinter  # This module requires gui to run (import).

# .pyw is for Windows; .pyi is for typing stub files.
# The extension order is needed for iomenu open/save dialogs.
py_extensions = ('.py', '.pyw', '.pyi')


# fix_x functions seem needed only once per process.

def fix_scaling(root):  # Called in filelist _test, pyshell, and run.
    """Scale fonts on HiDPI displays, once per process."""
    import tkinter.font
    scaling = root.tk_scaling()  # tkinter method new in 3.16
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


# On X11, Tk 8.6- signals mouse wheel rotations with <Button-4> and
# <Button-5> events.  With 8.7+, it generates <Mousewheel events
# as on other systems.  Used here, editor, tree, and test_sidebar.
root = tkinter.Tk()  # Use this as process root?
root.withdraw()
x11_buttons = root._windowingsystem == 'x11' and tkinter.TkVersion <= 8.6
root.destroy()

def wheel_event(event, widget=None):  # Bound in editor and tree.
    """Handle scrollwheel event.

    For MouseWheel up events, event.delta is generally= 120*n, but -1*n
    on darwin (or just Aqua?), where n can be > 1 if one scrolls fast.
    Macs use wheel down (delta = 1*n) to scroll up, so positive
    delta means to scroll up on both systems.

    When x11_buttons is True, Button-4,5 events signal up and down.

    The widget parameter is needed in tree so browser events can be
    passed to the underlying canvas vertical scrollbar.  If tree is
    replaced by ttk.Treeview, 'widget' can go.
    """
    up = event.num == 4 if x11_buttons else event.delta > 0
    # The following works, though +-4*n might be more standard,
    lines = -5 if up else 5
    widget = event.widget if widget is None else widget
    widget.yview('scroll', lines, 'units')
    return 'break'


if __name__ == '__main__':
    from unittest import main
    main('idlelib.idle_test.test_util', verbosity=2)
