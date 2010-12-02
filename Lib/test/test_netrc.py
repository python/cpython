
import netrc, os, unittest, sys
from test import support

TEST_NETRC = """

 #this is a comment
#this is a comment
# this is a comment

machine foo login log1 password pass1 account acct1
machine bar login log1 password pass# account acct1

macdef macro1
line1
line2

macdef macro2
line3
line4

default login log2 password pass2

"""

temp_filename = support.TESTFN

class NetrcTestCase(unittest.TestCase):

    def setUp(self):
        mode = 'w'
        if sys.platform not in ['cygwin']:
            mode += 't'
        fp = open(temp_filename, mode)
        fp.write(TEST_NETRC)
        fp.close()
        self.nrc = netrc.netrc(temp_filename)

    def tearDown(self):
        os.unlink(temp_filename)

    def test_case_1(self):
        self.assertEqual(self.nrc.hosts['foo'], ('log1', 'acct1', 'pass1'))
        self.assertEqual(self.nrc.hosts['default'], ('log2', None, 'pass2'))

    def test_macros(self):
        self.assertEqual(self.nrc.macros, {'macro1':['line1\n', 'line2\n'],
                                           'macro2':['line3\n', 'line4\n']})

    def test_parses_passwords_with_hash_character(self):
        self.assertEqual(self.nrc.hosts['bar'], ('log1', 'acct1', 'pass#'))

def test_main():
    support.run_unittest(NetrcTestCase)

if __name__ == "__main__":
    test_main()
