from unittest import TestCase

import json
from collections import OrderedDict

class TestUnicode(TestCase):
    # test_encoding1 and test_encoding2 from 2.x are irrelevant (only str
    # is supported as input, not bytes).

    def test_encoding3(self):
        u = '\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
        j = json.dumps(u)
        self.assertEqual(j, '"\\u03b1\\u03a9"')

    def test_encoding4(self):
        u = '\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
        j = json.dumps([u])
        self.assertEqual(j, '["\\u03b1\\u03a9"]')

    def test_encoding5(self):
        u = '\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
        j = json.dumps(u, ensure_ascii=False)
        self.assertEqual(j, '"{0}"'.format(u))

    def test_encoding6(self):
        u = '\N{GREEK SMALL LETTER ALPHA}\N{GREEK CAPITAL LETTER OMEGA}'
        j = json.dumps([u], ensure_ascii=False)
        self.assertEqual(j, '["{0}"]'.format(u))

    def test_big_unicode_encode(self):
        u = '\U0001d120'
        self.assertEqual(json.dumps(u), '"\\ud834\\udd20"')
        self.assertEqual(json.dumps(u, ensure_ascii=False), '"\U0001d120"')

    def test_big_unicode_decode(self):
        u = 'z\U0001d120x'
        self.assertEqual(json.loads('"' + u + '"'), u)
        self.assertEqual(json.loads('"z\\ud834\\udd20x"'), u)

    def test_unicode_decode(self):
        for i in range(0, 0xd7ff):
            u = chr(i)
            s = '"\\u{0:04x}"'.format(i)
            self.assertEqual(json.loads(s), u)

    def test_unicode_preservation(self):
        self.assertEqual(type(json.loads('""')), str)
        self.assertEqual(type(json.loads('"a"')), str)
        self.assertEqual(type(json.loads('["a"]')[0]), str)

    def test_bytes_encode(self):
        self.assertRaises(TypeError, json.dumps, b"hi")
        self.assertRaises(TypeError, json.dumps, [b"hi"])

    def test_bytes_decode(self):
        self.assertRaises(TypeError, json.loads, b'"hi"')
        self.assertRaises(TypeError, json.loads, b'["hi"]')


    def test_object_pairs_hook_with_unicode(self):
        s = '{"xkd":1, "kcw":2, "art":3, "hxm":4, "qrt":5, "pad":6, "hoy":7}'
        p = [("xkd", 1), ("kcw", 2), ("art", 3), ("hxm", 4),
             ("qrt", 5), ("pad", 6), ("hoy", 7)]
        self.assertEqual(json.loads(s), eval(s))
        self.assertEqual(json.loads(s, object_pairs_hook = lambda x: x), p)
        od = json.loads(s, object_pairs_hook = OrderedDict)
        self.assertEqual(od, OrderedDict(p))
        self.assertEqual(type(od), OrderedDict)
        # the object_pairs_hook takes priority over the object_hook
        self.assertEqual(json.loads(s,
                                    object_pairs_hook = OrderedDict,
                                    object_hook = lambda x: None),
                         OrderedDict(p))
