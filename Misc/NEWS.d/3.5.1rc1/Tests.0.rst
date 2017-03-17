- Issue #25449: Added tests for OrderedDict subclasses.

- Issue #25099: Make test_compileall not fail when an entry on sys.path cannot
  be written to (commonly seen in administrative installs on Windows).

- Issue #23919: Prevents assert dialogs appearing in the test suite.

- ``PCbuild\rt.bat`` now accepts an unlimited number of arguments to pass along
  to regrtest.py.  Previously there was a limit of 9.

