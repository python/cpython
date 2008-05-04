
:mod:`unittest` --- Unit testing framework
==========================================

.. module:: unittest
   :synopsis: Unit testing framework for Python.
.. moduleauthor:: Steve Purcell <stephen_purcell@yahoo.com>
.. sectionauthor:: Steve Purcell <stephen_purcell@yahoo.com>
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>
.. sectionauthor:: Raymond Hettinger <python@rcn.com>


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
fixtures using :class:`TestCase`, the :meth:`setUp` and :meth:`tearDown` methods
can be overridden to provide initialization and cleanup for the fixture.  With
:class:`FunctionTestCase`, existing functions can be passed to the constructor
for these purposes.  When the test is run, the fixture initialization is run
first; if it succeeds, the cleanup method is run after the test has been
executed, regardless of the outcome of the test.  Each instance of the
:class:`TestCase` will only be used to run a single test method, so a new
fixture is created for each test.

Test suites are implemented by the :class:`TestSuite` class.  This class allows
individual tests and test suites to be aggregated; when the suite is executed,
all tests added directly to the suite and in "child" test suites are run.

A test runner is an object that provides a single method, :meth:`run`, which
accepts a :class:`TestCase` or :class:`TestSuite` object as a parameter, and
returns a result object.  The class :class:`TestResult` is provided for use as
the result object. :mod:`unittest` provides the :class:`TextTestRunner` as an
example test runner which reports test results on the standard error stream by
default.  Alternate runners can be implemented for other environments (such as
graphical environments) without any need to derive from a specific class.


.. seealso::

   Module :mod:`doctest`
      Another test-support module with a very different flavor.

   `Simple Smalltalk Testing: With Patterns <http://www.XProgramming.com/testfram.htm>`_
      Kent Beck's original paper on testing frameworks using the pattern shared by
      :mod:`unittest`.


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
           self.seq = range(10)

       def testshuffle(self):
           # make sure the shuffled sequence does not lose any elements
           random.shuffle(self.seq)
           self.seq.sort()
           self.assertEqual(self.seq, range(10))

       def testchoice(self):
           element = random.choice(self.seq)
           self.assert_(element in self.seq)

       def testsample(self):
           self.assertRaises(ValueError, random.sample, self.seq, 20)
           for element in random.sample(self.seq, 5):
               self.assert_(element in self.seq)

   if __name__ == '__main__':
       unittest.main()

A testcase is created by subclassing :class:`unittest.TestCase`. The three
individual tests are defined with methods whose names start with the letters
``test``.  This naming convention informs the test runner about which methods
represent tests.

The crux of each test is a call to :meth:`assertEqual` to check for an expected
result; :meth:`assert_` to verify a condition; or :meth:`assertRaises` to verify
that an expected exception gets raised.  These methods are used instead of the
:keyword:`assert` statement so the test runner can accumulate all test results
and produce a report.

When a :meth:`setUp` method is defined, the test runner will run that method
prior to each test.  Likewise, if a :meth:`tearDown` method is defined, the test
runner will invoke that method after each test.  In the example, :meth:`setUp`
was used to create a fresh sequence for each test.

The final block shows a simple way to run the tests. :func:`unittest.main`
provides a command line interface to the test script.  When run from the command
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

   testchoice (__main__.TestSequenceFunctions) ... ok
   testsample (__main__.TestSequenceFunctions) ... ok
   testshuffle (__main__.TestSequenceFunctions) ... ok

   ----------------------------------------------------------------------
   Ran 3 tests in 0.110s

   OK

The above examples show the most commonly used :mod:`unittest` features which
are sufficient to meet many everyday testing needs.  The remainder of the
documentation explores the full feature set from first principles.


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

The simplest :class:`TestCase` subclass will simply override the :meth:`runTest`
method in order to perform specific testing code::

   import unittest

   class DefaultWidgetSizeTestCase(unittest.TestCase):
       def runTest(self):
           widget = Widget('The widget')
           self.assertEqual(widget.size(), (50, 50), 'incorrect default size')

Note that in order to test something, we use the one of the :meth:`assert\*` or
:meth:`fail\*` methods provided by the :class:`TestCase` base class.  If the
test fails, an exception will be raised, and :mod:`unittest` will identify the
test case as a :dfn:`failure`.  Any other exceptions will be treated as
:dfn:`errors`. This helps you identify where the problem is: :dfn:`failures` are
caused by incorrect results - a 5 where you expected a 6. :dfn:`Errors` are
caused by incorrect code - e.g., a :exc:`TypeError` caused by an incorrect
function call.

The way to run a test case will be described later.  For now, note that to
construct an instance of such a test case, we call its constructor without
arguments::

   testCase = DefaultWidgetSizeTestCase()

Now, such test cases can be numerous, and their set-up can be repetitive.  In
the above case, constructing a :class:`Widget` in each of 100 Widget test case
subclasses would mean unsightly duplication.

Luckily, we can factor out such set-up code by implementing a method called
:meth:`setUp`, which the testing framework will automatically call for us when
we run the test::

   import unittest

   class SimpleWidgetTestCase(unittest.TestCase):
       def setUp(self):
           self.widget = Widget('The widget')

   class DefaultWidgetSizeTestCase(SimpleWidgetTestCase):
       def runTest(self):
           self.failUnless(self.widget.size() == (50,50),
                           'incorrect default size')

   class WidgetResizeTestCase(SimpleWidgetTestCase):
       def runTest(self):
           self.widget.resize(100,150)
           self.failUnless(self.widget.size() == (100,150),
                           'wrong size after resize')

If the :meth:`setUp` method raises an exception while the test is running, the
framework will consider the test to have suffered an error, and the
:meth:`runTest` method will not be executed.

Similarly, we can provide a :meth:`tearDown` method that tidies up after the
:meth:`runTest` method has been run::

   import unittest

   class SimpleWidgetTestCase(unittest.TestCase):
       def setUp(self):
           self.widget = Widget('The widget')

       def tearDown(self):
           self.widget.dispose()
           self.widget = None

If :meth:`setUp` succeeded, the :meth:`tearDown` method will be run whether
:meth:`runTest` succeeded or not.

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

       def testDefaultSize(self):
           self.failUnless(self.widget.size() == (50,50),
                           'incorrect default size')

       def testResize(self):
           self.widget.resize(100,150)
           self.failUnless(self.widget.size() == (100,150),
                           'wrong size after resize')

Here we have not provided a :meth:`runTest` method, but have instead provided
two different test methods.  Class instances will now each run one of the
:meth:`test\*`  methods, with ``self.widget`` created and destroyed separately
for each instance.  When creating an instance we must specify the test method it
is to run.  We do this by passing the method name in the constructor::

   defaultSizeTestCase = WidgetTestCase('testDefaultSize')
   resizeTestCase = WidgetTestCase('testResize')

Test case instances are grouped together according to the features they test.
:mod:`unittest` provides a mechanism for this: the :dfn:`test suite`,
represented by :mod:`unittest`'s :class:`TestSuite` class::

   widgetTestSuite = unittest.TestSuite()
   widgetTestSuite.addTest(WidgetTestCase('testDefaultSize'))
   widgetTestSuite.addTest(WidgetTestCase('testResize'))

For the ease of running tests, as we will see later, it is a good idea to
provide in each test module a callable object that returns a pre-built test
suite::

   def suite():
       suite = unittest.TestSuite()
       suite.addTest(WidgetTestCase('testDefaultSize'))
       suite.addTest(WidgetTestCase('testResize'))
       return suite

or even::

   def suite():
       tests = ['testDefaultSize', 'testResize']

       return unittest.TestSuite(map(WidgetTestCase, tests))

Since it is a common pattern to create a :class:`TestCase` subclass with many
similarly named test functions, :mod:`unittest` provides a :class:`TestLoader`
class that can be used to automate the process of creating a test suite and
populating it with individual tests. For example, ::

   suite = unittest.TestLoader().loadTestsFromTestCase(WidgetTestCase)

will create a test suite that will run ``WidgetTestCase.testDefaultSize()`` and
``WidgetTestCase.testResize``. :class:`TestLoader` uses the ``'test'`` method
name prefix to identify test methods automatically.

Note that the order in which the various test cases will be run is determined by
sorting the test function names with the built-in :func:`cmp` function.

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

   Even though :class:`FunctionTestCase` can be used to quickly convert an existing
   test base over to a :mod:`unittest`\ -based system, this approach is not
   recommended.  Taking the time to set up proper :class:`TestCase` subclasses will
   make future test refactorings infinitely easier.


.. _unittest-contents:

Classes and functions
---------------------


.. class:: TestCase([methodName])

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
          suite.addTest(WidgetTestCase('testDefaultSize'))
          suite.addTest(WidgetTestCase('testResize'))
          return suite

   Here, we create two instances of :class:`WidgetTestCase`, each of which runs a
   single test.

   *methodName* defaults to ``'runTest'``.


.. class:: FunctionTestCase(testFunc[, setUp[, tearDown[, description]]])

   This class implements the portion of the :class:`TestCase` interface which
   allows the test runner to drive the test, but does not provide the methods which
   test code can use to check and report errors. This is used to create test cases
   using legacy test code, allowing it to be integrated into a :mod:`unittest`\
   -based test framework.


.. class:: TestSuite([tests])

   This class represents an aggregation of individual tests cases and test suites.
   The class presents the interface needed by the test runner to allow it to be run
   as any other test case.  Running a :class:`TestSuite` instance is the same as
   iterating over the suite, running each test individually.

   If *tests* is given, it must be an iterable of individual test cases or other
   test suites that will be used to build the suite initially. Additional methods
   are provided to add test cases and suites to the collection later on.


.. class:: TestLoader()

   This class is responsible for loading tests according to various criteria and
   returning them wrapped in a :class:`TestSuite`. It can load all tests within a
   given module or :class:`TestCase` subclass.


.. class:: TestResult()

   This class is used to compile information about which tests have succeeded and
   which have failed.


.. data:: defaultTestLoader

   Instance of the :class:`TestLoader` class intended to be shared.  If no
   customization of the :class:`TestLoader` is needed, this instance can be used
   instead of repeatedly creating new instances.


.. class:: TextTestRunner([stream[, descriptions[, verbosity]]])

   A basic test runner implementation which prints results on standard error.  It
   has a few configurable parameters, but is essentially very simple.  Graphical
   applications which run test suites should provide alternate implementations.


.. function:: main([module[, defaultTest[, argv[, testRunner[, testLoader]]]]])

   A command-line program that runs a set of tests; this is primarily for making
   test modules conveniently executable.  The simplest use for this function is to
   include the following line at the end of a test script::

      if __name__ == '__main__':
          unittest.main()

   The *testRunner* argument can either be a test runner class or an already
   created instance of it.

In some cases, the existing tests may have been written using the :mod:`doctest`
module.  If so, that module provides a  :class:`DocTestSuite` class that can
automatically build :class:`unittest.TestSuite` instances from the existing
:mod:`doctest`\ -based tests.


.. _testcase-objects:

TestCase Objects
----------------

Each :class:`TestCase` instance represents a single test, but each concrete
subclass may be used to define multiple tests --- the concrete class represents
a single test fixture.  The fixture is created and cleaned up for each test
case.

:class:`TestCase` instances provide three groups of methods: one group used to
run the test, another used by the test implementation to check conditions and
report failures, and some inquiry methods allowing information about the test
itself to be gathered.

Methods in the first group (running the test) are:


.. method:: TestCase.setUp()

   Method called to prepare the test fixture.  This is called immediately before
   calling the test method; any exception raised by this method will be considered
   an error rather than a test failure. The default implementation does nothing.


.. method:: TestCase.tearDown()

   Method called immediately after the test method has been called and the result
   recorded.  This is called even if the test method raised an exception, so the
   implementation in subclasses may need to be particularly careful about checking
   internal state.  Any exception raised by this method will be considered an error
   rather than a test failure.  This method will only be called if the
   :meth:`setUp` succeeds, regardless of the outcome of the test method. The
   default implementation does nothing.


.. method:: TestCase.run([result])

   Run the test, collecting the result into the test result object passed as
   *result*.  If *result* is omitted or :const:`None`, a temporary result object is
   created (by calling the :meth:`defaultTestCase` method) and used; this result
   object is not returned to :meth:`run`'s caller.

   The same effect may be had by simply calling the :class:`TestCase` instance.


.. method:: TestCase.debug()

   Run the test without collecting the result.  This allows exceptions raised by
   the test to be propagated to the caller, and can be used to support running
   tests under a debugger.

The test code can use any of the following methods to check for and report
failures.


.. method:: TestCase.assert_(expr[, msg])
            TestCase.failUnless(expr[, msg])
            TestCase.assertTrue(expr[, msg])

   Signal a test failure if *expr* is false; the explanation for the error will be
   *msg* if given, otherwise it will be :const:`None`.


.. method:: TestCase.assertEqual(first, second[, msg])
            TestCase.failUnlessEqual(first, second[, msg])

   Test that *first* and *second* are equal.  If the values do not compare equal,
   the test will fail with the explanation given by *msg*, or :const:`None`.  Note
   that using :meth:`failUnlessEqual` improves upon doing the comparison as the
   first parameter to :meth:`failUnless`:  the default value for *msg* can be
   computed to include representations of both *first* and *second*.


.. method:: TestCase.assertNotEqual(first, second[, msg])
            TestCase.failIfEqual(first, second[, msg])

   Test that *first* and *second* are not equal.  If the values do compare equal,
   the test will fail with the explanation given by *msg*, or :const:`None`.  Note
   that using :meth:`failIfEqual` improves upon doing the comparison as the first
   parameter to :meth:`failUnless` is that the default value for *msg* can be
   computed to include representations of both *first* and *second*.


.. method:: TestCase.assertAlmostEqual(first, second[, places[, msg]])
            TestCase.failUnlessAlmostEqual(first, second[, places[, msg]])

   Test that *first* and *second* are approximately equal by computing the
   difference, rounding to the given number of *places*, and comparing to zero.
   Note that comparing a given number of decimal places is not the same as
   comparing a given number of significant digits. If the values do not compare
   equal, the test will fail with the explanation given by *msg*, or :const:`None`.


.. method:: TestCase.assertNotAlmostEqual(first, second[, places[, msg]])
            TestCase.failIfAlmostEqual(first, second[, places[, msg]])

   Test that *first* and *second* are not approximately equal by computing the
   difference, rounding to the given number of *places*, and comparing to zero.
   Note that comparing a given number of decimal places is not the same as
   comparing a given number of significant digits. If the values do not compare
   equal, the test will fail with the explanation given by *msg*, or :const:`None`.


.. method:: TestCase.assertRaises(exception, callable, ...)
            TestCase.failUnlessRaises(exception, callable, ...)

   Test that an exception is raised when *callable* is called with any positional
   or keyword arguments that are also passed to :meth:`assertRaises`.  The test
   passes if *exception* is raised, is an error if another exception is raised, or
   fails if no exception is raised.  To catch any of a group of exceptions, a tuple
   containing the exception classes may be passed as *exception*.


.. method:: TestCase.failIf(expr[, msg])
            TestCase.assertFalse(expr[, msg])

   The inverse of the :meth:`failUnless` method is the :meth:`failIf` method.  This
   signals a test failure if *expr* is true, with *msg* or :const:`None` for the
   error message.


.. method:: TestCase.fail([msg])

   Signals a test failure unconditionally, with *msg* or :const:`None` for the
   error message.


.. attribute:: TestCase.failureException

   This class attribute gives the exception raised by the :meth:`test` method.  If
   a test framework needs to use a specialized exception, possibly to carry
   additional information, it must subclass this exception in order to "play fair"
   with the framework.  The initial value of this attribute is
   :exc:`AssertionError`.

Testing frameworks can use the following methods to collect information on the
test:


.. method:: TestCase.countTestCases()

   Return the number of tests represented by this test object.  For
   :class:`TestCase` instances, this will always be ``1``.


.. method:: TestCase.defaultTestResult()

   Return an instance of the test result class that should be used for this test
   case class (if no other result instance is provided to the :meth:`run` method).

   For :class:`TestCase` instances, this will always be an instance of
   :class:`TestResult`;  subclasses of :class:`TestCase` should override this as
   necessary.


.. method:: TestCase.id()

   Return a string identifying the specific test case.  This is usually the full
   name of the test method, including the module and class name.


.. method:: TestCase.shortDescription()

   Returns a one-line description of the test, or :const:`None` if no description
   has been provided.  The default implementation of this method returns the first
   line of the test method's docstring, if available, or :const:`None`.


.. _testsuite-objects:

TestSuite Objects
-----------------

:class:`TestSuite` objects behave much like :class:`TestCase` objects, except
they do not actually implement a test.  Instead, they are used to aggregate
tests into groups of tests that should be run together. Some additional methods
are available to add tests to :class:`TestSuite` instances:


.. method:: TestSuite.addTest(test)

   Add a :class:`TestCase` or :class:`TestSuite` to the suite.


.. method:: TestSuite.addTests(tests)

   Add all the tests from an iterable of :class:`TestCase` and :class:`TestSuite`
   instances to this test suite.

   This is equivalent to iterating over *tests*, calling :meth:`addTest` for each
   element.

:class:`TestSuite` shares the following methods with :class:`TestCase`:


.. method:: TestSuite.run(result)

   Run the tests associated with this suite, collecting the result into the test
   result object passed as *result*.  Note that unlike :meth:`TestCase.run`,
   :meth:`TestSuite.run` requires the result object to be passed in.


.. method:: TestSuite.debug()

   Run the tests associated with this suite without collecting the result. This
   allows exceptions raised by the test to be propagated to the caller and can be
   used to support running tests under a debugger.


.. method:: TestSuite.countTestCases()

   Return the number of tests represented by this test object, including all
   individual tests and sub-suites.

In the typical usage of a :class:`TestSuite` object, the :meth:`run` method is
invoked by a :class:`TestRunner` rather than by the end-user test harness.


.. _testresult-objects:

TestResult Objects
------------------

A :class:`TestResult` object stores the results of a set of tests.  The
:class:`TestCase` and :class:`TestSuite` classes ensure that results are
properly recorded; test authors do not need to worry about recording the outcome
of tests.

Testing frameworks built on top of :mod:`unittest` may want access to the
:class:`TestResult` object generated by running a set of tests for reporting
purposes; a :class:`TestResult` instance is returned by the
:meth:`TestRunner.run` method for this purpose.

:class:`TestResult` instances have the following attributes that will be of
interest when inspecting the results of running a set of tests:


.. attribute:: TestResult.errors

   A list containing 2-tuples of :class:`TestCase` instances and strings holding
   formatted tracebacks. Each tuple represents a test which raised an unexpected
   exception.


.. attribute:: TestResult.failures

   A list containing 2-tuples of :class:`TestCase` instances and strings holding
   formatted tracebacks. Each tuple represents a test where a failure was
   explicitly signalled using the :meth:`TestCase.fail\*` or
   :meth:`TestCase.assert\*` methods.


.. attribute:: TestResult.testsRun

   The total number of tests run so far.


.. method:: TestResult.wasSuccessful()

   Returns :const:`True` if all tests run so far have passed, otherwise returns
   :const:`False`.


.. method:: TestResult.stop()

   This method can be called to signal that the set of tests being run should be
   aborted by setting the :class:`TestResult`'s ``shouldStop`` attribute to
   :const:`True`.  :class:`TestRunner` objects should respect this flag and return
   without running any additional tests.

   For example, this feature is used by the :class:`TextTestRunner` class to stop
   the test framework when the user signals an interrupt from the keyboard.
   Interactive tools which provide :class:`TestRunner` implementations can use this
   in a similar manner.

The following methods of the :class:`TestResult` class are used to maintain the
internal data structures, and may be extended in subclasses to support
additional reporting requirements.  This is particularly useful in building
tools which support interactive reporting while tests are being run.


.. method:: TestResult.startTest(test)

   Called when the test case *test* is about to be run.

   The default implementation simply increments the instance's ``testsRun``
   counter.


.. method:: TestResult.stopTest(test)

   Called after the test case *test* has been executed, regardless of the outcome.

   The default implementation does nothing.


.. method:: TestResult.addError(test, err)

   Called when the test case *test* raises an unexpected exception *err* is a tuple
   of the form returned by :func:`sys.exc_info`: ``(type, value, traceback)``.

   The default implementation appends a tuple ``(test, formatted_err)`` to the
   instance's ``errors`` attribute, where *formatted_err* is a formatted
   traceback derived from *err*.


.. method:: TestResult.addFailure(test, err)

   Called when the test case *test* signals a failure. *err* is a tuple of the form
   returned by :func:`sys.exc_info`:  ``(type, value, traceback)``.

   The default implementation appends a tuple ``(test, formatted_err)`` to the
   instance's ``failures`` attribute, where *formatted_err* is a formatted
   traceback derived from *err*.


.. method:: TestResult.addSuccess(test)

   Called when the test case *test* succeeds.

   The default implementation does nothing.


.. _testloader-objects:

TestLoader Objects
------------------

The :class:`TestLoader` class is used to create test suites from classes and
modules.  Normally, there is no need to create an instance of this class; the
:mod:`unittest` module provides an instance that can be shared as
``unittest.defaultTestLoader``. Using a subclass or instance, however, allows
customization of some configurable properties.

:class:`TestLoader` objects have the following methods:


.. method:: TestLoader.loadTestsFromTestCase(testCaseClass)

   Return a suite of all tests cases contained in the :class:`TestCase`\ -derived
   :class:`testCaseClass`.


.. method:: TestLoader.loadTestsFromModule(module)

   Return a suite of all tests cases contained in the given module. This method
   searches *module* for classes derived from :class:`TestCase` and creates an
   instance of the class for each test method defined for the class.

   .. warning::

      While using a hierarchy of :class:`TestCase`\ -derived classes can be convenient
      in sharing fixtures and helper functions, defining test methods on base classes
      that are not intended to be instantiated directly does not play well with this
      method.  Doing so, however, can be useful when the fixtures are different and
      defined in subclasses.


.. method:: TestLoader.loadTestsFromName(name[, module])

   Return a suite of all tests cases given a string specifier.

   The specifier *name* is a "dotted name" that may resolve either to a module, a
   test case class, a test method within a test case class, a :class:`TestSuite`
   instance, or a callable object which returns a :class:`TestCase` or
   :class:`TestSuite` instance.  These checks are applied in the order listed here;
   that is, a method on a possible test case class will be picked up as "a test
   method within a test case class", rather than "a callable object".

   For example, if you have a module :mod:`SampleTests` containing a
   :class:`TestCase`\ -derived class :class:`SampleTestCase` with three test
   methods (:meth:`test_one`, :meth:`test_two`, and :meth:`test_three`), the
   specifier ``'SampleTests.SampleTestCase'`` would cause this method to return a
   suite which will run all three test methods.  Using the specifier
   ``'SampleTests.SampleTestCase.test_two'`` would cause it to return a test suite
   which will run only the :meth:`test_two` test method.  The specifier can refer
   to modules and packages which have not been imported; they will be imported as a
   side-effect.

   The method optionally resolves *name* relative to the given *module*.


.. method:: TestLoader.loadTestsFromNames(names[, module])

   Similar to :meth:`loadTestsFromName`, but takes a sequence of names rather than
   a single name.  The return value is a test suite which supports all the tests
   defined for each name.


.. method:: TestLoader.getTestCaseNames(testCaseClass)

   Return a sorted sequence of method names found within *testCaseClass*; this
   should be a subclass of :class:`TestCase`.

The following attributes of a :class:`TestLoader` can be configured either by
subclassing or assignment on an instance:


.. attribute:: TestLoader.testMethodPrefix

   String giving the prefix of method names which will be interpreted as test
   methods.  The default value is ``'test'``.

   This affects :meth:`getTestCaseNames` and all the :meth:`loadTestsFrom\*`
   methods.


.. attribute:: TestLoader.sortTestMethodsUsing

   Function to be used to compare method names when sorting them in
   :meth:`getTestCaseNames` and all the :meth:`loadTestsFrom\*` methods. The
   default value is the built-in :func:`cmp` function; the attribute can also be
   set to :const:`None` to disable the sort.


.. attribute:: TestLoader.suiteClass

   Callable object that constructs a test suite from a list of tests. No methods on
   the resulting object are needed.  The default value is the :class:`TestSuite`
   class.

   This affects all the :meth:`loadTestsFrom\*` methods.

