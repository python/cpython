# Backward compatibility -- you should use regrtest instead of this module.
import sys, regrtest
sys.argv[1:] = ["-vv"]
regrtest.main()
