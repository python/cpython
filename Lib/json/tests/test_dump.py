from unittest import TestCase
from cStringIO import StringIO

import json

class TestDump(TestCase):
    def test_dump(self):
        sio = StringIO()
        json.dump({}, sio)
        self.assertEquals(sio.getvalue(), '{}')

    def test_dumps(self):
        self.assertEquals(json.dumps({}), '{}')

    def test_encode_truefalse(self):
        self.assertEquals(json.dumps(
                 {True: False, False: True}, sort_keys=True),
                 '{"false": true, "true": false}')
        self.assertEquals(json.dumps(
                {2: 3.0, 4.0: 5L, False: 1, 6L: True}, sort_keys=True),
                '{"false": 1, "2": 3.0, "4.0": 5, "6": true}')
