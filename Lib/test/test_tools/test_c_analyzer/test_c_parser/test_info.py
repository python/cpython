import string
import unittest

from .util import PseudoStr, StrProxy, Object
from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_parser.info import (
        normalize_vartype, ID, Symbol, Variable,
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


class IDTests(unittest.TestCase):

    VALID_ARGS = (
            'x/y/z/spam.c',
            'func',
            'eggs',
            )
    VALID_KWARGS = dict(zip(ID._fields, VALID_ARGS))
    VALID_EXPECTED = VALID_ARGS

    def test_from_raw(self):
        tests = [
            ('', None),
            (None, None),
            ('spam', (None, None, 'spam')),
            (('spam',), (None, None, 'spam')),
            (('x/y/z/spam.c', 'spam'), ('x/y/z/spam.c', None, 'spam')),
            (self.VALID_ARGS, self.VALID_EXPECTED),
            (self.VALID_KWARGS, self.VALID_EXPECTED),
            ]
        for raw, expected in tests:
            with self.subTest(raw):
                id = ID.from_raw(raw)

                self.assertEqual(id, expected)

    def test_minimal(self):
        id = ID(
                filename=None,
                funcname=None,
                name='eggs',
                )

        self.assertEqual(id, (
                None,
                None,
                'eggs',
                ))

    def test_init_typical_global(self):
        id = ID(
                filename='x/y/z/spam.c',
                funcname=None,
                name='eggs',
                )

        self.assertEqual(id, (
                'x/y/z/spam.c',
                None,
                'eggs',
                ))

    def test_init_typical_local(self):
        id = ID(
                filename='x/y/z/spam.c',
                funcname='func',
                name='eggs',
                )

        self.assertEqual(id, (
                'x/y/z/spam.c',
                'func',
                'eggs',
                ))

    def test_init_all_missing(self):
        for value in ('', None):
            with self.subTest(repr(value)):
                id = ID(
                        filename=value,
                        funcname=value,
                        name=value,
                        )

                self.assertEqual(id, (
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
                 ),
             ('x/y/z/spam.c',
              'func',
              'eggs',
              )),
            ('non-str',
             dict(
                 filename=StrProxy('x/y/z/spam.c'),
                 funcname=Object(),
                 name=('a', 'b', 'c'),
                 ),
             ('x/y/z/spam.c',
              '<object>',
              "('a', 'b', 'c')",
              )),
            ]
        for summary, kwargs, expected in tests:
            with self.subTest(summary):
                id = ID(**kwargs)

                for field in ID._fields:
                    value = getattr(id, field)
                    self.assertIs(type(value), str)
                self.assertEqual(tuple(id), expected)

    def test_iterable(self):
        id = ID(**self.VALID_KWARGS)

        filename, funcname, name = id

        values = (filename, funcname, name)
        for value, expected in zip(values, self.VALID_EXPECTED):
            self.assertEqual(value, expected)

    def test_fields(self):
        id = ID('a', 'b', 'z')

        self.assertEqual(id.filename, 'a')
        self.assertEqual(id.funcname, 'b')
        self.assertEqual(id.name, 'z')

    def test_validate_typical(self):
        id = ID(
                filename='x/y/z/spam.c',
                funcname='func',
                name='eggs',
                )

        id.validate()  # This does not fail.

    def test_validate_missing_field(self):
        for field in ID._fields:
            with self.subTest(field):
                id = ID(**self.VALID_KWARGS)
                id = id._replace(**{field: None})

                if field == 'funcname':
                    id.validate()  # The field can be missing (not set).
                    id = id._replace(filename=None)
                    id.validate()  # Both fields can be missing (not set).
                    continue

                with self.assertRaises(TypeError):
                    id.validate()

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
            ]
        seen = set()
        for field, invalid in tests:
            for value in invalid:
                seen.add(value)
                with self.subTest(f'{field}={value!r}'):
                    id = ID(**self.VALID_KWARGS)
                    id = id._replace(**{field: value})

                    with self.assertRaises(ValueError):
                        id.validate()

        for field, invalid in tests:
            valid = seen - set(invalid)
            for value in valid:
                with self.subTest(f'{field}={value!r}'):
                    id = ID(**self.VALID_KWARGS)
                    id = id._replace(**{field: value})

                    id.validate()  # This does not fail.


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
