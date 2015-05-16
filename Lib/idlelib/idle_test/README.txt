README FOR IDLE TESTS IN IDLELIB.IDLE_TEST

0. Quick Start

Automated unit tests were added in 2.7 for Python 2.x and 3.3 for Python 3.x.
To run the tests from a command line:

python -m test.test_idle

Human-mediated tests were added later in 2.7 and in 3.4.

python -m idlelib.idle_test.htest


1. Test Files

The idle directory, idlelib, has over 60 xyz.py files. The idle_test
subdirectory should contain a test_xyz.py for each, where 'xyz' is lowercased
even if xyz.py is not. Here is a possible template, with the blanks after after
'.' and 'as', and before and after '_' to be filled in.

import unittest
from test.support import requires
import idlelib. as

class _Test(unittest.TestCase):

    def test_(self):

if __name__ == '__main__':
    unittest.main(verbosity=2)

Add the following at the end of xyy.py, with the appropriate name added after
'test_'. Some files already have something like this for htest.  If so, insert
the import and unittest.main lines before the htest lines.

if __name__ == "__main__":
    import unittest
    unittest.main('idlelib.idle_test.test_', verbosity=2, exit=False)



2. GUI Tests

When run as part of the Python test suite, Idle gui tests need to run
test.support.requires('gui') (test.test_support in 2.7).  A test is a gui test
if it creates a Tk root or master object either directly or indirectly by
instantiating a tkinter or idle class.  For the benefit of test processes that
either have no graphical environment available or are not allowed to use it, gui
tests must be 'guarded' by "requires('gui')" in a setUp function or method.
This will typically be setUpClass.

To avoid interfering with other gui tests, all gui objects must be destroyed and
deleted by the end of the test.  Widgets, such as a Tk root, created in a setUpX
function, should be destroyed in the corresponding tearDownX.  Module and class
widget attributes should also be deleted..

    @classmethod
    def setUpClass(cls):
        requires('gui')
        cls.root = tk.Tk()

    @classmethod
    def tearDownClass(cls):
        cls.root.destroy()
        del cls.root


Requires('gui') causes the test(s) it guards to be skipped if any of
a few conditions are met:
    
 - The tests are being run by regrtest.py, and it was started without enabling
   the "gui" resource with the "-u" command line option.
   
 - The tests are being run on Windows by a service that is not allowed to
   interact with the graphical environment.
   
 - The tests are being run on Mac OSX in a process that cannot make a window
   manager connection.
   
 - tkinter.Tk cannot be successfully instantiated for some reason.
 
 - test.support.use_resources has been set by something other than
   regrtest.py and does not contain "gui".
   
Tests of non-gui operations should avoid creating tk widgets. Incidental uses of
tk variables and messageboxes can be replaced by the mock classes in
idle_test/mock_tk.py. The mock text handles some uses of the tk Text widget.


3. Running Unit Tests

Assume that xyz.py and test_xyz.py both end with a unittest.main() call.
Running either from an Idle editor runs all tests in the test_xyz file with the
version of Python running Idle.  Test output appears in the Shell window.  The
'verbosity=2' option lists all test methods in the file, which is appropriate
when developing tests. The 'exit=False' option is needed in xyx.py files when an
htest follows.

The following command lines also run all test methods, including
gui tests, in test_xyz.py. (Both '-m idlelib' and '-m idlelib.idle' start
Idle and so cannot run tests.)

python -m idlelib.xyz
python -m idlelib.idle_test.test_xyz

The following runs all idle_test/test_*.py tests interactively.

>>> import unittest
>>> unittest.main('idlelib.idle_test', verbosity=2)

The following run all Idle tests at a command line.  Option '-v' is the same as
'verbosity=2'.  (For 2.7, replace 'test' in the second line with
'test.regrtest'.)

python -m unittest -v idlelib.idle_test
python -m test -v -ugui test_idle
python -m test.test_idle

The idle tests are 'discovered' by idlelib.idle_test.__init__.load_tests,
which is also imported into test.test_idle. Normally, neither file should be
changed when working on individual test modules. The third command runs
unittest indirectly through regrtest. The same happens when the entire test
suite is run with 'python -m test'. So that command must work for buildbots
to stay green. Idle tests must not disturb the environment in a way that
makes other tests fail (issue 18081).

To run an individual Testcase or test method, extend the dotted name given to
unittest on the command line.

python -m unittest -v idlelib.idle_test.test_xyz.Test_case.test_meth


4. Human-mediated Tests

Human-mediated tests are widget tests that cannot be automated but need human
verification. They are contained in idlelib/idle_test/htest.py, which has
instructions.  (Some modules need an auxiliary function, identified with # htest
# on the header line.)  The set is about complete, though some tests need
improvement. To run all htests, run the htest file from an editor or from the
command line with:

python -m idlelib.idle_test.htest
