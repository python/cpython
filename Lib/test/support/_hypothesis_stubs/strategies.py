import functools

from ._helpers import StubClass, stub_factory


class StubStrategy(StubClass):
    def map(self, pack):
        return self

    def flatmap(self, expand):
        return self

    def filter(self, condition):
        return self

    def __or__(self, other):
        return self


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
    "functions" "integers",
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
