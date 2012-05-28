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
        name = hp.get_expression()
        if not name:
            return
        if not evalfuncs and (name.find('(') != -1):
            return
        argspec = self.fetch_tip(name)
        if not argspec:
            return
        self.active_calltip = self._calltip_window()
        self.active_calltip.showtip(argspec, sur_paren[0], sur_paren[1])

    def fetch_tip(self, name):
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
        except:
            rpcclt = None
        if rpcclt:
            return rpcclt.remotecall("exec", "get_the_calltip",
                                     (name,), {})
        else:
            entity = self.get_entity(name)
            return get_argspec(entity)

    def get_entity(self, name):
        "Lookup name in a namespace spanning sys.modules and __main.dict__."
        if name:
            namespace = sys.modules.copy()
            namespace.update(__main__.__dict__)
            try:
                return eval(name, namespace)
                # any exception is possible if evalfuncs True in open_calltip
                # at least Syntax, Name, Attribute, Index, and Key E. if not
            except:
                return None

def _find_constructor(class_ob):
    "Find the nearest __init__() in the class tree."
    try:
        return class_ob.__init__.__func__
    except AttributeError:
        for base in class_ob.__bases__:
            init = _find_constructor(base)
            if init:
                return init
        return None

def get_argspec(ob):
    """Get a string describing the arguments for the given object,
       only if it is callable."""
    argspec = ""
    if ob is not None and hasattr(ob, '__call__'):
        if isinstance(ob, type):
            fob = _find_constructor(ob)
            if fob is None:
                fob = lambda: None
        elif isinstance(ob, types.MethodType):
            fob = ob.__func__
        else:
            fob = ob
        if isinstance(fob, (types.FunctionType, types.LambdaType)):
            argspec = inspect.formatargspec(*inspect.getfullargspec(fob))
            pat = re.compile('self\,?\s*')
            argspec = pat.sub("", argspec)
        doc = getattr(ob, "__doc__", "")
        if doc:
            doc = doc.lstrip()
            pos = doc.find("\n")
            if pos < 0 or pos > 70:
                pos = 70
            if argspec:
                argspec += "\n"
            argspec += doc[:pos]
    return argspec

#################################################
#
# Test code
#
def main():
    def t1(): "()"
    def t2(a, b=None): "(a, b=None)"
    def t3(a, *args): "(a, *args)"
    def t4(*args): "(*args)"
    def t5(a, *args): "(a, *args)"
    def t6(a, b=None, *args, **kw): "(a, b=None, *args, **kw)"

    class TC(object):
        "(ai=None, *b)"
        def __init__(self, ai=None, *b): "(ai=None, *b)"
        def t1(self): "()"
        def t2(self, ai, b=None): "(ai, b=None)"
        def t3(self, ai, *args): "(ai, *args)"
        def t4(self, *args): "(*args)"
        def t5(self, ai, *args): "(ai, *args)"
        def t6(self, ai, b=None, *args, **kw): "(ai, b=None, *args, **kw)"

    __main__.__dict__.update(locals())

    def test(tests):
        ct = CallTips()
        failed=[]
        for t in tests:
            expected = t.__doc__ + "\n" + t.__doc__
            name = t.__name__
            # exercise fetch_tip(), not just get_argspec()
            try:
                qualified_name = "%s.%s" % (t.__self__.__class__.__name__, name)
            except AttributeError:
                qualified_name = name
            argspec = ct.fetch_tip(qualified_name)
            if argspec != expected:
                failed.append(t)
                fmt = "%s - expected %s, but got %s"
                print(fmt % (t.__name__, expected, get_argspec(t)))
        print("%d of %d tests failed" % (len(failed), len(tests)))

    tc = TC()
    tests = (t1, t2, t3, t4, t5, t6,
             TC, tc.t1, tc.t2, tc.t3, tc.t4, tc.t5, tc.t6)

    test(tests)

if __name__ == '__main__':
    main()
