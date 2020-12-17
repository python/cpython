from typing import TypeVar

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.matcher import Matcher

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"

T = TypeVar("T")


class IsSame(BaseMatcher[T]):
    def __init__(self, obj: T) -> None:
        self.object = obj

    def _matches(self, item: T) -> bool:
        return item is self.object

    def describe_to(self, description: Description) -> None:
        description.append_text("same instance as ").append_text(hex(id(self.object))).append_text(
            " "
        ).append_description_of(self.object)

    def describe_mismatch(self, item: T, mismatch_description: Description) -> None:
        mismatch_description.append_text("was ")
        if item is not None:
            mismatch_description.append_text(hex(id(item))).append_text(" ")
        mismatch_description.append_description_of(item)


def same_instance(obj: T) -> Matcher[T]:
    """Matches if evaluated object is the same instance as a given object.

    :param obj: The object to compare against as the expected value.

    This matcher invokes the ``is`` identity operator to determine if the
    evaluated object is the the same object as ``obj``.

    """
    return IsSame(obj)
