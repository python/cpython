"""
Fodder for module finalization tests in test_module.
"""

import shutil
import sys
import test.final_a

x = 'b'

class C:
    def __del__(self, sys=sys):
        # Inspect module globals and builtins
        print("x =", x, file=sys.stderr)
        print("final_a.x =", test.final_a.x, file=sys.stderr)
        print("shutil.rmtree =", getattr(shutil.rmtree, '__name__', None), file=sys.stderr)
        print("len =", getattr(len, '__name__', None), file=sys.stderr)

c = C()
_underscored = C()
