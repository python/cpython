from typing import Any, Hashable, Mapping, TypeVar, Union

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.helpers.hasmethod import hasmethod
from hamcrest.core.helpers.wrap_matcher import wrap_matcher
from hamcrest.core.matcher import Matcher

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"

K = TypeVar("K", bound=Hashable)


class IsDictContainingKey(BaseMatcher[Mapping[K, Any]]):
    def __init__(self, key_matcher: Matcher[K]) -> None:
        self.key_matcher = key_matcher

    def _matches(self, item: Mapping[K, Any]) -> bool:
        if hasmethod(item, "keys"):
            for key in item.keys():
                if self.key_matcher.matches(key):
                    return True
        return False

    def describe_to(self, description: Description) -> None:
        description.append_text("a dictionary containing key ").append_description_of(
            self.key_matcher
        )


def has_key(key_match: Union[K, Matcher[K]]) -> Matcher[Mapping[K, Any]]:
    """Matches if dictionary contains an entry whose key satisfies a given
    matcher.

    :param key_match: The matcher to satisfy for the key, or an expected value
        for :py:func:`~hamcrest.core.core.isequal.equal_to` matching.

    This matcher iterates the evaluated dictionary, searching for any key-value
    entry whose key satisfies the given matcher. If a matching entry is found,
    ``has_key`` is satisfied.

    Any argument that is not a matcher is implicitly wrapped in an
    :py:func:`~hamcrest.core.core.isequal.equal_to` matcher to check for
    equality.

    Examples::

        has_key(equal_to('foo'))
        has_key('foo')

    """
    return IsDictContainingKey(wrap_matcher(key_match))
