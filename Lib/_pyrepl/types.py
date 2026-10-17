from collections.abc import Callable, Iterator

type Callback = Callable[[], object]
type SimpleContextManager = Iterator[None]
type KeySpec = str  # like r"\C-c"
type CommandName = str  # like "interrupt"
type EventData = list[str]
type EventTuple = tuple[CommandName, EventData]
type CursorXY = tuple[int, int]
type Dimensions = tuple[int, int]
type ScreenInfoRow = tuple[int, list[int]]
type Keymap = tuple[tuple[KeySpec, CommandName], ...]
type Completer = Callable[[str, int], str | None]
type CharBuffer = list[str]
type CharWidths = list[int]
type CompletionAction = tuple[str, Callable[[], str | None]]
