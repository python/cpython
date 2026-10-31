from dataclasses import dataclass, replace
from enum import Enum
import functools
import unittest

__all__ = [
    "given",
    "example",
    "assume",
    "reject",
    "register_random",
    "stateful",
    "strategies",
    "HealthCheck",
    "settings",
    "Verbosity",
]

from . import stateful, strategies


@dataclass(frozen=True)
class _Example:
    args: tuple
    kwargs: dict
    # Set by example.xfail(): the exception types the example must raise.
    raises: type[BaseException] | tuple[type[BaseException], ...] | None = None
    reason: str = ""

    def __repr__(self):
        bits = [repr(a) for a in self.args]
        bits += [f"{k}={v!r}" for k, v in self.kwargs.items()]
        return f"@example({', '.join(bits)})"


def _run_example(f, self, ex):
    if ex.raises is None:
        f(self, *ex.args, **ex.kwargs)
        return
    try:
        f(self, *ex.args, **ex.kwargs)
    except Exception as exc:
        if not isinstance(exc, ex.raises):
            raise
        return
    reason = f" because {ex.reason}" if ex.reason else ""
    raise AssertionError(
        f"Expected an exception from {ex!r}{reason}, "
        "but no exception was raised"
    )


def given(*_args, **_kwargs):
    def decorator(f):
        if examples := getattr(f, "_examples", []):

            @functools.wraps(f)
            def test_function(self):
                for ex in examples:
                    if len(ex.args) < 2:
                        subtest_args = ex.args
                    else:
                        # subTest takes up to one positional argument.
                        # When there are more, display them as a tuple
                        subtest_args = [ex.args]
                    with self.subTest(*subtest_args, **ex.kwargs):
                        _run_example(f, self, ex)

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


class example:
    """Stub for ``hypothesis.example``.

    Upstream, ``@example(...)`` adds an explicit input that Hypothesis
    always runs before generating any, with the same positional-or-keyword
    rules as ``@given``.  Here the explicit examples are all that runs:
    ``given`` replays each one in a ``subTest``.
    """

    def __init__(self, *args, **kwargs):
        if bool(args) == bool(kwargs):
            raise ValueError("Must specify exactly one of *args or **kwargs")
        self._example = _Example(args, kwargs)

    def __call__(self, f):
        base_func = getattr(f, "__wrapped__", f)
        if not hasattr(base_func, "_examples"):
            base_func._examples = []

        base_func._examples.append(self._example)

        if getattr(f, "_given", False):
            # If the given decorator is below all the example decorators,
            # it would be erroneously skipped, so we need to re-wrap the new
            # base function.
            f = given()(base_func)

        return f

    def xfail(self, condition=True, *, reason="", raises=BaseException):
        """Mark this example as an expected failure, as upstream does.

        Upstream, ``@example(...).xfail()`` means the example must raise
        one of the ``raises`` exceptions (any exception by default), like
        ``pytest.mark.xfail(strict=True)``: a different exception
        propagates, and an unexpected pass fails the test, with ``reason``
        in the message.  ``condition=False`` makes it an ordinary example.
        The stub replays it the same way.
        """
        if condition:
            self._example = replace(self._example, raises=raises,
                                    reason=reason)
        return self

    def via(self, whence, /):
        """Accept and ignore an example's provenance label.

        Upstream, ``@example(...).via("...")`` only records where the
        example came from, for tools that add or remove ``@example``
        decorators (HypoFuzz, for instance); it changes nothing at run
        time there either.
        """
        return self


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
