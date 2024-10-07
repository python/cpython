
class SLICER(object):
    def __getitem__(self, arg):
        return arg

SLICE = SLICER()

class SHIFTER(object):
    def shift(self, arg):
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

    def __getitem__(self, arg):
        if isinstance(arg, tuple):
            return tuple(self.shift(x) for x in arg)
        else:
            return self.shift(arg)

SHIFT = SHIFTER()

class CLOSER(object):
    def __getitem__(self, arg):
        if isinstance(arg, slice):
            start = arg.start
            stop = arg.stop
            step = arg.step
            if isinstance(stop, int):
                if step is None:
                    stop += 1
                elif isinstance(step, int):
                    stop += step
            return slice(start, stop, step)
        else:
            return arg

CLOSE = CLOSER()
