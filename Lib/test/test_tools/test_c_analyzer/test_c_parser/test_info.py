import string
import unittest

from ..util import PseudoStr, StrProxy, Object
from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_analyzer_common.info import ID
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
            ID('x/y/z/spam.c', 'func', 'eggs'),
            Symbol.KIND.VARIABLE,
            False,
            )
    VALID_KWARGS = dict(zip(Symbol._fields, VALID_ARGS))
    VALID_EXPECTED = VALID_ARGS

    def test_init_typical_binary_local(self):
        id = ID(None, None, 'spam')
        symbol = Symbol(
                id=id,
                kind=Symbol.KIND.VARIABLE,
                external=False,
                )

        self.assertEqual(symbol, (
            id,
            Symbol.KIND.VARIABLE,
            False,
            ))

    def test_init_typical_binary_global(self):
        id = ID('Python/ceval.c', None, 'spam')
        symbol = Symbol(
                id=id,
                kind=Symbol.KIND.VARIABLE,
                external=False,
                )

        self.assertEqual(symbol, (
            id,
            Symbol.KIND.VARIABLE,
            False,
            ))

    def test_init_coercion(self):
        tests = [
            ('str subclass',
             dict(
                 id=PseudoStr('eggs'),
                 kind=PseudoStr('variable'),
                 external=0,
                 ),
             (ID(None, None, 'eggs'),
              Symbol.KIND.VARIABLE,
              False,
              )),
            ('with filename',
             dict(
                 id=('x/y/z/spam.c', 'eggs'),
                 kind=PseudoStr('variable'),
                 external=0,
                 ),
             (ID('x/y/z/spam.c', None, 'eggs'),
              Symbol.KIND.VARIABLE,
              False,
              )),
            ('non-str 1',
             dict(
                 id=('a', 'b', 'c'),
                 kind=StrProxy('variable'),
                 external=0,
                 ),
             (ID('a', 'b', 'c'),
              Symbol.KIND.VARIABLE,
              False,
              )),
            ('non-str 2',
             dict(
                 id=('a', 'b', 'c'),
                 kind=Object(),
                 external=0,
                 ),
             (ID('a', 'b', 'c'),
              '<object>',
              False,
              )),
            ]
        for summary, kwargs, expected in tests:
            with self.subTest(summary):
                symbol = Symbol(**kwargs)

                for field in Symbol._fields:
                    value = getattr(symbol, field)
                    if field == 'external':
                        self.assertIs(type(value), bool)
                    elif field == 'id':
                        self.assertIs(type(value), ID)
                    else:
                        self.assertIs(type(value), str)
                self.assertEqual(tuple(symbol), expected)

    def test_init_all_missing(self):
        id = ID(None, None, 'spam')

        symbol = Symbol(id)

        self.assertEqual(symbol, (
            id,
            Symbol.KIND.VARIABLE,
            None,
            ))

    def test_fields(self):
        id = ID('z', 'x', 'a')

        symbol = Symbol(id, 'b', False)

        self.assertEqual(symbol.id, id)
        self.assertEqual(symbol.kind, 'b')
        self.assertIs(symbol.external, False)

    def test___getattr__(self):
        id = ID('z', 'x', 'a')
        symbol = Symbol(id, 'b', False)

        filename = symbol.filename
        funcname = symbol.funcname
        name = symbol.name

        self.assertEqual(filename, 'z')
        self.assertEqual(funcname, 'x')
        self.assertEqual(name, 'a')

    def test_validate_typical(self):
        id = ID('z', 'x', 'a')

        symbol = Symbol(
                id=id,
                kind=Symbol.KIND.VARIABLE,
                external=False,
                )

        symbol.validate()  # This does not fail.

    def test_validate_missing_field(self):
        for field in Symbol._fields:
            with self.subTest(field):
                symbol = Symbol(**self.VALID_KWARGS)
                symbol = symbol._replace(**{field: None})

                with self.assertRaises(TypeError):
                    symbol.validate()

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
            ('id', notnames),
            ('kind', ('bogus',)),
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
            ('x/y/z/spam.c', 'func', 'eggs'),
            'int',
            )
    VALID_KWARGS = dict(zip(Variable._fields, VALID_ARGS))
    VALID_EXPECTED = VALID_ARGS

    def test_init_typical_global(self):
        static = Variable(
                id=ID(
                    filename='x/y/z/spam.c',
                    funcname=None,
                    name='eggs',
                    ),
                vartype='int',
                )

        self.assertEqual(static, (
                ('x/y/z/spam.c', None, 'eggs'),
                'int',
                ))

    def test_init_typical_local(self):
        static = Variable(
                id=ID(
                    filename='x/y/z/spam.c',
                    funcname='func',
                    name='eggs',
                    ),
                vartype='int',
                )

        self.assertEqual(static, (
                ('x/y/z/spam.c', 'func', 'eggs'),
                'int',
                ))

    def test_init_all_missing(self):
        for value in ('', None):
            with self.subTest(repr(value)):
                static = Variable(
                        id=value,
                        vartype=value,
                        )

                self.assertEqual(static, (
                        None,
                        None,
                        ))

    def test_init_all_coerced(self):
        id = ID('x/y/z/spam.c', 'func', 'spam')
        tests = [
            ('str subclass',
             dict(
                 id=(
                    PseudoStr('x/y/z/spam.c'),
                    PseudoStr('func'),
                    PseudoStr('spam'),
                    ),
                 vartype=PseudoStr('int'),
                 ),
             (id,
              'int',
              )),
            ('non-str 1',
             dict(
                 id=id,
                 vartype=Object(),
                 ),
             (id,
              '<object>',
              )),
            ('non-str 2',
             dict(
                 id=id,
                 vartype=StrProxy('variable'),
                 ),
             (id,
              'variable',
              )),
            ('non-str',
             dict(
                 id=id,
                 vartype=('a', 'b', 'c'),
                 ),
             (id,
              "('a', 'b', 'c')",
              )),
            ]
        for summary, kwargs, expected in tests:
            with self.subTest(summary):
                static = Variable(**kwargs)

                for field in Variable._fields:
                    value = getattr(static, field)
                    if field == 'id':
                        self.assertIs(type(value), ID)
                    else:
                        self.assertIs(type(value), str)
                self.assertEqual(tuple(static), expected)

    def test_iterable(self):
        static = Variable(**self.VALID_KWARGS)

        id, vartype = static

        values = (id, vartype)
        for value, expected in zip(values, self.VALID_EXPECTED):
            self.assertEqual(value, expected)

    def test_fields(self):
        static = Variable(('a', 'b', 'z'), 'x')

        self.assertEqual(static.id, ('a', 'b', 'z'))
        self.assertEqual(static.vartype, 'x')

    def test___getattr__(self):
        static = Variable(('a', 'b', 'z'), 'x')

        self.assertEqual(static.filename, 'a')
        self.assertEqual(static.funcname, 'b')
        self.assertEqual(static.name, 'z')

    def test_validate_typical(self):
        static = Variable(
                id=ID(
                    filename='x/y/z/spam.c',
                    funcname='func',
                    name='eggs',
                    ),
                vartype='int',
                )

        static.validate()  # This does not fail.

    def test_validate_missing_field(self):
        for field in Variable._fields:
            with self.subTest(field):
                static = Variable(**self.VALID_KWARGS)
                static = static._replace(**{field: None})

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
            ('id', ()),  # Any non-empty str is okay.
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
