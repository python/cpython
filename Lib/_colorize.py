import os
import sys

from collections.abc import Callable, Iterator, Mapping
from dataclasses import dataclass, field, Field

COLORIZE = True


# types
if False:
    from typing import IO, Self, ClassVar
    _theme: Theme


class ANSIColors:
    RESET = "\x1b[0m"

    BLACK = "\x1b[30m"
    BLUE = "\x1b[34m"
    CYAN = "\x1b[36m"
    GREEN = "\x1b[32m"
    GREY = "\x1b[90m"
    MAGENTA = "\x1b[35m"
    RED = "\x1b[31m"
    WHITE = "\x1b[37m"  # more like LIGHT GRAY
    YELLOW = "\x1b[33m"

    BOLD = "\x1b[1m"
    BOLD_BLACK = "\x1b[1;30m"  # DARK GRAY
    BOLD_BLUE = "\x1b[1;34m"
    BOLD_CYAN = "\x1b[1;36m"
    BOLD_GREEN = "\x1b[1;32m"
    BOLD_MAGENTA = "\x1b[1;35m"
    BOLD_RED = "\x1b[1;31m"
    BOLD_WHITE = "\x1b[1;37m"  # actual WHITE
    BOLD_YELLOW = "\x1b[1;33m"

    # intense = like bold but without being bold
    INTENSE_BLACK = "\x1b[90m"
    INTENSE_BLUE = "\x1b[94m"
    INTENSE_CYAN = "\x1b[96m"
    INTENSE_GREEN = "\x1b[92m"
    INTENSE_MAGENTA = "\x1b[95m"
    INTENSE_RED = "\x1b[91m"
    INTENSE_WHITE = "\x1b[97m"
    INTENSE_YELLOW = "\x1b[93m"

    BACKGROUND_BLACK = "\x1b[40m"
    BACKGROUND_BLUE = "\x1b[44m"
    BACKGROUND_CYAN = "\x1b[46m"
    BACKGROUND_GREEN = "\x1b[42m"
    BACKGROUND_MAGENTA = "\x1b[45m"
    BACKGROUND_RED = "\x1b[41m"
    BACKGROUND_WHITE = "\x1b[47m"
    BACKGROUND_YELLOW = "\x1b[43m"

    INTENSE_BACKGROUND_BLACK = "\x1b[100m"
    INTENSE_BACKGROUND_BLUE = "\x1b[104m"
    INTENSE_BACKGROUND_CYAN = "\x1b[106m"
    INTENSE_BACKGROUND_GREEN = "\x1b[102m"
    INTENSE_BACKGROUND_MAGENTA = "\x1b[105m"
    INTENSE_BACKGROUND_RED = "\x1b[101m"
    INTENSE_BACKGROUND_WHITE = "\x1b[107m"
    INTENSE_BACKGROUND_YELLOW = "\x1b[103m"


ColorCodes = set()
NoColors = ANSIColors()

for attr, code in ANSIColors.__dict__.items():
    if not attr.startswith("__"):
        ColorCodes.add(code)
        setattr(NoColors, attr, "")


#
# Experimental theming support (see gh-133346)
#

# - Create a theme by copying an existing `Theme` with one or more sections
#   replaced, using `default_theme.copy_with()`;
# - create a theme section by copying an existing `ThemeSection` with one or
#   more colors replaced, using for example `default_theme.syntax.copy_with()`;
# - create a theme from scratch by instantiating a `Theme` data class with
#   the required sections (which are also dataclass instances).
#
# Then call `_colorize.set_theme(your_theme)` to set it.
#
# Put your theme configuration in $PYTHONSTARTUP for the interactive shell,
# or sitecustomize.py in your virtual environment or Python installation for
# other uses.  Your applications can call `_colorize.set_theme()` too.
#
# Note that thanks to the dataclasses providing default values for all fields,
# creating a new theme or theme section from scratch is possible without
# specifying all keys.
#
# For example, here's a theme that makes punctuation and operators less prominent:
#
#   try:
#       from _colorize import set_theme, default_theme, Syntax, ANSIColors
#   except ImportError:
#       pass
#   else:
#       theme_with_dim_operators = default_theme.copy_with(
#           syntax=Syntax(op=ANSIColors.INTENSE_BLACK),
#       )
#       set_theme(theme_with_dim_operators)
#       del set_theme, default_theme, Syntax, ANSIColors, theme_with_dim_operators
#
# Guarding the import ensures that your .pythonstartup file will still work in
# Python 3.13 and older. Deleting the variables ensures they don't remain in your
# interactive shell's global scope.

class ThemeSection(Mapping[str, str]):
    """A mixin/base class for theme sections.

    It enables dictionary access to a section, as well as implements convenience
    methods.
    """

    # The two types below are just that: types to inform the type checker that the
    # mixin will work in context of those fields existing
    __dataclass_fields__: ClassVar[dict[str, Field[str]]]
    _name_to_value: Callable[[str], str]

    def __post_init__(self) -> None:
        name_to_value = {}
        for color_name in self.__dataclass_fields__:
            name_to_value[color_name] = getattr(self, color_name)
        super().__setattr__('_name_to_value', name_to_value.__getitem__)

    def copy_with(self, **kwargs: str) -> Self:
        color_state: dict[str, str] = {}
        for color_name in self.__dataclass_fields__:
            color_state[color_name] = getattr(self, color_name)
        color_state.update(kwargs)
        return type(self)(**color_state)

    @classmethod
    def no_colors(cls) -> Self:
        color_state: dict[str, str] = {}
        for color_name in cls.__dataclass_fields__:
            color_state[color_name] = ""
        return cls(**color_state)

    def __getitem__(self, key: str) -> str:
        return self._name_to_value(key)

    def __len__(self) -> int:
        return len(self.__dataclass_fields__)

    def __iter__(self) -> Iterator[str]:
        return iter(self.__dataclass_fields__)


@dataclass(frozen=True, kw_only=True)
class Argparse(ThemeSection):
    usage: str = ANSIColors.BOLD_BLUE
    prog: str = ANSIColors.BOLD_MAGENTA
    prog_extra: str = ANSIColors.MAGENTA
    heading: str = ANSIColors.BOLD_BLUE
    summary_long_option: str = ANSIColors.CYAN
    summary_short_option: str = ANSIColors.GREEN
    summary_label: str = ANSIColors.YELLOW
    summary_action: str = ANSIColors.GREEN
    long_option: str = ANSIColors.BOLD_CYAN
    short_option: str = ANSIColors.BOLD_GREEN
    label: str = ANSIColors.BOLD_YELLOW
    action: str = ANSIColors.BOLD_GREEN
    reset: str = ANSIColors.RESET


@dataclass(frozen=True)
class Syntax(ThemeSection):
    prompt: str = ANSIColors.BOLD_MAGENTA
    keyword: str = ANSIColors.BOLD_BLUE
    keyword_constant: str = ANSIColors.BOLD_BLUE
    builtin: str = ANSIColors.CYAN
    comment: str = ANSIColors.RED
    string: str = ANSIColors.GREEN
    number: str = ANSIColors.YELLOW
    op: str = ANSIColors.RESET
    definition: str = ANSIColors.BOLD
    soft_keyword: str = ANSIColors.BOLD_BLUE
    reset: str = ANSIColors.RESET


@dataclass(frozen=True)
class Traceback(ThemeSection):
    type: str = ANSIColors.BOLD_MAGENTA
    message: str = ANSIColors.MAGENTA
    filename: str = ANSIColors.MAGENTA
    line_no: str = ANSIColors.MAGENTA
    frame: str = ANSIColors.MAGENTA
    error_highlight: str = ANSIColors.BOLD_RED
    error_range: str = ANSIColors.RED
    reset: str = ANSIColors.RESET


@dataclass(frozen=True)
class Unittest(ThemeSection):
    passed: str = ANSIColors.GREEN
    warn: str = ANSIColors.YELLOW
    fail: str = ANSIColors.RED
    fail_info: str = ANSIColors.BOLD_RED
    reset: str = ANSIColors.RESET


@dataclass(frozen=True)
class Theme:
    """A suite of themes for all sections of Python.

    When adding a new one, remember to also modify `copy_with` and `no_colors`
    below.
    """
    argparse: Argparse = field(default_factory=Argparse)
    syntax: Syntax = field(default_factory=Syntax)
    traceback: Traceback = field(default_factory=Traceback)
    unittest: Unittest = field(default_factory=Unittest)

    def copy_with(
        self,
        *,
        argparse: Argparse | None = None,
        syntax: Syntax | None = None,
        traceback: Traceback | None = None,
        unittest: Unittest | None = None,
    ) -> Self:
        """Return a new Theme based on this instance with some sections replaced.

        Themes are immutable to protect against accidental modifications that
        could lead to invalid terminal states.
        """
        return type(self)(
            argparse=argparse or self.argparse,
            syntax=syntax or self.syntax,
            traceback=traceback or self.traceback,
            unittest=unittest or self.unittest,
        )

    @classmethod
    def no_colors(cls) -> Self:
        """Return a new Theme where colors in all sections are empty strings.

        This allows writing user code as if colors are always used. The color
        fields will be ANSI color code strings when colorization is desired
        and possible, and empty strings otherwise.
        """
        return cls(
            argparse=Argparse.no_colors(),
            syntax=Syntax.no_colors(),
            traceback=Traceback.no_colors(),
            unittest=Unittest.no_colors(),
        )


def get_colors(
    colorize: bool = False, *, file: IO[str] | IO[bytes] | None = None
) -> ANSIColors:
    if colorize or can_colorize(file=file):
        return ANSIColors()
    else:
        return NoColors


def decolor(text: str) -> str:
    """Remove ANSI color codes from a string."""
    for code in ColorCodes:
        text = text.replace(code, "")
    return text


def can_colorize(*, file: IO[str] | IO[bytes] | None = None) -> bool:

    def _safe_getenv(k: str, fallback: str | None = None) -> str | None:
        """Exception-safe environment retrieval. See gh-128636."""
        try:
            return os.environ.get(k, fallback)
        except Exception:
            return fallback

    if file is None:
        file = sys.stdout

    if not sys.flags.ignore_environment:
        if _safe_getenv("PYTHON_COLORS") == "0":
            return False
        if _safe_getenv("PYTHON_COLORS") == "1":
            return True
    if _safe_getenv("NO_COLOR"):
        return False
    if not COLORIZE:
        return False
    if _safe_getenv("FORCE_COLOR"):
        return True
    if _safe_getenv("TERM") == "dumb":
        return False

    if not hasattr(file, "fileno"):
        return False

    if sys.platform == "win32":
        try:
            import nt

            if not nt._supports_virtual_terminal():
                return False
        except (ImportError, AttributeError):
            return False

    try:
        return os.isatty(file.fileno())
    except OSError:
        return hasattr(file, "isatty") and file.isatty()


default_theme = Theme()
theme_no_color = default_theme.no_colors()


def get_theme(
    *,
    tty_file: IO[str] | IO[bytes] | None = None,
    force_color: bool = False,
    force_no_color: bool = False,
) -> Theme:
    """Returns the currently set theme, potentially in a zero-color variant.

    In cases where colorizing is not possible (see `can_colorize`), the returned
    theme contains all empty strings in all color definitions.
    See `Theme.no_colors()` for more information.

    It is recommended not to cache the result of this function for extended
    periods of time because the user might influence theme selection by
    the interactive shell, a debugger, or application-specific code. The
    environment (including environment variable state and console configuration
    on Windows) can also change in the course of the application life cycle.
    """
    if force_color or (not force_no_color and
                       can_colorize(file=tty_file)):
        return _theme
    return theme_no_color


def set_theme(t: Theme) -> None:
    global _theme

    if not isinstance(t, Theme):
        raise ValueError(f"Expected Theme object, found {t}")

    _theme = t


set_theme(default_theme)
