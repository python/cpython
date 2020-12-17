import operator
from typing import Any, Callable

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.matcher import Matcher

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"


class OrderingComparison(BaseMatcher[Any]):
    def __init__(
        self,
        value: Any,
        comparison_function: Callable[[Any, Any], bool],
        comparison_description: str,
    ) -> None:
        self.value = value
        self.comparison_function = comparison_function
        self.comparison_description = comparison_description

    def _matches(self, item: Any) -> bool:
        return self.comparison_function(item, self.value)

    def describe_to(self, description: Description) -> None:
        description.append_text("a value ").append_text(self.comparison_description).append_text(
            " "
        ).append_description_of(self.value)


def greater_than(value: Any) -> Matcher[Any]:
    """Matches if object is greater than a given value.

    :param value: The value to compare against.

    """
    return OrderingComparison(value, operator.gt, "greater than")


def greater_than_or_equal_to(value: Any) -> Matcher[Any]:
    """Matches if object is greater than or equal to a given value.

    :param value: The value to compare against.

    """
    return OrderingComparison(value, operator.ge, "greater than or equal to")


def less_than(value: Any) -> Matcher[Any]:
    """Matches if object is less than a given value.

    :param value: The value to compare against.

    """
    return OrderingComparison(value, operator.lt, "less than")


def less_than_or_equal_to(value: Any) -> Matcher[Any]:
    """Matches if object is less than or equal to a given value.

    :param value: The value to compare against.

    """
    return OrderingComparison(value, operator.le, "less than or equal to")
