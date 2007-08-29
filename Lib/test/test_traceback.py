"""Test cases for traceback module"""

import unittest
from test.test_support import run_unittest, is_jython

import traceback

class TracebackCases(unittest.TestCase):
    # For now, a very minimal set of tests.  I want to be sure that
    # formatting of SyntaxErrors works based on changes for 2.1.

    def get_exception_format(self, func, exc):
        try:
            func()
        except exc as value:
            return traceback.format_exception_only(exc, value)
        else:
            raise ValueError("call did not raise exception")

    def syntax_error_with_caret(self):
        compile("def fact(x):\n\treturn x!\n", "?", "exec")

    def syntax_error_without_caret(self):
        # XXX why doesn't compile raise the same traceback?
        import test.badsyntax_nocaret

    def syntax_error_bad_indentation(self):
        compile("def spam():\n  print(1)\n print(2)", "?", "exec")

    def test_caret(self):
        err = self.get_exception_format(self.syntax_error_with_caret,
                                        SyntaxError)
        self.assertEqual(len(err), 4)
        self.assert_(err[1].strip() == "return x!")
        self.assert_("^" in err[2]) # third line has caret
        self.assertEqual(err[1].find("!"), err[2].find("^")) # in the right place

    def test_nocaret(self):
        if is_jython:
            # jython adds a caret in this case (why shouldn't it?)
            return
        err = self.get_exception_format(self.syntax_error_without_caret,
                                        SyntaxError)
        self.assertEqual(len(err), 3)
        self.assert_(err[1].strip() == "[x for x in x] = x")

    def test_bad_indentation(self):
        err = self.get_exception_format(self.syntax_error_bad_indentation,
                                        IndentationError)
        self.assertEqual(len(err), 4)
        self.assertEqual(err[1].strip(), "print(2)")
        self.assert_("^" in err[2])
        self.assertEqual(err[1].find(")"), err[2].find("^"))

    def test_base_exception(self):
        # Test that exceptions derived from BaseException are formatted right
        e = KeyboardInterrupt()
        lst = traceback.format_exception_only(e.__class__, e)
        self.assertEqual(lst, ['KeyboardInterrupt\n'])

    def test_format_exception_only_bad__str__(self):
        class X(Exception):
            def __str__(self):
                1/0
        err = traceback.format_exception_only(X, X())
        self.assertEqual(len(err), 1)
        str_value = '<unprintable %s object>' % X.__name__
        if X.__module__ in ('__main__', '__builtin__'):
            str_name = X.__name__
        else:
            str_name = '.'.join([X.__module__, X.__name__])
        self.assertEqual(err[0], "%s: %s\n" % (str_name, str_value))

    def test_without_exception(self):
        err = traceback.format_exception_only(None, None)
        self.assertEqual(err, ['None\n'])


def test_main():
    run_unittest(TracebackCases)


if __name__ == "__main__":
    test_main()
