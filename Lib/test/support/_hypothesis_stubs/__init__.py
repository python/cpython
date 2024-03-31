from enum import Enum
import functools
import unittest

__all__ = [
    "given",
    "example",
    "assume",
    "reject",
    "register_random",
    "strategies",
    "HealthCheck",
    "settings",
    "Verbosity",
]

from . import strategies


def given(*_args, **_kwargs):
    def decorator(f):
        if examples := getattr(f, "_examples", []):

            @functools.wraps(f)
            def test_function(self):
                for example_args, example_kwargs in examples:
                    with self.subTest(*example_args, **example_kwargs):
                        f(self, *example_args, **example_kwargs)

        else:
            # If we have found no examples, we must skip the test. If @example
            # is applied after @given, it will re-wrap the test to remove the
            # skip decorator.
            test_function = unittest.skip(
                "Hypothesis required for property test with no " +
                "specified examples"
            )(f)

        test_function._given = True
        return test_function

    return decorator


def example(*args, **kwargs):
    if bool(args) == bool(kwargs):
        raise ValueError("Must specify exactly one of *args or **kwargs")

    def decorator(f):
        base_func = getattr(f, "__wrapped__", f)
        if not hasattr(base_func, "_examples"):
            base_func._examples = []

        base_func._examples.append((args, kwargs))

        if getattr(f, "_given", False):
            # If the given decorator is below all the example decorators,
            # it would be erroneously skipped, so we need to re-wrap the new
            # base function.
            f = given()(base_func)

        return f

    return decorator


def assume(condition):
    if not condition:
        raise unittest.SkipTest("Unsatisfied assumption")
    return True


def reject():
    assume(False)


def register_random(*args, **kwargs):
    pass  # pragma: no cover


def settings(*args, **kwargs):
    return lambda f: f  # pragma: nocover


class HealthCheck(Enum):
    data_too_large = 1
    filter_too_much = 2
    too_slow = 3
    return_value = 5
    large_base_example = 7
    not_a_test_method = 8

    @classmethod
    def all(cls):
        return list(cls)


class Verbosity(Enum):
    quiet = 0
    normal = 1
    verbose = 2
    debug = 3


class Phase(Enum):
    explicit = 0
    reuse = 1
    generate = 2
    target = 3
    shrink = 4
    explain = 5
