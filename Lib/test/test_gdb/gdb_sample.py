# Sample script for use by test_gdb
from _typing import _idfunc

def foo(a, b, c):
    bar(a=a, b=b, c=c)

def bar(a, b, c):
    baz(a, b, c)

def baz(*args):
    _idfunc(42)

foo(1, 2, 3)
