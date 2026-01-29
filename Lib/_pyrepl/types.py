import enum
from collections.abc import Callable, Iterator
from dataclasses import dataclass, field

type Callback = Callable[[], object]
type SimpleContextManager = Iterator[None]
type KeySpec = str  # like r"\C-c"
type CommandName = str  # like "interrupt"
type EventTuple = tuple[CommandName, str]
type Completer = Callable[[str, int], str | None]
type CharBuffer = list[str]
type CharWidths = list[int]


VI_MODE_INSERT = 0
VI_MODE_NORMAL = 1


class ViFindDirection(str, enum.Enum):
    FORWARD = "forward"
    BACKWARD = "backward"


@dataclass
class ViFindState:
    last_char: str | None = None
    last_direction: ViFindDirection | None = None
    last_inclusive: bool = True  # f/F=True, t/T=False
    pending_direction: ViFindDirection | None = None
    pending_inclusive: bool = True

@dataclass
class ViUndoState:
    buffer_snapshot: CharBuffer = field(default_factory=list)
    pos_snapshot: int = 0
