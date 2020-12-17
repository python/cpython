from typing import Optional, TypeVar

from hamcrest.core.description import Description
from hamcrest.core.matcher import Matcher
from hamcrest.core.string_description import tostring

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"

T = TypeVar("T")


class BaseMatcher(Matcher[T]):
    """Base class for all :py:class:`~hamcrest.core.matcher.Matcher`
    implementations.

    Most implementations can just implement :py:obj:`_matches`, leaving the
    handling of any mismatch description to the ``matches`` method. But if it
    makes more sense to generate the mismatch description during the matching,
    override :py:meth:`~hamcrest.core.matcher.Matcher.matches` instead.

    """

    def __str__(self) -> str:
        return tostring(self)

    def _matches(self, item: T) -> bool:
        raise NotImplementedError("_matches")

    def matches(self, item: T, mismatch_description: Optional[Description] = None) -> bool:
        match_result = self._matches(item)
        if not match_result and mismatch_description:
            self.describe_mismatch(item, mismatch_description)
        return match_result

    def describe_mismatch(self, item: T, mismatch_description: Description) -> None:
        mismatch_description.append_text("was ").append_description_of(item)

    def describe_match(self, item: T, match_description: Description) -> None:
        match_description.append_text("was ").append_description_of(item)
