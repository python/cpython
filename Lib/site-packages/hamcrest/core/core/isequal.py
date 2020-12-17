from typing import Any

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.matcher import Matcher

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"


class IsEqual(BaseMatcher[Any]):
    def __init__(self, equals: Any) -> None:
        self.object = equals

    def _matches(self, item: Any) -> bool:
        return item == self.object

    def describe_to(self, description: Description) -> None:
        nested_matcher = isinstance(self.object, Matcher)
        if nested_matcher:
            description.append_text("<")
        description.append_description_of(self.object)
        if nested_matcher:
            description.append_text(">")


def equal_to(obj: Any) -> Matcher[Any]:
    """Matches if object is equal to a given object.

    :param obj: The object to compare against as the expected value.

    This matcher compares the evaluated object to ``obj`` for equality."""
    return IsEqual(obj)
