# Check every path through every method of UserDict

from UserDict import UserDict

d0 = {}
d1 = {"one": 1}
d2 = {"one": 1, "two": 2}

# Test constructors

u = UserDict()
u0 = UserDict(d0)
u1 = UserDict(d1)
u2 = UserDict(d2)

uu = UserDict(u)
uu0 = UserDict(u0)
uu1 = UserDict(u1)
uu2 = UserDict(u2)

# Test __repr__

assert str(u0) == str(d0)
assert repr(u1) == repr(d1)
assert `u2` == `d2`

# Test __cmp__ and __len__

all = [d0, d1, d2, u, u0, u1, u2, uu, uu0, uu1, uu2]
for a in all:
    for b in all:
        assert cmp(a, b) == cmp(len(a), len(b))

# Test __getitem__

assert u2["one"] == 1
try:
    u1["two"]
except KeyError:
    pass
else:
    assert 0, "u1['two'] shouldn't exist"

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
    assert 0, "u3['three'] shouldn't exist"

# Test clear

u3.clear()
assert u3 == {}

# Test copy()

u2a = u2.copy()
assert u2a == u2

class MyUserDict(UserDict):
    def display(self): print self

m2 = MyUserDict(u2)
m2a = m2.copy()
assert m2a == m2

# Test keys, items, values

assert u2.keys() == d2.keys()
assert u2.items() == d2.items()
assert u2.values() == d2.values()

# Test has_key

for i in u2.keys():
    assert u2.has_key(i) == 1
    assert u1.has_key(i) == d1.has_key(i)
    assert u0.has_key(i) == d0.has_key(i)

# Test update

t = UserDict()
t.update(u2)
assert t == u2

# Test get

for i in u2.keys():
    assert u2.get(i) == u2[i]
    assert u1.get(i) == d1.get(i)
    assert u0.get(i) == d0.get(i)
