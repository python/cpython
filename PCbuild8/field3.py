# An absurd workaround for the lack of arithmetic in MS's resource compiler.
# After building Python, run this, then paste the output into the appropriate
# part of PC\python_nt.rc.
# Example output:
#
# * For 2.3a0,
# * PY_MICRO_VERSION = 0
# * PY_RELEASE_LEVEL = 'alpha' = 0xA
# * PY_RELEASE_SERIAL = 1
# *
# * and 0*1000 + 10*10 + 1 = 101.
# */
# #define FIELD3 101

import sys

major, minor, micro, level, serial = sys.version_info
levelnum = {'alpha': 0xA,
            'beta': 0xB,
            'candidate': 0xC,
            'final': 0xF,
           }[level]
string = sys.version.split()[0] # like '2.3a0'

print " * For %s," % string
print " * PY_MICRO_VERSION = %d" % micro
print " * PY_RELEASE_LEVEL = %r = %s" % (level, hex(levelnum))
print " * PY_RELEASE_SERIAL = %d" % serial
print " *"

field3 = micro * 1000 + levelnum * 10 + serial

print " * and %d*1000 + %d*10 + %d = %d" % (micro, levelnum, serial, field3)
print " */"
print "#define FIELD3", field3
