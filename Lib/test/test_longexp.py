# this test has a malloc problem on OS/2+EMX, so skip test in that environment

import sys
from test_support import TestFailed

REPS = 65580

if sys.platform == "os2emx":
    raise TestFailed, "OS/2+EMX port has malloc problems with long expressions"
else:
    l = eval("[" + "2," * REPS + "]")
    print len(l)
