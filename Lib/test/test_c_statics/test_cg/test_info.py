import string
import unittest

from test.test_c_statics.cg.info import StaticVar


class PseudoStr(str):
    pass

class Proxy:
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return self.value

class Object:
    def __repr__(self):
        return '<object>'


class StaticVarTests(unittest.TestCase):

    VALID_ARGS = (
            'x/y/z/spam.c',
            'func',
            'eggs',
            'int',
            )
    VALID_KWARGS = dict(zip(StaticVar._fields, VALID_ARGS))
    VALID_EXPECTED = VALID_ARGS

    def test_normalize_vartype(self):
        tests = [
                (None, None),
                ('', ''),
                ('int', 'int'),
                (PseudoStr('int'), 'int'),
                (Proxy('int'), 'int'),
                ]
        for vartype, expected in tests:
            with self.subTest(vartype):
                normalized = StaticVar.normalize_vartype(vartype)

                self.assertEqual(normalized, expected)

    def test_init_typical_global(self):
        static = StaticVar(
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
        static = StaticVar(
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
        static = StaticVar(
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
                static = StaticVar(
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
                 filename=Proxy('x/y/z/spam.c'),
                 funcname=Proxy('func'),
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
                static = StaticVar(**kwargs)

                for field in StaticVar._fields:
                    value = getattr(static, field)
                    self.assertIs(type(value), str)
                self.assertEqual(tuple(static), expected)

    def test_iterable(self):
        static = StaticVar(**self.VALID_KWARGS)

        filename, funcname, name, vartype = static

        values = (filename, funcname, name, vartype)
        for value, expected in zip(values, self.VALID_EXPECTED):
            self.assertEqual(value, expected)

    def test_fields(self):
        static = StaticVar('a', 'b', 'z', 'x')

        self.assertEqual(static.filename, 'a')
        self.assertEqual(static.funcname, 'b')
        self.assertEqual(static.name, 'z')
        self.assertEqual(static.vartype, 'x')

    def test_validate_typical(self):
        static = StaticVar(
                filename='x/y/z/spam.c',
                funcname='func',
                name='eggs',
                vartype='int',
                )

        static.validate()  # This does not fail.

    def test_validate_missing_field(self):
        for field in StaticVar._fields:
            with self.subTest(field):
                static = StaticVar(**self.VALID_KWARGS)
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
                    static = StaticVar(**self.VALID_KWARGS)
                    static = static._replace(**{field: value})

                    with self.assertRaises(ValueError):
                        static.validate()

        for field, invalid in tests:
            valid = seen - set(invalid)
            for value in valid:
                with self.subTest(f'{field}={value!r}'):
                    static = StaticVar(**self.VALID_KWARGS)
                    static = static._replace(**{field: value})

                    static.validate()  # This does not fail.
