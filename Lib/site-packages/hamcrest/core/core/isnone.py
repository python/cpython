from typing import Any, Optional

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.matcher import Matcher

from .isnot import is_not

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"


class IsNone(BaseMatcher[Optional[Any]]):
    def _matches(self, item: Any) -> bool:
        return item is None

    def describe_to(self, description: Description) -> None:
        description.append_text("None")


def none() -> Matcher[Optional[Any]]:
    """Matches if object is ``None``."""
    return IsNone()


def not_none() -> Matcher[Optional[Any]]:
    """Matches if object is not ``None``."""
    return is_not(none())
