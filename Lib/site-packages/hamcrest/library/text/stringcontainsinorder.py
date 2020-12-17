from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.helpers.hasmethod import hasmethod
from hamcrest.core.matcher import Matcher

__author__ = "Romilly Cocking"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"


class StringContainsInOrder(BaseMatcher[str]):
    def __init__(self, *substrings) -> None:
        for substring in substrings:
            if not isinstance(substring, str):
                raise TypeError(self.__class__.__name__ + " requires string arguments")
        self.substrings = substrings

    def _matches(self, item: str) -> bool:
        if not hasmethod(item, "find"):
            return False
        from_index = 0
        for substring in self.substrings:
            from_index = item.find(substring, from_index)
            if from_index == -1:
                return False
        return True

    def describe_to(self, description: Description) -> None:
        description.append_list("a string containing ", ", ", " in order", self.substrings)


def string_contains_in_order(*substrings: str) -> Matcher[str]:
    """Matches if object is a string containing a given list of substrings in
    relative order.

    :param string1,...:  A comma-separated list of strings.

    This matcher first checks whether the evaluated object is a string. If so,
    it checks whether it contains a given list of strings, in relative order to
    each other. The searches are performed starting from the beginning of the
    evaluated string.

    Example::

        string_contains_in_order("bc", "fg", "jkl")

    will match "abcdefghijklm".

    """
    return StringContainsInOrder(*substrings)
