from collections.abc import Sized
from typing import Union

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.helpers.hasmethod import hasmethod
from hamcrest.core.helpers.wrap_matcher import wrap_matcher
from hamcrest.core.matcher import Matcher

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"


class HasLength(BaseMatcher[Sized]):
    def __init__(self, len_matcher: Matcher[int]) -> None:
        self.len_matcher = len_matcher

    def _matches(self, item: Sized) -> bool:
        if not hasmethod(item, "__len__"):
            return False
        return self.len_matcher.matches(len(item))

    def describe_mismatch(self, item: Sized, mismatch_description: Description) -> None:
        super(HasLength, self).describe_mismatch(item, mismatch_description)
        if hasmethod(item, "__len__"):
            mismatch_description.append_text(" with length of ").append_description_of(len(item))

    def describe_to(self, description: Description) -> None:
        description.append_text("an object with length of ").append_description_of(self.len_matcher)


def has_length(match: Union[int, Matcher[int]]) -> Matcher[Sized]:
    """Matches if ``len(item)`` satisfies a given matcher.

    :param match: The matcher to satisfy, or an expected value for
        :py:func:`~hamcrest.core.core.isequal.equal_to` matching.

    This matcher invokes the :py:func:`len` function on the evaluated object to
    get its length, passing the result to a given matcher for evaluation.

    If the ``match`` argument is not a matcher, it is implicitly wrapped in an
    :py:func:`~hamcrest.core.core.isequal.equal_to` matcher to check for
    :equality.

    Examples::

        has_length(greater_than(6))
        has_length(5)

    """
    return HasLength(wrap_matcher(match))
