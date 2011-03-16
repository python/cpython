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

    # XXX: This causes test_pyclbr.py to fail, but only because the
    #      introspection-based is_method() code in the test can't
    #      distinguish between this and a genuine method function like m().
    #      The pyclbr.py module gets this right as it parses the text.
    #
    #f = f

    def m(self): pass

    @staticmethod
    def sm(self): pass

    @classmethod
    def cm(self): pass
