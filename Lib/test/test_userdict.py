# Check every path through every method of UserDict

from test.test_support import verify, verbose
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

##########################
# Test Dict Mixin

from UserDict import DictMixin

class SeqDict(DictMixin):
    """Dictionary lookalike implemented with lists.

    Used to test and demonstrate DictMixin
    """
    def __init__(self):
        self.keylist = []
        self.valuelist = []
    def __getitem__(self, key):
        try:
            i = self.keylist.index(key)
        except ValueError:
            raise KeyError
        return self.valuelist[i]
    def __setitem__(self, key, value):
        self.keylist.append(key)
        self.valuelist.append(value)
    def __delitem__(self, key):
        try:
            i = self.keylist.index(key)
        except ValueError:
            raise KeyError
        self.keylist.pop(i)
        self.valuelist.pop(i)
    def keys(self):
        return list(self.keylist)

## Setup test and verify working of the test class
s = SeqDict()                   # check init
s[10] = 'ten'                   # exercise setitem
s[20] = 'twenty'
s[30] = 'thirty'
del s[20]                       # exercise delitem
verify(s[10] == 'ten')          # check getitem and setitem
verify(s.keys() == [10, 30])    # check keys() and delitem

## Now, test the DictMixin methods one by one
verify(s.has_key(10))                                       # has_key
verify(not s.has_key(20))

verify(10 in s)                                             # __contains__
verify(20 not in s)

verify([k for k in s] == [10, 30])                          # __iter__

verify(len(s) == 2)                                         # __len__

verify(list(s.iteritems()) == [(10,'ten'), (30, 'thirty')]) # iteritems

verify(list(s.iterkeys()) == [10, 30])                      # iterkeys

verify(list(s.itervalues()) == ['ten', 'thirty'])           # itervalues

verify(s.values() == ['ten', 'thirty'])                     # values

verify(s.items() == [(10,'ten'), (30, 'thirty')])           # items

verify(s.get(10) == 'ten')                                  # get
verify(s.get(15,'fifteen') == 'fifteen')
verify(s.get(15) == None)

verify(s.setdefault(40, 'forty') == 'forty')                # setdefault
verify(s.setdefault(10, 'null') == 'ten')
del s[40]

verify(s.pop(10) == 'ten')                                  # pop
verify(10 not in s)
s[10] = 'ten'

k, v = s.popitem()                                          # popitem
verify(k not in s)
s[k] = v

s.clear()                                                   # clear
verify(len(s) == 0)

try:                                                        # empty popitem
    s.popitem()
except KeyError:
    pass
else:
    verify(0, "popitem from an empty list should raise KeyError")

s.update({10: 'ten', 20:'twenty'})                          # update
verify(s[10]=='ten' and s[20]=='twenty')

verify(s == {10: 'ten', 20:'twenty'})                       # cmp
t = SeqDict()
t[20] = 'twenty'
t[10] = 'ten'
verify(s == t)
