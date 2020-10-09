# Copyright (C) 2003-2013 Python Software Foundation
import copy
import operator
import pickle
import unittest
import plistlib
import os
import datetime
import codecs
import binascii
import collections
from test import support
from test.support import os_helper
from io import BytesIO

from plistlib import UID

ALL_FORMATS=(plistlib.FMT_XML, plistlib.FMT_BINARY)

# The testdata is generated using Mac/Tools/plistlib_generate_testdata.py
# (which using PyObjC to control the Cocoa classes for generating plists)
TESTDATA={
    plistlib.FMT_XML: binascii.a2b_base64(b'''
        PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiPz4KPCFET0NU
        WVBFIHBsaXN0IFBVQkxJQyAiLS8vQXBwbGUvL0RURCBQTElTVCAxLjAvL0VO
        IiAiaHR0cDovL3d3dy5hcHBsZS5jb20vRFREcy9Qcm9wZXJ0eUxpc3QtMS4w
        LmR0ZCI+CjxwbGlzdCB2ZXJzaW9uPSIxLjAiPgo8ZGljdD4KCTxrZXk+YUJp
        Z0ludDwva2V5PgoJPGludGVnZXI+OTIyMzM3MjAzNjg1NDc3NTc2NDwvaW50
        ZWdlcj4KCTxrZXk+YUJpZ0ludDI8L2tleT4KCTxpbnRlZ2VyPjkyMjMzNzIw
        MzY4NTQ3NzU4NTI8L2ludGVnZXI+Cgk8a2V5PmFEYXRlPC9rZXk+Cgk8ZGF0
        ZT4yMDA0LTEwLTI2VDEwOjMzOjMzWjwvZGF0ZT4KCTxrZXk+YURpY3Q8L2tl
        eT4KCTxkaWN0PgoJCTxrZXk+YUZhbHNlVmFsdWU8L2tleT4KCQk8ZmFsc2Uv
        PgoJCTxrZXk+YVRydWVWYWx1ZTwva2V5PgoJCTx0cnVlLz4KCQk8a2V5PmFV
        bmljb2RlVmFsdWU8L2tleT4KCQk8c3RyaW5nPk3DpHNzaWcsIE1hw588L3N0
        cmluZz4KCQk8a2V5PmFub3RoZXJTdHJpbmc8L2tleT4KCQk8c3RyaW5nPiZs
        dDtoZWxsbyAmYW1wOyAnaGknIHRoZXJlISZndDs8L3N0cmluZz4KCQk8a2V5
        PmRlZXBlckRpY3Q8L2tleT4KCQk8ZGljdD4KCQkJPGtleT5hPC9rZXk+CgkJ
        CTxpbnRlZ2VyPjE3PC9pbnRlZ2VyPgoJCQk8a2V5PmI8L2tleT4KCQkJPHJl
        YWw+MzIuNTwvcmVhbD4KCQkJPGtleT5jPC9rZXk+CgkJCTxhcnJheT4KCQkJ
        CTxpbnRlZ2VyPjE8L2ludGVnZXI+CgkJCQk8aW50ZWdlcj4yPC9pbnRlZ2Vy
        PgoJCQkJPHN0cmluZz50ZXh0PC9zdHJpbmc+CgkJCTwvYXJyYXk+CgkJPC9k
        aWN0PgoJPC9kaWN0PgoJPGtleT5hRmxvYXQ8L2tleT4KCTxyZWFsPjAuNTwv
        cmVhbD4KCTxrZXk+YUxpc3Q8L2tleT4KCTxhcnJheT4KCQk8c3RyaW5nPkE8
        L3N0cmluZz4KCQk8c3RyaW5nPkI8L3N0cmluZz4KCQk8aW50ZWdlcj4xMjwv
        aW50ZWdlcj4KCQk8cmVhbD4zMi41PC9yZWFsPgoJCTxhcnJheT4KCQkJPGlu
        dGVnZXI+MTwvaW50ZWdlcj4KCQkJPGludGVnZXI+MjwvaW50ZWdlcj4KCQkJ
        PGludGVnZXI+MzwvaW50ZWdlcj4KCQk8L2FycmF5PgoJPC9hcnJheT4KCTxr
        ZXk+YU5lZ2F0aXZlQmlnSW50PC9rZXk+Cgk8aW50ZWdlcj4tODAwMDAwMDAw
        MDA8L2ludGVnZXI+Cgk8a2V5PmFOZWdhdGl2ZUludDwva2V5PgoJPGludGVn
        ZXI+LTU8L2ludGVnZXI+Cgk8a2V5PmFTdHJpbmc8L2tleT4KCTxzdHJpbmc+
        RG9vZGFoPC9zdHJpbmc+Cgk8a2V5PmFuRW1wdHlEaWN0PC9rZXk+Cgk8ZGlj
        dC8+Cgk8a2V5PmFuRW1wdHlMaXN0PC9rZXk+Cgk8YXJyYXkvPgoJPGtleT5h
        bkludDwva2V5PgoJPGludGVnZXI+NzI4PC9pbnRlZ2VyPgoJPGtleT5uZXN0
        ZWREYXRhPC9rZXk+Cgk8YXJyYXk+CgkJPGRhdGE+CgkJUEd4dmRITWdiMlln
        WW1sdVlYSjVJR2QxYm1zK0FBRUNBenhzYjNSeklHOW1JR0pwYm1GeWVTQm5k
        VzVyCgkJUGdBQkFnTThiRzkwY3lCdlppQmlhVzVoY25rZ1ozVnVhejRBQVFJ
        RFBHeHZkSE1nYjJZZ1ltbHVZWEo1CgkJSUdkMWJtcytBQUVDQXp4c2IzUnpJ
        RzltSUdKcGJtRnllU0JuZFc1clBnQUJBZ004Ykc5MGN5QnZaaUJpCgkJYVc1
        aGNua2daM1Z1YXo0QUFRSURQR3h2ZEhNZ2IyWWdZbWx1WVhKNUlHZDFibXMr
        QUFFQ0F6eHNiM1J6CgkJSUc5bUlHSnBibUZ5ZVNCbmRXNXJQZ0FCQWdNOGJH
        OTBjeUJ2WmlCaWFXNWhjbmtnWjNWdWF6NEFBUUlECgkJUEd4dmRITWdiMlln
        WW1sdVlYSjVJR2QxYm1zK0FBRUNBdz09CgkJPC9kYXRhPgoJPC9hcnJheT4K
        CTxrZXk+c29tZURhdGE8L2tleT4KCTxkYXRhPgoJUEdKcGJtRnllU0JuZFc1
        clBnPT0KCTwvZGF0YT4KCTxrZXk+c29tZU1vcmVEYXRhPC9rZXk+Cgk8ZGF0
        YT4KCVBHeHZkSE1nYjJZZ1ltbHVZWEo1SUdkMWJtcytBQUVDQXp4c2IzUnpJ
        RzltSUdKcGJtRnllU0JuZFc1clBnQUJBZ004CgliRzkwY3lCdlppQmlhVzVo
        Y25rZ1ozVnVhejRBQVFJRFBHeHZkSE1nYjJZZ1ltbHVZWEo1SUdkMWJtcytB
        QUVDQXp4cwoJYjNSeklHOW1JR0pwYm1GeWVTQm5kVzVyUGdBQkFnTThiRzkw
        Y3lCdlppQmlhVzVoY25rZ1ozVnVhejRBQVFJRFBHeHYKCWRITWdiMllnWW1s
        dVlYSjVJR2QxYm1zK0FBRUNBenhzYjNSeklHOW1JR0pwYm1GeWVTQm5kVzVy
        UGdBQkFnTThiRzkwCgljeUJ2WmlCaWFXNWhjbmtnWjNWdWF6NEFBUUlEUEd4
        dmRITWdiMllnWW1sdVlYSjVJR2QxYm1zK0FBRUNBdz09Cgk8L2RhdGE+Cgk8
        a2V5PsOFYmVucmFhPC9rZXk+Cgk8c3RyaW5nPlRoYXQgd2FzIGEgdW5pY29k
        ZSBrZXkuPC9zdHJpbmc+CjwvZGljdD4KPC9wbGlzdD4K'''),
    plistlib.FMT_BINARY: binascii.a2b_base64(b'''
        YnBsaXN0MDDfEBABAgMEBQYHCAkKCwwNDg8QERITFCgpLzAxMjM0NTc2OFdh
        QmlnSW50WGFCaWdJbnQyVWFEYXRlVWFEaWN0VmFGbG9hdFVhTGlzdF8QD2FO
        ZWdhdGl2ZUJpZ0ludFxhTmVnYXRpdmVJbnRXYVN0cmluZ1thbkVtcHR5RGlj
        dFthbkVtcHR5TGlzdFVhbkludFpuZXN0ZWREYXRhWHNvbWVEYXRhXHNvbWVN
        b3JlRGF0YWcAxQBiAGUAbgByAGEAYRN/////////1BQAAAAAAAAAAIAAAAAA
        AAAsM0GcuX30AAAA1RUWFxgZGhscHR5bYUZhbHNlVmFsdWVaYVRydWVWYWx1
        ZV1hVW5pY29kZVZhbHVlXWFub3RoZXJTdHJpbmdaZGVlcGVyRGljdAgJawBN
        AOQAcwBzAGkAZwAsACAATQBhAN9fEBU8aGVsbG8gJiAnaGknIHRoZXJlIT7T
        HyAhIiMkUWFRYlFjEBEjQEBAAAAAAACjJSYnEAEQAlR0ZXh0Iz/gAAAAAAAA
        pSorLCMtUUFRQhAMoyUmLhADE////+1foOAAE//////////7VkRvb2RhaNCg
        EQLYoTZPEPo8bG90cyBvZiBiaW5hcnkgZ3Vuaz4AAQIDPGxvdHMgb2YgYmlu
        YXJ5IGd1bms+AAECAzxsb3RzIG9mIGJpbmFyeSBndW5rPgABAgM8bG90cyBv
        ZiBiaW5hcnkgZ3Vuaz4AAQIDPGxvdHMgb2YgYmluYXJ5IGd1bms+AAECAzxs
        b3RzIG9mIGJpbmFyeSBndW5rPgABAgM8bG90cyBvZiBiaW5hcnkgZ3Vuaz4A
        AQIDPGxvdHMgb2YgYmluYXJ5IGd1bms+AAECAzxsb3RzIG9mIGJpbmFyeSBn
        dW5rPgABAgM8bG90cyBvZiBiaW5hcnkgZ3Vuaz4AAQIDTTxiaW5hcnkgZ3Vu
        az5fEBdUaGF0IHdhcyBhIHVuaWNvZGUga2V5LgAIACsAMwA8AEIASABPAFUA
        ZwB0AHwAiACUAJoApQCuALsAygDTAOQA7QD4AQQBDwEdASsBNgE3ATgBTwFn
        AW4BcAFyAXQBdgF/AYMBhQGHAYwBlQGbAZ0BnwGhAaUBpwGwAbkBwAHBAcIB
        xQHHAsQC0gAAAAAAAAIBAAAAAAAAADkAAAAAAAAAAAAAAAAAAALs'''),
    'KEYED_ARCHIVE': binascii.a2b_base64(b'''
        YnBsaXN0MDDUAQIDBAUGHB1YJHZlcnNpb25YJG9iamVjdHNZJGFyY2hpdmVy
        VCR0b3ASAAGGoKMHCA9VJG51bGzTCQoLDA0OVnB5dHlwZVYkY2xhc3NZTlMu
        c3RyaW5nEAGAAl8QE0tleUFyY2hpdmUgVUlEIFRlc3TTEBESExQZWiRjbGFz
        c25hbWVYJGNsYXNzZXNbJGNsYXNzaGludHNfEBdPQ19CdWlsdGluUHl0aG9u
        VW5pY29kZaQVFhcYXxAXT0NfQnVpbHRpblB5dGhvblVuaWNvZGVfEBBPQ19Q
        eXRob25Vbmljb2RlWE5TU3RyaW5nWE5TT2JqZWN0ohobXxAPT0NfUHl0aG9u
        U3RyaW5nWE5TU3RyaW5nXxAPTlNLZXllZEFyY2hpdmVy0R4fVHJvb3SAAQAI
        ABEAGgAjAC0AMgA3ADsAQQBIAE8AVgBgAGIAZAB6AIEAjACVAKEAuwDAANoA
        7QD2AP8BAgEUAR0BLwEyATcAAAAAAAACAQAAAAAAAAAgAAAAAAAAAAAAAAAA
        AAABOQ=='''),
}


class TestPlistlib(unittest.TestCase):

    def tearDown(self):
        try:
            os.unlink(os_helper.TESTFN)
        except:
            pass

    def _create(self, fmt=None):
        pl = dict(
            aString="Doodah",
            aList=["A", "B", 12, 32.5, [1, 2, 3]],
            aFloat = 0.5,
            anInt = 728,
            aBigInt = 2 ** 63 - 44,
            aBigInt2 = 2 ** 63 + 44,
            aNegativeInt = -5,
            aNegativeBigInt = -80000000000,
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
        with open(os_helper.TESTFN, 'wb') as fp:
            plistlib.dump(pl, fp)

        with open(os_helper.TESTFN, 'rb') as fp:
            pl2 = plistlib.load(fp)

        self.assertEqual(dict(pl), dict(pl2))

        self.assertRaises(AttributeError, plistlib.dump, pl, 'filename')
        self.assertRaises(AttributeError, plistlib.load, 'filename')

    def test_invalid_type(self):
        pl = [ object() ]

        for fmt in ALL_FORMATS:
            with self.subTest(fmt=fmt):
                self.assertRaises(TypeError, plistlib.dumps, pl, fmt=fmt)

    def test_invalid_uid(self):
        with self.assertRaises(TypeError):
            UID("not an int")
        with self.assertRaises(ValueError):
            UID(2 ** 64)
        with self.assertRaises(ValueError):
            UID(-19)

    def test_int(self):
        for pl in [0, 2**8-1, 2**8, 2**16-1, 2**16, 2**32-1, 2**32,
                   2**63-1, 2**64-1, 1, -2**63]:
            for fmt in ALL_FORMATS:
                with self.subTest(pl=pl, fmt=fmt):
                    data = plistlib.dumps(pl, fmt=fmt)
                    pl2 = plistlib.loads(data)
                    self.assertIsInstance(pl2, int)
                    self.assertEqual(pl, pl2)
                    data2 = plistlib.dumps(pl2, fmt=fmt)
                    self.assertEqual(data, data2)

        for fmt in ALL_FORMATS:
            for pl in (2 ** 64 + 1, 2 ** 127-1, -2**64, -2 ** 127):
                with self.subTest(pl=pl, fmt=fmt):
                    self.assertRaises(OverflowError, plistlib.dumps,
                                      pl, fmt=fmt)

    def test_bytearray(self):
        for pl in (b'<binary gunk>', b"<lots of binary gunk>\0\1\2\3" * 10):
            for fmt in ALL_FORMATS:
                with self.subTest(pl=pl, fmt=fmt):
                    data = plistlib.dumps(bytearray(pl), fmt=fmt)
                    pl2 = plistlib.loads(data)
                    self.assertIsInstance(pl2, bytes)
                    self.assertEqual(pl2, pl)
                    data2 = plistlib.dumps(pl2, fmt=fmt)
                    self.assertEqual(data, data2)

    def test_bytes(self):
        pl = self._create()
        data = plistlib.dumps(pl)
        pl2 = plistlib.loads(data)
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

    def test_uid(self):
        data = UID(1)
        self.assertEqual(plistlib.loads(plistlib.dumps(data, fmt=plistlib.FMT_BINARY)), data)
        dict_data = {
            'uid0': UID(0),
            'uid2': UID(2),
            'uid8': UID(2 ** 8),
            'uid16': UID(2 ** 16),
            'uid32': UID(2 ** 32),
            'uid63': UID(2 ** 63)
        }
        self.assertEqual(plistlib.loads(plistlib.dumps(dict_data, fmt=plistlib.FMT_BINARY)), dict_data)

    def test_uid_data(self):
        uid = UID(1)
        self.assertEqual(uid.data, 1)

    def test_uid_eq(self):
        self.assertEqual(UID(1), UID(1))
        self.assertNotEqual(UID(1), UID(2))
        self.assertNotEqual(UID(1), "not uid")

    def test_uid_hash(self):
        self.assertEqual(hash(UID(1)), hash(UID(1)))

    def test_uid_repr(self):
        self.assertEqual(repr(UID(1)), "UID(1)")

    def test_uid_index(self):
        self.assertEqual(operator.index(UID(1)), 1)

    def test_uid_pickle(self):
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            self.assertEqual(pickle.loads(pickle.dumps(UID(19), protocol=proto)), UID(19))

    def test_uid_copy(self):
        self.assertEqual(copy.copy(UID(1)), UID(1))
        self.assertEqual(copy.deepcopy(UID(1)), UID(1))

    def test_appleformatting(self):
        for fmt in ALL_FORMATS:
            with self.subTest(fmt=fmt):
                pl = plistlib.loads(TESTDATA[fmt])
                data = plistlib.dumps(pl, fmt=fmt)
                self.assertEqual(data, TESTDATA[fmt],
                    "generated data was not identical to Apple's output")


    def test_appleformattingfromliteral(self):
        self.maxDiff = None
        for fmt in ALL_FORMATS:
            with self.subTest(fmt=fmt):
                pl = self._create(fmt=fmt)
                pl2 = plistlib.loads(TESTDATA[fmt], fmt=fmt)
                self.assertEqual(dict(pl), dict(pl2),
                    "generated data was not identical to Apple's output")
                pl2 = plistlib.loads(TESTDATA[fmt])
                self.assertEqual(dict(pl), dict(pl2),
                    "generated data was not identical to Apple's output")

    def test_bytesio(self):
        for fmt in ALL_FORMATS:
            with self.subTest(fmt=fmt):
                b = BytesIO()
                pl = self._create(fmt=fmt)
                plistlib.dump(pl, b, fmt=fmt)
                pl2 = plistlib.load(BytesIO(b.getvalue()), fmt=fmt)
                self.assertEqual(dict(pl), dict(pl2))
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
                if fmt != plistlib.FMT_BINARY:
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
                data = plistlib.dumps(testString, fmt=plistlib.FMT_XML)
                if c != "\r":
                    self.assertEqual(plistlib.loads(data), testString)
            else:
                with self.assertRaises(ValueError):
                    plistlib.dumps(testString, fmt=plistlib.FMT_XML)
            plistlib.dumps(testString, fmt=plistlib.FMT_BINARY)

    def test_non_bmp_characters(self):
        pl = {'python': '\U0001f40d'}
        for fmt in ALL_FORMATS:
            with self.subTest(fmt=fmt):
                data = plistlib.dumps(pl, fmt=fmt)
                self.assertEqual(plistlib.loads(data), pl)

    def test_lone_surrogates(self):
        for fmt in ALL_FORMATS:
            with self.subTest(fmt=fmt):
                with self.assertRaises(UnicodeEncodeError):
                    plistlib.dumps('\ud8ff', fmt=fmt)
                with self.assertRaises(UnicodeEncodeError):
                    plistlib.dumps('\udcff', fmt=fmt)

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

    def test_dump_invalid_format(self):
        with self.assertRaises(ValueError):
            plistlib.dumps({}, fmt="blah")

    def test_load_invalid_file(self):
        with self.assertRaises(plistlib.InvalidFileException):
            plistlib.loads(b"these are not plist file contents")

    def test_modified_uid_negative(self):
        neg_uid = UID(1)
        neg_uid.data = -1  # dodge the negative check in the constructor
        with self.assertRaises(ValueError):
            plistlib.dumps(neg_uid, fmt=plistlib.FMT_BINARY)

    def test_modified_uid_huge(self):
        huge_uid = UID(1)
        huge_uid.data = 2 ** 64  # dodge the size check in the constructor
        with self.assertRaises(OverflowError):
            plistlib.dumps(huge_uid, fmt=plistlib.FMT_BINARY)


class TestBinaryPlistlib(unittest.TestCase):

    def test_nonstandard_refs_size(self):
        # Issue #21538: Refs and offsets are 24-bit integers
        data = (b'bplist00'
                b'\xd1\x00\x00\x01\x00\x00\x02QaQb'
                b'\x00\x00\x08\x00\x00\x0f\x00\x00\x11'
                b'\x00\x00\x00\x00\x00\x00'
                b'\x03\x03'
                b'\x00\x00\x00\x00\x00\x00\x00\x03'
                b'\x00\x00\x00\x00\x00\x00\x00\x00'
                b'\x00\x00\x00\x00\x00\x00\x00\x13')
        self.assertEqual(plistlib.loads(data), {'a': 'b'})

    def test_dump_duplicates(self):
        # Test effectiveness of saving duplicated objects
        for x in (None, False, True, 12345, 123.45, 'abcde', b'abcde',
                  datetime.datetime(2004, 10, 26, 10, 33, 33),
                  bytearray(b'abcde'), [12, 345], (12, 345), {'12': 345}):
            with self.subTest(x=x):
                data = plistlib.dumps([x]*1000, fmt=plistlib.FMT_BINARY)
                self.assertLess(len(data), 1100, repr(data))

    def test_identity(self):
        for x in (None, False, True, 12345, 123.45, 'abcde', b'abcde',
                  datetime.datetime(2004, 10, 26, 10, 33, 33),
                  bytearray(b'abcde'), [12, 345], (12, 345), {'12': 345}):
            with self.subTest(x=x):
                data = plistlib.dumps([x]*2, fmt=plistlib.FMT_BINARY)
                a, b = plistlib.loads(data)
                if isinstance(x, tuple):
                    x = list(x)
                self.assertEqual(a, x)
                self.assertEqual(b, x)
                self.assertIs(a, b)

    def test_cycles(self):
        # recursive list
        a = []
        a.append(a)
        b = plistlib.loads(plistlib.dumps(a, fmt=plistlib.FMT_BINARY))
        self.assertIs(b[0], b)
        # recursive tuple
        a = ([],)
        a[0].append(a)
        b = plistlib.loads(plistlib.dumps(a, fmt=plistlib.FMT_BINARY))
        self.assertIs(b[0][0], b)
        # recursive dict
        a = {}
        a['x'] = a
        b = plistlib.loads(plistlib.dumps(a, fmt=plistlib.FMT_BINARY))
        self.assertIs(b['x'], b)

    def test_large_timestamp(self):
        # Issue #26709: 32-bit timestamp out of range
        for ts in -2**31-1, 2**31:
            with self.subTest(ts=ts):
                d = (datetime.datetime.utcfromtimestamp(0) +
                     datetime.timedelta(seconds=ts))
                data = plistlib.dumps(d, fmt=plistlib.FMT_BINARY)
                self.assertEqual(plistlib.loads(data), d)

    def test_invalid_binary(self):
        for data in [
                # too short data
                b'',
                # too large offset_table_offset and nonstandard offset_size
                b'\x00\x08'
                b'\x00\x00\x00\x00\x00\x00\x03\x01'
                b'\x00\x00\x00\x00\x00\x00\x00\x01'
                b'\x00\x00\x00\x00\x00\x00\x00\x00'
                b'\x00\x00\x00\x00\x00\x00\x00\x2a',
                # integer overflow in offset_table_offset
                b'\x00\x08'
                b'\x00\x00\x00\x00\x00\x00\x01\x01'
                b'\x00\x00\x00\x00\x00\x00\x00\x01'
                b'\x00\x00\x00\x00\x00\x00\x00\x00'
                b'\xff\xff\xff\xff\xff\xff\xff\xff',
                # offset_size = 0
                b'\x00\x08'
                b'\x00\x00\x00\x00\x00\x00\x00\x01'
                b'\x00\x00\x00\x00\x00\x00\x00\x01'
                b'\x00\x00\x00\x00\x00\x00\x00\x00'
                b'\x00\x00\x00\x00\x00\x00\x00\x09',
                # ref_size = 0
                b'\xa1\x01\x00\x08\x0a'
                b'\x00\x00\x00\x00\x00\x00\x01\x00'
                b'\x00\x00\x00\x00\x00\x00\x00\x02'
                b'\x00\x00\x00\x00\x00\x00\x00\x00'
                b'\x00\x00\x00\x00\x00\x00\x00\x0b',
                # integer overflow in offset
                b'\x00\xff\xff\xff\xff\xff\xff\xff\xff'
                b'\x00\x00\x00\x00\x00\x00\x08\x01'
                b'\x00\x00\x00\x00\x00\x00\x00\x01'
                b'\x00\x00\x00\x00\x00\x00\x00\x00'
                b'\x00\x00\x00\x00\x00\x00\x00\x09',
                # invalid ASCII
                b'\x51\xff\x08'
                b'\x00\x00\x00\x00\x00\x00\x01\x01'
                b'\x00\x00\x00\x00\x00\x00\x00\x01'
                b'\x00\x00\x00\x00\x00\x00\x00\x00'
                b'\x00\x00\x00\x00\x00\x00\x00\x0a',
                # invalid UTF-16
                b'\x61\xd8\x00\x08'
                b'\x00\x00\x00\x00\x00\x00\x01\x01'
                b'\x00\x00\x00\x00\x00\x00\x00\x01'
                b'\x00\x00\x00\x00\x00\x00\x00\x00'
                b'\x00\x00\x00\x00\x00\x00\x00\x0b',
                ]:
            with self.assertRaises(plistlib.InvalidFileException):
                plistlib.loads(b'bplist00' + data, fmt=plistlib.FMT_BINARY)


class TestKeyedArchive(unittest.TestCase):
    def test_keyed_archive_data(self):
        # This is the structure of a NSKeyedArchive packed plist
        data = {
            '$version': 100000,
            '$objects': [
                '$null', {
                    'pytype': 1,
                    '$class': UID(2),
                    'NS.string': 'KeyArchive UID Test'
                },
                {
                    '$classname': 'OC_BuiltinPythonUnicode',
                    '$classes': [
                        'OC_BuiltinPythonUnicode',
                        'OC_PythonUnicode',
                        'NSString',
                        'NSObject'
                    ],
                    '$classhints': [
                        'OC_PythonString', 'NSString'
                    ]
                }
            ],
            '$archiver': 'NSKeyedArchiver',
            '$top': {
                'root': UID(1)
            }
        }
        self.assertEqual(plistlib.loads(TESTDATA["KEYED_ARCHIVE"]), data)


class MiscTestCase(unittest.TestCase):
    def test__all__(self):
        not_exported = {"PlistFormat", "PLISTHEADER"}
        support.check__all__(self, plistlib, not_exported=not_exported)


if __name__ == '__main__':
    unittest.main()
