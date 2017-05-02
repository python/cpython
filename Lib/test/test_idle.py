import unittest
from test import support
from test.support import import_module

# Skip test if _thread or _tkinter wasn't built, or idlelib is missing,
# or if tcl/tk version before 8.5, which is needed for ttk widgets.

import_module('threading')  # imported by PyShell, imports _thread
tk = import_module('tkinter')  # imports _tkinter
idlelib = import_module('idlelib')
idlelib.testing = True  # Avoid locale-changed test error

# Without test_main present, test.libregrtest.runtest.runtest_inner
# calls (line 173) unittest.TestLoader().loadTestsFromModule(module)
# which calls load_tests() if it finds it. (Unittest.main does the same.)
from idlelib.idle_test import load_tests

if __name__ == '__main__':
    unittest.main(verbosity=2, exit=False)
