from UserList import UserList

def f(*a, **k):
    print a, k

def g(x, *y, **z):
    print x, y, z

def h(j=1, a=2, h=3):
    print j, a, h

f()
f(1)
f(1, 2)
f(1, 2, 3)

f(1, 2, 3, *(4, 5))
f(1, 2, 3, *[4, 5])
f(1, 2, 3, *UserList([4, 5])
f(1, 2, 3, **{'a':4, 'b':5})
f(1, 2, 3, *(4, 5), **{'a':6, 'b':7})
f(1, 2, 3, x=4, y=5, *(6, 7), **{'a':8, 'b':9})

try:
    g()
except TypeError, err:
    print "TypeError:", err
else:
    print "should raise TypeError: not enough arguments; expected 1, got 0"
    
try:
    g(*())
except TypeError, err:
    print "TypeError:", err
else:
    print "should raise TypeError: not enough arguments; expected 1, got 0"
    
try:
    g(*(), **{})
except TypeError, err:
    print "TypeError:", err
else:
    print "should raise TypeError: not enough arguments; expected 1, got 0"
    
g(1)
g(1, 2)
g(1, 2, 3)
g(1, 2, 3, *(4, 5))
try:
    g(1, 2, 3, **{'x':4, 'y':5})
except TypeError, err:
    print err
else:
    print "should raise TypeError: keyword parameter redefined"
    
try:
    g(1, 2, 3, a=4, b=5, *(6, 7), **{'a':8, 'b':9})
except TypeError, err:
    print err
else:
    print "should raise TypeError: keyword parameter redefined"

try:
    f(**{1:2})
except TypeError, err:
    print err
else:
    print "should raise TypeError: keywords must be strings"

try:
    h(**{'e': 2})
except TypeError, err:
    print err
else:
    print "should raise TypeError: unexpected keyword argument: e"

try:
    h(*h)
except TypeError, err:
    print err
else:
    print "should raise TypeError: * argument must be a tuple"

try:
    h(**h)
except TypeError, err:
    print err
else:
    print "should raise TypeError: ** argument must be a dictionary"

def f2(*a, **b):
    return a, b

d = {}
for i in range(512):
    key = 'k%d' % i
    d[key] = i
a, b = f2(1, *(2, 3), **d)
print len(a), len(b), b == d
