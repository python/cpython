README FOR IDLE TESTS IN IDLELIB.IDLE_TEST


1. Test Files

The idle directory, idlelib, has over 60 xyz.py files. The idle_test
subdirectory should contain a test_xyy.py for each. (For test modules, make
'xyz' lower case, and possibly shorten it.) Each file should start with the
something like the following template, with the blanks after after '.' and 'as',
and before and after '_' filled in.
---
import unittest
from test.support import requires
import idlelib. as

class _Test(unittest.TestCase):

    def test_(self):

if __name__ == '__main__':
    unittest.main(verbosity=2, exit=2)
---
Idle tests are run with unittest; do not use regrtest's test_main.

Once test_xyy is written, the following should go at the end of xyy.py,
with xyz (lowercased) added after 'test_'.
---
if __name__ == "__main__":
    from test import support; support.use_resources = ['gui']
    import unittest
    unittest.main('idlelib.idle_test.test_', verbosity=2, exit=False)
---


2. Gui Tests

Gui tests need 'requires' and 'use_resources' from test.support
(test.test_support in 2.7). A test is a gui test if it creates a Tk root or
master object either directly or indirectly by instantiating a tkinter or
idle class. For the benefit of buildbot machines that do not have a graphics
screen, gui tests must be 'guarded' by "requires('gui')" in a setUp
function or method. This will typically be setUpClass.

All gui objects must be destroyed by the end of the test, perhaps in a tearDown
function. Creating the Tk root directly in a setUp allows a reference to be saved
so it can be properly destroyed in the corresponding tearDown. 
---
    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = tk.Tk()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
---

Support.requires('gui') returns true if it is either called in a main module
(which never happens on buildbots) or if use_resources contains 'gui'.
Use_resources is set by test.regrtest but not by unittest. So when running
tests in another module with unittest, we set it ourselves, as in the xyz.py
template above.

Since non-gui tests always run, but gui tests only sometimes, tests of non-gui
operations should best avoid needing a gui. Methods that make incidental use of
tkinter (tk) variables and messageboxes can do this by using the mock classes in
idle_test/mock_tk.py. There is also a mock text that will handle some uses of the
tk Text widget.


3. Running Tests

Assume that xyz.py and test_xyz.py end with the "if __name__" statements given
above. In Idle, pressing F5 in an editor window with either loaded will run all
tests in the test_xyz file with the version of Python running Idle.  The test
report and any tracebacks will appear in the Shell window. The options in these
"if __name__" statements are appropriate for developers running (as opposed to
importing) either of the files during development: verbosity=2 lists all test
methods in the file; exit=False avoids a spurious sys.exit traceback that would
otherwise occur when running in Idle. The following command lines also run
all test methods, including gui tests, in test_xyz.py. (The exceptions are that
idlelib and idlelib.idle start Idle and idlelib.PyShell should (issue 18330).)

python -m idlelib.xyz  # With the capitalization of the xyz module
python -m idlelib.idle_test.test_xyz

To run all idle_test/test_*.py tests, either interactively
('>>>', with unittest imported) or from a command line, use one of the
following. (Notes: unittest does not run gui tests; in 2.7, 'test ' (with the
space) is 'test.regrtest '; where present, -v and -ugui can be omitted.)

>>> unittest.main('idlelib.idle_test', verbosity=2, exit=False)
python -m unittest -v idlelib.idle_test
python -m test -v -ugui test_idle
python -m test.test_idle

The idle tests are 'discovered' by idlelib.idle_test.__init__.load_tests,
which is also imported into test.test_idle. Normally, neither file should be
changed when working on individual test modules. The third command runs runs
unittest indirectly through regrtest. The same happens when the entire test
suite is run with 'python -m test'. So that command must work for buildbots
to stay green. Idle tests must not disturb the environment in a way that
makes other tests fail (issue 18081).

To run an individual Testcase or test method, extend the dotted name given to
unittest on the command line. (But gui tests will not this way.)

python -m unittest -v idlelib.idle_test.text_xyz.Test_case.test_meth
