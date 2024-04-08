import decimal
from io import StringIO
from collections import OrderedDict
from test.test_json import PyTest, CTest
from test import support


class TestDecode:
    def test_decimal(self):
        rval = self.loads('1.1', parse_float=decimal.Decimal)
        self.assertIsInstance(rval, decimal.Decimal)
        self.assertEqual(rval, decimal.Decimal('1.1'))

    def test_float(self):
        rval = self.loads('1', parse_int=float)
        self.assertIsInstance(rval, float)
        self.assertEqual(rval, 1.0)

    def test_bytes(self):
        self.assertEqual(self.loads(b"1"), 1)

    def test_parse_constant(self):
        for constant, expected in [
            ("Infinity", "INFINITY"),
            ("-Infinity", "-INFINITY"),
            ("NaN", "NAN"),
        ]:
            self.assertEqual(
                self.loads(constant, parse_constant=str.upper), expected
            )

    def test_constant_invalid_case(self):
        for constant in [
            "nan", "NAN", "naN", "infinity", "INFINITY", "inFiniTy"
        ]:
            with self.assertRaises(self.JSONDecodeError):
                self.loads(constant)

    def test_empty_objects(self):
        self.assertEqual(self.loads('{}'), {})
        self.assertEqual(self.loads('[]'), [])
        self.assertEqual(self.loads('""'), "")

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

    def test_decoder_optimizations(self):
        # Several optimizations were made that skip over calls to
        # the whitespace regex, so this test is designed to try and
        # exercise the uncommon cases. The array cases are already covered.
        rval = self.loads('{   "key"    :    "value"    ,  "k":"v"    }')
        self.assertEqual(rval, {"key":"value", "k":"v"})

    def check_keys_reuse(self, source, loads):
        rval = loads(source)
        (a, b), (c, d) = sorted(rval[0]), sorted(rval[1])
        self.assertIs(a, c)
        self.assertIs(b, d)

    def test_keys_reuse(self):
        s = '[{"a_key": 1, "b_\xe9": 2}, {"a_key": 3, "b_\xe9": 4}]'
        self.check_keys_reuse(s, self.loads)
        decoder = self.json.decoder.JSONDecoder()
        self.check_keys_reuse(s, decoder.decode)
        self.assertFalse(decoder.memo)

    def test_extra_data(self):
        s = '[1, 2, 3]5'
        msg = 'Extra data'
        self.assertRaisesRegex(self.JSONDecodeError, msg, self.loads, s)

    def test_invalid_escape(self):
        s = '["abc\\y"]'
        msg = 'escape'
        self.assertRaisesRegex(self.JSONDecodeError, msg, self.loads, s)

    def test_invalid_input_type(self):
        msg = 'the JSON object must be str'
        for value in [1, 3.14, [], {}, None]:
            self.assertRaisesRegex(TypeError, msg, self.loads, value)

    def test_string_with_utf8_bom(self):
        # see #18958
        bom_json = "[1,2,3]".encode('utf-8-sig').decode('utf-8')
        with self.assertRaises(self.JSONDecodeError) as cm:
            self.loads(bom_json)
        self.assertIn('BOM', str(cm.exception))
        with self.assertRaises(self.JSONDecodeError) as cm:
            self.json.load(StringIO(bom_json))
        self.assertIn('BOM', str(cm.exception))
        # make sure that the BOM is not detected in the middle of a string
        bom = ''.encode('utf-8-sig').decode('utf-8')
        bom_in_str = f'"{bom}"'
        self.assertEqual(self.loads(bom_in_str), '\ufeff')
        self.assertEqual(self.json.load(StringIO(bom_in_str)), '\ufeff')

    def test_negative_index(self):
        d = self.json.JSONDecoder()
        self.assertRaises(ValueError, d.raw_decode, 'a'*42, -50000)

    def test_limit_int(self):
        maxdigits = 5000
        with support.adjust_int_max_str_digits(maxdigits):
            self.loads('1' * maxdigits)
            with self.assertRaises(ValueError):
                self.loads('1' * (maxdigits + 1))


class TestPyDecode(TestDecode, PyTest): pass
class TestCDecode(TestDecode, CTest): pass
