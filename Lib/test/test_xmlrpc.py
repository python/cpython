import test_support
import unittest
import xmlrpclib

alist = [{'astring': 'foo@bar.baz.spam',
          'afloat': 7283.43,
          'anotherlist': ['.zyx.41'],
          'abase64': xmlrpclib.Binary("my dog has fleas"),
          'boolean': xmlrpclib.False,
          }]

class XMLRPCTestCase(unittest.TestCase):

    def test_dump_load(self):
        self.assertEquals(alist,
                          xmlrpclib.loads(xmlrpclib.dumps((alist,)))[0][0])

def test_main():
    test_support.run_unittest(XMLRPCTestCase)


if __name__ == "__main__":
    test_main()
