# This may be loaded as a module, in the current __main__ module,
# or in another __main__ module.


#######################################
# functions

def spam_minimal():
    # no arg defaults or kwarg defaults
    # no annotations
    # no local vars
    # no free vars
    # no globals
    # no builtins
    # no attr access (names)
    # no code
    return


def spam_full(a, b, /, c, d:int=1, *args, e, f:object=None, **kwargs) -> tuple:
    # arg defaults, kwarg defaults
    # annotations
    # all kinds of local vars, except cells
    # no free vars
    # some globals
    # some builtins
    # some attr access (names)
    x = args
    y = kwargs
    z = (a, b, c, d)
    kwargs['e'] = e
    kwargs['f'] = f
    extras = list((x, y, z, spam, spam.__name__))
    return tuple(a, b, c, d, e, f, args, kwargs), extras


def spam(x):
    return x, None


def spam_N(x):
    def eggs_nested(y):
        return None, y
    return eggs_nested, x


def spam_C(x):
    a = 1
    def eggs_closure(y):
        return None, y, a, x
    return eggs_closure, a, x


def spam_NN(x):
    def eggs_nested_N(y):
        def ham_nested(z):
            return None, z
        return ham_nested, y
    return eggs_nested_N, x


def spam_NC(x):
    a = 1
    def eggs_nested_C(y):
        def ham_closure(z):
            return None, z, y, a, x
        return ham_closure, y
    return eggs_nested_C, a, x


def spam_CN(x):
    a = 1
    def eggs_closure_N(y):
        def ham_C_nested(z):
            return None, z
        return ham_C_nested, y, a, x
    return eggs_closure_N, a, x


def spam_CC(x):
    a = 1
    def eggs_closure_C(y):
        b = 2
        def ham_C_closure(z):
            return None, z, b, y, a, x
        return ham_C_closure, b, y, a, x
    return eggs_closure_N, a, x


eggs_nested, *_ = spam_N(1)
eggs_closure, *_ = spam_C(1)
eggs_nested_N, *_ = spam_NN(1)
eggs_nested_C, *_ = spam_NC(1)
eggs_closure_N, *_ = spam_CN(1)
eggs_closure_C, *_ = spam_CC(1)

ham_nested, *_ = eggs_nested_N(2)
ham_closure, *_ = eggs_nested_C(2)
ham_C_nested, *_ = eggs_closure_N(2)
ham_C_closure, *_ = eggs_closure_C(2)


TOP_FUNCTIONS = [
    # shallow
    spam_minimal,
    spam_full,
    spam,
    # outer func
    spam_N,
    spam_C,
    spam_NN,
    spam_NC,
    spam_CN,
    spam_CC,
]
NESTED_FUNCTIONS = [
    # inner func
    eggs_nested,
    eggs_closure,
    eggs_nested_N,
    eggs_nested_C,
    eggs_closure_N,
    eggs_closure_C,
    # inner inner func
    ham_nested,
    ham_closure,
    ham_C_nested,
    ham_C_closure,
]
FUNCTIONS = [
    *TOP_FUNCTIONS,
    *NESTED_FUNCTIONS,
]


# XXX set dishonest __file__, __module__, __name__


#######################################
# function-like

# generators

def gen_spam_1(*args):
     for arg in args:
         yield arg


def gen_spam_2(*args):
    yield from args


async def async_spam():
    pass
coro_spam = async_spam()
coro_spam.close()


async def asyncgen_spam(*args):
    for arg in args:
        yield arg
asynccoro_spam = asyncgen_spam(1, 2, 3)


FUNCTION_LIKE = [
    gen_spam_1,
    gen_spam_2,
    async_spam,
    coro_spam,  # actually FunctionType?
    asyncgen_spam,
    asynccoro_spam,  # actually FunctionType?
]


#######################################
# classes

class Spam:
    # minimal
    pass


class SpamOkay:
    def okay(self):
        return True


class SpamFull:

    a: object
    b: object
    c: object

    @staticmethod
    def staticmeth(cls):
        return True

    @classmethod
    def classmeth(cls):
        return True

    def __new__(cls, *args, **kwargs):
        return super().__new__(cls)

    def __init__(self, a, b, c):
        self.a = a
        self.b = b
        self.c = c

    # __repr__
    # __str__
    # ...

    def __eq__(self, other):
        if not isinstance(other, SpamFull):
            return NotImplemented
        return (self.a == other.a and
                self.b == other.b and
                self.c == other.c)

    @property
    def prop(self):
        return True


class SubSpamFull(SpamFull):
    ...


class SubTuple(tuple):
    ...


def class_eggs_inner():
    class EggsNested:
        ...
    return EggsNested
EggsNested = class_eggs_inner()


TOP_CLASSES = {
    Spam: (),
    SpamOkay: (),
    SpamFull: (1, 2, 3),
    SubSpamFull: (1, 2, 3),
    SubTuple: ([1, 2, 3],),
}
CLASSES_WITHOUT_EQUALITY = [
    Spam,
    SpamOkay,
]
BUILTIN_SUBCLASSES = [
    SubTuple,
]
NESTED_CLASSES = {
    EggsNested: (),
}
CLASSES = {
    **TOP_CLASSES,
    **NESTED_CLASSES,
}


#######################################
# exceptions

class MimimalError(Exception):
    pass


class RichError(Exception):
    def __init__(self, msg, value=None):
        super().__init__(msg, value)
        self.msg = msg
        self.value = value

    def __eq__(self, other):
        if not isinstance(other, RichError):
            return NotImplemented
        if self.msg != other.msg:
            return False
        if self.value != other.value:
            return False
        return True
