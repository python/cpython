"""Example of a generator: re-implement the built-in range function
without actually constructing the list of values.

OldStyleRange is coded in the way required to work in a 'for' loop before
iterators were introduced into the language; using __getitem__ and __len__ .

"""
def handleargs(arglist):
    """Take list of arguments and extract/create proper start, stop, and step
    values and return in a tuple"""
    try:
        if len(arglist) == 1:
            return 0, int(arglist[0]), 1
        elif len(arglist) == 2:
            return int(arglist[0]), int(arglist[1]), 1
        elif len(arglist) == 3:
            if arglist[2] == 0:
                raise ValueError("step argument must not be zero")
            return tuple(int(x) for x in arglist)
        else:
            raise TypeError("range() accepts 1-3 arguments, given", len(arglist))
    except TypeError:
        raise TypeError("range() arguments must be numbers or strings "
        "representing numbers")

def genrange(*a):
    """Function to implement 'range' as a generator"""
    start, stop, step = handleargs(a)
    value = start
    while value < stop:
        yield value
        value += step

class oldrange:
    """Class implementing a range object.
    To the user the instances feel like immutable sequences
    (and you can't concatenate or slice them)

    Done using the old way (pre-iterators; __len__ and __getitem__) to have an
    object be used by a 'for' loop.

    """

    def __init__(self, *a):
        """ Initialize start, stop, and step values along with calculating the
        nubmer of values (what __len__ will return) in the range"""
        self.start, self.stop, self.step = handleargs(a)
        self.len = max(0, (self.stop - self.start) // self.step)

    def __repr__(self):
        """implement repr(x) which is also used by print"""
        return 'range(%r, %r, %r)' % (self.start, self.stop, self.step)

    def __len__(self):
        """implement len(x)"""
        return self.len

    def __getitem__(self, i):
        """implement x[i]"""
        if 0 <= i <= self.len:
            return self.start + self.step * i
        else:
            raise IndexError('range[i] index out of range')


def test():
    import time, __builtin__
    #Just a quick sanity check
    correct_result = __builtin__.range(5, 100, 3)
    oldrange_result = list(oldrange(5, 100, 3))
    genrange_result = list(genrange(5, 100, 3))
    if genrange_result != correct_result or oldrange_result != correct_result:
        raise Exception("error in implementation:\ncorrect   = %s"
                         "\nold-style = %s\ngenerator = %s" %
                         (correct_result, oldrange_result, genrange_result))
    print("Timings for range(1000):")
    t1 = time.time()
    for i in oldrange(1000):
        pass
    t2 = time.time()
    for i in genrange(1000):
        pass
    t3 = time.time()
    for i in __builtin__.range(1000):
        pass
    t4 = time.time()
    print(t2-t1, 'sec (old-style class)')
    print(t3-t2, 'sec (generator)')
    print(t4-t3, 'sec (built-in)')


if __name__ == '__main__':
    test()
