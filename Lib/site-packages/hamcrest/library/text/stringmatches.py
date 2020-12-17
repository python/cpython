import re
from typing import Pattern, Union

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.matcher import Matcher

__author__ = "Chris Rose"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"


class StringMatchesPattern(BaseMatcher[str]):
    def __init__(self, pattern) -> None:
        self.pattern = pattern

    def describe_to(self, description: Description) -> None:
        description.append_text("a string matching '").append_text(
            self.pattern.pattern
        ).append_text("'")

    def _matches(self, item: str) -> bool:
        return self.pattern.search(item) is not None


def matches_regexp(pattern: Union[str, Pattern[str]]) -> Matcher[str]:
    """Matches if object is a string containing a match for a given regular
    expression.

    :param pattern: The regular expression to search for.

    This matcher first checks whether the evaluated object is a string. If so,
    it checks if the regular expression ``pattern`` matches anywhere within the
    evaluated object.

    """
    if isinstance(pattern, str):
        pattern = re.compile(pattern)

    return StringMatchesPattern(pattern)
