from __future__ import annotations

__all__ = [
    "next_block", "make_constant",
    "Style", "C99_STYLE", "C11_STYLE", "DOXYGEN_STYLE",
    "CWriter",
]  # fmt: skip

import contextlib
import enum
from io import StringIO
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from collections.abc import Iterator
    from typing import Any, Final, Literal


def next_block(w: int) -> int:
    """Compute the smallest multiple of 4 strictly larger than *w*."""
    return ((w + 3) & ~0x03) if (w % 4) else (w + 4)


_MASKSIZE: Final[int] = next_block(len("0x00000000"))


def make_constant(key: str, bit: int, name_maxsize: int) -> str:
    assert bit <= 32, f"{key}: mask does not on an uint32_t"
    member_name = key.ljust(name_maxsize)
    member_mask = format(1 << bit, "008x")
    member_mask = f"0x{member_mask}".ljust(_MASKSIZE)
    return f"#define {member_name}{member_mask}// bit = {bit}"


class Style(enum.IntEnum):
    C99 = enum.auto()
    C11 = enum.auto()
    DOXYGEN = enum.auto()


C99_STYLE: Final[Literal[Style.C99]] = Style.C99
C11_STYLE: Final[Literal[Style.C11]] = Style.C11
DOXYGEN_STYLE: Final[Literal[Style.DOXYGEN]] = Style.DOXYGEN

_COMMENT_INLINE_STYLE: Final[dict[Style, tuple[str, str, str]]] = {
    C99_STYLE: ("// ", "", ""),
    C11_STYLE: ("/* ", " */", ""),
    DOXYGEN_STYLE: ("/** ", " */", ""),
}

_COMMENT_BLOCK_STYLE: Final[dict[Style, tuple[str, str, str]]] = {
    C99_STYLE: ("// ", "", ""),
    C11_STYLE: ("/*", " */", " * "),
    DOXYGEN_STYLE: ("/**", " */", " * "),
}


class CWriter:
    def __init__(self, *, indentsize: int = 4) -> None:
        self._stream = StringIO()
        self._indent = " " * indentsize
        self._prefix = ""

    def comment(
        self, text: str, *, level: int = 0, style: Style = C11_STYLE
    ) -> None:
        """Add a C comment, possibly using doxygen style."""
        if len(text) < 72 and "\n" not in text:
            prolog, epilog, _ = _COMMENT_INLINE_STYLE[style]
            self.write(prolog, text, epilog, sep="", level=level)
        else:
            prolog, epilog, prefix = _COMMENT_BLOCK_STYLE[style]
            self.write(prolog, level=level)
            with self.prefixed(prefix):
                for line in text.splitlines():
                    self.write(line, level=level)
            self.write(epilog, level=level)

    @contextlib.contextmanager
    def prefixed(self, prefix: str) -> Iterator[None]:
        old_prefix = self._prefix
        self._prefix = prefix
        try:
            yield
        finally:
            self._prefix = old_prefix

    def _prefix_at(self, level: int) -> str:
        return "".join((self._indent * level, self._prefix))

    def write(
        self, *args: Any, sep: str = " ", end: str = "\n", level: int = 0
    ) -> None:
        if prefix := self._prefix_at(level):
            self._write(prefix, sep="", end="")
        self._write(*args, sep=sep, end=end)

    def write_blankline(self) -> None:
        self._write()

    def _write(self, *args: Any, sep: str = " ", end: str = "\n") -> None:
        print(*args, sep=sep, end=end, file=self._stream)

    def build(self) -> str:
        # inject directives to temporarily disable external C formatters
        return "\n".join(
            (
                "// clang-format off",
                self._stream.getvalue().rstrip(),
                "// clang-format on",
            )
        )
