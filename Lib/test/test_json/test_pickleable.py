import json
import pickle
import unittest

class TestPickleable(unittest.TestCase):
    def test_json_decode_error_is_pickleable(self):
        e = json.JSONDecodeError(msg="abc", doc="def", pos=7)

        pickled = pickle.dumps(e)
        unpickled = pickle.loads(pickled)

        self.assertEqual(unpickled.msg, e.msg)
        self.assertEqual(unpickled.doc, e.doc)
        self.assertEqual(unpickled.pos, e.pos)
