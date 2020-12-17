from hamcrest.core.selfdescribing import SelfDescribing

from .base_description import BaseDescription

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"


def tostring(selfdescribing: SelfDescribing) -> str:
    """Returns the description of a
    :py:class:`~hamcrest.core.selfdescribing.SelfDescribing` object as a
    string.

    :param selfdescribing: The object to be described.
    :returns: The description of the object.
    """
    return str(StringDescription().append_description_of(selfdescribing))


class StringDescription(BaseDescription):
    """A :py:class:`~hamcrest.core.description.Description` that is stored as a
    string.

    """

    def __init__(self) -> None:
        self.out = ""

    def __str__(self) -> str:
        """Returns the description."""
        return self.out

    def append(self, string: str) -> None:
        self.out += str(string)
