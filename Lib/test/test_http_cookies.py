# Simple test suite for http/cookies.py

import copy
import unittest
import doctest
from http import cookies
import pickle
from test import support


class CookieTests(unittest.TestCase):

    def test_basic(self):
        cases = [
            {'data': 'chips=ahoy; vienna=finger',
             'dict': {'chips':'ahoy', 'vienna':'finger'},
             'repr': "<SimpleCookie: chips='ahoy' vienna='finger'>",
             'output': 'Set-Cookie: chips=ahoy\nSet-Cookie: vienna=finger'},

            {'data': 'keebler="E=mc2; L=\\"Loves\\"; fudge=\\012;"',
             'dict': {'keebler' : 'E=mc2; L="Loves"; fudge=\012;'},
             'repr': '''<SimpleCookie: keebler='E=mc2; L="Loves"; fudge=\\n;'>''',
             'output': 'Set-Cookie: keebler="E=mc2; L=\\"Loves\\"; fudge=\\012;"'},

            # Check illegal cookies that have an '=' char in an unquoted value
            {'data': 'keebler=E=mc2',
             'dict': {'keebler' : 'E=mc2'},
             'repr': "<SimpleCookie: keebler='E=mc2'>",
             'output': 'Set-Cookie: keebler=E=mc2'},

            # Cookies with ':' character in their name. Though not mentioned in
            # RFC, servers / browsers allow it.

             {'data': 'key:term=value:term',
             'dict': {'key:term' : 'value:term'},
             'repr': "<SimpleCookie: key:term='value:term'>",
             'output': 'Set-Cookie: key:term=value:term'},

            # issue22931 - Adding '[' and ']' as valid characters in cookie
            # values as defined in RFC 6265
            {
                'data': 'a=b; c=[; d=r; f=h',
                'dict': {'a':'b', 'c':'[', 'd':'r', 'f':'h'},
                'repr': "<SimpleCookie: a='b' c='[' d='r' f='h'>",
                'output': '\n'.join((
                    'Set-Cookie: a=b',
                    'Set-Cookie: c=[',
                    'Set-Cookie: d=r',
                    'Set-Cookie: f=h'
                ))
            }
        ]

        for case in cases:
            C = cookies.SimpleCookie()
            C.load(case['data'])
            self.assertEqual(repr(C), case['repr'])
            self.assertEqual(C.output(sep='\n'), case['output'])
            for k, v in sorted(case['dict'].items()):
                self.assertEqual(C[k].value, v)

    def test_obsolete_rfc850_date_format(self):
        # Test cases with different days and dates in obsolete RFC 850 format
        test_cases = [
            # from RFC 850, change EST to GMT
            # https://datatracker.ietf.org/doc/html/rfc850#section-2
            {
                'data': 'key=value; expires=Saturday, 01-Jan-83 00:00:00 GMT',
                'output': 'Saturday, 01-Jan-83 00:00:00 GMT'
            },
            {
                'data': 'key=value; expires=Friday, 19-Nov-82 16:59:30 GMT',
                'output': 'Friday, 19-Nov-82 16:59:30 GMT'
            },
            # from RFC 9110
            # https://www.rfc-editor.org/rfc/rfc9110.html#section-5.6.7-6
            {
                'data': 'key=value; expires=Sunday, 06-Nov-94 08:49:37 GMT',
                'output': 'Sunday, 06-Nov-94 08:49:37 GMT'
            },
            # other test cases
            {
                'data': 'key=value; expires=Wednesday, 09-Nov-94 08:49:37 GMT',
                'output': 'Wednesday, 09-Nov-94 08:49:37 GMT'
            },
            {
                'data': 'key=value; expires=Friday, 11-Nov-94 08:49:37 GMT',
                'output': 'Friday, 11-Nov-94 08:49:37 GMT'
            },
            {
                'data': 'key=value; expires=Monday, 14-Nov-94 08:49:37 GMT',
                'output': 'Monday, 14-Nov-94 08:49:37 GMT'
            },
        ]

        for case in test_cases:
            with self.subTest(data=case['data']):
                C = cookies.SimpleCookie()
                C.load(case['data'])

                # Extract the cookie name from the data string
                cookie_name = case['data'].split('=')[0]

                # Check if the cookie is loaded correctly
                self.assertIn(cookie_name, C)
                self.assertEqual(C[cookie_name].get('expires'), case['output'])

    def test_unquote(self):
        cases = [
            (r'a="b=\""', 'b="'),
            (r'a="b=\\"', 'b=\\'),
            (r'a="b=\="', 'b=='),
            (r'a="b=\n"', 'b=n'),
            (r'a="b=\042"', 'b="'),
            (r'a="b=\134"', 'b=\\'),
            (r'a="b=\377"', 'b=\xff'),
            (r'a="b=\400"', 'b=400'),
            (r'a="b=\42"', 'b=42'),
            (r'a="b=\\042"', 'b=\\042'),
            (r'a="b=\\134"', 'b=\\134'),
            (r'a="b=\\\""', 'b=\\"'),
            (r'a="b=\\\042"', 'b=\\"'),
            (r'a="b=\134\""', 'b=\\"'),
            (r'a="b=\134\042"', 'b=\\"'),
        ]
        for encoded, decoded in cases:
            with self.subTest(encoded):
                C = cookies.SimpleCookie()
                C.load(encoded)
                self.assertEqual(C['a'].value, decoded)

    @support.requires_resource('cpu')
    def test_unquote_large(self):
        n = 10**6
        for encoded in r'\\', r'\134':
            with self.subTest(encoded):
                data = 'a="b=' + encoded*n + ';"'
                C = cookies.SimpleCookie()
                C.load(data)
                value = C['a'].value
                self.assertEqual(value[:3], 'b=\\')
                self.assertEqual(value[-2:], '\\;')
                self.assertEqual(len(value), n + 3)

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
        self.assertEndsWith(C.output(), 'GMT')

        # loading 'expires'
        C = cookies.SimpleCookie()
        C.load('Customer="W"; expires=Wed, 01 Jan 2010 00:00:00 GMT')
        self.assertEqual(C['Customer']['expires'],
                         'Wed, 01 Jan 2010 00:00:00 GMT')
        C = cookies.SimpleCookie()
        C.load('Customer="W"; expires=Wed, 01 Jan 98 00:00:00 GMT')
        self.assertEqual(C['Customer']['expires'],
                         'Wed, 01 Jan 98 00:00:00 GMT')

        # 'max-age'
        C = cookies.SimpleCookie('Customer="WILE_E_COYOTE"')
        C['Customer']['max-age'] = 10
        self.assertEqual(C.output(),
                         'Set-Cookie: Customer="WILE_E_COYOTE"; Max-Age=10')

    def test_set_secure_httponly_attrs(self):
        C = cookies.SimpleCookie('Customer="WILE_E_COYOTE"')
        C['Customer']['secure'] = True
        C['Customer']['httponly'] = True
        self.assertEqual(C.output(),
            'Set-Cookie: Customer="WILE_E_COYOTE"; HttpOnly; Secure')

    def test_set_secure_httponly_partitioned_attrs(self):
        C = cookies.SimpleCookie('Customer="WILE_E_COYOTE"')
        C['Customer']['secure'] = True
        C['Customer']['httponly'] = True
        C['Customer']['partitioned'] = True
        self.assertEqual(C.output(),
            'Set-Cookie: Customer="WILE_E_COYOTE"; HttpOnly; Partitioned; Secure')

    def test_samesite_attrs(self):
        samesite_values = ['Strict', 'Lax', 'strict', 'lax']
        for val in samesite_values:
            with self.subTest(val=val):
                C = cookies.SimpleCookie('Customer="WILE_E_COYOTE"')
                C['Customer']['samesite'] = val
                self.assertEqual(C.output(),
                    'Set-Cookie: Customer="WILE_E_COYOTE"; SameSite=%s' % val)

                C = cookies.SimpleCookie()
                C.load('Customer="WILL_E_COYOTE"; SameSite=%s' % val)
                self.assertEqual(C['Customer']['samesite'], val)

    def test_secure_httponly_false_if_not_present(self):
        C = cookies.SimpleCookie()
        C.load('eggs=scrambled; Path=/bacon')
        self.assertFalse(C['eggs']['httponly'])
        self.assertFalse(C['eggs']['secure'])

    def test_secure_httponly_true_if_present(self):
        # Issue 16611
        C = cookies.SimpleCookie()
        C.load('eggs=scrambled; httponly; secure; Path=/bacon')
        self.assertTrue(C['eggs']['httponly'])
        self.assertTrue(C['eggs']['secure'])

    def test_secure_httponly_true_if_have_value(self):
        # This isn't really valid, but demonstrates what the current code
        # is expected to do in this case.
        C = cookies.SimpleCookie()
        C.load('eggs=scrambled; httponly=foo; secure=bar; Path=/bacon')
        self.assertTrue(C['eggs']['httponly'])
        self.assertTrue(C['eggs']['secure'])
        # Here is what it actually does; don't depend on this behavior.  These
        # checks are testing backward compatibility for issue 16611.
        self.assertEqual(C['eggs']['httponly'], 'foo')
        self.assertEqual(C['eggs']['secure'], 'bar')

    def test_extra_spaces(self):
        C = cookies.SimpleCookie()
        C.load('eggs  =  scrambled  ;  secure  ;  path  =  bar   ; foo=foo   ')
        self.assertEqual(C.output(),
            'Set-Cookie: eggs=scrambled; Path=bar; Secure\r\nSet-Cookie: foo=foo')

    def test_quoted_meta(self):
        # Try cookie with quoted meta-data
        C = cookies.SimpleCookie()
        C.load('Customer="WILE_E_COYOTE"; Version="1"; Path="/acme"')
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

    def test_invalid_cookies(self):
        # Accepting these could be a security issue
        C = cookies.SimpleCookie()
        for s in (']foo=x', '[foo=x', 'blah]foo=x', 'blah[foo=x',
                  'Set-Cookie: foo=bar', 'Set-Cookie: foo',
                  'foo=bar; baz', 'baz; foo=bar',
                  'secure;foo=bar', 'Version=1;foo=bar'):
            C.load(s)
            self.assertEqual(dict(C), {})
            self.assertEqual(C.output(), '')

    def test_pickle(self):
        rawdata = 'Customer="WILE_E_COYOTE"; Path=/acme; Version=1'
        expected_output = 'Set-Cookie: %s' % rawdata

        C = cookies.SimpleCookie()
        C.load(rawdata)
        self.assertEqual(C.output(), expected_output)

        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(proto=proto):
                C1 = pickle.loads(pickle.dumps(C, protocol=proto))
                self.assertEqual(C1.output(), expected_output)

    def test_illegal_chars(self):
        rawdata = "a=b; c,d=e"
        C = cookies.SimpleCookie()
        with self.assertRaises(cookies.CookieError):
            C.load(rawdata)

    def test_comment_quoting(self):
        c = cookies.SimpleCookie()
        c['foo'] = '\N{COPYRIGHT SIGN}'
        self.assertEqual(str(c['foo']), 'Set-Cookie: foo="\\251"')
        c['foo']['comment'] = 'comment \N{COPYRIGHT SIGN}'
        self.assertEqual(
            str(c['foo']),
            'Set-Cookie: foo="\\251"; Comment="comment \\251"'
        )


class MorselTests(unittest.TestCase):
    """Tests for the Morsel object."""

    def test_defaults(self):
        morsel = cookies.Morsel()
        self.assertIsNone(morsel.key)
        self.assertIsNone(morsel.value)
        self.assertIsNone(morsel.coded_value)
        self.assertEqual(morsel.keys(), cookies.Morsel._reserved.keys())
        for key, val in morsel.items():
            self.assertEqual(val, '', key)

    def test_reserved_keys(self):
        M = cookies.Morsel()
        # tests valid and invalid reserved keys for Morsels
        for i in M._reserved:
            # Test that all valid keys are reported as reserved and set them
            self.assertTrue(M.isReservedKey(i))
            M[i] = '%s_value' % i
        for i in M._reserved:
            # Test that valid key values come out fine
            self.assertEqual(M[i], '%s_value' % i)
        for i in "the holy hand grenade".split():
            # Test that invalid keys raise CookieError
            self.assertRaises(cookies.CookieError,
                              M.__setitem__, i, '%s_value' % i)

    def test_setter(self):
        M = cookies.Morsel()
        # tests the .set method to set keys and their values
        for i in M._reserved:
            # Makes sure that all reserved keys can't be set this way
            self.assertRaises(cookies.CookieError,
                              M.set, i, '%s_value' % i, '%s_value' % i)
        for i in "thou cast _the- !holy! ^hand| +*grenade~".split():
            # Try typical use case. Setting decent values.
            # Check output and js_output.
            M['path'] = '/foo' # Try a reserved key as well
            M.set(i, "%s_val" % i, "%s_coded_val" % i)
            self.assertEqual(M.key, i)
            self.assertEqual(M.value, "%s_val" % i)
            self.assertEqual(M.coded_value, "%s_coded_val" % i)
            self.assertEqual(
                M.output(),
                "Set-Cookie: %s=%s; Path=/foo" % (i, "%s_coded_val" % i))
            expected_js_output = """
        <script type="text/javascript">
        <!-- begin hiding
        document.cookie = "%s=%s; Path=/foo";
        // end hiding -->
        </script>
        """ % (i, "%s_coded_val" % i)
            self.assertEqual(M.js_output(), expected_js_output)
        for i in ["foo bar", "foo@bar"]:
            # Try some illegal characters
            self.assertRaises(cookies.CookieError,
                              M.set, i, '%s_value' % i, '%s_value' % i)

    def test_set_properties(self):
        morsel = cookies.Morsel()
        with self.assertRaises(AttributeError):
            morsel.key = ''
        with self.assertRaises(AttributeError):
            morsel.value = ''
        with self.assertRaises(AttributeError):
            morsel.coded_value = ''

    def test_eq(self):
        base_case = ('key', 'value', '"value"')
        attribs = {
            'path': '/',
            'comment': 'foo',
            'domain': 'example.com',
            'version': 2,
        }
        morsel_a = cookies.Morsel()
        morsel_a.update(attribs)
        morsel_a.set(*base_case)
        morsel_b = cookies.Morsel()
        morsel_b.update(attribs)
        morsel_b.set(*base_case)
        self.assertTrue(morsel_a == morsel_b)
        self.assertFalse(morsel_a != morsel_b)
        cases = (
            ('key', 'value', 'mismatch'),
            ('key', 'mismatch', '"value"'),
            ('mismatch', 'value', '"value"'),
        )
        for case_b in cases:
            with self.subTest(case_b):
                morsel_b = cookies.Morsel()
                morsel_b.update(attribs)
                morsel_b.set(*case_b)
                self.assertFalse(morsel_a == morsel_b)
                self.assertTrue(morsel_a != morsel_b)

        morsel_b = cookies.Morsel()
        morsel_b.update(attribs)
        morsel_b.set(*base_case)
        morsel_b['comment'] = 'bar'
        self.assertFalse(morsel_a == morsel_b)
        self.assertTrue(morsel_a != morsel_b)

        # test mismatched types
        self.assertFalse(cookies.Morsel() == 1)
        self.assertTrue(cookies.Morsel() != 1)
        self.assertFalse(cookies.Morsel() == '')
        self.assertTrue(cookies.Morsel() != '')
        items = list(cookies.Morsel().items())
        self.assertFalse(cookies.Morsel() == items)
        self.assertTrue(cookies.Morsel() != items)

        # morsel/dict
        morsel = cookies.Morsel()
        morsel.set(*base_case)
        morsel.update(attribs)
        self.assertTrue(morsel == dict(morsel))
        self.assertFalse(morsel != dict(morsel))

    def test_copy(self):
        morsel_a = cookies.Morsel()
        morsel_a.set('foo', 'bar', 'baz')
        morsel_a.update({
            'version': 2,
            'comment': 'foo',
        })
        morsel_b = morsel_a.copy()
        self.assertIsInstance(morsel_b, cookies.Morsel)
        self.assertIsNot(morsel_a, morsel_b)
        self.assertEqual(morsel_a, morsel_b)

        morsel_b = copy.copy(morsel_a)
        self.assertIsInstance(morsel_b, cookies.Morsel)
        self.assertIsNot(morsel_a, morsel_b)
        self.assertEqual(morsel_a, morsel_b)

    def test_setitem(self):
        morsel = cookies.Morsel()
        morsel['expires'] = 0
        self.assertEqual(morsel['expires'], 0)
        morsel['Version'] = 2
        self.assertEqual(morsel['version'], 2)
        morsel['DOMAIN'] = 'example.com'
        self.assertEqual(morsel['domain'], 'example.com')

        with self.assertRaises(cookies.CookieError):
            morsel['invalid'] = 'value'
        self.assertNotIn('invalid', morsel)

    def test_setdefault(self):
        morsel = cookies.Morsel()
        morsel.update({
            'domain': 'example.com',
            'version': 2,
        })
        # this shouldn't override the default value
        self.assertEqual(morsel.setdefault('expires', 'value'), '')
        self.assertEqual(morsel['expires'], '')
        self.assertEqual(morsel.setdefault('Version', 1), 2)
        self.assertEqual(morsel['version'], 2)
        self.assertEqual(morsel.setdefault('DOMAIN', 'value'), 'example.com')
        self.assertEqual(morsel['domain'], 'example.com')

        with self.assertRaises(cookies.CookieError):
            morsel.setdefault('invalid', 'value')
        self.assertNotIn('invalid', morsel)

    def test_update(self):
        attribs = {'expires': 1, 'Version': 2, 'DOMAIN': 'example.com'}
        # test dict update
        morsel = cookies.Morsel()
        morsel.update(attribs)
        self.assertEqual(morsel['expires'], 1)
        self.assertEqual(morsel['version'], 2)
        self.assertEqual(morsel['domain'], 'example.com')
        # test iterable update
        morsel = cookies.Morsel()
        morsel.update(list(attribs.items()))
        self.assertEqual(morsel['expires'], 1)
        self.assertEqual(morsel['version'], 2)
        self.assertEqual(morsel['domain'], 'example.com')
        # test iterator update
        morsel = cookies.Morsel()
        morsel.update((k, v) for k, v in attribs.items())
        self.assertEqual(morsel['expires'], 1)
        self.assertEqual(morsel['version'], 2)
        self.assertEqual(morsel['domain'], 'example.com')

        with self.assertRaises(cookies.CookieError):
            morsel.update({'invalid': 'value'})
        self.assertNotIn('invalid', morsel)
        self.assertRaises(TypeError, morsel.update)
        self.assertRaises(TypeError, morsel.update, 0)

    def test_pickle(self):
        morsel_a = cookies.Morsel()
        morsel_a.set('foo', 'bar', 'baz')
        morsel_a.update({
            'version': 2,
            'comment': 'foo',
        })
        for proto in range(pickle.HIGHEST_PROTOCOL + 1):
            with self.subTest(proto=proto):
                morsel_b = pickle.loads(pickle.dumps(morsel_a, proto))
                self.assertIsInstance(morsel_b, cookies.Morsel)
                self.assertEqual(morsel_b, morsel_a)
                self.assertEqual(str(morsel_b), str(morsel_a))

    def test_repr(self):
        morsel = cookies.Morsel()
        self.assertEqual(repr(morsel), '<Morsel: None=None>')
        self.assertEqual(str(morsel), 'Set-Cookie: None=None')
        morsel.set('key', 'val', 'coded_val')
        self.assertEqual(repr(morsel), '<Morsel: key=coded_val>')
        self.assertEqual(str(morsel), 'Set-Cookie: key=coded_val')
        morsel.update({
            'path': '/',
            'comment': 'foo',
            'domain': 'example.com',
            'max-age': 0,
            'secure': 0,
            'version': 1,
        })
        self.assertEqual(repr(morsel),
                '<Morsel: key=coded_val; Comment=foo; Domain=example.com; '
                'Max-Age=0; Path=/; Version=1>')
        self.assertEqual(str(morsel),
                'Set-Cookie: key=coded_val; Comment=foo; Domain=example.com; '
                'Max-Age=0; Path=/; Version=1')
        morsel['secure'] = True
        morsel['httponly'] = 1
        self.assertEqual(repr(morsel),
                '<Morsel: key=coded_val; Comment=foo; Domain=example.com; '
                'HttpOnly; Max-Age=0; Path=/; Secure; Version=1>')
        self.assertEqual(str(morsel),
                'Set-Cookie: key=coded_val; Comment=foo; Domain=example.com; '
                'HttpOnly; Max-Age=0; Path=/; Secure; Version=1')

        morsel = cookies.Morsel()
        morsel.set('key', 'val', 'coded_val')
        morsel['expires'] = 0
        self.assertRegex(repr(morsel),
                r'<Morsel: key=coded_val; '
                r'expires=\w+, \d+ \w+ \d+ \d+:\d+:\d+ \w+>')
        self.assertRegex(str(morsel),
                r'Set-Cookie: key=coded_val; '
                r'expires=\w+, \d+ \w+ \d+ \d+:\d+:\d+ \w+')


def load_tests(loader, tests, pattern):
    tests.addTest(doctest.DocTestSuite(cookies))
    return tests


if __name__ == '__main__':
    unittest.main()
