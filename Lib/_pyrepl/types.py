from typing import Any, Callable, Iterator

Callback = Callable[[], Any]
SimpleContextManager = Iterator[None]
KeySpec = str  # like r"\C-c"
CommandName = str  # like "interrupt"
