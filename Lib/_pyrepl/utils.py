import re
import unicodedata
from enum import Enum

ANSI_ESCAPE_SEQUENCE = re.compile(r"\x1b\[[ -@]*[A-~]")


class CalcScreen(Enum):
    CALC_COMPLETE_SCREEN = 1
    CALC_APPEND_SCREEN = 2


def str_width(c: str) -> int:
    w = unicodedata.east_asian_width(c)
    if w in ('N', 'Na', 'H', 'A'):
        return 1
    return 2


def wlen(s: str) -> int:
    length = sum(str_width(i) for i in s)

    # remove lengths of any escape sequences
    return length - sum(len(i) for i in ANSI_ESCAPE_SEQUENCE.findall(s))
