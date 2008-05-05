from unittest import TestCase

import json

class TestUnicode(TestCase):
    def test_encoding1(self):
        encoder = json.JSONEncoder(encoding='utf-8')
        u = u'\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
        s = u.encode('utf-8')
        ju = encoder.encode(u)
        js = encoder.encode(s)
        self.assertEquals(ju, js)

    def test_encoding2(self):
        u = u'\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
        s = u.encode('utf-8')
        ju = json.dumps(u, encoding='utf-8')
        js = json.dumps(s, encoding='utf-8')
        self.assertEquals(ju, js)

    def test_encoding3(self):
        u = u'\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
        j = json.dumps(u)
        self.assertEquals(j, '"\\u03b1\\u03a9"')

    def test_encoding4(self):
        u = u'\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
        j = json.dumps([u])
        self.assertEquals(j, '["\\u03b1\\u03a9"]')

    def test_encoding5(self):
        u = u'\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
        j = json.dumps(u, ensure_ascii=False)
        self.assertEquals(j, u'"{0}"'.format(u))

    def test_encoding6(self):
        u = u'\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
        j = json.dumps([u], ensure_ascii=False)
        self.assertEquals(j, u'["{0}"]'.format(u))

    def test_big_unicode_encode(self):
        u = u'\U0001d120'
        self.assertEquals(json.dumps(u), '"\\ud834\\udd20"')
        self.assertEquals(json.dumps(u, ensure_ascii=False), u'"\U0001d120"')

    def test_big_unicode_decode(self):
        u = u'z\U0001d120x'
        self.assertEquals(json.loads('"' + u + '"'), u)
        self.assertEquals(json.loads('"z\\ud834\\udd20x"'), u)

    def test_unicode_decode(self):
        for i in range(0, 0xd7ff):
            u = unichr(i)
            js = '"\\u{0:04x}"'.format(i)
            self.assertEquals(json.loads(js), u)
