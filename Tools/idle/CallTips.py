import string
import sys
import types

class CallTips:

    menudefs = [
    ]

    keydefs = {
        '<<paren-open>>': ['<Key-parenleft>'],
        '<<paren-close>>': ['<Key-parenright>'],
        '<<check-calltip-cancel>>': ['<KeyRelease>', '<ButtonRelease>']
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
        # dont return "break" so the key is inserted.

    def paren_close_event(self, event):
        # Now just hides, but later we should check if other
        # paren'd expressions remain open.
        # dont return "break" so the key is inserted.
        self._remove_calltip_window()

    def check_calltip_cancel_event(self, event):
        # This doesnt quite work correctly as it is processed
        # _before_ the key is handled.  Thus, when the "Up" key
        # is pressed, this test happens before the cursor is moved.
        # This will do for now.
        if self.calltip:
            # If we have moved before the start of the calltip,
            # or off the calltip line, then cancel the tip.
            # (Later need to be smarter about multi-line, etc)
            if self.text.compare("insert", "<=", self.calltip_start) or \
               self.text.compare("insert", ">", self.calltip_start + " lineend"):
                self._remove_calltip_window()
        
        # dont return "break" so the event is handled normally.
        
    def get_object_at_cursor(self,
                             wordchars="._" + string.uppercase + string.lowercase + string.digits):
        # XXX - This need to be moved to a better place
        # as the "." attribute lookup code can also use it.
        text = self.text
        index = 0
        while 1:
            index = index + 1
            indexspec = "insert-%dc" % index
            ch = text.get(indexspec)
            if ch not in wordchars:
                break
            if text.compare(indexspec, "<=", "1.0"):
                break
        # Now attempt to locate the object.
	# How is this for a hack!
	word = text.get("insert-%dc" % (index-1,), "insert")
	if word:
            import sys, __main__
            namespace = sys.modules.copy()
            namespace.update(__main__.__dict__)
            try:
                    return eval(word, namespace)
            except:
                    pass
        return None # Can't find an object.
		
def get_arg_text(ob):
    # Get a string describing the arguments for the given object.
    argText = ""
    if ob is not None:
        argOffset = 0
        # bit of a hack for methods - turn it into a function
        # but we drop the "self" param.
        if type(ob)==types.MethodType:
            ob = ob.im_func
            argOffset = 1
        # Try and build one for Python defined functions
        if type(ob) in [types.FunctionType, types.LambdaType]:
            try:
                realArgs = ob.func_code.co_varnames[argOffset:ob.func_code.co_argcount]
                defaults = ob.func_defaults or []
                defaults = list(map(lambda name: "=%s" % name, defaults))
                defaults = [""] * (len(realArgs)-len(defaults)) + defaults
                argText = string.join( map(lambda arg, dflt: arg+dflt, realArgs, defaults), ", ")
                if len(realArgs)+argOffset < (len(ob.func_code.co_varnames) - len(ob.func_code.co_names) ):
                    if argText: argText = argText + ", "
                    argText = argText + "..."
                argText = "(%s)" % argText
            except:
                pass
        # Can't build an argument list - see if we can use a docstring.
        if not argText and hasattr(ob, "__doc__") and ob.__doc__:
            pos = string.find(ob.__doc__, "\n")
            if pos<0 or pos>70: pos=70
            argText = ob.__doc__[:pos]

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
    def t6(a, b=None, *args, **kw): "(a, b=None, ...)"

    class TC:
        def t1(self): "()"
        def t2(self, a, b=None): "(a, b=None)"
        def t3(self, a, *args): "(a, ...)"
        def t4(self, *args): "(...)"
        def t5(self, a, *args): "(a, ...)"
        def t6(self, a, b=None, *args, **kw): "(a, b=None, ...)"


    def test( tests ):
        failed=[]
        for t in tests:
            if get_arg_text(t) != t.__doc__:
                failed.append(t)
                print "%s - expected %s, but got %s" % (t, `t.__doc__`, `get_arg_text(t)`)
        print "%d of %d tests failed" % (len(failed), len(tests))

        tc = TC()
        tests = t1, t2, t3, t4, t5, t6, \
                tc.t1, tc.t2, tc.t3, tc.t4, tc.t5, tc.t6

    test(tests)
