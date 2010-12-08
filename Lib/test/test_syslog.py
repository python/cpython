
from test import support
syslog = support.import_module("syslog") #skip if not supported
import unittest

# XXX(nnorwitz): This test sucks.  I don't know of a platform independent way
# to verify that the messages were really logged.
# The only purpose of this test is to verify the code doesn't crash or leak.

class Test(unittest.TestCase):

    def test_openlog(self):
        syslog.openlog('python')
        # Issue #6697.
        self.assertRaises(UnicodeEncodeError, syslog.openlog, '\uD800')

    def test_syslog(self):
        syslog.openlog('python')
        syslog.syslog('test message from python test_syslog')
        syslog.syslog(syslog.LOG_ERR, 'test error from python test_syslog')

    def test_closelog(self):
        syslog.openlog('python')
        syslog.closelog()

    def test_setlogmask(self):
        syslog.setlogmask(syslog.LOG_DEBUG)

    def test_log_mask(self):
        syslog.LOG_MASK(syslog.LOG_INFO)

    def test_log_upto(self):
        syslog.LOG_UPTO(syslog.LOG_INFO)

def test_main():
    support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
