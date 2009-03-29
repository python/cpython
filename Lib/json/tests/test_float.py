import math
from unittest import TestCase

import json

class TestFloat(TestCase):
    def test_floats(self):
        for num in [1617161771.7650001, math.pi, math.pi**100,
                    math.pi**-100, 3.1]:
            self.assertEquals(float(json.dumps(num)), num)
            self.assertEquals(json.loads(json.dumps(num)), num)
            self.assertEquals(json.loads(unicode(json.dumps(num))), num)

    def test_ints(self):
        for num in [1, 1L, 1<<32, 1<<64]:
            self.assertEquals(json.dumps(num), str(num))
            self.assertEquals(int(json.dumps(num)), num)
            self.assertEquals(json.loads(json.dumps(num)), num)
            self.assertEquals(json.loads(unicode(json.dumps(num))), num)
