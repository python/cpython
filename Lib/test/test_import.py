from test_support import TESTFN, TestFailed

import os
import random
import sys

# Brief digression to test that import is case-sensitive:  if we got this
# far, we know for sure that "random" exists.
try:
    import RAnDoM
except ImportError:
    pass
else:
    raise TestFailed("import of RAnDoM should have failed (case mismatch)")

# Another brief digression to test the accuracy of manifest float constants.
import double_const  # don't blink -- that *was* the test

def test_with_extension(ext): # ext normally ".py"; perhaps ".pyw"
    source = TESTFN + ext
    pyo = TESTFN + os.extsep + "pyo"
    if sys.platform.startswith('java'):
        pyc = TESTFN + "$py.class"
    else:
        pyc = TESTFN + os.extsep + "pyc"

    f = open(source, "w")
    print >> f, "# This tests Python's ability to import a", ext, "file."
    a = random.randrange(1000)
    b = random.randrange(1000)
    print >> f, "a =", a
    print >> f, "b =", b
    f.close()

    try:
        try:
            mod = __import__(TESTFN)
        except ImportError, err:
            raise ValueError("import from %s failed: %s" % (ext, err))

        if mod.a != a or mod.b != b:
            print a, "!=", mod.a
            print b, "!=", mod.b
            raise ValueError("module loaded (%s) but contents invalid" % mod)
    finally:
        os.unlink(source)

    try:
        try:
            reload(mod)
        except ImportError, err:
            raise ValueError("import from .pyc/.pyo failed: %s" % err)
    finally:
        try:
            os.unlink(pyc)
        except os.error:
            pass
        try:
            os.unlink(pyo)
        except os.error:
            pass
        del sys.modules[TESTFN]

sys.path.insert(0, os.curdir)
try:
    test_with_extension(os.extsep + "py")
    if sys.platform.startswith("win"):
        for ext in ".PY", ".Py", ".pY", ".pyw", ".PYW", ".pYw":
            test_with_extension(ext)
finally:
    del sys.path[0]
