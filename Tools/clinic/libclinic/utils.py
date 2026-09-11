import collections
import dataclasses as dc
import enum
import hashlib
import os
import re
import string
from collections.abc import Iterable
from typing import Literal, Final


def read_file(filename: str) -> str | None:
    """Return the content of the file, or None if it does not exist."""
    try:
        with open(filename, encoding="utf-8") as fp:
            return fp.read()
    except FileNotFoundError:
        return None


def write_file(filename: str, new_contents: str) -> bool:
    """Write new content to file, iff the content changed.

    Return True if the file was written.
    """
    if read_file(filename) == new_contents:
        # no change: avoid modifying the file modification time
        return False
    # Atomic write using a temporary file and os.replace()
    filename_new = f"{filename}.new"
    with open(filename_new, "w", encoding="utf-8") as fp:
        fp.write(new_contents)
    try:
        os.replace(filename_new, filename)
    except:
        os.unlink(filename_new)
        raise
    return True


@dc.dataclass(slots=True, frozen=True)
class FileChange:
    filename: str
    # None if the file does not exist yet.
    old_contents: str | None
    new_contents: str


@dc.dataclass(slots=True)
class FileWriter:
    """Write the generated files.

    In the dry run mode no file is written, the changes are only recorded.
    """

    dry_run: bool = False
    changes: list[FileChange] = dc.field(default_factory=list)
    # (filename, changed) for every file which was passed to write().
    files: list[tuple[str, bool]] = dc.field(default_factory=list)

    def makedirs(self, dirname: str) -> None:
        if not self.dry_run:
            os.makedirs(dirname)
        elif os.path.exists(dirname):
            # Create nothing, but fail as os.makedirs() does, so that
            # the caller can report an existing non-directory.
            raise FileExistsError(dirname)

    def write(self, filename: str, new_contents: str) -> None:
        if not self.dry_run:
            changed = write_file(filename, new_contents)
        else:
            old_contents = read_file(filename)
            changed = old_contents != new_contents
            if changed:
                self.changes.append(
                    FileChange(filename, old_contents, new_contents))
        self.files.append((filename, changed))

    def update_times(self, source: str, generated: Iterable[str],
                     changed: bool) -> None:
        """Keep the generated files newer than the source file.

        The build system does not know that the source file depends on
        the files generated from it, so the source file is touched to
        force its recompilation.
        """
        if self.dry_run:
            return
        if changed:
            os.utime(source)
            for filename in generated:
                os.utime(filename)
        else:
            mtime = os.stat(source).st_mtime_ns
            for filename in generated:
                if os.stat(filename).st_mtime_ns <= mtime:
                    os.utime(filename)


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
class NullType:
    def __repr__(self) -> str:
        return '<Null>'


NULL = NullType()
