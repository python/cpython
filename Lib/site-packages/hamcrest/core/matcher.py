from typing import Generic, Optional, TypeVar

from hamcrest.core.description import Description

from .selfdescribing import SelfDescribing

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"

T = TypeVar("T")


class Matcher(Generic[T], SelfDescribing):
    """A matcher over acceptable values.

    A matcher is able to describe itself to give feedback when it fails.

    Matcher implementations should *not* directly implement this protocol.
    Instead, *extend* the :py:class:`~hamcrest.core.base_matcher.BaseMatcher`
    class, which will ensure that the
    :py:class:`~hamcrest.core.matcher.Matcher` API can grow to support new
    features and remain compatible with all
    :py:class:`~hamcrest.core.matcher.Matcher` implementations.

    """

    def matches(self, item: T, mismatch_description: Optional[Description] = None) -> bool:
        """Evaluates the matcher for argument item.

        If a mismatch is detected and argument ``mismatch_description`` is
        provided, it will generate a description of why the matcher has not
        accepted the item.

        :param item: The object against which the matcher is evaluated.
        :param mismatch_description:
        :returns: ``True`` if ``item`` matches, otherwise ``False``.

        """
        raise NotImplementedError("matches")

    def describe_mismatch(self, item: T, mismatch_description: Description) -> None:
        """Generates a description of why the matcher has not accepted the
        item.

        The description will be part of a larger description of why a matching
        failed, so it should be concise.

        This method assumes that ``matches(item)`` is ``False``, but will not
        check this.

        :param item: The item that the
            :py:class:`~hamcrest.core.matcher.Matcher` has rejected.
        :param mismatch_description: The description to be built or appended
            to.

        """
        raise NotImplementedError("describe_mismatch")

    def describe_match(self, item: T, match_description: Description) -> None:
        """Generates a description of why the matcher has accepted the item.

        The description may be part of a larger description of why a matching
        failed, so it should be concise.

        This method assumes that ``matches(item)`` is ``True``, but will not
        check this.

        :param item: The item that the
            :py:class:`~hamcrest.core.matcher.Matcher` has accepted.
        :param match_description: The description to be built or appended to.

        """
        raise NotImplementedError("describe_match")
