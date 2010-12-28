# Simple test suite for http/cookies.py

from test.support import run_unittest, run_doctest
import unittest
from http import cookies

import warnings
warnings.filterwarnings("ignore",
                        ".* class is insecure.*",
                        DeprecationWarning)

class CookieTests(unittest.TestCase):
    def test_basic(self):
        cases = [
            { 'data': 'chips=ahoy; vienna=finger',
              'dict': {'chips':'ahoy', 'vienna':'finger'},
              'repr': "<SimpleCookie: chips='ahoy' vienna='finger'>",
              'output': 'Set-Cookie: chips=ahoy\nSet-Cookie: vienna=finger',
            },

            { 'data': 'keebler="E=mc2; L=\\"Loves\\"; fudge=\\012;"',
              'dict': {'keebler' : 'E=mc2; L="Loves"; fudge=\012;'},
              'repr': '''<SimpleCookie: keebler='E=mc2; L="Loves"; fudge=\\n;'>''',
              'output': 'Set-Cookie: keebler="E=mc2; L=\\"Loves\\"; fudge=\\012;"',
            },

            # Check illegal cookies that have an '=' char in an unquoted value
            { 'data': 'keebler=E=mc2',
              'dict': {'keebler' : 'E=mc2'},
              'repr': "<SimpleCookie: keebler='E=mc2'>",
              'output': 'Set-Cookie: keebler=E=mc2',
            }
        ]

        for case in cases:
            C = cookies.SimpleCookie()
            C.load(case['data'])
            self.assertEqual(repr(C), case['repr'])
            self.assertEqual(C.output(sep='\n'), case['output'])
            for k, v in sorted(case['dict'].items()):
                self.assertEqual(C[k].value, v)

    def test_load(self):
        C = cookies.SimpleCookie()
        C.load('Customer="WILE_E_COYOTE"; Version=1; Path=/acme')

        self.assertEqual(C['Customer'].value, 'WILE_E_COYOTE')
        self.assertEqual(C['Customer']['version'], '1')
        self.assertEqual(C['Customer']['path'], '/acme')

        self.assertEqual(C.output(['path']),
            'Set-Cookie: Customer="WILE_E_COYOTE"; Path=/acme')
        self.assertEqual(C.js_output(), r"""
        <script type="text/javascript">
        <!-- begin hiding
        document.cookie = "Customer=\"WILE_E_COYOTE\"; Path=/acme; Version=1";
        // end hiding -->
        </script>
        """)
        self.assertEqual(C.js_output(['path']), r"""
        <script type="text/javascript">
        <!-- begin hiding
        document.cookie = "Customer=\"WILE_E_COYOTE\"; Path=/acme";
        // end hiding -->
        </script>
        """)

    def test_extended_encode(self):
        # Issue 9824: some browsers don't follow the standard; we now
        # encode , and ; to keep them from tripping up.
        C = cookies.SimpleCookie()
        C['val'] = "some,funky;stuff"
        self.assertEqual(C.output(['val']),
            'Set-Cookie: val="some\\054funky\\073stuff"')

    def test_special_attrs(self):
        # 'expires'
        C = cookies.SimpleCookie('Customer="WILE_E_COYOTE"')
        C['Customer']['expires'] = 0
        # can't test exact output, it always depends on current date/time
        self.assertTrue(C.output().endswith('GMT'))

        # loading 'expires'
        C = cookies.SimpleCookie()
        C.load('Customer="W"; expires=Wed, 01-Jan-2010 00:00:00 GMT')
        self.assertEqual(C['Customer']['expires'],
                         'Wed, 01-Jan-2010 00:00:00 GMT')
        C = cookies.SimpleCookie()
        C.load('Customer="W"; expires=Wed, 01-Jan-98 00:00:00 GMT')
        self.assertEqual(C['Customer']['expires'],
                         'Wed, 01-Jan-98 00:00:00 GMT')

        # 'max-age'
        C = cookies.SimpleCookie('Customer="WILE_E_COYOTE"')
        C['Customer']['max-age'] = 10
        self.assertEqual(C.output(),
                         'Set-Cookie: Customer="WILE_E_COYOTE"; Max-Age=10')

        # others
        C = cookies.SimpleCookie('Customer="WILE_E_COYOTE"')
        C['Customer']['secure'] = True
        C['Customer']['httponly'] = True
        self.assertEqual(C.output(),
            'Set-Cookie: Customer="WILE_E_COYOTE"; httponly; secure')

    def test_quoted_meta(self):
        # Try cookie with quoted meta-data
        C = cookies.SimpleCookie()
        C.load('Customer="WILE_E_COYOTE"; Version="1"; Path="/acme"')
        self.assertEqual(C['Customer'].value, 'WILE_E_COYOTE')
        self.assertEqual(C['Customer']['version'], '1')
        self.assertEqual(C['Customer']['path'], '/acme')

def test_main():
    run_unittest(CookieTests)
    run_doctest(cookies)

if __name__ == '__main__':
    test_main()
