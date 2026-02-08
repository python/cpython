"""
Vi-specific commands for pyrepl.
"""

from .commands import Command, MotionCommand, KillCommand, delete
from . import input as _input
from .types import ViFindDirection
from .trace import trace
from .vi_motions import (
    vi_eow,
    vi_forward_word as _vi_forward_word,
    vi_forward_word_ws as _vi_forward_word_ws,
    vi_bow,
    vi_bow_ws,
    find_char_forward,
    find_char_backward,
)


class ViKillCommand(KillCommand):
    """Base class for Vi kill commands that modify the buffer."""
    modifies_buffer = True


# ============================================================================
# Vi-specific Motion Commands
# ============================================================================

class first_non_whitespace_character(MotionCommand):
    def do(self) -> None:
        self.reader.pos = self.reader.first_non_whitespace()


class end_of_word(MotionCommand):
    def do(self) -> None:
        r = self.reader
        for _ in range(r.get_arg()):
            r.pos = vi_eow(r.buffer, r.pos)


class vi_forward_word(MotionCommand):
    def do(self) -> None:
        r = self.reader
        for _ in range(r.get_arg()):
            r.pos = _vi_forward_word(r.buffer, r.pos)

class vi_forward_word_ws(MotionCommand):
    def do(self) -> None:
        r = self.reader
        for _ in range(r.get_arg()):
            r.pos = _vi_forward_word_ws(r.buffer, r.pos)

class vi_backward_word(MotionCommand):
    def do(self) -> None:
        r = self.reader
        for _ in range(r.get_arg()):
            r.pos = vi_bow(r.buffer, r.pos)


class vi_backward_word_ws(MotionCommand):
    def do(self) -> None:
        r = self.reader
        for _ in range(r.get_arg()):
            r.pos = vi_bow_ws(r.buffer, r.pos)



# ============================================================================
# Mode Switching Commands
# ============================================================================

class vi_normal_mode(Command):
    def do(self) -> None:
        self.reader.enter_normal_mode()


class vi_insert_mode(Command):
    def do(self) -> None:
        self.reader.enter_insert_mode()


class vi_append_mode(Command):
    def do(self) -> None:
        if self.reader.pos < len(self.reader.buffer):
            self.reader.pos += 1
        self.reader.enter_insert_mode()


class vi_append_eol(Command):
    def do(self) -> None:
        while self.reader.pos < len(self.reader.buffer):
            if self.reader.buffer[self.reader.pos] == '\n':
                break
            self.reader.pos += 1
        self.reader.enter_insert_mode()


class vi_insert_bol(Command):
    def do(self) -> None:
        self.reader.pos = self.reader.first_non_whitespace()
        self.reader.enter_insert_mode()


class vi_open_below(Command):
    def do(self) -> None:
        while self.reader.pos < len(self.reader.buffer):
            if self.reader.buffer[self.reader.pos] == '\n':
                break
            self.reader.pos += 1

        self.reader.insert('\n')
        self.reader.enter_insert_mode()


class vi_open_above(Command):
    def do(self) -> None:
        while self.reader.pos > 0 and self.reader.buffer[self.reader.pos - 1] != '\n':
            self.reader.pos -= 1

        self.reader.insert('\n')
        self.reader.pos -= 1
        self.reader.enter_insert_mode()


# ============================================================================
# Delete Commands
# ============================================================================

class vi_delete(delete):
    """Delete character under cursor (x)."""
    modifies_buffer = True


class vi_delete_word(ViKillCommand):
    """Delete from cursor to start of next word (dw)."""
    def do(self) -> None:
        r = self.reader
        for _ in range(r.get_arg()):
            end = _vi_forward_word(r.buffer, r.pos)
            if end > r.pos:
                self.kill_range(r.pos, end)


class vi_delete_line(ViKillCommand):
    """Delete entire line content (dd)."""
    def do(self) -> None:
        r = self.reader
        self.kill_range(r.bol(), r.eol())


class vi_delete_to_bol(ViKillCommand):
    """Delete from cursor to beginning of line (d0)."""
    def do(self) -> None:
        r = self.reader
        self.kill_range(r.bol(), r.pos)


class vi_delete_to_eol(ViKillCommand):
    """Delete from cursor to end of line (d$ or D)."""
    def do(self) -> None:
        r = self.reader
        self.kill_range(r.pos, r.eol())


class vi_delete_char_before(ViKillCommand):
    """Delete character before cursor (X)."""
    def do(self) -> None:
        r = self.reader
        for _ in range(r.get_arg()):
            if r.pos > 0:
                self.kill_range(r.pos - 1, r.pos)


# ============================================================================
# Find Commands (f/F/t/T)
# ============================================================================

def _make_char_capture_keymap(execute_cmd: str, cancel_cmd: str) -> tuple[tuple[str, str], ...]:
    """Create a keymap where all printable ASCII maps to execute_cmd.

    Once execute_cmd is called, it will pop our input translator."""
    entries = []
    for i in range(32, 127):
        if i == 92:  # backslash needs escaping
            entries.append((r"\\", execute_cmd))
        else:
            entries.append((chr(i), execute_cmd))
    entries.append((r"\<escape>", cancel_cmd))
    return tuple(entries)

_find_char_keymap = _make_char_capture_keymap("vi-find-execute", "vi-find-cancel")
_replace_char_keymap = _make_char_capture_keymap("vi-replace-execute", "vi-replace-cancel")


class vi_find_char(Command):
    """Start forward find (f). Waits for target character."""
    def do(self) -> None:
        r = self.reader
        r.vi_find.pending_direction = ViFindDirection.FORWARD
        r.vi_find.pending_inclusive = True
        trans = _input.KeymapTranslator(
            _find_char_keymap,
            invalid_cls="vi-find-cancel",
            character_cls="vi-find-execute"
        )
        r.push_input_trans(trans)


class vi_find_char_back(Command):
    """Start backward find (F). Waits for target character."""
    def do(self) -> None:
        r = self.reader
        r.vi_find.pending_direction = ViFindDirection.BACKWARD
        r.vi_find.pending_inclusive = True
        trans = _input.KeymapTranslator(
            _find_char_keymap,
            invalid_cls="vi-find-cancel",
            character_cls="vi-find-execute"
        )
        r.push_input_trans(trans)


class vi_till_char(Command):
    """Start forward till (t). Waits for target character."""
    def do(self) -> None:
        r = self.reader
        r.vi_find.pending_direction = ViFindDirection.FORWARD
        r.vi_find.pending_inclusive = False
        trans = _input.KeymapTranslator(
            _find_char_keymap,
            invalid_cls="vi-find-cancel",
            character_cls="vi-find-execute"
        )
        r.push_input_trans(trans)


class vi_till_char_back(Command):
    """Start backward till (T). Waits for target character."""
    def do(self) -> None:
        r = self.reader
        r.vi_find.pending_direction = ViFindDirection.BACKWARD
        r.vi_find.pending_inclusive = False
        trans = _input.KeymapTranslator(
            _find_char_keymap,
            invalid_cls="vi-find-cancel",
            character_cls="vi-find-execute"
        )
        r.push_input_trans(trans)


class vi_find_execute(MotionCommand):
    """Execute the pending find with the pressed character."""
    def do(self) -> None:
        r = self.reader
        r.pop_input_trans()

        char = self.event[-1]
        if not char:
            return

        direction = r.vi_find.pending_direction
        inclusive = r.vi_find.pending_inclusive

        # Store for repeat with ; and ,
        r.vi_find.last_char = char
        r.vi_find.last_direction = direction
        r.vi_find.last_inclusive = inclusive

        r.vi_find.pending_direction = None
        self._execute_find(char, direction, inclusive)

    def _execute_find(self, char: str, direction: ViFindDirection | None, inclusive: bool) -> None:
        r = self.reader
        for _ in range(r.get_arg()):
            if direction == ViFindDirection.FORWARD:
                new_pos = find_char_forward(r.buffer, r.pos, char)
                if new_pos is not None:
                    if not inclusive:
                        new_pos -= 1
                    if new_pos > r.pos:
                        r.pos = new_pos
            else:
                new_pos = find_char_backward(r.buffer, r.pos, char)
                if new_pos is not None:
                    if not inclusive:
                        new_pos += 1
                    if new_pos < r.pos:
                        r.pos = new_pos


class vi_find_cancel(Command):
    """Cancel pending find operation."""
    def do(self) -> None:
        r = self.reader
        r.pop_input_trans()
        r.vi_find.pending_direction = None


# ============================================================================
# Repeat Find Commands (; and ,)
# ============================================================================

class vi_repeat_find(MotionCommand):
    """Repeat last f/F/t/T in the same direction (;)."""
    def do(self) -> None:
        r = self.reader
        if r.vi_find.last_char is None:
            return

        char = r.vi_find.last_char
        direction = r.vi_find.last_direction
        inclusive = r.vi_find.last_inclusive

        for _ in range(r.get_arg()):
            if direction == ViFindDirection.FORWARD:
                new_pos = find_char_forward(r.buffer, r.pos, char)
                if new_pos is not None:
                    if not inclusive:
                        new_pos -= 1
                    if new_pos > r.pos:
                        r.pos = new_pos
            else:
                new_pos = find_char_backward(r.buffer, r.pos, char)
                if new_pos is not None:
                    if not inclusive:
                        new_pos += 1
                    if new_pos < r.pos:
                        r.pos = new_pos


class vi_repeat_find_opposite(MotionCommand):
    """Repeat last f/F/t/T in opposite direction (,)."""
    def do(self) -> None:
        r = self.reader
        if r.vi_find.last_char is None:
            return

        char = r.vi_find.last_char
        direction = (ViFindDirection.BACKWARD
                     if r.vi_find.last_direction == ViFindDirection.FORWARD
                     else ViFindDirection.FORWARD)
        inclusive = r.vi_find.last_inclusive

        for _ in range(r.get_arg()):
            if direction == ViFindDirection.FORWARD:
                new_pos = find_char_forward(r.buffer, r.pos, char)
                if new_pos is not None:
                    if not inclusive:
                        new_pos -= 1
                    if new_pos > r.pos:
                        r.pos = new_pos
            else:
                new_pos = find_char_backward(r.buffer, r.pos, char)
                if new_pos is not None:
                    if not inclusive:
                        new_pos += 1
                    if new_pos < r.pos:
                        r.pos = new_pos


# ============================================================================
# Change Commands
# ============================================================================

class vi_change_word(ViKillCommand):
    """Change from cursor to end of word (cw)."""
    def do(self) -> None:
        r = self.reader
        for _ in range(r.get_arg()):
            end = vi_eow(r.buffer, r.pos) + 1  # +1 to include last char
            if end > r.pos:
                self.kill_range(r.pos, end)
        r.enter_insert_mode()


class vi_change_to_eol(ViKillCommand):
    """Change from cursor to end of line (C)."""
    def do(self) -> None:
        r = self.reader
        self.kill_range(r.pos, r.eol())
        r.enter_insert_mode()


class vi_substitute_char(ViKillCommand):
    """Delete character under cursor and enter insert mode (s)."""
    def do(self) -> None:
        r = self.reader
        for _ in range(r.get_arg()):
            if r.pos < len(r.buffer):
                self.kill_range(r.pos, r.pos + 1)
        r.enter_insert_mode()


# ============================================================================
# Replace Commands
# ============================================================================

class vi_replace_char(Command):
    """Replace character under cursor with next typed character (r)."""
    def do(self) -> None:
        r = self.reader
        translator = _input.KeymapTranslator(
            _replace_char_keymap,
            invalid_cls="vi-replace-cancel",
            character_cls="vi-replace-execute"
        )
        r.push_input_trans(translator)

class vi_replace_execute(Command):
    """Execute character replacement with the pressed character."""
    modifies_buffer = True

    def do(self) -> None:
        r = self.reader
        r.pop_input_trans()
        pending_char = self.event[-1]
        if not pending_char or r.pos >= len(r.buffer):
            return
        r.buffer[r.pos] = pending_char
        r.dirty = True

class vi_replace_cancel(Command):
    """Cancel pending replace operation."""
    def do(self) -> None:
        r = self.reader
        r.pop_input_trans()


# ============================================================================
# Undo Commands
# ============================================================================

class vi_undo(Command):
    """Undo last change (u)."""
    def do(self) -> None:
        r = self.reader
        trace("vi_undo: undo_stack size =", len(r.undo_stack))
        if not r.undo_stack:
            return
        state = r.undo_stack.pop()
        r.buffer[:] = state.buffer_snapshot
        r.pos = state.pos_snapshot
        r.dirty = True
