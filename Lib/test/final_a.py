"""
Fodder for module finalization tests in test_module.
"""

import shutil
import sys
import test.final_b

x = 'a'

class C:
    def __del__(self, sys=sys):
        # Inspect module globals and builtins
        print("x =", x, file=sys.stderr)
        print("final_b.x =", test.final_b.x, file=sys.stderr)
        print("shutil.rmtree =", getattr(shutil.rmtree, '__name__', None), sys.stderr)
        print("len =", getattr(len, '__name__', None), sys.stderr)

c = C()
_underscored = C()
