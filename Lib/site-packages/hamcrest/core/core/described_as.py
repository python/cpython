import re
from typing import Any, Optional, Tuple

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.matcher import Matcher

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"

ARG_PATTERN = re.compile("%([0-9]+)")


class DescribedAs(BaseMatcher[Any]):
    def __init__(
        self, description_template: str, matcher: Matcher[Any], *values: Tuple[Any, ...]
    ) -> None:
        self.template = description_template
        self.matcher = matcher
        self.values = values

    def matches(self, item: Any, mismatch_description: Optional[Description] = None) -> bool:
        return self.matcher.matches(item, mismatch_description)

    def describe_mismatch(self, item: Any, mismatch_description: Description) -> None:
        self.matcher.describe_mismatch(item, mismatch_description)

    def describe_to(self, description: Description) -> None:
        text_start = 0
        for match in re.finditer(ARG_PATTERN, self.template):
            description.append_text(self.template[text_start : match.start()])
            arg_index = int(match.group()[1:])
            description.append_description_of(self.values[arg_index])
            text_start = match.end()

        if text_start < len(self.template):
            description.append_text(self.template[text_start:])


def described_as(description: str, matcher: Matcher[Any], *values) -> Matcher[Any]:
    """Adds custom failure description to a given matcher.

    :param description: Overrides the matcher's description.
    :param matcher: The matcher to satisfy.
    :param value1,...: Optional comma-separated list of substitution values.

    The description may contain substitution placeholders %0, %1, etc. These
    will be replaced by any values that follow the matcher.

    """
    return DescribedAs(description, matcher, *values)
