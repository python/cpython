:mod:`unittest` --- Unit testing framework
==========================================

.. module:: unittest
   :synopsis: Unit testing framework for Python.
.. moduleauthor:: Steve Purcell <stephen_purcell@yahoo.com>
.. sectionauthor:: Steve Purcell <stephen_purcell@yahoo.com>
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>
.. sectionauthor:: Raymond Hettinger <python@rcn.com>

(If you are already familiar with the basic concepts of testing, you might want
to skip to :ref:`the list of assert methods <assert-methods>`.)

The Python unit testing framework, sometimes referred to as "PyUnit," is a
Python language version of JUnit, by Kent Beck and Erich Gamma. JUnit is, in
turn, a Java version of Kent's Smalltalk testing framework.  Each is the de
facto standard unit testing framework for its respective language.

:mod:`unittest` supports test automation, sharing of setup and shutdown code for
tests, aggregation of tests into collections, and independence of the tests from
the reporting framework.  The :mod:`unittest` module provides classes that make
it easy to support these qualities for a set of tests.

To achieve this, :mod:`unittest` supports some important concepts:

test fixture
   A :dfn:`test fixture` represents the preparation needed to perform one or more
   tests, and any associate cleanup actions.  This may involve, for example,
   creating temporary or proxy databases, directories, or starting a server
   process.

test case
   A :dfn:`test case` is the smallest unit of testing.  It checks for a specific
   response to a particular set of inputs.  :mod:`unittest` provides a base class,
   :class:`TestCase`, which may be used to create new test cases.

test suite
   A :dfn:`test suite` is a collection of test cases, test suites, or both.  It is
   used to aggregate tests that should be executed together.

test runner
   A :dfn:`test runner` is a component which orchestrates the execution of tests
   and provides the outcome to the user.  The runner may use a graphical interface,
   a textual interface, or return a special value to indicate the results of
   executing the tests.

The test case and test fixture concepts are supported through the
:class:`TestCase` and :class:`FunctionTestCase` classes; the former should be
used when creating new tests, and the latter can be used when integrating
existing test code with a :mod:`unittest`\ -driven framework. When building test
fixtures using :class:`TestCase`, the :meth:`~TestCase.setUp` and
:meth:`~TestCase.tearDown` methods can be overridden to provide initialization
and cleanup for the fixture.  With :class:`FunctionTestCase`, existing functions
can be passed to the constructor for these purposes.  When the test is run, the
fixture initialization is run first; if it succeeds, the cleanup method is run
after the test has been executed, regardless of the outcome of the test.  Each
instance of the :class:`TestCase` will only be used to run a single test method,
so a new fixture is created for each test.

Test suites are implemented by the :class:`TestSuite` class.  This class allows
individual tests and test suites to be aggregated; when the suite is executed,
all tests added directly to the suite and in "child" test suites are run.

A test runner is an object that provides a single method,
:meth:`~TestRunner.run`, which accepts a :class:`TestCase` or :class:`TestSuite`
object as a parameter, and returns a result object.  The class
:class:`TestResult` is provided for use as the result object. :mod:`unittest`
provides the :class:`TextTestRunner` as an example test runner which reports
test results on the standard error stream by default.  Alternate runners can be
implemented for other environments (such as graphical environments) without any
need to derive from a specific class.


.. seealso::

   Module :mod:`doctest`
      Another test-support module with a very different flavor.

   `unittest2: A backport of new unittest features for Python 2.4-2.6 <http://pypi.python.org/pypi/unittest2>`_
      Many new features were added to unittest in Python 2.7, including test
      discovery. unittest2 allows you to use these features with earlier
      versions of Python.

   `Simple Smalltalk Testing: With Patterns <http://www.XProgramming.com/testfram.htm>`_
      Kent Beck's original paper on testing frameworks using the pattern shared
      by :mod:`unittest`.

   `Nose <http://code.google.com/p/python-nose/>`_ and `py.test <http://pytest.org>`_
      Third-party unittest frameworks with a lighter-weight syntax for writing
      tests.  For example, ``assert func(10) == 42``.

   `The Python Testing Tools Taxonomy <http://pycheesecake.org/wiki/PythonTestingToolsTaxonomy>`_
      An extensive list of Python testing tools including functional testing
      frameworks and mock object libraries.

   `Testing in Python Mailing List <http://lists.idyll.org/listinfo/testing-in-python>`_
      A special-interest-group for discussion of testing, and testing tools,
      in Python.

.. _unittest-minimal-example:

Basic example
-------------

The :mod:`unittest` module provides a rich set of tools for constructing and
running tests.  This section demonstrates that a small subset of the tools
suffice to meet the needs of most users.

Here is a short script to test three functions from the :mod:`random` module::

   import random
   import unittest

   class TestSequenceFunctions(unittest.TestCase):

       def setUp(self):
           self.seq = list(range(10))

       def test_shuffle(self):
           # make sure the shuffled sequence does not lose any elements
           random.shuffle(self.seq)
           self.seq.sort()
           self.assertEqual(self.seq, list(range(10)))

           # should raise an exception for an immutable sequence
           self.assertRaises(TypeError, random.shuffle, (1,2,3))

       def test_choice(self):
           element = random.choice(self.seq)
           self.assertTrue(element in self.seq)

       def test_sample(self):
           with self.assertRaises(ValueError):
               random.sample(self.seq, 20)
           for element in random.sample(self.seq, 5):
               self.assertTrue(element in self.seq)

   if __name__ == '__main__':
       unittest.main()

A testcase is created by subclassing :class:`unittest.TestCase`.  The three
individual tests are defined with methods whose names start with the letters
``test``.  This naming convention informs the test runner about which methods
represent tests.

The crux of each test is a call to :meth:`~TestCase.assertEqual` to check for an
expected result; :meth:`~TestCase.assertTrue` to verify a condition; or
:meth:`~TestCase.assertRaises` to verify that an expected exception gets raised.
These methods are used instead of the :keyword:`assert` statement so the test
runner can accumulate all test results and produce a report.

When a :meth:`~TestCase.setUp` method is defined, the test runner will run that
method prior to each test.  Likewise, if a :meth:`~TestCase.tearDown` method is
defined, the test runner will invoke that method after each test.  In the
example, :meth:`~TestCase.setUp` was used to create a fresh sequence for each
test.

The final block shows a simple way to run the tests. :func:`unittest.main`
provides a command-line interface to the test script.  When run from the command
line, the above script produces an output that looks like this::

   ...
   ----------------------------------------------------------------------
   Ran 3 tests in 0.000s

   OK

Instead of :func:`unittest.main`, there are other ways to run the tests with a
finer level of control, less terse output, and no requirement to be run from the
command line.  For example, the last two lines may be replaced with::

   suite = unittest.TestLoader().loadTestsFromTestCase(TestSequenceFunctions)
   unittest.TextTestRunner(verbosity=2).run(suite)

Running the revised script from the interpreter or another script produces the
following output::

   test_choice (__main__.TestSequenceFunctions) ... ok
   test_sample (__main__.TestSequenceFunctions) ... ok
   test_shuffle (__main__.TestSequenceFunctions) ... ok

   ----------------------------------------------------------------------
   Ran 3 tests in 0.110s

   OK

The above examples show the most commonly used :mod:`unittest` features which
are sufficient to meet many everyday testing needs.  The remainder of the
documentation explores the full feature set from first principles.


.. _unittest-command-line-interface:

Command-Line Interface
----------------------

The unittest module can be used from the command line to run tests from
modules, classes or even individual test methods::

   python -m unittest test_module1 test_module2
   python -m unittest test_module.TestClass
   python -m unittest test_module.TestClass.test_method

You can pass in a list with any combination of module names, and fully
qualified class or method names.

You can run tests with more detail (higher verbosity) by passing in the -v flag::

   python -m unittest -v test_module

When executed without arguments :ref:`unittest-test-discovery` is started::

   python -m unittest

For a list of all the command-line options::

   python -m unittest -h

.. versionchanged:: 3.2
   In earlier versions it was only possible to run individual test methods and
   not modules or classes.


Command-line options
~~~~~~~~~~~~~~~~~~~~

:program:`unittest` supports these command-line options:

.. program:: unittest

.. cmdoption:: -b, --buffer

   The standard output and standard error streams are buffered during the test
   run. Output during a passing test is discarded. Output is echoed normally
   on test fail or error and is added to the failure messages.

.. cmdoption:: -c, --catch

   Control-C during the test run waits for the current test to end and then
   reports all the results so far. A second control-C raises the normal
   :exc:`KeyboardInterrupt` exception.

   See `Signal Handling`_ for the functions that provide this functionality.

.. cmdoption:: -f, --failfast

   Stop the test run on the first error or failure.

.. versionadded:: 3.2
   The command-line options :option:`-b`, :option:`-c` and :option:`-f`
   were added.

The command line can also be used for test discovery, for running all of the
tests in a project or just a subset.


.. _unittest-test-discovery:

Test Discovery
--------------

.. versionadded:: 3.2

Unittest supports simple test discovery. For a project's tests to be
compatible with test discovery they must all be importable from the top level
directory of the project (in other words, they must all be in Python packages).

Test discovery is implemented in :meth:`TestLoader.discover`, but can also be
used from the command line. The basic command-line usage is::

   cd project_directory
   python -m unittest discover

.. note::

   As a shortcut, ``python -m unittest`` is the equivalent of
   ``python -m unittest discover``. If you want to pass arguments to test
   discovery the `discover` sub-command must be used explicitly.

The ``discover`` sub-command has the following options:

.. program:: unittest discover

.. cmdoption:: -v, --verbose

   Verbose output

.. cmdoption:: -s directory

   Directory to start discovery ('.' default)

.. cmdoption:: -p pattern

   Pattern to match test files ('test*.py' default)

.. cmdoption:: -t directory

   Top level directory of project (defaults to start directory)

The :option:`-s`, :option:`-p`, and :option:`-t` options can be passed in
as positional arguments in that order. The following two command lines
are equivalent::

   python -m unittest discover -s project_directory -p '*_test.py'
   python -m unittest discover project_directory '*_test.py'

As well as being a path it is possible to pass a package name, for example
``myproject.subpackage.test``, as the start directory. The package name you
supply will then be imported and its location on the filesystem will be used
as the start directory.

.. caution::

    Test discovery loads tests by importing them. Once test discovery has found
    all the test files from the start directory you specify it turns the paths
    into package names to import. For example :file:`foo/bar/baz.py` will be
    imported as ``foo.bar.baz``.

    If you have a package installed globally and attempt test discovery on
    a different copy of the package then the import *could* happen from the
    wrong place. If this happens test discovery will warn you and exit.

    If you supply the start directory as a package name rather than a
    path to a directory then discover assumes that whichever location it
    imports from is the location you intended, so you will not get the
    warning.

Test modules and packages can customize test loading and discovery by through
the `load_tests protocol`_.


.. _organizing-tests:

Organizing test code
--------------------

The basic building blocks of unit testing are :dfn:`test cases` --- single
scenarios that must be set up and checked for correctness.  In :mod:`unittest`,
test cases are represented by instances of :mod:`unittest`'s :class:`TestCase`
class. To make your own test cases you must write subclasses of
:class:`TestCase`, or use :class:`FunctionTestCase`.

An instance of a :class:`TestCase`\ -derived class is an object that can
completely run a single test method, together with optional set-up and tidy-up
code.

The testing code of a :class:`TestCase` instance should be entirely self
contained, such that it can be run either in isolation or in arbitrary
combination with any number of other test cases.

The simplest :class:`TestCase` subclass will simply override the
:meth:`~TestCase.runTest` method in order to perform specific testing code::

   import unittest

   class DefaultWidgetSizeTestCase(unittest.TestCase):
       def runTest(self):
           widget = Widget('The widget')
           self.assertEqual(widget.size(), (50, 50), 'incorrect default size')

Note that in order to test something, we use the one of the :meth:`assert\*`
methods provided by the :class:`TestCase` base class.  If the test fails, an
exception will be raised, and :mod:`unittest` will identify the test case as a
:dfn:`failure`.  Any other exceptions will be treated as :dfn:`errors`. This
helps you identify where the problem is: :dfn:`failures` are caused by incorrect
results - a 5 where you expected a 6. :dfn:`Errors` are caused by incorrect
code - e.g., a :exc:`TypeError` caused by an incorrect function call.

The way to run a test case will be described later.  For now, note that to
construct an instance of such a test case, we call its constructor without
arguments::

   testCase = DefaultWidgetSizeTestCase()

Now, such test cases can be numerous, and their set-up can be repetitive.  In
the above case, constructing a :class:`Widget` in each of 100 Widget test case
subclasses would mean unsightly duplication.

Luckily, we can factor out such set-up code by implementing a method called
:meth:`~TestCase.setUp`, which the testing framework will automatically call for
us when we run the test::

   import unittest

   class SimpleWidgetTestCase(unittest.TestCase):
       def setUp(self):
           self.widget = Widget('The widget')

   class DefaultWidgetSizeTestCase(SimpleWidgetTestCase):
       def runTest(self):
           self.assertEqual(self.widget.size(), (50,50),
                            'incorrect default size')

   class WidgetResizeTestCase(SimpleWidgetTestCase):
       def runTest(self):
           self.widget.resize(100,150)
           self.assertEqual(self.widget.size(), (100,150),
                            'wrong size after resize')

If the :meth:`~TestCase.setUp` method raises an exception while the test is
running, the framework will consider the test to have suffered an error, and the
:meth:`~TestCase.runTest` method will not be executed.

Similarly, we can provide a :meth:`~TestCase.tearDown` method that tidies up
after the :meth:`~TestCase.runTest` method has been run::

   import unittest

   class SimpleWidgetTestCase(unittest.TestCase):
       def setUp(self):
           self.widget = Widget('The widget')

       def tearDown(self):
           self.widget.dispose()
           self.widget = None

If :meth:`~TestCase.setUp` succeeded, the :meth:`~TestCase.tearDown` method will
be run whether :meth:`~TestCase.runTest` succeeded or not.

Such a working environment for the testing code is called a :dfn:`fixture`.

Often, many small test cases will use the same fixture.  In this case, we would
end up subclassing :class:`SimpleWidgetTestCase` into many small one-method
classes such as :class:`DefaultWidgetSizeTestCase`.  This is time-consuming and
discouraging, so in the same vein as JUnit, :mod:`unittest` provides a simpler
mechanism::

   import unittest

   class WidgetTestCase(unittest.TestCase):
       def setUp(self):
           self.widget = Widget('The widget')

       def tearDown(self):
           self.widget.dispose()
           self.widget = None

       def test_default_size(self):
           self.assertEqual(self.widget.size(), (50,50),
                            'incorrect default size')

       def test_resize(self):
           self.widget.resize(100,150)
           self.assertEqual(self.widget.size(), (100,150),
                            'wrong size after resize')

Here we have not provided a :meth:`~TestCase.runTest` method, but have instead
provided two different test methods.  Class instances will now each run one of
the :meth:`test_\*` methods, with ``self.widget`` created and destroyed
separately for each instance.  When creating an instance we must specify the
test method it is to run.  We do this by passing the method name in the
constructor::

   defaultSizeTestCase = WidgetTestCase('test_default_size')
   resizeTestCase = WidgetTestCase('test_resize')

Test case instances are grouped together according to the features they test.
:mod:`unittest` provides a mechanism for this: the :dfn:`test suite`,
represented by :mod:`unittest`'s :class:`TestSuite` class::

   widgetTestSuite = unittest.TestSuite()
   widgetTestSuite.addTest(WidgetTestCase('test_default_size'))
   widgetTestSuite.addTest(WidgetTestCase('test_resize'))

For the ease of running tests, as we will see later, it is a good idea to
provide in each test module a callable object that returns a pre-built test
suite::

   def suite():
       suite = unittest.TestSuite()
       suite.addTest(WidgetTestCase('test_default_size'))
       suite.addTest(WidgetTestCase('test_resize'))
       return suite

or even::

   def suite():
       tests = ['test_default_size', 'test_resize']

       return unittest.TestSuite(map(WidgetTestCase, tests))

Since it is a common pattern to create a :class:`TestCase` subclass with many
similarly named test functions, :mod:`unittest` provides a :class:`TestLoader`
class that can be used to automate the process of creating a test suite and
populating it with individual tests. For example, ::

   suite = unittest.TestLoader().loadTestsFromTestCase(WidgetTestCase)

will create a test suite that will run ``WidgetTestCase.test_default_size()`` and
``WidgetTestCase.test_resize``. :class:`TestLoader` uses the ``'test'`` method
name prefix to identify test methods automatically.

Note that the order in which the various test cases will be run is
determined by sorting the test function names with respect to the
built-in ordering for strings.

Often it is desirable to group suites of test cases together, so as to run tests
for the whole system at once.  This is easy, since :class:`TestSuite` instances
can be added to a :class:`TestSuite` just as :class:`TestCase` instances can be
added to a :class:`TestSuite`::

   suite1 = module1.TheTestSuite()
   suite2 = module2.TheTestSuite()
   alltests = unittest.TestSuite([suite1, suite2])

You can place the definitions of test cases and test suites in the same modules
as the code they are to test (such as :file:`widget.py`), but there are several
advantages to placing the test code in a separate module, such as
:file:`test_widget.py`:

* The test module can be run standalone from the command line.

* The test code can more easily be separated from shipped code.

* There is less temptation to change test code to fit the code it tests without
  a good reason.

* Test code should be modified much less frequently than the code it tests.

* Tested code can be refactored more easily.

* Tests for modules written in C must be in separate modules anyway, so why not
  be consistent?

* If the testing strategy changes, there is no need to change the source code.


.. _legacy-unit-tests:

Re-using old test code
----------------------

Some users will find that they have existing test code that they would like to
run from :mod:`unittest`, without converting every old test function to a
:class:`TestCase` subclass.

For this reason, :mod:`unittest` provides a :class:`FunctionTestCase` class.
This subclass of :class:`TestCase` can be used to wrap an existing test
function.  Set-up and tear-down functions can also be provided.

Given the following test function::

   def testSomething():
       something = makeSomething()
       assert something.name is not None
       # ...

one can create an equivalent test case instance as follows::

   testcase = unittest.FunctionTestCase(testSomething)

If there are additional set-up and tear-down methods that should be called as
part of the test case's operation, they can also be provided like so::

   testcase = unittest.FunctionTestCase(testSomething,
                                        setUp=makeSomethingDB,
                                        tearDown=deleteSomethingDB)

To make migrating existing test suites easier, :mod:`unittest` supports tests
raising :exc:`AssertionError` to indicate test failure. However, it is
recommended that you use the explicit :meth:`TestCase.fail\*` and
:meth:`TestCase.assert\*` methods instead, as future versions of :mod:`unittest`
may treat :exc:`AssertionError` differently.

.. note::

   Even though :class:`FunctionTestCase` can be used to quickly convert an
   existing test base over to a :mod:`unittest`\ -based system, this approach is
   not recommended.  Taking the time to set up proper :class:`TestCase`
   subclasses will make future test refactorings infinitely easier.

In some cases, the existing tests may have been written using the :mod:`doctest`
module.  If so, :mod:`doctest` provides a :class:`DocTestSuite` class that can
automatically build :class:`unittest.TestSuite` instances from the existing
:mod:`doctest`\ -based tests.


.. _unittest-skipping:

Skipping tests and expected failures
------------------------------------

.. versionadded:: 3.1

Unittest supports skipping individual test methods and even whole classes of
tests.  In addition, it supports marking a test as a "expected failure," a test
that is broken and will fail, but shouldn't be counted as a failure on a
:class:`TestResult`.

Skipping a test is simply a matter of using the :func:`skip` :term:`decorator`
or one of its conditional variants.

Basic skipping looks like this: ::

   class MyTestCase(unittest.TestCase):

       @unittest.skip("demonstrating skipping")
       def test_nothing(self):
           self.fail("shouldn't happen")

       @unittest.skipIf(mylib.__version__ < (1, 3),
                        "not supported in this library version")
       def test_format(self):
           # Tests that work for only a certain version of the library.
           pass

       @unittest.skipUnless(sys.platform.startswith("win"), "requires Windows")
       def test_windows_support(self):
           # windows specific testing code
           pass

This is the output of running the example above in verbose mode: ::

   test_format (__main__.MyTestCase) ... skipped 'not supported in this library version'
   test_nothing (__main__.MyTestCase) ... skipped 'demonstrating skipping'
   test_windows_support (__main__.MyTestCase) ... skipped 'requires Windows'

   ----------------------------------------------------------------------
   Ran 3 tests in 0.005s

   OK (skipped=3)

Classes can be skipped just like methods: ::

   @skip("showing class skipping")
   class MySkippedTestCase(unittest.TestCase):
       def test_not_run(self):
           pass

:meth:`TestCase.setUp` can also skip the test.  This is useful when a resource
that needs to be set up is not available.

Expected failures use the :func:`expectedFailure` decorator. ::

   class ExpectedFailureTestCase(unittest.TestCase):
       @unittest.expectedFailure
       def test_fail(self):
           self.assertEqual(1, 0, "broken")

It's easy to roll your own skipping decorators by making a decorator that calls
:func:`skip` on the test when it wants it to be skipped.  This decorator skips
the test unless the passed object has a certain attribute: ::

   def skipUnlessHasattr(obj, attr):
       if hasattr(obj, attr):
           return lambda func: func
       return unittest.skip("{0!r} doesn't have {1!r}".format(obj, attr))

The following decorators implement test skipping and expected failures:

.. decorator:: skip(reason)

   Unconditionally skip the decorated test.  *reason* should describe why the
   test is being skipped.

.. decorator:: skipIf(condition, reason)

   Skip the decorated test if *condition* is true.

.. decorator:: skipUnless(condition, reason)

   Skip the decorated test unless *condition* is true.

.. decorator:: expectedFailure

   Mark the test as an expected failure.  If the test fails when run, the test
   is not counted as a failure.

Skipped tests will not have :meth:`setUp` or :meth:`tearDown` run around them.
Skipped classes will not have :meth:`setUpClass` or :meth:`tearDownClass` run.


.. _unittest-contents:

Classes and functions
---------------------

This section describes in depth the API of :mod:`unittest`.


.. _testcase-objects:

Test cases
~~~~~~~~~~

.. class:: TestCase(methodName='runTest')

   Instances of the :class:`TestCase` class represent the smallest testable units
   in the :mod:`unittest` universe.  This class is intended to be used as a base
   class, with specific tests being implemented by concrete subclasses.  This class
   implements the interface needed by the test runner to allow it to drive the
   test, and methods that the test code can use to check for and report various
   kinds of failure.

   Each instance of :class:`TestCase` will run a single test method: the method
   named *methodName*.  If you remember, we had an earlier example that went
   something like this::

      def suite():
          suite = unittest.TestSuite()
          suite.addTest(WidgetTestCase('test_default_size'))
          suite.addTest(WidgetTestCase('test_resize'))
          return suite

   Here, we create two instances of :class:`WidgetTestCase`, each of which runs a
   single test.

   *methodName* defaults to :meth:`runTest`.

   :class:`TestCase` instances provide three groups of methods: one group used
   to run the test, another used by the test implementation to check conditions
   and report failures, and some inquiry methods allowing information about the
   test itself to be gathered.

   Methods in the first group (running the test) are:


   .. method:: setUp()

      Method called to prepare the test fixture.  This is called immediately
      before calling the test method; any exception raised by this method will
      be considered an error rather than a test failure. The default
      implementation does nothing.


   .. method:: tearDown()

      Method called immediately after the test method has been called and the
      result recorded.  This is called even if the test method raised an
      exception, so the implementation in subclasses may need to be particularly
      careful about checking internal state.  Any exception raised by this
      method will be considered an error rather than a test failure.  This
      method will only be called if the :meth:`setUp` succeeds, regardless of
      the outcome of the test method. The default implementation does nothing.


   .. method:: setUpClass()

      A class method called before tests in an individual class run.
      ``setUpClass`` is called with the class as the only argument
      and must be decorated as a :func:`classmethod`::

        @classmethod
        def setUpClass(cls):
            ...

      See `Class and Module Fixtures`_ for more details.

      .. versionadded:: 3.2


   .. method:: tearDownClass()

      A class method called after tests in an individual class have run.
      ``tearDownClass`` is called with the class as the only argument
      and must be decorated as a :meth:`classmethod`::

        @classmethod
        def tearDownClass(cls):
            ...

      See `Class and Module Fixtures`_ for more details.

      .. versionadded:: 3.2


   .. method:: run(result=None)

      Run the test, collecting the result into the test result object passed as
      *result*.  If *result* is omitted or ``None``, a temporary result
      object is created (by calling the :meth:`defaultTestResult` method) and
      used. The result object is not returned to :meth:`run`'s caller.

      The same effect may be had by simply calling the :class:`TestCase`
      instance.


   .. method:: skipTest(reason)

      Calling this during a test method or :meth:`setUp` skips the current
      test.  See :ref:`unittest-skipping` for more information.

      .. versionadded:: 3.1


   .. method:: debug()

      Run the test without collecting the result.  This allows exceptions raised
      by the test to be propagated to the caller, and can be used to support
      running tests under a debugger.

   .. _assert-methods:

   The :class:`TestCase` class provides a number of methods to check for and
   report failures, such as:

   +-----------------------------------------+-----------------------------+---------------+
   | Method                                  | Checks that                 | New in        |
   +=========================================+=============================+===============+
   | :meth:`assertEqual(a, b)                | ``a == b``                  |               |
   | <TestCase.assertEqual>`                 |                             |               |
   +-----------------------------------------+-----------------------------+---------------+
   | :meth:`assertNotEqual(a, b)             | ``a != b``                  |               |
   | <TestCase.assertNotEqual>`              |                             |               |
   +-----------------------------------------+-----------------------------+---------------+
   | :meth:`assertTrue(x)                    | ``bool(x) is True``         |               |
   | <TestCase.assertTrue>`                  |                             |               |
   +-----------------------------------------+-----------------------------+---------------+
   | :meth:`assertFalse(x)                   | ``bool(x) is False``        |               |
   | <TestCase.assertFalse>`                 |                             |               |
   +-----------------------------------------+-----------------------------+---------------+
   | :meth:`assertIs(a, b)                   | ``a is b``                  | 3.1           |
   | <TestCase.assertIs>`                    |                             |               |
   +-----------------------------------------+-----------------------------+---------------+
   | :meth:`assertIsNot(a, b)                | ``a is not b``              | 3.1           |
   | <TestCase.assertIsNot>`                 |                             |               |
   +-----------------------------------------+-----------------------------+---------------+
   | :meth:`assertIsNone(x)                  | ``x is None``               | 3.1           |
   | <TestCase.assertIsNone>`                |                             |               |
   +-----------------------------------------+-----------------------------+---------------+
   | :meth:`assertIsNotNone(x)               | ``x is not None``           | 3.1           |
   | <TestCase.assertIsNotNone>`             |                             |               |
   +-----------------------------------------+-----------------------------+---------------+
   | :meth:`assertIn(a, b)                   | ``a in b``                  | 3.1           |
   | <TestCase.assertIn>`                    |                             |               |
   +-----------------------------------------+-----------------------------+---------------+
   | :meth:`assertNotIn(a, b)                | ``a not in b``              | 3.1           |
   | <TestCase.assertNotIn>`                 |                             |               |
   +-----------------------------------------+-----------------------------+---------------+
   | :meth:`assertIsInstance(a, b)           | ``isinstance(a, b)``        | 3.2           |
   | <TestCase.assertIsInstance>`            |                             |               |
   +-----------------------------------------+-----------------------------+---------------+
   | :meth:`assertNotIsInstance(a, b)        | ``not isinstance(a, b)``    | 3.2           |
   | <TestCase.assertNotIsInstance>`         |                             |               |
   +-----------------------------------------+-----------------------------+---------------+

   All the assert methods (except :meth:`assertRaises`,
   :meth:`assertRaisesRegex`, :meth:`assertWarns`, :meth:`assertWarnsRegex`)
   accept a *msg* argument that, if specified, is used as the error message on
   failure (see also :data:`longMessage`).

   .. method:: assertEqual(first, second, msg=None)

      Test that *first* and *second* are equal.  If the values do not compare
      equal, the test will fail.

      In addition, if *first* and *second* are the exact same type and one of
      list, tuple, dict, set, frozenset or str or any type that a subclass
      registers with :meth:`addTypeEqualityFunc` the type specific equality
      function will be called in order to generate a more useful default
      error message (see also the :ref:`list of type-specific methods
      <type-specific-methods>`).

      .. versionchanged:: 3.1
         Added the automatic calling of type specific equality function.

      .. versionchanged:: 3.2
         :meth:`assertMultiLineEqual` added as the default type equality
         function for comparing strings.


   .. method:: assertNotEqual(first, second, msg=None)

      Test that *first* and *second* are not equal.  If the values do compare
      equal, the test will fail.

   .. method:: assertTrue(expr, msg=None)
               assertFalse(expr, msg=None)

      Test that *expr* is true (or false).

      Note that this is equivalent to ``bool(expr) is True`` and not to ``expr
      is True`` (use ``assertIs(expr, True)`` for the latter).  This method
      should also be avoided when more specific methods are available (e.g.
      ``assertEqual(a, b)`` instead of ``assertTrue(a == b)``), because they
      provide a better error message in case of failure.


   .. method:: assertIs(first, second, msg=None)
               assertIsNot(first, second, msg=None)

      Test that *first* and *second* evaluate (or don't evaluate) to the same object.

      .. versionadded:: 3.1


   .. method:: assertIsNone(expr, msg=None)
               assertIsNotNone(expr, msg=None)

      Test that *expr* is (or is not) None.

      .. versionadded:: 3.1


   .. method:: assertIn(first, second, msg=None)
               assertNotIn(first, second, msg=None)

      Test that *first* is (or is not) in *second*.

      .. versionadded:: 3.1


   .. method:: assertIsInstance(obj, cls, msg=None)
               assertNotIsInstance(obj, cls, msg=None)

      Test that *obj* is (or is not) an instance of *cls* (which can be a
      class or a tuple of classes, as supported by :func:`isinstance`).

      .. versionadded:: 3.2



   It is also possible to check that exceptions and warnings are raised using
   the following methods:

   +---------------------------------------------------------+--------------------------------------+------------+
   | Method                                                  | Checks that                          | New in     |
   +=========================================================+======================================+============+
   | :meth:`assertRaises(exc, fun, *args, **kwds)            | ``fun(*args, **kwds)`` raises `exc`  |            |
   | <TestCase.assertRaises>`                                |                                      |            |
   +---------------------------------------------------------+--------------------------------------+------------+
   | :meth:`assertRaisesRegex(exc, re, fun, *args, **kwds)   | ``fun(*args, **kwds)`` raises `exc`  | 3.1        |
   | <TestCase.assertRaisesRegex>`                           | and the message matches `re`         |            |
   +---------------------------------------------------------+--------------------------------------+------------+
   | :meth:`assertWarns(warn, fun, *args, **kwds)            | ``fun(*args, **kwds)`` raises `warn` | 3.2        |
   | <TestCase.assertWarns>`                                 |                                      |            |
   +---------------------------------------------------------+--------------------------------------+------------+
   | :meth:`assertWarnsRegex(warn, re, fun, *args, **kwds)   | ``fun(*args, **kwds)`` raises `warn` | 3.2        |
   | <TestCase.assertWarnsRegex>`                            | and the message matches `re`         |            |
   +---------------------------------------------------------+--------------------------------------+------------+

   .. method:: assertRaises(exception, callable, *args, **kwds)
               assertRaises(exception)

      Test that an exception is raised when *callable* is called with any
      positional or keyword arguments that are also passed to
      :meth:`assertRaises`.  The test passes if *exception* is raised, is an
      error if another exception is raised, or fails if no exception is raised.
      To catch any of a group of exceptions, a tuple containing the exception
      classes may be passed as *exception*.

      If only the *exception* argument is given, returns a context manager so
      that the code under test can be written inline rather than as a function::

         with self.assertRaises(SomeException):
             do_something()

      The context manager will store the caught exception object in its
      :attr:`exception` attribute.  This can be useful if the intention
      is to perform additional checks on the exception raised::

         with self.assertRaises(SomeException) as cm:
             do_something()

         the_exception = cm.exception
         self.assertEqual(the_exception.error_code, 3)

      .. versionchanged:: 3.1
         Added the ability to use :meth:`assertRaises` as a context manager.

      .. versionchanged:: 3.2
         Added the :attr:`exception` attribute.


   .. method:: assertRaisesRegex(exception, regex, callable, *args, **kwds)
               assertRaisesRegex(exception, regex)

      Like :meth:`assertRaises` but also tests that *regex* matches
      on the string representation of the raised exception.  *regex* may be
      a regular expression object or a string containing a regular expression
      suitable for use by :func:`re.search`.  Examples::

         self.assertRaisesRegex(ValueError, 'invalid literal for.*XYZ$',
                                int, 'XYZ')

      or::

         with self.assertRaisesRegex(ValueError, 'literal'):
            int('XYZ')

      .. versionadded:: 3.1
         under the name ``assertRaisesRegexp``.
      .. versionchanged:: 3.2
         Renamed to :meth:`assertRaisesRegex`.


   .. method:: assertWarns(warning, callable, *args, **kwds)
               assertWarns(warning)

      Test that a warning is triggered when *callable* is called with any
      positional or keyword arguments that are also passed to
      :meth:`assertWarns`.  The test passes if *warning* is triggered and
      fails if it isn't.  Also, any unexpected exception is an error.
      To catch any of a group of warnings, a tuple containing the warning
      classes may be passed as *warnings*.

      If only the *warning* argument is given, returns a context manager so
      that the code under test can be written inline rather than as a function::

         with self.assertWarns(SomeWarning):
             do_something()

      The context manager will store the caught warning object in its
      :attr:`warning` attribute, and the source line which triggered the
      warnings in the :attr:`filename` and :attr:`lineno` attributes.
      This can be useful if the intention is to perform additional checks
      on the exception raised::

         with self.assertWarns(SomeWarning) as cm:
             do_something()

         self.assertIn('myfile.py', cm.filename)
         self.assertEqual(320, cm.lineno)

      This method works regardless of the warning filters in place when it
      is called.

      .. versionadded:: 3.2


   .. method:: assertWarnsRegex(warning, regex, callable, *args, **kwds)
               assertWarnsRegex(warning, regex)

      Like :meth:`assertWarns` but also tests that *regex* matches on the
      message of the triggered warning.  *regex* may be a regular expression
      object or a string containing a regular expression suitable for use
      by :func:`re.search`.  Example::

         self.assertWarnsRegex(DeprecationWarning,
                               r'legacy_function\(\) is deprecated',
                               legacy_function, 'XYZ')

      or::

         with self.assertWarnsRegex(RuntimeWarning, 'unsafe frobnicating'):
             frobnicate('/etc/passwd')

      .. versionadded:: 3.2



   There are also other methods used to perform more specific checks, such as:

   +---------------------------------------+--------------------------------+--------------+
   | Method                                | Checks that                    | New in       |
   +=======================================+================================+==============+
   | :meth:`assertAlmostEqual(a, b)        | ``round(a-b, 7) == 0``         |              |
   | <TestCase.assertAlmostEqual>`         |                                |              |
   +---------------------------------------+--------------------------------+--------------+
   | :meth:`assertNotAlmostEqual(a, b)     | ``round(a-b, 7) != 0``         |              |
   | <TestCase.assertNotAlmostEqual>`      |                                |              |
   +---------------------------------------+--------------------------------+--------------+
   | :meth:`assertGreater(a, b)            | ``a > b``                      | 3.1          |
   | <TestCase.assertGreater>`             |                                |              |
   +---------------------------------------+--------------------------------+--------------+
   | :meth:`assertGreaterEqual(a, b)       | ``a >= b``                     | 3.1          |
   | <TestCase.assertGreaterEqual>`        |                                |              |
   +---------------------------------------+--------------------------------+--------------+
   | :meth:`assertLess(a, b)               | ``a < b``                      | 3.1          |
   | <TestCase.assertLess>`                |                                |              |
   +---------------------------------------+--------------------------------+--------------+
   | :meth:`assertLessEqual(a, b)          | ``a <= b``                     | 3.1          |
   | <TestCase.assertLessEqual>`           |                                |              |
   +---------------------------------------+--------------------------------+--------------+
   | :meth:`assertRegex(s, re)             | ``regex.search(s)``            | 3.1          |
   | <TestCase.assertRegex>`               |                                |              |
   +---------------------------------------+--------------------------------+--------------+
   | :meth:`assertNotRegex(s, re)          | ``not regex.search(s)``        | 3.2          |
   | <TestCase.assertNotRegex>`            |                                |              |
   +---------------------------------------+--------------------------------+--------------+
   | :meth:`assertDictContainsSubset(a, b) | all the key/value pairs        | 3.1          |
   | <TestCase.assertDictContainsSubset>`  | in `a` exist in `b`            |              |
   +---------------------------------------+--------------------------------+--------------+
   | :meth:`assertCountEqual(a, b)         | `a` and `b` have the same      | 3.2          |
   | <TestCase.assertCountEqual>`          | elements in the same number,   |              |
   |                                       | regardless of their order      |              |
   +---------------------------------------+--------------------------------+--------------+


   .. method:: assertAlmostEqual(first, second, places=7, msg=None, delta=None)
               assertNotAlmostEqual(first, second, places=7, msg=None, delta=None)

      Test that *first* and *second* are approximately (or not approximately)
      equal by computing the difference, rounding to the given number of
      decimal *places* (default 7), and comparing to zero.  Note that these
      methods round the values to the given number of *decimal places* (i.e.
      like the :func:`round` function) and not *significant digits*.

      If *delta* is supplied instead of *places* then the difference
      between *first* and *second* must be less (or more) than *delta*.

      Supplying both *delta* and *places* raises a ``TypeError``.

      .. versionchanged:: 3.2
         :meth:`assertAlmostEqual` automatically considers almost equal objects
         that compare equal.  :meth:`assertNotAlmostEqual` automatically fails
         if the objects compare equal.  Added the *delta* keyword argument.


   .. method:: assertGreater(first, second, msg=None)
               assertGreaterEqual(first, second, msg=None)
               assertLess(first, second, msg=None)
               assertLessEqual(first, second, msg=None)

      Test that *first* is respectively >, >=, < or <= than *second* depending
      on the method name.  If not, the test will fail::

         >>> self.assertGreaterEqual(3, 4)
         AssertionError: "3" unexpectedly not greater than or equal to "4"

      .. versionadded:: 3.1


   .. method:: assertRegex(text, regex, msg=None)
               assertNotRegex(text, regex, msg=None)

      Test that a *regex* search matches (or does not match) *text*.  In case
      of failure, the error message will include the pattern and the *text* (or
      the pattern and the part of *text* that unexpectedly matched).  *regex*
      may be a regular expression object or a string containing a regular
      expression suitable for use by :func:`re.search`.

      .. versionadded:: 3.1
         under the name ``assertRegexpMatches``.
      .. versionchanged:: 3.2
         The method ``assertRegexpMatches()`` has been renamed to
         :meth:`.assertRegex`.
      .. versionadded:: 3.2
         :meth:`.assertNotRegex`.


   .. method:: assertDictContainsSubset(expected, actual, msg=None)

      Tests whether the key/value pairs in dictionary *actual* are a
      superset of those in *expected*.  If not, an error message listing
      the missing keys and mismatched values is generated.

      .. versionadded:: 3.1


   .. method:: assertCountEqual(expected, actual, msg=None)

      Test that sequence *expected* contains the same elements as *actual*,
      regardless of their order. When they don't, an error message listing the
      differences between the sequences will be generated.

      Duplicate elements are *not* ignored when comparing *actual* and
      *expected*. It verifies if each element has the same count in both
      sequences. Equivalent to:
      ``assertEqual(Counter(iter(expected)), Counter(iter(actual)))``
      but works with sequences of unhashable objects as well.

      .. versionadded:: 3.2

   .. method:: assertSameElements(actual, expected, msg=None)

      Test that sequence *expected* contains the same elements as *actual*,
      regardless of their order. When they don't, an error message listing
      the differences between the sequences will be generated.

      Duplicate elements are ignored when comparing *actual* and *expected*.
      It is the equivalent of ``assertEqual(set(expected), set(actual))``
      but it works with sequences of unhashable objects as well. Because
      duplicates are ignored, this method has been deprecated in favour of
      :meth:`assertCountEqual`.

      .. versionadded:: 3.1
      .. deprecated:: 3.2


   .. _type-specific-methods:

   The :meth:`assertEqual` method dispatches the equality check for objects of
   the same type to different type-specific methods.  These methods are already
   implemented for most of the built-in types, but it's also possible to
   register new methods using :meth:`addTypeEqualityFunc`:

   .. method:: addTypeEqualityFunc(typeobj, function)

      Registers a type-specific method called by :meth:`assertEqual` to check
      if two objects of exactly the same *typeobj* (not subclasses) compare
      equal.  *function* must take two positional arguments and a third msg=None
      keyword argument just as :meth:`assertEqual` does.  It must raise
      :data:`self.failureException(msg) <failureException>` when inequality
      between the first two parameters is detected -- possibly providing useful
      information and explaining the inequalities in details in the error
      message.

      .. versionadded:: 3.1

   The list of type-specific methods automatically used by
   :meth:`~TestCase.assertEqual` are summarized in the following table.  Note
   that it's usually not necessary to invoke these methods directly.

   +-----------------------------------------+-----------------------------+--------------+
   | Method                                  | Used to compare             | New in       |
   +=========================================+=============================+==============+
   | :meth:`assertMultiLineEqual(a, b)       | strings                     | 3.1          |
   | <TestCase.assertMultiLineEqual>`        |                             |              |
   +-----------------------------------------+-----------------------------+--------------+
   | :meth:`assertSequenceEqual(a, b)        | sequences                   | 3.1          |
   | <TestCase.assertSequenceEqual>`         |                             |              |
   +-----------------------------------------+-----------------------------+--------------+
   | :meth:`assertListEqual(a, b)            | lists                       | 3.1          |
   | <TestCase.assertListEqual>`             |                             |              |
   +-----------------------------------------+-----------------------------+--------------+
   | :meth:`assertTupleEqual(a, b)           | tuples                      | 3.1          |
   | <TestCase.assertTupleEqual>`            |                             |              |
   +-----------------------------------------+-----------------------------+--------------+
   | :meth:`assertSetEqual(a, b)             | sets or frozensets          | 3.1          |
   | <TestCase.assertSetEqual>`              |                             |              |
   +-----------------------------------------+-----------------------------+--------------+
   | :meth:`assertDictEqual(a, b)            | dicts                       | 3.1          |
   | <TestCase.assertDictEqual>`             |                             |              |
   +-----------------------------------------+-----------------------------+--------------+



   .. method:: assertMultiLineEqual(first, second, msg=None)

      Test that the multiline string *first* is equal to the string *second*.
      When not equal a diff of the two strings highlighting the differences
      will be included in the error message. This method is used by default
      when comparing strings with :meth:`assertEqual`.

      .. versionadded:: 3.1


   .. method:: assertSequenceEqual(seq1, seq2, msg=None, seq_type=None)

      Tests that two sequences are equal.  If a *seq_type* is supplied, both
      *seq1* and *seq2* must be instances of *seq_type* or a failure will
      be raised.  If the sequences are different an error message is
      constructed that shows the difference between the two.

      This method is not called directly by :meth:`assertEqual`, but
      it's used to implement :meth:`assertListEqual` and
      :meth:`assertTupleEqual`.

      .. versionadded:: 3.1


   .. method:: assertListEqual(list1, list2, msg=None)
               assertTupleEqual(tuple1, tuple2, msg=None)

      Tests that two lists or tuples are equal.  If not an error message is
      constructed that shows only the differences between the two.  An error
      is also raised if either of the parameters are of the wrong type.
      These methods are used by default when comparing lists or tuples with
      :meth:`assertEqual`.

      .. versionadded:: 3.1


   .. method:: assertSetEqual(set1, set2, msg=None)

      Tests that two sets are equal.  If not, an error message is constructed
      that lists the differences between the sets.  This method is used by
      default when comparing sets or frozensets with :meth:`assertEqual`.

      Fails if either of *set1* or *set2* does not have a :meth:`set.difference`
      method.

      .. versionadded:: 3.1


   .. method:: assertDictEqual(expected, actual, msg=None)

      Test that two dictionaries are equal.  If not, an error message is
      constructed that shows the differences in the dictionaries. This
      method will be used by default to compare dictionaries in
      calls to :meth:`assertEqual`.

      .. versionadded:: 3.1



   .. _other-methods-and-attrs:

   Finally the :class:`TestCase` provides the following methods and attributes:


   .. method:: fail(msg=None)

      Signals a test failure unconditionally, with *msg* or ``None`` for
      the error message.


   .. attribute:: failureException

      This class attribute gives the exception raised by the test method.  If a
      test framework needs to use a specialized exception, possibly to carry
      additional information, it must subclass this exception in order to "play
      fair" with the framework.  The initial value of this attribute is
      :exc:`AssertionError`.


   .. attribute:: longMessage

      If set to ``True`` then any explicit failure message you pass in to the
      :ref:`assert methods <assert-methods>` will be appended to the end of the
      normal failure message.  The normal messages contain useful information
      about the objects involved, for example the message from assertEqual
      shows you the repr of the two unequal objects. Setting this attribute
      to ``True`` allows you to have a custom error message in addition to the
      normal one.

      This attribute defaults to ``True``. If set to False then a custom message
      passed to an assert method will silence the normal message.

      The class setting can be overridden in individual tests by assigning an
      instance attribute to ``True`` or ``False`` before calling the assert methods.

      .. versionadded:: 3.1


   .. attribute:: maxDiff

      This attribute controls the maximum length of diffs output by assert
      methods that report diffs on failure. It defaults to 80*8 characters.
      Assert methods affected by this attribute are
      :meth:`assertSequenceEqual` (including all the sequence comparison
      methods that delegate to it), :meth:`assertDictEqual` and
      :meth:`assertMultiLineEqual`.

      Setting ``maxDiff`` to None means that there is no maximum length of
      diffs.

      .. versionadded:: 3.2


   Testing frameworks can use the following methods to collect information on
   the test:


   .. method:: countTestCases()

      Return the number of tests represented by this test object.  For
      :class:`TestCase` instances, this will always be ``1``.


   .. method:: defaultTestResult()

      Return an instance of the test result class that should be used for this
      test case class (if no other result instance is provided to the
      :meth:`run` method).

      For :class:`TestCase` instances, this will always be an instance of
      :class:`TestResult`; subclasses of :class:`TestCase` should override this
      as necessary.


   .. method:: id()

      Return a string identifying the specific test case.  This is usually the
      full name of the test method, including the module and class name.


   .. method:: shortDescription()

      Returns a description of the test, or ``None`` if no description
      has been provided.  The default implementation of this method
      returns the first line of the test method's docstring, if available,
      or ``None``.

      .. versionchanged:: 3.1
         In 3.1 this was changed to add the test name to the short description
         even in the presence of a docstring.  This caused compatibility issues
         with unittest extensions and adding the test name was moved to the
         :class:`TextTestResult` in Python 3.2.


   .. method:: addCleanup(function, *args, **kwargs)

      Add a function to be called after :meth:`tearDown` to cleanup resources
      used during the test. Functions will be called in reverse order to the
      order they are added (LIFO). They are called with any arguments and
      keyword arguments passed into :meth:`addCleanup` when they are
      added.

      If :meth:`setUp` fails, meaning that :meth:`tearDown` is not called,
      then any cleanup functions added will still be called.

      .. versionadded:: 3.2


   .. method:: doCleanups()

      This method is called unconditionally after :meth:`tearDown`, or
      after :meth:`setUp` if :meth:`setUp` raises an exception.

      It is responsible for calling all the cleanup functions added by
      :meth:`addCleanup`. If you need cleanup functions to be called
      *prior* to :meth:`tearDown` then you can call :meth:`doCleanups`
      yourself.

      :meth:`doCleanups` pops methods off the stack of cleanup
      functions one at a time, so it can be called at any time.

      .. versionadded:: 3.2


.. class:: FunctionTestCase(testFunc, setUp=None, tearDown=None, description=None)

   This class implements the portion of the :class:`TestCase` interface which
   allows the test runner to drive the test, but does not provide the methods
   which test code can use to check and report errors.  This is used to create
   test cases using legacy test code, allowing it to be integrated into a
   :mod:`unittest`-based test framework.


.. _deprecated-aliases:

Deprecated aliases
##################

For historical reasons, some of the :class:`TestCase` methods had one or more
aliases that are now deprecated.  The following table lists the correct names
along with their deprecated aliases:

   ==============================  ====================== ======================
    Method Name                     Deprecated alias       Deprecated alias
   ==============================  ====================== ======================
    :meth:`.assertEqual`            failUnlessEqual        assertEquals
    :meth:`.assertNotEqual`         failIfEqual            assertNotEquals
    :meth:`.assertTrue`             failUnless             assert\_
    :meth:`.assertFalse`            failIf
    :meth:`.assertRaises`           failUnlessRaises
    :meth:`.assertAlmostEqual`      failUnlessAlmostEqual  assertAlmostEquals
    :meth:`.assertNotAlmostEqual`   failIfAlmostEqual      assertNotAlmostEquals
    :meth:`.assertRegex`                                   assertRegexpMatches
    :meth:`.assertRaisesRegex`                             assertRaisesRegexp
   ==============================  ====================== ======================

   .. deprecated-removed:: 3.1 3.3
         the fail* aliases listed in the second column.
   .. deprecated:: 3.2
         the assert* aliases listed in the third column.
   .. deprecated:: 3.2
         ``assertRegexpMatches`` and ``assertRaisesRegexp`` have been renamed to
         :meth:`.assertRegex` and :meth:`.assertRaisesRegex`


.. _testsuite-objects:

Grouping tests
~~~~~~~~~~~~~~

.. class:: TestSuite(tests=())

   This class represents an aggregation of individual tests cases and test suites.
   The class presents the interface needed by the test runner to allow it to be run
   as any other test case.  Running a :class:`TestSuite` instance is the same as
   iterating over the suite, running each test individually.

   If *tests* is given, it must be an iterable of individual test cases or other
   test suites that will be used to build the suite initially. Additional methods
   are provided to add test cases and suites to the collection later on.

   :class:`TestSuite` objects behave much like :class:`TestCase` objects, except
   they do not actually implement a test.  Instead, they are used to aggregate
   tests into groups of tests that should be run together. Some additional
   methods are available to add tests to :class:`TestSuite` instances:


   .. method:: TestSuite.addTest(test)

      Add a :class:`TestCase` or :class:`TestSuite` to the suite.


   .. method:: TestSuite.addTests(tests)

      Add all the tests from an iterable of :class:`TestCase` and :class:`TestSuite`
      instances to this test suite.

      This is equivalent to iterating over *tests*, calling :meth:`addTest` for
      each element.

   :class:`TestSuite` shares the following methods with :class:`TestCase`:


   .. method:: run(result)

      Run the tests associated with this suite, collecting the result into the
      test result object passed as *result*.  Note that unlike
      :meth:`TestCase.run`, :meth:`TestSuite.run` requires the result object to
      be passed in.


   .. method:: debug()

      Run the tests associated with this suite without collecting the
      result. This allows exceptions raised by the test to be propagated to the
      caller and can be used to support running tests under a debugger.


   .. method:: countTestCases()

      Return the number of tests represented by this test object, including all
      individual tests and sub-suites.


   .. method:: __iter__()

      Tests grouped by a :class:`TestSuite` are always accessed by iteration.
      Subclasses can lazily provide tests by overriding :meth:`__iter__`. Note
      that this method maybe called several times on a single suite
      (for example when counting tests or comparing for equality)
      so the tests returned must be the same for repeated iterations.

      .. versionchanged:: 3.2
         In earlier versions the :class:`TestSuite` accessed tests directly rather
         than through iteration, so overriding :meth:`__iter__` wasn't sufficient
         for providing tests.

   In the typical usage of a :class:`TestSuite` object, the :meth:`run` method
   is invoked by a :class:`TestRunner` rather than by the end-user test harness.


Loading and running tests
~~~~~~~~~~~~~~~~~~~~~~~~~

.. class:: TestLoader()

   The :class:`TestLoader` class is used to create test suites from classes and
   modules.  Normally, there is no need to create an instance of this class; the
   :mod:`unittest` module provides an instance that can be shared as
   ``unittest.defaultTestLoader``. Using a subclass or instance, however, allows
   customization of some configurable properties.

   :class:`TestLoader` objects have the following methods:


   .. method:: loadTestsFromTestCase(testCaseClass)

      Return a suite of all tests cases contained in the :class:`TestCase`\ -derived
      :class:`testCaseClass`.


   .. method:: loadTestsFromModule(module)

      Return a suite of all tests cases contained in the given module. This
      method searches *module* for classes derived from :class:`TestCase` and
      creates an instance of the class for each test method defined for the
      class.

      .. note::

         While using a hierarchy of :class:`TestCase`\ -derived classes can be
         convenient in sharing fixtures and helper functions, defining test
         methods on base classes that are not intended to be instantiated
         directly does not play well with this method.  Doing so, however, can
         be useful when the fixtures are different and defined in subclasses.

      If a module provides a ``load_tests`` function it will be called to
      load the tests. This allows modules to customize test loading.
      This is the `load_tests protocol`_.

      .. versionchanged:: 3.2
         Support for ``load_tests`` added.


   .. method:: loadTestsFromName(name, module=None)

      Return a suite of all tests cases given a string specifier.

      The specifier *name* is a "dotted name" that may resolve either to a
      module, a test case class, a test method within a test case class, a
      :class:`TestSuite` instance, or a callable object which returns a
      :class:`TestCase` or :class:`TestSuite` instance.  These checks are
      applied in the order listed here; that is, a method on a possible test
      case class will be picked up as "a test method within a test case class",
      rather than "a callable object".

      For example, if you have a module :mod:`SampleTests` containing a
      :class:`TestCase`\ -derived class :class:`SampleTestCase` with three test
      methods (:meth:`test_one`, :meth:`test_two`, and :meth:`test_three`), the
      specifier ``'SampleTests.SampleTestCase'`` would cause this method to
      return a suite which will run all three test methods. Using the specifier
      ``'SampleTests.SampleTestCase.test_two'`` would cause it to return a test
      suite which will run only the :meth:`test_two` test method. The specifier
      can refer to modules and packages which have not been imported; they will
      be imported as a side-effect.

      The method optionally resolves *name* relative to the given *module*.


   .. method:: loadTestsFromNames(names, module=None)

      Similar to :meth:`loadTestsFromName`, but takes a sequence of names rather
      than a single name.  The return value is a test suite which supports all
      the tests defined for each name.


   .. method:: getTestCaseNames(testCaseClass)

      Return a sorted sequence of method names found within *testCaseClass*;
      this should be a subclass of :class:`TestCase`.


   .. method:: discover(start_dir, pattern='test*.py', top_level_dir=None)

      Find and return all test modules from the specified start directory,
      recursing into subdirectories to find them. Only test files that match
      *pattern* will be loaded. (Using shell style pattern matching.) Only
      module names that are importable (i.e. are valid Python identifiers) will
      be loaded.

      All test modules must be importable from the top level of the project. If
      the start directory is not the top level directory then the top level
      directory must be specified separately.

      If importing a module fails, for example due to a syntax error, then this
      will be recorded as a single error and discovery will continue.

      If a test package name (directory with :file:`__init__.py`) matches the
      pattern then the package will be checked for a ``load_tests``
      function. If this exists then it will be called with *loader*, *tests*,
      *pattern*.

      If load_tests exists then discovery does *not* recurse into the package,
      ``load_tests`` is responsible for loading all tests in the package.

      The pattern is deliberately not stored as a loader attribute so that
      packages can continue discovery themselves. *top_level_dir* is stored so
      ``load_tests`` does not need to pass this argument in to
      ``loader.discover()``.

      *start_dir* can be a dotted module name as well as a directory.

      .. versionadded:: 3.2


   The following attributes of a :class:`TestLoader` can be configured either by
   subclassing or assignment on an instance:


   .. attribute:: testMethodPrefix

      String giving the prefix of method names which will be interpreted as test
      methods.  The default value is ``'test'``.

      This affects :meth:`getTestCaseNames` and all the :meth:`loadTestsFrom\*`
      methods.


   .. attribute:: sortTestMethodsUsing

      Function to be used to compare method names when sorting them in
      :meth:`getTestCaseNames` and all the :meth:`loadTestsFrom\*` methods.


   .. attribute:: suiteClass

      Callable object that constructs a test suite from a list of tests. No
      methods on the resulting object are needed.  The default value is the
      :class:`TestSuite` class.

      This affects all the :meth:`loadTestsFrom\*` methods.


.. class:: TestResult

   This class is used to compile information about which tests have succeeded
   and which have failed.

   A :class:`TestResult` object stores the results of a set of tests.  The
   :class:`TestCase` and :class:`TestSuite` classes ensure that results are
   properly recorded; test authors do not need to worry about recording the
   outcome of tests.

   Testing frameworks built on top of :mod:`unittest` may want access to the
   :class:`TestResult` object generated by running a set of tests for reporting
   purposes; a :class:`TestResult` instance is returned by the
   :meth:`TestRunner.run` method for this purpose.

   :class:`TestResult` instances have the following attributes that will be of
   interest when inspecting the results of running a set of tests:


   .. attribute:: errors

      A list containing 2-tuples of :class:`TestCase` instances and strings
      holding formatted tracebacks. Each tuple represents a test which raised an
      unexpected exception.

   .. attribute:: failures

      A list containing 2-tuples of :class:`TestCase` instances and strings
      holding formatted tracebacks. Each tuple represents a test where a failure
      was explicitly signalled using the :meth:`TestCase.fail\*` or
      :meth:`TestCase.assert\*` methods.

   .. attribute:: skipped

      A list containing 2-tuples of :class:`TestCase` instances and strings
      holding the reason for skipping the test.

      .. versionadded:: 3.1

   .. attribute:: expectedFailures

      A list containing 2-tuples of :class:`TestCase` instances and strings
      holding formatted tracebacks.  Each tuple represents an expected failure
      of the test case.

   .. attribute:: unexpectedSuccesses

      A list containing :class:`TestCase` instances that were marked as expected
      failures, but succeeded.

   .. attribute:: shouldStop

      Set to ``True`` when the execution of tests should stop by :meth:`stop`.


   .. attribute:: testsRun

      The total number of tests run so far.


   .. attribute:: buffer

      If set to true, ``sys.stdout`` and ``sys.stderr`` will be buffered in between
      :meth:`startTest` and :meth:`stopTest` being called. Collected output will
      only be echoed onto the real ``sys.stdout`` and ``sys.stderr`` if the test
      fails or errors. Any output is also attached to the failure / error message.

      .. versionadded:: 3.2


   .. attribute:: failfast

      If set to true :meth:`stop` will be called on the first failure or error,
      halting the test run.

      .. versionadded:: 3.2


   .. method:: wasSuccessful()

      Return ``True`` if all tests run so far have passed, otherwise returns
      ``False``.


   .. method:: stop()

      This method can be called to signal that the set of tests being run should
      be aborted by setting the :attr:`shouldStop` attribute to ``True``.
      :class:`TestRunner` objects should respect this flag and return without
      running any additional tests.

      For example, this feature is used by the :class:`TextTestRunner` class to
      stop the test framework when the user signals an interrupt from the
      keyboard.  Interactive tools which provide :class:`TestRunner`
      implementations can use this in a similar manner.

   The following methods of the :class:`TestResult` class are used to maintain
   the internal data structures, and may be extended in subclasses to support
   additional reporting requirements.  This is particularly useful in building
   tools which support interactive reporting while tests are being run.


   .. method:: startTest(test)

      Called when the test case *test* is about to be run.

   .. method:: stopTest(test)

      Called after the test case *test* has been executed, regardless of the
      outcome.

   .. method:: startTestRun(test)

      Called once before any tests are executed.

      .. versionadded:: 3.2


   .. method:: stopTestRun(test)

      Called once after all tests are executed.

      .. versionadded:: 3.2


   .. method:: addError(test, err)

      Called when the test case *test* raises an unexpected exception *err* is a
      tuple of the form returned by :func:`sys.exc_info`: ``(type, value,
      traceback)``.

      The default implementation appends a tuple ``(test, formatted_err)`` to
      the instance's :attr:`errors` attribute, where *formatted_err* is a
      formatted traceback derived from *err*.


   .. method:: addFailure(test, err)

      Called when the test case *test* signals a failure. *err* is a tuple of
      the form returned by :func:`sys.exc_info`: ``(type, value, traceback)``.

      The default implementation appends a tuple ``(test, formatted_err)`` to
      the instance's :attr:`failures` attribute, where *formatted_err* is a
      formatted traceback derived from *err*.


   .. method:: addSuccess(test)

      Called when the test case *test* succeeds.

      The default implementation does nothing.


   .. method:: addSkip(test, reason)

      Called when the test case *test* is skipped.  *reason* is the reason the
      test gave for skipping.

      The default implementation appends a tuple ``(test, reason)`` to the
      instance's :attr:`skipped` attribute.


   .. method:: addExpectedFailure(test, err)

      Called when the test case *test* fails, but was marked with the
      :func:`expectedFailure` decorator.

      The default implementation appends a tuple ``(test, formatted_err)`` to
      the instance's :attr:`expectedFailures` attribute, where *formatted_err*
      is a formatted traceback derived from *err*.


   .. method:: addUnexpectedSuccess(test)

      Called when the test case *test* was marked with the
      :func:`expectedFailure` decorator, but succeeded.

      The default implementation appends the test to the instance's
      :attr:`unexpectedSuccesses` attribute.


.. class:: TextTestResult(stream, descriptions, verbosity)

   A concrete implementation of :class:`TestResult` used by the
   :class:`TextTestRunner`.

   .. versionadded:: 3.2
      This class was previously named ``_TextTestResult``. The old name still
      exists as an alias but is deprecated.


.. data:: defaultTestLoader

   Instance of the :class:`TestLoader` class intended to be shared.  If no
   customization of the :class:`TestLoader` is needed, this instance can be used
   instead of repeatedly creating new instances.


.. class:: TextTestRunner(stream=sys.stderr, descriptions=True, verbosity=1, runnerclass=None, warnings=None)

   A basic test runner implementation which prints results on standard error.  It
   has a few configurable parameters, but is essentially very simple.  Graphical
   applications which run test suites should provide alternate implementations.

   By default this runner shows :exc:`DeprecationWarning`,
   :exc:`PendingDeprecationWarning`, and :exc:`ImportWarning` even if they are
   :ref:`ignored by default <warning-ignored>`. Deprecation warnings caused by
   :ref:`deprecated unittest methods <deprecated-aliases>` are also
   special-cased and, when the warning filters are ``'default'`` or ``'always'``,
   they will appear only once per-module, in order to avoid too many warning
   messages.  This behavior can be overridden using the :option`-Wd` or
   :option:`-Wa` options and leaving *warnings* to ``None``.

   .. method:: _makeResult()

      This method returns the instance of ``TestResult`` used by :meth:`run`.
      It is not intended to be called directly, but can be overridden in
      subclasses to provide a custom ``TestResult``.

      ``_makeResult()`` instantiates the class or callable passed in the
      ``TextTestRunner`` constructor as the ``resultclass`` argument. It
      defaults to :class:`TextTestResult` if no ``resultclass`` is provided.
      The result class is instantiated with the following arguments::

        stream, descriptions, verbosity

   .. versionchanged:: 3.2
      Added the ``warnings`` argument.

.. function:: main(module='__main__', defaultTest=None, argv=None, testRunner=None, \
                   testLoader=unittest.loader.defaultTestLoader, exit=True, verbosity=1, \
                   failfast=None, catchbreak=None, buffer=None, warnings=None)

   A command-line program that runs a set of tests; this is primarily for making
   test modules conveniently executable.  The simplest use for this function is to
   include the following line at the end of a test script::

      if __name__ == '__main__':
          unittest.main()

   You can run tests with more detailed information by passing in the verbosity
   argument::

      if __name__ == '__main__':
          unittest.main(verbosity=2)

   The *testRunner* argument can either be a test runner class or an already
   created instance of it. By default ``main`` calls :func:`sys.exit` with
   an exit code indicating success or failure of the tests run.

   ``main`` supports being used from the interactive interpreter by passing in the
   argument ``exit=False``. This displays the result on standard output without
   calling :func:`sys.exit`::

      >>> from unittest import main
      >>> main(module='test_module', exit=False)

   The ``failfast``, ``catchbreak`` and ``buffer`` parameters have the same
   effect as the same-name `command-line options`_.

   The *warning* argument specifies the :ref:`warning filter <warning-filter>`
   that should be used while running the tests.  If it's not specified, it will
   remain ``None`` if a :option:`-W` option is passed to :program:`python`,
   otherwise it will be set to ``'default'``.

   Calling ``main`` actually returns an instance of the ``TestProgram`` class.
   This stores the result of the tests run as the ``result`` attribute.

   .. versionchanged:: 3.2
      The ``exit``, ``verbosity``, ``failfast``, ``catchbreak``, ``buffer``,
      and ``warnings`` parameters were added.


load_tests Protocol
###################

.. versionadded:: 3.2

Modules or packages can customize how tests are loaded from them during normal
test runs or test discovery by implementing a function called ``load_tests``.

If a test module defines ``load_tests`` it will be called by
:meth:`TestLoader.loadTestsFromModule` with the following arguments::

    load_tests(loader, standard_tests, None)

It should return a :class:`TestSuite`.

*loader* is the instance of :class:`TestLoader` doing the loading.
*standard_tests* are the tests that would be loaded by default from the
module. It is common for test modules to only want to add or remove tests
from the standard set of tests.
The third argument is used when loading packages as part of test discovery.

A typical ``load_tests`` function that loads tests from a specific set of
:class:`TestCase` classes may look like::

    test_cases = (TestCase1, TestCase2, TestCase3)

    def load_tests(loader, tests, pattern):
        suite = TestSuite()
        for test_class in test_cases:
            tests = loader.loadTestsFromTestCase(test_class)
            suite.addTests(tests)
        return suite

If discovery is started, either from the command line or by calling
:meth:`TestLoader.discover`, with a pattern that matches a package
name then the package :file:`__init__.py` will be checked for ``load_tests``.

.. note::

   The default pattern is 'test*.py'. This matches all Python files
   that start with 'test' but *won't* match any test directories.

   A pattern like 'test*' will match test packages as well as
   modules.

If the package :file:`__init__.py` defines ``load_tests`` then it will be
called and discovery not continued into the package. ``load_tests``
is called with the following arguments::

    load_tests(loader, standard_tests, pattern)

This should return a :class:`TestSuite` representing all the tests
from the package. (``standard_tests`` will only contain tests
collected from :file:`__init__.py`.)

Because the pattern is passed into ``load_tests`` the package is free to
continue (and potentially modify) test discovery. A 'do nothing'
``load_tests`` function for a test package would look like::

    def load_tests(loader, standard_tests, pattern):
        # top level directory cached on loader instance
        this_dir = os.path.dirname(__file__)
        package_tests = loader.discover(start_dir=this_dir, pattern=pattern)
        standard_tests.addTests(package_tests)
        return standard_tests


Class and Module Fixtures
-------------------------

Class and module level fixtures are implemented in :class:`TestSuite`. When
the test suite encounters a test from a new class then :meth:`tearDownClass`
from the previous class (if there is one) is called, followed by
:meth:`setUpClass` from the new class.

Similarly if a test is from a different module from the previous test then
``tearDownModule`` from the previous module is run, followed by
``setUpModule`` from the new module.

After all the tests have run the final ``tearDownClass`` and
``tearDownModule`` are run.

Note that shared fixtures do not play well with [potential] features like test
parallelization and they break test isolation. They should be used with care.

The default ordering of tests created by the unittest test loaders is to group
all tests from the same modules and classes together. This will lead to
``setUpClass`` / ``setUpModule`` (etc) being called exactly once per class and
module. If you randomize the order, so that tests from different modules and
classes are adjacent to each other, then these shared fixture functions may be
called multiple times in a single test run.

Shared fixtures are not intended to work with suites with non-standard
ordering. A ``BaseTestSuite`` still exists for frameworks that don't want to
support shared fixtures.

If there are any exceptions raised during one of the shared fixture functions
the test is reported as an error. Because there is no corresponding test
instance an ``_ErrorHolder`` object (that has the same interface as a
:class:`TestCase`) is created to represent the error. If you are just using
the standard unittest test runner then this detail doesn't matter, but if you
are a framework author it may be relevant.


setUpClass and tearDownClass
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These must be implemented as class methods::

    import unittest

    class Test(unittest.TestCase):
        @classmethod
        def setUpClass(cls):
            cls._connection = createExpensiveConnectionObject()

        @classmethod
        def tearDownClass(cls):
            cls._connection.destroy()

If you want the ``setUpClass`` and ``tearDownClass`` on base classes called
then you must call up to them yourself. The implementations in
:class:`TestCase` are empty.

If an exception is raised during a ``setUpClass`` then the tests in the class
are not run and the ``tearDownClass`` is not run. Skipped classes will not
have ``setUpClass`` or ``tearDownClass`` run. If the exception is a
``SkipTest`` exception then the class will be reported as having been skipped
instead of as an error.


setUpModule and tearDownModule
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These should be implemented as functions::

    def setUpModule():
        createConnection()

    def tearDownModule():
        closeConnection()

If an exception is raised in a ``setUpModule`` then none of the tests in the
module will be run and the ``tearDownModule`` will not be run. If the exception is a
``SkipTest`` exception then the module will be reported as having been skipped
instead of as an error.


Signal Handling
---------------

.. versionadded:: 3.2

The :option:`-c/--catch <unittest -c>` command-line option to unittest,
along with the ``catchbreak`` parameter to :func:`unittest.main()`, provide
more friendly handling of control-C during a test run. With catch break
behavior enabled control-C will allow the currently running test to complete,
and the test run will then end and report all the results so far. A second
control-c will raise a :exc:`KeyboardInterrupt` in the usual way.

The control-c handling signal handler attempts to remain compatible with code or
tests that install their own :const:`signal.SIGINT` handler. If the ``unittest``
handler is called but *isn't* the installed :const:`signal.SIGINT` handler,
i.e. it has been replaced by the system under test and delegated to, then it
calls the default handler. This will normally be the expected behavior by code
that replaces an installed handler and delegates to it. For individual tests
that need ``unittest`` control-c handling disabled the :func:`removeHandler`
decorator can be used.

There are a few utility functions for framework authors to enable control-c
handling functionality within test frameworks.

.. function:: installHandler()

   Install the control-c handler. When a :const:`signal.SIGINT` is received
   (usually in response to the user pressing control-c) all registered results
   have :meth:`~TestResult.stop` called.


.. function:: registerResult(result)

   Register a :class:`TestResult` object for control-c handling. Registering a
   result stores a weak reference to it, so it doesn't prevent the result from
   being garbage collected.

   Registering a :class:`TestResult` object has no side-effects if control-c
   handling is not enabled, so test frameworks can unconditionally register
   all results they create independently of whether or not handling is enabled.


.. function:: removeResult(result)

   Remove a registered result. Once a result has been removed then
   :meth:`~TestResult.stop` will no longer be called on that result object in
   response to a control-c.


.. function:: removeHandler(function=None)

   When called without arguments this function removes the control-c handler
   if it has been installed. This function can also be used as a test decorator
   to temporarily remove the handler whilst the test is being executed::

      @unittest.removeHandler
      def test_signal_handling(self):
          ...
