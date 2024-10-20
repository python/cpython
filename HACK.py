
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
    class of FROM1, anf FROM1[slice] converts
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
        elif isinstance(arg, tuple):
            return tuple(self[x] for x in arg) # useful or not ?
        else:
            return arg

FROM1 = FROM1CONVERTER()

def tests(*args):
    import ast
    for code in args:
        print(code, '->', end=' ')
        try:
            print(ast.unparse(ast.parse(code)))
        except Exception as e:
            print(e.__class__.__name__)

def tests1():
    tests(
        "a[i]",
        "a[i:j]",
        "a[i:j:k]",
        "a[i:j::k]",
        "a{i}",
        "a{i:j}",
        "a{i:j:k}",
        "a{i:j::k}",
    )

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
