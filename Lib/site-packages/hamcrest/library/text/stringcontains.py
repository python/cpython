from hamcrest.core.helpers.hasmethod import hasmethod
from hamcrest.core.matcher import Matcher
from hamcrest.library.text.substringmatcher import SubstringMatcher

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"


class StringContains(SubstringMatcher):
    def __init__(self, substring) -> None:
        super(StringContains, self).__init__(substring)

    def _matches(self, item: str) -> bool:
        if not hasmethod(item, "find"):
            return False
        return item.find(self.substring) >= 0

    def relationship(self):
        return "containing"


def contains_string(substring: str) -> Matcher[str]:
    """Matches if object is a string containing a given string.

    :param string: The string to search for.

    This matcher first checks whether the evaluated object is a string. If so,
    it checks whether it contains ``string``.

    Example::

        contains_string("def")

    will match "abcdefg".

    """
    return StringContains(substring)
