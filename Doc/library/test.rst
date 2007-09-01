
:mod:`test` --- Regression tests package for Python
===================================================

.. module:: test
   :synopsis: Regression tests package containing the testing suite for Python.
.. sectionauthor:: Brett Cannon <brett@python.org>


The :mod:`test` package contains all regression tests for Python as well as the
modules :mod:`test.test_support` and :mod:`test.regrtest`.
:mod:`test.test_support` is used to enhance your tests while
:mod:`test.regrtest` drives the testing suite.

Each module in the :mod:`test` package whose name starts with ``test_`` is a
testing suite for a specific module or feature. All new tests should be written
using the :mod:`unittest` or :mod:`doctest` module.  Some older tests are
written using a "traditional" testing style that compares output printed to
``sys.stdout``; this style of test is considered deprecated.


.. seealso::

   Module :mod:`unittest`
      Writing PyUnit regression tests.

   Module :mod:`doctest`
      Tests embedded in documentation strings.


.. _writing-tests:

Writing Unit Tests for the :mod:`test` package
----------------------------------------------

.. % 

It is preferred that tests that use the :mod:`unittest` module follow a few
guidelines. One is to name the test module by starting it with ``test_`` and end
it with the name of the module being tested. The test methods in the test module
should start with ``test_`` and end with a description of what the method is
testing. This is needed so that the methods are recognized by the test driver as
test methods. Also, no documentation string for the method should be included. A
comment (such as ``# Tests function returns only True or False``) should be used
to provide documentation for test methods. This is done because documentation
strings get printed out if they exist and thus what test is being run is not
stated.

A basic boilerplate is often used::

   import unittest
   from test import test_support

   class MyTestCase1(unittest.TestCase):

       # Only use setUp() and tearDown() if necessary

       def setUp(self):
           ... code to execute in preparation for tests ...

       def tearDown(self):
           ... code to execute to clean up after tests ...

       def test_feature_one(self):
           # Test feature one.
           ... testing code ...

       def test_feature_two(self):
           # Test feature two.
           ... testing code ...

       ... more test methods ...

   class MyTestCase2(unittest.TestCase):
       ... same structure as MyTestCase1 ...

   ... more test classes ...

   def test_main():
       test_support.run_unittest(MyTestCase1,
                                 MyTestCase2,
                                 ... list other tests ...
                                )

   if __name__ == '__main__':
       test_main()

This boilerplate code allows the testing suite to be run by :mod:`test.regrtest`
as well as on its own as a script.

The goal for regression testing is to try to break code. This leads to a few
guidelines to be followed:

* The testing suite should exercise all classes, functions, and constants. This
  includes not just the external API that is to be presented to the outside world
  but also "private" code.

* Whitebox testing (examining the code being tested when the tests are being
  written) is preferred. Blackbox testing (testing only the published user
  interface) is not complete enough to make sure all boundary and edge cases are
  tested.

* Make sure all possible values are tested including invalid ones. This makes
  sure that not only all valid values are acceptable but also that improper values
  are handled correctly.

* Exhaust as many code paths as possible. Test where branching occurs and thus
  tailor input to make sure as many different paths through the code are taken.

* Add an explicit test for any bugs discovered for the tested code. This will
  make sure that the error does not crop up again if the code is changed in the
  future.

* Make sure to clean up after your tests (such as close and remove all temporary
  files).

* If a test is dependent on a specific condition of the operating system then
  verify the condition already exists before attempting the test.

* Import as few modules as possible and do it as soon as possible. This
  minimizes external dependencies of tests and also minimizes possible anomalous
  behavior from side-effects of importing a module.

* Try to maximize code reuse. On occasion, tests will vary by something as small
  as what type of input is used. Minimize code duplication by subclassing a basic
  test class with a class that specifies the input::

     class TestFuncAcceptsSequences(unittest.TestCase):

         func = mySuperWhammyFunction

         def test_func(self):
             self.func(self.arg)

     class AcceptLists(TestFuncAcceptsSequences):
         arg = [1,2,3]

     class AcceptStrings(TestFuncAcceptsSequences):
         arg = 'abc'

     class AcceptTuples(TestFuncAcceptsSequences):
         arg = (1,2,3)


.. seealso::

   Test Driven Development
      A book by Kent Beck on writing tests before code.


.. _regrtest:

Running tests using :mod:`test.regrtest`
----------------------------------------

:mod:`test.regrtest` can be used as a script to drive Python's regression test
suite. Running the script by itself automatically starts running all regression
tests in the :mod:`test` package. It does this by finding all modules in the
package whose name starts with ``test_``, importing them, and executing the
function :func:`test_main` if present. The names of tests to execute may also be
passed to the script. Specifying a single regression test (:program:`python
regrtest.py` :option:`test_spam.py`) will minimize output and only print whether
the test passed or failed and thus minimize output.

Running :mod:`test.regrtest` directly allows what resources are available for
tests to use to be set. You do this by using the :option:`-u` command-line
option. Run :program:`python regrtest.py` :option:`-uall` to turn on all
resources; specifying :option:`all` as an option for :option:`-u` enables all
possible resources. If all but one resource is desired (a more common case), a
comma-separated list of resources that are not desired may be listed after
:option:`all`. The command :program:`python regrtest.py`
:option:`-uall,-audio,-largefile` will run :mod:`test.regrtest` with all
resources except the :option:`audio` and :option:`largefile` resources. For a
list of all resources and more command-line options, run :program:`python
regrtest.py` :option:`-h`.

Some other ways to execute the regression tests depend on what platform the
tests are being executed on. On Unix, you can run :program:`make` :option:`test`
at the top-level directory where Python was built. On Windows, executing
:program:`rt.bat` from your :file:`PCBuild` directory will run all regression
tests.


:mod:`test.test_support` --- Utility functions for tests
========================================================

.. module:: test.test_support
   :synopsis: Support for Python regression tests.


The :mod:`test.test_support` module provides support for Python's regression
tests.

This module defines the following exceptions:


.. exception:: TestFailed

   Exception to be raised when a test fails. This is deprecated in favor of
   :mod:`unittest`\ -based tests and :class:`unittest.TestCase`'s assertion
   methods.


.. exception:: TestSkipped

   Subclass of :exc:`TestFailed`. Raised when a test is skipped. This occurs when a
   needed resource (such as a network connection) is not available at the time of
   testing.


.. exception:: ResourceDenied

   Subclass of :exc:`TestSkipped`. Raised when a resource (such as a network
   connection) is not available. Raised by the :func:`requires` function.

The :mod:`test.test_support` module defines the following constants:


.. data:: verbose

   :const:`True` when verbose output is enabled. Should be checked when more
   detailed information is desired about a running test. *verbose* is set by
   :mod:`test.regrtest`.


.. data:: have_unicode

   :const:`True` when Unicode support is available.


.. data:: is_jython

   :const:`True` if the running interpreter is Jython.


.. data:: TESTFN

   Set to the path that a temporary file may be created at. Any temporary that is
   created should be closed and unlinked (removed).

The :mod:`test.test_support` module defines the following functions:


.. function:: forget(module_name)

   Removes the module named *module_name* from ``sys.modules`` and deletes any
   byte-compiled files of the module.


.. function:: is_resource_enabled(resource)

   Returns :const:`True` if *resource* is enabled and available. The list of
   available resources is only set when :mod:`test.regrtest` is executing the
   tests.


.. function:: requires(resource[, msg])

   Raises :exc:`ResourceDenied` if *resource* is not available. *msg* is the
   argument to :exc:`ResourceDenied` if it is raised. Always returns true if called
   by a function whose ``__name__`` is ``'__main__'``. Used when tests are executed
   by :mod:`test.regrtest`.


.. function:: findfile(filename)

   Return the path to the file named *filename*. If no match is found *filename* is
   returned. This does not equal a failure since it could be the path to the file.


.. function:: run_unittest(*classes)

   Execute :class:`unittest.TestCase` subclasses passed to the function. The
   function scans the classes for methods starting with the prefix ``test_`` and
   executes the tests individually.

   It is also legal to pass strings as parameters; these should be keys in
   ``sys.modules``. Each associated module will be scanned by
   ``unittest.TestLoader.loadTestsFromModule()``. This is usually seen in the
   following :func:`test_main` function::

      def test_main():
          test_support.run_unittest(__name__)

   This will run all tests defined in the named module.


.. function:: catch_warning()

   This is a context manager that guards the warnings filter from being
   permanently changed and records the data of the last warning that has been
   issued.

   Use like this::

      with catch_warning() as w:
          warnings.warn("foo")
          assert str(w.message) == "foo"


.. function:: captured_stdout()

   This is a context manager than runs the :keyword:`with` statement body using
   a :class:`StringIO.StringIO` object as sys.stdout.  That object can be
   retrieved using the ``as`` clause of the with statement.

   Example use::

      with captured_stdout() as s:
          print "hello"
      assert s.getvalue() == "hello"


The :mod:`test.test_support` module defines the following classes:

.. class:: TransientResource(exc[, **kwargs])

   Instances are a context manager that raises :exc:`ResourceDenied` if the
   specified exception type is raised.  Any keyword arguments are treated as
   attribute/value pairs to be compared against any exception raised within the
   :keyword:`with` statement.  Only if all pairs match properly against
   attributes on the exception is :exc:`ResourceDenied` raised.


.. class:: EnvironmentVarGuard()

   Class used to temporarily set or unset environment variables.  Instances can be
   used as a context manager.


.. method:: EnvironmentVarGuard.set(envvar, value)

   Temporarily set the environment variable ``envvar`` to the value of ``value``.


.. method:: EnvironmentVarGuard.unset(envvar)

   Temporarily unset the environment variable ``envvar``.
