# Simple test suite for Cookie.py

from test.test_support import run_unittest, run_doctest, check_warnings
import unittest
import Cookie
import pickle


class CookieTests(unittest.TestCase):
    # Currently this only tests SimpleCookie
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
            C = Cookie.SimpleCookie()
            C.load(case['data'])
            self.assertEqual(repr(C), case['repr'])
            self.assertEqual(C.output(sep='\n'), case['output'])
            for k, v in sorted(case['dict'].iteritems()):
                self.assertEqual(C[k].value, v)

    def test_load(self):
        C = Cookie.SimpleCookie()
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

        # loading 'expires'
        C = Cookie.SimpleCookie()
        C.load('Customer="W"; expires=Wed, 01 Jan 2010 00:00:00 GMT')
        self.assertEqual(C['Customer']['expires'],
                         'Wed, 01 Jan 2010 00:00:00 GMT')
        C = Cookie.SimpleCookie()
        C.load('Customer="W"; expires=Wed, 01 Jan 98 00:00:00 GMT')
        self.assertEqual(C['Customer']['expires'],
                         'Wed, 01 Jan 98 00:00:00 GMT')

    def test_extended_encode(self):
        # Issue 9824: some browsers don't follow the standard; we now
        # encode , and ; to keep them from tripping up.
        C = Cookie.SimpleCookie()
        C['val'] = "some,funky;stuff"
        self.assertEqual(C.output(['val']),
            'Set-Cookie: val="some\\054funky\\073stuff"')

    def test_set_secure_httponly_attrs(self):
        C = Cookie.SimpleCookie('Customer="WILE_E_COYOTE"')
        C['Customer']['secure'] = True
        C['Customer']['httponly'] = True
        self.assertEqual(C.output(),
            'Set-Cookie: Customer="WILE_E_COYOTE"; httponly; secure')

    def test_secure_httponly_false_if_not_present(self):
        C = Cookie.SimpleCookie()
        C.load('eggs=scrambled; Path=/bacon')
        self.assertFalse(C['eggs']['httponly'])
        self.assertFalse(C['eggs']['secure'])

    def test_secure_httponly_true_if_present(self):
        # Issue 16611
        C = Cookie.SimpleCookie()
        C.load('eggs=scrambled; httponly; secure; Path=/bacon')
        self.assertTrue(C['eggs']['httponly'])
        self.assertTrue(C['eggs']['secure'])

    def test_secure_httponly_true_if_have_value(self):
        # This isn't really valid, but demonstrates what the current code
        # is expected to do in this case.
        C = Cookie.SimpleCookie()
        C.load('eggs=scrambled; httponly=foo; secure=bar; Path=/bacon')
        self.assertTrue(C['eggs']['httponly'])
        self.assertTrue(C['eggs']['secure'])
        # Here is what it actually does; don't depend on this behavior.  These
        # checks are testing backward compatibility for issue 16611.
        self.assertEqual(C['eggs']['httponly'], 'foo')
        self.assertEqual(C['eggs']['secure'], 'bar')

    def test_bad_attrs(self):
        # Issue 16611: make sure we don't break backward compatibility.
        C = Cookie.SimpleCookie()
        C.load('cookie=with; invalid; version; second=cookie;')
        self.assertEqual(C.output(),
            'Set-Cookie: cookie=with\r\nSet-Cookie: second=cookie')

    def test_extra_spaces(self):
        C = Cookie.SimpleCookie()
        C.load('eggs  =  scrambled  ;  secure  ;  path  =  bar   ; foo=foo   ')
        self.assertEqual(C.output(),
            'Set-Cookie: eggs=scrambled; Path=bar; secure\r\nSet-Cookie: foo=foo')

    def test_quoted_meta(self):
        # Try cookie with quoted meta-data
        C = Cookie.SimpleCookie()
        C.load('Customer="WILE_E_COYOTE"; Version="1"; Path="/acme"')
        self.assertEqual(C['Customer'].value, 'WILE_E_COYOTE')
        self.assertEqual(C['Customer']['version'], '1')
        self.assertEqual(C['Customer']['path'], '/acme')

    def test_invalid_cookies(self):
        # Accepting these could be a security issue
        C = Cookie.SimpleCookie()
        for s in (']foo=x', '[foo=x', 'blah]foo=x', 'blah[foo=x'):
            C.load(s)
            self.assertEqual(dict(C), {})
            self.assertEqual(C.output(), '')

    def test_pickle(self):
        rawdata = 'Customer="WILE_E_COYOTE"; Path=/acme; Version=1'
        expected_output = 'Set-Cookie: %s' % rawdata

        C = Cookie.SimpleCookie()
        C.load(rawdata)
        self.assertEqual(C.output(), expected_output)

        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            C1 = pickle.loads(pickle.dumps(C, protocol=proto))
            self.assertEqual(C1.output(), expected_output)


def test_main():
    run_unittest(CookieTests)
    if Cookie.__doc__ is not None:
        with check_warnings(('.+Cookie class is insecure; do not use it',
                             DeprecationWarning)):
            run_doctest(Cookie)

if __name__ == '__main__':
    test_main()
