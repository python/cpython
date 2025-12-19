from collections.abc import Callable, Iterator

type Callback = Callable[[], object]
type SimpleContextManager = Iterator[None]
type KeySpec = str  # like r"\C-c"
type CommandName = str  # like "interrupt"
type EventTuple = tuple[CommandName, str]
type Completer = Callable[[str, int], str | None]
type CharBuffer = list[str]
type CharWidths = list[int]
