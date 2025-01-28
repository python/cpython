from test.support import import_helper, threading_helper
syslog = import_helper.import_module("syslog") #skip if not supported
from test import support
import sys
import threading
import time
import unittest
from textwrap import dedent

# XXX(nnorwitz): This test sucks.  I don't know of a platform independent way
# to verify that the messages were really logged.
# The only purpose of this test is to verify the code doesn't crash or leak.

class Test(unittest.TestCase):

    def tearDown(self):
        syslog.closelog()

    def test_openlog(self):
        syslog.openlog('python')
        # Issue #6697.
        self.assertRaises(UnicodeEncodeError, syslog.openlog, '\uD800')

    def test_syslog(self):
        syslog.openlog('python')
        syslog.syslog('test message from python test_syslog')
        syslog.syslog(syslog.LOG_ERR, 'test error from python test_syslog')

    def test_syslog_implicit_open(self):
        syslog.closelog() # Make sure log is closed
        syslog.syslog('test message from python test_syslog')
        syslog.syslog(syslog.LOG_ERR, 'test error from python test_syslog')

    def test_closelog(self):
        syslog.openlog('python')
        syslog.closelog()
        syslog.closelog()  # idempotent operation

    def test_setlogmask(self):
        mask = syslog.LOG_UPTO(syslog.LOG_WARNING)
        oldmask = syslog.setlogmask(mask)
        self.assertEqual(syslog.setlogmask(0), mask)
        self.assertEqual(syslog.setlogmask(oldmask), mask)

    def test_log_mask(self):
        mask = syslog.LOG_UPTO(syslog.LOG_WARNING)
        self.assertTrue(mask & syslog.LOG_MASK(syslog.LOG_WARNING))
        self.assertTrue(mask & syslog.LOG_MASK(syslog.LOG_ERR))
        self.assertFalse(mask & syslog.LOG_MASK(syslog.LOG_INFO))

    def test_openlog_noargs(self):
        syslog.openlog()
        syslog.syslog('test message from python test_syslog')

    @threading_helper.requires_working_threading()
    def test_syslog_threaded(self):
        start = threading.Event()
        stop = False
        def opener():
            start.wait(10)
            i = 1
            while not stop:
                syslog.openlog(f'python-test-{i}')  # new string object
                i += 1
        def logger():
            start.wait(10)
            while not stop:
                syslog.syslog('test message from python test_syslog')

        orig_si = sys.getswitchinterval()
        support.setswitchinterval(1e-9)
        try:
            threads = [threading.Thread(target=opener)]
            threads += [threading.Thread(target=logger) for k in range(10)]
            with threading_helper.start_threads(threads):
                start.set()
                time.sleep(0.1)
                stop = True
        finally:
            sys.setswitchinterval(orig_si)

    def test_subinterpreter_syslog(self):
        # syslog.syslog() is not allowed in subinterpreters, but only if
        # syslog.openlog() hasn't been called in the main interpreter yet.
        with self.subTest('before openlog()'):
            code = dedent('''
                import syslog
                caught_error = False
                try:
                    syslog.syslog('foo')
                except RuntimeError:
                    caught_error = True
                assert(caught_error)
            ''')
            res = support.run_in_subinterp(code)
            self.assertEqual(res, 0)

        syslog.openlog()
        try:
            with self.subTest('after openlog()'):
                code = dedent('''
                    import syslog
                    syslog.syslog('foo')
                ''')
                res = support.run_in_subinterp(code)
                self.assertEqual(res, 0)
        finally:
            syslog.closelog()

    def test_subinterpreter_openlog(self):
        try:
            code = dedent('''
                import syslog
                caught_error = False
                try:
                    syslog.openlog()
                except RuntimeError:
                    caught_error = True

                assert(caught_error)
            ''')
            res = support.run_in_subinterp(code)
            self.assertEqual(res, 0)
        finally:
            syslog.closelog()

    def test_subinterpreter_closelog(self):
        syslog.openlog('python')
        try:
            code = dedent('''
                import syslog
                caught_error = False
                try:
                    syslog.closelog()
                except RuntimeError:
                    caught_error = True

                assert(caught_error)
            ''')
            res = support.run_in_subinterp(code)
            self.assertEqual(res, 0)
        finally:
            syslog.closelog()


if __name__ == "__main__":
    unittest.main()
