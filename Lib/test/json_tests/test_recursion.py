from unittest import TestCase

import json

class JSONTestObject:
    pass


class RecursiveJSONEncoder(json.JSONEncoder):
    recurse = False
    def default(self, o):
        if o is JSONTestObject:
            if self.recurse:
                return [JSONTestObject]
            else:
                return 'JSONTestObject'
        return json.JSONEncoder.default(o)


class TestRecursion(TestCase):
    def test_listrecursion(self):
        x = []
        x.append(x)
        try:
            json.dumps(x)
        except ValueError:
            pass
        else:
            self.fail("didn't raise ValueError on list recursion")
        x = []
        y = [x]
        x.append(y)
        try:
            json.dumps(x)
        except ValueError:
            pass
        else:
            self.fail("didn't raise ValueError on alternating list recursion")
        y = []
        x = [y, y]
        # ensure that the marker is cleared
        json.dumps(x)

    def test_dictrecursion(self):
        x = {}
        x["test"] = x
        try:
            json.dumps(x)
        except ValueError:
            pass
        else:
            self.fail("didn't raise ValueError on dict recursion")
        x = {}
        y = {"a": x, "b": x}
        # ensure that the marker is cleared
        json.dumps(x)

    def test_defaultrecursion(self):
        enc = RecursiveJSONEncoder()
        self.assertEqual(enc.encode(JSONTestObject), '"JSONTestObject"')
        enc.recurse = True
        try:
            enc.encode(JSONTestObject)
        except ValueError:
            pass
        else:
            self.fail("didn't raise ValueError on default recursion")


    def test_highly_nested_objects(self):
        # test that loading highly-nested objects doesn't segfault when C
        # accelerations are used. See #12017
        with self.assertRaises(RuntimeError):
            json.loads('{"a":' * 100000 + '1' + '}' * 100000)
        with self.assertRaises(RuntimeError):
            json.loads('{"a":' * 100000 + '[1]' + '}' * 100000)
        with self.assertRaises(RuntimeError):
            json.loads('[' * 100000 + '1' + ']' * 100000)

