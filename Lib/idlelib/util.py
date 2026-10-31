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
import re
import sys

# .pyw is for Windows; .pyi is for typing stub files.
# The extension order is needed for iomenu open/save dialogs.
py_extensions = ('.py', '.pyw', '.pyi')


# fix_x functions seem only needed once per process.

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


_cli_token_re = re.compile(r"""
    (?P<backslashes>\\*)(?P<quotes>"+)
  | (?P<literal>\\+|[^ \t"\\]+)  # backslashes not followed by a quote
  | (?P<space>[ \t]+)
""", re.VERBOSE)


def _split_windows(cli_string):
    """Split a command line into arguments as the C runtime does.

    See https://learn.microsoft.com/cpp/c-language/parsing-c-command-line-arguments
    """
    args = []
    arg = None  # None when not in an argument.
    quoted = False
    for m in _cli_token_re.finditer(cli_string):
        match m.lastgroup:
            case 'space' if not quoted:
                if arg is not None:
                    args.append(arg)
                    arg = None
            case 'space' | 'literal':
                arg = (arg or '') + m[0]
            case _:  # Backslashes followed by quotes.
                count = len(m['backslashes'])
                escaped = count % 2  # Odd backslashes escape a quote.
                bare = len(m['quotes']) - escaped
                # In a quoted part every two quotes give a literal quote;
                # if not quoted, the first quote opens a quoted part.
                literal = escaped + ((bare + quoted - 1) // 2 if bare else 0)
                arg = (arg or '') + '\\' * (count // 2) + '"' * literal
                quoted ^= bare % 2
    if arg is not None:
        args.append(arg)
    return args


def split_cli_args(cli_string):  # Called in query.
    "Split a command line as the Python executable does (gh-93016)."
    if sys.platform == 'win32':
        return _split_windows(cli_string)
    import shlex
    return shlex.split(cli_string)


def join_cli_args(cli_args):  # Called in query.
    "Join arguments into a command line which split_cli_args() splits back."
    if sys.platform == 'win32':
        import subprocess
        return subprocess.list2cmdline(cli_args)
    import shlex
    return shlex.join(cli_args)


if __name__ == '__main__':
    from unittest import main
    main('idlelib.idle_test.test_util', verbosity=2)
