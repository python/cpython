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

sys.path.insert(0, os.curdir)

source = TESTFN + ".py"
pyc = TESTFN + ".pyc"
pyo = TESTFN + ".pyo"

f = open(source, "w")
print >> f, "# This will test Python's ability to import a .py file"
a = random.randrange(1000)
b = random.randrange(1000)
print >> f, "a =", a
print >> f, "b =", b
f.close()

try:
    try:
        mod = __import__(TESTFN)
    except ImportError, err:
        raise ValueError, "import from .py failed: %s" % err

    if mod.a != a or mod.b != b:
        print a, "!=", mod.a
        print b, "!=", mod.b
        raise ValueError, "module loaded (%s) but contents invalid" % mod
finally:
    os.unlink(source)

try:
    try:
        reload(mod)
    except ImportError, err:
        raise ValueError, "import from .pyc/.pyo failed: %s" % err
finally:
    try:
        os.unlink(pyc)
    except os.error:
        pass
    try:
        os.unlink(pyo)
    except os.error:
        pass

del sys.path[0]
