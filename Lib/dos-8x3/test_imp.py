from test_support import TESTFN

import os
import random

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
