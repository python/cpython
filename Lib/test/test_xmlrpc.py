import sys
import unittest
import xmlrpclib
from test import test_support

try:
    unicode
except NameError:
    have_unicode = False
else:
    have_unicode = True

alist = [{'astring': 'foo@bar.baz.spam',
          'afloat': 7283.43,
          'anint': 2**20,
          'ashortlong': 2L,
          'anotherlist': ['.zyx.41'],
          'abase64': xmlrpclib.Binary("my dog has fleas"),
          'boolean': xmlrpclib.False,
          'unicode': u'\u4000\u6000\u8000',
          u'ukey\u4000': 'regular value',
          }]

class XMLRPCTestCase(unittest.TestCase):

    def test_dump_load(self):
        self.assertEquals(alist,
                          xmlrpclib.loads(xmlrpclib.dumps((alist,)))[0][0])

    def test_dump_big_long(self):
        self.assertRaises(OverflowError, xmlrpclib.dumps, (2L**99,))

    def test_dump_bad_dict(self):
        self.assertRaises(TypeError, xmlrpclib.dumps, ({(1,2,3): 1},))

    def test_dump_big_int(self):
        if sys.maxint > 2L**31-1:
            self.assertRaises(OverflowError, xmlrpclib.dumps,
                              (int(2L**34),))

    def test_dump_none(self):
        value = alist + [None]
        arg1 = (alist + [None],)
        strg = xmlrpclib.dumps(arg1, allow_none=True)
        self.assertEquals(value,
                          xmlrpclib.loads(strg)[0][0])
        self.assertRaises(TypeError, xmlrpclib.dumps, (arg1,))

    def test_default_encoding_issues(self):
        # SF bug #1115989: wrong decoding in '_stringify'
        utf8 = """<?xml version='1.0' encoding='iso-8859-1'?>
                  <params>
                    <param><value>
                      <string>abc \x95</string>
                      </value></param>
                    <param><value>
                      <struct>
                        <member>
                          <name>def \x96</name>
                          <value><string>ghi \x97</string></value>
                          </member>
                        </struct>
                      </value></param>
                  </params>
                  """
        old_encoding = sys.getdefaultencoding()
        reload(sys) # ugh!
        sys.setdefaultencoding("iso-8859-1")
        try:
            (s, d), m = xmlrpclib.loads(utf8)
        finally:
            sys.setdefaultencoding(old_encoding)
        items = d.items()
        if have_unicode:
            self.assertEquals(s, u"abc \x95")
            self.assert_(isinstance(s, unicode))
            self.assertEquals(items, [(u"def \x96", u"ghi \x97")])
            self.assert_(isinstance(items[0][0], unicode))
            self.assert_(isinstance(items[0][1], unicode))
        else:
            self.assertEquals(s, "abc \xc2\x95")
            self.assertEquals(items, [("def \xc2\x96", "ghi \xc2\x97")])

def test_main():
    test_support.run_unittest(XMLRPCTestCase)


if __name__ == "__main__":
    test_main()
