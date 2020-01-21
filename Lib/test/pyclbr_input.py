"""Test cases for test_pyclbr.py"""

def f(): pass

class Other(object):
    @classmethod
    def foo(c): pass

    def om(self): pass

class B (object):
    def bm(self): pass

class C (B):
    foo = Other().foo
    om = Other.om

    d = 10

    f = f

    def m(self): pass

    @staticmethod
    def sm(self): pass

    @classmethod
    def cm(self): pass
