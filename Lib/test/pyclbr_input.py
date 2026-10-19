"""Test cases for test_pyclbr.py"""

def f(): pass

class Other(object):
    @classmethod
    def foo(c): pass

    def om(self): pass

class B (object):
    def bm(self): pass

class C (B):
    d = 10

    # This one is correctly considered by both test_pyclbr.py and pyclbr.py
    # as a non-method of C.
    foo = Other().foo

    # This causes test_pyclbr.py to fail, but only because the
    # introspection-based is_method() code in the test can't
    # distinguish between this and a genuine method function like m().
    #
    # The pyclbr.py module gets this right as it parses the text.
    om = Other.om
    f = f

    def m(self): pass

    @staticmethod
    def sm(self): pass

    @classmethod
    def cm(self): pass

# Check that mangling is correctly handled

class a:
    def a(self): pass
    def _(self): pass
    def _a(self): pass
    def __(self): pass
    def ___(self): pass
    def __a(self): pass

class _:
    def a(self): pass
    def _(self): pass
    def _a(self): pass
    def __(self): pass
    def ___(self): pass
    def __a(self): pass

class __:
    def a(self): pass
    def _(self): pass
    def _a(self): pass
    def __(self): pass
    def ___(self): pass
    def __a(self): pass

class ___:
    def a(self): pass
    def _(self): pass
    def _a(self): pass
    def __(self): pass
    def ___(self): pass
    def __a(self): pass

class _a:
    def a(self): pass
    def _(self): pass
    def _a(self): pass
    def __(self): pass
    def ___(self): pass
    def __a(self): pass

class __a:
    def a(self): pass
    def _(self): pass
    def _a(self): pass
    def __(self): pass
    def ___(self): pass
    def __a(self): pass
