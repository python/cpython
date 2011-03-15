# The following example may crash or not depending on the platform.
# E.g. on 32-bit Intel Linux in a "standard" configuration it seems to
# crash on Python 2.5 (but not 2.4 nor 2.3).  On Windows the import
# eventually fails to find the module, possibly because we run out of
# file handles.

# The point of this example is to show that sys.setrecursionlimit() is a
# hack, and not a robust solution.  This example simply exercises a path
# where it takes many C-level recursions, consuming a lot of stack
# space, for each Python-level recursion.  So 1000 times this amount of
# stack space may be too much for standard platforms already.

import sys
if 'recursion_limit_too_high' in sys.modules:
    del sys.modules['recursion_limit_too_high']
import recursion_limit_too_high
