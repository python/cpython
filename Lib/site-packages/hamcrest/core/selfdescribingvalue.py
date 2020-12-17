import warnings
from typing import Any

from hamcrest.core.description import Description
from hamcrest.core.selfdescribing import SelfDescribing

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"


class SelfDescribingValue(SelfDescribing):
    """Wrap any value in a ``SelfDescribingValue`` to satisfy the
    :py:class:`~hamcrest.core.selfdescribing.SelfDescribing` interface.

    **Deprecated:** No need for this class now that
    :py:meth:`~hamcrest.core.description.Description.append_description_of`
    handles any type of value.

    """

    def __init__(self, value: Any) -> None:
        warnings.warn("SelfDescribingValue no longer needed", DeprecationWarning)
        self.value = value

    def describe_to(self, description: Description) -> None:
        """Generates a description of the value."""
        description.append_description_of(self.value)
