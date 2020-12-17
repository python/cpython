from typing import Optional, TypeVar, Union

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.helpers.wrap_matcher import wrap_matcher
from hamcrest.core.matcher import Matcher

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"

T = TypeVar("T")


class AllOf(BaseMatcher[T]):
    def __init__(self, *matchers: Matcher[T], **kwargs):
        self.matchers = matchers
        self.describe_matcher_in_mismatch = kwargs.pop(
            "describe_matcher_in_mismatch", True
        )  # No keyword-only args in 2.7 :-(
        self.describe_all_mismatches = kwargs.pop("describe_all_mismatches", False)

    def matches(self, item: T, mismatch_description: Optional[Description] = None) -> bool:
        found_mismatch = False
        for i, matcher in enumerate(self.matchers):
            if not matcher.matches(item):
                if mismatch_description:
                    if self.describe_matcher_in_mismatch:
                        mismatch_description.append_description_of(matcher).append_text(" ")
                    matcher.describe_mismatch(item, mismatch_description)
                found_mismatch = True
                if not self.describe_all_mismatches:
                    break
                elif i < len(self.matchers) - 1 and mismatch_description:
                    mismatch_description.append_text(" and ")
        return not found_mismatch

    def describe_mismatch(self, item: T, mismatch_description: Description) -> None:
        self.matches(item, mismatch_description)

    def describe_to(self, description: Description) -> None:
        description.append_list("(", " and ", ")", self.matchers)


def all_of(*items: Union[Matcher[T], T]) -> Matcher[T]:
    """Matches if all of the given matchers evaluate to ``True``.

    :param matcher1,...:  A comma-separated list of matchers.

    The matchers are evaluated from left to right using short-circuit
    evaluation, so evaluation stops as soon as a matcher returns ``False``.

    Any argument that is not a matcher is implicitly wrapped in an
    :py:func:`~hamcrest.core.core.isequal.equal_to` matcher to check for
    equality.

    """
    return AllOf(*[wrap_matcher(item) for item in items])
