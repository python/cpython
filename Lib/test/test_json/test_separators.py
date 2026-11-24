import textwrap
from test.test_json import PyTest, CTest


class TestSeparators:
    def test_separators(self):
        h = [['blorpie'], ['whoops'], [], 'd-shtaeou', 'd-nthiouh', 'i-vhbjkhnth',
             {'nifty': 87}, {'field': 'yes', 'morefield': False} ]

        expect = textwrap.dedent("""\
        [
          [
            "blorpie"
          ] ,
          [
            "whoops"
          ] ,
          [] ,
          "d-shtaeou" ,
          "d-nthiouh" ,
          "i-vhbjkhnth" ,
          {
            "nifty" : 87
          } ,
          {
            "field" : "yes" ,
            "morefield" : false
          }
        ]""")


        d1 = self.dumps(h)
        d2 = self.dumps(h, indent=2, sort_keys=True, separators=(' ,', ' : '))

        h1 = self.loads(d1)
        h2 = self.loads(d2)

        self.assertEqual(h1, h)
        self.assertEqual(h2, h)
        self.assertEqual(d2, expect)

    def test_illegal_separators(self):
        h = {1: 2, 3: 4}
        self.assertRaises(TypeError, self.dumps, h, separators=(b', ', ': '))
        self.assertRaises(TypeError, self.dumps, h, separators=(', ', b': '))
        self.assertRaises(TypeError, self.dumps, h, separators=(b', ', b': '))

    def test_str_subclass_separators(self):
        # Don't call obj.__str__() on str subclasses (see gh-148241)
        class StrSubclass(str):
            def __str__(self):
                return "StrSubclass"

        h = {'a': [1, 2], 'b': 3}
        seps = (StrSubclass(','), StrSubclass(':'))
        self.assertEqual(self.dumps(h, separators=seps),
                         '{"a":[1,2],"b":3}')
        self.assertEqual(self.dumps(h, separators=seps, sort_keys=True),
                         '{"a":[1,2],"b":3}')
        self.assertEqual(self.dumps(h, separators=seps, indent=1),
                         '{\n "a":[\n  1,\n  2\n ],\n "b":3\n}')


class TestPySeparators(TestSeparators, PyTest): pass
class TestCSeparators(TestSeparators, CTest): pass
