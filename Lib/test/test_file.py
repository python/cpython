from test_support import TESTFN
from UserList import UserList

# verify writelines with instance sequence
l = UserList(['1', '2'])
f = open(TESTFN, 'wb')
f.writelines(l)
f.close()
f = open(TESTFN, 'rb')
buf = f.read()
f.close()
assert buf == '12'

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
