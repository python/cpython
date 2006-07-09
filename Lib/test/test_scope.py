from test.test_support import verify, TestFailed, check_syntax, vereq

import warnings
warnings.filterwarnings("ignore", r"import \*", SyntaxWarning, "<string>")

print "1. simple nesting"

def make_adder(x):
    def adder(y):
        return x + y
    return adder

inc = make_adder(1)
plus10 = make_adder(10)

vereq(inc(1), 2)
vereq(plus10(-2), 8)

print "2. extra nesting"

def make_adder2(x):
    def extra(): # check freevars passing through non-use scopes
        def adder(y):
            return x + y
        return adder
    return extra()

inc = make_adder2(1)
plus10 = make_adder2(10)

vereq(inc(1), 2)
vereq(plus10(-2), 8)

print "3. simple nesting + rebinding"

def make_adder3(x):
    def adder(y):
        return x + y
    x = x + 1 # check tracking of assignment to x in defining scope
    return adder

inc = make_adder3(0)
plus10 = make_adder3(9)

vereq(inc(1), 2)
vereq(plus10(-2), 8)

print "4. nesting with global but no free"

def make_adder4(): # XXX add exta level of indirection
    def nest():
        def nest():
            def adder(y):
                return global_x + y # check that plain old globals work
            return adder
        return nest()
    return nest()

global_x = 1
adder = make_adder4()
vereq(adder(1), 2)

global_x = 10
vereq(adder(-2), 8)

print "5. nesting through class"

def make_adder5(x):
    class Adder:
        def __call__(self, y):
            return x + y
    return Adder()

inc = make_adder5(1)
plus10 = make_adder5(10)

vereq(inc(1), 2)
vereq(plus10(-2), 8)

print "6. nesting plus free ref to global"

def make_adder6(x):
    global global_nest_x
    def adder(y):
        return global_nest_x + y
    global_nest_x = x
    return adder

inc = make_adder6(1)
plus10 = make_adder6(10)

vereq(inc(1), 11) # there's only one global
vereq(plus10(-2), 8)

print "7. nearest enclosing scope"

def f(x):
    def g(y):
        x = 42 # check that this masks binding in f()
        def h(z):
            return x + z
        return h
    return g(2)

test_func = f(10)
vereq(test_func(5), 47)

print "8. mixed freevars and cellvars"

def identity(x):
    return x

def f(x, y, z):
    def g(a, b, c):
        a = a + x # 3
        def h():
            # z * (4 + 9)
            # 3 * 13
            return identity(z * (b + y))
        y = c + z # 9
        return h
    return g

g = f(1, 2, 3)
h = g(2, 4, 6)
vereq(h(), 39)

print "9. free variable in method"

def test():
    method_and_var = "var"
    class Test:
        def method_and_var(self):
            return "method"
        def test(self):
            return method_and_var
        def actual_global(self):
            return str("global")
        def str(self):
            return str(self)
    return Test()

t = test()
vereq(t.test(), "var")
vereq(t.method_and_var(), "method")
vereq(t.actual_global(), "global")

method_and_var = "var"
class Test:
    # this class is not nested, so the rules are different
    def method_and_var(self):
        return "method"
    def test(self):
        return method_and_var
    def actual_global(self):
        return str("global")
    def str(self):
        return str(self)

t = Test()
vereq(t.test(), "var")
vereq(t.method_and_var(), "method")
vereq(t.actual_global(), "global")

print "10. recursion"

def f(x):
    def fact(n):
        if n == 0:
            return 1
        else:
            return n * fact(n - 1)
    if x >= 0:
        return fact(x)
    else:
        raise ValueError, "x must be >= 0"

vereq(f(6), 720)


print "11. unoptimized namespaces"

check_syntax("""\
def unoptimized_clash1(strip):
    def f(s):
        from string import *
        return strip(s) # ambiguity: free or local
    return f
""")

check_syntax("""\
def unoptimized_clash2():
    from string import *
    def f(s):
        return strip(s) # ambiguity: global or local
    return f
""")

check_syntax("""\
def unoptimized_clash2():
    from string import *
    def g():
        def f(s):
            return strip(s) # ambiguity: global or local
        return f
""")

# XXX could allow this for exec with const argument, but what's the point
check_syntax("""\
def error(y):
    exec "a = 1"
    def f(x):
        return x + y
    return f
""")

check_syntax("""\
def f(x):
    def g():
        return x
    del x # can't del name
""")

check_syntax("""\
def f():
    def g():
         from string import *
         return strip # global or local?
""")

# and verify a few cases that should work

exec """
def noproblem1():
    from string import *
    f = lambda x:x

def noproblem2():
    from string import *
    def f(x):
        return x + 1

def noproblem3():
    from string import *
    def f(x):
        global y
        y = x
"""

print "12. lambdas"

f1 = lambda x: lambda y: x + y
inc = f1(1)
plus10 = f1(10)
vereq(inc(1), 2)
vereq(plus10(5), 15)

f2 = lambda x: (lambda : lambda y: x + y)()
inc = f2(1)
plus10 = f2(10)
vereq(inc(1), 2)
vereq(plus10(5), 15)

f3 = lambda x: lambda y: global_x + y
global_x = 1
inc = f3(None)
vereq(inc(2), 3)

f8 = lambda x, y, z: lambda a, b, c: lambda : z * (b + y)
g = f8(1, 2, 3)
h = g(2, 4, 6)
vereq(h(), 18)

print "13. UnboundLocal"

def errorInOuter():
    print y
    def inner():
        return y
    y = 1

def errorInInner():
    def inner():
        return y
    inner()
    y = 1

try:
    errorInOuter()
except UnboundLocalError:
    pass
else:
    raise TestFailed

try:
    errorInInner()
except NameError:
    pass
else:
    raise TestFailed

# test for bug #1501934: incorrect LOAD/STORE_GLOBAL generation
global_x = 1
def f():
    global_x += 1
try:
    f()
except UnboundLocalError:
    pass
else:
    raise TestFailed, 'scope of global_x not correctly determined'

print "14. complex definitions"

def makeReturner(*lst):
    def returner():
        return lst
    return returner

vereq(makeReturner(1,2,3)(), (1,2,3))

def makeReturner2(**kwargs):
    def returner():
        return kwargs
    return returner

vereq(makeReturner2(a=11)()['a'], 11)

def makeAddPair((a, b)):
    def addPair((c, d)):
        return (a + c, b + d)
    return addPair

vereq(makeAddPair((1, 2))((100, 200)), (101,202))

print "15. scope of global statements"
# Examples posted by Samuele Pedroni to python-dev on 3/1/2001

# I
x = 7
def f():
    x = 1
    def g():
        global x
        def i():
            def h():
                return x
            return h()
        return i()
    return g()
vereq(f(), 7)
vereq(x, 7)

# II
x = 7
def f():
    x = 1
    def g():
        x = 2
        def i():
            def h():
                return x
            return h()
        return i()
    return g()
vereq(f(), 2)
vereq(x, 7)

# III
x = 7
def f():
    x = 1
    def g():
        global x
        x = 2
        def i():
            def h():
                return x
            return h()
        return i()
    return g()
vereq(f(), 2)
vereq(x, 2)

# IV
x = 7
def f():
    x = 3
    def g():
        global x
        x = 2
        def i():
            def h():
                return x
            return h()
        return i()
    return g()
vereq(f(), 2)
vereq(x, 2)

# XXX what about global statements in class blocks?
# do they affect methods?

x = 12
class Global:
    global x
    x = 13
    def set(self, val):
        x = val
    def get(self):
        return x

g = Global()
vereq(g.get(), 13)
g.set(15)
vereq(g.get(), 13)

print "16. check leaks"

class Foo:
    count = 0

    def __init__(self):
        Foo.count += 1

    def __del__(self):
        Foo.count -= 1

def f1():
    x = Foo()
    def f2():
        return x
    f2()

for i in range(100):
    f1()

vereq(Foo.count, 0)

print "17. class and global"

def test(x):
    class Foo:
        global x
        def __call__(self, y):
            return x + y
    return Foo()

x = 0
vereq(test(6)(2), 8)
x = -1
vereq(test(3)(2), 5)

looked_up_by_load_name = False
class X:
    # Implicit globals inside classes are be looked up by LOAD_NAME, not
    # LOAD_GLOBAL.
    locals()['looked_up_by_load_name'] = True
    passed = looked_up_by_load_name

verify(X.passed)

print "18. verify that locals() works"

def f(x):
    def g(y):
        def h(z):
            return y + z
        w = x + y
        y += 3
        return locals()
    return g

d = f(2)(4)
verify(d.has_key('h'))
del d['h']
vereq(d, {'x': 2, 'y': 7, 'w': 6})

print "19. var is bound and free in class"

def f(x):
    class C:
        def m(self):
            return x
        a = x
    return C

inst = f(3)()
vereq(inst.a, inst.m())

print "20. interaction with trace function"

import sys
def tracer(a,b,c):
    return tracer

def adaptgetter(name, klass, getter):
    kind, des = getter
    if kind == 1:       # AV happens when stepping from this line to next
        if des == "":
            des = "_%s__%s" % (klass.__name__, name)
        return lambda obj: getattr(obj, des)

class TestClass:
    pass

sys.settrace(tracer)
adaptgetter("foo", TestClass, (1, ""))
sys.settrace(None)

try: sys.settrace()
except TypeError: pass
else: raise TestFailed, 'sys.settrace() did not raise TypeError'

print "20. eval and exec with free variables"

def f(x):
    return lambda: x + 1

g = f(3)
try:
    eval(g.func_code)
except TypeError:
    pass
else:
    print "eval() should have failed, because code contained free vars"

try:
    exec g.func_code
except TypeError:
    pass
else:
    print "exec should have failed, because code contained free vars"

print "21. list comprehension with local variables"

try:
    print bad
except NameError:
    pass
else:
    print "bad should not be defined"

def x():
    [bad for s in 'a b' for bad in s.split()]

x()
try:
    print bad
except NameError:
    pass

print "22. eval with free variables"

def f(x):
    def g():
        x
        eval("x + 1")
    return g

f(4)()
