"""Verify that warnings are issued for global statements following use"""

from test_support import check_syntax

import warnings

warnings.filterwarnings("error", category=SyntaxWarning, module=__name__)

def compile_and_catch_warning(text):
    try:
        compile(text, "<test code>", "exec")
    except SyntaxWarning, msg:
        print "got SyntaxWarning as expected"
    else:
        print "expected SyntaxWarning"

prog_text_1 = """
def wrong1():
    a = 1
    b = 2
    global a
    global b
"""
compile_and_catch_warning(prog_text_1)

prog_text_2 = """
def wrong2():
    print x
    global x
"""
compile_and_catch_warning(prog_text_2)

prog_text_3 = """
def wrong3():
    print x
    x = 2
    global x
"""
compile_and_catch_warning(prog_text_3)
