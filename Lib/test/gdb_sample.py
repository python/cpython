# Sample script for use by test_gdb.py

def foo(a, b, c):
    bar(a, b, c)

def bar(a, b, c):
    baz(a, b, c)

def baz(*args):
    print(42)

foo(1, 2, 3)
