
import netrc, os, tempfile, test_support, unittest

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

temp_filename = tempfile.mktemp()

class NetrcTestCase(unittest.TestCase):

    def setUp (self):
        fp = open(temp_filename, 'wt')
        fp.write(TEST_NETRC)
        fp.close()
        self.netrc = netrc.netrc(temp_filename)

    def tearDown (self):
        del self.netrc
        os.unlink(temp_filename)

    def test_case_1(self):
        self.assert_(self.netrc.macros == {'macro1':['line1\n', 'line2\n'],
                                           'macro2':['line3\n', 'line4\n']}
                                           )
        self.assert_(self.netrc.hosts['foo'] == ('log1', 'acct1', 'pass1'))
        self.assert_(self.netrc.hosts['default'] == ('log2', None, 'pass2'))


if __name__ == "__main__":
    test_support.run_unittest(NetrcTestCase)
