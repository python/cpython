from unittest import TestCase

import json

# from http://json.org/JSON_checker/test/pass3.json
JSON = r'''
{
    "JSON Test Pattern pass3": {
        "The outermost value": "must be an object or array.",
        "In this test": "It is an object."
    }
}
'''

class TestPass3(TestCase):
    def test_parse(self):
        # test in/out equivalence and parsing
        res = json.loads(JSON)
        out = json.dumps(res)
        self.assertEquals(res, json.loads(out))
