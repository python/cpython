# Test the atexit module.
from test_support import TESTFN, vereq, is_jython
import atexit
from os import popen, unlink
import sys

executable = sys.executable
if is_jython:
    executable = "jython"

input = """\
import atexit

def handler1():
    print "handler1"

def handler2(*args, **kargs):
    print "handler2", args, kargs

atexit.register(handler1)
atexit.register(handler2)
atexit.register(handler2, 7, kw="abc")
"""

fname = TESTFN + ".py"
f = file(fname, "w")
f.write(input)
f.close()

p = popen("%s %s" % (executable, fname))
output = p.read()
p.close()
vereq(output, """\
handler2 (7,) {'kw': 'abc'}
handler2 () {}
handler1
""")

input = """\
def direct():
    print "direct exit"

import sys
sys.exitfunc = direct

# Make sure atexit doesn't drop
def indirect():
    print "indirect exit"

import atexit
atexit.register(indirect)
"""

f = file(fname, "w")
f.write(input)
f.close()

p = popen("%s %s" % (executable, fname))
output = p.read()
p.close()
vereq(output, """\
indirect exit
direct exit
""")

unlink(fname)
