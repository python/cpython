import string
import unittest

from .util import PseudoStr, StrProxy, Object
from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_parser.info import (
        normalize_vartype, Symbol, Variable,
        )


class NormalizeVartypeTests(unittest.TestCase):

    def test_basic(self):
        tests = [
                (None, None),
                ('', ''),
                ('int', 'int'),
                (PseudoStr('int'), 'int'),
                (StrProxy('int'), 'int'),
                ]
        for vartype, expected in tests:
            with self.subTest(vartype):
                normalized = normalize_vartype(vartype)

                self.assertEqual(normalized, expected)


class SymbolTests(unittest.TestCase):

    VALID_ARGS = (
            'eggs',
            Symbol.KIND.VARIABLE,
            False,
            'x/y/z/spam.c',
            'func',
            'int',
            )
    VALID_KWARGS = dict(zip(Symbol._fields, VALID_ARGS))
    VALID_EXPECTED = VALID_ARGS

    def test_init_typical_binary(self):
        symbol = Symbol(
                name='spam',
                kind=Symbol.KIND.VARIABLE,
                external=False,
                filename='Python/ceval.c',
                )

        self.assertEqual(symbol, (
            'spam',
            Symbol.KIND.VARIABLE,
            False,
            'Python/ceval.c',
            None,
            None,
            ))

    def test_init_typical_source(self):
        symbol = Symbol(
                name='spam',
                kind=Symbol.KIND.VARIABLE,
                external=False,
                filename='Python/ceval.c',
                declaration='static const int',
                )

        self.assertEqual(symbol, (
            'spam',
            Symbol.KIND.VARIABLE,
            False,
            'Python/ceval.c',
            None,
            'static const int',
            ))

    def test_init_coercion(self):
        tests = [
            ('str subclass',
             dict(
                 name=PseudoStr('eggs'),
                 kind=PseudoStr('variable'),
                 external=0,
                 filename=PseudoStr('x/y/z/spam.c'),
                 funcname=PseudoStr('func'),
                 declaration=PseudoStr('int'),
                 ),
             ('eggs',
              Symbol.KIND.VARIABLE,
              False,
              'x/y/z/spam.c',
              'func',
              'int',
              )),
            ('non-str',
             dict(
                 name=('a', 'b', 'c'),
                 kind=StrProxy('variable'),
                 external=0,
                 filename=StrProxy('x/y/z/spam.c'),
                 funcname=StrProxy('func'),
                 declaration=Object(),
                 ),
             ("('a', 'b', 'c')",
              Symbol.KIND.VARIABLE,
              False,
              'x/y/z/spam.c',
              'func',
              '<object>',
              )),
            ]
        for summary, kwargs, expected in tests:
            with self.subTest(summary):
                symbol = Symbol(**kwargs)

                for field in Symbol._fields:
                    value = getattr(symbol, field)
                    if field == 'external':
                        self.assertIs(type(value), bool)
                    else:
                        self.assertIs(type(value), str)
                self.assertEqual(tuple(symbol), expected)

    def test_init_all_missing(self):
        symbol = Symbol('spam')

        self.assertEqual(symbol, (
            'spam',
            Symbol.KIND.VARIABLE,
            None,
            None,
            None,
            None,
            ))

    def test_fields(self):
        static = Symbol('a', 'b', False, 'z', 'x', 'w')

        self.assertEqual(static.name, 'a')
        self.assertEqual(static.kind, 'b')
        self.assertIs(static.external, False)
        self.assertEqual(static.filename, 'z')
        self.assertEqual(static.funcname, 'x')
        self.assertEqual(static.declaration, 'w')

    def test_validate_typical(self):
        symbol = Symbol(
                name='spam',
                kind=Symbol.KIND.VARIABLE,
                external=False,
                filename='Python/ceval.c',
                declaration='static const int',
                )

        symbol.validate()  # This does not fail.

    def test_validate_missing_field(self):
        for field in Symbol._fields:
            with self.subTest(field):
                symbol = Symbol(**self.VALID_KWARGS)
                symbol = symbol._replace(**{field: None})

                if field in ('funcname', 'declaration'):
                    symbol.validate()  # The field can be missing (not set).
                    continue

                with self.assertRaises(TypeError):
                    symbol.validate()
        with self.subTest('combined'):
            symbol = Symbol(**self.VALID_KWARGS)
            symbol = symbol._replace(filename=None, funcname=None)

            symbol.validate()  # The fields together can be missing (not set).

    def test_validate_bad_field(self):
        badch = tuple(c for c in string.punctuation + string.digits)
        notnames = (
                '1a',
                'a.b',
                'a-b',
                '&a',
                'a++',
                ) + badch
        tests = [
            ('name', notnames),
            ('kind', ('bogus',)),
            ('filename', ()),  # Any non-empty str is okay.
            ('funcname', notnames),
            ('declaration', ()),  # Any non-empty str is okay.
            ]
        seen = set()
        for field, invalid in tests:
            for value in invalid:
                if field != 'kind':
                    seen.add(value)
                with self.subTest(f'{field}={value!r}'):
                    symbol = Symbol(**self.VALID_KWARGS)
                    symbol = symbol._replace(**{field: value})

                    with self.assertRaises(ValueError):
                        symbol.validate()

        for field, invalid in tests:
            if field == 'kind':
                continue
            valid = seen - set(invalid)
            for value in valid:
                with self.subTest(f'{field}={value!r}'):
                    symbol = Symbol(**self.VALID_KWARGS)
                    symbol = symbol._replace(**{field: value})

                    symbol.validate()  # This does not fail.


class VariableTests(unittest.TestCase):

    VALID_ARGS = (
            'x/y/z/spam.c',
            'func',
            'eggs',
            'int',
            )
    VALID_KWARGS = dict(zip(Variable._fields, VALID_ARGS))
    VALID_EXPECTED = VALID_ARGS

    def test_init_typical_global(self):
        static = Variable(
                filename='x/y/z/spam.c',
                funcname=None,
                name='eggs',
                vartype='int',
                )

        self.assertEqual(static, (
                'x/y/z/spam.c',
                None,
                'eggs',
                'int',
                ))

    def test_init_typical_local(self):
        static = Variable(
                filename='x/y/z/spam.c',
                funcname='func',
                name='eggs',
                vartype='int',
                )

        self.assertEqual(static, (
                'x/y/z/spam.c',
                'func',
                'eggs',
                'int',
                ))

    def test_coercion_typical(self):
        static = Variable(
                filename='x/y/z/spam.c',
                funcname='func',
                name='eggs',
                vartype='int',
                )

        self.assertEqual(static, (
                'x/y/z/spam.c',
                'func',
                'eggs',
                'int',
                ))

    def test_init_all_missing(self):
        for value in ('', None):
            with self.subTest(repr(value)):
                static = Variable(
                        filename=value,
                        funcname=value,
                        name=value,
                        vartype=value,
                        )

                self.assertEqual(static, (
                        None,
                        None,
                        None,
                        None,
                        ))

    def test_init_all_coerced(self):
        tests = [
            ('str subclass',
             dict(
                 filename=PseudoStr('x/y/z/spam.c'),
                 funcname=PseudoStr('func'),
                 name=PseudoStr('eggs'),
                 vartype=PseudoStr('int'),
                 ),
             ('x/y/z/spam.c',
              'func',
              'eggs',
              'int',
              )),
            ('non-str',
             dict(
                 filename=StrProxy('x/y/z/spam.c'),
                 funcname=StrProxy('func'),
                 name=('a', 'b', 'c'),
                 vartype=Object(),
                 ),
             ('x/y/z/spam.c',
              'func',
              "('a', 'b', 'c')",
              '<object>',
              )),
            ]
        for summary, kwargs, expected in tests:
            with self.subTest(summary):
                static = Variable(**kwargs)

                for field in Variable._fields:
                    value = getattr(static, field)
                    self.assertIs(type(value), str)
                self.assertEqual(tuple(static), expected)

    def test_iterable(self):
        static = Variable(**self.VALID_KWARGS)

        filename, funcname, name, vartype = static

        values = (filename, funcname, name, vartype)
        for value, expected in zip(values, self.VALID_EXPECTED):
            self.assertEqual(value, expected)

    def test_fields(self):
        static = Variable('a', 'b', 'z', 'x')

        self.assertEqual(static.filename, 'a')
        self.assertEqual(static.funcname, 'b')
        self.assertEqual(static.name, 'z')
        self.assertEqual(static.vartype, 'x')

    def test_validate_typical(self):
        static = Variable(
                filename='x/y/z/spam.c',
                funcname='func',
                name='eggs',
                vartype='int',
                )

        static.validate()  # This does not fail.

    def test_validate_missing_field(self):
        for field in Variable._fields:
            with self.subTest(field):
                static = Variable(**self.VALID_KWARGS)
                static = static._replace(**{field: None})

                if field == 'funcname':
                    static.validate()  # The field can be missing (not set).
                    continue

                with self.assertRaises(TypeError):
                    static.validate()

    def test_validate_bad_field(self):
        badch = tuple(c for c in string.punctuation + string.digits)
        notnames = (
                '1a',
                'a.b',
                'a-b',
                '&a',
                'a++',
                ) + badch
        tests = [
            ('filename', ()),  # Any non-empty str is okay.
            ('funcname', notnames),
            ('name', notnames),
            ('vartype', ()),  # Any non-empty str is okay.
            ]
        seen = set()
        for field, invalid in tests:
            for value in invalid:
                seen.add(value)
                with self.subTest(f'{field}={value!r}'):
                    static = Variable(**self.VALID_KWARGS)
                    static = static._replace(**{field: value})

                    with self.assertRaises(ValueError):
                        static.validate()

        for field, invalid in tests:
            valid = seen - set(invalid)
            for value in valid:
                with self.subTest(f'{field}={value!r}'):
                    static = Variable(**self.VALID_KWARGS)
                    static = static._replace(**{field: value})

                    static.validate()  # This does not fail.
