# -*- coding: koi8-r -*-

import test.test_support, unittest
import os

class SourceEncodingTest(unittest.TestCase):

    def test_pep263(self):
        self.assertEqual(
            u"Питон".encode("utf-8"),
            '\xd0\x9f\xd0\xb8\xd1\x82\xd0\xbe\xd0\xbd'
        )
        self.assertEqual(
            u"\П".encode("utf-8"),
            '\\\xd0\x9f'
        )

    def test_compilestring(self):
        # see #1882
        c = compile("\n# coding: utf-8\nu = u'\xc3\xb3'\n", "dummy", "exec")
        d = {}
        exec c in d
        self.assertEqual(d['u'], u'\xf3')

    def test_issue3297(self):
        c = compile("a, b = '\U0001010F', '\\U0001010F'", "dummy", "exec")
        d = {}
        exec(c, d)
        self.assertEqual(d['a'], d['b'])
        self.assertEqual(len(d['a']), len(d['b']))

    def test_issue7820(self):
        # Ensure that check_bom() restores all bytes in the right order if
        # check_bom() fails in pydebug mode: a buffer starts with the first
        # byte of a valid BOM, but next bytes are different

        # one byte in common with the UTF-16-LE BOM
        self.assertRaises(SyntaxError, eval, '\xff\x20')

        # two bytes in common with the UTF-8 BOM
        self.assertRaises(SyntaxError, eval, '\xef\xbb\x20')

    def test_error_message(self):
        compile('# -*- coding: iso-8859-15 -*-\n', 'dummy', 'exec')
        compile('\xef\xbb\xbf\n', 'dummy', 'exec')
        compile('\xef\xbb\xbf# -*- coding: utf-8 -*-\n', 'dummy', 'exec')
        with self.assertRaisesRegexp(SyntaxError, 'fake'):
            compile('# -*- coding: fake -*-\n', 'dummy', 'exec')
        with self.assertRaisesRegexp(SyntaxError, 'iso-8859-15'):
            compile('\xef\xbb\xbf# -*- coding: iso-8859-15 -*-\n',
                    'dummy', 'exec')
        with self.assertRaisesRegexp(SyntaxError, 'BOM'):
            compile('\xef\xbb\xbf# -*- coding: iso-8859-15 -*-\n',
                    'dummy', 'exec')
        with self.assertRaisesRegexp(SyntaxError, 'fake'):
            compile('\xef\xbb\xbf# -*- coding: fake -*-\n', 'dummy', 'exec')
        with self.assertRaisesRegexp(SyntaxError, 'BOM'):
            compile('\xef\xbb\xbf# -*- coding: fake -*-\n', 'dummy', 'exec')

    def test_non_unicode_codec(self):
        with self.assertRaisesRegexp(SyntaxError,
                                     'codec did not return a unicode'):
            from test import bad_coding3


    def test_bad_coding(self):
        module_name = 'bad_coding'
        self.verify_bad_module(module_name)

    def test_bad_coding2(self):
        module_name = 'bad_coding2'
        self.verify_bad_module(module_name)

    def verify_bad_module(self, module_name):
        self.assertRaises(SyntaxError, __import__, 'test.' + module_name)

        path = os.path.dirname(__file__)
        filename = os.path.join(path, module_name + '.py')
        with open(filename) as fp:
            text = fp.read()
        self.assertRaises(SyntaxError, compile, text, filename, 'exec')

    def test_error_from_string(self):
        # See http://bugs.python.org/issue6289
        input = u"# coding: ascii\n\N{SNOWMAN}".encode('utf-8')
        with self.assertRaises(SyntaxError) as c:
            compile(input, "<string>", "exec")
        expected = "'ascii' codec can't decode byte 0xe2 in position 16: " \
                   "ordinal not in range(128)"
        self.assertTrue(c.exception.args[0].startswith(expected))


def test_main():
    test.test_support.run_unittest(SourceEncodingTest)

if __name__ == "__main__":
    test_main()
