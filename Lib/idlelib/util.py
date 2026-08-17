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


if __name__ == '__main__':
    from unittest import main
    main('idlelib.idle_test.test_util', verbosity=2)
