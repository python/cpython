from typing import Sequence, TypeVar, Union

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.core.anyof import any_of
from hamcrest.core.description import Description
from hamcrest.core.helpers.wrap_matcher import wrap_matcher
from hamcrest.core.matcher import Matcher

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"

T = TypeVar("T")


class IsSequenceOnlyContaining(BaseMatcher[Sequence[T]]):
    def __init__(self, matcher: Matcher[T]) -> None:
        self.matcher = matcher

    def _matches(self, item: Sequence[T]) -> bool:
        try:
            sequence = list(item)
            if len(sequence) == 0:
                return False
            for element in sequence:
                if not self.matcher.matches(element):
                    return False
            return True
        except TypeError:
            return False

    def describe_to(self, description: Description) -> None:
        description.append_text("a sequence containing items matching ").append_description_of(
            self.matcher
        )


def only_contains(*items: Union[Matcher[T], T]) -> Matcher[Sequence[T]]:
    """Matches if each element of sequence satisfies any of the given matchers.

    :param match1,...: A comma-separated list of matchers.

    This matcher iterates the evaluated sequence, confirming whether each
    element satisfies any of the given matchers.

    Example::

        only_contains(less_than(4))

    will match ``[3,1,2]``.

    Any argument that is not a matcher is implicitly wrapped in an
    :py:func:`~hamcrest.core.core.isequal.equal_to` matcher to check for
    equality.

    """
    matchers = []
    for item in items:
        matchers.append(wrap_matcher(item))
    return IsSequenceOnlyContaining(any_of(*matchers))
