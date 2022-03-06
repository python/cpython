import string
import unittest

from ..util import PseudoStr, StrProxy, Object
from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_analyzer.common.info import UNKNOWN, ID
    from c_analyzer.variables.info import (
            normalize_vartype, Variable
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


class VariableTests(unittest.TestCase):

    VALID_ARGS = (
            ('x/y/z/spam.c', 'func', 'eggs'),
            'static',
            'int',
            )
    VALID_KWARGS = dict(zip(Variable._fields, VALID_ARGS))
    VALID_EXPECTED = VALID_ARGS

    def test_init_typical_global(self):
        for storage in ('static', 'extern', 'implicit'):
            with self.subTest(storage):
                static = Variable(
                        id=ID(
                            filename='x/y/z/spam.c',
                            funcname=None,
                            name='eggs',
                            ),
                        storage=storage,
                        vartype='int',
                        )

                self.assertEqual(static, (
                        ('x/y/z/spam.c', None, 'eggs'),
                        storage,
                        'int',
                        ))

    def test_init_typical_local(self):
        for storage in ('static', 'local'):
            with self.subTest(storage):
                static = Variable(
                        id=ID(
                            filename='x/y/z/spam.c',
                            funcname='func',
                            name='eggs',
                            ),
                        storage=storage,
                        vartype='int',
                        )

        self.assertEqual(static, (
                ('x/y/z/spam.c', 'func', 'eggs'),
                storage,
                'int',
                ))

    def test_init_all_missing(self):
        for value in ('', None):
            with self.subTest(repr(value)):
                static = Variable(
                        id=value,
                        storage=value,
                        vartype=value,
                        )

                self.assertEqual(static, (
                        None,
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
                 storage=PseudoStr('static'),
                 vartype=PseudoStr('int'),
                 ),
             (id,
              'static',
              'int',
              )),
            ('non-str 1',
             dict(
                 id=id,
                 storage=Object(),
                 vartype=Object(),
                 ),
             (id,
              '<object>',
              '<object>',
              )),
            ('non-str 2',
             dict(
                 id=id,
                 storage=StrProxy('static'),
                 vartype=StrProxy('variable'),
                 ),
             (id,
              'static',
              'variable',
              )),
            ('non-str',
             dict(
                 id=id,
                 storage=('a', 'b', 'c'),
                 vartype=('x', 'y', 'z'),
                 ),
             (id,
              "('a', 'b', 'c')",
              "('x', 'y', 'z')",
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

        id, storage, vartype = static

        values = (id, storage, vartype)
        for value, expected in zip(values, self.VALID_EXPECTED):
            self.assertEqual(value, expected)

    def test_fields(self):
        static = Variable(('a', 'b', 'z'), 'x', 'y')

        self.assertEqual(static.id, ('a', 'b', 'z'))
        self.assertEqual(static.storage, 'x')
        self.assertEqual(static.vartype, 'y')

    def test___getattr__(self):
        static = Variable(('a', 'b', 'z'), 'x', 'y')

        self.assertEqual(static.filename, 'a')
        self.assertEqual(static.funcname, 'b')
        self.assertEqual(static.name, 'z')

    def test_validate_typical(self):
        validstorage = ('static', 'extern', 'implicit', 'local')
        self.assertEqual(set(validstorage), set(Variable.STORAGE))

        for storage in validstorage:
            with self.subTest(storage):
                static = Variable(
                        id=ID(
                            filename='x/y/z/spam.c',
                            funcname='func',
                            name='eggs',
                            ),
                        storage=storage,
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
        for field in ('storage', 'vartype'):
            with self.subTest(field):
                static = Variable(**self.VALID_KWARGS)
                static = static._replace(**{field: UNKNOWN})

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
            ('storage', ('external', 'global') + notnames),
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
            if field == 'id':
                continue
            valid = seen - set(invalid)
            for value in valid:
                with self.subTest(f'{field}={value!r}'):
                    static = Variable(**self.VALID_KWARGS)
                    static = static._replace(**{field: value})

                    static.validate()  # This does not fail.
