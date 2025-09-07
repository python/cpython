"""Message protocol for sampling daemon communication."""

from dataclasses import dataclass
from typing import Any, get_args

@dataclass
class StartCommand:
    pass

@dataclass
class StatsCommand:
    req_id: Any | None = None
    limit: int | None = None
    sort: int | None = None
    show_summary: bool | None = None

@dataclass
class ExportCommand:
    filename: str
    req_id: Any | None = None

@dataclass
class ShutdownCommand:
    pass

@dataclass
class AckResponse:
    cmd: str
    success: bool = True

@dataclass
class StatsResponse:
    text: str
    req_id: Any | None = None


@dataclass
class ErrorResponse:
    msg: str
    req_id: Any | None = None

CommandMessage = StartCommand | StatsCommand | ExportCommand | ShutdownCommand
ResponseMessage = AckResponse | StatsResponse | ErrorResponse
SamplingMessage = CommandMessage | ResponseMessage
COMMAND_TYPES = get_args(CommandMessage)
RESPONSE_TYPES = get_args(ResponseMessage)

def is_profiler_command(msg) -> bool:
    return isinstance(msg, COMMAND_TYPES)

def is_profiler_response(msg) -> bool:
    return isinstance(msg, RESPONSE_TYPES)
