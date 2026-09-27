"""Properties of TOML numeric representations through the public API."""

import decimal
import math
import unittest

from test.support.hypothesis_helper import hypothesis

from . import tomllib


class TestProperties(unittest.TestCase):
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
