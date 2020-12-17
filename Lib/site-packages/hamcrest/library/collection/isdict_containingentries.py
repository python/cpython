from typing import Any, Hashable, Mapping, Optional, TypeVar, Union, overload

from hamcrest.core.base_matcher import BaseMatcher
from hamcrest.core.description import Description
from hamcrest.core.helpers.wrap_matcher import wrap_matcher
from hamcrest.core.matcher import Matcher

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"

K = TypeVar("K", bound=Hashable)
V = TypeVar("V")


class IsDictContainingEntries(BaseMatcher[Mapping[K, V]]):
    def __init__(self, value_matchers) -> None:
        self.value_matchers = sorted(value_matchers.items())

    def _not_a_dictionary(
        self, item: Mapping[K, V], mismatch_description: Optional[Description]
    ) -> bool:
        if mismatch_description:
            mismatch_description.append_description_of(item).append_text(" is not a mapping object")
        return False

    def matches(
        self, item: Mapping[K, V], mismatch_description: Optional[Description] = None
    ) -> bool:
        for key, value_matcher in self.value_matchers:

            try:
                if not key in item:
                    if mismatch_description:
                        mismatch_description.append_text("no ").append_description_of(
                            key
                        ).append_text(" key in ").append_description_of(item)
                    return False
            except TypeError:
                return self._not_a_dictionary(item, mismatch_description)

            try:
                actual_value = item[key]
            except TypeError:
                return self._not_a_dictionary(item, mismatch_description)

            if not value_matcher.matches(actual_value):
                if mismatch_description:
                    mismatch_description.append_text("value for ").append_description_of(
                        key
                    ).append_text(" ")
                    value_matcher.describe_mismatch(actual_value, mismatch_description)
                return False

        return True

    def describe_mismatch(self, item: Mapping[K, V], mismatch_description: Description) -> None:
        self.matches(item, mismatch_description)

    def describe_keyvalue(self, index: int, value: V, description: Description) -> None:
        """Describes key-value pair at given index."""
        description.append_description_of(index).append_text(": ").append_description_of(value)

    def describe_to(self, description: Description) -> None:
        description.append_text("a dictionary containing {")
        first = True
        for key, value in self.value_matchers:
            if not first:
                description.append_text(", ")
            self.describe_keyvalue(key, value, description)
            first = False
        description.append_text("}")


# Keyword argument form
@overload
def has_entries(**keys_valuematchers: Union[Matcher[V], V]) -> Matcher[Mapping[str, V]]:
    ...


# Key to matcher dict form
@overload
def has_entries(keys_valuematchers: Mapping[K, Union[Matcher[V], V]]) -> Matcher[Mapping[K, V]]:
    ...


# Alternating key/matcher form
@overload
def has_entries(*keys_valuematchers: Any) -> Matcher[Mapping[Any, Any]]:
    ...


def has_entries(*keys_valuematchers, **kv_args):
    """Matches if dictionary contains entries satisfying a dictionary of keys
    and corresponding value matchers.

    :param matcher_dict: A dictionary mapping keys to associated value matchers,
        or to expected values for
        :py:func:`~hamcrest.core.core.isequal.equal_to` matching.

    Note that the keys must be actual keys, not matchers. Any value argument
    that is not a matcher is implicitly wrapped in an
    :py:func:`~hamcrest.core.core.isequal.equal_to` matcher to check for
    equality.

    Examples::

        has_entries({'foo':equal_to(1), 'bar':equal_to(2)})
        has_entries({'foo':1, 'bar':2})

    ``has_entries`` also accepts a list of keyword arguments:

    .. function:: has_entries(keyword1=value_matcher1[, keyword2=value_matcher2[, ...]])

    :param keyword1: A keyword to look up.
    :param valueMatcher1: The matcher to satisfy for the value, or an expected
        value for :py:func:`~hamcrest.core.core.isequal.equal_to` matching.

    Examples::

        has_entries(foo=equal_to(1), bar=equal_to(2))
        has_entries(foo=1, bar=2)

    Finally, ``has_entries`` also accepts a list of alternating keys and their
    value matchers:

    .. function:: has_entries(key1, value_matcher1[, ...])

    :param key1: A key (not a matcher) to look up.
    :param valueMatcher1: The matcher to satisfy for the value, or an expected
        value for :py:func:`~hamcrest.core.core.isequal.equal_to` matching.

    Examples::

        has_entries('foo', equal_to(1), 'bar', equal_to(2))
        has_entries('foo', 1, 'bar', 2)

    """
    if len(keys_valuematchers) == 1:
        try:
            base_dict = keys_valuematchers[0].copy()
            for key in base_dict:
                base_dict[key] = wrap_matcher(base_dict[key])
        except AttributeError:
            raise ValueError(
                "single-argument calls to has_entries must pass a dict as the argument"
            )
    else:
        if len(keys_valuematchers) % 2:
            raise ValueError("has_entries requires key-value pairs")
        base_dict = {}
        for index in range(int(len(keys_valuematchers) / 2)):
            base_dict[keys_valuematchers[2 * index]] = wrap_matcher(
                keys_valuematchers[2 * index + 1]
            )

    for key, value in kv_args.items():
        base_dict[key] = wrap_matcher(value)

    return IsDictContainingEntries(base_dict)
