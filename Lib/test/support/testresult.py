'''Test runner and result class for the regression test suite.

'''

import io
import sys
import time
import traceback
import unittest

import xml.etree.ElementTree as ET

from datetime import datetime

class RegressionTestResult(unittest.TestResult):
    separator1 = '=' * 70 + '\n'
    separator2 = '-' * 70 + '\n'

    def __init__(self, stream, descriptions, verbosity):
        super().__init__(stream, descriptions, verbosity)
        self.stream = stream
        self.descriptions = descriptions
        self.buffer = True
        self.__suite = ET.Element('testsuite')
        self.__suite.set('start', datetime.utcnow().isoformat(' '))

        self.__e = None
        self.__start_time = None
        self.__results = []
        self.__verbose = bool(verbosity)

    @classmethod
    def __getId(cls, test):
        try:
            test_id = test.id
        except AttributeError:
            return str(test)
        try:
            return test_id()
        except TypeError:
            return str(test_id)
        return repr(test)

    def getDescription(self, test):
        doc_first_line = test.shortDescription()
        if self.descriptions and doc_first_line:
            return '\n'.join((str(test), doc_first_line))
        else:
            return str(test)

    def startTest(self, test):
        super().startTest(test)
        self.__e = e = ET.SubElement(self.__suite, 'testcase')
        self.__start_time = time.perf_counter()
        if self.__verbose:
            self.stream.write(f'{self.getDescription(test)} ... ')
            self.stream.flush()

    def _add_result(self, test, capture=False, **args):
        e = self.__e
        self.__e = None
        if e is None:
            return
        e.set('name', args.pop('name', self.__getId(test)))
        e.set('status', args.pop('status', 'run'))
        e.set('result', args.pop('result', 'completed'))
        if self.__start_time:
            e.set('time', f'{time.perf_counter() - self.__start_time:0.6f}')

        if capture:
            stdout = self._stdout_buffer.getvalue().rstrip()
            ET.SubElement(e, 'system-out').text = stdout
            stderr = self._stderr_buffer.getvalue().rstrip()
            ET.SubElement(e, 'system-err').text = stderr

        for k, v in args.items():
            if not k or not v:
                continue
            e2 = ET.SubElement(e, k)
            if hasattr(v, 'items'):
                for k2, v2 in v.items():
                    if k2:
                        e2.set(k2, str(v2))
                    else:
                        e2.text = str(v2)
            else:
                e2.text = str(v)

    def __write(self, c, word):
        if self.__verbose:
            self.stream.write(f'{word}\n')

    @classmethod
    def __makeErrorDict(cls, err_type, err_value, err_tb):
        if isinstance(err_type, type):
            if err_type.__module__ == 'builtins':
                typename = err_type.__name__
            else:
                typename = f'{err_type.__module__}.{err_type.__name__}'
        else:
            typename = repr(err_type)

        # TODO: Limit err_tb
        tb = traceback.format_exception(err_type, err_value, err_tb)
        msg = traceback.format_exception(err_type, err_value, None)

        return {
            'type': typename,
            'message': ''.join(msg),
            '': ''.join(tb),
        }

    def addError(self, test, err):
        self._add_result(test, True, error=self.__makeErrorDict(*err))
        super().addError(test, err)
        self.__write('E', 'ERROR')

    def addExpectedFailure(self, test, err):
        self._add_result(test, True, output=self.__makeErrorDict(*err))
        super().addExpectedFailure(test, err)
        self.__write('x', 'expected failure')

    def addFailure(self, test, err):
        self._add_result(test, True, failure=self.__makeErrorDict(*err))
        super().addFailure(test, err)
        self.__write('F', 'FAIL')

    def addSkip(self, test, reason):
        self._add_result(test, skipped=reason)
        super().addSkip(test, reason)
        self.__write('S', f'skipped {reason!r}')

    def addSuccess(self, test):
        self._add_result(test)
        super().addSuccess(test)
        self.__write('.', 'ok')

    def addUnexpectedSuccess(self, test):
        self._add_result(test, outcome='UNEXPECTED_SUCCESS')
        super().addUnexpectedSuccess(test)
        self.__write('u', 'unexpected success')

    def printErrors(self):
        if self.__verbose:
            self.stream.write('\n')
        self.printErrorList('ERROR', self.errors)
        self.printErrorList('FAIL', self.failures)

    def printErrorList(self, flavor, errors):
        for test, err in errors:
            self.stream.write(self.separator1)
            self.stream.write(f'{flavor}: {self.getDescription(test)}\n')
            self.stream.write(self.separator2)
            self.stream.write('%s\n' % err)

    def get_xml_element(self):
        e = self.__suite
        e.set('tests', str(self.testsRun))
        e.set('errors', str(len(self.errors)))
        e.set('failures', str(len(self.failures)))
        return e

class RegressionTestRunner:
    def __init__(self, stream, verbosity):
        self.result = RegressionTestResult(stream, None, verbosity)

    def run(self, test):
        test(self.result)
        return self.result

if __name__ == '__main__':
    class TestTests(unittest.TestCase):
        def test_pass(self):
            pass

        def test_pass_slow(self):
            time.sleep(1.0)

        def test_fail(self):
            print('stdout', file=sys.stdout)
            print('stderr', file=sys.stderr)
            self.fail('failure message')

        def test_error(self):
            print('stdout', file=sys.stdout)
            print('stderr', file=sys.stderr)
            raise RuntimeError('error message')

    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestTests))
    stream = io.StringIO()
    runner = RegressionTestRunner(sys.stdout, sum(a == '-v' for a in sys.argv))
    result = runner.run(suite)
    print('Output:', stream.getvalue())
    print('XML: ', end='')
    for s in ET.tostringlist(result.get_xml_element()):
        print(s.decode(), end='')
    print()
