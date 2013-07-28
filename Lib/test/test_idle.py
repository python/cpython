import unittest
from test import support
from test.support import import_module, use_resources

# Skip test if _thread or _tkinter wasn't built or idlelib was deleted.
import_module('threading')  # imported by PyShell, imports _thread
tk = import_module('tkinter')  # imports _tkinter
idletest = import_module('idlelib.idle_test')

# If buildbot improperly sets gui resource (#18365, #18441), remove it
# so requires('gui') tests are skipped while non-gui tests still run.
# If there is a problem with Macs, see #18441, msg 193805
if use_resources and 'gui' in use_resources:
    try:
        root = tk.Tk()
        root.destroy()
    except tk.TclError:
        while 'gui' in use_resources:
            use_resources.remove('gui')

# Without test_main present, regrtest.runtest_inner (line1219) calls
# unittest.TestLoader().loadTestsFromModule(this_module) which calls
# load_tests() if it finds it. (Unittest.main does the same.)
load_tests = idletest.load_tests

if __name__ == '__main__':
    # Until unittest supports resources, we emulate regrtest's -ugui
    # so loaded tests run the same as if textually present here.
    # If any Idle test ever needs another resource, add it to the list.
    support.use_resources = ['gui']  # use_resources is initially None
    unittest.main(verbosity=2, exit=False)
