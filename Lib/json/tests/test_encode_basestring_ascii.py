from collections import OrderedDict
from json.tests import PyTest, CTest


CASES = [
    (u'/\\"\ucafe\ubabe\uab98\ufcde\ubcda\uef4a\x08\x0c\n\r\t`1~!@#$%^&*()_+-=[]{}|;:\',./<>?', '"/\\\\\\"\\ucafe\\ubabe\\uab98\\ufcde\\ubcda\\uef4a\\b\\f\\n\\r\\t`1~!@#$%^&*()_+-=[]{}|;:\',./<>?"'),
    (u'\u0123\u4567\u89ab\ucdef\uabcd\uef4a', '"\\u0123\\u4567\\u89ab\\ucdef\\uabcd\\uef4a"'),
    (u'controls', '"controls"'),
    (u'\x08\x0c\n\r\t', '"\\b\\f\\n\\r\\t"'),
    (u'{"object with 1 member":["array with 1 element"]}', '"{\\"object with 1 member\\":[\\"array with 1 element\\"]}"'),
    (u' s p a c e d ', '" s p a c e d "'),
    (u'\U0001d120', '"\\ud834\\udd20"'),
    (u'\u03b1\u03a9', '"\\u03b1\\u03a9"'),
    ('\xce\xb1\xce\xa9', '"\\u03b1\\u03a9"'),
    (u'\u03b1\u03a9', '"\\u03b1\\u03a9"'),
    ('\xce\xb1\xce\xa9', '"\\u03b1\\u03a9"'),
    (u'\u03b1\u03a9', '"\\u03b1\\u03a9"'),
    (u'\u03b1\u03a9', '"\\u03b1\\u03a9"'),
    (u"`1~!@#$%^&*()_+-={':[,]}|;.</>?", '"`1~!@#$%^&*()_+-={\':[,]}|;.</>?"'),
    (u'\x08\x0c\n\r\t', '"\\b\\f\\n\\r\\t"'),
    (u'\u0123\u4567\u89ab\ucdef\uabcd\uef4a', '"\\u0123\\u4567\\u89ab\\ucdef\\uabcd\\uef4a"'),
]

class TestEncodeBasestringAscii(object):
    def test_encode_basestring_ascii(self):
        fname = self.json.encoder.encode_basestring_ascii.__name__
        for input_string, expect in CASES:
            result = self.json.encoder.encode_basestring_ascii(input_string)
            self.assertEqual(result, expect,
                '{0!r} != {1!r} for {2}({3!r})'.format(
                    result, expect, fname, input_string))

    def test_ordered_dict(self):
        # See issue 6105
        items = [('one', 1), ('two', 2), ('three', 3), ('four', 4), ('five', 5)]
        s = self.dumps(OrderedDict(items))
        self.assertEqual(s, '{"one": 1, "two": 2, "three": 3, "four": 4, "five": 5}')


class TestPyEncodeBasestringAscii(TestEncodeBasestringAscii, PyTest): pass
class TestCEncodeBasestringAscii(TestEncodeBasestringAscii, CTest): pass
