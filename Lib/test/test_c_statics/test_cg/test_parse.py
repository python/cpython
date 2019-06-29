import textwrap
import unittest

from test.test_c_statics.cg.parse import (
        iter_statements, parse_func, parse_var, parse_compound,
        iter_variables,
        )


class TestCaseBase(unittest.TestCase):

    @property
    def calls(self):
        try:
            return self._calls
        except AttributeError:
            self._calls = []
            return self._calls


class IterStatementsTests(TestCaseBase):

    def test_(self):
        ...


class ParseFuncTests(TestCaseBase):

    def test_(self):
        ...


class ParseVarTests(TestCaseBase):

    def test_(self):
        ...


class ParseCompoundTests(TestCaseBase):

    def test_(self):
        ...


class IterVariablesTests(TestCaseBase):

    maxDiff = None

    _return_iter_source_lines = None
    _return_iter_statements = None
    _return_parse_func = None
    _return_parse_var = None
    _return_parse_compound = None

    def _iter_source_lines(self, filename):
        self.calls.append(
                ('_iter_source_lines', (filename,)))
        return self._return_iter_source_lines.splitlines()

    def _iter_statements(self, lines):
        self.calls.append(
                ('_iter_statements', (lines,)))
        try:
            return self._return_iter_statements.pop(0)
        except IndexError:
            return ('???', False)

    def _parse_func(self, lines):
        self.calls.append(
                ('_parse_func', (lines,)))
        try:
            return self._return_parse_func.pop(0)
        except IndexError:
            return ('???', '???', '???')

    def _parse_var(self, lines):
        self.calls.append(
                ('_parse_var', (lines,)))
        try:
            return self._return_parse_var.pop(0)
        except IndexError:
            return ('???', '???')

    def _parse_compound(self, lines):
        self.calls.append(
                ('_parse_compound', (lines,)))
        try:
            return self._return_parse_compound.pop(0)
        except IndexError:
            return (['???'], ['???'])

    def test_empty_file(self):
        self._return_iter_source_lines = ''
        self._return_iter_statements = [
            [],
            ]
        self._return_parse_func = None
        self._return_parse_var = None
        self._return_parse_compound = None

        srcvars = list(iter_variables('spam.c',
                                      _iter_source_lines=self._iter_source_lines,
                                      _iter_statements=self._iter_statements,
                                      _parse_func=self._parse_func,
                                      _parse_var=self._parse_var,
                                      _parse_compound=self._parse_compound,
                                      ))

        self.assertEqual(srcvars, [])
        self.assertEqual(self.calls, [
            ('_iter_source_lines', ('spam.c',)),
            ('_iter_statements', ([],)),
            ])

    def test_no_statements(self):
        content = textwrap.dedent('''
        ...
        ''')
        self._return_iter_source_lines = content
        self._return_iter_statements = [
            [],
            ]
        self._return_parse_func = None
        self._return_parse_var = None
        self._return_parse_compound = None

        srcvars = list(iter_variables('spam.c',
                                      _iter_source_lines=self._iter_source_lines,
                                      _iter_statements=self._iter_statements,
                                      _parse_func=self._parse_func,
                                      _parse_var=self._parse_var,
                                      _parse_compound=self._parse_compound,
                                      ))

        self.assertEqual(srcvars, [])
        self.assertEqual(self.calls, [
            ('_iter_source_lines', ('spam.c',)),
            ('_iter_statements', (content.splitlines(),)),
            ])

    def test_typical(self):
        content = textwrap.dedent('''
        ...
        ''')
        self._return_iter_source_lines = content
        self._return_iter_statements = [
            [('<lines 1>', True),  # var1
             ('<lines 2>', True),  # non-var
             ('<lines 3>', True),  # var2
             ('<lines 4>', False),  # func1
             ('<lines 9>', True),  # var4
             ],
            # func1
            [('<lines 5>', True),  # var3
             ('<lines 6>', False),  # if
             ('<lines 8>', True),  # non-var
             ],
            # if
            [('<lines 7>', True),  # var2 ("collision" with global var)
             ],
            ]
        self._return_parse_func = [
            ('func1', '<sig 1>', '<body 1>'),
            ]
        self._return_parse_var = [
            ('var1', '<vartype 1>'),
            (None, None),
            ('var2', '<vartype 2>'),
            ('var3', '<vartype 3>'),
            ('var2', '<vartype 2b>'),
            ('var4', '<vartype 4>'),
            (None, None),
            ('var5', '<vartype 5>'),
            ]
        self._return_parse_compound = [
            (['<simple>'], ['<block 1>']),
            ]

        srcvars = list(iter_variables('spam.c',
                                      _iter_source_lines=self._iter_source_lines,
                                      _iter_statements=self._iter_statements,
                                      _parse_func=self._parse_func,
                                      _parse_var=self._parse_var,
                                      _parse_compound=self._parse_compound,
                                      ))

        self.assertEqual(srcvars, [
            (None, 'var1', '<vartype 1>'),
            (None, 'var2', '<vartype 2>'),
            ('func1', 'var3', '<vartype 3>'),
            ('func1', 'var2', '<vartype 2b>'),
            ('func1', 'var4', '<vartype 4>'),
            (None, 'var5', '<vartype 5>'),
            ])
        self.assertEqual(self.calls, [
            ('_iter_source_lines', ('spam.c',)),
            ('_iter_statements', (content.splitlines(),)),
            ('_parse_var', ('<lines 1>',)),
            ('_parse_var', ('<lines 2>',)),
            ('_parse_var', ('<lines 3>',)),
            ('_parse_func', ('<lines 4>',)),
            ('_iter_statements', (['<body 1>'],)),
            ('_parse_var', ('<lines 5>',)),
            ('_parse_compound', ('<lines 6>',)),
            ('_parse_var', ('<simple>',)),
            ('_parse_var', ('<lines 8>',)),
            ('_iter_statements', (['<block 1>'],)),
            ('_parse_var', ('<lines 7>',)),
            ('_parse_var', ('<lines 9>',)),
            ])

    def test_no_locals(self):
        content = textwrap.dedent('''
        ...
        ''')
        self._return_iter_source_lines = content
        self._return_iter_statements = [
            [('<lines 1>', True),  # var1
             ('<lines 2>', True),  # non-var
             ('<lines 3>', True),  # var2
             ('<lines 4>', False),  # func1
             ],
            # func1
            [('<lines 5>', True),  # non-var
             ('<lines 6>', False),  # if
             ('<lines 8>', True),  # non-var
             ],
            # if
            [('<lines 7>', True),  # non-var
             ],
            ]
        self._return_parse_func = [
            ('func1', '<sig 1>', '<body 1>'),
            ]
        self._return_parse_var = [
            ('var1', '<vartype 1>'),
            (None, None),
            ('var2', '<vartype 2>'),
            (None, None),
            (None, None),
            (None, None),
            (None, None),
            ]
        self._return_parse_compound = [
            (['<simple>'], ['<block 1>']),
            ]

        srcvars = list(iter_variables('spam.c',
                                      _iter_source_lines=self._iter_source_lines,
                                      _iter_statements=self._iter_statements,
                                      _parse_func=self._parse_func,
                                      _parse_var=self._parse_var,
                                      _parse_compound=self._parse_compound,
                                      ))

        self.assertEqual(srcvars, [
            (None, 'var1', '<vartype 1>'),
            (None, 'var2', '<vartype 2>'),
            ])
        self.assertEqual(self.calls, [
            ('_iter_source_lines', ('spam.c',)),
            ('_iter_statements', (content.splitlines(),)),
            ('_parse_var', ('<lines 1>',)),
            ('_parse_var', ('<lines 2>',)),
            ('_parse_var', ('<lines 3>',)),
            ('_parse_func', ('<lines 4>',)),
            ('_iter_statements', (['<body 1>'],)),
            ('_parse_var', ('<lines 5>',)),
            ('_parse_compound', ('<lines 6>',)),
            ('_parse_var', ('<simple>',)),
            ('_parse_var', ('<lines 8>',)),
            ('_iter_statements', (['<block 1>'],)),
            ('_parse_var', ('<lines 7>',)),
            ])
