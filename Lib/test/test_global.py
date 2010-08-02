"""Verify that warnings are issued for global statements following use."""

from test.test_support import run_unittest, check_syntax_error
import unittest
import warnings


class GlobalTests(unittest.TestCase):

    def test1(self):
        prog_text_1 = """\
def wrong1():
    a = 1
    b = 2
    global a
    global b
"""
        check_syntax_error(self, prog_text_1)

    def test2(self):
        prog_text_2 = """\
def wrong2():
    print x
    global x
"""
        check_syntax_error(self, prog_text_2)

    def test3(self):
        prog_text_3 = """\
def wrong3():
    print x
    x = 2
    global x
"""
        check_syntax_error(self, prog_text_3)

    def test4(self):
        prog_text_4 = """\
global x
x = 2
"""
        # this should work
        compile(prog_text_4, "<test string>", "exec")


def test_main():
    with warnings.catch_warnings():
        warnings.filterwarnings("error", module="<test string>")
        run_unittest(GlobalTests)

if __name__ == "__main__":
    test_main()
