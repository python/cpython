from typing import TypeVar, Union

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.helpers.wrap_matcher import wrap_matcher
from hamcrest.core.matcher import Matcher

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"

T = TypeVar("T")


class AnyOf(BaseMatcher[T]):
    def __init__(self, *matchers: Matcher[T]) -> None:
        self.matchers = matchers

    def _matches(self, item: T) -> bool:
        for matcher in self.matchers:
            if matcher.matches(item):
                return True
        return False

    def describe_to(self, description: Description) -> None:
        description.append_list("(", " or ", ")", self.matchers)


def any_of(*items: Union[Matcher[T], T]) -> Matcher[T]:
    """Matches if any of the given matchers evaluate to ``True``.

    :param matcher1,...:  A comma-separated list of matchers.

    The matchers are evaluated from left to right using short-circuit
    evaluation, so evaluation stops as soon as a matcher returns ``True``.

    Any argument that is not a matcher is implicitly wrapped in an
    :py:func:`~hamcrest.core.core.isequal.equal_to` matcher to check for
    equality.

    """
    return AnyOf(*[wrap_matcher(item) for item in items])
