# Check every path through every method of UserList

from UserList import UserList

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

assert str(u0) == str(l0)
assert repr(u1) == repr(l1)
assert `u2` == `l2`

# Test __cmp__ and __len__

def mycmp(a, b):
    r = cmp(a, b)
    if r < 0: return -1
    if r > 0: return 1
    return r

all = [l0, l1, l2, u, u0, u1, u2, uu, uu0, uu1, uu2]
for a in all:
    for b in all:
        assert mycmp(a, b) == mycmp(len(a), len(b))

# Test __getitem__

for i in range(len(u2)):
    assert u2[i] == i

# Test __setitem__

uu2[0] = 0
uu2[1] = 100
try:
    uu2[2] = 200
except IndexError:
    pass
else:
    assert 0, "uu2[2] shouldn't be assignable"

# Test __delitem__

del uu2[1]
del uu2[0]
try:
    del uu2[0]
except IndexError:
    pass
else:
    assert 0, "uu2[0] shouldn't be deletable"

# Test __getslice__

for i in range(-3, 4):
    assert u2[:i] == l2[:i]
    assert u2[i:] == l2[i:]
    for j in range(-3, 4):
        assert u2[i:j] == l2[i:j]

# Test __setslice__

for i in range(-3, 4):
    u2[:i] = l2[:i]
    assert u2 == l2
    u2[i:] = l2[i:]
    assert u2 == l2
    for j in range(-3, 4):
        u2[i:j] = l2[i:j]
        assert u2 == l2

uu2 = u2[:]
uu2[:0] = [-2, -1]
assert uu2 == [-2, -1, 0, 1]
uu2[0:] = []
assert uu2 == []

# Test __delslice__

uu2 = u2[:]
del uu2[1:2]
del uu2[0:1]
assert uu2 == []

uu2 = u2[:]
del uu2[1:]
del uu2[:1]
assert uu2 == []

# Test __add__, __radd__, __mul__ and __rmul__

assert u1 + [] == [] + u1 == u1
assert u1 + [1] == u2
assert [-1] + u1 == [-1, 0]
assert u2 == u2*1 == 1*u2
assert u2+u2 == u2*2 == 2*u2
assert u2+u2+u2 == u2*3 == 3*u2

# Test append

u = u1[:]
u.append(1)
assert u == u2

# Test insert

u = u2[:]
u.insert(0, -1)
assert u == [-1, 0, 1]

# Test pop

u = [-1] + u2
u.pop()
assert u == [-1, 0]
u.pop(0)
assert u == [0]

# Test remove

u = u2[:]
u.remove(1)
assert u == u1

# Test count
u = u2*3
assert u.count(0) == 3
assert u.count(1) == 3
assert u.count(2) == 0


# Test index

assert u2.index(0) == 0
assert u2.index(1) == 1
try:
    u2.index(2)
except ValueError:
    pass
else:
    assert 0, "expected ValueError"

# Test reverse

u = u2[:]
u.reverse()
assert u == [1, 0]
u.reverse()
assert u == u2

# Test sort

u = UserList([1, 0])
u.sort()
assert u == u2

# Test extend

u = u1[:]
u.extend(u2)
assert u == u1 + u2

