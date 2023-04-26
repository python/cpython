# line 1
def wrap(foo=None):
    def wrapper(func):
        return func
    return wrapper

# line 7
def replace(func):
    def insteadfunc():
        print('hello')
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

# line 57
def with_comment():  # hello
    world

# line 61
multiline_sig = [
    lambda x, \
            y: x+y,
    None,
    ]

# line 68
def func69():
    class cls70:
        def func71():
            pass
    return cls70
extra74 = 74

# line 76
def func77(): pass
(extra78, stuff78) = 'xy'
extra79 = 'stop'

# line 81
class cls82:
    def func83(): pass
(extra84, stuff84) = 'xy'
extra85 = 'stop'

# line 87
def func88():
    # comment
    return 90

# line 92
def f():
    class X:
        def g():
            "doc"
            return 42
    return X
method_in_dynamic_class = f().g

#line 101
def keyworded(*arg1, arg2=1):
    pass

#line 105
def annotated(arg1: list):
    pass

#line 109
def keyword_only_arg(*, arg):
    pass

@wrap(lambda: None)
def func114():
    return 115

class ClassWithMethod:
    def method(self):
        pass

from functools import wraps

def decorator(func):
    @wraps(func)
    def fake():
        return 42
    return fake

#line 129
@decorator
def real():
    return 20

#line 134
class cls135:
    def func136():
        def func137():
            never_reached1
            never_reached2

# line 141
class cls142:
    a = """
class cls149:
    ...
"""

# line 148
class cls149:

    def func151(self):
        pass

'''
class cls160:
    pass
'''

# line 159
class cls160:

    def func162(self):
        pass

# line 165
class cls166:
    a = '''
    class cls175:
        ...
    '''

# line 172
class cls173:

    class cls175:
        pass

# line 178
class cls179:
    pass

# line 182
class cls183:

    class cls185:

        def func186(self):
            pass

def class_decorator(cls):
    return cls

# line 193
@class_decorator
@class_decorator
class cls196:

    @class_decorator
    @class_decorator
    class cls200:
        pass

class cls203:
    class cls204:
        class cls205:
            pass
    class cls207:
        class cls205:
            pass

# line 211
def func212():
    class cls213:
        pass
    return cls213

# line 217
class cls213:
    def func219(self):
        class cls220:
            pass
        return cls220

# line 224
async def func225():
    class cls226:
        pass
    return cls226

# line 230
class cls226:
    async def func232(self):
        class cls233:
            pass
        return cls233

if True:
    class cls238:
        class cls239:
            '''if clause cls239'''
else:
    class cls238:
        class cls239:
            '''else clause 239'''
            pass

#line 247
def positional_only_arg(a, /):
    pass

#line 251
def all_markers(a, b, /, c, d, *, e, f):
    pass

# line 255
def all_markers_with_args_and_kwargs(a, b, /, c, d, *args, e, f, **kwargs):
    pass

#line 259
def all_markers_with_defaults(a, b=1, /, c=2, d=3, *, e=4, f=5):
    pass

# line 263
def deco_factory(**kwargs):
    def deco(f):
        @wraps(f)
        def wrapper(*a, **kwd):
            kwd.update(kwargs)
            return f(*a, **kwd)
        return wrapper
    return deco

@deco_factory(foo=(1 + 2), bar=lambda: 1)
def complex_decorated(foo=0, bar=lambda: 0):
    return foo + bar()
