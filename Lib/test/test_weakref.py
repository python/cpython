import sys
import weakref

from test_support import TestFailed, verify


class C:
    pass


print "Basic Weak References"

print "-- Liveness and referent identity"

o = C()
ref = weakref.ref(o)
verify(ref() is not None, "weak reference to live object should be live")
o2 = ref()
verify(ref() is not None, "weak ref should still be live")
verify(o is o2, "<ref>() should return original object if live")
del o, o2
del ref

cbcalled = 0
def callback(o):
    global cbcalled
    cbcalled = 1

o = C()
ref2 = weakref.ref(o, callback)
del o
verify(cbcalled,
       "callback did not properly set 'cbcalled'")
verify(ref2() is None,
       "ref2 should be dead after deleting object reference")
del ref2


print "-- Reference objects with callbacks"
o = C()
o.bar = 1
ref1 = weakref.ref(o, id)
ref2 = weakref.ref(o, id)
del o
verify(ref1() is None,
       "expected reference to be invalidated")
verify(ref2() is None,
       "expected reference to be invalidated")


print "-- Proxy objects with callbacks"
o = C()
o.bar = 1
ref1 = weakref.proxy(o, id)
ref2 = weakref.proxy(o, id)
del o
try:
    ref1.bar
except weakref.ReferenceError:
    pass
else:
    raise TestFailed("expected ReferenceError exception")
try:
    ref2.bar
except weakref.ReferenceError:
    pass
else:
    raise TestFailed("expected ReferenceError exception")


print "-- Re-use of weak reference objects"
print "     reference objects"

o = C()
ref1 = weakref.ref(o)
# create a proxy to make sure that there's an intervening creation
# between these two; it should make no difference
proxy = weakref.proxy(o)
ref2 = weakref.ref(o)
verify(ref1 is ref2,
       "reference object w/out callback should have been re-used")

o = C()
proxy = weakref.proxy(o)
ref1 = weakref.ref(o)
ref2 = weakref.ref(o)
verify(ref1 is ref2,
       "reference object w/out callback should have been re-used")
verify(weakref.getweakrefcount(o) == 2,
       "wrong weak ref count for object")
del proxy
verify(weakref.getweakrefcount(o) == 1,
       "wrong weak ref count for object after deleting proxy")

print "     proxy objects"

o = C()
ref3 = weakref.proxy(o)
ref4 = weakref.proxy(o)
verify(ref3 is ref4,
       "proxy object w/out callback should have been re-used")


def clearing1(r):
    print "clearing ref 1"

def clearing2(r):
    print "clearing ref 2"

o = C()
ref1 = weakref.ref(o, clearing1)
ref2 = weakref.ref(o, clearing2)
verify(weakref.getweakrefcount(o) == 2,
       "got wrong number of weak reference objects")
del o

o = C()
ref1 = weakref.ref(o, clearing1)
ref2 = weakref.ref(o, clearing2)
del ref1
verify(weakref.getweakrefs(o) == [ref2],
       "list of refs does not match")
del o

o = C()
ref1 = weakref.ref(o, clearing1)
ref2 = weakref.ref(o, clearing2)
del ref2
verify(weakref.getweakrefs(o) == [ref1],
       "list of refs does not match")
del o

print
print "Weak Valued Dictionaries"

class Object:
    def __init__(self, arg):
        self.arg = arg
    def __repr__(self):
        return "<Object %r>" % self.arg

dict = weakref.mapping()
objects = map(Object, range(10))
for o in objects:
    dict[o.arg] = o
print "objects are stored in weak dict"
for o in objects:
    verify(weakref.getweakrefcount(o) == 1,
           "wrong number of weak references to %r!" % o)
    verify(o is dict[o.arg],
           "wrong object returned by weak dict!")
items1 = dict.items()
items2 = dict.copy().items()
items1.sort()
items2.sort()
verify(items1 == items2,
       "cloning of weak-valued dictionary did not work!")
del items1, items2
dict.clear()
print "weak dict test complete"

print
print "Weak Keyed Dictionaries"

dict = weakref.mapping(weakkeys=1)
objects = map(Object, range(10))
for o in objects:
    dict[o] = o.arg
print "objects are stored in weak dict"
for o in objects:
    verify(weakref.getweakrefcount(o) == 1,
           "wrong number of weak references to %r!" % o)
    verify(o.arg is dict[o],
           "wrong object returned by weak dict!")
items1 = dict.items()
items2 = dict.copy().items()
items1.sort()
items2.sort()
verify(items1 == items2,
       "cloning of weak-keyed dictionary did not work!")
del items1, items2
del objects, o
verify(len(dict)==0, "deleting the keys did not clear the dictionary")
print "weak key dict test complete"


print
print "Non-callable Proxy References"
print "XXX -- tests not written!"


def test_proxy(o, proxy):
    o.foo = 1
    verify(proxy.foo == 1,
           "proxy does not reflect attribute addition")
    o.foo = 2
    verify(proxy.foo == 2,
           "proxy does not reflect attribute modification")
    del o.foo
    verify(not hasattr(proxy, 'foo'),
           "proxy does not reflect attribute removal")

    proxy.foo = 1
    verify(o.foo == 1,
           "object does not reflect attribute addition via proxy")
    proxy.foo = 2
    verify(o.foo == 2,
           "object does not reflect attribute modification via proxy")
    del proxy.foo
    verify(not hasattr(o, 'foo'),
           "object does not reflect attribute removal via proxy")


o = C()
test_proxy(o, weakref.proxy(o))

print
print "Callable Proxy References"

class Callable:
    bar = None
    def __call__(self, x):
        self.bar = x

o = Callable()
ref1 = weakref.proxy(o)

test_proxy(o, ref1)

verify(type(ref1) is weakref.CallableProxyType,
       "proxy is not of callable type")
ref1('twinkies!')
verify(o.bar == 'twinkies!',
       "call through proxy not passed through to original")

try:
    ref1()
except TypeError:
    # expect due to too few args
    pass
else:
    raise TestFailed("did not catch expected TypeError -- too few args")

try:
    ref1(1, 2, 3)
except TypeError:
    # expect due to too many args
    pass
else:
    raise TestFailed("did not catch expected TypeError -- too many args")
