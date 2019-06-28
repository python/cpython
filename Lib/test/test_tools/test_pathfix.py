import unittest, os, shutil, subprocess
from test.test_tools import import_tool, scriptsdir
from unittest.mock import patch
from tempfile import mkdtemp
import sys


class TestPathFixUnit(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.pathfix = import_tool('pathfix')

    test_cases = [
        (b'#!/usr/bin/python', b'#! /usr/bin/python -f\n'),
        (b'#!python -f', b'#! /usr/bin/python -f\n'),
        (b'#! /usr/bin/python -s', b'#! /usr/bin/python -fs\n'),
        (b'#!/usr/bin/python -f sfj', b'#! /usr/bin/python -f sfj\n'),
        (b'#!/usr/python -s sfj', b'#! /usr/bin/python -fs sfj\n'),
    ]

    def test_fixline(self):
        for line, output in self.test_cases:
            with self.subTest(line=line, output=output):
                self.pathfix.add_flag = b'f'
                self.pathfix.new_interpreter = b'/usr/bin/python'
                testing_output = self.pathfix.fixline(line)
                self.assertEqual(testing_output, output)

    test_cases_for_parsing = [
        (b'#!/usr/bin/python', (b'', b'')),
        (b'#! /usr/bin/python -f', (b'f', b'')),
        (b'#!/usr/bin/python -f sfj', (b'f', b'sfj')),
        (b'#!/usr/bin/python -f sfj af bg', (b'f', b'sfj af bg')),
    ]

    def test_parse_shebangs(self):
        for line, output in self.test_cases_for_parsing:
            with self.subTest(line=line, output=output):
                self.assertEqual(self.pathfix.parse_shebang(line), output)

    test_cases_without_f_option = [
        b'#!/usr/bin/python',
        b'#!python -f',
        b'#! /usr/bin/python -s',
        b'#!/usr/bin/python -f sfj',
        b'#!/usr/python -s sfj',
    ]

    def test_fixline_without_f_option(self):
        for line in self.test_cases_without_f_option:
            with self.subTest(line=line):
                self.pathfix.new_interpreter = b'/usr/bin/python'
                self.pathfix.add_flag = None
                testing_output = self.pathfix.fixline(line)
                self.assertEqual(testing_output, b'#! ' + self.pathfix.new_interpreter + b'\n')

    input_file = (
        b"#! /usr/bin/env python -u\n"
        b"import random.randint\n"
        b"print(random.randint(1,7))\n"
    )
    output_file = (
        b"#! /usr/bin/python -Ru\n"
        b"import random.randint\n"
        b"print(random.randint(1,7))\n"
    )
    def test_main(self):

        with open('file', 'wb') as f:
            f.write(self.input_file)

        with self.assertRaises(SystemExit) as cm:
            with patch(f'{__name__}.TestPathFixUnit.pathfix.getopt') as mgetopt:
                mgetopt.getopt.return_value = (
                    [('-i', '/usr/bin/python'), ('-f', 'R')],
                    ['./file'],
                )
                self.pathfix.main()

        self.assertEqual(cm.exception.code, 0)

        with open('file', 'rb') as f:
            self.assertEqual(f.read(), self.output_file)
        os.remove('file')
        os.remove('file~')


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
        subprocess.call([sys.executable, self.script, '-i', '/usr/bin/python', '-f', "", self.temp_file])
        with open(self.temp_file) as f:
            output = f.read()
        self.assertEqual(output, '#! /usr/bin/python -R\n' + 'print("Hello world")\n')
        with open(self.temp_file + '~') as f:
            output = f.read()
        self.assertEqual(output, '#! /usr/bin/env python -R\n' + 'print("Hello world")\n')

    def test_pathfix_keeping_argument(self):
        with open(self.temp_file, 'w') as f:
            f.write('#! /usr/bin/env python -W something\n' + 'print("Hello world")\n')
        subprocess.call([sys.executable, self.script, '-i', '/usr/bin/python', '-f', "", self.temp_file])
        with open(self.temp_file) as f:
            output = f.read()
        self.assertEqual(output, '#! /usr/bin/python -W something\n' + 'print("Hello world")\n')
        with open(self.temp_file + '~') as f:
            output = f.read()
        self.assertEqual(output, '#! /usr/bin/env python -W something\n' + 'print("Hello world")\n')

    def test_pathfix_adding_flag(self):
        subprocess.call([sys.executable, self.script, '-i', '/usr/bin/python', '-f', 's', self.temp_file])
        with open(self.temp_file) as f:
            output = f.read()
        self.assertEqual(output, '#! /usr/bin/python -sR\n' + 'print("Hello world")\n')
        with open(self.temp_file + '~') as f:
            output = f.read()
        self.assertEqual(output, '#! /usr/bin/env python -R\n' + 'print("Hello world")\n')

    def test_pathfix_adding_multiliteral_flag(self):
        result = subprocess.run([sys.executable, self.script, '-i', '/usr/bin/python', '-f', 'OO', self.temp_file],
                                capture_output=True
                                )
        with open(self.temp_file) as f:
            output = f.read()
        self.assertEqual(output, '#! /usr/bin/env python -R\n' + 'print("Hello world")\n')

        self.assertEqual(result.stderr, b'-f: just one literal flags can be added')

    def test_pathfix_replacing_interpreter(self):
        subprocess.call([sys.executable, self.script, '-i', '/usr/bin/python', self.temp_file])
        with open(self.temp_file) as f:
            output = f.read()
        self.assertEqual(output, '#! /usr/bin/python\n' + 'print("Hello world")\n')
        with open(self.temp_file + '~') as f:
            output = f.read()
        self.assertEqual(output, '#! /usr/bin/env python -R\n' + 'print("Hello world")\n')

    def test_pathfix_without_backup(self):
        subprocess.call([sys.executable, self.script, '-i', '/usr/bin/python', '-n', self.temp_file])
        with open(self.temp_file) as f:
            output = f.read()
        self.assertEqual(output, '#! /usr/bin/python\n' + 'print("Hello world")\n')

        assert (self.temp_file + '~') not in os.listdir(self.temp_dir)


if __name__ == '__main__':
    unittest.main()
