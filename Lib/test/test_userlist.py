# Check every path through every method of UserList

from UserList import UserList
from test_support import TestFailed

# Use check instead of assert so -O doesn't render the
# test useless.
def check(predicate, msg):
    if not predicate:
        raise TestFailed(msg + " failed")

l0 = []
l1 = [0]
l2 = [0, 1]

# Test constructors

u = UserList()
u0 = UserList(l0)
u1 = UserList(l1)
u2 = UserList(l2)

uu = UserList(u)
uu0 = UserList(u0)
uu1 = UserList(u1)
uu2 = UserList(u2)

v = UserList(tuple(u))
class OtherList:
    def __init__(self, initlist):
        self.__data = initlist
    def __len__(self):
        return len(self.__data)
    def __getitem__(self, i):
        return self.__data[i]
v0 = UserList(OtherList(u0))
vv = UserList("this is also a sequence")

# Test __repr__

check(str(u0) == str(l0), "str(u0) == str(l0)")
check(repr(u1) == repr(l1), "repr(u1) == repr(l1)")
check(`u2` == `l2`, "`u2` == `l2`")

# Test __cmp__ and __len__

def mycmp(a, b):
    r = cmp(a, b)
    if r < 0: return -1
    if r > 0: return 1
    return r

all = [l0, l1, l2, u, u0, u1, u2, uu, uu0, uu1, uu2]
for a in all:
    for b in all:
        check(mycmp(a, b) == mycmp(len(a), len(b)),
              "mycmp(a, b) == mycmp(len(a), len(b))")

# Test __getitem__

for i in range(len(u2)):
    check(u2[i] == i, "u2[i] == i")

# Test __setitem__

uu2[0] = 0
uu2[1] = 100
try:
    uu2[2] = 200
except IndexError:
    pass
else:
    raise TestFailed("uu2[2] shouldn't be assignable")

# Test __delitem__

del uu2[1]
del uu2[0]
try:
    del uu2[0]
except IndexError:
    pass
else:
    raise TestFailed("uu2[0] shouldn't be deletable")

# Test __getslice__

for i in range(-3, 4):
    check(u2[:i] == l2[:i], "u2[:i] == l2[:i]")
    check(u2[i:] == l2[i:], "u2[i:] == l2[i:]")
    for j in range(-3, 4):
        check(u2[i:j] == l2[i:j], "u2[i:j] == l2[i:j]")

# Test __setslice__

for i in range(-3, 4):
    u2[:i] = l2[:i]
    check(u2 == l2, "u2 == l2")
    u2[i:] = l2[i:]
    check(u2 == l2, "u2 == l2")
    for j in range(-3, 4):
        u2[i:j] = l2[i:j]
        check(u2 == l2, "u2 == l2")

uu2 = u2[:]
uu2[:0] = [-2, -1]
check(uu2 == [-2, -1, 0, 1], "uu2 == [-2, -1, 0, 1]")
uu2[0:] = []
check(uu2 == [], "uu2 == []")

# Test __contains__
for i in u2:
    check(i in u2, "i in u2")
for i in min(u2)-1, max(u2)+1:
    check(i not in u2, "i not in u2")

# Test __delslice__

uu2 = u2[:]
del uu2[1:2]
del uu2[0:1]
check(uu2 == [], "uu2 == []")

uu2 = u2[:]
del uu2[1:]
del uu2[:1]
check(uu2 == [], "uu2 == []")

# Test __add__, __radd__, __mul__ and __rmul__

check(u1 + [] == [] + u1 == u1, "u1 + [] == [] + u1 == u1")
check(u1 + [1] == u2, "u1 + [1] == u2")
check([-1] + u1 == [-1, 0], "[-1] + u1 == [-1, 0]")
check(u2 == u2*1 == 1*u2, "u2 == u2*1 == 1*u2")
check(u2+u2 == u2*2 == 2*u2, "u2+u2 == u2*2 == 2*u2")
check(u2+u2+u2 == u2*3 == 3*u2, "u2+u2+u2 == u2*3 == 3*u2")

# Test append

u = u1[:]
u.append(1)
check(u == u2, "u == u2")

# Test insert

u = u2[:]
u.insert(0, -1)
check(u == [-1, 0, 1], "u == [-1, 0, 1]")

# Test pop

u = [-1] + u2
u.pop()
check(u == [-1, 0], "u == [-1, 0]")
u.pop(0)
check(u == [0], "u == [0]")

# Test remove

u = u2[:]
u.remove(1)
check(u == u1, "u == u1")

# Test count
u = u2*3
check(u.count(0) == 3, "u.count(0) == 3")
check(u.count(1) == 3, "u.count(1) == 3")
check(u.count(2) == 0, "u.count(2) == 0")


# Test index

check(u2.index(0) == 0, "u2.index(0) == 0")
check(u2.index(1) == 1, "u2.index(1) == 1")
try:
    u2.index(2)
except ValueError:
    pass
else:
    raise TestFailed("expected ValueError")

# Test reverse

u = u2[:]
u.reverse()
check(u == [1, 0], "u == [1, 0]")
u.reverse()
check(u == u2, "u == u2")

# Test sort

u = UserList([1, 0])
u.sort()
check(u == u2, "u == u2")

# Test extend

u = u1[:]
u.extend(u2)
check(u == u1 + u2, "u == u1 + u2")
