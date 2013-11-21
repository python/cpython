# Copyright (C) 2003-2013 Python Software Foundation

import unittest
import plistlib
import os
import datetime
import codecs
import binascii
import collections
from test import support
from io import BytesIO

ALL_FORMATS=(plistlib.FMT_XML, plistlib.FMT_BINARY)

# The testdata is generated using Mac/Tools/plistlib_generate_testdata.py
# (which using PyObjC to control the Cocoa classes for generating plists)
TESTDATA={
    plistlib.FMT_XML: binascii.a2b_base64(b'''
        PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiPz4KPCFET0NU
        WVBFIHBsaXN0IFBVQkxJQyAiLS8vQXBwbGUvL0RURCBQTElTVCAxLjAvL0VO
        IiAiaHR0cDovL3d3dy5hcHBsZS5jb20vRFREcy9Qcm9wZXJ0eUxpc3QtMS4w
        LmR0ZCI+CjxwbGlzdCB2ZXJzaW9uPSIxLjAiPgo8ZGljdD4KCTxrZXk+YURh
        dGU8L2tleT4KCTxkYXRlPjIwMDQtMTAtMjZUMTA6MzM6MzNaPC9kYXRlPgoJ
        PGtleT5hRGljdDwva2V5PgoJPGRpY3Q+CgkJPGtleT5hRmFsc2VWYWx1ZTwv
        a2V5PgoJCTxmYWxzZS8+CgkJPGtleT5hVHJ1ZVZhbHVlPC9rZXk+CgkJPHRy
        dWUvPgoJCTxrZXk+YVVuaWNvZGVWYWx1ZTwva2V5PgoJCTxzdHJpbmc+TcOk
        c3NpZywgTWHDnzwvc3RyaW5nPgoJCTxrZXk+YW5vdGhlclN0cmluZzwva2V5
        PgoJCTxzdHJpbmc+Jmx0O2hlbGxvICZhbXA7ICdoaScgdGhlcmUhJmd0Ozwv
        c3RyaW5nPgoJCTxrZXk+ZGVlcGVyRGljdDwva2V5PgoJCTxkaWN0PgoJCQk8
        a2V5PmE8L2tleT4KCQkJPGludGVnZXI+MTc8L2ludGVnZXI+CgkJCTxrZXk+
        Yjwva2V5PgoJCQk8cmVhbD4zMi41PC9yZWFsPgoJCQk8a2V5PmM8L2tleT4K
        CQkJPGFycmF5PgoJCQkJPGludGVnZXI+MTwvaW50ZWdlcj4KCQkJCTxpbnRl
        Z2VyPjI8L2ludGVnZXI+CgkJCQk8c3RyaW5nPnRleHQ8L3N0cmluZz4KCQkJ
        PC9hcnJheT4KCQk8L2RpY3Q+Cgk8L2RpY3Q+Cgk8a2V5PmFGbG9hdDwva2V5
        PgoJPHJlYWw+MC41PC9yZWFsPgoJPGtleT5hTGlzdDwva2V5PgoJPGFycmF5
        PgoJCTxzdHJpbmc+QTwvc3RyaW5nPgoJCTxzdHJpbmc+Qjwvc3RyaW5nPgoJ
        CTxpbnRlZ2VyPjEyPC9pbnRlZ2VyPgoJCTxyZWFsPjMyLjU8L3JlYWw+CgkJ
        PGFycmF5PgoJCQk8aW50ZWdlcj4xPC9pbnRlZ2VyPgoJCQk8aW50ZWdlcj4y
        PC9pbnRlZ2VyPgoJCQk8aW50ZWdlcj4zPC9pbnRlZ2VyPgoJCTwvYXJyYXk+
        Cgk8L2FycmF5PgoJPGtleT5hU3RyaW5nPC9rZXk+Cgk8c3RyaW5nPkRvb2Rh
        aDwvc3RyaW5nPgoJPGtleT5hbkVtcHR5RGljdDwva2V5PgoJPGRpY3QvPgoJ
        PGtleT5hbkVtcHR5TGlzdDwva2V5PgoJPGFycmF5Lz4KCTxrZXk+YW5JbnQ8
        L2tleT4KCTxpbnRlZ2VyPjcyODwvaW50ZWdlcj4KCTxrZXk+bmVzdGVkRGF0
        YTwva2V5PgoJPGFycmF5PgoJCTxkYXRhPgoJCVBHeHZkSE1nYjJZZ1ltbHVZ
        WEo1SUdkMWJtcytBQUVDQXp4c2IzUnpJRzltSUdKcGJtRnllU0JuZFc1cgoJ
        CVBnQUJBZ004Ykc5MGN5QnZaaUJpYVc1aGNua2daM1Z1YXo0QUFRSURQR3h2
        ZEhNZ2IyWWdZbWx1WVhKNQoJCUlHZDFibXMrQUFFQ0F6eHNiM1J6SUc5bUlH
        SnBibUZ5ZVNCbmRXNXJQZ0FCQWdNOGJHOTBjeUJ2WmlCaQoJCWFXNWhjbmtn
        WjNWdWF6NEFBUUlEUEd4dmRITWdiMllnWW1sdVlYSjVJR2QxYm1zK0FBRUNB
        enhzYjNSegoJCUlHOW1JR0pwYm1GeWVTQm5kVzVyUGdBQkFnTThiRzkwY3lC
        dlppQmlhVzVoY25rZ1ozVnVhejRBQVFJRAoJCVBHeHZkSE1nYjJZZ1ltbHVZ
        WEo1SUdkMWJtcytBQUVDQXc9PQoJCTwvZGF0YT4KCTwvYXJyYXk+Cgk8a2V5
        PnNvbWVEYXRhPC9rZXk+Cgk8ZGF0YT4KCVBHSnBibUZ5ZVNCbmRXNXJQZz09
        Cgk8L2RhdGE+Cgk8a2V5PnNvbWVNb3JlRGF0YTwva2V5PgoJPGRhdGE+CglQ
        R3h2ZEhNZ2IyWWdZbWx1WVhKNUlHZDFibXMrQUFFQ0F6eHNiM1J6SUc5bUlH
        SnBibUZ5ZVNCbmRXNXJQZ0FCQWdNOAoJYkc5MGN5QnZaaUJpYVc1aGNua2da
        M1Z1YXo0QUFRSURQR3h2ZEhNZ2IyWWdZbWx1WVhKNUlHZDFibXMrQUFFQ0F6
        eHMKCWIzUnpJRzltSUdKcGJtRnllU0JuZFc1clBnQUJBZ004Ykc5MGN5QnZa
        aUJpYVc1aGNua2daM1Z1YXo0QUFRSURQR3h2CglkSE1nYjJZZ1ltbHVZWEo1
        SUdkMWJtcytBQUVDQXp4c2IzUnpJRzltSUdKcGJtRnllU0JuZFc1clBnQUJB
        Z004Ykc5MAoJY3lCdlppQmlhVzVoY25rZ1ozVnVhejRBQVFJRFBHeHZkSE1n
        YjJZZ1ltbHVZWEo1SUdkMWJtcytBQUVDQXc9PQoJPC9kYXRhPgoJPGtleT7D
        hWJlbnJhYTwva2V5PgoJPHN0cmluZz5UaGF0IHdhcyBhIHVuaWNvZGUga2V5
        Ljwvc3RyaW5nPgo8L2RpY3Q+CjwvcGxpc3Q+Cg=='''),
    plistlib.FMT_BINARY: binascii.a2b_base64(b'''
        YnBsaXN0MDDcAQIDBAUGBwgJCgsMDQ4iIykqKywtLy4wVWFEYXRlVWFEaWN0
        VmFGbG9hdFVhTGlzdFdhU3RyaW5nW2FuRW1wdHlEaWN0W2FuRW1wdHlMaXN0
        VWFuSW50Wm5lc3RlZERhdGFYc29tZURhdGFcc29tZU1vcmVEYXRhZwDFAGIA
        ZQBuAHIAYQBhM0GcuX30AAAA1Q8QERITFBUWFxhbYUZhbHNlVmFsdWVaYVRy
        dWVWYWx1ZV1hVW5pY29kZVZhbHVlXWFub3RoZXJTdHJpbmdaZGVlcGVyRGlj
        dAgJawBNAOQAcwBzAGkAZwAsACAATQBhAN9fEBU8aGVsbG8gJiAnaGknIHRo
        ZXJlIT7TGRobHB0eUWFRYlFjEBEjQEBAAAAAAACjHyAhEAEQAlR0ZXh0Iz/g
        AAAAAAAApSQlJh0nUUFRQhAMox8gKBADVkRvb2RhaNCgEQLYoS5PEPo8bG90
        cyBvZiBiaW5hcnkgZ3Vuaz4AAQIDPGxvdHMgb2YgYmluYXJ5IGd1bms+AAEC
        Azxsb3RzIG9mIGJpbmFyeSBndW5rPgABAgM8bG90cyBvZiBiaW5hcnkgZ3Vu
        az4AAQIDPGxvdHMgb2YgYmluYXJ5IGd1bms+AAECAzxsb3RzIG9mIGJpbmFy
        eSBndW5rPgABAgM8bG90cyBvZiBiaW5hcnkgZ3Vuaz4AAQIDPGxvdHMgb2Yg
        YmluYXJ5IGd1bms+AAECAzxsb3RzIG9mIGJpbmFyeSBndW5rPgABAgM8bG90
        cyBvZiBiaW5hcnkgZ3Vuaz4AAQIDTTxiaW5hcnkgZ3Vuaz5fEBdUaGF0IHdh
        cyBhIHVuaWNvZGUga2V5LgAIACEAJwAtADQAOgBCAE4AWgBgAGsAdACBAJAA
        mQCkALAAuwDJANcA4gDjAOQA+wETARoBHAEeASABIgErAS8BMQEzATgBQQFH
        AUkBSwFNAVEBUwFaAVsBXAFfAWECXgJsAAAAAAAAAgEAAAAAAAAAMQAAAAAA
        AAAAAAAAAAAAAoY='''),
}


class TestPlistlib(unittest.TestCase):

    def tearDown(self):
        try:
            os.unlink(support.TESTFN)
        except:
            pass

    def _create(self, fmt=None):
        pl = dict(
            aString="Doodah",
            aList=["A", "B", 12, 32.5, [1, 2, 3]],
            aFloat = 0.5,
            anInt = 728,
            aDict=dict(
                anotherString="<hello & 'hi' there!>",
                aUnicodeValue='M\xe4ssig, Ma\xdf',
                aTrueValue=True,
                aFalseValue=False,
                deeperDict=dict(a=17, b=32.5, c=[1, 2, "text"]),
            ),
            someData = b"<binary gunk>",
            someMoreData = b"<lots of binary gunk>\0\1\2\3" * 10,
            nestedData = [b"<lots of binary gunk>\0\1\2\3" * 10],
            aDate = datetime.datetime(2004, 10, 26, 10, 33, 33),
            anEmptyDict = dict(),
            anEmptyList = list()
        )
        pl['\xc5benraa'] = "That was a unicode key."
        return pl

    def test_create(self):
        pl = self._create()
        self.assertEqual(pl["aString"], "Doodah")
        self.assertEqual(pl["aDict"]["aFalseValue"], False)

    def test_io(self):
        pl = self._create()
        with open(support.TESTFN, 'wb') as fp:
            plistlib.dump(pl, fp)

        with open(support.TESTFN, 'rb') as fp:
            pl2 = plistlib.load(fp)

        self.assertEqual(dict(pl), dict(pl2))

        self.assertRaises(AttributeError, plistlib.dump, pl, 'filename')
        self.assertRaises(AttributeError, plistlib.load, 'filename')


    def test_bytes(self):
        pl = self._create()
        data = plistlib.dumps(pl)
        pl2 = plistlib.loads(data)
        self.assertNotIsInstance(pl, plistlib._InternalDict)
        self.assertEqual(dict(pl), dict(pl2))
        data2 = plistlib.dumps(pl2)
        self.assertEqual(data, data2)

    def test_indentation_array(self):
        data = [[[[[[[[{'test': b'aaaaaa'}]]]]]]]]
        self.assertEqual(plistlib.loads(plistlib.dumps(data)), data)

    def test_indentation_dict(self):
        data = {'1': {'2': {'3': {'4': {'5': {'6': {'7': {'8': {'9': b'aaaaaa'}}}}}}}}}
        self.assertEqual(plistlib.loads(plistlib.dumps(data)), data)

    def test_indentation_dict_mix(self):
        data = {'1': {'2': [{'3': [[[[[{'test': b'aaaaaa'}]]]]]}]}}
        self.assertEqual(plistlib.loads(plistlib.dumps(data)), data)

    def test_appleformatting(self):
        for use_builtin_types in (True, False):
            for fmt in ALL_FORMATS:
                with self.subTest(fmt=fmt, use_builtin_types=use_builtin_types):
                    pl = plistlib.loads(TESTDATA[fmt],
                        use_builtin_types=use_builtin_types)
                    data = plistlib.dumps(pl, fmt=fmt)
                    self.assertEqual(data, TESTDATA[fmt],
                        "generated data was not identical to Apple's output")


    def test_appleformattingfromliteral(self):
        self.maxDiff = None
        for fmt in ALL_FORMATS:
            with self.subTest(fmt=fmt):
                pl = self._create(fmt=fmt)
                pl2 = plistlib.loads(TESTDATA[fmt])
                self.assertEqual(dict(pl), dict(pl2),
                    "generated data was not identical to Apple's output")

    def test_bytesio(self):
        for fmt in ALL_FORMATS:
            with self.subTest(fmt=fmt):
                b = BytesIO()
                pl = self._create(fmt=fmt)
                plistlib.dump(pl, b, fmt=fmt)
                pl2 = plistlib.load(BytesIO(b.getvalue()))
                self.assertEqual(dict(pl), dict(pl2))

    def test_keysort_bytesio(self):
        pl = collections.OrderedDict()
        pl['b'] = 1
        pl['a'] = 2
        pl['c'] = 3

        for fmt in ALL_FORMATS:
            for sort_keys in (False, True):
                with self.subTest(fmt=fmt, sort_keys=sort_keys):
                    b = BytesIO()

                    plistlib.dump(pl, b, fmt=fmt, sort_keys=sort_keys)
                    pl2 = plistlib.load(BytesIO(b.getvalue()),
                        dict_type=collections.OrderedDict)

                    self.assertEqual(dict(pl), dict(pl2))
                    if sort_keys:
                        self.assertEqual(list(pl2.keys()), ['a', 'b', 'c'])
                    else:
                        self.assertEqual(list(pl2.keys()), ['b', 'a', 'c'])

    def test_keysort(self):
        pl = collections.OrderedDict()
        pl['b'] = 1
        pl['a'] = 2
        pl['c'] = 3

        for fmt in ALL_FORMATS:
            for sort_keys in (False, True):
                with self.subTest(fmt=fmt, sort_keys=sort_keys):
                    data = plistlib.dumps(pl, fmt=fmt, sort_keys=sort_keys)
                    pl2 = plistlib.loads(data, dict_type=collections.OrderedDict)

                    self.assertEqual(dict(pl), dict(pl2))
                    if sort_keys:
                        self.assertEqual(list(pl2.keys()), ['a', 'b', 'c'])
                    else:
                        self.assertEqual(list(pl2.keys()), ['b', 'a', 'c'])

    def test_keys_no_string(self):
        pl = { 42: 'aNumber' }

        for fmt in ALL_FORMATS:
            with self.subTest(fmt=fmt):
                self.assertRaises(TypeError, plistlib.dumps, pl, fmt=fmt)

                b = BytesIO()
                self.assertRaises(TypeError, plistlib.dump, pl, b, fmt=fmt)

    def test_skipkeys(self):
        pl = {
            42: 'aNumber',
            'snake': 'aWord',
        }

        for fmt in ALL_FORMATS:
            with self.subTest(fmt=fmt):
                data = plistlib.dumps(
                    pl, fmt=fmt, skipkeys=True, sort_keys=False)

                pl2 = plistlib.loads(data)
                self.assertEqual(pl2, {'snake': 'aWord'})

                fp = BytesIO()
                plistlib.dump(
                    pl, fp, fmt=fmt, skipkeys=True, sort_keys=False)
                data = fp.getvalue()
                pl2 = plistlib.loads(fp.getvalue())
                self.assertEqual(pl2, {'snake': 'aWord'})

    def test_tuple_members(self):
        pl = {
            'first': (1, 2),
            'second': (1, 2),
            'third': (3, 4),
        }

        for fmt in ALL_FORMATS:
            with self.subTest(fmt=fmt):
                data = plistlib.dumps(pl, fmt=fmt)
                pl2 = plistlib.loads(data)
                self.assertEqual(pl2, {
                    'first': [1, 2],
                    'second': [1, 2],
                    'third': [3, 4],
                })
                self.assertIsNot(pl2['first'], pl2['second'])

    def test_list_members(self):
        pl = {
            'first': [1, 2],
            'second': [1, 2],
            'third': [3, 4],
        }

        for fmt in ALL_FORMATS:
            with self.subTest(fmt=fmt):
                data = plistlib.dumps(pl, fmt=fmt)
                pl2 = plistlib.loads(data)
                self.assertEqual(pl2, {
                    'first': [1, 2],
                    'second': [1, 2],
                    'third': [3, 4],
                })
                self.assertIsNot(pl2['first'], pl2['second'])

    def test_dict_members(self):
        pl = {
            'first': {'a': 1},
            'second': {'a': 1},
            'third': {'b': 2 },
        }

        for fmt in ALL_FORMATS:
            with self.subTest(fmt=fmt):
                data = plistlib.dumps(pl, fmt=fmt)
                pl2 = plistlib.loads(data)
                self.assertEqual(pl2, {
                    'first': {'a': 1},
                    'second': {'a': 1},
                    'third': {'b': 2 },
                })
                self.assertIsNot(pl2['first'], pl2['second'])

    def test_controlcharacters(self):
        for i in range(128):
            c = chr(i)
            testString = "string containing %s" % c
            if i >= 32 or c in "\r\n\t":
                # \r, \n and \t are the only legal control chars in XML
                plistlib.dumps(testString, fmt=plistlib.FMT_XML)
            else:
                self.assertRaises(ValueError,
                                  plistlib.dumps,
                                  testString)

    def test_nondictroot(self):
        for fmt in ALL_FORMATS:
            with self.subTest(fmt=fmt):
                test1 = "abc"
                test2 = [1, 2, 3, "abc"]
                result1 = plistlib.loads(plistlib.dumps(test1, fmt=fmt))
                result2 = plistlib.loads(plistlib.dumps(test2, fmt=fmt))
                self.assertEqual(test1, result1)
                self.assertEqual(test2, result2)

    def test_invalidarray(self):
        for i in ["<key>key inside an array</key>",
                  "<key>key inside an array2</key><real>3</real>",
                  "<true/><key>key inside an array3</key>"]:
            self.assertRaises(ValueError, plistlib.loads,
                              ("<plist><array>%s</array></plist>"%i).encode())

    def test_invaliddict(self):
        for i in ["<key><true/>k</key><string>compound key</string>",
                  "<key>single key</key>",
                  "<string>missing key</string>",
                  "<key>k1</key><string>v1</string><real>5.3</real>"
                  "<key>k1</key><key>k2</key><string>double key</string>"]:
            self.assertRaises(ValueError, plistlib.loads,
                              ("<plist><dict>%s</dict></plist>"%i).encode())
            self.assertRaises(ValueError, plistlib.loads,
                              ("<plist><array><dict>%s</dict></array></plist>"%i).encode())

    def test_invalidinteger(self):
        self.assertRaises(ValueError, plistlib.loads,
                          b"<plist><integer>not integer</integer></plist>")

    def test_invalidreal(self):
        self.assertRaises(ValueError, plistlib.loads,
                          b"<plist><integer>not real</integer></plist>")

    def test_xml_encodings(self):
        base = TESTDATA[plistlib.FMT_XML]

        for xml_encoding, encoding, bom in [
                    (b'utf-8', 'utf-8', codecs.BOM_UTF8),
                    (b'utf-16', 'utf-16-le', codecs.BOM_UTF16_LE),
                    (b'utf-16', 'utf-16-be', codecs.BOM_UTF16_BE),
                    # Expat does not support UTF-32
                    #(b'utf-32', 'utf-32-le', codecs.BOM_UTF32_LE),
                    #(b'utf-32', 'utf-32-be', codecs.BOM_UTF32_BE),
                ]:

            pl = self._create(fmt=plistlib.FMT_XML)
            with self.subTest(encoding=encoding):
                data = base.replace(b'UTF-8', xml_encoding)
                data = bom + data.decode('utf-8').encode(encoding)
                pl2 = plistlib.loads(data)
                self.assertEqual(dict(pl), dict(pl2))


class TestPlistlibDeprecated(unittest.TestCase):
    def test_io_deprecated(self):
        pl_in = {
            'key': 42,
            'sub': {
                'key': 9,
                'alt': 'value',
                'data': b'buffer',
            }
        }
        pl_out = plistlib._InternalDict({
            'key': 42,
            'sub': plistlib._InternalDict({
                'key': 9,
                'alt': 'value',
                'data': plistlib.Data(b'buffer'),
            })
        })

        self.addCleanup(support.unlink, support.TESTFN)
        with self.assertWarns(DeprecationWarning):
            plistlib.writePlist(pl_in, support.TESTFN)

        with self.assertWarns(DeprecationWarning):
            pl2 = plistlib.readPlist(support.TESTFN)

        self.assertEqual(pl_out, pl2)

        os.unlink(support.TESTFN)

        with open(support.TESTFN, 'wb') as fp:
            with self.assertWarns(DeprecationWarning):
                plistlib.writePlist(pl_in, fp)

        with open(support.TESTFN, 'rb') as fp:
            with self.assertWarns(DeprecationWarning):
                pl2 = plistlib.readPlist(fp)

        self.assertEqual(pl_out, pl2)

    def test_bytes_deprecated(self):
        pl = {
            'key': 42,
            'sub': {
                'key': 9,
                'alt': 'value',
                'data': b'buffer',
            }
        }
        with self.assertWarns(DeprecationWarning):
            data = plistlib.writePlistToBytes(pl)

        with self.assertWarns(DeprecationWarning):
            pl2 = plistlib.readPlistFromBytes(data)

        self.assertIsInstance(pl2, plistlib._InternalDict)
        self.assertEqual(pl2, plistlib._InternalDict(
            key=42,
            sub=plistlib._InternalDict(
                key=9,
                alt='value',
                data=plistlib.Data(b'buffer'),
            )
        ))

        with self.assertWarns(DeprecationWarning):
            data2 = plistlib.writePlistToBytes(pl2)
        self.assertEqual(data, data2)

    def test_dataobject_deprecated(self):
        in_data = { 'key': plistlib.Data(b'hello') }
        out_data = { 'key': b'hello' }

        buf = plistlib.dumps(in_data)

        cur = plistlib.loads(buf)
        self.assertEqual(cur, out_data)
        self.assertNotEqual(cur, in_data)

        cur = plistlib.loads(buf, use_builtin_types=False)
        self.assertNotEqual(cur, out_data)
        self.assertEqual(cur, in_data)

        with self.assertWarns(DeprecationWarning):
            cur = plistlib.readPlistFromBytes(buf)
        self.assertNotEqual(cur, out_data)
        self.assertEqual(cur, in_data)


def test_main():
    support.run_unittest(TestPlistlib, TestPlistlibDeprecated)


if __name__ == '__main__':
    test_main()
