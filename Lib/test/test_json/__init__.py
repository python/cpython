import os
import json
import doctest
import unittest

from test import support
from test.support import import_helper


# import json with and without accelerations
cjson = import_helper.import_fresh_module('json', fresh=['_json'])
pyjson = import_helper.import_fresh_module('json', blocked=['_json'])
# JSONDecodeError is cached inside the _json module
cjson.JSONDecodeError = cjson.decoder.JSONDecodeError = json.JSONDecodeError

# create two base classes that will be used by the other tests
class PyTest(unittest.TestCase):
    json = pyjson
    loads = staticmethod(pyjson.loads)
    dumps = staticmethod(pyjson.dumps)
    JSONDecodeError = staticmethod(pyjson.JSONDecodeError)

@unittest.skipUnless(cjson, 'requires _json')
class CTest(unittest.TestCase):
    if cjson is not None:
        json = cjson
        loads = staticmethod(cjson.loads)
        dumps = staticmethod(cjson.dumps)
        JSONDecodeError = staticmethod(cjson.JSONDecodeError)

# test PyTest and CTest checking if the functions come from the right module
class TestPyTest(PyTest):
    def test_pyjson(self):
        self.assertEqual(self.json.scanner.make_scanner.__module__,
                         'json.scanner')
        self.assertEqual(self.json.decoder.scanstring.__module__,
                         'json.decoder')
        self.assertEqual(self.json.encoder.encode_basestring_ascii.__module__,
                         'json.encoder')

class TestCTest(CTest):
    def test_cjson(self):
        self.assertEqual(self.json.scanner.make_scanner.__module__, '_json')
        self.assertEqual(self.json.decoder.scanstring.__module__, '_json')
        self.assertEqual(self.json.encoder.c_make_encoder.__module__, '_json')
        self.assertEqual(self.json.encoder.encode_basestring_ascii.__module__,
                         '_json')


def load_tests(loader, _, pattern):
    suite = unittest.TestSuite()
    for mod in (json, json.encoder, json.decoder):
        suite.addTest(doctest.DocTestSuite(mod))
    suite.addTest(TestPyTest('test_pyjson'))
    suite.addTest(TestCTest('test_cjson'))

    pkg_dir = os.path.dirname(__file__)
    return support.load_package_tests(pkg_dir, loader, suite, pattern)
