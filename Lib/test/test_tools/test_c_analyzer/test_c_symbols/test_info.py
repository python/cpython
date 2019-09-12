import string
import unittest

from ..util import PseudoStr, StrProxy, Object
from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_analyzer_common.info import ID
    from c_symbols.info import Symbol


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
