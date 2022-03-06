import string
import unittest

from ..util import PseudoStr, StrProxy, Object
from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_analyzer.common.info import (
            UNKNOWN,
            ID,
            )


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
