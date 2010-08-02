# Testing the trace module

from test.support import run_unittest, TESTFN, rmtree, unlink, captured_stdout
import unittest
import trace
import os, sys

class TestCoverage(unittest.TestCase):
    def tearDown(self):
        rmtree(TESTFN)
        unlink(TESTFN)

    def _coverage(self, tracer):
        tracer.run('from test import test_pprint; test_pprint.test_main()')
        r = tracer.results()
        r.write_results(show_missing=True, summary=True, coverdir=TESTFN)

    def test_coverage(self):
        tracer = trace.Trace(trace=0, count=1)
        with captured_stdout() as stdout:
            self._coverage(tracer)
        stdout = stdout.getvalue()
        self.assertTrue("pprint.py" in stdout)
        self.assertTrue("case.py" in stdout)   # from unittest
        files = os.listdir(TESTFN)
        self.assertTrue("pprint.cover" in files)
        self.assertTrue("unittest.case.cover" in files)

    def test_coverage_ignore(self):
        # Ignore all files, nothing should be traced nor printed
        libpath = os.path.normpath(os.path.dirname(os.__file__))
        # sys.prefix does not work when running from a checkout
        tracer = trace.Trace(ignoredirs=[sys.prefix, sys.exec_prefix, libpath],
                             trace=0, count=1)
        with captured_stdout() as stdout:
            self._coverage(tracer)
        self.assertEquals(stdout.getvalue(), "")
        if os.path.exists(TESTFN):
            files = os.listdir(TESTFN)
            self.assertEquals(files, [])


def test_main():
    run_unittest(__name__)

if __name__ == "__main__":
    test_main()
