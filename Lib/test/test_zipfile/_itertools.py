import itertools


# from jaraco.itertools 6.3.0
class Counter:
    """
    Wrap an iterable in an object that stores the count of items
    that pass through it.

    >>> items = Counter(range(20))
    >>> items.count
    0
    >>> values = list(items)
    >>> items.count
    20
    """

    def __init__(self, i):
        self.count = 0
        self.iter = zip(itertools.count(1), i)

    def __iter__(self):
        return self

    def __next__(self):
        self.count, result = next(self.iter)
        return result


# from more_itertools v8.13.0
def always_iterable(obj, base_type=(str, bytes)):
    if obj is None:
        return iter(())

    if (base_type is not None) and isinstance(obj, base_type):
        return iter((obj,))

    try:
        return iter(obj)
    except TypeError:
        return iter((obj,))
