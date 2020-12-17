from typing import MutableSequence, Optional, Sequence, TypeVar, Union, cast

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.helpers.wrap_matcher import wrap_matcher
from hamcrest.core.matcher import Matcher

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"

T = TypeVar("T")


class MatchInAnyOrder(object):
    def __init__(
        self, matchers: Sequence[Matcher[T]], mismatch_description: Optional[Description]
    ) -> None:
        self.matchers = cast(MutableSequence[Matcher[T]], matchers[:])
        self.mismatch_description = mismatch_description

    def matches(self, item: T) -> bool:
        return self.isnotsurplus(item) and self.ismatched(item)

    def isfinished(self, item: Sequence[T]) -> bool:
        if not self.matchers:
            return True
        if self.mismatch_description:
            self.mismatch_description.append_text("no item matches: ").append_list(
                "", ", ", "", self.matchers
            ).append_text(" in ").append_list("[", ", ", "]", item)
        return False

    def isnotsurplus(self, item: T) -> bool:
        if not self.matchers:
            if self.mismatch_description:
                self.mismatch_description.append_text("not matched: ").append_description_of(item)
            return False
        return True

    def ismatched(self, item: T) -> bool:
        for index, matcher in enumerate(self.matchers):
            if matcher.matches(item):
                del self.matchers[index]
                return True

        if self.mismatch_description:
            self.mismatch_description.append_text("not matched: ").append_description_of(item)
        return False


class IsSequenceContainingInAnyOrder(BaseMatcher[Sequence[T]]):
    def __init__(self, matchers: Sequence[Matcher[T]]) -> None:
        self.matchers = matchers

    def matches(
        self, item: Sequence[T], mismatch_description: Optional[Description] = None
    ) -> bool:
        try:
            sequence = list(item)
            matchsequence = MatchInAnyOrder(self.matchers, mismatch_description)
            for element in sequence:
                if not matchsequence.matches(element):
                    return False
            return matchsequence.isfinished(sequence)
        except TypeError:
            if mismatch_description:
                super(IsSequenceContainingInAnyOrder, self).describe_mismatch(
                    item, mismatch_description
                )
            return False

    def describe_mismatch(self, item: Sequence[T], mismatch_description: Description) -> None:
        self.matches(item, mismatch_description)

    def describe_to(self, description: Description) -> None:
        description.append_text("a sequence over ").append_list(
            "[", ", ", "]", self.matchers
        ).append_text(" in any order")


def contains_inanyorder(*items: Union[Matcher[T], T]) -> Matcher[Sequence[T]]:
    """Matches if sequences's elements, in any order, satisfy a given list of
    matchers.

    :param match1,...: A comma-separated list of matchers.

    This matcher iterates the evaluated sequence, seeing if each element
    satisfies any of the given matchers. The matchers are tried from left to
    right, and when a satisfied matcher is found, it is no longer a candidate
    for the remaining elements. If a one-to-one correspondence is established
    between elements and matchers, ``contains_inanyorder`` is satisfied.

    Any argument that is not a matcher is implicitly wrapped in an
    :py:func:`~hamcrest.core.core.isequal.equal_to` matcher to check for
    equality.

    """

    matchers = []
    for item in items:
        matchers.append(wrap_matcher(item))
    return IsSequenceContainingInAnyOrder(matchers)
