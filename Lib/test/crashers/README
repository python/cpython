This directory only contains tests for outstanding bugs that cause the
interpreter to segfault.  Ideally this directory should always be empty, but
sometimes it may not be easy to fix the underlying cause and the bug is deemed
too obscure to invest the effort.

Each test should fail when run from the command line:

	./python Lib/test/crashers/weakref_in_del.py

Put as much info into a docstring or comments to help determine the cause of the
failure, as well as a bugs.python.org issue number if it exists.  Particularly
note if the cause is system or environment dependent and what the variables are.

Once the crash is fixed, the test case should be moved into an appropriate test
(even if it was originally from the test suite).  This ensures the regression
doesn't happen again.  And if it does, it should be easier to track down.
