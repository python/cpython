# Check every path through every method of UserDict

from test_support import verify, verbose
from UserDict import UserDict, IterableUserDict

d0 = {}
d1 = {"one": 1}
d2 = {"one": 1, "two": 2}

# Test constructors

u = UserDict()
u0 = UserDict(d0)
u1 = UserDict(d1)
u2 = IterableUserDict(d2)

uu = UserDict(u)
uu0 = UserDict(u0)
uu1 = UserDict(u1)
uu2 = UserDict(u2)

# Test __repr__

verify(str(u0) == str(d0))
verify(repr(u1) == repr(d1))
verify(`u2` == `d2`)

# Test __cmp__ and __len__

all = [d0, d1, d2, u, u0, u1, u2, uu, uu0, uu1, uu2]
for a in all:
    for b in all:
        verify(cmp(a, b) == cmp(len(a), len(b)))

# Test __getitem__

verify(u2["one"] == 1)
try:
    u1["two"]
except KeyError:
    pass
else:
    verify(0, "u1['two'] shouldn't exist")

# Test __setitem__

u3 = UserDict(u2)
u3["two"] = 2
u3["three"] = 3

# Test __delitem__

del u3["three"]
try:
    del u3["three"]
except KeyError:
    pass
else:
    verify(0, "u3['three'] shouldn't exist")

# Test clear

u3.clear()
verify(u3 == {})

# Test copy()

u2a = u2.copy()
verify(u2a == u2)

class MyUserDict(UserDict):
    def display(self): print self

m2 = MyUserDict(u2)
m2a = m2.copy()
verify(m2a == m2)

# SF bug #476616 -- copy() of UserDict subclass shared data
m2['foo'] = 'bar'
verify(m2a != m2)

# Test keys, items, values

verify(u2.keys() == d2.keys())
verify(u2.items() == d2.items())
verify(u2.values() == d2.values())

# Test has_key and "in".

for i in u2.keys():
    verify(u2.has_key(i) == 1)
    verify((i in u2) == 1)
    verify(u1.has_key(i) == d1.has_key(i))
    verify((i in u1) == (i in d1))
    verify(u0.has_key(i) == d0.has_key(i))
    verify((i in u0) == (i in d0))

# Test update

t = UserDict()
t.update(u2)
verify(t == u2)

# Test get

for i in u2.keys():
    verify(u2.get(i) == u2[i])
    verify(u1.get(i) == d1.get(i))
    verify(u0.get(i) == d0.get(i))

# Test "in" iteration.
for i in xrange(20):
    u2[i] = str(i)
ikeys = []
for k in u2:
    ikeys.append(k)
ikeys.sort()
keys = u2.keys()
keys.sort()
verify(ikeys == keys)
