from test.test_json import PyTest, CTest
from test.support import bigaddrspacetest


CASES = [
    (
        '/\\"\ucafe\ubabe\uab98\ufcde\ubcda\uef4a\x08\x0c\n\r\t`1~!@#$%^&*()_+-=[]{}|;:\',./<>?',
        '"/\\\\\\"\ucafe\ubabe\uab98\ufcde\ubcda\uef4a\\b\\f\\n\\r\\t`1~!@#$%^&*()_+-=[]{}|;:\',./<>?"',
    ),
    (
        '\u0123\u4567\u89ab\ucdef\uabcd\uef4a',
        '"\u0123\u4567\u89ab\ucdef\uabcd\uef4a"',
    ),
    ('controls', '"controls"'),
    ('\x08\x0c\n\r\t', '"\\b\\f\\n\\r\\t"'),
    (
        '{"object with 1 member":["array with 1 element"]}',
        '"{\\"object with 1 member\\":[\\"array with 1 element\\"]}"',
    ),
    (' s p a c e d ', '" s p a c e d "'),
    ('\U0001d120', '"\U0001d120"'),
    ('\u03b1\u03a9', '"\u03b1\u03a9"'),
    ("`1~!@#$%^&*()_+-={':[,]}|;.</>?", '"`1~!@#$%^&*()_+-={\':[,]}|;.</>?"'),
    ('\x08\x0c\n\r\t', '"\\b\\f\\n\\r\\t"'),
    (
        '\u0123\u4567\u89ab\ucdef\uabcd\uef4a',
        '"\u0123\u4567\u89ab\ucdef\uabcd\uef4a"',
    ),
]


class TestEncodeBasestring:
    def test_encode_basestring(self):
        filename = self.json.encoder.encode_basestring.__name__
        for input_string, expect in CASES:
            result = self.json.encoder.encode_basestring(input_string)
            self.assertEqual(
                result,
                expect,
                '{0!r} != {1!r} for {2}({3!r})'.format(
                    result, expect, filename, input_string
                ),
            )


class TestPyEncodeBasestring(TestEncodeBasestring, PyTest):
    pass


class TestCEncodeBasestring(TestEncodeBasestring, CTest):
    @bigaddrspacetest
    def test_overflow(self):
        size = (2**32) // 6 + 1
        s = "\x00" * size
        with self.assertRaises(OverflowError):
            self.json.encoder.encode_basestring(s)
