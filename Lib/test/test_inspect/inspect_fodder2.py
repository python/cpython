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

# line 276
parenthesized_lambda = (
    lambda: ())
parenthesized_lambda2 = [
    lambda: ()][0]
parenthesized_lambda3 = {0:
    lambda: ()}[0]

# line 285
post_line_parenthesized_lambda1 = (lambda: ()
)

# line 289
nested_lambda = (
    lambda right: [].map(
        lambda length: ()))

# line 294
if True:
    class cls296:
        def f():
            pass
else:
    class cls296:
        def g():
            pass

# line 304
if False:
    class cls310:
        def f():
            pass
else:
    class cls310:
        def g():
            pass

# line 314
class ClassWithCodeObject:
    import sys
    code = sys._getframe(0).f_code

import enum

# line 321
class enum322(enum.Enum):
    A = 'a'

# line 325
class enum326(enum.IntEnum):
    A = 1

# line 329
class flag330(enum.Flag):
    A = 1

# line 333
class flag334(enum.IntFlag):
    A = 1

# line 337
simple_enum338 = enum.Enum('simple_enum338', 'A')
simple_enum339 = enum.IntEnum('simple_enum339', 'A')
simple_flag340 = enum.Flag('simple_flag340', 'A')
simple_flag341 = enum.IntFlag('simple_flag341', 'A')

import typing

# line 345
class nt346(typing.NamedTuple):
    x: int
    y: int

# line 350
nt351 = typing.NamedTuple('nt351', (('x', int), ('y', int)))

# line 353
class td354(typing.TypedDict):
    x: int
    y: int

# line 358
td359 = typing.TypedDict('td359', (('x', int), ('y', int)))

import dataclasses

# line 363
@dataclasses.dataclass
class dc364:
    x: int
    y: int

# line 369
dc370 = dataclasses.make_dataclass('dc370', (('x', int), ('y', int)))
dc371 = dataclasses.make_dataclass('dc370', (('x', int), ('y', int)), module=__name__)

import inspect
import itertools

# line 376
ge377 = (
    inspect.currentframe()
    for i in itertools.count()
)

# line 382
def func383():
    # line 384
    ge385 = (
        inspect.currentframe()
        for i in itertools.count()
    )
    return ge385

# line 391
@decorator
# comment
def func394():
    return 395

# line 397
@decorator

def func400():
    return 401

pass # end of file
