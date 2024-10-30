"""
index01 --- an experiment to mix 0-based and 1-based indices

Defines several objects used in the modified Python grammar
in order to accept the a{i} notation and closed slices.
These objects are integrated into builtins,
so they are directly accessible (no need for qualified names).
"""

import builtins

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

builtins.CLOSED0 =  CLOSED0 = CLOSED0CONVERTER()

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

builtins.FROM1 = FROM1 = FROM1CONVERTER()

class STAR1CONVERTER(object):
    """
    Class of STAR1, and STAR1[args] applies FROM1 on each arg
    Used in a transformation object{*args} -> object[*STAR1(args)]
    So, args can be any iterable
    """
    def __getitem__(self, arg):
        return tuple(FROM1[x] for x in arg)

builtins.STAR1 = STAR1 = STAR1CONVERTER()

########################################
