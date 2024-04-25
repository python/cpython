import io
import os
import sys

COLORIZE = True


class ANSIColors:
    BOLD_GREEN = "\x1b[1;32m"
    BOLD_MAGENTA = "\x1b[1;35m"
    BOLD_RED = "\x1b[1;31m"
    GREEN = "\x1b[32m"
    GREY = "\x1b[90m"
    MAGENTA = "\x1b[35m"
    RED = "\x1b[31m"
    RESET = "\x1b[0m"
    YELLOW = "\x1b[33m"


def get_colors(colorize: bool = False) -> ANSIColors:
    if colorize or can_colorize():
        return ANSIColors()

    ansi_colors = ANSIColors()

    # Set color attributes to empty strings
    for attr in dir(ansi_colors):
        if not attr.startswith("__"):
            setattr(ansi_colors, attr, "")

    return ansi_colors


def can_colorize() -> bool:
    if sys.platform == "win32":
        try:
            import nt

            if not nt._supports_virtual_terminal():
                return False
        except (ImportError, AttributeError):
            return False
    if not sys.flags.ignore_environment:
        if os.environ.get("PYTHON_COLORS") == "0":
            return False
        if os.environ.get("PYTHON_COLORS") == "1":
            return True
        if "NO_COLOR" in os.environ:
            return False
    if not COLORIZE:
        return False
    if not sys.flags.ignore_environment:
        if "FORCE_COLOR" in os.environ:
            return True
        if os.environ.get("TERM") == "dumb":
            return False

    if not hasattr(sys.stderr, "fileno"):
        return False

    try:
        return os.isatty(sys.stderr.fileno())
    except io.UnsupportedOperation:
        return sys.stderr.isatty()
