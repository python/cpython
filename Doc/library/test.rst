
:mod:`test` --- Regression tests package for Python
===================================================

.. module:: test
   :synopsis: Regression tests package containing the testing suite for Python.
.. sectionauthor:: Brett Cannon <brett@python.org>

.. note::
    The :mod:`test` package is meant for internal use by Python only. It is
    documented for the benefit of the core developers of Python. Any use of
    this package outside of Python's standard library is discouraged as code
    mentioned here can change or be removed without notice between releases of
    Python.


The :mod:`test` package contains all regression tests for Python as well as the
modules :mod:`test.support` and :mod:`test.regrtest`.
:mod:`test.support` is used to enhance your tests while
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
   from test import support

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
       support.run_unittest(MyTestCase1,
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
  includes not just the external API that is to be presented to the outside
  world but also "private" code.

* Whitebox testing (examining the code being tested when the tests are being
  written) is preferred. Blackbox testing (testing only the published user
  interface) is not complete enough to make sure all boundary and edge cases
  are tested.

* Make sure all possible values are tested including invalid ones. This makes
  sure that not only all valid values are acceptable but also that improper
  values are handled correctly.

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
  as what type of input is used. Minimize code duplication by subclassing a
  basic test class with a class that specifies the input::

     class TestFuncAcceptsSequences(unittest.TestCase):

         func = mySuperWhammyFunction

         def test_func(self):
             self.func(self.arg)

     class AcceptLists(TestFuncAcceptsSequences):
         arg = [1, 2, 3]

     class AcceptStrings(TestFuncAcceptsSequences):
         arg = 'abc'

     class AcceptTuples(TestFuncAcceptsSequences):
         arg = (1, 2, 3)


.. seealso::

   Test Driven Development
      A book by Kent Beck on writing tests before code.


.. _regrtest:

Running tests using the command-line interface
----------------------------------------------

The :mod:`test.regrtest` module can be run as a script to drive Python's regression
test suite, thanks to the :option:`-m` option: :program:`python -m test.regrtest`.
Running the script by itself automatically starts running all regression
tests in the :mod:`test` package. It does this by finding all modules in the
package whose name starts with ``test_``, importing them, and executing the
function :func:`test_main` if present. The names of tests to execute may also
be passed to the script. Specifying a single regression test (:program:`python
-m test.regrtest test_spam`) will minimize output and only print whether
the test passed or failed and thus minimize output.

Running :mod:`test.regrtest` directly allows what resources are available for
tests to use to be set. You do this by using the ``-u`` command-line
option. Specifying ``all`` as the value for the ``-u`` option enables all
possible resources: :program:`python -m test.regrtest -uall`.
If all but one resource is desired (a more common case), a
comma-separated list of resources that are not desired may be listed after
``all``. The command :program:`python -m test.regrtest -uall,-audio,-largefile`
will run :mod:`test.regrtest` with all resources except the ``audio`` and
``largefile`` resources. For a list of all resources and more command-line
options, run :program:`python -m test.regrtest -h`.

Some other ways to execute the regression tests depend on what platform the
tests are being executed on. On Unix, you can run :program:`make test` at the
top-level directory where Python was built. On Windows, executing
:program:`rt.bat` from your :file:`PCBuild` directory will run all regression
tests.

.. versionchanged:: 2.7.14
   The :mod:`test` package can be run as a script: :program:`python -m test`.
   This works the same as running the :mod:`test.regrtest` module.


:mod:`test.support` --- Utility functions for tests
===================================================

.. module:: test.support
   :synopsis: Support for Python regression tests.

.. note::

   The :mod:`test.test_support` module has been renamed to :mod:`test.support`
   in Python 3.x and 2.7.14.  The name ``test.test_support`` has been retained
   as an alias in 2.7.

The :mod:`test.support` module provides support for Python's regression
tests.

This module defines the following exceptions:


.. exception:: TestFailed

   Exception to be raised when a test fails. This is deprecated in favor of
   :mod:`unittest`\ -based tests and :class:`unittest.TestCase`'s assertion
   methods.


.. exception:: ResourceDenied

   Subclass of :exc:`unittest.SkipTest`. Raised when a resource (such as a
   network connection) is not available. Raised by the :func:`requires`
   function.

The :mod:`test.support` module defines the following constants:


.. data:: verbose

   :const:`True` when verbose output is enabled. Should be checked when more
   detailed information is desired about a running test. *verbose* is set by
   :mod:`test.regrtest`.


.. data:: have_unicode

   :const:`True` when Unicode support is available.


.. data:: is_jython

   :const:`True` if the running interpreter is Jython.


.. data:: TESTFN

   Set to a name that is safe to use as the name of a temporary file.  Any
   temporary file that is created should be closed and unlinked (removed).

The :mod:`test.support` module defines the following functions:


.. function:: forget(module_name)

   Remove the module named *module_name* from ``sys.modules`` and delete any
   byte-compiled files of the module.


.. function:: is_resource_enabled(resource)

   Return :const:`True` if *resource* is enabled and available. The list of
   available resources is only set when :mod:`test.regrtest` is executing the
   tests.


.. function:: requires(resource[, msg])

   Raise :exc:`ResourceDenied` if *resource* is not available. *msg* is the
   argument to :exc:`ResourceDenied` if it is raised. Always returns
   :const:`True` if called by a function whose ``__name__`` is ``'__main__'``.
   Used when tests are executed by :mod:`test.regrtest`.


.. function:: findfile(filename)

   Return the path to the file named *filename*. If no match is found
   *filename* is returned. This does not equal a failure since it could be the
   path to the file.


.. function:: run_unittest(*classes)

   Execute :class:`unittest.TestCase` subclasses passed to the function. The
   function scans the classes for methods starting with the prefix ``test_``
   and executes the tests individually.

   It is also legal to pass strings as parameters; these should be keys in
   ``sys.modules``. Each associated module will be scanned by
   ``unittest.TestLoader.loadTestsFromModule()``. This is usually seen in the
   following :func:`test_main` function::

      def test_main():
          support.run_unittest(__name__)

   This will run all tests defined in the named module.


.. function:: check_warnings(*filters, quiet=True)

   A convenience wrapper for :func:`warnings.catch_warnings()` that makes it
   easier to test that a warning was correctly raised.  It is approximately
   equivalent to calling ``warnings.catch_warnings(record=True)`` with
   :meth:`warnings.simplefilter` set to ``always`` and with the option to
   automatically validate the results that are recorded.

   ``check_warnings`` accepts 2-tuples of the form ``("message regexp",
   WarningCategory)`` as positional arguments. If one or more *filters* are
   provided, or if the optional keyword argument *quiet* is :const:`False`,
   it checks to make sure the warnings are as expected:  each specified filter
   must match at least one of the warnings raised by the enclosed code or the
   test fails, and if any warnings are raised that do not match any of the
   specified filters the test fails.  To disable the first of these checks,
   set *quiet* to :const:`True`.

   If no arguments are specified, it defaults to::

      check_warnings(("", Warning), quiet=True)

   In this case all warnings are caught and no errors are raised.

   On entry to the context manager, a :class:`WarningRecorder` instance is
   returned. The underlying warnings list from
   :func:`~warnings.catch_warnings` is available via the recorder object's
   :attr:`warnings` attribute.  As a convenience, the attributes of the object
   representing the most recent warning can also be accessed directly through
   the recorder object (see example below).  If no warning has been raised,
   then any of the attributes that would otherwise be expected on an object
   representing a warning will return :const:`None`.

   The recorder object also has a :meth:`reset` method, which clears the
   warnings list.

   The context manager is designed to be used like this::

      with check_warnings(("assertion is always true", SyntaxWarning),
                          ("", UserWarning)):
          exec('assert(False, "Hey!")')
          warnings.warn(UserWarning("Hide me!"))

   In this case if either warning was not raised, or some other warning was
   raised, :func:`check_warnings` would raise an error.

   When a test needs to look more deeply into the warnings, rather than
   just checking whether or not they occurred, code like this can be used::

      with check_warnings(quiet=True) as w:
          warnings.warn("foo")
          assert str(w.args[0]) == "foo"
          warnings.warn("bar")
          assert str(w.args[0]) == "bar"
          assert str(w.warnings[0].args[0]) == "foo"
          assert str(w.warnings[1].args[0]) == "bar"
          w.reset()
          assert len(w.warnings) == 0

   Here all warnings will be caught, and the test code tests the captured
   warnings directly.

   .. versionadded:: 2.6
   .. versionchanged:: 2.7
      New optional arguments *filters* and *quiet*.


.. function:: check_py3k_warnings(*filters, quiet=False)

   Similar to :func:`check_warnings`, but for Python 3 compatibility warnings.
   If ``sys.py3kwarning == 1``, it checks if the warning is effectively raised.
   If ``sys.py3kwarning == 0``, it checks that no warning is raised.  It
   accepts 2-tuples of the form ``("message regexp", WarningCategory)`` as
   positional arguments.  When the optional keyword argument *quiet* is
   :const:`True`, it does not fail if a filter catches nothing.  Without
   arguments, it defaults to::

      check_py3k_warnings(("", DeprecationWarning), quiet=False)

   .. versionadded:: 2.7


.. function:: captured_stdout()

   This is a context manager that runs the :keyword:`with` statement body using
   a :class:`StringIO.StringIO` object as sys.stdout.  That object can be
   retrieved using the ``as`` clause of the :keyword:`with` statement.

   Example use::

      with captured_stdout() as s:
          print "hello"
      assert s.getvalue() == "hello\n"

   .. versionadded:: 2.6


.. function:: import_module(name, deprecated=False)

   This function imports and returns the named module. Unlike a normal
   import, this function raises :exc:`unittest.SkipTest` if the module
   cannot be imported.

   Module and package deprecation messages are suppressed during this import
   if *deprecated* is :const:`True`.

   .. versionadded:: 2.7


.. function:: import_fresh_module(name, fresh=(), blocked=(), deprecated=False)

   This function imports and returns a fresh copy of the named Python module
   by removing the named module from ``sys.modules`` before doing the import.
   Note that unlike :func:`reload`, the original module is not affected by
   this operation.

   *fresh* is an iterable of additional module names that are also removed
   from the ``sys.modules`` cache before doing the import.

   *blocked* is an iterable of module names that are replaced with :const:`0`
   in the module cache during the import to ensure that attempts to import
   them raise :exc:`ImportError`.

   The named module and any modules named in the *fresh* and *blocked*
   parameters are saved before starting the import and then reinserted into
   ``sys.modules`` when the fresh import is complete.

   Module and package deprecation messages are suppressed during this import
   if *deprecated* is :const:`True`.

   This function will raise :exc:`unittest.SkipTest` if the named module
   cannot be imported.

   Example use::

      # Get copies of the warnings module for testing without
      # affecting the version being used by the rest of the test suite
      # One copy uses the C implementation, the other is forced to use
      # the pure Python fallback implementation
      py_warnings = import_fresh_module('warnings', blocked=['_warnings'])
      c_warnings = import_fresh_module('warnings', fresh=['_warnings'])

   .. versionadded:: 2.7


The :mod:`test.support` module defines the following classes:

.. class:: TransientResource(exc[, **kwargs])

   Instances are a context manager that raises :exc:`ResourceDenied` if the
   specified exception type is raised.  Any keyword arguments are treated as
   attribute/value pairs to be compared against any exception raised within the
   :keyword:`with` statement.  Only if all pairs match properly against
   attributes on the exception is :exc:`ResourceDenied` raised.

   .. versionadded:: 2.6
.. class:: EnvironmentVarGuard()

   Class used to temporarily set or unset environment variables.  Instances can
   be used as a context manager and have a complete dictionary interface for
   querying/modifying the underlying ``os.environ``. After exit from the
   context manager all changes to environment variables done through this
   instance will be rolled back.

   .. versionadded:: 2.6
   .. versionchanged:: 2.7
      Added dictionary interface.


.. method:: EnvironmentVarGuard.set(envvar, value)

   Temporarily set the environment variable ``envvar`` to the value of
   ``value``.


.. method:: EnvironmentVarGuard.unset(envvar)

   Temporarily unset the environment variable ``envvar``.


.. class:: WarningsRecorder()

   Class used to record warnings for unit tests. See documentation of
   :func:`check_warnings` above for more details.

   .. versionadded:: 2.6
