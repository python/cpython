from __future__ import unicode_literals

import unittest
from test import test_support

class TestFuture(unittest.TestCase):
    def assertType(self, obj, typ):
        self.assert_(type(obj) is typ,
            "type(%r) is %r, not %r" % (obj, type(obj), typ))

    def test_unicode_strings(self):
        self.assertType("", unicode)
        self.assertType('', unicode)
        self.assertType(r"", unicode)
        self.assertType(r'', unicode)
        self.assertType(""" """, unicode)
        self.assertType(''' ''', unicode)
        self.assertType(r""" """, unicode)
        self.assertType(r''' ''', unicode)
        self.assertType(u"", unicode)
        self.assertType(u'', unicode)
        self.assertType(ur"", unicode)
        self.assertType(ur'', unicode)
        self.assertType(u""" """, unicode)
        self.assertType(u''' ''', unicode)
        self.assertType(ur""" """, unicode)
        self.assertType(ur''' ''', unicode)

        self.assertType(b"", str)
        self.assertType(b'', str)
        self.assertType(br"", str)
        self.assertType(br'', str)
        self.assertType(b""" """, str)
        self.assertType(b''' ''', str)
        self.assertType(br""" """, str)
        self.assertType(br''' ''', str)

        self.assertType('' '', unicode)
        self.assertType('' u'', unicode)
        self.assertType(u'' '', unicode)
        self.assertType(u'' u'', unicode)

def test_main():
    test_support.run_unittest(TestFuture)

if __name__ == "__main__":
    test_main()
