"""Verify that warnings are issued for global statements following use."""

from test_support import check_syntax

import warnings

warnings.filterwarnings("error", module="<test code>")

def compile_and_check(text, should_fail=1):
    try:
        compile(text, "<test code>", "exec")
    except SyntaxError, msg:
        if should_fail:
            print "got SyntaxError as expected"
        else:
            print "raised unexpected SyntaxError:", text
    else:
        if should_fail:
            print "should have raised SyntaxError:", text
        else:
            print "as expected, no SyntaxError"

prog_text_1 = """
def wrong1():
    a = 1
    b = 2
    global a
    global b
"""
compile_and_check(prog_text_1)

prog_text_2 = """
def wrong2():
    print x
    global x
"""
compile_and_check(prog_text_2)

prog_text_3 = """
def wrong3():
    print x
    x = 2
    global x
"""
compile_and_check(prog_text_3)

prog_text_4 = """
global x
x = 2
"""
compile_and_check(prog_text_4, 0)
