#   Copyright 2000-2010 Michael Hudson-Doyle <micahel@gmail.com>
#                       Antonio Cuni
#                       Armin Rigo
#
#                        All Rights Reserved
#
#
# Permission to use, copy, modify, and distribute this software and
# its documentation for any purpose is hereby granted without fee,
# provided that the above copyright notice appear in all copies and
# that both that copyright notice and this permission notice appear in
# supporting documentation.
#
# THE AUTHOR MICHAEL HUDSON DISCLAIMS ALL WARRANTIES WITH REGARD TO
# THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS, IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL,
# INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
# RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
# CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

import os

# Catgories of actions:
#  killing
#  yanking
#  motion
#  editing
#  history
#  finishing
# [completion]


class Command:
    finish = 0
    kills_digit_arg = 1

    def __init__(self, reader, event_name, event):
        self.reader = reader
        self.event = event
        self.event_name = event_name

    def do(self):
        pass


class KillCommand(Command):
    def kill_range(self, start, end):
        if start == end:
            return
        r = self.reader
        b = r.buffer
        text = b[start:end]
        del b[start:end]
        if is_kill(r.last_command):
            if start < r.pos:
                r.kill_ring[-1] = text + r.kill_ring[-1]
            else:
                r.kill_ring[-1] = r.kill_ring[-1] + text
        else:
            r.kill_ring.append(text)
        r.pos = start
        r.dirty = 1


class YankCommand(Command):
    pass


class MotionCommand(Command):
    pass


class EditCommand(Command):
    pass


class FinishCommand(Command):
    finish = 1
    pass


def is_kill(command):
    return command and issubclass(command, KillCommand)


def is_yank(command):
    return command and issubclass(command, YankCommand)


# etc


class digit_arg(Command):
    kills_digit_arg = 0

    def do(self):
        r = self.reader
        c = self.event[-1]
        if c == "-":
            if r.arg is not None:
                r.arg = -r.arg
            else:
                r.arg = -1
        else:
            d = int(c)
            if r.arg is None:
                r.arg = d
            else:
                if r.arg < 0:
                    r.arg = 10 * r.arg - d
                else:
                    r.arg = 10 * r.arg + d
        r.dirty = 1


class clear_screen(Command):
    def do(self):
        r = self.reader
        r.console.clear()
        r.dirty = 1


class refresh(Command):
    def do(self):
        self.reader.dirty = 1


class repaint(Command):
    def do(self):
        self.reader.dirty = 1
        self.reader.console.repaint_prep()


class kill_line(KillCommand):
    def do(self):
        r = self.reader
        b = r.buffer
        eol = r.eol()
        for c in b[r.pos : eol]:
            if not c.isspace():
                self.kill_range(r.pos, eol)
                return
        else:
            self.kill_range(r.pos, eol + 1)


class unix_line_discard(KillCommand):
    def do(self):
        r = self.reader
        self.kill_range(r.bol(), r.pos)


# XXX unix_word_rubout and backward_kill_word should actually
# do different things...


class unix_word_rubout(KillCommand):
    def do(self):
        r = self.reader
        for i in range(r.get_arg()):
            self.kill_range(r.bow(), r.pos)


class kill_word(KillCommand):
    def do(self):
        r = self.reader
        for i in range(r.get_arg()):
            self.kill_range(r.pos, r.eow())


class backward_kill_word(KillCommand):
    def do(self):
        r = self.reader
        for i in range(r.get_arg()):
            self.kill_range(r.bow(), r.pos)


class yank(YankCommand):
    def do(self):
        r = self.reader
        if not r.kill_ring:
            r.error("nothing to yank")
            return
        r.insert(r.kill_ring[-1])


class yank_pop(YankCommand):
    def do(self):
        r = self.reader
        b = r.buffer
        if not r.kill_ring:
            r.error("nothing to yank")
            return
        if not is_yank(r.last_command):
            r.error("previous command was not a yank")
            return
        repl = len(r.kill_ring[-1])
        r.kill_ring.insert(0, r.kill_ring.pop())
        t = r.kill_ring[-1]
        b[r.pos - repl : r.pos] = t
        r.pos = r.pos - repl + len(t)
        r.dirty = 1


class interrupt(FinishCommand):
    def do(self):
        import signal

        self.reader.console.finish()
        os.kill(os.getpid(), signal.SIGINT)


class suspend(Command):
    def do(self):
        import signal

        r = self.reader
        p = r.pos
        r.console.finish()
        os.kill(os.getpid(), signal.SIGSTOP)
        ## this should probably be done
        ## in a handler for SIGCONT?
        r.console.prepare()
        r.pos = p
        r.posxy = 0, 0
        r.dirty = 1
        r.console.screen = []


class up(MotionCommand):
    def do(self):
        r = self.reader
        for i in range(r.get_arg()):
            bol1 = r.bol()
            if bol1 == 0:
                if r.historyi > 0:
                    r.select_item(r.historyi - 1)
                    return
                r.pos = 0
                r.error("start of buffer")
                return
            bol2 = r.bol(bol1 - 1)
            line_pos = r.pos - bol1
            if line_pos > bol1 - bol2 - 1:
                r.sticky_y = line_pos
                r.pos = bol1 - 1
            else:
                r.pos = bol2 + line_pos


class down(MotionCommand):
    def do(self):
        r = self.reader
        b = r.buffer
        for i in range(r.get_arg()):
            bol1 = r.bol()
            eol1 = r.eol()
            if eol1 == len(b):
                if r.historyi < len(r.history):
                    r.select_item(r.historyi + 1)
                    r.pos = r.eol(0)
                    return
                r.pos = len(b)
                r.error("end of buffer")
                return
            eol2 = r.eol(eol1 + 1)
            if r.pos - bol1 > eol2 - eol1 - 1:
                r.pos = eol2
            else:
                r.pos = eol1 + (r.pos - bol1) + 1


class left(MotionCommand):
    def do(self):
        r = self.reader
        for i in range(r.get_arg()):
            p = r.pos - 1
            if p >= 0:
                r.pos = p
            else:
                self.reader.error("start of buffer")


class right(MotionCommand):
    def do(self):
        r = self.reader
        b = r.buffer
        for i in range(r.get_arg()):
            p = r.pos + 1
            if p <= len(b):
                r.pos = p
            else:
                self.reader.error("end of buffer")


class beginning_of_line(MotionCommand):
    def do(self):
        self.reader.pos = self.reader.bol()


class end_of_line(MotionCommand):
    def do(self):
        self.reader.pos = self.reader.eol()


class home(MotionCommand):
    def do(self):
        self.reader.pos = 0


class end(MotionCommand):
    def do(self):
        self.reader.pos = len(self.reader.buffer)


class forward_word(MotionCommand):
    def do(self):
        r = self.reader
        for i in range(r.get_arg()):
            r.pos = r.eow()


class backward_word(MotionCommand):
    def do(self):
        r = self.reader
        for i in range(r.get_arg()):
            r.pos = r.bow()


class self_insert(EditCommand):
    def do(self):
        r = self.reader
        r.insert(self.event * r.get_arg())


class insert_nl(EditCommand):
    def do(self):
        r = self.reader
        r.insert("\n" * r.get_arg())


class transpose_characters(EditCommand):
    def do(self):
        r = self.reader
        b = r.buffer
        s = r.pos - 1
        if s < 0:
            r.error("cannot transpose at start of buffer")
        else:
            if s == len(b):
                s -= 1
            t = min(s + r.get_arg(), len(b) - 1)
            c = b[s]
            del b[s]
            b.insert(t, c)
            r.pos = t
            r.dirty = 1


class backspace(EditCommand):
    def do(self):
        r = self.reader
        b = r.buffer
        for i in range(r.get_arg()):
            if r.pos > 0:
                r.pos -= 1
                del b[r.pos]
                r.dirty = 1
            else:
                self.reader.error("can't backspace at start")


class delete(EditCommand):
    def do(self):
        r = self.reader
        b = r.buffer
        if (
            r.pos == 0
            and len(b) == 0  # this is something of a hack
            and self.event[-1] == "\004"
        ):
            r.update_screen()
            r.console.finish()
            raise EOFError
        for i in range(r.get_arg()):
            if r.pos != len(b):
                del b[r.pos]
                r.dirty = 1
            else:
                self.reader.error("end of buffer")


class accept(FinishCommand):
    def do(self):
        pass


class invalid_key(Command):
    def do(self):
        pending = self.reader.console.getpending()
        s = "".join(self.event) + pending.data
        self.reader.error("`%r' not bound" % s)


class invalid_command(Command):
    def do(self):
        s = self.event_name
        self.reader.error("command `%s' not known" % s)


class qIHelp(Command):
    def do(self):
        from .reader import disp_str

        r = self.reader
        pending = r.console.getpending().data
        disp = disp_str((self.event + pending).encode())[0]
        r.insert(disp * r.get_arg())
        r.pop_input_trans()


class QITrans:
    def push(self, evt):
        self.evt = evt

    def get(self):
        return ("qIHelp", self.evt.raw)


class quoted_insert(Command):
    kills_digit_arg = 0

    def do(self):
        # XXX in Python 3, processing insert/C-q/C-v keys crashes
        # because of a mixture of str and bytes.  Disable these keys.
        pass
        # self.reader.push_input_trans(QITrans())
