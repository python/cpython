""" Test script for the unicodedata module.

    Written by Marc-Andre Lemburg (mal@lemburg.com).

    (c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""#"
import unittest, test.test_support
import hashlib, sys

encoding = 'utf-8'


### Run tests

class UnicodeMethodsTest(unittest.TestCase):

    # update this, if the database changes
    expectedchecksum = 'c198ed264497f108434b3f576d4107237221cc8a'

    def test_method_checksum(self):
        h = hashlib.sha1()
        for i in range(65536):
            char = unichr(i)
            data = [
                # Predicates (single char)
                u"01"[char.isalnum()],
                u"01"[char.isalpha()],
                u"01"[char.isdecimal()],
                u"01"[char.isdigit()],
                u"01"[char.islower()],
                u"01"[char.isnumeric()],
                u"01"[char.isspace()],
                u"01"[char.istitle()],
                u"01"[char.isupper()],

                # Predicates (multiple chars)
                u"01"[(char + u'abc').isalnum()],
                u"01"[(char + u'abc').isalpha()],
                u"01"[(char + u'123').isdecimal()],
                u"01"[(char + u'123').isdigit()],
                u"01"[(char + u'abc').islower()],
                u"01"[(char + u'123').isnumeric()],
                u"01"[(char + u' \t').isspace()],
                u"01"[(char + u'abc').istitle()],
                u"01"[(char + u'ABC').isupper()],

                # Mappings (single char)
                char.lower(),
                char.upper(),
                char.title(),

                # Mappings (multiple chars)
                (char + u'abc').lower(),
                (char + u'ABC').upper(),
                (char + u'abc').title(),
                (char + u'ABC').title(),

                ]
            h.update(u''.join(data).encode(encoding))
        result = h.hexdigest()
        self.assertEqual(result, self.expectedchecksum)

class UnicodeDatabaseTest(unittest.TestCase):

    def setUp(self):
        # In case unicodedata is not available, this will raise an ImportError,
        # but the other test cases will still be run
        import unicodedata
        self.db = unicodedata

    def tearDown(self):
        del self.db

class UnicodeFunctionsTest(UnicodeDatabaseTest):

    # update this, if the database changes
    expectedchecksum = '4e389f97e9f88b8b7ab743121fd643089116f9f2'

    def test_function_checksum(self):
        data = []
        h = hashlib.sha1()

        for i in range(0x10000):
            char = unichr(i)
            data = [
                # Properties
                str(self.db.digit(char, -1)),
                str(self.db.numeric(char, -1)),
                str(self.db.decimal(char, -1)),
                self.db.category(char),
                self.db.bidirectional(char),
                self.db.decomposition(char),
                str(self.db.mirrored(char)),
                str(self.db.combining(char)),
            ]
            h.update(''.join(data))
        result = h.hexdigest()
        self.assertEqual(result, self.expectedchecksum)

    def test_digit(self):
        self.assertEqual(self.db.digit(u'A', None), None)
        self.assertEqual(self.db.digit(u'9'), 9)
        self.assertEqual(self.db.digit(u'\u215b', None), None)
        self.assertEqual(self.db.digit(u'\u2468'), 9)

        self.assertRaises(TypeError, self.db.digit)
        self.assertRaises(TypeError, self.db.digit, u'xx')
        self.assertRaises(ValueError, self.db.digit, u'x')

    def test_numeric(self):
        self.assertEqual(self.db.numeric(u'A',None), None)
        self.assertEqual(self.db.numeric(u'9'), 9)
        self.assertEqual(self.db.numeric(u'\u215b'), 0.125)
        self.assertEqual(self.db.numeric(u'\u2468'), 9.0)

        self.assertRaises(TypeError, self.db.numeric)
        self.assertRaises(TypeError, self.db.numeric, u'xx')
        self.assertRaises(ValueError, self.db.numeric, u'x')

    def test_decimal(self):
        self.assertEqual(self.db.decimal(u'A',None), None)
        self.assertEqual(self.db.decimal(u'9'), 9)
        self.assertEqual(self.db.decimal(u'\u215b', None), None)
        self.assertEqual(self.db.decimal(u'\u2468', None), None)

        self.assertRaises(TypeError, self.db.decimal)
        self.assertRaises(TypeError, self.db.decimal, u'xx')
        self.assertRaises(ValueError, self.db.decimal, u'x')

    def test_category(self):
        self.assertEqual(self.db.category(u'\uFFFE'), 'Cn')
        self.assertEqual(self.db.category(u'a'), 'Ll')
        self.assertEqual(self.db.category(u'A'), 'Lu')

        self.assertRaises(TypeError, self.db.category)
        self.assertRaises(TypeError, self.db.category, u'xx')

    def test_bidirectional(self):
        self.assertEqual(self.db.bidirectional(u'\uFFFE'), '')
        self.assertEqual(self.db.bidirectional(u' '), 'WS')
        self.assertEqual(self.db.bidirectional(u'A'), 'L')

        self.assertRaises(TypeError, self.db.bidirectional)
        self.assertRaises(TypeError, self.db.bidirectional, u'xx')

    def test_decomposition(self):
        self.assertEqual(self.db.decomposition(u'\uFFFE'),'')
        self.assertEqual(self.db.decomposition(u'\u00bc'), '<fraction> 0031 2044 0034')

        self.assertRaises(TypeError, self.db.decomposition)
        self.assertRaises(TypeError, self.db.decomposition, u'xx')

    def test_mirrored(self):
        self.assertEqual(self.db.mirrored(u'\uFFFE'), 0)
        self.assertEqual(self.db.mirrored(u'a'), 0)
        self.assertEqual(self.db.mirrored(u'\u2201'), 1)

        self.assertRaises(TypeError, self.db.mirrored)
        self.assertRaises(TypeError, self.db.mirrored, u'xx')

    def test_combining(self):
        self.assertEqual(self.db.combining(u'\uFFFE'), 0)
        self.assertEqual(self.db.combining(u'a'), 0)
        self.assertEqual(self.db.combining(u'\u20e1'), 230)

        self.assertRaises(TypeError, self.db.combining)
        self.assertRaises(TypeError, self.db.combining, u'xx')

    def test_normalize(self):
        self.assertRaises(TypeError, self.db.normalize)
        self.assertRaises(ValueError, self.db.normalize, 'unknown', u'xx')
        self.assertEqual(self.db.normalize('NFKC', u''), u'')
        # The rest can be found in test_normalization.py
        # which requires an external file.

    def test_east_asian_width(self):
        eaw = self.db.east_asian_width
        self.assertRaises(TypeError, eaw, 'a')
        self.assertRaises(TypeError, eaw, u'')
        self.assertRaises(TypeError, eaw, u'ra')
        self.assertEqual(eaw(u'\x1e'), 'N')
        self.assertEqual(eaw(u'\x20'), 'Na')
        self.assertEqual(eaw(u'\uC894'), 'W')
        self.assertEqual(eaw(u'\uFF66'), 'H')
        self.assertEqual(eaw(u'\uFF1F'), 'F')
        self.assertEqual(eaw(u'\u2010'), 'A')

class UnicodeMiscTest(UnicodeDatabaseTest):

    def test_decimal_numeric_consistent(self):
        # Test that decimal and numeric are consistent,
        # i.e. if a character has a decimal value,
        # its numeric value should be the same.
        count = 0
        for i in xrange(0x10000):
            c = unichr(i)
            dec = self.db.decimal(c, -1)
            if dec != -1:
                self.assertEqual(dec, self.db.numeric(c))
                count += 1
        self.assert_(count >= 10) # should have tested at least the ASCII digits

    def test_digit_numeric_consistent(self):
        # Test that digit and numeric are consistent,
        # i.e. if a character has a digit value,
        # its numeric value should be the same.
        count = 0
        for i in xrange(0x10000):
            c = unichr(i)
            dec = self.db.digit(c, -1)
            if dec != -1:
                self.assertEqual(dec, self.db.numeric(c))
                count += 1
        self.assert_(count >= 10) # should have tested at least the ASCII digits

    def test_bug_1704793(self):
        if sys.maxunicode == 65535:
            self.assertRaises(KeyError, self.db.lookup, "GOTHIC LETTER FAIHU")

def test_main():
    test.test_support.run_unittest(
        UnicodeMiscTest,
        UnicodeMethodsTest,
        UnicodeFunctionsTest
    )

if __name__ == "__main__":
    test_main()
