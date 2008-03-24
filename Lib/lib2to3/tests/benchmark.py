#!/usr/bin/env python2.5
"""
This is a benchmarking script to test the speed of 2to3's pattern matching
system. It's equivalent to "refactor.py -f all" for every Python module
in sys.modules, but without engaging the actual transformations.
"""

__author__ = "Collin Winter <collinw at gmail.com>"

# Python imports
import os.path
import sys
from time import time

# Test imports
from .support import adjust_path
adjust_path()

# Local imports
from .. import refactor

### Mock code for refactor.py and the fixers
###############################################################################
class Options:
    def __init__(self, **kwargs):
        for k, v in kwargs.items():
            setattr(self, k, v)

        self.verbose = False

def dummy_transform(*args, **kwargs):
    pass

### Collect list of modules to match against
###############################################################################
files = []
for mod in sys.modules.values():
    if mod is None or not hasattr(mod, '__file__'):
        continue
    f = mod.__file__
    if f.endswith('.pyc'):
        f = f[:-1]
    if f.endswith('.py'):
        files.append(f)

### Set up refactor and run the benchmark
###############################################################################
options = Options(fix=["all"], print_function=False, doctests_only=False)
refactor = refactor.RefactoringTool(options)
for fixer in refactor.fixers:
    # We don't want them to actually fix the tree, just match against it.
    fixer.transform = dummy_transform

t = time()
for f in files:
    print "Matching", f
    refactor.refactor_file(f)
print "%d seconds to match %d files" % (time() - t, len(sys.modules))
