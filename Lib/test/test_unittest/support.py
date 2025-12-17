import unittest


class TestEquality(object):
    """Used as a mixin for TestCase"""

    # Check for a valid __eq__ implementation
    def test_eq(self):
        for obj_1, obj_2 in self.eq_pairs:
            self.assertEqual(obj_1, obj_2)
            self.assertEqual(obj_2, obj_1)

    # Check for a valid __ne__ implementation
    def test_ne(self):
        for obj_1, obj_2 in self.ne_pairs:
            self.assertNotEqual(obj_1, obj_2)
            self.assertNotEqual(obj_2, obj_1)

class TestHashing(object):
    """Used as a mixin for TestCase"""

    # Check for a valid __hash__ implementation
    def test_hash(self):
        for obj_1, obj_2 in self.eq_pairs:
            try:
                if not hash(obj_1) == hash(obj_2):
                    self.fail("%r and %r do not hash equal" % (obj_1, obj_2))
            except Exception as e:
                self.fail("Problem hashing %r and %r: %s" % (obj_1, obj_2, e))

        for obj_1, obj_2 in self.ne_pairs:
            try:
                if hash(obj_1) == hash(obj_2):
                    self.fail("%s and %s hash equal, but shouldn't" %
                              (obj_1, obj_2))
            except Exception as e:
                self.fail("Problem hashing %s and %s: %s" % (obj_1, obj_2, e))


class _BaseLoggingResult(unittest.TestResult):
    def __init__(self, log):
        self._events = log
        super().__init__()

    def startTest(self, test):
        self._events.append('startTest')
        super().startTest(test)

    def startTestRun(self):
        self._events.append('startTestRun')
        super().startTestRun()

    def stopTest(self, test):
        self._events.append('stopTest')
        super().stopTest(test)

    def stopTestRun(self):
        self._events.append('stopTestRun')
        super().stopTestRun()

    def addFailure(self, *args):
        self._events.append('addFailure')
        super().addFailure(*args)

    def addSuccess(self, *args):
        self._events.append('addSuccess')
        super().addSuccess(*args)

    def addError(self, *args):
        self._events.append('addError')
        super().addError(*args)

    def addSkip(self, *args):
        self._events.append('addSkip')
        super().addSkip(*args)

    def addExpectedFailure(self, *args):
        self._events.append('addExpectedFailure')
        super().addExpectedFailure(*args)

    def addUnexpectedSuccess(self, *args):
        self._events.append('addUnexpectedSuccess')
        super().addUnexpectedSuccess(*args)


class LegacyLoggingResult(_BaseLoggingResult):
    """
    A legacy TestResult implementation, without an addSubTest method,
    which records its method calls.
    """

    @property
    def addSubTest(self):
        raise AttributeError


class LoggingResult(_BaseLoggingResult):
    """
    A TestResult implementation which records its method calls.
    """

    def addSubTest(self, test, subtest, err):
        if err is None:
            self._events.append('addSubTestSuccess')
        else:
            self._events.append('addSubTestFailure')
        super().addSubTest(test, subtest, err)


class ResultWithNoStartTestRunStopTestRun(object):
    """An object honouring TestResult before startTestRun/stopTestRun."""

    def __init__(self):
        self.failures = []
        self.errors = []
        self.testsRun = 0
        self.skipped = []
        self.expectedFailures = []
        self.unexpectedSuccesses = []
        self.shouldStop = False

    def startTest(self, test):
        pass

    def stopTest(self, test):
        pass

    def addError(self, test):
        pass

    def addFailure(self, test):
        pass

    def addSuccess(self, test):
        pass

    def wasSuccessful(self):
        return True


class BufferedWriter:
    def __init__(self):
        self.result = ''
        self.buffer = ''

    def write(self, arg):
        self.buffer += arg

    def flush(self):
        self.result += self.buffer
        self.buffer = ''

    def getvalue(self):
        return self.result
