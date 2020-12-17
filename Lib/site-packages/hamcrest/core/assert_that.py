import warnings
from typing import Optional, TypeVar, cast, overload

from hamcrest.core.matcher import Matcher
from hamcrest.core.string_description import StringDescription

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"
# unittest integration; hide these frames from tracebacks
__unittest = True
# py.test integration; hide these frames from tracebacks
__tracebackhide__ = True

T = TypeVar("T")


@overload
def assert_that(actual: T, matcher: Matcher[T], reason: str = "") -> None:
    ...


@overload
def assert_that(assertion: bool, reason: str = "") -> None:
    ...


def assert_that(actual, matcher=None, reason=""):
    """Asserts that actual value satisfies matcher. (Can also assert plain
    boolean condition.)

    :param actual: The object to evaluate as the actual value.
    :param matcher: The matcher to satisfy as the expected condition.
    :param reason: Optional explanation to include in failure description.

    ``assert_that`` passes the actual value to the matcher for evaluation. If
    the matcher is not satisfied, an exception is thrown describing the
    mismatch.

    ``assert_that`` is designed to integrate well with PyUnit and other unit
    testing frameworks. The exception raised for an unmet assertion is an
    :py:exc:`AssertionError`, which PyUnit reports as a test failure.

    With a different set of parameters, ``assert_that`` can also verify a
    boolean condition:

    .. function:: assert_that(assertion[, reason])

    :param assertion:  Boolean condition to verify.
    :param reason:  Optional explanation to include in failure description.

    This is equivalent to the :py:meth:`~unittest.TestCase.assertTrue` method
    of :py:class:`unittest.TestCase`, but offers greater flexibility in test
    writing by being a standalone function.

    """
    if isinstance(matcher, Matcher):
        _assert_match(actual=actual, matcher=matcher, reason=reason)
    else:
        if isinstance(actual, Matcher):
            warnings.warn("arg1 should be boolean, but was {}".format(type(actual)))
        _assert_bool(assertion=cast(bool, actual), reason=cast(str, matcher))


def _assert_match(actual: T, matcher: Matcher[T], reason: str) -> None:
    if not matcher.matches(actual):
        description = StringDescription()
        description.append_text(reason).append_text("\nExpected: ").append_description_of(
            matcher
        ).append_text("\n     but: ")
        matcher.describe_mismatch(actual, description)
        description.append_text("\n")
        raise AssertionError(description)


def _assert_bool(assertion: bool, reason: Optional[str] = None) -> None:
    if not assertion:
        if not reason:
            reason = "Assertion failed"
        raise AssertionError(reason)
