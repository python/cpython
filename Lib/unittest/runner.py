"""Running tests"""

import sys
import time
import warnings

from _colorize import get_colors

from . import result
from .case import _SubTest
from .signals import registerResult

__unittest = True


class _WritelnDecorator(object):
    """Used to decorate file-like objects with a handy 'writeln' method"""
    def __init__(self, stream):
        self.stream = stream

    def __getattr__(self, attr):
        if attr in ('stream', '__getstate__'):
            raise AttributeError(attr)
        return getattr(self.stream, attr)

    def writeln(self, arg=None):
        if arg:
            self.write(arg)
        self.write('\n')  # text-mode streams translate to \r\n if needed


class TextTestResult(result.TestResult):
    """A test result class that can print formatted text results to a stream.

    Used by TextTestRunner.
    """
    separator1 = '=' * 70
    separator2 = '-' * 70

    def __init__(self, stream, descriptions, verbosity, *, durations=None):
        """Construct a TextTestResult. Subclasses should accept **kwargs
        to ensure compatibility as the interface changes."""
        super(TextTestResult, self).__init__(stream, descriptions, verbosity)
        self.stream = stream
        self.showAll = verbosity > 1
        self.dots = verbosity == 1
        self.descriptions = descriptions
        self._ansi = get_colors(file=stream)
        self._newline = True
        self.durations = durations

    def getDescription(self, test):
        doc_first_line = test.shortDescription()
        if self.descriptions and doc_first_line:
            return '\n'.join((str(test), doc_first_line))
        else:
            return str(test)

    def startTest(self, test):
        super(TextTestResult, self).startTest(test)
        if self.showAll:
            self.stream.write(self.getDescription(test))
            self.stream.write(" ... ")
            self.stream.flush()
            self._newline = False

    def _write_status(self, test, status):
        is_subtest = isinstance(test, _SubTest)
        if is_subtest or self._newline:
            if not self._newline:
                self.stream.writeln()
            if is_subtest:
                self.stream.write("  ")
            self.stream.write(self.getDescription(test))
            self.stream.write(" ... ")
        self.stream.writeln(status)
        self.stream.flush()
        self._newline = True

    def addSubTest(self, test, subtest, err):
        if err is not None:
            red, reset = self._ansi.RED, self._ansi.RESET
            if self.showAll:
                if issubclass(err[0], subtest.failureException):
                    self._write_status(subtest, f"{red}FAIL{reset}")
                else:
                    self._write_status(subtest, f"{red}ERROR{reset}")
            elif self.dots:
                if issubclass(err[0], subtest.failureException):
                    self.stream.write(f"{red}F{reset}")
                else:
                    self.stream.write(f"{red}E{reset}")
                self.stream.flush()
        super(TextTestResult, self).addSubTest(test, subtest, err)

    def addSuccess(self, test):
        super(TextTestResult, self).addSuccess(test)
        green, reset = self._ansi.GREEN, self._ansi.RESET
        if self.showAll:
            self._write_status(test, f"{green}ok{reset}")
        elif self.dots:
            self.stream.write(f"{green}.{reset}")
            self.stream.flush()

    def addError(self, test, err):
        super(TextTestResult, self).addError(test, err)
        red, reset = self._ansi.RED, self._ansi.RESET
        if self.showAll:
            self._write_status(test, f"{red}ERROR{reset}")
        elif self.dots:
            self.stream.write(f"{red}E{reset}")
            self.stream.flush()

    def addFailure(self, test, err):
        super(TextTestResult, self).addFailure(test, err)
        red, reset = self._ansi.RED, self._ansi.RESET
        if self.showAll:
            self._write_status(test, f"{red}FAIL{reset}")
        elif self.dots:
            self.stream.write(f"{red}F{reset}")
            self.stream.flush()

    def addSkip(self, test, reason):
        super(TextTestResult, self).addSkip(test, reason)
        yellow, reset = self._ansi.YELLOW, self._ansi.RESET
        if self.showAll:
            self._write_status(test, f"{yellow}skipped{reset} {reason!r}")
        elif self.dots:
            self.stream.write(f"{yellow}s{reset}")
            self.stream.flush()

    def addExpectedFailure(self, test, err):
        super(TextTestResult, self).addExpectedFailure(test, err)
        yellow, reset = self._ansi.YELLOW, self._ansi.RESET
        if self.showAll:
            self.stream.writeln(f"{yellow}expected failure{reset}")
            self.stream.flush()
        elif self.dots:
            self.stream.write(f"{yellow}x{reset}")
            self.stream.flush()

    def addUnexpectedSuccess(self, test):
        super(TextTestResult, self).addUnexpectedSuccess(test)
        red, reset = self._ansi.RED, self._ansi.RESET
        if self.showAll:
            self.stream.writeln(f"{red}unexpected success{reset}")
            self.stream.flush()
        elif self.dots:
            self.stream.write(f"{red}u{reset}")
            self.stream.flush()

    def printErrors(self):
        bold_red = self._ansi.BOLD_RED
        red = self._ansi.RED
        reset = self._ansi.RESET
        if self.dots or self.showAll:
            self.stream.writeln()
            self.stream.flush()
        self.printErrorList(f"{red}ERROR{reset}", self.errors)
        self.printErrorList(f"{red}FAIL{reset}", self.failures)
        unexpectedSuccesses = getattr(self, "unexpectedSuccesses", ())
        if unexpectedSuccesses:
            self.stream.writeln(self.separator1)
            for test in unexpectedSuccesses:
                self.stream.writeln(
                    f"{red}UNEXPECTED SUCCESS{bold_red}: "
                    f"{self.getDescription(test)}{reset}"
                )
            self.stream.flush()

    def printErrorList(self, flavour, errors):
        bold_red, reset = self._ansi.BOLD_RED, self._ansi.RESET
        for test, err in errors:
            self.stream.writeln(self.separator1)
            self.stream.writeln(
                f"{flavour}{bold_red}: {self.getDescription(test)}{reset}"
            )
            self.stream.writeln(self.separator2)
            self.stream.writeln("%s" % err)
            self.stream.flush()


class TextTestRunner(object):
    """A test runner class that displays results in textual form.

    It prints out the names of tests as they are run, errors as they
    occur, and a summary of the results at the end of the test run.
    """
    resultclass = TextTestResult

    def __init__(self, stream=None, descriptions=True, verbosity=1,
                 failfast=False, buffer=False, resultclass=None, warnings=None,
                 *, tb_locals=False, durations=None):
        """Construct a TextTestRunner.

        Subclasses should accept **kwargs to ensure compatibility as the
        interface changes.
        """
        if stream is None:
            stream = sys.stderr
        self.stream = _WritelnDecorator(stream)
        self.descriptions = descriptions
        self.verbosity = verbosity
        self.failfast = failfast
        self.buffer = buffer
        self.tb_locals = tb_locals
        self.durations = durations
        self.warnings = warnings
        if resultclass is not None:
            self.resultclass = resultclass

    def _makeResult(self):
        try:
            return self.resultclass(self.stream, self.descriptions,
                                    self.verbosity, durations=self.durations)
        except TypeError:
            # didn't accept the durations argument
            return self.resultclass(self.stream, self.descriptions,
                                    self.verbosity)

    def _printDurations(self, result):
        if not result.collectedDurations:
            return
        ls = sorted(result.collectedDurations, key=lambda x: x[1],
                    reverse=True)
        if self.durations > 0:
            ls = ls[:self.durations]
        self.stream.writeln("Slowest test durations")
        if hasattr(result, 'separator2'):
            self.stream.writeln(result.separator2)
        hidden = False
        for test, elapsed in ls:
            if self.verbosity < 2 and elapsed < 0.001:
                hidden = True
                continue
            self.stream.writeln("%-10s %s" % ("%.3fs" % elapsed, test))
        if hidden:
            self.stream.writeln("\n(durations < 0.001s were hidden; "
                                "use -v to show these durations)")
        else:
            self.stream.writeln("")

    def run(self, test):
        "Run the given test case or test suite."
        result = self._makeResult()
        registerResult(result)
        result.failfast = self.failfast
        result.buffer = self.buffer
        result.tb_locals = self.tb_locals
        with warnings.catch_warnings():
            if self.warnings:
                # if self.warnings is set, use it to filter all the warnings
                warnings.simplefilter(self.warnings)
            start_time = time.perf_counter()
            startTestRun = getattr(result, 'startTestRun', None)
            if startTestRun is not None:
                startTestRun()
            try:
                test(result)
            finally:
                stopTestRun = getattr(result, 'stopTestRun', None)
                if stopTestRun is not None:
                    stopTestRun()
            stop_time = time.perf_counter()
        time_taken = stop_time - start_time
        result.printErrors()
        if self.durations is not None:
            self._printDurations(result)

        if hasattr(result, 'separator2'):
            self.stream.writeln(result.separator2)

        run = result.testsRun
        self.stream.writeln("Ran %d test%s in %.3fs" %
                            (run, run != 1 and "s" or "", time_taken))
        self.stream.writeln()

        expected_fails = unexpected_successes = skipped = 0
        try:
            results = map(len, (result.expectedFailures,
                                result.unexpectedSuccesses,
                                result.skipped))
        except AttributeError:
            pass
        else:
            expected_fails, unexpected_successes, skipped = results

        infos = []
        ansi = get_colors(file=self.stream)
        bold_red = ansi.BOLD_RED
        green = ansi.GREEN
        red = ansi.RED
        reset = ansi.RESET
        yellow = ansi.YELLOW

        if not result.wasSuccessful():
            self.stream.write(f"{bold_red}FAILED{reset}")
            failed, errored = len(result.failures), len(result.errors)
            if failed:
                infos.append(f"{bold_red}failures={failed}{reset}")
            if errored:
                infos.append(f"{bold_red}errors={errored}{reset}")
        elif run == 0 and not skipped:
            self.stream.write(f"{yellow}NO TESTS RAN{reset}")
        else:
            self.stream.write(f"{green}OK{reset}")
        if skipped:
            infos.append(f"{yellow}skipped={skipped}{reset}")
        if expected_fails:
            infos.append(f"{yellow}expected failures={expected_fails}{reset}")
        if unexpected_successes:
            infos.append(
                f"{red}unexpected successes={unexpected_successes}{reset}"
            )
        if infos:
            self.stream.writeln(" (%s)" % (", ".join(infos),))
        else:
            self.stream.write("\n")
        self.stream.flush()
        return result
