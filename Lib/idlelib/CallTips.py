"""CallTips.py - An IDLE Extension to Jog Your Memory

Call Tips are floating windows which display function, class, and method
parameter and docstring information when you type an opening parenthesis, and
which disappear when you type a closing parenthesis.

"""
import re
import sys
import types
import inspect

from idlelib import CallTipWindow
from idlelib.HyperParser import HyperParser

import __main__

class CallTips:

    menudefs = [
        ('edit', [
            ("Show call tip", "<<force-open-calltip>>"),
        ])
    ]

    def __init__(self, editwin=None):
        if editwin is None:  # subprocess and test
            self.editwin = None
        else:
            self.editwin = editwin
            self.text = editwin.text
            self.active_calltip = None
            self._calltip_window = self._make_tk_calltip_window

    def close(self):
        self._calltip_window = None

    def _make_tk_calltip_window(self):
        # See __init__ for usage
        return CallTipWindow.CallTip(self.text)

    def _remove_calltip_window(self, event=None):
        if self.active_calltip:
            self.active_calltip.hidetip()
            self.active_calltip = None

    def force_open_calltip_event(self, event):
        "The user selected the menu entry or hotkey, open the tip."
        self.open_calltip(True)

    def try_open_calltip_event(self, event):
        """Happens when it would be nice to open a CallTip, but not really
        necessary, for example after an opening bracket, so function calls
        won't be made.
        """
        self.open_calltip(False)

    def refresh_calltip_event(self, event):
        if self.active_calltip and self.active_calltip.is_active():
            self.open_calltip(False)

    def open_calltip(self, evalfuncs):
        self._remove_calltip_window()

        hp = HyperParser(self.editwin, "insert")
        sur_paren = hp.get_surrounding_brackets('(')
        if not sur_paren:
            return
        hp.set_index(sur_paren[0])
        expression  = hp.get_expression()
        if not expression:
            return
        if not evalfuncs and (expression.find('(') != -1):
            return
        argspec = self.fetch_tip(expression)
        if not argspec:
            return
        self.active_calltip = self._calltip_window()
        self.active_calltip.showtip(argspec, sur_paren[0], sur_paren[1])

    def fetch_tip(self, expression):
        """Return the argument list and docstring of a function or class.

        If there is a Python subprocess, get the calltip there.  Otherwise,
        either this fetch_tip() is running in the subprocess or it was
        called in an IDLE running without the subprocess.

        The subprocess environment is that of the most recently run script.  If
        two unrelated modules are being edited some calltips in the current
        module may be inoperative if the module was not the last to run.

        To find methods, fetch_tip must be fed a fully qualified name.

        """
        try:
            rpcclt = self.editwin.flist.pyshell.interp.rpcclt
        except AttributeError:
            rpcclt = None
        if rpcclt:
            return rpcclt.remotecall("exec", "get_the_calltip",
                                     (expression,), {})
        else:
            return get_argspec(get_entity(expression))

def get_entity(expression):
    """Return the object corresponding to expression evaluated
    in a namespace spanning sys.modules and __main.dict__.
    """
    if expression:
        namespace = sys.modules.copy()
        namespace.update(__main__.__dict__)
        try:
            return eval(expression, namespace)
        except BaseException:
            # An uncaught exception closes idle, and eval can raise any
            # exception, especially if user classes are involved.
            return None

# The following are used in both get_argspec and tests
_first_param = re.compile('(?<=\()\w*\,?\s*')
_default_callable_argspec = "No docstring, see docs."

def get_argspec(ob):
    '''Return a string describing the arguments and return of a callable object.

    For Python-coded functions and methods, the first line is introspected.
    Delete 'self' parameter for classes (.__init__) and bound methods.
    The last line is the first line of the doc string.  For builtins, this typically
    includes the arguments in addition to the return value.

    '''
    argspec = ""
    if hasattr(ob, '__call__'):
        if isinstance(ob, type):
            fob = getattr(ob, '__init__', None)
        elif isinstance(ob.__call__, types.MethodType):
            fob = ob.__call__
        else:
            fob = ob
        if isinstance(fob, (types.FunctionType, types.MethodType)):
            argspec = inspect.formatargspec(*inspect.getfullargspec(fob))
            if (isinstance(ob, (type, types.MethodType)) or
                    isinstance(ob.__call__, types.MethodType)):
                argspec = _first_param.sub("", argspec)

        if isinstance(ob.__call__, types.MethodType):
            doc = ob.__call__.__doc__
        else:
            doc = getattr(ob, "__doc__", "")
        if doc:
            doc = doc.lstrip()
            pos = doc.find("\n")
            if pos < 0 or pos > 70:
                pos = 70
            if argspec:
                argspec += "\n"
            argspec += doc[:pos]
        if not argspec:
            argspec = _default_callable_argspec
    return argspec

if __name__ == '__main__':
    from unittest import main
    main('idlelib.idle_test.test_calltips', verbosity=2, exit=False)
