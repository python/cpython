# CallTips.py - An IDLE extension that provides "Call Tips" - ie, a floating window that
# displays parameter information as you open parens.

import string
import sys
import types

class CallTips:

    menudefs = [
    ]

    keydefs = {
        '<<paren-open>>': ['<Key-parenleft>'],
        '<<paren-close>>': ['<Key-parenright>'],
        '<<check-calltip-cancel>>': ['<KeyRelease>'],
        '<<calltip-cancel>>': ['<ButtonPress>', '<Key-Escape>'],
    }

    windows_keydefs = {
    }

    unix_keydefs = {
    }

    def __init__(self, editwin):
        self.editwin = editwin
        self.text = editwin.text
        self.calltip = None
        if hasattr(self.text, "make_calltip_window"):
            self._make_calltip_window = self.text.make_calltip_window
        else:
            self._make_calltip_window = self._make_tk_calltip_window

    def close(self):
        self._make_calltip_window = None

    # Makes a Tk based calltip window.  Used by IDLE, but not Pythonwin.
    # See __init__ above for how this is used.
    def _make_tk_calltip_window(self):
        import CallTipWindow
        return CallTipWindow.CallTip(self.text)

    def _remove_calltip_window(self):
        if self.calltip:
            self.calltip.hidetip()
            self.calltip = None
        
    def paren_open_event(self, event):
        self._remove_calltip_window()
        arg_text = get_arg_text(self.get_object_at_cursor())
        if arg_text:
            self.calltip_start = self.text.index("insert")
            self.calltip = self._make_calltip_window()
            self.calltip.showtip(arg_text)
        return "" #so the event is handled normally.

    def paren_close_event(self, event):
        # Now just hides, but later we should check if other
        # paren'd expressions remain open.
        self._remove_calltip_window()
        return "" #so the event is handled normally.

    def check_calltip_cancel_event(self, event):
        if self.calltip:
            # If we have moved before the start of the calltip,
            # or off the calltip line, then cancel the tip.
            # (Later need to be smarter about multi-line, etc)
            if self.text.compare("insert", "<=", self.calltip_start) or \
               self.text.compare("insert", ">", self.calltip_start + " lineend"):
                self._remove_calltip_window()
        return "" #so the event is handled normally.

    def calltip_cancel_event(self, event):
        self._remove_calltip_window()
        return "" #so the event is handled normally.

    def get_object_at_cursor(self,
                             wordchars="._" + string.uppercase + string.lowercase + string.digits):
        # XXX - This needs to be moved to a better place
        # so the "." attribute lookup code can also use it.
        text = self.text
        chars = text.get("insert linestart", "insert")
        i = len(chars)
        while i and chars[i-1] in wordchars:
            i = i-1
        word = chars[i:]
        if word:
            # How is this for a hack!
            import sys, __main__
            namespace = sys.modules.copy()
            namespace.update(__main__.__dict__)
            try:
                    return eval(word, namespace)
            except:
                    pass
        return None # Can't find an object.

def _find_constructor(class_ob):
    # Given a class object, return a function object used for the
    # constructor (ie, __init__() ) or None if we can't find one.
    try:
        return class_ob.__init__.im_func
    except AttributeError:
        for base in class_ob.__bases__:
            rc = _find_constructor(base)
            if rc is not None: return rc
    return None

def get_arg_text(ob):
    # Get a string describing the arguments for the given object.
    argText = ""
    if ob is not None:
        argOffset = 0
        if type(ob)==types.ClassType:
            # Look for the highest __init__ in the class chain.
            fob = _find_constructor(ob)
            if fob is None:
                fob = lambda: None
            else:
                argOffset = 1
        elif type(ob)==types.MethodType:
            # bit of a hack for methods - turn it into a function
            # but we drop the "self" param.
            fob = ob.im_func
            argOffset = 1
        else:
            fob = ob
        # Try and build one for Python defined functions
        if type(fob) in [types.FunctionType, types.LambdaType]:
            try:
                realArgs = fob.func_code.co_varnames[argOffset:fob.func_code.co_argcount]
                defaults = fob.func_defaults or []
                defaults = list(map(lambda name: "=%s" % name, defaults))
                defaults = [""] * (len(realArgs)-len(defaults)) + defaults
                items = map(lambda arg, dflt: arg+dflt, realArgs, defaults)
                if fob.func_code.co_flags & 0x4:
                    items.append("...")
                if fob.func_code.co_flags & 0x8:
                    items.append("***")
                argText = string.join(items , ", ")
                argText = "(%s)" % argText
            except:
                pass
        # See if we can use the docstring
        if hasattr(ob, "__doc__") and ob.__doc__:
            pos = string.find(ob.__doc__, "\n")
            if pos<0 or pos>70: pos=70
            if argText: argText = argText + "\n"
            argText = argText + ob.__doc__[:pos]

    return argText

#################################################
#
# Test code
#
if __name__=='__main__':

    def t1(): "()"
    def t2(a, b=None): "(a, b=None)"
    def t3(a, *args): "(a, ...)"
    def t4(*args): "(...)"
    def t5(a, *args): "(a, ...)"
    def t6(a, b=None, *args, **kw): "(a, b=None, ..., ***)"

    class TC:
        "(a=None, ...)"
        def __init__(self, a=None, *b): "(a=None, ...)"
        def t1(self): "()"
        def t2(self, a, b=None): "(a, b=None)"
        def t3(self, a, *args): "(a, ...)"
        def t4(self, *args): "(...)"
        def t5(self, a, *args): "(a, ...)"
        def t6(self, a, b=None, *args, **kw): "(a, b=None, ..., ***)"

    def test( tests ):
        failed=[]
        for t in tests:
            expected = t.__doc__ + "\n" + t.__doc__
            if get_arg_text(t) != expected:
                failed.append(t)
                print "%s - expected %s, but got %s" % (t, `expected`, `get_arg_text(t)`)
        print "%d of %d tests failed" % (len(failed), len(tests))

    tc = TC()
    tests = t1, t2, t3, t4, t5, t6, \
            TC, tc.t1, tc.t2, tc.t3, tc.t4, tc.t5, tc.t6

    test(tests)
