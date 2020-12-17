from typing import Any, Optional

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.matcher import Matcher

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"


class IsAnything(BaseMatcher[Any]):
    def __init__(self, description: Optional[str]) -> None:
        self.description = description or "ANYTHING"  # type: str

    def _matches(self, item: Any) -> bool:
        return True

    def describe_to(self, description: Description) -> None:
        description.append_text(self.description)


def anything(description: Optional[str] = None) -> Matcher[Any]:
    """Matches anything.

    :param description: Optional string used to describe this matcher.

    This matcher always evaluates to ``True``. Specify this in composite
    matchers when the value of a particular element is unimportant.

    """
    return IsAnything(description)
