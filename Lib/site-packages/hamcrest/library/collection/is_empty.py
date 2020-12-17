from typing import Optional, Sized

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.matcher import Matcher

__author__ = "Chris Rose"
__copyright__ = "Copyright 2012 hamcrest.org"
__license__ = "BSD, see License.txt"


class IsEmpty(BaseMatcher[Sized]):
    def matches(self, item: Sized, mismatch_description: Optional[Description] = None) -> bool:
        try:
            if len(item) == 0:
                return True

            if mismatch_description:
                mismatch_description.append_text("has %d item(s)" % len(item))

        except TypeError:
            if mismatch_description:
                mismatch_description.append_text("does not support length")

        return False

    def describe_to(self, description: Description) -> None:
        description.append_text("an empty collection")


def empty() -> Matcher[Sized]:
    """
    This matcher matches any collection-like object that responds to the
    __len__ method, and has a length of 0.
    """
    return IsEmpty()
