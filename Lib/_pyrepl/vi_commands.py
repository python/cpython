"""
Vi-specific commands for pyrepl.
"""

from .commands import Command, MotionCommand


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
