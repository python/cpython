from test.test_support import verify, verbose, TestFailed, sortdict
from collections import UserDict, UserList

def e(a, b):
    print(a, b)

def f(*a, **k):
    print(a, sortdict(k))

def g(x, *y, **z):
    print(x, y, sortdict(z))

def h(j=1, a=2, h=3):
    print(j, a, h)

f()
f(1)
f(1, 2)
f(1, 2, 3)

f(1, 2, 3, *(4, 5))
f(1, 2, 3, *[4, 5])
f(1, 2, 3, *UserList([4, 5]))
f(1, 2, 3, **{'a':4, 'b':5})
f(1, 2, 3, *(4, 5), **{'a':6, 'b':7})
f(1, 2, 3, x=4, y=5, *(6, 7), **{'a':8, 'b':9})


f(1, 2, 3, **UserDict(a=4, b=5))
f(1, 2, 3, *(4, 5), **UserDict(a=6, b=7))
f(1, 2, 3, x=4, y=5, *(6, 7), **UserDict(a=8, b=9))


# Verify clearing of SF bug #733667
try:
    e(c=3)
except TypeError:
    pass
else:
    print("should raise TypeError: e() got an unexpected keyword argument 'c'")

try:
    g()
except TypeError as err:
    print("TypeError:", err)
else:
    print("should raise TypeError: not enough arguments; expected 1, got 0")

try:
    g(*())
except TypeError as err:
    print("TypeError:", err)
else:
    print("should raise TypeError: not enough arguments; expected 1, got 0")

try:
    g(*(), **{})
except TypeError as err:
    print("TypeError:", err)
else:
    print("should raise TypeError: not enough arguments; expected 1, got 0")

g(1)
g(1, 2)
g(1, 2, 3)
g(1, 2, 3, *(4, 5))
class Nothing: pass
try:
    g(*Nothing())
except TypeError as attr:
    pass
else:
    print("should raise TypeError")

class Nothing:
    def __len__(self):
        return 5
try:
    g(*Nothing())
except TypeError as attr:
    pass
else:
    print("should raise TypeError")

class Nothing:
    def __len__(self):
        return 5
    def __getitem__(self, i):
        if i < 3:
            return i
        else:
            raise IndexError(i)
g(*Nothing())

class Nothing:
    def __init__(self):
        self.c = 0
    def __iter__(self):
        return self
try:
    g(*Nothing())
except TypeError as attr:
    pass
else:
    print("should raise TypeError")

class Nothing:
    def __init__(self):
        self.c = 0
    def __iter__(self):
        return self
    def __next__(self):
        if self.c == 4:
            raise StopIteration
        c = self.c
        self.c += 1
        return c
g(*Nothing())

# make sure the function call doesn't stomp on the dictionary?
d = {'a': 1, 'b': 2, 'c': 3}
d2 = d.copy()
verify(d == d2)
g(1, d=4, **d)
print(sortdict(d))
print(sortdict(d2))
verify(d == d2, "function call modified dictionary")

# what about willful misconduct?
def saboteur(**kw):
    kw['x'] = locals() # yields a cyclic kw
    return kw
d = {}
kw = saboteur(a=1, **d)
verify(d == {})
# break the cycle
del kw['x']

try:
    g(1, 2, 3, **{'x':4, 'y':5})
except TypeError as err:
    print(err)
else:
    print("should raise TypeError: keyword parameter redefined")

try:
    g(1, 2, 3, a=4, b=5, *(6, 7), **{'a':8, 'b':9})
except TypeError as err:
    print(err)
else:
    print("should raise TypeError: keyword parameter redefined")

try:
    f(**{1:2})
except TypeError as err:
    print(err)
else:
    print("should raise TypeError: keywords must be strings")

try:
    h(**{'e': 2})
except TypeError as err:
    print(err)
else:
    print("should raise TypeError: unexpected keyword argument: e")

try:
    h(*h)
except TypeError as err:
    print(err)
else:
    print("should raise TypeError: * argument must be a tuple")

try:
    dir(*h)
except TypeError as err:
    print(err)
else:
    print("should raise TypeError: * argument must be a tuple")

try:
    None(*h)
except TypeError as err:
    print(err)
else:
    print("should raise TypeError: * argument must be a tuple")

try:
    h(**h)
except TypeError as err:
    print(err)
else:
    print("should raise TypeError: ** argument must be a dictionary")

try:
    dir(**h)
except TypeError as err:
    print(err)
else:
    print("should raise TypeError: ** argument must be a dictionary")

try:
    None(**h)
except TypeError as err:
    print(err)
else:
    print("should raise TypeError: ** argument must be a dictionary")

try:
    dir(b=1,**{'b':1})
except TypeError as err:
    print(err)
else:
    print("should raise TypeError: dir() got multiple values for keyword argument 'b'")

def f2(*a, **b):
    return a, b

d = {}
for i in range(512):
    key = 'k%d' % i
    d[key] = i
a, b = f2(1, *(2, 3), **d)
print(len(a), len(b), b == d)

class Foo:
    def method(self, arg1, arg2):
        return arg1 + arg2

x = Foo()
print(Foo.method(*(x, 1, 2)))
print(Foo.method(x, *(1, 2)))
print(Foo.method(*(1, 2, 3)))
print(Foo.method(1, *(2, 3)))

# A PyCFunction that takes only positional parameters should allow an
# empty keyword dictionary to pass without a complaint, but raise a
# TypeError if the dictionary is non-empty.
id(1, **{})
try:
    id(1, **{"foo": 1})
except TypeError:
    pass
else:
    raise TestFailed('expected TypeError; no exception raised')

a, b, d, e, v, k = 'A', 'B', 'D', 'E', 'V', 'K'
funcs = []
maxargs = {}
for args in ['', 'a', 'ab']:
    for defargs in ['', 'd', 'de']:
        for vararg in ['', 'v']:
            for kwarg in ['', 'k']:
                name = 'z' + args + defargs + vararg + kwarg
                arglist = list(args) + ['%s="%s"' % (x, x) for x in defargs]
                if vararg: arglist.append('*' + vararg)
                if kwarg: arglist.append('**' + kwarg)
                decl = (('def %s(%s): print("ok %s", a, b, d, e, v, ' +
                         'type(k) is type ("") and k or sortdict(k))')
                         % (name, ', '.join(arglist), name))
                exec(decl)
                func = eval(name)
                funcs.append(func)
                maxargs[func] = len(args + defargs)

for name in ['za', 'zade', 'zabk', 'zabdv', 'zabdevk']:
    func = eval(name)
    for args in [(), (1, 2), (1, 2, 3, 4, 5)]:
        for kwargs in ['', 'a', 'd', 'ad', 'abde']:
            kwdict = {}
            for k in kwargs: kwdict[k] = k + k
            print(func.__name__, args, sortdict(kwdict), '->', end=' ')
            try: func(*args, **kwdict)
            except TypeError as err: print(err)
