import sys
import os
from array import array

from test_support import verify, TESTFN, TestFailed
from UserList import UserList

# verify writelines with instance sequence
l = UserList(['1', '2'])
f = open(TESTFN, 'wb')
f.writelines(l)
f.close()
f = open(TESTFN, 'rb')
buf = f.read()
f.close()
verify(buf == '12')

# verify readinto
a = array('c', 'x'*10)
f = open(TESTFN, 'rb')
n = f.readinto(a)
f.close()
verify(buf == a.tostring()[:n])

# verify writelines with integers
f = open(TESTFN, 'wb')
try:
    f.writelines([1, 2, 3])
except TypeError:
    pass
else:
    print "writelines accepted sequence of integers"
f.close()

# verify writelines with integers in UserList
f = open(TESTFN, 'wb')
l = UserList([1,2,3])
try:
    f.writelines(l)
except TypeError:
    pass
else:
    print "writelines accepted sequence of integers"
f.close()

# verify writelines with non-string object
class NonString: pass

f = open(TESTFN, 'wb')
try:
    f.writelines([NonString(), NonString()])
except TypeError:
    pass
else:
    print "writelines accepted sequence of non-string objects"
f.close()

# verify that we get a sensible error message for bad mode argument
bad_mode = "qwerty"
try:
    open(TESTFN, bad_mode)
except IOError, msg:
    if msg[0] != 0:
        s = str(msg)
        if s.find(TESTFN) != -1 or s.find(bad_mode) == -1:
            print "bad error message for invalid mode: %s" % s
    # if msg[0] == 0, we're probably on Windows where there may be
    # no obvious way to discover why open() failed.
else:
    print "no error for invalid mode: %s" % bad_mode

f = open(TESTFN)
if f.name != TESTFN:
    raise TestFailed, 'file.name should be "%s"' % TESTFN
if f.isatty():
    raise TestFailed, 'file.isatty() should be false'

if f.closed:
    raise TestFailed, 'file.closed should be false'

try:
    f.readinto("")
except TypeError:
    pass
else:
    raise TestFailed, 'file.readinto("") should raise a TypeError'

f.close()
if not f.closed:
    raise TestFailed, 'file.closed should be true'

methods = ['fileno', 'flush', 'isatty', 'read', 'readinto', 'readline', 'readlines', 'seek', 'tell', 'truncate', 'write', 'xreadlines' ]
if sys.platform.startswith('atheos'):
    methods.remove('truncate')

for methodname in methods:
    method = getattr(f, methodname)
    try:
        method()
    except ValueError:
        pass
    else:
        raise TestFailed, 'file.%s() on a closed file should raise a ValueError' % methodname

try:
    f.writelines([])
except ValueError:
    pass
else:
    raise TestFailed, 'file.writelines([]) on a closed file should raise a ValueError'

os.unlink(TESTFN)
