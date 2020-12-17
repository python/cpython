from typing import Any, Iterable

from hamcrest.core.description import Description
from hamcrest.core.helpers.hasmethod import hasmethod
from hamcrest.core.helpers.ismock import ismock

__author__ = "Jon Reid"
__copyright__ = "Copyright 2011 hamcrest.org"
__license__ = "BSD, see License.txt"


class BaseDescription(Description):
    """Base class for all :py:class:`~hamcrest.core.description.Description`
    implementations.

    """

    def append_text(self, text: str) -> Description:
        self.append(text)
        return self

    def append_description_of(self, value: Any) -> Description:
        if not ismock(value) and hasmethod(value, "describe_to"):
            value.describe_to(self)
        elif isinstance(value, str):
            self.append(repr(value))
        else:
            description = str(value)
            if description[:1] == "<" and description[-1:] == ">":
                self.append(description)
            else:
                self.append("<")
                self.append(description)
                self.append(">")
        return self

    def append_list(self, start: str, separator: str, end: str, list: Iterable[Any]) -> Description:
        separate = False

        self.append(start)
        for item in list:
            if separate:
                self.append(separator)
            self.append_description_of(item)
            separate = True
        self.append(end)
        return self

    def append(self, string: str) -> None:
        """Append the string to the description."""
        raise NotImplementedError("append")

    def append_string_in_python_syntax(self, string: str) -> None:
        self.append("'")
        for ch in string:
            self.append(character_in_python_syntax(ch))
        self.append("'")


def character_in_python_syntax(ch: str) -> str:
    if ch == "'":
        return "'"
    elif ch == "\n":
        return "\\n"
    elif ch == "\r":
        return "\\r"
    elif ch == "\t":
        return "\\t"
    else:
        return ch
