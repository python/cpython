
########################################

class SLICER(object):
    """
    class of SLICE, used to convert an expression in brackets / braces
    into a slice(...) or extension.
    """
    def __getitem__(self, arg):
        return arg

SLICE = SLICER()

########################################

class CLOSED0CONVERTER(object):
    """
    class of CLOSED0, and CLOSED0[slice] converts
    a 0-based closed slice into an equivalent 0-based semi-open slice.
    Results are wrong if slice is 1-based (but no way to check it).
    """
    def __getitem__(self, arg):
        assert isinstance(arg, slice)
        start = arg.start
        stop = arg.stop
        step = 1 if arg.step is None else arg.step
        if isinstance(stop, int) and isinstance(step, int):
            if step > 0:
                stop = None if stop == -1 else stop+1
            if step < 0:
                stop = None if stop == 0 else stop-1
        return slice(start, stop, arg.step)

CLOSED0 = CLOSED0CONVERTER()

class FROM1CONVERTER(object):
    """
    class of FROM1, and FROM1[slice] converts
    a 1-based semi-open slice into an equivalent 0-based semi-open slice.
    Also works for integers. Objects of other types are returned unchanged.
    """
    def __getitem__(self, arg):
        if isinstance(arg, int):
            return arg - 1
        elif isinstance(arg, slice):
            start = arg.start
            stop = arg.stop
            step = arg.step
            if isinstance(start, int): start -= 1
            if isinstance(stop, int): stop -= 1
            return slice(start, stop, step)
        else:
            return arg

FROM1 = FROM1CONVERTER()

class STAR1CONVERTER(object):
    """
    Class of STAR1, and STAR1[args] applies FROM1 on each arg
    Used in a transformation object{*args} -> object[*STAR1(args)]
    So, args can be any iterable
    """
    def __getitem__(self, arg):
        return tuple(FROM1[x] for x in arg)

STAR1 = STAR1CONVERTER()


def test(*args):
    import ast
    for code in args:
        print(code, '->', end=' ')
        try:
            print(ast.unparse(ast.parse(code)))
        except Exception as e:
            print(e.__class__.__name__)

def tests():
    brackets = [
        "a[i]",
        "a[i:]",
        "a[i:j]",
        "a[i:j:k]",
        "a[i::k]",
        "a[:j]",
        "a[:j:k]",
        "a[::k]",
        "a[:]",
        "a[::]",
        "a[i:j:]",
        "a[i:j:k:]",
        "a[:j:]",
        "a[:j:k:]",
    ]
    test(*brackets)
    print()
    braces = [code.replace("[", "{").replace("]", "}") for code in brackets]
    test(*braces)

########################################

import builtins

class RANGE(object):
    """
    Enables to write range[i:j:k] in addition to range(i, j, k).
    Main interest: also works with closed slices.
    """
    def __call__(self, *args):
        return builtins.range(*args)

    def __getitem__(self, arg):
        if isinstance(arg, slice):
            step = 1 if arg.step is None else arg.step
            return builtins.range(arg.start, arg.stop, step)
        else:
            raise ValueError("only range[slice] allowed")

range = RANGE()

########################################
