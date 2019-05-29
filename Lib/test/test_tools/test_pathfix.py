import unittest
from test.test_tools import import_tool


class TestPathFix(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.pathfix = import_tool('pathfix')

        cls.test_cases = [
            (b'#!/usr/bin/python', b'#! /usr/bin/python -f\n'),
            (b'#!python -f', b'#! /usr/bin/python -f\n'),
            (b'#! /usr/bin/python -s', b'#! /usr/bin/python -fs\n'),
            (b'#!/usr/bin/python -f sfj', b'#! /usr/bin/python -f sfj\n'),
            (b'#!/usr/python -s sfj', b'#! /usr/bin/python -fs sfj\n'),
        ]

        cls.test_cases_for_parsing = [
            (b'#!/usr/bin/python', (b'', b'')),
            (b'#! /usr/bin/python -f', (b'f',b'')),
            (b'#!/usr/bin/python -f sfj', (b'f', b'sfj')),
            (b'#!/usr/bin/python -f sfj af bg', (b'f', b'sfj af bg')),
        ]

        cls.test_cases_without_f_option = [
            b'#!/usr/bin/python',
            b'#!python -f',
            b'#! /usr/bin/python -s',
            b'#!/usr/bin/python -f sfj',
            b'#!/usr/python -s sfj',
        ]

    def test_parse_shebangs(self):
        for line, output in self.test_cases_for_parsing:
            self.assertEqual(self.pathfix.parse_shebang(line), output)

    def test_fixline(self):
        for line, output in self.test_cases:
            self.pathfix.add_flag = b'f'
            self.pathfix.new_interpreter = b'/usr/bin/python'
            testing_output = self.pathfix.fixline(line)
            self.assertEqual(testing_output, output)

    def test_fixline_without_f_option(self):
        for line in self.test_cases_without_f_option:
            self.pathfix.new_interpreter = b'/usr/bin/python'
            self.pathfix.add_flag = None
            testing_output = self.pathfix.fixline(line)
            self.assertEqual(testing_output, b'#! ' + self.pathfix.new_interpreter + b'\n')

if __name__ == '__main__':
    unittest.main()