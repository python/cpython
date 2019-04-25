import unittest, os, subprocess
from test.test_tools import import_tool, scriptsdir
import sys
from test import support


class TestPathfixFunctional(unittest.TestCase):
    script = os.path.join(scriptsdir, 'pathfix.py')

    @classmethod
    def setupUpClass(cls):
        self.addCleanup(support.unlink, support.TESTFN)

    def setUp(self):
        self.temp_file = support.TESTFN

        with open(self.temp_file, 'w') as f:
            f.write('#! /usr/bin/env python -R\n' + 'print("Hello world")\n')

    def test_pathfix_keeping_flags(self):
        subprocess.call([sys.executable, self.script, '-i', '/usr/bin/python', '-k', '-n', self.temp_file])
        with open(self.temp_file) as f:
            output = f.read()
        self.assertEqual(output, '#! /usr/bin/python -R\n' + 'print("Hello world")\n')

    def test_pathfix_without_keeping_flags(self):
        subprocess.call([sys.executable, self.script, '-i', '/usr/bin/python', '-n', self.temp_file])
        with open(self.temp_file) as f:
            output = f.read()
        self.assertEqual(output, '#! /usr/bin/python\n' + 'print("Hello world")\n')


if __name__ == '__main__':
    unittest.main()
