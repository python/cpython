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

import re
import unicodedata
import traceback

from . import commands, input

_r_csi_seq = re.compile(r"\033\[[ -@]*[A-~]")


def _make_unctrl_map():
    uc_map = {}
    for i in range(256):
        c = chr(i)
        if unicodedata.category(c)[0] != "C":
            uc_map[i] = c
    for i in range(32):
        uc_map[i] = "^" + chr(ord("A") + i - 1)
    uc_map[ord(b"\t")] = "    "  # display TABs as 4 characters
    uc_map[ord(b"\177")] = "^?"
    for i in range(256):
        if i not in uc_map:
            uc_map[i] = "\\%03o" % i
    return uc_map


def _my_unctrl(c, u=_make_unctrl_map()):
    # takes an integer, returns a unicode
    if c in u:
        return u[c]
    else:
        if unicodedata.category(c).startswith("C"):
            return r"\u%04x" % ord(c)
        else:
            return c


if "a"[0] == b"a":
    # When running tests with python2, bytes characters are bytes.
    def _my_unctrl(c, uc=_my_unctrl):
        return uc(ord(c))


def disp_str(buffer, join="".join, uc=_my_unctrl):
    """disp_str(buffer:string) -> (string, [int])

    Return the string that should be the printed represenation of
    |buffer| and a list detailing where the characters of |buffer|
    get used up.  E.g.:

    >>> disp_str(chr(3))
    ('^C', [1, 0])

    the list always contains 0s or 1s at present; it could conceivably
    go higher as and when unicode support happens."""
    # disp_str proved to be a bottleneck for large inputs,
    # so it needs to be rewritten in C; it's not required though.
    s = [uc(x) for x in buffer]
    b = []  # XXX: bytearray
    for x in s:
        b.append(1)
        b.extend([0] * (len(x) - 1))
    return join(s), b


del _my_unctrl

del _make_unctrl_map

# syntax classes:

[SYNTAX_WHITESPACE, SYNTAX_WORD, SYNTAX_SYMBOL] = range(3)


def make_default_syntax_table():
    # XXX perhaps should use some unicodedata here?
    st = {}
    for c in map(chr, range(256)):
        st[c] = SYNTAX_SYMBOL
    for c in [a for a in map(chr, range(256)) if a.isalnum()]:
        st[c] = SYNTAX_WORD
    st["\n"] = st[" "] = SYNTAX_WHITESPACE
    return st


default_keymap = tuple(
    [
        (r"\C-a", "beginning-of-line"),
        (r"\C-b", "left"),
        (r"\C-c", "interrupt"),
        (r"\C-d", "delete"),
        (r"\C-e", "end-of-line"),
        (r"\C-f", "right"),
        (r"\C-g", "cancel"),
        (r"\C-h", "backspace"),
        (r"\C-j", "accept"),
        (r"\<return>", "accept"),
        (r"\C-k", "kill-line"),
        (r"\C-l", "clear-screen"),
        (r"\C-m", "accept"),
        (r"\C-q", "quoted-insert"),
        (r"\C-t", "transpose-characters"),
        (r"\C-u", "unix-line-discard"),
        (r"\C-v", "quoted-insert"),
        (r"\C-w", "unix-word-rubout"),
        (r"\C-x\C-u", "upcase-region"),
        (r"\C-y", "yank"),
        (r"\C-z", "suspend"),
        (r"\M-b", "backward-word"),
        (r"\M-c", "capitalize-word"),
        (r"\M-d", "kill-word"),
        (r"\M-f", "forward-word"),
        (r"\M-l", "downcase-word"),
        (r"\M-t", "transpose-words"),
        (r"\M-u", "upcase-word"),
        (r"\M-y", "yank-pop"),
        (r"\M--", "digit-arg"),
        (r"\M-0", "digit-arg"),
        (r"\M-1", "digit-arg"),
        (r"\M-2", "digit-arg"),
        (r"\M-3", "digit-arg"),
        (r"\M-4", "digit-arg"),
        (r"\M-5", "digit-arg"),
        (r"\M-6", "digit-arg"),
        (r"\M-7", "digit-arg"),
        (r"\M-8", "digit-arg"),
        (r"\M-9", "digit-arg"),
        # (r'\M-\n', 'insert-nl'),
        ("\\\\", "self-insert"),
    ]
    + [(c, "self-insert") for c in map(chr, range(32, 127)) if c != "\\"]
    + [(c, "self-insert") for c in map(chr, range(128, 256)) if c.isalpha()]
    + [
        (r"\<up>", "up"),
        (r"\<down>", "down"),
        (r"\<left>", "left"),
        (r"\<right>", "right"),
        (r"\<insert>", "quoted-insert"),
        (r"\<delete>", "delete"),
        (r"\<backspace>", "backspace"),
        (r"\M-\<backspace>", "backward-kill-word"),
        (r"\<end>", "end-of-line"),  # was 'end'
        (r"\<home>", "beginning-of-line"),  # was 'home'
        (r"\<f1>", "help"),
        (r"\EOF", "end"),  # the entries in the terminfo database for xterms
        (r"\EOH", "home"),  # seem to be wrong.  this is a less than ideal
        # workaround
    ]
)

class Reader:
    """The Reader class implements the bare bones of a command reader,
    handling such details as editing and cursor motion.  What it does
    not support are such things as completion or history support -
    these are implemented elsewhere.

    Instance variables of note include:

      * buffer:
        A *list* (*not* a string at the moment :-) containing all the
        characters that have been entered.
      * console:
        Hopefully encapsulates the OS dependent stuff.
      * pos:
        A 0-based index into `buffer' for where the insertion point
        is.
      * screeninfo:
        Ahem.  This list contains some info needed to move the
        insertion point around reasonably efficiently.  I'd like to
        get rid of it, because its contents are obtuse (to put it
        mildly) but I haven't worked out if that is possible yet.
      * cxy, lxy:
        the position of the insertion point in screen ... XXX
      * syntax_table:
        Dictionary mapping characters to `syntax class'; read the
        emacs docs to see what this means :-)
      * commands:
        Dictionary mapping command names to command classes.
      * arg:
        The emacs-style prefix argument.  It will be None if no such
        argument has been provided.
      * dirty:
        True if we need to refresh the display.
      * kill_ring:
        The emacs-style kill-ring; manipulated with yank & yank-pop
      * ps1, ps2, ps3, ps4:
        prompts.  ps1 is the prompt for a one-line input; for a
        multiline input it looks like:
            ps2> first line of input goes here
            ps3> second and further
            ps3> lines get ps3
            ...
            ps4> and the last one gets ps4
        As with the usual top-level, you can set these to instances if
        you like; str() will be called on them (once) at the beginning
        of each command.  Don't put really long or newline containing
        strings here, please!
        This is just the default policy; you can change it freely by
        overriding get_prompt() (and indeed some standard subclasses
        do).
      * finished:
        handle1 will set this to a true value if a command signals
        that we're done.
    """

    msg_at_bottom = True

    def __init__(self, console):
        self.buffer = []
        # Enable the use of `insert` without a `prepare` call - necessary to
        # facilitate the tab completion hack implemented for
        # <https://bugs.python.org/issue25660>.
        self.pos = 0
        self.ps1 = "->> "
        self.ps2 = "/>> "
        self.ps3 = "|.. "
        self.ps4 = r"\__ "
        self.kill_ring = []
        self.arg = None
        self.finished = 0
        self.console = console
        self.commands = {}
        self.msg = ""
        for v in vars(commands).values():
            if (
                isinstance(v, type)
                and issubclass(v, commands.Command)
                and v.__name__[0].islower()
            ):
                self.commands[v.__name__] = v
                self.commands[v.__name__.replace("_", "-")] = v
        self.syntax_table = make_default_syntax_table()
        self.input_trans_stack = []
        self.keymap = self.collect_keymap()
        self.input_trans = input.KeymapTranslator(
            self.keymap, invalid_cls="invalid-key", character_cls="self-insert"
        )

    def collect_keymap(self):
        return default_keymap

    def calc_screen(self):
        """The purpose of this method is to translate changes in
        self.buffer into changes in self.screen.  Currently it rips
        everything down and starts from scratch, which whilst not
        especially efficient is certainly simple(r).
        """
        lines = self.get_unicode().split("\n")
        screen = []
        screeninfo = []
        w = self.console.width - 1
        p = self.pos
        for ln, line in zip(range(len(lines)), lines):
            ll = len(line)
            if 0 <= p <= ll:
                if self.msg and not self.msg_at_bottom:
                    for mline in self.msg.split("\n"):
                        screen.append(mline)
                        screeninfo.append((0, []))
                self.lxy = p, ln
            prompt = self.get_prompt(ln, ll >= p >= 0)
            while "\n" in prompt:
                pre_prompt, _, prompt = prompt.partition("\n")
                screen.append(pre_prompt)
                screeninfo.append((0, []))
            p -= ll + 1
            prompt, lp = self.process_prompt(prompt)
            l, l2 = disp_str(line)
            wrapcount = (len(l) + lp) // w
            if wrapcount == 0:
                screen.append(prompt + l)
                screeninfo.append((lp, l2 + [1]))
            else:
                screen.append(prompt + l[: w - lp] + "\\")
                screeninfo.append((lp, l2[: w - lp]))
                for i in range(-lp + w, -lp + wrapcount * w, w):
                    screen.append(l[i : i + w] + "\\")
                    screeninfo.append((0, l2[i : i + w]))
                screen.append(l[wrapcount * w - lp :])
                screeninfo.append((0, l2[wrapcount * w - lp :] + [1]))
        self.screeninfo = screeninfo
        self.cxy = self.pos2xy(self.pos)
        if self.msg and self.msg_at_bottom:
            for mline in self.msg.split("\n"):
                screen.append(mline)
                screeninfo.append((0, []))
        return screen

    def process_prompt(self, prompt):
        """Process the prompt.

        This means calculate the length of the prompt. The character \x01
        and \x02 are used to bracket ANSI control sequences and need to be
        excluded from the length calculation.  So also a copy of the prompt
        is returned with these control characters removed."""

        # The logic below also ignores the length of common escape
        # sequences if they were not explicitly within \x01...\x02.
        # They are CSI (or ANSI) sequences  ( ESC [ ... LETTER )

        out_prompt = ""
        l = len(prompt)
        pos = 0
        while True:
            s = prompt.find("\x01", pos)
            if s == -1:
                break
            e = prompt.find("\x02", s)
            if e == -1:
                break
            # Found start and end brackets, subtract from string length
            l = l - (e - s + 1)
            keep = prompt[pos:s]
            l -= sum(map(len, _r_csi_seq.findall(keep)))
            out_prompt += keep + prompt[s + 1 : e]
            pos = e + 1
        keep = prompt[pos:]
        l -= sum(map(len, _r_csi_seq.findall(keep)))
        out_prompt += keep
        return out_prompt, l

    def bow(self, p=None):
        """Return the 0-based index of the word break preceding p most
        immediately.

        p defaults to self.pos; word boundaries are determined using
        self.syntax_table."""
        if p is None:
            p = self.pos
        st = self.syntax_table
        b = self.buffer
        p -= 1
        while p >= 0 and st.get(b[p], SYNTAX_WORD) != SYNTAX_WORD:
            p -= 1
        while p >= 0 and st.get(b[p], SYNTAX_WORD) == SYNTAX_WORD:
            p -= 1
        return p + 1

    def eow(self, p=None):
        """Return the 0-based index of the word break following p most
        immediately.

        p defaults to self.pos; word boundaries are determined using
        self.syntax_table."""
        if p is None:
            p = self.pos
        st = self.syntax_table
        b = self.buffer
        while p < len(b) and st.get(b[p], SYNTAX_WORD) != SYNTAX_WORD:
            p += 1
        while p < len(b) and st.get(b[p], SYNTAX_WORD) == SYNTAX_WORD:
            p += 1
        return p

    def bol(self, p=None):
        """Return the 0-based index of the line break preceding p most
        immediately.

        p defaults to self.pos."""
        # XXX there are problems here.
        if p is None:
            p = self.pos
        b = self.buffer
        p -= 1
        while p >= 0 and b[p] != "\n":
            p -= 1
        return p + 1

    def eol(self, p=None):
        """Return the 0-based index of the line break following p most
        immediately.

        p defaults to self.pos."""
        if p is None:
            p = self.pos
        b = self.buffer
        while p < len(b) and b[p] != "\n":
            p += 1
        return p

    def get_arg(self, default=1):
        """Return any prefix argument that the user has supplied,
        returning `default' if there is None.  `default' defaults
        (groan) to 1."""
        if self.arg is None:
            return default
        else:
            return self.arg

    def get_prompt(self, lineno, cursor_on_line):
        """Return what should be in the left-hand margin for line
        `lineno'."""
        if self.arg is not None and cursor_on_line:
            return "(arg: %s) " % self.arg
        if "\n" in self.buffer:
            if lineno == 0:
                res = self.ps2
            elif lineno == self.buffer.count("\n"):
                res = self.ps4
            else:
                res = self.ps3
        else:
            res = self.ps1
        
        if traceback._can_colorize():
            res = traceback._ANSIColors.BOLD_MAGENTA + res + traceback._ANSIColors.RESET
        # Lazily call str() on self.psN, and cache the results using as key
        # the object on which str() was called.  This ensures that even if the
        # same object is used e.g. for ps1 and ps2, str() is called only once.
        if res not in self._pscache:
            self._pscache[res] = str(res)
        return self._pscache[res]

    def push_input_trans(self, itrans):
        self.input_trans_stack.append(self.input_trans)
        self.input_trans = itrans

    def pop_input_trans(self):
        self.input_trans = self.input_trans_stack.pop()

    def pos2xy(self, pos):
        """Return the x, y coordinates of position 'pos'."""
        # this *is* incomprehensible, yes.
        y = 0
        assert 0 <= pos <= len(self.buffer)
        if pos == len(self.buffer):
            y = len(self.screeninfo) - 1
            p, l2 = self.screeninfo[y]
            return p + len(l2) - 1, y
        else:
            for p, l2 in self.screeninfo:
                l = l2.count(1)
                if l > pos:
                    break
                else:
                    pos -= l
                    y += 1
            c = 0
            i = 0
            while c < pos:
                c += l2[i]
                i += 1
            while l2[i] == 0:
                i += 1
            return p + i, y

    def insert(self, text):
        """Insert 'text' at the insertion point."""
        self.buffer[self.pos : self.pos] = list(text)
        self.pos += len(text)
        self.dirty = 1

    def update_cursor(self):
        """Move the cursor to reflect changes in self.pos"""
        self.cxy = self.pos2xy(self.pos)
        self.console.move_cursor(*self.cxy)

    def after_command(self, cmd):
        """This function is called to allow post command cleanup."""
        if getattr(cmd, "kills_digit_arg", 1):
            if self.arg is not None:
                self.dirty = 1
            self.arg = None

    def prepare(self):
        """Get ready to run.  Call restore when finished.  You must not
        write to the console in between the calls to prepare and
        restore."""
        try:
            self.console.prepare()
            self.arg = None
            self.screeninfo = []
            self.finished = 0
            del self.buffer[:]
            self.pos = 0
            self.dirty = 1
            self.last_command = None
            self._pscache = {}
        except:
            self.restore()
            raise

    def last_command_is(self, klass):
        if not self.last_command:
            return 0
        return issubclass(klass, self.last_command)

    def restore(self):
        """Clean up after a run."""
        self.console.restore()

    def finish(self):
        """Called when a command signals that we're finished."""
        pass

    def error(self, msg="none"):
        self.msg = "! " + msg + " "
        self.dirty = 1
        self.console.beep()

    def update_screen(self):
        if self.dirty:
            self.refresh()

    def refresh(self):
        """Recalculate and refresh the screen."""
        # this call sets up self.cxy, so call it first.
        screen = self.calc_screen()
        self.console.refresh(screen, self.cxy)
        self.dirty = 0  # forgot this for a while (blush)

    def do_cmd(self, cmd):
        # print cmd
        if isinstance(cmd[0], str):
            cmd = self.commands.get(cmd[0], commands.invalid_command)(self, *cmd)
        elif isinstance(cmd[0], type):
            cmd = cmd[0](self, *cmd)
        else:
            return  # nothing to do

        cmd.do()

        self.after_command(cmd)

        if self.dirty:
            self.refresh()
        else:
            self.update_cursor()

        if not isinstance(cmd, commands.digit_arg):
            self.last_command = cmd.__class__

        self.finished = cmd.finish
        if self.finished:
            self.console.finish()
            self.finish()

    def handle1(self, block=1):
        """Handle a single event.  Wait as long as it takes if block
        is true (the default), otherwise return None if no event is
        pending."""

        if self.msg:
            self.msg = ""
            self.dirty = 1

        while 1:
            event = self.console.get_event(block)
            if not event:  # can only happen if we're not blocking
                return None

            translate = True

            if event.evt == "key":
                self.input_trans.push(event)
            elif event.evt == "scroll":
                self.refresh()
            elif event.evt == "resize":
                self.refresh()
            else:
                translate = False

            if translate:
                cmd = self.input_trans.get()
            else:
                cmd = event.evt, event.data

            if cmd is None:
                if block:
                    continue
                else:
                    return None

            self.do_cmd(cmd)
            return 1

    def push_char(self, char):
        self.console.push_char(char)
        self.handle1(0)

    def readline(self, returns_unicode=False, startup_hook=None):
        """Read a line.  The implementation of this method also shows
        how to drive Reader if you want more control over the event
        loop."""
        self.prepare()
        try:
            if startup_hook is not None:
                startup_hook()
            self.refresh()
            while not self.finished:
                self.handle1()
            if returns_unicode:
                return self.get_unicode()
            return self.get_buffer()
        finally:
            self.restore()

    def bind(self, spec, command):
        self.keymap = self.keymap + ((spec, command),)
        self.input_trans = input.KeymapTranslator(
            self.keymap, invalid_cls="invalid-key", character_cls="self-insert"
        )

    def get_buffer(self, encoding=None):
        if encoding is None:
            encoding = self.console.encoding
        return self.get_unicode().encode(encoding)

    def get_unicode(self):
        """Return the current buffer as a unicode string."""
        return "".join(self.buffer)


def test():
    from .unix_console import UnixConsole

    reader = Reader(UnixConsole())
    reader.ps1 = "**> "
    reader.ps2 = "/*> "
    reader.ps3 = "|*> "
    reader.ps4 = r"\*> "
    while reader.readline():
        pass


if __name__ == "__main__":
    test()
