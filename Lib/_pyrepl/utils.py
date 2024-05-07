import re
import unicodedata

ANSI_ESCAPE_SEQUENCE = re.compile(r"\x1b\[[ -@]*[A-~]")


def str_width(c: str) -> int:
    w = unicodedata.east_asian_width(c)
    if w in ('N', 'Na', 'H', 'A'):
        return 1
    return 2


def wlen(s: str) -> int:
    length = sum(str_width(i) for i in s)

    # remove lengths of any escape sequences
    return length - sum(len(i) for i in ANSI_ESCAPE_SEQUENCE.findall(s))
