# Copyright (C) 2003 Python Software Foundation

import unittest
import plistlib
import os
from test import test_support

class TestPlistlib(unittest.TestCase):

    def tearDown(self):
        try:
            os.unlink(test_support.TESTFN)
        except:
            pass

    def _create(self):
        pl = plistlib.Plist(
            aString="Doodah",
            aList=["A", "B", 12, 32.1, [1, 2, 3]],
            aFloat = 0.1,
            anInt = 728,
            aDict=plistlib.Dict(
                anotherString="<hello & hi there!>",
                aUnicodeValue=u'M\xe4ssig, Ma\xdf',
                aTrueValue=True,
                aFalseValue=False,
            ),
            someData = plistlib.Data("<binary gunk>"),
            someMoreData = plistlib.Data("<lots of binary gunk>" * 10),
        )
        pl['anotherInt'] = 42
        try:
            from xml.utils.iso8601 import parse
            import time
        except ImportError:
            pass
        else:
            pl['aDate'] = plistlib.Date(time.mktime(time.gmtime()))
        return pl

    def test_create(self):
        pl = self._create()
        self.assertEqual(pl["aString"], "Doodah")
        self.assertEqual(pl["aDict"]["aFalseValue"], False)

    def test_io(self):
        pl = self._create()
        pl.write(test_support.TESTFN)
        pl2 = plistlib.Plist.fromFile(test_support.TESTFN)
        self.assertEqual(dict(pl), dict(pl2))

    def test_stringio(self):
        from StringIO import StringIO
        f = StringIO()
        pl = self._create()
        pl.write(f)
        pl2 = plistlib.Plist.fromFile(StringIO(f.getvalue()))
        self.assertEqual(dict(pl), dict(pl2))

    def test_cstringio(self):
        from cStringIO import StringIO
        f = StringIO()
        pl = self._create()
        pl.write(f)
        pl2 = plistlib.Plist.fromFile(StringIO(f.getvalue()))
        self.assertEqual(dict(pl), dict(pl2))



def test_main():
    test_support.run_unittest(TestPlistlib)


if __name__ == '__main__':
    test_main()
