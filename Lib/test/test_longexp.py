import sys
from test_support import TestSkipped

REPS = 65580

if sys.platform == 'mac':
    import gestalt
    if gestalt.gestalt('sysv') > 0x9ff:
        raise TestSkipped, 'Triggers pathological malloc slowdown on OSX MacPython'

l = eval("[" + "2," * REPS + "]")
print len(l)
