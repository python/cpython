README FOR IDLE TESTS IN IDLELIB.IDLE_TEST

The idle directory, idlelib, has over 60 xyz.py files. The idle_test
subdirectory should contain a test_xyy.py for each one. (For test modules,
make 'xyz' lower case.) Each should start with the following cut-paste
template, with the blanks after after '.'. 'as', and '_' filled in.
---
import unittest
import idlelib. as

class Test_(unittest.TestCase):

    def test_(self):

if __name__ == '__main__':
    unittest.main(verbosity=2, exit=2)
---
Idle tests are run with unittest; do not use regrtest's test_main.

Once test_xyy is written, the following should go at the end of xyy.py,
with xyz (lowercased) added after 'test_'.
---
if __name__ == "__main__":
    import unittest
    unittest.main('idlelib.idle_test.test_', verbosity=2, exit=False)
---

In Idle, pressing F5 in an editor window with either xyz.py or test_xyz.py
loaded will then run the test with the version of Python running Idle and
tracebacks will appear in the Shell window. The options are appropriate for
developers running (as opposed to importing) either type of file during
development: verbosity=2 lists all test_y methods; exit=False avoids a
spurious sys.exit traceback when running in Idle. The following command
lines also run test_xyz.py

python -m idlelib.xyz  # With the capitalization of the xyz module
python -m unittest -v idlelib.idle_test.test_xyz

To run all idle tests either interactively ('>>>', with unittest imported)
or from a command line, use one of the following.

>>> unittest.main('idlelib.idle_test', verbosity=2, exit=False)
python -m unittest -v idlelib.idle_test
python -m test.test_idle
python -m test test_idle

The idle tests are 'discovered' in idlelib.idle_test.__init__.load_tests,
which is also imported into test.test_idle. Normally, neither file should be
changed when working on individual test modules. The last command runs runs
unittest indirectly through regrtest. The same happens when the entire test
suite is run with 'python -m test'. So it must work for buildbots to stay green.

To run an individual Testcase or test method, extend the
dotted name given to unittest on the command line.

python -m unittest -v idlelib.idle_test.text_xyz.Test_case.test_meth

To disable test/test_idle.py, there are at least two choices.
a. Comment out 'load_tests' line, no no tests are discovered (simple and safe);
Running no tests passes, so there is no indication that nothing was run.
b.Before that line, make module an unexpected skip for regrtest with
import unittest; raise unittest.SkipTest('skip for buildbots')
When run directly with unittest, this causes a normal exit and traceback.