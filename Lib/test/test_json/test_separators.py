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


class TestPySeparators(TestSeparators, PyTest): pass
class TestCSeparators(TestSeparators, CTest): pass
