#!/usr/bin/env python
"""
Python unit testing framework, based on Erich Gamma's JUnit and Kent Beck's
Smalltalk testing framework.

Further information is available in the bundled documentation, and from

  http://pyunit.sourceforge.net/

This module contains the core framework classes that form the basis of
specific test cases and suites (TestCase, TestSuite etc.), and also a
text-based utility class for running the tests and reporting the results
(TextTestRunner).

Copyright (c) 1999, 2000, 2001 Steve Purcell
This module is free software, and you may redistribute it and/or modify
it under the same terms as Python itself, so long as this copyright message
and disclaimer are retained in their original form.

IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF
THIS CODE, EVEN IF THE AUTHOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE.  THE CODE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS,
AND THERE IS NO OBLIGATION WHATSOEVER TO PROVIDE MAINTENANCE,
SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
"""

__author__ = "Steve Purcell (stephen_purcell@yahoo.com)"
__version__ = "$Revision$"[11:-2]

import time
import sys
import traceback
import string
import os

##############################################################################
# A platform-specific concession to help the code work for JPython users
##############################################################################

plat = string.lower(sys.platform)
_isJPython = string.find(plat, 'java') >= 0 or string.find(plat, 'jdk') >= 0
del plat


##############################################################################
# Test framework core
##############################################################################

class TestResult:
    """Holder for test result information.

    Test results are automatically managed by the TestCase and TestSuite
    classes, and do not need to be explicitly manipulated by writers of tests.

    Each instance holds the total number of tests run, and collections of
    failures and errors that occurred among those test runs. The collections
    contain tuples of (testcase, exceptioninfo), where exceptioninfo is a
    tuple of values as returned by sys.exc_info().
    """
    def __init__(self):
        self.failures = []
        self.errors = []
        self.testsRun = 0
        self.shouldStop = 0

    def startTest(self, test):
        "Called when the given test is about to be run"
        self.testsRun = self.testsRun + 1

    def stopTest(self, test):
        "Called when the given test has been run"
        pass

    def addError(self, test, err):
        "Called when an error has occurred"
        self.errors.append((test, err))

    def addFailure(self, test, err):
        "Called when a failure has occurred"
        self.failures.append((test, err))

    def wasSuccessful(self):
        "Tells whether or not this result was a success"
        return len(self.failures) == len(self.errors) == 0

    def stop(self):
        "Indicates that the tests should be aborted"
        self.shouldStop = 1
    
    def __repr__(self):
        return "<%s run=%i errors=%i failures=%i>" % \
               (self.__class__, self.testsRun, len(self.errors),
                len(self.failures))


class TestCase:
    """A class whose instances are single test cases.

    Test authors should subclass TestCase for their own tests. Construction 
    and deconstruction of the test's environment ('fixture') can be
    implemented by overriding the 'setUp' and 'tearDown' methods respectively.

    By default, the test code itself should be placed in a method named
    'runTest'.
    
    If the fixture may be used for many test cases, create as 
    many test methods as are needed. When instantiating such a TestCase
    subclass, specify in the constructor arguments the name of the test method
    that the instance is to execute.
    """
    def __init__(self, methodName='runTest'):
        """Create an instance of the class that will use the named test
           method when executed. Raises a ValueError if the instance does
           not have a method with the specified name.
        """
        try:
            self.__testMethod = getattr(self,methodName)
        except AttributeError:
            raise ValueError, "no such test method in %s: %s" % \
                  (self.__class__, methodName)

    def setUp(self):
        "Hook method for setting up the test fixture before exercising it."
        pass

    def tearDown(self):
        "Hook method for deconstructing the test fixture after testing it."
        pass

    def countTestCases(self):
        return 1

    def defaultTestResult(self):
        return TestResult()

    def shortDescription(self):
        """Returns a one-line description of the test, or None if no
        description has been provided.

        The default implementation of this method returns the first line of
        the specified test method's docstring.
        """
        doc = self.__testMethod.__doc__
        return doc and string.strip(string.split(doc, "\n")[0]) or None

    def id(self):
        return "%s.%s" % (self.__class__, self.__testMethod.__name__)

    def __str__(self):
        return "%s (%s)" % (self.__testMethod.__name__, self.__class__)

    def __repr__(self):
        return "<%s testMethod=%s>" % \
               (self.__class__, self.__testMethod.__name__)

    def run(self, result=None):
        return self(result)

    def __call__(self, result=None):
        if result is None: result = self.defaultTestResult()
        result.startTest(self)
        try:
            try:
                self.setUp()
            except:
                result.addError(self,self.__exc_info())
                return

            try:
                self.__testMethod()
            except AssertionError, e:
                result.addFailure(self,self.__exc_info())
            except:
                result.addError(self,self.__exc_info())

            try:
                self.tearDown()
            except:
                result.addError(self,self.__exc_info())
        finally:
            result.stopTest(self)

    def debug(self):
        self.setUp()
        self.__testMethod()
        self.tearDown()

    def assert_(self, expr, msg=None):
        """Equivalent of built-in 'assert', but is not optimised out when
           __debug__ is false.
        """
        if not expr:
            raise AssertionError, msg

    failUnless = assert_

    def failIf(self, expr, msg=None):
        "Fail the test if the expression is true."
        apply(self.assert_,(not expr,msg))

    def assertRaises(self, excClass, callableObj, *args, **kwargs):
        """Assert that an exception of class excClass is thrown
           by callableObj when invoked with arguments args and keyword
           arguments kwargs. If a different type of exception is
           thrown, it will not be caught, and the test case will be
           deemed to have suffered an error, exactly as for an
           unexpected exception.
        """
        try:
            apply(callableObj, args, kwargs)
        except excClass:
            return
        else:
            if hasattr(excClass,'__name__'): excName = excClass.__name__
            else: excName = str(excClass)
            raise AssertionError, excName

    def fail(self, msg=None):
        """Fail immediately, with the given message."""
        raise AssertionError, msg
                                   
    def __exc_info(self):
        """Return a version of sys.exc_info() with the traceback frame
           minimised; usually the top level of the traceback frame is not
           needed.
        """
        exctype, excvalue, tb = sys.exc_info()
        newtb = tb.tb_next
        if newtb is None:
            return (exctype, excvalue, tb)
        return (exctype, excvalue, newtb)


class TestSuite:
    """A test suite is a composite test consisting of a number of TestCases.

    For use, create an instance of TestSuite, then add test case instances.
    When all tests have been added, the suite can be passed to a test
    runner, such as TextTestRunner. It will run the individual test cases
    in the order in which they were added, aggregating the results. When
    subclassing, do not forget to call the base class constructor.
    """
    def __init__(self, tests=()):
        self._tests = []
        self.addTests(tests)

    def __repr__(self):
        return "<%s tests=%s>" % (self.__class__, self._tests)

    __str__ = __repr__

    def countTestCases(self):
        cases = 0
        for test in self._tests:
            cases = cases + test.countTestCases()
        return cases

    def addTest(self, test):
        self._tests.append(test)

    def addTests(self, tests):
        for test in tests:
            self.addTest(test)

    def run(self, result):
        return self(result)

    def __call__(self, result):
        for test in self._tests:
            if result.shouldStop:
                break
            test(result)
        return result

    def debug(self):
        for test in self._tests: test.debug()
        


class FunctionTestCase(TestCase):
    """A test case that wraps a test function.

    This is useful for slipping pre-existing test functions into the
    PyUnit framework. Optionally, set-up and tidy-up functions can be
    supplied. As with TestCase, the tidy-up ('tearDown') function will
    always be called if the set-up ('setUp') function ran successfully.
    """

    def __init__(self, testFunc, setUp=None, tearDown=None,
                 description=None):
        TestCase.__init__(self)
        self.__setUpFunc = setUp
        self.__tearDownFunc = tearDown
        self.__testFunc = testFunc
        self.__description = description

    def setUp(self):
        if self.__setUpFunc is not None:
            self.__setUpFunc()

    def tearDown(self):
        if self.__tearDownFunc is not None:
            self.__tearDownFunc()

    def runTest(self):
        self.__testFunc()

    def id(self):
        return self.__testFunc.__name__

    def __str__(self):
        return "%s (%s)" % (self.__class__, self.__testFunc.__name__)

    def __repr__(self):
        return "<%s testFunc=%s>" % (self.__class__, self.__testFunc)

    def shortDescription(self):
        if self.__description is not None: return self.__description
        doc = self.__testFunc.__doc__
        return doc and string.strip(string.split(doc, "\n")[0]) or None



##############################################################################
# Convenience functions
##############################################################################

def getTestCaseNames(testCaseClass, prefix, sortUsing=cmp):
    """Extracts all the names of functions in the given test case class
       and its base classes that start with the given prefix. This is used
       by makeSuite().
    """
    testFnNames = filter(lambda n,p=prefix: n[:len(p)] == p,
                         dir(testCaseClass))
    for baseclass in testCaseClass.__bases__:
        testFnNames = testFnNames + \
                      getTestCaseNames(baseclass, prefix, sortUsing=None)
    if sortUsing:
        testFnNames.sort(sortUsing)
    return testFnNames


def makeSuite(testCaseClass, prefix='test', sortUsing=cmp):
    """Returns a TestSuite instance built from all of the test functions
       in the given test case class whose names begin with the given
       prefix. The cases are sorted by their function names
       using the supplied comparison function, which defaults to 'cmp'.
    """
    cases = map(testCaseClass,
                getTestCaseNames(testCaseClass, prefix, sortUsing))
    return TestSuite(cases)


def createTestInstance(name, module=None):
    """Finds tests by their name, optionally only within the given module.

    Return the newly-constructed test, ready to run. If the name contains a ':'
    then the portion of the name after the colon is used to find a specific
    test case within the test case class named before the colon.

    Examples:
     findTest('examples.listtests.suite')
        -- returns result of calling 'suite'
     findTest('examples.listtests.ListTestCase:checkAppend')
        -- returns result of calling ListTestCase('checkAppend')
     findTest('examples.listtests.ListTestCase:check-')
        -- returns result of calling makeSuite(ListTestCase, prefix="check")
    """
          
    spec = string.split(name, ':')
    if len(spec) > 2: raise ValueError, "illegal test name: %s" % name
    if len(spec) == 1:
        testName = spec[0]
        caseName = None
    else:
        testName, caseName = spec
    parts = string.split(testName, '.')
    if module is None:
        if len(parts) < 2:
            raise ValueError, "incomplete test name: %s" % name
        constructor = __import__(string.join(parts[:-1],'.'))
        parts = parts[1:]
    else:
        constructor = module
    for part in parts:
        constructor = getattr(constructor, part)
    if not callable(constructor):
        raise ValueError, "%s is not a callable object" % constructor
    if caseName:
        if caseName[-1] == '-':
            prefix = caseName[:-1]
            if not prefix:
                raise ValueError, "prefix too short: %s" % name
            test = makeSuite(constructor, prefix=prefix)
        else:
            test = constructor(caseName)
    else:
        test = constructor()
    if not hasattr(test,"countTestCases"):
        raise TypeError, \
              "object %s found with spec %s is not a test" % (test, name)
    return test


##############################################################################
# Text UI
##############################################################################

class _WritelnDecorator:
    """Used to decorate file-like objects with a handy 'writeln' method"""
    def __init__(self,stream):
        self.stream = stream
        if _isJPython:
            import java.lang.System
            self.linesep = java.lang.System.getProperty("line.separator")
        else:
            self.linesep = os.linesep

    def __getattr__(self, attr):
        return getattr(self.stream,attr)

    def writeln(self, *args):
        if args: apply(self.write, args)
        self.write(self.linesep)

        
class _JUnitTextTestResult(TestResult):
    """A test result class that can print formatted text results to a stream.

    Used by JUnitTextTestRunner.
    """
    def __init__(self, stream):
        self.stream = stream
        TestResult.__init__(self)

    def addError(self, test, error):
        TestResult.addError(self,test,error)
        self.stream.write('E')
        self.stream.flush()
        if error[0] is KeyboardInterrupt:
            self.shouldStop = 1
 
    def addFailure(self, test, error):
        TestResult.addFailure(self,test,error)
        self.stream.write('F')
        self.stream.flush()
 
    def startTest(self, test):
        TestResult.startTest(self,test)
        self.stream.write('.')
        self.stream.flush()

    def printNumberedErrors(self,errFlavour,errors):
        if not errors: return
        if len(errors) == 1:
            self.stream.writeln("There was 1 %s:" % errFlavour)
        else:
            self.stream.writeln("There were %i %ss:" %
                                (len(errors), errFlavour))
        i = 1
        for test,error in errors:
            errString = string.join(apply(traceback.format_exception,error),"")
            self.stream.writeln("%i) %s" % (i, test))
            self.stream.writeln(errString)
            i = i + 1
 
    def printErrors(self):
        self.printNumberedErrors("error",self.errors)

    def printFailures(self):
        self.printNumberedErrors("failure",self.failures)

    def printHeader(self):
        self.stream.writeln()
        if self.wasSuccessful():
            self.stream.writeln("OK (%i tests)" % self.testsRun)
        else:
            self.stream.writeln("!!!FAILURES!!!")
            self.stream.writeln("Test Results")
            self.stream.writeln()
            self.stream.writeln("Run: %i ; Failures: %i ; Errors: %i" %
                                (self.testsRun, len(self.failures),
                                 len(self.errors)))
            
    def printResult(self):
        self.printHeader()
        self.printErrors()
        self.printFailures()


class JUnitTextTestRunner:
    """A test runner class that displays results in textual form.
    
    The display format approximates that of JUnit's 'textui' test runner.
    This test runner may be removed in a future version of PyUnit.
    """
    def __init__(self, stream=sys.stderr):
        self.stream = _WritelnDecorator(stream)

    def run(self, test):
        "Run the given test case or test suite."
        result = _JUnitTextTestResult(self.stream)
        startTime = time.time()
        test(result)
        stopTime = time.time()
        self.stream.writeln()
        self.stream.writeln("Time: %.3fs" % float(stopTime - startTime))
        result.printResult()
        return result


##############################################################################
# Verbose text UI
##############################################################################

class _VerboseTextTestResult(TestResult):
    """A test result class that can print formatted text results to a stream.

    Used by VerboseTextTestRunner.
    """
    def __init__(self, stream, descriptions):
        TestResult.__init__(self)
        self.stream = stream
        self.lastFailure = None
        self.descriptions = descriptions
        
    def startTest(self, test):
        TestResult.startTest(self, test)
        if self.descriptions:
            self.stream.write(test.shortDescription() or str(test))
        else:
            self.stream.write(str(test))
        self.stream.write(" ... ")

    def stopTest(self, test):
        TestResult.stopTest(self, test)
        if self.lastFailure is not test:
            self.stream.writeln("ok")

    def addError(self, test, err):
        TestResult.addError(self, test, err)
        self._printError("ERROR", test, err)
        self.lastFailure = test
        if err[0] is KeyboardInterrupt:
            self.shouldStop = 1

    def addFailure(self, test, err):
        TestResult.addFailure(self, test, err)
        self._printError("FAIL", test, err)
        self.lastFailure = test

    def _printError(self, flavour, test, err):
        errLines = []
        separator1 = "\t" + '=' * 70
        separator2 = "\t" + '-' * 70
        if not self.lastFailure is test:
            self.stream.writeln()
            self.stream.writeln(separator1)
        self.stream.writeln("\t%s" % flavour)
        self.stream.writeln(separator2)
        for line in apply(traceback.format_exception, err):
            for l in string.split(line,"\n")[:-1]:
                self.stream.writeln("\t%s" % l)
        self.stream.writeln(separator1)


class VerboseTextTestRunner:
    """A test runner class that displays results in textual form.
    
    It prints out the names of tests as they are run, errors as they
    occur, and a summary of the results at the end of the test run.
    """
    def __init__(self, stream=sys.stderr, descriptions=1):
        self.stream = _WritelnDecorator(stream)
        self.descriptions = descriptions

    def run(self, test):
        "Run the given test case or test suite."
        result = _VerboseTextTestResult(self.stream, self.descriptions)
        startTime = time.time()
        test(result)
        stopTime = time.time()
        timeTaken = float(stopTime - startTime)
        self.stream.writeln("-" * 78)
        run = result.testsRun
        self.stream.writeln("Ran %d test%s in %.3fs" %
                            (run, run > 1 and "s" or "", timeTaken))
        self.stream.writeln()
        if not result.wasSuccessful():
            self.stream.write("FAILED (")
            failed, errored = map(len, (result.failures, result.errors))
            if failed:
                self.stream.write("failures=%d" % failed)
            if errored:
                if failed: self.stream.write(", ")
                self.stream.write("errors=%d" % errored)
            self.stream.writeln(")")
        else:
            self.stream.writeln("OK")
        return result
        

# Which flavour of TextTestRunner is the default?
TextTestRunner = VerboseTextTestRunner


##############################################################################
# Facilities for running tests from the command line
##############################################################################

class TestProgram:
    """A command-line program that runs a set of tests; this is primarily
       for making test modules conveniently executable.
    """
    USAGE = """\
Usage: %(progName)s [-h|--help] [test[:(casename|prefix-)]] [...]

Examples:
  %(progName)s                               - run default set of tests
  %(progName)s MyTestSuite                   - run suite 'MyTestSuite'
  %(progName)s MyTestCase:checkSomething     - run MyTestCase.checkSomething
  %(progName)s MyTestCase:check-             - run all 'check*' test methods
                                               in MyTestCase
"""
    def __init__(self, module='__main__', defaultTest=None,
                 argv=None, testRunner=None):
        if type(module) == type(''):
            self.module = __import__(module)
            for part in string.split(module,'.')[1:]:
                self.module = getattr(self.module, part)
        else:
            self.module = module
        if argv is None:
            argv = sys.argv
        self.defaultTest = defaultTest
        self.testRunner = testRunner
        self.progName = os.path.basename(argv[0])
        self.parseArgs(argv)
        self.createTests()
        self.runTests()

    def usageExit(self, msg=None):
        if msg: print msg
        print self.USAGE % self.__dict__
        sys.exit(2)

    def parseArgs(self, argv):
        import getopt
        try:
            options, args = getopt.getopt(argv[1:], 'hH', ['help'])
            opts = {}
            for opt, value in options:
                if opt in ('-h','-H','--help'):
                    self.usageExit()
            if len(args) == 0 and self.defaultTest is None:
                raise getopt.error, "No default test is defined."
            if len(args) > 0:
                self.testNames = args
            else:
                self.testNames = (self.defaultTest,)
        except getopt.error, msg:
            self.usageExit(msg)

    def createTests(self):
        tests = []
        for testName in self.testNames:
            tests.append(createTestInstance(testName, self.module))
        self.test = TestSuite(tests)

    def runTests(self):
        if self.testRunner is None:
            self.testRunner = TextTestRunner()
        result = self.testRunner.run(self.test)
        sys.exit(not result.wasSuccessful())    

main = TestProgram


##############################################################################
# Executing this module from the command line
##############################################################################

if __name__ == "__main__":
    main(module=None)
