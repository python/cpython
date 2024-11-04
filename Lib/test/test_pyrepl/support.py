import os
from code import InteractiveConsole
from functools import partial
from typing import Iterable
from unittest.mock import MagicMock

from _pyrepl.console import Console, Event
from _pyrepl.readline import ReadlineAlikeReader, ReadlineConfig
from _pyrepl.simple_interact import _strip_final_indent


def multiline_input(reader: ReadlineAlikeReader, namespace: dict | None = None):
    saved = reader.more_lines
    try:
        reader.more_lines = partial(more_lines, namespace=namespace)
        reader.ps1 = reader.ps2 = ">>>"
        reader.ps3 = reader.ps4 = "..."
        return reader.readline()
    finally:
        reader.more_lines = saved
        reader.paste_mode = False


def more_lines(text: str, namespace: dict | None = None):
    if namespace is None:
        namespace = {}
    src = _strip_final_indent(text)
    console = InteractiveConsole(namespace, filename="<stdin>")
    try:
        code = console.compile(src, "<stdin>", "single")
    except (OverflowError, SyntaxError, ValueError):
        return False
    else:
        return code is None


def code_to_events(code: str):
    for c in code:
        yield Event(evt="key", data=c, raw=bytearray(c.encode("utf-8")))


def clean_screen(screen: Iterable[str]):
    """Cleans color and console characters out of a screen output.

    This is useful for screen testing, it increases the test readability since
    it strips out all the unreadable side of the screen.
    """
    output = []
    for line in screen:
        if line.startswith(">>>") or line.startswith("..."):
            line = line[3:]
        output.append(line)
    return "\n".join(output).strip()


def prepare_reader(console: Console, **kwargs):
    config = ReadlineConfig(readline_completer=kwargs.pop("readline_completer", None))
    reader = ReadlineAlikeReader(console=console, config=config)
    reader.more_lines = partial(more_lines, namespace=None)
    reader.paste_mode = True  # Avoid extra indents

    def get_prompt(lineno, cursor_on_line) -> str:
        return ""

    reader.get_prompt = get_prompt  # Remove prompt for easier calculations of (x, y)

    for key, val in kwargs.items():
        setattr(reader, key, val)

    return reader


def prepare_console(events: Iterable[Event], **kwargs) -> MagicMock | Console:
    console = MagicMock()
    console.get_event.side_effect = events
    console.height = 100
    console.width = 80
    for key, val in kwargs.items():
        setattr(console, key, val)
    return console


def handle_all_events(
    events, prepare_console=prepare_console, prepare_reader=prepare_reader
):
    console = prepare_console(events)
    reader = prepare_reader(console)
    try:
        while True:
            reader.handle1()
    except StopIteration:
        pass
    except KeyboardInterrupt:
        pass
    return reader, console


handle_events_narrow_console = partial(
    handle_all_events,
    prepare_console=partial(prepare_console, width=10),
)


def make_clean_env() -> dict[str, str]:
    clean_env = os.environ.copy()
    for k in clean_env.copy():
        if k.startswith("PYTHON"):
            clean_env.pop(k)
    clean_env.pop("FORCE_COLOR", None)
    clean_env.pop("NO_COLOR", None)
    return clean_env


class FakeConsole(Console):
    def __init__(self, events, encoding="utf-8") -> None:
        self.events = iter(events)
        self.encoding = encoding
        self.screen = []
        self.height = 100
        self.width = 80

    def get_event(self, block: bool = True) -> Event | None:
        return next(self.events)

    def getpending(self) -> Event:
        return self.get_event(block=False)

    def getheightwidth(self) -> tuple[int, int]:
        return self.height, self.width

    def refresh(self, screen: list[str], xy: tuple[int, int]) -> None:
        pass

    def prepare(self) -> None:
        pass

    def restore(self) -> None:
        pass

    def move_cursor(self, x: int, y: int) -> None:
        pass

    def set_cursor_vis(self, visible: bool) -> None:
        pass

    def push_char(self, char: int | bytes) -> None:
        pass

    def beep(self) -> None:
        pass

    def clear(self) -> None:
        pass

    def finish(self) -> None:
        pass

    def flushoutput(self) -> None:
        pass

    def forgetinput(self) -> None:
        pass

    def wait(self, timeout: float | None = None) -> bool:
        return True

    def repaint(self) -> None:
        pass
