import sys
import test_support
import unittest
import xmlrpclib

alist = [{'astring': 'foo@bar.baz.spam',
          'afloat': 7283.43,
          'anint': 2**20,
          'ashortlong': 2L,
          'anotherlist': ['.zyx.41'],
          'abase64': xmlrpclib.Binary("my dog has fleas"),
          'boolean': xmlrpclib.False,
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

def test_main():
    test_support.run_unittest(XMLRPCTestCase)


if __name__ == "__main__":
    test_main()
