from test.test_support import TESTFN, TestFailed

import os
import random
import sys
import py_compile

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

def remove_files(name):
    for f in (name + os.extsep + "py",
              name + os.extsep + "pyc",
              name + os.extsep + "pyo",
              name + os.extsep + "pyw",
              name + "$py.class"):
        if os.path.exists(f):
            os.remove(f)

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

# Verify that the imp module can correctly load and find .py files
import imp
x = imp.find_module("os")
os = imp.load_module("os", *x)

def test_module_with_large_stack(module):
    # create module w/list of 65000 elements to test bug #561858
    filename = module + os.extsep + 'py'

    # create a file with a list of 65000 elements
    f = open(filename, 'w+')
    f.write('d = [\n')
    for i in range(65000):
        f.write('"",\n')
    f.write(']')
    f.close()

    # compile & remove .py file, we only need .pyc (or .pyo)
    f = open(filename, 'r')
    py_compile.compile(filename)
    f.close()
    os.unlink(filename)

    # need to be able to load from current dir
    sys.path.append('')

    # this used to crash
    exec 'import ' + module

    # cleanup
    del sys.path[-1]
    for ext in 'pyc', 'pyo':
        fname = module + os.extsep + ext
        if os.path.exists(fname):
            os.unlink(fname)

test_module_with_large_stack('longlist')

def test_failing_import_sticks():
    source = TESTFN + os.extsep + "py"
    f = open(source, "w")
    print >> f, "a = 1/0"
    f.close()

    # New in 2.4, we shouldn't be able to import that no matter how often
    # we try.
    sys.path.insert(0, os.curdir)
    try:
        for i in 1, 2, 3:
            try:
                mod = __import__(TESTFN)
            except ZeroDivisionError:
                if TESTFN in sys.modules:
                    raise TestFailed("damaged module in sys.modules", i)
            else:
                raise TestFailed("was able to import a damaged module", i)
    finally:
        sys.path.pop(0)
        remove_files(TESTFN)

test_failing_import_sticks()

def test_failing_reload():
    # A failing reload should leave the module object in sys.modules.
    source = TESTFN + os.extsep + "py"
    f = open(source, "w")
    print >> f, "a = 1"
    print >> f, "b = 2"
    f.close()

    sys.path.insert(0, os.curdir)
    try:
        mod = __import__(TESTFN)
        if TESTFN not in sys.modules:
            raise TestFailed("expected module in sys.modules")
        if mod.a != 1 or mod.b != 2:
            raise TestFailed("module has wrong attribute values")

        # On WinXP, just replacing the .py file wasn't enough to
        # convince reload() to reparse it.  Maybe the timestamp didn't
        # move enough.  We force it to get reparsed by removing the
        # compiled file too.
        remove_files(TESTFN)

        # Now damage the module.
        f = open(source, "w")
        print >> f, "a = 10"
        print >> f, "b = 20//0"
        f.close()
        try:
            reload(mod)
        except ZeroDivisionError:
            pass
        else:
            raise TestFailed("was able to reload a damaged module")

        # But we still expect the module to be in sys.modules.
        mod = sys.modules.get(TESTFN)
        if mod is None:
            raise TestFailed("expected module to still be in sys.modules")
        # We should have replaced a w/ 10, but the old b value should
        # stick.
        if mod.a != 10 or mod.b != 2:
            raise TestFailed("module has wrong attribute values")

    finally:
        sys.path.pop(0)
        remove_files(TESTFN)
        if TESTFN in sys.modules:
            del sys.modules[TESTFN]

test_failing_reload()
