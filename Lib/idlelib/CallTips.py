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
_self_pat = re.compile('self\,?\s*')
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
                argspec = _self_pat.sub("", argspec)

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

#################################################
#
# Test code tests CallTips.fetch_tip, get_entity, and get_argspec

def main():
    # Putting expected in docstrings results in doubled tips for test
    def t1(): "()"
    def t2(a, b=None): "(a, b=None)"
    def t3(a, *args): "(a, *args)"
    def t4(*args): "(*args)"
    def t5(a, *args): "(a, *args)"
    def t6(a, b=None, *args, **kw): "(a, b=None, *args, **kw)"

    class TC(object):
        "(ai=None, *b)"
        def __init__(self, ai=None, *b): "(self, ai=None, *b)"
        def t1(self): "(self)"
        def t2(self, ai, b=None): "(self, ai, b=None)"
        def t3(self, ai, *args): "(self, ai, *args)"
        def t4(self, *args): "(self, *args)"
        def t5(self, ai, *args): "(self, ai, *args)"
        def t6(self, ai, b=None, *args, **kw): "(self, ai, b=None, *args, **kw)"
        @classmethod
        def cm(cls, a): "(cls, a)"
        @staticmethod
        def sm(b): "(b)"
        def __call__(self, ci): "(ci)"

    tc = TC()

    # Python classes that inherit builtin methods
    class Int(int):  "Int(x[, base]) -> integer"
    class List(list): "List() -> new empty list"
    # Simulate builtin with no docstring for default argspec test
    class SB:  __call__ = None

    __main__.__dict__.update(locals())  # required for get_entity eval()

    num_tests = num_fail = 0
    tip = CallTips().fetch_tip

    def test(expression, expected):
        nonlocal num_tests, num_fail
        num_tests += 1
        argspec = tip(expression)
        if argspec != expected:
            num_fail += 1
            fmt = "%s - expected\n%r\n - but got\n%r"
            print(fmt % (expression, expected, argspec))

    def test_builtins():
        # if first line of a possibly multiline compiled docstring changes,
        # must change corresponding test string
        test('int',  "int(x[, base]) -> integer")
        test('Int',  Int.__doc__)
        test('types.MethodType', "method(function, instance)")
        test('list', "list() -> new empty list")
        test('List', List.__doc__)
        test('list.__new__',
               'T.__new__(S, ...) -> a new object with type S, a subtype of T')
        test('list.__init__',
               'x.__init__(...) initializes x; see help(type(x)) for signature')
        append_doc =  "L.append(object) -> None -- append object to end"
        test('list.append', append_doc)
        test('[].append', append_doc)
        test('List.append', append_doc)
        test('SB()', _default_callable_argspec)

    def test_funcs():
        for func  in (t1, t2, t3, t4, t5, t6, TC,):
            fdoc = func.__doc__
            test(func.__name__, fdoc + "\n" + fdoc)
        for func in (TC.t1, TC.t2, TC.t3, TC.t4, TC.t5, TC.t6, TC.cm, TC.sm):
            fdoc = func.__doc__
            test('TC.'+func.__name__, fdoc + "\n" + fdoc)

    def test_methods():
        for func  in (tc.t1, tc.t2, tc.t3, tc.t4, tc.t5, tc.t6):
            fdoc = func.__doc__
            test('tc.'+func.__name__, _self_pat.sub("", fdoc) + "\n" + fdoc)
        fdoc = tc.__call__.__doc__
        test('tc', fdoc + "\n" + fdoc)

    def test_non_callables():
        # expression evaluates, but not to a callable
        for expr in ('0', '0.0' 'num_tests', b'num_tests', '[]', '{}'):
            test(expr, '')
        # expression does not evaluate, but raises an exception
        for expr in ('1a', 'xyx', 'num_tests.xyz', '[int][1]', '{0:int}[1]'):
            test(expr, '')

    test_builtins()
    test_funcs()
    test_non_callables()
    test_methods()

    print("%d of %d tests failed" % (num_fail, num_tests))

if __name__ == '__main__':
    main()
