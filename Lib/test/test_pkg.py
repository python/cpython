# Test packages (dotted-name import)

import sys, os, string, tempfile, traceback
from os import mkdir, rmdir             # Can't test if these fail
del mkdir, rmdir
from test_support import verbose, TestFailed

# Helpers to create and destroy hierarchies.

def mkhier(root, descr):
    mkdir(root)
    for name, contents in descr:
        comps = string.split(name)
        fullname = root
        for c in comps:
            fullname = os.path.join(fullname, c)
        if contents is None:
            mkdir(fullname)
        else:
            if verbose: print "write", fullname
            f = open(fullname, "w")
            f.write(contents)
            if contents and contents[-1] != '\n':
                f.write('\n')
            f.close()

def mkdir(x):
    if verbose: print "mkdir", x
    os.mkdir(x)

def cleanout(root):
    names = os.listdir(root)
    for name in names:
        fullname = os.path.join(root, name)
        if os.path.isdir(fullname) and not os.path.islink(fullname):
            cleanout(fullname)
        else:
            os.remove(fullname)
    rmdir(root)

def rmdir(x):
    if verbose: print "rmdir", x
    os.rmdir(x)

def fixdir(lst):
    try:
        lst.remove('__builtins__')
    except ValueError:
        pass
    return lst

# Helper to run a test

def runtest(hier, code):
    root = tempfile.mktemp()
    mkhier(root, hier)
    savepath = sys.path[:]
    codefile = tempfile.mktemp()
    f = open(codefile, "w")
    f.write(code)
    f.close()
    try:
        sys.path.insert(0, root)
        if verbose: print "sys.path =", sys.path
        try:
            execfile(codefile, globals(), {})
        except:
            traceback.print_exc(file=sys.stdout)
    finally:
        sys.path[:] = savepath
        try:
            cleanout(root)
        except (os.error, IOError):
            pass
        os.remove(codefile)

# Test descriptions

tests = [
    ("t1", [("t1", None), ("t1 __init__.py", "")], "import t1"),
    
    ("t2", [
    ("t2", None),
    ("t2 __init__.py", "'doc for t2'; print __name__, 'loading'"),
    ("t2 sub", None),
    ("t2 sub __init__.py", ""),
    ("t2 sub subsub", None),
    ("t2 sub subsub __init__.py", "print __name__, 'loading'; spam = 1"),
    ],
"""
import t2
print t2.__doc__
import t2.sub
import t2.sub.subsub
print t2.__name__, t2.sub.__name__, t2.sub.subsub.__name__
import t2
from t2 import *
print dir()
from t2 import sub
from t2.sub import subsub
from t2.sub.subsub import spam
print sub.__name__, subsub.__name__
print sub.subsub.__name__
print dir()
import t2.sub
import t2.sub.subsub
print t2.__name__, t2.sub.__name__, t2.sub.subsub.__name__
from t2 import *
print dir()
"""),
    
    ("t3", [
    ("t3", None),
    ("t3 __init__.py", "print __name__, 'loading'"),
    ("t3 sub", None),
    ("t3 sub __init__.py", ""),
    ("t3 sub subsub", None),
    ("t3 sub subsub __init__.py", "print __name__, 'loading'; spam = 1"),
    ],
"""
import t3.sub.subsub
print t3.__name__, t3.sub.__name__, t3.sub.subsub.__name__
reload(t3)
reload(t3.sub)
reload(t3.sub.subsub)
"""),
    
    ("t4", [
    ("t4.py", "print 'THIS SHOULD NOT BE PRINTED (t4.py)'"),
    ("t4", None),
    ("t4 __init__.py", "print __name__, 'loading'"),
    ("t4 sub.py", "print 'THIS SHOULD NOT BE PRINTED (sub.py)'"),
    ("t4 sub", None),
    ("t4 sub __init__.py", ""),
    ("t4 sub subsub.py", "print 'THIS SHOULD NOT BE PRINTED (subsub.py)'"),
    ("t4 sub subsub", None),
    ("t4 sub subsub __init__.py", "print __name__, 'loading'; spam = 1"),
    ],
"""
from t4.sub.subsub import *
print "t4.sub.subsub.spam =", spam
"""),

    ("t5", [
    ("t5", None),
    ("t5 __init__.py", "import t5.foo"),
    ("t5 string.py", "print __name__, 'loading'; spam = 1"),
    ("t5 foo.py",
     "print __name__, 'loading'; import string; print string.spam"),
     ],
"""
import t5
from t5 import *
print dir()
import t5
print fixdir(dir(t5))
print fixdir(dir(t5.foo))
print fixdir(dir(t5.string))
"""),

    ("t6", [
    ("t6", None),
    ("t6 __init__.py", "__all__ = ['spam', 'ham', 'eggs']"),
    ("t6 spam.py", "print __name__, 'loading'"),
    ("t6 ham.py", "print __name__, 'loading'"),
    ("t6 eggs.py", "print __name__, 'loading'"),
    ],
"""
import t6
print fixdir(dir(t6))
from t6 import *
print fixdir(dir(t6))
print dir()
"""),
    
    ("t7", [
    ("t7.py", "print 'Importing t7.py'"),
    ("t7", None),
    ("t7 __init__.py", "print __name__, 'loading'"),
    ("t7 sub.py", "print 'THIS SHOULD NOT BE PRINTED (sub.py)'"),
    ("t7 sub", None),
    ("t7 sub __init__.py", ""),
    ("t7 sub subsub.py", "print 'THIS SHOULD NOT BE PRINTED (subsub.py)'"),
    ("t7 sub subsub", None),
    ("t7 sub subsub __init__.py", "print __name__, 'loading'; spam = 1"),
    ],
"""
t7, sub, subsub = None, None, None
import t7 as tas
print dir(tas)
assert not t7
from t7 import sub as subpar
print dir(subpar)
assert not t7 and not sub
from t7.sub import subsub as subsubsub
print dir(subsubsub)
assert not t7 and not sub and not subsub
from t7.sub.subsub import spam as ham
print "t7.sub.subsub.spam =", ham
assert not t7 and not sub and not subsub
"""),

]

nontests = [
    ("x5", [], ("import a" + ".a"*400)),
    ("x6", [], ("import a" + ".a"*499)),
    ("x7", [], ("import a" + ".a"*500)),
    ("x8", [], ("import a" + ".a"*1100)),
    ("x9", [], ("import " + "a"*400)),
    ("x10", [], ("import " + "a"*500)),
    ("x11", [], ("import " + "a"*998)),
    ("x12", [], ("import " + "a"*999)),
    ("x13", [], ("import " + "a"*999)),
    ("x14", [], ("import " + "a"*2000)),
]

"""XXX Things to test

import package without __init__
import package with __init__
__init__ importing submodule
__init__ importing global module
__init__ defining variables
submodule importing other submodule
submodule importing global module
submodule import submodule via global name
from package import submodule
from package import subpackage
from package import variable (defined in __init__)
from package import * (defined in __init__)
"""

# Run the tests

args = []
if __name__ == '__main__':
    args = sys.argv[1:]
    if args and args[0] == '-q':
        verbose = 0
        del args[0]

for name, hier, code in tests:
    if args and name not in args:
        print "skipping test", name
        continue
    print "running test", name
    runtest(hier, code)

# Test
import sys
import imp
try:
    import sys.imp
except ImportError:
    # This is what we expect
    pass
else:
    raise TestFailed, "No ImportError exception on 'import sys.imp'"
