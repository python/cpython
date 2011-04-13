from unittest import TestCase

import json
import textwrap
from io import StringIO

class TestIndent(TestCase):
    def test_indent(self):
        h = [['blorpie'], ['whoops'], [], 'd-shtaeou', 'd-nthiouh', 'i-vhbjkhnth',
             {'nifty': 87}, {'field': 'yes', 'morefield': False} ]

        expect = textwrap.dedent("""\
        [
        \t[
        \t\t"blorpie"
        \t],
        \t[
        \t\t"whoops"
        \t],
        \t[],
        \t"d-shtaeou",
        \t"d-nthiouh",
        \t"i-vhbjkhnth",
        \t{
        \t\t"nifty": 87
        \t},
        \t{
        \t\t"field": "yes",
        \t\t"morefield": false
        \t}
        ]""")


        d1 = json.dumps(h)
        d2 = json.dumps(h, indent=2, sort_keys=True, separators=(',', ': '))
        d3 = json.dumps(h, indent='\t', sort_keys=True, separators=(',', ': '))

        h1 = json.loads(d1)
        h2 = json.loads(d2)
        h3 = json.loads(d3)

        self.assertEqual(h1, h)
        self.assertEqual(h2, h)
        self.assertEqual(h3, h)
        self.assertEqual(d2, expect.expandtabs(2))
        self.assertEqual(d3, expect)

    def test_indent0(self):
        h = {3: 1}
        def check(indent, expected):
            d1 = json.dumps(h, indent=indent)
            self.assertEqual(d1, expected)

            sio = StringIO()
            json.dump(h, sio, indent=indent)
            self.assertEqual(sio.getvalue(), expected)

        # indent=0 should emit newlines
        check(0, '{\n"3": 1\n}')
        # indent=None is more compact
        check(None, '{"3": 1}')
