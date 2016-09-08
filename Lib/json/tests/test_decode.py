import decimal
from StringIO import StringIO
from collections import OrderedDict
from json.tests import PyTest, CTest


class TestDecode(object):
    def test_decimal(self):
        rval = self.loads('1.1', parse_float=decimal.Decimal)
        self.assertTrue(isinstance(rval, decimal.Decimal))
        self.assertEqual(rval, decimal.Decimal('1.1'))

    def test_float(self):
        rval = self.loads('1', parse_int=float)
        self.assertTrue(isinstance(rval, float))
        self.assertEqual(rval, 1.0)

    def test_decoder_optimizations(self):
        # Several optimizations were made that skip over calls to
        # the whitespace regex, so this test is designed to try and
        # exercise the uncommon cases. The array cases are already covered.
        rval = self.loads('{   "key"    :    "value"    ,  "k":"v"    }')
        self.assertEqual(rval, {"key":"value", "k":"v"})

    def test_empty_objects(self):
        self.assertEqual(self.loads('{}'), {})
        self.assertEqual(self.loads('[]'), [])
        self.assertEqual(self.loads('""'), u"")
        self.assertIsInstance(self.loads('""'), unicode)

    def test_object_pairs_hook(self):
        s = '{"xkd":1, "kcw":2, "art":3, "hxm":4, "qrt":5, "pad":6, "hoy":7}'
        p = [("xkd", 1), ("kcw", 2), ("art", 3), ("hxm", 4),
             ("qrt", 5), ("pad", 6), ("hoy", 7)]
        self.assertEqual(self.loads(s), eval(s))
        self.assertEqual(self.loads(s, object_pairs_hook=lambda x: x), p)
        self.assertEqual(self.json.load(StringIO(s),
                                        object_pairs_hook=lambda x: x), p)
        od = self.loads(s, object_pairs_hook=OrderedDict)
        self.assertEqual(od, OrderedDict(p))
        self.assertEqual(type(od), OrderedDict)
        # the object_pairs_hook takes priority over the object_hook
        self.assertEqual(self.loads(s, object_pairs_hook=OrderedDict,
                                    object_hook=lambda x: None),
                         OrderedDict(p))
        # check that empty object literals work (see #17368)
        self.assertEqual(self.loads('{}', object_pairs_hook=OrderedDict),
                         OrderedDict())
        self.assertEqual(self.loads('{"empty": {}}',
                                    object_pairs_hook=OrderedDict),
                         OrderedDict([('empty', OrderedDict())]))

    def test_extra_data(self):
        s = '[1, 2, 3]5'
        msg = 'Extra data'
        self.assertRaisesRegexp(ValueError, msg, self.loads, s)

    def test_invalid_escape(self):
        s = '["abc\\y"]'
        msg = 'escape'
        self.assertRaisesRegexp(ValueError, msg, self.loads, s)

    def test_negative_index(self):
        d = self.json.JSONDecoder()
        self.assertRaises(ValueError, d.raw_decode, 'a'*42, -50000)
        self.assertRaises(ValueError, d.raw_decode, u'a'*42, -50000)

class TestPyDecode(TestDecode, PyTest): pass
class TestCDecode(TestDecode, CTest): pass
