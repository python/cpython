from unittest import TestCase
from io import StringIO

import json

class TestDump(TestCase):
    def test_dump(self):
        sio = StringIO()
        json.dump({}, sio)
        self.assertEqual(sio.getvalue(), '{}')

    def test_dumps(self):
        self.assertEqual(json.dumps({}), '{}')

    def test_encode_truefalse(self):
        self.assertEqual(json.dumps(
                 {True: False, False: True}, sort_keys=True),
                 '{"false": true, "true": false}')
        self.assertEqual(json.dumps(
                {2: 3.0, 4.0: 5, False: 1, 6: True}, sort_keys=True),
                '{"false": 1, "2": 3.0, "4.0": 5, "6": true}')
