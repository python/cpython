# Backward compatibility -- you should use regrtest instead of this module.
from warnings import warnpy3k
warnpy3k("the test.testall module has been removed in Python 3.0",
            stacklevel=2)
del warnpy3k


import sys, regrtest
sys.argv[1:] = ["-vv"]
regrtest.main()
