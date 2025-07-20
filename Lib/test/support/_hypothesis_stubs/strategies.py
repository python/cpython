import functools

from ._helpers import StubClass, stub_factory


class StubStrategy(StubClass):
    def __make_trailing_repr(self, transformation_name, func):
        func_name = func.__name__ or repr(func)
        return f"{self!r}.{transformation_name}({func_name})"

    def map(self, pack):
        return self._with_repr(self.__make_trailing_repr("map", pack))

    def flatmap(self, expand):
        return self._with_repr(self.__make_trailing_repr("flatmap", expand))

    def filter(self, condition):
        return self._with_repr(self.__make_trailing_repr("filter", condition))

    def __or__(self, other):
        new_repr = f"one_of({self!r}, {other!r})"
        return self._with_repr(new_repr)


_STRATEGIES = {
    "binary",
    "booleans",
    "builds",
    "characters",
    "complex_numbers",
    "composite",
    "data",
    "dates",
    "datetimes",
    "decimals",
    "deferred",
    "dictionaries",
    "emails",
    "fixed_dictionaries",
    "floats",
    "fractions",
    "from_regex",
    "from_type",
    "frozensets",
    "functions",
    "integers",
    "iterables",
    "just",
    "lists",
    "none",
    "nothing",
    "one_of",
    "permutations",
    "random_module",
    "randoms",
    "recursive",
    "register_type_strategy",
    "runner",
    "sampled_from",
    "sets",
    "shared",
    "slices",
    "timedeltas",
    "times",
    "text",
    "tuples",
    "uuids",
}

__all__ = sorted(_STRATEGIES)


def composite(f):
    strategy = stub_factory(StubStrategy, f.__name__)

    @functools.wraps(f)
    def inner(*args, **kwargs):
        return strategy(*args, **kwargs)

    return inner


def __getattr__(name):
    if name not in _STRATEGIES:
        raise AttributeError(f"Unknown attribute {name}")

    return stub_factory(StubStrategy, f"hypothesis.strategies.{name}")


def __dir__():
    return __all__
