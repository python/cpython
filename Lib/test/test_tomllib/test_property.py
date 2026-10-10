"""Properties of TOML values and table definitions through the public API."""

import datetime
import decimal
import io
import math
import unittest

from test.support.hypothesis_helper import hypothesis

from . import tomllib


def quote(text: str, *, long_escape: bool = False) -> str:
    """Represent scalar characters using only TOML Unicode escapes."""
    # Characters outside the Basic Multilingual Plane need eight hex digits.
    return '"' + ''.join(
        f'\\U{ord(char):08x}' if long_escape or ord(char) > 0xffff
        else f'\\u{ord(char):04x}'
        for char in text
    ) + '"'


class TestProperties(unittest.TestCase):
    @hypothesis.given(
        text=hypothesis.strategies.text(),
        long_escape=hypothesis.strategies.booleans(),
    )
    @hypothesis.example(text='', long_escape=False)
    # Escaped controls and punctuation must not become document delimiters.
    @hypothesis.example(text='\x00\t\n\r"\\.', long_escape=False)
    # Immediately before/after the surrogate range, the last BMP character,
    # the first non-BMP character, and the largest Unicode scalar value.
    @hypothesis.example(text='\ud7ff\ue000\uffff\U00010000\U0010ffff',
                        long_escape=True)
    def test_escaped_strings_and_keys(
        self, text: str, long_escape: bool,
    ) -> None:
        # text() excludes surrogates, which aren't Unicode scalar values.
        token = quote(text, long_escape=long_escape)
        document = f'{token} = {token}'
        expected = {text: text}
        self.assertEqual(tomllib.loads(document), expected)
        self.assertEqual(tomllib.load(io.BytesIO(document.encode())), expected)

    @hypothesis.given(
        codepoint=hypothesis.strategies.one_of(
            # UTF-16 surrogate code points are not Unicode scalar values.
            hypothesis.strategies.integers(0xd800, 0xdfff),
            # Above Unicode's maximum, but still representable by eight hex
            # digits in a syntactically complete TOML escape.
            hypothesis.strategies.integers(0x110000, 0xffffffff),
        ),
    )
    @hypothesis.example(codepoint=0xd800)  # First surrogate code point.
    @hypothesis.example(codepoint=0xdfff)  # Last surrogate code point.
    # First value above Unicode's maximum (U+10FFFF).
    @hypothesis.example(codepoint=0x110000)
    # Largest value representable by an eight-digit escape.
    @hypothesis.example(codepoint=0xffffffff)
    def test_non_scalar_escapes_rejected(self, codepoint: int) -> None:
        token = f'"\\U{codepoint:08x}"'
        for document in (f'key = {token}', f'{token} = 0'):
            with self.assertRaises(tomllib.TOMLDecodeError, msg=document):
                tomllib.loads(document)

    @hypothesis.given(
        # Limit document size, not the alphabet of quoted key components.
        path=hypothesis.strategies.lists(
            hypothesis.strategies.text(max_size=12), min_size=2, max_size=6,
        ).map(tuple),
        value=hypothesis.strategies.integers(),
    )
    @hypothesis.example(path=('', ''), value=0)
    # Dots inside components, repeated names at different depths, and escaped
    # delimiters must not alter the nesting structure.
    @hypothesis.example(path=('a.b', 'a', 'a', '"\n'), value=-1)
    def test_equivalent_table_forms(
        self, path: tuple[str, ...], value: int,
    ) -> None:
        keys = tuple(quote(key) for key in path)
        expected: int | dict[str, object] = value
        inline = str(value)
        for key, token in zip(reversed(path), reversed(keys)):
            expected = {key: expected}
            inline = f'{{{token} = {inline}}}'

        documents = (
            f'{".".join(keys)} = {value}',
            f'[{".".join(keys[:-1])}]\n{keys[-1]} = {value}',
            # Remove only the outermost braces to make a document.
            inline[1:-1],
        )
        for document in documents:
            self.assertEqual(tomllib.loads(document), expected, document)

    @hypothesis.given(key=hypothesis.strategies.text())
    @hypothesis.example(key='')
    @hypothesis.example(key='a.b')
    # The snake is a non-BMP character: both spellings must use a long escape
    # for it, even when surrounding characters use short escapes.
    @hypothesis.example(key='"\\\n\U0001f40d')
    def test_duplicate_keys_rejected(self, key: str) -> None:
        first = quote(key)
        second = quote(key, long_escape=True)
        documents = (
            f'{first} = 0\n{second} = 1',
            f'container = {{{first} = 0, {second} = 1}}',
            f'[container]\n{first} = 0\n{second} = 1',
        )
        for document in documents:
            with self.assertRaises(tomllib.TOMLDecodeError, msg=document):
                tomllib.loads(document)

    @hypothesis.given(key=hypothesis.strategies.text())
    @hypothesis.example(key='')
    # A quoted dot belongs to one key rather than separating two tables.
    @hypothesis.example(key='a.b')
    def test_implicit_parent_can_be_defined_only_once(self, key: str) -> None:
        first = quote(key)
        second = quote(key, long_escape=True)
        document = f'[{first}.child]\nvalue = 0\n[{second}]\nother = 1'
        self.assertEqual(
            tomllib.loads(document),
            {key: {'child': {'value': 0}, 'other': 1}},
        )
        with self.assertRaises(tomllib.TOMLDecodeError):
            tomllib.loads(f'{document}\n[{first}]\nanother = 2')

    @hypothesis.given(key=hypothesis.strategies.text())
    @hypothesis.example(key='')
    @hypothesis.example(key='a.b')
    # Include the lowest and highest Unicode scalar values in a single key.
    @hypothesis.example(key='\x00\U0010ffff')
    def test_inline_tables_cannot_be_extended(self, key: str) -> None:
        token = quote(key)
        for inline in ('{}', '{child = {}}'):
            self.assertEqual(
                tomllib.loads(f'{token} = {inline}'),
                {key: {} if inline == '{}' else {'child': {}}},
            )
            for extension in (
                f'{token}.new = 1',
                f'[{token}]\nnew = 1',
                f'[{token}.new]\nvalue = 1',
                f'[[{token}.new]]\nvalue = 1',
                f'{token}.child.new = 1',
            ):
                document = f'{token} = {inline}\n{extension}'
                with self.assertRaises(tomllib.TOMLDecodeError, msg=document):
                    tomllib.loads(document)

    @hypothesis.given(
        key=hypothesis.strategies.text(),
        values=hypothesis.strategies.lists(
            hypothesis.strategies.integers(), min_size=2, max_size=10,
        ).map(tuple),
    )
    # Distinct values make accidental reuse of an earlier table observable.
    @hypothesis.example(key='', values=(0, 1))
    @hypothesis.example(key='a.b', values=(1, -1, 0))
    def test_array_of_tables_has_independent_children(
        self, key: str, values: tuple[int, ...],
    ) -> None:
        token = quote(key)
        document = '\n'.join(
            f'[[{token}]]\ninline = {{value = {value}}}\n'
            f'[{token}.child]\nvalue = {value}'
            for value in values
        )
        expected = {
            key: [{'inline': {'value': value}, 'child': {'value': value}}
                  for value in values],
        }
        self.assertEqual(tomllib.loads(document), expected)

    @hypothesis.given(value=hypothesis.strategies.integers())
    @hypothesis.example(value=0)
    # TOML recommends support for signed 64-bit integers.
    @hypothesis.example(value=-(2**63))  # Lower bound.
    @hypothesis.example(value=2**63 - 1)  # Upper bound.
    # tomllib also accepts Python integers well beyond the recommended range.
    @hypothesis.example(value=2**128)
    def test_integer_representations(self, value: int) -> None:
        magnitude = str(abs(value))
        sign = '-' if value < 0 else '+'
        tokens = [str(value), sign + '_'.join(magnitude)]
        if value >= 0:
            for prefix, code in (('0b', 'b'), ('0o', 'o'), ('0x', 'x')):
                digits = format(value, code)
                tokens.extend((prefix + digits, prefix + '_'.join(digits)))
        for token in tokens:
            actual = tomllib.loads(f'value = {token}')['value']
            self.assertIs(type(actual), int, token)
            self.assertEqual(actual, value, token)

    @hypothesis.given(value=hypothesis.strategies.floats())
    @hypothesis.example(value=0.0)
    # Ordinary equality cannot distinguish this from positive zero.
    @hypothesis.example(value=-0.0)
    # The smallest positive binary64 subnormal must not underflow to zero.
    @hypothesis.example(value=5e-324)
    @hypothesis.example(value=float('inf'))
    @hypothesis.example(value=float('-inf'))
    @hypothesis.example(value=float('nan'))
    def test_float_representations(self, value: float) -> None:
        token = repr(value)
        document = f'value = {token}'
        actual = tomllib.loads(document)['value']
        self.assertIs(type(actual), float)
        if math.isnan(value):
            self.assertTrue(math.isnan(actual))
        else:
            self.assertEqual(actual, value)
            self.assertEqual(math.copysign(1, actual), math.copysign(1, value))

        precise = tomllib.loads(document, parse_float=decimal.Decimal)['value']
        self.assertIs(type(precise), decimal.Decimal)
        self.assertEqual(precise.as_tuple(), decimal.Decimal(token).as_tuple())

    @hypothesis.given(
        value=hypothesis.strategies.datetimes(),
        offset_minutes=hypothesis.strategies.one_of(
            hypothesis.strategies.none(),
            hypothesis.strategies.integers(-1439, 1439),
        ),
    )
    # Converting to UTC would move this valid local date before year 1.
    @hypothesis.example(value=datetime.datetime.min, offset_minutes=1439)
    # Converting to UTC would move this valid local date past year 9999.
    @hypothesis.example(value=datetime.datetime.max, offset_minutes=-1439)
    # A leap day in a century divisible by 400 is valid.
    @hypothesis.example(value=datetime.datetime(2000, 2, 29),
                        offset_minutes=None)
    # TOML carries an offset but cannot encode Python's fold flag.
    @hypothesis.example(value=datetime.datetime(2024, 11, 3, 1, 30, fold=1),
                        offset_minutes=0)
    def test_datetime_representations(
        self, value: datetime.datetime, offset_minutes: int | None,
    ) -> None:
        zone = (None if offset_minutes is None else
                datetime.timezone(datetime.timedelta(minutes=offset_minutes)))
        expected = value.replace(tzinfo=zone, fold=0)
        for separator in ('T', 't', ' '):
            token = expected.isoformat(sep=separator)
            actual = tomllib.loads(f'value = {token}')['value']
            self.assertIs(type(actual), datetime.datetime)
            # Compare wall time and offset separately, even at year boundaries
            # where converting a valid local datetime to UTC could overflow.
            self.assertEqual(
                actual.replace(tzinfo=None), expected.replace(tzinfo=None),
            )
            self.assertEqual(actual.utcoffset(), expected.utcoffset())

    @hypothesis.given(
        value=hypothesis.strategies.times(),
        extra_digits=hypothesis.strategies.text(
            alphabet='0123456789', min_size=1, max_size=12,
        ),
    )
    # Digits beyond microsecond precision must not create a nonzero time.
    @hypothesis.example(value=datetime.time.min, extra_digits='1')
    # Truncation must not round up into the next day.
    @hypothesis.example(value=datetime.time.max, extra_digits='999999')
    def test_fractional_seconds_are_truncated(
        self, value: datetime.time, extra_digits: str,
    ) -> None:
        token = value.isoformat(timespec='microseconds') + extra_digits
        actual = tomllib.loads(f'value = {token}')['value']
        self.assertIs(type(actual), datetime.time)
        self.assertEqual(actual, value.replace(fold=0))

    @hypothesis.given(
        values=hypothesis.strategies.lists(
            hypothesis.strategies.text(max_size=20), max_size=10,
        ).map(tuple),
    )
    @hypothesis.example(values=())
    # Comment markers, quotes, backslashes and encoded newlines are data when
    # they occur inside the string rather than between array elements.
    @hypothesis.example(values=('', '#', '\n\r', '"\\'))
    def test_array_whitespace_and_comments(
        self, values: tuple[str, ...],
    ) -> None:
        # Escaped values contain no physical newlines, so changing the line
        # endings changes only formatting, not the contents of the strings.
        tokens = tuple(quote(value) for value in values)
        plain = f'array = [{", ".join(tokens)}]'
        decorated = 'array\t= [\n# before items\n' + ''.join(
            f'\t{token}, # after item\n' for token in tokens
        ) + '] # after array\n'
        expected = {'array': list(values)}
        for document in (plain, decorated, decorated.replace('\n', '\r\n')):
            self.assertEqual(tomllib.loads(document), expected, document)

    @unittest.expectedFailure
    @hypothesis.given(
        body=hypothesis.strategies.text(alphabet='abc 012#', max_size=40),
        opening=hypothesis.strategies.sampled_from(('"', 'key="', 'key="""')),
    )
    # Minimal counterexample found by Hypothesis: an unfinished quoted key.
    @hypothesis.example(body='', opening='"')
    @hypothesis.example(body='', opening='key="')
    @hypothesis.example(body='abc', opening='key="""')
    def test_unterminated_escape_position(self, body: str, opening: str) -> None:
        # An unfinished escape currently advances the error position past EOF.
        # Keep every generated case in that failing region, so this expected
        # failure does not hide unrelated error-position failures.
        # Related, but distinct: https://github.com/hukkin/tomli/pull/307
        document = opening + body + '\\'
        with self.assertRaises(tomllib.TOMLDecodeError) as caught:
            tomllib.loads(document)
        error = caught.exception
        self.assertEqual(error.doc, document)
        self.assertGreaterEqual(error.pos, 0)
        # EOF itself is a valid location, but a position beyond it is not.
        self.assertLessEqual(error.pos, len(error.doc))


if __name__ == '__main__':
    unittest.main()
