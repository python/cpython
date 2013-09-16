
import netrc, os, unittest, sys
from test import test_support

TEST_NETRC = """
machine foo login log1 password pass1 account acct1

macdef macro1
line1
line2

macdef macro2
line3
line4

default login log2 password pass2

"""

temp_filename = test_support.TESTFN

class NetrcTestCase(unittest.TestCase):

    def setUp (self):
        mode = 'w'
        if sys.platform not in ['cygwin']:
            mode += 't'
        fp = open(temp_filename, mode)
        fp.write(TEST_NETRC)
        fp.close()
        self.netrc = netrc.netrc(temp_filename)

    def tearDown (self):
        del self.netrc
        test_support.unlink(temp_filename)

    def test_case_1(self):
        self.assert_(self.netrc.macros == {'macro1':['line1\n', 'line2\n'],
                                           'macro2':['line3\n', 'line4\n']}
                                           )
        self.assert_(self.netrc.hosts['foo'] == ('log1', 'acct1', 'pass1'))
        self.assert_(self.netrc.hosts['default'] == ('log2', None, 'pass2'))

    if os.name == 'posix':
        def test_security(self):
            # This test is incomplete since we are normally not run as root and
            # therefore can't test the file ownership being wrong.
            os.unlink(temp_filename)
            d = test_support.TESTFN
            try:
                os.mkdir(d)
                fn = os.path.join(d, '.netrc')
                with open(fn, 'wt') as f:
                    f.write(TEST_NETRC)
                with test_support.EnvironmentVarGuard() as environ:
                    environ.set('HOME', d)
                    os.chmod(fn, 0600)
                    self.netrc = netrc.netrc()
                    self.test_case_1()
                    os.chmod(fn, 0622)
                    self.assertRaises(netrc.NetrcParseError, netrc.netrc)
            finally:
                test_support.rmtree(d)

def test_main():
    test_support.run_unittest(NetrcTestCase)

if __name__ == "__main__":
    test_main()
