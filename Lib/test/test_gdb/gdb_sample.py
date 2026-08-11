# Sample script for use by test_gdb

def foo(a, b, c):
    bar(a=a, b=b, c=c)

def bar(a, b, c):
    baz(a, b, c)

def baz(*args):
    id(42)

foo(1, 2, 3)
