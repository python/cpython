# line 1
def wrap(foo=None):
    def wrapper(func):
        return func
    return wrapper

# line 7
def replace(func):
    def insteadfunc():
        print 'hello'
    return insteadfunc

# line 13
@wrap()
@wrap(wrap)
def wrapped():
    pass

# line 19
@replace
def gone():
    pass

# line 24
oll = lambda m: m

# line 27
tll = lambda g: g and \
g and \
g

# line 32
tlli = lambda d: d and \
    d

# line 36
def onelinefunc(): pass

# line 39
def manyargs(arg1, arg2,
arg3, arg4): pass

# line 43
def twolinefunc(m): return m and \
m

# line 47
a = [None,
     lambda x: x,
     None]

# line 52
def setfunc(func):
    globals()["anonymous"] = func
setfunc(lambda x, y: x*y)
