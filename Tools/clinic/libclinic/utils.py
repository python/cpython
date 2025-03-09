import collections
import enum
import hashlib
import os
import re
import string
from typing import Literal, Final


def write_file(filename: str, new_contents: str) -> None:
    """Write new content to file, iff the content changed."""
    try:
        with open(filename, encoding="utf-8") as fp:
            old_contents = fp.read()

        if old_contents == new_contents:
            # no change: avoid modifying the file modification time
            return
    except FileNotFoundError:
        pass
    # Atomic write using a temporary file and os.replace()
    filename_new = f"{filename}.new"
    with open(filename_new, "w", encoding="utf-8") as fp:
        fp.write(new_contents)
    try:
        os.replace(filename_new, filename)
    except:
        os.unlink(filename_new)
        raise


def compute_checksum(input_: str, length: int | None = None) -> str:
    checksum = hashlib.sha1(input_.encode("utf-8")).hexdigest()
    if length:
        checksum = checksum[:length]
    return checksum


def create_regex(
    before: str, after: str, word: bool = True, whole_line: bool = True
) -> re.Pattern[str]:
    """Create a regex object for matching marker lines."""
    group_re = r"\w+" if word else ".+"
    before = re.escape(before)
    after = re.escape(after)
    pattern = rf"{before}({group_re}){after}"
    if whole_line:
        pattern = rf"^{pattern}$"
    return re.compile(pattern)


class FormatCounterFormatter(string.Formatter):
    """
    This counts how many instances of each formatter
    "replacement string" appear in the format string.

    e.g. after evaluating "string {a}, {b}, {c}, {a}"
         the counts dict would now look like
         {'a': 2, 'b': 1, 'c': 1}
    """

    def __init__(self) -> None:
        self.counts = collections.Counter[str]()

    def get_value(
        self, key: str, args: object, kwargs: object  # type: ignore[override]
    ) -> Literal[""]:
        self.counts[key] += 1
        return ""


VersionTuple = tuple[int, int]


class Sentinels(enum.Enum):
    unspecified = "unspecified"
    unknown = "unknown"

    def __repr__(self) -> str:
        return f"<{self.value.capitalize()}>"


unspecified: Final = Sentinels.unspecified
unknown: Final = Sentinels.unknown


# This one needs to be a distinct class, unlike the other two
class Null:
    def __repr__(self) -> str:
        return '<Null>'


NULL = Null()
