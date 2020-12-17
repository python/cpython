from typing import Any, Iterable, Sequence

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"


class Description:
    """A description of a :py:class:`~hamcrest.core.matcher.Matcher`.

    A :py:class:`~hamcrest.core.matcher.Matcher` will describe itself to a
    description which can later be used for reporting.

    """

    def append_text(self, text: str) -> "Description":
        """Appends some plain text to the description.

        :returns: ``self``, for chaining

        """
        raise NotImplementedError("append_text")

    def append_description_of(self, value: Any) -> "Description":
        """Appends description of given value to this description.

        If the value implements
        :py:meth:`~hamcrest.core.selfdescribing.SelfDescribing.describe_to`,
        then it will be used.

        :returns: ``self``, for chaining

        """
        raise NotImplementedError("append_description_of")

    def append_list(
        self, start: str, separator: str, end: str, list: Iterable[Any]
    ) -> "Description":
        """Appends a list of objects to the description.

        :param start: String that will begin the list description.
        :param separator: String that will separate each object in the
            description.
        :param end: String that will end the list description.
        :param list: List of objects to be described.

        :returns: ``self``, for chaining

        """
        raise NotImplementedError("append_list")
