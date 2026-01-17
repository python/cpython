import codecs
from collections import OrderedDict
from test.test_json import PyTest, CTest


class TestUnicode:
    # test_encoding1 and test_encoding2 from 2.x are irrelevant (only str
    # is supported as input, not bytes).

    def test_encoding3(self):
        u = '\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
        j = self.dumps(u)
        self.assertEqual(j, '"\\u03b1\\u03a9"')

    def test_encoding4(self):
        u = '\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
        j = self.dumps([u])
        self.assertEqual(j, '["\\u03b1\\u03a9"]')

    def test_encoding5(self):
        u = '\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
        j = self.dumps(u, ensure_ascii=False)
        self.assertEqual(j, f'"{u}"')

    def test_encoding6(self):
        u = '\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
        j = self.dumps([u], ensure_ascii=False)
        self.assertEqual(j, f'["{u}"]')

    def test_encoding7(self):
        u = '\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
        j = self.dumps(u + "\n", ensure_ascii=False)
        self.assertEqual(j, f'"{u}\\n"')

    def test_ascii_non_printable_encode(self):
        u = '\b\t\n\f\r\x00\x1f\x7f'
        self.assertEqual(self.dumps(u),
                         '"\\b\\t\\n\\f\\r\\u0000\\u001f\\u007f"')
        self.assertEqual(self.dumps(u, ensure_ascii=False),
                         '"\\b\\t\\n\\f\\r\\u0000\\u001f\x7f"')

    def test_ascii_non_printable_decode(self):
        self.assertEqual(self.loads('"\\b\\t\\n\\f\\r"'),
                         '\b\t\n\f\r')
        s = ''.join(map(chr, range(32)))
        for c in s:
            self.assertRaises(self.JSONDecodeError, self.loads, f'"{c}"')
        self.assertEqual(self.loads(f'"{s}"', strict=False), s)
        self.assertEqual(self.loads('"\x7f"'), '\x7f')

    def test_escaped_decode(self):
        self.assertEqual(self.loads('"\\b\\t\\n\\f\\r"'), '\b\t\n\f\r')
        self.assertEqual(self.loads('"\\"\\\\\\/"'), '"\\/')
        for c in set(map(chr, range(0x100))) - set('"\\/bfnrt'):
            self.assertRaises(self.JSONDecodeError, self.loads, f'"\\{c}"')
            self.assertRaises(self.JSONDecodeError, self.loads, f'"\\{c}"', strict=False)

    def test_big_unicode_encode(self):
        u = '\U0001d120'
        self.assertEqual(self.dumps(u), '"\\ud834\\udd20"')
        self.assertEqual(self.dumps(u, ensure_ascii=False), '"\U0001d120"')

    def test_big_unicode_decode(self):
        u = 'z\U0001d120x'
        self.assertEqual(self.loads(f'"{u}"'), u)
        self.assertEqual(self.loads('"z\\ud834\\udd20x"'), u)

    def test_unicode_decode(self):
        for i in range(0, 0xd7ff):
            u = chr(i)
            s = f'"\\u{i:04x}"'
            self.assertEqual(self.loads(s), u)

    def test_single_surrogate_encode(self):
        self.assertEqual(self.dumps('\uD83D'), '"\\ud83d"')
        self.assertEqual(self.dumps('\uD83D', ensure_ascii=False), '"\ud83d"')
        self.assertEqual(self.dumps('\uDC0D'), '"\\udc0d"')
        self.assertEqual(self.dumps('\uDC0D', ensure_ascii=False), '"\udc0d"')

    def test_single_surrogate_decode(self):
        self.assertEqual(self.loads('"\uD83D"'), '\ud83d')
        self.assertEqual(self.loads('"\\uD83D"'), '\ud83d')
        self.assertEqual(self.loads('"\udc0d"'), '\udc0d')
        self.assertEqual(self.loads('"\\udc0d"'), '\udc0d')

    def test_unicode_preservation(self):
        self.assertEqual(type(self.loads('""')), str)
        self.assertEqual(type(self.loads('"a"')), str)
        self.assertEqual(type(self.loads('["a"]')[0]), str)

    def test_bytes_encode(self):
        self.assertRaises(TypeError, self.dumps, b"hi")
        self.assertRaises(TypeError, self.dumps, [b"hi"])

    def test_bytes_decode(self):
        for encoding, bom in [
                ('utf-8', codecs.BOM_UTF8),
                ('utf-16be', codecs.BOM_UTF16_BE),
                ('utf-16le', codecs.BOM_UTF16_LE),
                ('utf-32be', codecs.BOM_UTF32_BE),
                ('utf-32le', codecs.BOM_UTF32_LE),
            ]:
            data = ["a\xb5\u20ac\U0001d120"]
            encoded = self.dumps(data).encode(encoding)
            self.assertEqual(self.loads(bom + encoded), data)
            self.assertEqual(self.loads(encoded), data)
        self.assertRaises(UnicodeDecodeError, self.loads, b'["\x80"]')
        # RFC-7159 and ECMA-404 extend JSON to allow documents that
        # consist of only a string, which can present a special case
        # not covered by the encoding detection patterns specified in
        # RFC-4627 for utf-16-le (XX 00 XX 00).
        self.assertEqual(self.loads('"\u2600"'.encode('utf-16-le')),
                         '\u2600')
        # Encoding detection for small (<4) bytes objects
        # is implemented as a special case. RFC-7159 and ECMA-404
        # allow single codepoint JSON documents which are only two
        # bytes in utf-16 encodings w/o BOM.
        self.assertEqual(self.loads(b'5\x00'), 5)
        self.assertEqual(self.loads(b'\x007'), 7)
        self.assertEqual(self.loads(b'57'), 57)

    def test_object_pairs_hook_with_unicode(self):
        s = '{"xkd":1, "kcw":2, "art":3, "hxm":4, "qrt":5, "pad":6, "hoy":7}'
        p = [("xkd", 1), ("kcw", 2), ("art", 3), ("hxm", 4),
             ("qrt", 5), ("pad", 6), ("hoy", 7)]
        self.assertEqual(self.loads(s), eval(s))
        self.assertEqual(self.loads(s, object_pairs_hook = lambda x: x), p)
        od = self.loads(s, object_pairs_hook = OrderedDict)
        self.assertEqual(od, OrderedDict(p))
        self.assertEqual(type(od), OrderedDict)
        # the object_pairs_hook takes priority over the object_hook
        self.assertEqual(self.loads(s, object_pairs_hook = OrderedDict,
                                    object_hook = lambda x: None),
                         OrderedDict(p))


class TestPyUnicode(TestUnicode, PyTest): pass
class TestCUnicode(TestUnicode, CTest): pass
