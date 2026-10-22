"""Pretty printing for one-shot sampling stack dumps."""

import contextlib
import linecache
import os
import sys
from traceback import _byte_offset_to_character_offset

import _colorize

from .collector import extract_lineno, filter_internal_frames, iter_async_frames
from .constants import (
    THREAD_STATUS_GIL_REQUESTED,
    THREAD_STATUS_HAS_EXCEPTION,
    THREAD_STATUS_HAS_GIL,
    THREAD_STATUS_MAIN_THREAD,
    THREAD_STATUS_ON_CPU,
    THREAD_STATUS_UNKNOWN,
)
from .opcode_utils import format_opcode


_STATUS_LABELS = (
    (THREAD_STATUS_MAIN_THREAD, "main thread"),
    (THREAD_STATUS_HAS_GIL, "has GIL"),
    (THREAD_STATUS_ON_CPU, "on CPU"),
    (THREAD_STATUS_GIL_REQUESTED, "waiting for GIL"),
    (THREAD_STATUS_HAS_EXCEPTION, "has exception"),
)


def _theme_for(file, colorize):
    if colorize is True:
        return _colorize.get_theme(force_color=True).profiler_dump
    if colorize is False:
        return _colorize.get_theme(force_no_color=True).profiler_dump
    return _colorize.get_theme(tty_file=file).profiler_dump


def _color(text, color, theme):
    if not color:
        return text
    return f"{color}{text}{theme.reset}"


def _frame_fields(frame):
    if isinstance(frame, tuple):
        filename = frame[0] if len(frame) > 0 else ""
        location = frame[1] if len(frame) > 1 else None
        qualname = frame[2] if len(frame) > 2 else "<unknown>"
        opcode = frame[3] if len(frame) > 3 else None
    else:
        filename = getattr(frame, "filename", "")
        location = getattr(frame, "location", None)
        qualname = getattr(frame, "qualname", None)
        if qualname is None:
            qualname = getattr(frame, "funcname", "<unknown>")
        opcode = getattr(frame, "opcode", None)
    return filename, location, qualname, opcode


def _location_field(location, index, default=None):
    if location is None:
        return default
    try:
        value = location[index]
    except (IndexError, TypeError):
        return default
    return default if value is None else value


def _status_text(status):
    labels = [label for flag, label in _STATUS_LABELS if status & flag]
    has_state = status & (
        THREAD_STATUS_HAS_GIL
        | THREAD_STATUS_GIL_REQUESTED
        | THREAD_STATUS_HAS_EXCEPTION
    )
    if not has_state and not status & (THREAD_STATUS_UNKNOWN | THREAD_STATUS_ON_CPU):
        labels.append("idle")
    return ", ".join(labels) if labels else None


def _is_async_dump(stack_frames):
    return bool(stack_frames) and hasattr(stack_frames[0], "awaited_by")


def _iter_dump_sections(stack_frames):
    if not stack_frames:
        return

    if _is_async_dump(stack_frames):
        for frames, thread_id, leaf_id in iter_async_frames(stack_frames):
            frames = filter_internal_frames(frames)
            if frames:
                yield None, thread_id, None, frames, f"task {leaf_id}"
        return

    for interpreter_info in stack_frames:
        interpreter_id = getattr(interpreter_info, "interpreter_id", None)
        for thread_info in getattr(interpreter_info, "threads", ()):
            frames = getattr(thread_info, "frame_info", None) or []
            frames = filter_internal_frames(frames)
            yield (
                interpreter_id,
                getattr(thread_info, "thread_id", None),
                getattr(thread_info, "status", None),
                frames,
                None,
            )


def _display_filename(filename):
    if not filename or filename == "~":
        return filename
    with contextlib.suppress(ValueError):
        relpath = os.path.relpath(filename)
        if not relpath.startswith(".." + os.sep) and relpath != "..":
            return relpath
    return filename


def _format_frame(frame, theme):
    filename, location, qualname, opcode = _frame_fields(frame)
    source_filename = filename
    lineno = extract_lineno(location)
    qualname_part = _color(qualname, theme.frame, theme)

    if filename == "~" and lineno == 0:
        line = f"  {qualname_part}"
    else:
        filename = _display_filename(filename)
        if filename:
            file_part = _color(f'"{filename}"', theme.filename, theme)
            if lineno > 0:
                line_part = _color(str(lineno), theme.line_no, theme)
                line = f"  File {file_part}, line {line_part}, in {qualname_part}"
            else:
                line = f"  File {file_part}, in {qualname_part}"
        else:
            line = f"  {qualname_part}"

    if opcode is not None:
        line = f"{line}  {_color(f'opcode={format_opcode(opcode)}', theme.opcode, theme)}"

    lines = [line]
    source = _source_line(source_filename, location, lineno, theme)
    if source:
        lines.append(f"    {source}")
    return lines


def _source_offsets(line, location, lineno):
    end_lineno = _location_field(location, 1, lineno)
    col_offset = _location_field(location, 2, -1)
    end_col_offset = _location_field(location, 3, -1)

    if col_offset < 0 or end_col_offset < 0 or end_lineno < lineno:
        return None

    start = _byte_offset_to_character_offset(line, col_offset)
    if end_lineno == lineno:
        end = _byte_offset_to_character_offset(line, end_col_offset)
    else:
        end = len(line)
    if start < 0 or end <= start:
        return None
    return start, end


def _trim_source_line(line, offsets):
    stripped = line.lstrip()
    leading = len(line) - len(stripped)
    if offsets is None:
        return stripped, None

    start, end = offsets
    start = max(start - leading, 0)
    end = max(end - leading, start + 1)
    end = min(end, len(stripped))
    return stripped, (start, end)


def _highlight_source_line(line, offsets, theme):
    if offsets is None or offsets[1] <= offsets[0]:
        return _color(line, theme.source, theme)

    start, end = offsets
    parts = []
    if line[:start]:
        parts.append(_color(line[:start], theme.source, theme))
    parts.append(_color(line[start:end], theme.source_highlight, theme))
    if line[end:]:
        parts.append(_color(line[end:], theme.source, theme))
    return "".join(parts)


def _source_line(filename, location, lineno, theme):
    if not filename or filename == "~" or lineno <= 0:
        return None
    line = linecache.getline(filename, lineno).removesuffix("\n")
    if not line:
        return None

    offsets = _source_offsets(line, location, lineno)
    line, offsets = _trim_source_line(line, offsets)
    if not line:
        return None
    return _highlight_source_line(line, offsets, theme)


def _section_header(
    *,
    pid,
    interpreter_id,
    thread_id,
    status,
    label,
    show_pid,
    show_interpreter,
    theme,
):
    subject = "Stack dump"
    if show_pid and pid is not None:
        subject = f"{subject} for PID {pid}"
    if thread_id is not None:
        subject = f"{subject}, thread {thread_id}"

    details = []
    if show_interpreter and interpreter_id is not None:
        details.append(f"interpreter {interpreter_id}")
    if label:
        details.append(label)
    if status is not None:
        status_text = _status_text(status)
        if status_text:
            details.append(status_text)

    suffix = "most recent call last"
    if details:
        suffix = f"{'; '.join(details)}; {suffix}"
    return _color(f"{subject} ({suffix}):", theme.header, theme)


def format_stack_dump(stack_frames, *, pid=None, file=None, colorize=None):
    """Return a formatted one-shot stack dump."""
    if file is None:
        file = sys.stdout

    theme = _theme_for(file, colorize)
    lines = []
    sections = list(_iter_dump_sections(stack_frames))
    if not sections:
        if pid is None:
            return f"{_color('No Python stacks found', theme.warning, theme)}\n"
        return f"{_color(f'No Python stacks found for PID {pid}', theme.warning, theme)}\n"

    interpreter_ids = {
        interpreter_id
        for interpreter_id, _thread_id, _status, _frames, _label in sections
        if interpreter_id is not None
    }
    show_interpreter = len(interpreter_ids) > 1

    for section_index, (interpreter_id, thread_id, status, frames, label) in enumerate(sections):
        if section_index:
            lines.append("")
        lines.append(
            _section_header(
                pid=pid,
                interpreter_id=interpreter_id,
                thread_id=thread_id,
                status=status,
                label=label,
                show_pid=section_index == 0,
                show_interpreter=show_interpreter,
                theme=theme,
            )
        )

        if not frames:
            lines.append(_color("No Python frames", theme.warning, theme))
            continue

        for frame in reversed(frames):
            lines.extend(_format_frame(frame, theme))

    return "\n".join(lines) + "\n"


def print_stack_dump(stack_frames, *, pid=None, file=None, colorize=None):
    """Pretty-print a one-shot stack dump."""
    if file is None:
        file = sys.stdout
    file.write(format_stack_dump(stack_frames, pid=pid, file=file, colorize=colorize))
