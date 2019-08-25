import unittest, os, shutil, subprocess
from test.test_tools import import_tool, scriptsdir
from tempfile import mkdtemp
import sys

class TestPathfixFunctional(unittest.TestCase):
    script = os.path.join(scriptsdir, 'pathfix.py')

    def setUp(self):
        self.temp_dir = mkdtemp()
        self.temp_file = os.path.join(self.temp_dir, 'script')

        with open(self.temp_file, 'w') as f:
            f.write('#! /usr/bin/env python -R\n' + 'print("Hello world")\n')

    def tearDown(self):
        shutil.rmtree(self.temp_dir)

    def test_pathfix_keeping_flags(self):
        subprocess.call([sys.executable, self.script, '-i', '/usr/bin/python', '-k', self.temp_file])
        with open(self.temp_file) as f:
            output = f.read()
        self.assertEqual(output, '#! /usr/bin/python -R\n' + 'print("Hello world")\n')
        with open(self.temp_file + '~') as f:
            output = f.read()
        self.assertEqual(output, '#! /usr/bin/env python -R\n' + 'print("Hello world")\n')

    def test_pathfix_without_keeping_flags(self):
        subprocess.call([sys.executable, self.script, '-i', '/usr/bin/python', self.temp_file])
        with open(self.temp_file) as f:
            output = f.read()
        self.assertEqual(output, '#! /usr/bin/python\n' + 'print("Hello world")\n')
        with open(self.temp_file + '~') as f:
            output = f.read()
        self.assertEqual(output, '#! /usr/bin/env python -R\n' + 'print("Hello world")\n')


if __name__ == '__main__':
    unittest.main()
