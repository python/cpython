"""Doctests for module reloading.

>>> from xreload import xreload
>>> from test.test_xreload import make_mod
>>> make_mod()
>>> import x
>>> C = x.C
>>> Cfoo = C.foo
>>> Cbar = C.bar
>>> Cstomp = C.stomp
>>> b = C()
>>> bfoo = b.foo
>>> b.foo()
42
>>> bfoo()
42
>>> Cfoo(b)
42
>>> Cbar()
42 42
>>> Cstomp()
42 42 42
>>> make_mod(repl="42", subst="24")
>>> xreload(x)
<module 'x' (built-in)>
>>> b.foo()
24
>>> bfoo()
24
>>> Cfoo(b)
24
>>> Cbar()
24 24
>>> Cstomp()
24 24 24

"""

SAMPLE_CODE = """
class C:
    def foo(self):
        print(42)
    @classmethod
    def bar(cls):
        print(42, 42)
    @staticmethod
    def stomp():
        print (42, 42, 42)
"""

import os
import sys
import shutil
import doctest
import xreload
import tempfile
from test.test_support import run_unittest

tempdir = None
save_path = None


def setUp(unused=None):
    global tempdir, save_path
    tempdir = tempfile.mkdtemp()
    save_path = list(sys.path)
    sys.path.append(tempdir)


def tearDown(unused=None):
    global tempdir, save_path
    if save_path is not None:
        sys.path = save_path
        save_path = None
    if tempdir is not None:
        shutil.rmtree(tempdir)
        tempdir = None
        

def make_mod(name="x", repl=None, subst=None):
    if not tempdir:
        setUp()
        assert tempdir
    fn = os.path.join(tempdir, name + ".py")
    f = open(fn, "w")
    sample = SAMPLE_CODE
    if repl is not None and subst is not None:
        sample = sample.replace(repl, subst)
    try:
        f.write(sample)
    finally:
        f.close()


def test_suite():
    return doctest.DocTestSuite(setUp=setUp, tearDown=tearDown)


def test_main():
    run_unittest(test_suite())

if __name__ == "__main__":
    test_main()
