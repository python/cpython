"""
Vi-specific commands for pyrepl.
"""

from .commands import Command, MotionCommand, KillCommand
from . import input as _input
from .types import ViFindDirection


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
            r.pos = r.vi_eow()


class vi_forward_word(MotionCommand):
    def do(self) -> None:
        r = self.reader
        for _ in range(r.get_arg()):
            r.pos = r.vi_forward_word()

class vi_forward_word_ws(MotionCommand):
    def do(self) -> None:
        r = self.reader
        for _ in range(r.get_arg()):
            r.pos = r.vi_forward_word_ws()

class vi_backward_word(MotionCommand):
    def do(self) -> None:
        r = self.reader
        for _ in range(r.get_arg()):
            r.pos = r.vi_bow()


class vi_backward_word_ws(MotionCommand):
    def do(self) -> None:
        r = self.reader
        for _ in range(r.get_arg()):
            r.pos = r.vi_bow_ws()



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

class vi_delete_word(KillCommand):
    """Delete from cursor to start of next word (dw)."""
    def do(self) -> None:
        r = self.reader
        for _ in range(r.get_arg()):
            end = r.vi_forward_word()
            if end > r.pos:
                self.kill_range(r.pos, end)


class vi_delete_line(KillCommand):
    """Delete entire line content (dd)."""
    def do(self) -> None:
        r = self.reader
        self.kill_range(r.bol(), r.eol())


class vi_delete_to_bol(KillCommand):
    """Delete from cursor to beginning of line (d0)."""
    def do(self) -> None:
        r = self.reader
        self.kill_range(r.bol(), r.pos)


class vi_delete_to_eol(KillCommand):
    """Delete from cursor to end of line (d$)."""
    def do(self) -> None:
        r = self.reader
        self.kill_range(r.pos, r.eol())


# ============================================================================
# Find Commands (f/F/t/T)
# ============================================================================

def _make_find_char_keymap() -> tuple[tuple[str, str], ...]:
    """Create a keymap where all printable ASCII maps to vi-find-execute.

    Once vi-find-execute is called, it will pop our input translator."""
    entries = []
    for i in range(32, 127):
        if i == 92:  # backslash needs escaping
            entries.append((r"\\", "vi-find-execute"))
        else:
            entries.append((chr(i), "vi-find-execute"))
    entries.append((r"\<escape>", "vi-find-cancel"))
    return tuple(entries)


_find_char_keymap = _make_find_char_keymap()


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
                new_pos = r.find_char_forward(char)
                if new_pos is not None:
                    if not inclusive:
                        new_pos -= 1
                    if new_pos > r.pos:
                        r.pos = new_pos
            else:
                new_pos = r.find_char_backward(char)
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
                new_pos = r.find_char_forward(char)
                if new_pos is not None:
                    if not inclusive:
                        new_pos -= 1
                    if new_pos > r.pos:
                        r.pos = new_pos
            else:
                new_pos = r.find_char_backward(char)
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
                new_pos = r.find_char_forward(char)
                if new_pos is not None:
                    if not inclusive:
                        new_pos -= 1
                    if new_pos > r.pos:
                        r.pos = new_pos
            else:
                new_pos = r.find_char_backward(char)
                if new_pos is not None:
                    if not inclusive:
                        new_pos += 1
                    if new_pos < r.pos:
                        r.pos = new_pos
