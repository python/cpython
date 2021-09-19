"""Verify that warnings are issued for global statements following use."""

from test.support import check_syntax_error, check_warnings
import unittest
import warnings


class GlobalTests(unittest.TestCase):

    def setUp(self):
        self._warnings_manager = check_warnings()
        self._warnings_manager.__enter__()
        warnings.filterwarnings("error", module="<test string>")

    def tearDown(self):
        self._warnings_manager.__exit__(None, None, None)


    def test1(self):
        prog_text_1 = """\
def wrong1():
    a = 1
    b = 2
    global a
    global b
"""
        check_syntax_error(self, prog_text_1, lineno=4, offset=5)

    def test2(self):
        prog_text_2 = """\
def wrong2():
    print(x)
    global x
"""
        check_syntax_error(self, prog_text_2, lineno=3, offset=5)

    def test3(self):
        prog_text_3 = """\
def wrong3():
    print(x)
    x = 2
    global x
"""
        check_syntax_error(self, prog_text_3, lineno=4, offset=5)

    def test4(self):
        prog_text_4 = """\
global x
x = 2
"""
        # this should work
        compile(prog_text_4, "<test string>", "exec")


def setUpModule():
    cm = warnings.catch_warnings()
    cm.__enter__()
    unittest.addModuleCleanup(cm.__exit__, None, None, None)
    warnings.filterwarnings("error", module="<test string>")


if __name__ == "__main__":
    unittest.main()
