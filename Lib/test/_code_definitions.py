
def simple_script():
    assert True


def complex_script():
    obj = 'a string'
    pickle = __import__('pickle')
    def spam_minimal():
        pass
    spam_minimal()
    data = pickle.dumps(obj)
    res = pickle.loads(data)
    assert res == obj, (res, obj)


def script_with_globals():
    obj1, obj2 = spam(42)
    assert obj1 == 42
    assert obj2 is None


def script_with_explicit_empty_return():
    return None


def script_with_return():
    return True


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


def spam_with_builtins():
    x = 42
    values = (42,)
    checks = tuple(callable(v) for v in values)
    res = callable(values), tuple(values), list(values), checks
    print(res)


def spam_with_globals_and_builtins():
    func1 = spam
    func2 = spam_minimal
    funcs = (func1, func2)
    checks = tuple(callable(f) for f in funcs)
    res = callable(funcs), tuple(funcs), list(funcs), checks
    print(res)


def spam_args_attrs_and_builtins(a, b, /, c, d, *args, e, f, **kwargs):
    if args.__len__() > 2:
        return None
    return a, b, c, d, e, f, args, kwargs


def spam_returns_arg(x):
    return x


def spam_with_inner_not_closure():
    def eggs():
        pass
    eggs()


def spam_with_inner_closure():
    x = 42
    def eggs():
        print(x)
    eggs()


def spam_annotated(a: int, b: str, c: object) -> tuple:
    return a, b, c


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
    return eggs_closure_C, a, x


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
    simple_script,
    complex_script,
    script_with_globals,
    script_with_explicit_empty_return,
    script_with_return,
    spam_minimal,
    spam_with_builtins,
    spam_with_globals_and_builtins,
    spam_args_attrs_and_builtins,
    spam_returns_arg,
    spam_with_inner_not_closure,
    spam_with_inner_closure,
    spam_annotated,
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

STATELESS_FUNCTIONS = [
    simple_script,
    complex_script,
    script_with_explicit_empty_return,
    script_with_return,
    spam,
    spam_minimal,
    spam_with_builtins,
    spam_args_attrs_and_builtins,
    spam_returns_arg,
    spam_annotated,
    spam_with_inner_not_closure,
    spam_with_inner_closure,
    spam_N,
    spam_C,
    spam_NN,
    spam_NC,
    spam_CN,
    spam_CC,
    eggs_nested,
    eggs_nested_N,
    ham_nested,
    ham_C_nested
]
STATELESS_CODE = [
    *STATELESS_FUNCTIONS,
    script_with_globals,
    spam_with_globals_and_builtins,
    spam_full,
]

PURE_SCRIPT_FUNCTIONS = [
    simple_script,
    complex_script,
    script_with_explicit_empty_return,
    spam_minimal,
    spam_with_builtins,
    spam_with_inner_not_closure,
    spam_with_inner_closure,
]
SCRIPT_FUNCTIONS = [
    *PURE_SCRIPT_FUNCTIONS,
    script_with_globals,
    spam_with_globals_and_builtins,
]


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
    asyncgen_spam,
]
FUNCTION_LIKE_APPLIED = [
    coro_spam,  # actually FunctionType?
    asynccoro_spam,  # actually FunctionType?
]
