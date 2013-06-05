"""Classes that replace tkinter gui objects used by an object being tested.
A gui object is anything with a master or parent paramenter, which is typically
required in spite of what the doc strings say.
"""

class Var(object):
    "Use for String/Int/BooleanVar: incomplete"
    def __init__(self, master=None, value=None, name=None):
        self.master = master
        self.value = value
        self.name = name
    def set(self, value):
        self.value = value
    def get(self):
        return self.value

class Mbox_func(object):
    """Generic mock for messagebox functions. All have same call signature.
    Mbox instantiates once for each function. Tester uses attributes.
    """
    def __init__(self):
        self.result = None  # The return for all show funcs
    def __call__(self, title, message, *args, **kwds):
        # Save all args for possible examination by tester
        self.title = title
        self.message = message
        self.args = args
        self.kwds = kwds
        return self.result  # Set by tester for ask functions

class Mbox(object):
    """Mock for tkinter.messagebox with an Mbox_func for each function.
    This module was 'tkMessageBox' in 2.x; hence the 'import as' in  3.x.
    Example usage in test_module.py for testing functios in module.py:
    ---
from idlelib.idle_test.mock_tk import Mbox
import module

orig_mbox = module.tkMessageBox
showerror = Mbox.showerror  # example, for attribute access in test methods

class Test(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        module.tkMessageBox = Mbox

    @classmethod
    def tearDownClass(cls):
        module.tkMessageBox = orig_mbox
    ---
    When tkMessageBox functions are the only gui making calls in a method,
    this replacement makes the method gui-free and unit-testable.
    For 'ask' functions, set func.result return before calling method.
    """
    askokcancel = Mbox_func()     # True or False
    askquestion = Mbox_func()     # 'yes' or 'no'
    askretrycancel = Mbox_func()  # True or False
    askyesno = Mbox_func()        # True or False
    askyesnocancel = Mbox_func()  # True, False, or None
    showerror = Mbox_func()    # None
    showinfo = Mbox_func()     # None
    showwarning = Mbox_func()  # None
