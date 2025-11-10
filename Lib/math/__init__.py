"""
math module -- Mathematical functions
"""

from _math import *

# gh-140824: Fix module name for pickle
def patch_module(objs, module):
    for obj in objs:
        if not hasattr(obj, "__module__"):
            continue
        obj.__module__ = module
patch_module([obj for name, obj in globals().items()
              if not name.startswith('_')], 'math')

from _math_integer import comb, factorial, gcd, isqrt, lcm, perm
patch_module([comb, factorial, gcd, isqrt, lcm, perm], 'math.integer')

del patch_module
