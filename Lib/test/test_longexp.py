# this test has a malloc problem on OS/2+EMX, so skip test in that environment

import sys
from test_support import TestFailed, TestSkipped

REPS = 65580

if sys.platform == 'mac':
	import gestalt
	if gestalt.gestalt('sysv') > 0x9ff:
		raise TestSkipped, 'Triggers pathological malloc slowdown on OSX MacPython'
if sys.platform == "os2emx":
    raise TestFailed, "OS/2+EMX port has malloc problems with long expressions"
else:
    l = eval("[" + "2," * REPS + "]")
    print len(l)
