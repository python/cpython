import os
import sys

COLORIZE = True


# types
if False:
    from collections.abc import Iterator
    from typing import IO, Literal, Self, ClassVar
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


class CursesColors:
    """Curses color constants for terminal UI theming."""
    BLACK = 0
    RED = 1
    GREEN = 2
    YELLOW = 3
    BLUE = 4
    MAGENTA = 5
    CYAN = 6
    WHITE = 7
    DEFAULT = -1


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


class ThemeSection:
    """A mixin/base class for theme sections.

    It enables dictionary access to a section, as well as implements convenience
    methods.
    """

    # The type below is just that: a type to inform the type checker that the
    # mixin will work in context of those fields existing
    _fields: ClassVar[tuple[str, ...]]

    def copy_with(self, **kwargs: str) -> Self:
        color_state: dict[str, str] = {}
        for color_name in self._fields:
            color_state[color_name] = getattr(self, color_name)
        color_state.update(kwargs)
        return type(self)(**color_state)

    @classmethod
    def no_colors(cls) -> Self:
        color_state: dict[str, str] = {}
        for color_name in cls._fields:
            color_state[color_name] = ""
        return cls(**color_state)

    # Mapping protocol implementation

    def __getitem__(self, key: str) -> str:
        if key in self._fields:
            return getattr(self, key)  # type: ignore[no-any-return]
        raise KeyError(key)

    def __len__(self) -> int:
        return len(self._fields)

    def __iter__(self) -> Iterator[str]:
        return iter(self._fields)

    def __contains__(self, key: object) -> bool:
        return key in self._fields

    def keys(self) -> tuple[str, ...]:
        return self._fields

    def values(self) -> tuple[str, ...]:
        return tuple(getattr(self, f) for f in self._fields)

    def items(self) -> tuple[tuple[str, str], ...]:
        return tuple((f, getattr(self, f)) for f in self._fields)

    def get(self, key: str, default: str | None = None) -> str | None:
        if key in self._fields:
            return getattr(self, key)  # type: ignore[no-any-return]
        return default

    def __repr__(self) -> str:
        fields = ", ".join(f"{f}={getattr(self, f)!r}" for f in self._fields)
        return f"{type(self).__name__}({fields})"

    def __eq__(self, other: object) -> bool:
        if type(self) is not type(other):
            return NotImplemented
        return all(getattr(self, f) == getattr(other, f) for f in self._fields)

    def __hash__(self) -> int:
        return hash(tuple(getattr(self, f) for f in self._fields))


class Argparse(ThemeSection):
    _fields = (
        "usage",
        "prog",
        "prog_extra",
        "heading",
        "summary_long_option",
        "summary_short_option",
        "summary_label",
        "summary_action",
        "long_option",
        "short_option",
        "label",
        "action",
        "default",
        "interpolated_value",
        "reset",
        "error",
        "warning",
        "message",
    )

    def __init__(
        self,
        *,
        usage: str = ANSIColors.BOLD_BLUE,
        prog: str = ANSIColors.BOLD_MAGENTA,
        prog_extra: str = ANSIColors.MAGENTA,
        heading: str = ANSIColors.BOLD_BLUE,
        summary_long_option: str = ANSIColors.CYAN,
        summary_short_option: str = ANSIColors.GREEN,
        summary_label: str = ANSIColors.YELLOW,
        summary_action: str = ANSIColors.GREEN,
        long_option: str = ANSIColors.BOLD_CYAN,
        short_option: str = ANSIColors.BOLD_GREEN,
        label: str = ANSIColors.BOLD_YELLOW,
        action: str = ANSIColors.BOLD_GREEN,
        default: str = ANSIColors.GREY,
        interpolated_value: str = ANSIColors.YELLOW,
        reset: str = ANSIColors.RESET,
        error: str = ANSIColors.BOLD_MAGENTA,
        warning: str = ANSIColors.BOLD_YELLOW,
        message: str = ANSIColors.MAGENTA,
    ):
        self.usage = usage
        self.prog = prog
        self.prog_extra = prog_extra
        self.heading = heading
        self.summary_long_option = summary_long_option
        self.summary_short_option = summary_short_option
        self.summary_label = summary_label
        self.summary_action = summary_action
        self.long_option = long_option
        self.short_option = short_option
        self.label = label
        self.action = action
        self.default = default
        self.interpolated_value = interpolated_value
        self.reset = reset
        self.error = error
        self.warning = warning
        self.message = message


class Difflib(ThemeSection):
    """A 'git diff'-like theme for `difflib.unified_diff`."""

    _fields = ("added", "context", "header", "hunk", "removed", "reset")

    def __init__(
        self,
        *,
        added: str = ANSIColors.GREEN,
        context: str = ANSIColors.RESET,  # context lines
        header: str = ANSIColors.BOLD,  # eg "---" and "+++" lines
        hunk: str = ANSIColors.CYAN,  # the "@@" lines
        removed: str = ANSIColors.RED,
        reset: str = ANSIColors.RESET,
    ):
        self.added = added
        self.context = context
        self.header = header
        self.hunk = hunk
        self.removed = removed
        self.reset = reset


class LiveProfiler(ThemeSection):
    """Theme section for the live profiling TUI (Tachyon profiler).

    Colors use CursesColors constants (BLACK, RED, GREEN, YELLOW,
    BLUE, MAGENTA, CYAN, WHITE, DEFAULT).
    """

    _fields = (
        "title_fg",
        "title_bg",
        "pid_fg",
        "uptime_fg",
        "time_fg",
        "interval_fg",
        "thread_all_fg",
        "thread_single_fg",
        "bar_good_fg",
        "bar_bad_fg",
        "on_gil_fg",
        "off_gil_fg",
        "waiting_gil_fg",
        "gc_fg",
        "func_total_fg",
        "func_exec_fg",
        "func_stack_fg",
        "func_shown_fg",
        "sorted_header_fg",
        "sorted_header_bg",
        "normal_header_fg",
        "normal_header_bg",
        "samples_fg",
        "file_fg",
        "func_fg",
        "trend_up_fg",
        "trend_down_fg",
        "medal_gold_fg",
        "medal_silver_fg",
        "medal_bronze_fg",
        "background_style",
    )

    def __init__(
        self,
        *,
        # Header colors
        title_fg: int = CursesColors.CYAN,
        title_bg: int = CursesColors.DEFAULT,
        # Status display colors
        pid_fg: int = CursesColors.CYAN,
        uptime_fg: int = CursesColors.GREEN,
        time_fg: int = CursesColors.YELLOW,
        interval_fg: int = CursesColors.MAGENTA,
        # Thread view colors
        thread_all_fg: int = CursesColors.GREEN,
        thread_single_fg: int = CursesColors.MAGENTA,
        # Progress bar colors
        bar_good_fg: int = CursesColors.GREEN,
        bar_bad_fg: int = CursesColors.RED,
        # Stats colors
        on_gil_fg: int = CursesColors.GREEN,
        off_gil_fg: int = CursesColors.RED,
        waiting_gil_fg: int = CursesColors.YELLOW,
        gc_fg: int = CursesColors.MAGENTA,
        # Function display colors
        func_total_fg: int = CursesColors.CYAN,
        func_exec_fg: int = CursesColors.GREEN,
        func_stack_fg: int = CursesColors.YELLOW,
        func_shown_fg: int = CursesColors.MAGENTA,
        # Table header colors (for sorted column highlight)
        sorted_header_fg: int = CursesColors.BLACK,
        sorted_header_bg: int = CursesColors.CYAN,
        # Normal header colors (non-sorted columns) - use reverse video style
        normal_header_fg: int = CursesColors.BLACK,
        normal_header_bg: int = CursesColors.WHITE,
        # Data row colors
        samples_fg: int = CursesColors.CYAN,
        file_fg: int = CursesColors.GREEN,
        func_fg: int = CursesColors.YELLOW,
        # Trend indicator colors
        trend_up_fg: int = CursesColors.GREEN,
        trend_down_fg: int = CursesColors.RED,
        # Medal colors for top functions
        medal_gold_fg: int = CursesColors.RED,
        medal_silver_fg: int = CursesColors.YELLOW,
        medal_bronze_fg: int = CursesColors.GREEN,
        # Background style: 'dark' or 'light'
        background_style: Literal["dark", "light"] = "dark",
    ):
        self.title_fg = title_fg
        self.title_bg = title_bg
        self.pid_fg = pid_fg
        self.uptime_fg = uptime_fg
        self.time_fg = time_fg
        self.interval_fg = interval_fg
        self.thread_all_fg = thread_all_fg
        self.thread_single_fg = thread_single_fg
        self.bar_good_fg = bar_good_fg
        self.bar_bad_fg = bar_bad_fg
        self.on_gil_fg = on_gil_fg
        self.off_gil_fg = off_gil_fg
        self.waiting_gil_fg = waiting_gil_fg
        self.gc_fg = gc_fg
        self.func_total_fg = func_total_fg
        self.func_exec_fg = func_exec_fg
        self.func_stack_fg = func_stack_fg
        self.func_shown_fg = func_shown_fg
        self.sorted_header_fg = sorted_header_fg
        self.sorted_header_bg = sorted_header_bg
        self.normal_header_fg = normal_header_fg
        self.normal_header_bg = normal_header_bg
        self.samples_fg = samples_fg
        self.file_fg = file_fg
        self.func_fg = func_fg
        self.trend_up_fg = trend_up_fg
        self.trend_down_fg = trend_down_fg
        self.medal_gold_fg = medal_gold_fg
        self.medal_silver_fg = medal_silver_fg
        self.medal_bronze_fg = medal_bronze_fg
        self.background_style = background_style


LiveProfilerLight = LiveProfiler(
    # Header colors
    title_fg=CursesColors.BLUE,  # Blue is more readable than cyan on light bg

    # Status display colors - darker colors for light backgrounds
    pid_fg=CursesColors.BLUE,
    uptime_fg=CursesColors.BLACK,
    time_fg=CursesColors.BLACK,
    interval_fg=CursesColors.BLUE,

    # Thread view colors
    thread_all_fg=CursesColors.BLACK,
    thread_single_fg=CursesColors.BLUE,

    # Stats colors
    waiting_gil_fg=CursesColors.RED,
    gc_fg=CursesColors.BLUE,

    # Function display colors
    func_total_fg=CursesColors.BLUE,
    func_exec_fg=CursesColors.BLACK,
    func_stack_fg=CursesColors.BLACK,
    func_shown_fg=CursesColors.BLUE,

    # Table header colors (for sorted column highlight)
    sorted_header_fg=CursesColors.WHITE,
    sorted_header_bg=CursesColors.BLUE,

    # Normal header colors (non-sorted columns)
    normal_header_fg=CursesColors.WHITE,
    normal_header_bg=CursesColors.BLACK,

    # Data row colors - use dark colors readable on white
    samples_fg=CursesColors.BLACK,
    file_fg=CursesColors.BLACK,
    func_fg=CursesColors.BLUE,  # Blue is more readable than magenta on light bg

    # Medal colors for top functions
    medal_silver_fg=CursesColors.BLUE,

    # Background style
    background_style="light",
)


class Syntax(ThemeSection):
    _fields = (
        "prompt",
        "keyword",
        "keyword_constant",
        "builtin",
        "comment",
        "string",
        "number",
        "op",
        "definition",
        "soft_keyword",
        "reset",
    )

    def __init__(
        self,
        *,
        prompt: str = ANSIColors.BOLD_MAGENTA,
        keyword: str = ANSIColors.BOLD_BLUE,
        keyword_constant: str = ANSIColors.BOLD_BLUE,
        builtin: str = ANSIColors.CYAN,
        comment: str = ANSIColors.RED,
        string: str = ANSIColors.GREEN,
        number: str = ANSIColors.YELLOW,
        op: str = ANSIColors.RESET,
        definition: str = ANSIColors.BOLD,
        soft_keyword: str = ANSIColors.BOLD_BLUE,
        reset: str = ANSIColors.RESET,
    ):
        self.prompt = prompt
        self.keyword = keyword
        self.keyword_constant = keyword_constant
        self.builtin = builtin
        self.comment = comment
        self.string = string
        self.number = number
        self.op = op
        self.definition = definition
        self.soft_keyword = soft_keyword
        self.reset = reset


class Traceback(ThemeSection):
    _fields = (
        "type",
        "message",
        "filename",
        "line_no",
        "frame",
        "error_highlight",
        "error_range",
        "reset",
    )

    def __init__(
        self,
        *,
        type: str = ANSIColors.BOLD_MAGENTA,
        message: str = ANSIColors.MAGENTA,
        filename: str = ANSIColors.MAGENTA,
        line_no: str = ANSIColors.MAGENTA,
        frame: str = ANSIColors.MAGENTA,
        error_highlight: str = ANSIColors.BOLD_RED,
        error_range: str = ANSIColors.RED,
        reset: str = ANSIColors.RESET,
    ):
        self.type = type
        self.message = message
        self.filename = filename
        self.line_no = line_no
        self.frame = frame
        self.error_highlight = error_highlight
        self.error_range = error_range
        self.reset = reset


class Unittest(ThemeSection):
    _fields = ("passed", "warn", "fail", "fail_info", "reset")

    def __init__(
        self,
        *,
        passed: str = ANSIColors.GREEN,
        warn: str = ANSIColors.YELLOW,
        fail: str = ANSIColors.RED,
        fail_info: str = ANSIColors.BOLD_RED,
        reset: str = ANSIColors.RESET,
    ):
        self.passed = passed
        self.warn = warn
        self.fail = fail
        self.fail_info = fail_info
        self.reset = reset


class Theme:
    """A suite of themes for all sections of Python.

    When adding a new one, remember to also modify `copy_with` and `no_colors`
    below.
    """

    _fields = (
        "argparse",
        "difflib",
        "live_profiler",
        "syntax",
        "traceback",
        "unittest",
    )

    def __init__(
        self,
        *,
        argparse: Argparse | None = None,
        difflib: Difflib | None = None,
        live_profiler: LiveProfiler | None = None,
        syntax: Syntax | None = None,
        traceback: Traceback | None = None,
        unittest: Unittest | None = None,
    ):
        self.argparse = argparse if argparse is not None else Argparse()
        self.difflib = difflib if difflib is not None else Difflib()
        self.live_profiler = (
            live_profiler if live_profiler is not None else LiveProfiler()
        )
        self.syntax = syntax if syntax is not None else Syntax()
        self.traceback = traceback if traceback is not None else Traceback()
        self.unittest = unittest if unittest is not None else Unittest()

    def copy_with(
        self,
        *,
        argparse: Argparse | None = None,
        difflib: Difflib | None = None,
        live_profiler: LiveProfiler | None = None,
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
            difflib=difflib or self.difflib,
            live_profiler=live_profiler or self.live_profiler,
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
            difflib=Difflib.no_colors(),
            live_profiler=LiveProfiler.no_colors(),
            syntax=Syntax.no_colors(),
            traceback=Traceback.no_colors(),
            unittest=Unittest.no_colors(),
        )

    def __repr__(self) -> str:
        fields = ", ".join(f"{f}={getattr(self, f)!r}" for f in self._fields)
        return f"{type(self).__name__}({fields})"

    def __eq__(self, other: object) -> bool:
        if type(self) is not type(other):
            return NotImplemented
        return all(getattr(self, f) == getattr(other, f) for f in self._fields)

    def __hash__(self) -> int:
        return hash(tuple(getattr(self, f) for f in self._fields))


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

# Convenience theme with light profiler colors (for white/light terminal backgrounds)
light_profiler_theme = default_theme.copy_with(live_profiler=LiveProfilerLight)


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
