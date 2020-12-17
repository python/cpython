from decimal import Decimal
from math import fabs
from typing import Any, Union, overload

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.matcher import Matcher

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"

Number = Union[float, Decimal]  # Argh, https://github.com/python/mypy/issues/3186


def isnumeric(value: Any) -> bool:
    """Confirm that 'value' can be treated numerically; duck-test accordingly
    """
    if isinstance(value, (float, complex, int)):
        return True

    try:
        _ = (fabs(value) + 0 - 0) * 1
        return True
    except ArithmeticError:
        return True
    except:
        return False


class IsCloseTo(BaseMatcher[Number]):
    def __init__(self, value: Number, delta: Number) -> None:
        if not isnumeric(value):
            raise TypeError("IsCloseTo value must be numeric")
        if not isnumeric(delta):
            raise TypeError("IsCloseTo delta must be numeric")

        self.value = value
        self.delta = delta

    def _matches(self, item: Number) -> bool:
        if not isnumeric(item):
            return False
        return self._diff(item) <= self.delta

    def _diff(self, item: Number) -> float:
        # TODO - Fails for mixed floats & Decimals
        return fabs(item - self.value)  # type: ignore

    def describe_mismatch(self, item: Number, mismatch_description: Description) -> None:
        if not isnumeric(item):
            super(IsCloseTo, self).describe_mismatch(item, mismatch_description)
        else:
            actual_delta = self._diff(item)
            mismatch_description.append_description_of(item).append_text(
                " differed by "
            ).append_description_of(actual_delta)

    def describe_to(self, description: Description) -> None:
        description.append_text("a numeric value within ").append_description_of(
            self.delta
        ).append_text(" of ").append_description_of(self.value)


@overload
def close_to(value: float, delta: float) -> Matcher[float]:
    ...


@overload
def close_to(value: Decimal, delta: Decimal) -> Matcher[Decimal]:
    ...


def close_to(value, delta):
    """Matches if object is a number close to a given value, within a given
    delta.

    :param value: The value to compare against as the expected value.
    :param delta: The maximum delta between the values for which the numbers
        are considered close.

    This matcher compares the evaluated object against ``value`` to see if the
    difference is within a positive ``delta``.

    Example::

        close_to(3.0, 0.25)

    """
    return IsCloseTo(value, delta)
