from unittest import TestCase

import json
import textwrap

class TestIndent(TestCase):
    def test_indent(self):
        h = [['blorpie'], ['whoops'], [], 'd-shtaeou', 'd-nthiouh', 'i-vhbjkhnth',
             {'nifty': 87}, {'field': 'yes', 'morefield': False} ]

        expect = textwrap.dedent("""\
        [
          [
            "blorpie"
          ],
          [
            "whoops"
          ],
          [],
          "d-shtaeou",
          "d-nthiouh",
          "i-vhbjkhnth",
          {
            "nifty": 87
          },
          {
            "field": "yes",
            "morefield": false
          }
        ]""")


        d1 = json.dumps(h)
        d2 = json.dumps(h, indent=2, sort_keys=True, separators=(',', ': '))

        h1 = json.loads(d1)
        h2 = json.loads(d2)

        self.assertEquals(h1, h)
        self.assertEquals(h2, h)
        self.assertEquals(d2, expect)
