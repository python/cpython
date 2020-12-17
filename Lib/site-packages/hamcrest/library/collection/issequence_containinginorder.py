import warnings
from typing import Optional, Sequence, TypeVar, Union

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.helpers.wrap_matcher import wrap_matcher
from hamcrest.core.matcher import Matcher

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"

T = TypeVar("T")


class MatchingInOrder(object):
    def __init__(
        self, matchers: Sequence[Matcher[T]], mismatch_description: Optional[Description]
    ) -> None:
        self.matchers = matchers
        self.mismatch_description = mismatch_description
        self.next_match_index = 0

    def matches(self, item: T) -> bool:
        return self.isnotsurplus(item) and self.ismatched(item)

    def isfinished(self) -> bool:
        if self.next_match_index < len(self.matchers):
            if self.mismatch_description:
                self.mismatch_description.append_text("No item matched: ").append_description_of(
                    self.matchers[self.next_match_index]
                )
            return False
        return True

    def ismatched(self, item: T) -> bool:
        matcher = self.matchers[self.next_match_index]
        if not matcher.matches(item):
            if self.mismatch_description:
                self.mismatch_description.append_text("item " + str(self.next_match_index) + ": ")
                matcher.describe_mismatch(item, self.mismatch_description)
            return False
        self.next_match_index += 1
        return True

    def isnotsurplus(self, item: T) -> bool:
        if len(self.matchers) <= self.next_match_index:
            if self.mismatch_description:
                self.mismatch_description.append_text("Not matched: ").append_description_of(item)
            return False
        return True


class IsSequenceContainingInOrder(BaseMatcher[Sequence[T]]):
    def __init__(self, matchers: Sequence[Matcher[T]]) -> None:
        self.matchers = matchers

    def matches(
        self, item: Sequence[T], mismatch_description: Optional[Description] = None
    ) -> bool:
        try:
            matchsequence = MatchingInOrder(self.matchers, mismatch_description)
            for element in item:
                if not matchsequence.matches(element):
                    return False
            return matchsequence.isfinished()
        except TypeError:
            if mismatch_description:
                super(IsSequenceContainingInOrder, self).describe_mismatch(
                    item, mismatch_description
                )
            return False

    def describe_mismatch(self, item: Sequence[T], mismatch_description: Description) -> None:
        self.matches(item, mismatch_description)

    def describe_to(self, description: Description) -> None:
        description.append_text("a sequence containing ").append_list("[", ", ", "]", self.matchers)


def contains_exactly(*items: Union[Matcher[T], T]) -> Matcher[Sequence[T]]:
    """Matches if sequence's elements satisfy a given list of matchers, in order.

    :param match1,...: A comma-separated list of matchers.

    This matcher iterates the evaluated sequence and a given list of matchers,
    seeing if each element satisfies its corresponding matcher.

    Any argument that is not a matcher is implicitly wrapped in an
    :py:func:`~hamcrest.core.core.isequal.equal_to` matcher to check for
    equality.

    """
    matchers = []
    for item in items:
        matchers.append(wrap_matcher(item))
    return IsSequenceContainingInOrder(matchers)


def contains(*items: Union[Matcher[T], T]) -> Matcher[Sequence[T]]:
    """Deprecated - use contains_exactly(*items)"""
    warnings.warn("deprecated - use contains_exactly(*items)", DeprecationWarning)
    return contains_exactly(*items)
