"""Utilities to get a password and/or the current user name.

getpass(prompt[, stream[, echo_char]]) - Prompt for a password, with echo
turned off and optional keyboard feedback.
getuser() - Get the user name from the environment or password database.

GetPassWarning - This UserWarning is issued when getpass() cannot prevent
                 echoing of the password contents while reading.

On Windows, the msvcrt module will be used.

"""

# Authors: Piers Lauder (original)
#          Guido van Rossum (Windows support and cleanup)
#          Gregory P. Smith (tty support & GetPassWarning)

import contextlib
import io
import os
import sys

__all__ = ["getpass","getuser","GetPassWarning"]


class GetPassWarning(UserWarning): pass


# Default POSIX control character mappings
_POSIX_CTRL_CHARS = {
    'ERASE': '\x7f',   # DEL/Backspace
    'KILL': '\x15',    # Ctrl+U - kill line
    'WERASE': '\x17',  # Ctrl+W - erase word
    'LNEXT': '\x16',   # Ctrl+V - literal next
    'EOF': '\x04',     # Ctrl+D - EOF
    'INTR': '\x03',    # Ctrl+C - interrupt
    'SOH': '\x01',     # Ctrl+A - start of heading (beginning of line)
    'ENQ': '\x05',     # Ctrl+E - enquiry (end of line)
    'VT': '\x0b',      # Ctrl+K - vertical tab (kill forward)
}


def _get_terminal_ctrl_chars(fd):
    """Extract control characters from terminal settings.

    Returns a dict mapping control char names to their str values.
    Falls back to POSIX defaults if termios isn't available.
    """
    res = _POSIX_CTRL_CHARS.copy()
    try:
        old = termios.tcgetattr(fd)
        cc = old[6]  # Index 6 is the control characters array
    except (termios.error, OSError):
        return res
    # Ctrl+A/E/K are not in termios, use POSIX defaults
    for name in ('ERASE', 'KILL', 'WERASE', 'LNEXT', 'EOF', 'INTR'):
        cap = getattr(termios, f'V{name}')
        if cap < len(cc):
            res[name] = cc[cap].decode('latin-1')
    return res


def unix_getpass(prompt='Password: ', stream=None, *, echo_char=None):
    """Prompt for a password, with echo turned off.

    Args:
      prompt: Written on stream to ask for the input.  Default: 'Password: '
      stream: A writable file object to display the prompt.  Defaults to
              the tty.  If no tty is available defaults to sys.stderr.
      echo_char: A single ASCII character to mask input (e.g., '*').
              If None, input is hidden.
    Returns:
      The seKr3t input.
    Raises:
      EOFError: If our input tty or stdin was closed.
      GetPassWarning: When we were unable to turn echo off on the input.

    Always restores terminal settings before returning.
    """
    _check_echo_char(echo_char)

    passwd = None
    with contextlib.ExitStack() as stack:
        try:
            # Always try reading and writing directly on the tty first.
            fd = os.open('/dev/tty', os.O_RDWR|os.O_NOCTTY)
            tty = io.FileIO(fd, 'w+')
            stack.enter_context(tty)
            input = io.TextIOWrapper(tty)
            stack.enter_context(input)
            if not stream:
                stream = input
        except OSError:
            # If that fails, see if stdin can be controlled.
            stack.close()
            try:
                fd = sys.stdin.fileno()
            except (AttributeError, ValueError):
                fd = None
                passwd = fallback_getpass(prompt, stream)
            input = sys.stdin
            if not stream:
                stream = sys.stderr

        if fd is not None:
            try:
                old = termios.tcgetattr(fd)     # a copy to save
                new = old[:]
                new[3] &= ~termios.ECHO  # 3 == 'lflags'
                # Extract control characters before changing terminal mode
                term_ctrl_chars = None
                if echo_char:
                    new[3] &= ~termios.ICANON
                    term_ctrl_chars = _get_terminal_ctrl_chars(fd)
                tcsetattr_flags = termios.TCSAFLUSH
                if hasattr(termios, 'TCSASOFT'):
                    tcsetattr_flags |= termios.TCSASOFT
                try:
                    termios.tcsetattr(fd, tcsetattr_flags, new)
                    passwd = _raw_input(prompt, stream, input=input,
                                        echo_char=echo_char,
                                        term_ctrl_chars=term_ctrl_chars)

                finally:
                    termios.tcsetattr(fd, tcsetattr_flags, old)
                    stream.flush()  # issue7208
            except termios.error:
                if passwd is not None:
                    # _raw_input succeeded.  The final tcsetattr failed.  Reraise
                    # instead of leaving the terminal in an unknown state.
                    raise
                # We can't control the tty or stdin.  Give up and use normal IO.
                # fallback_getpass() raises an appropriate warning.
                if stream is not input:
                    # clean up unused file objects before blocking
                    stack.close()
                passwd = fallback_getpass(prompt, stream)

        stream.write('\n')
        return passwd


def win_getpass(prompt='Password: ', stream=None, *, echo_char=None):
    """Prompt for password with echo off, using Windows getwch()."""
    if sys.stdin is not sys.__stdin__:
        return fallback_getpass(prompt, stream)
    _check_echo_char(echo_char)

    for c in prompt:
        msvcrt.putwch(c)
    pw = ""
    while 1:
        c = msvcrt.getwch()
        if c == '\r' or c == '\n':
            break
        if c == '\003':
            raise KeyboardInterrupt
        if c == '\b':
            if echo_char and pw:
                msvcrt.putwch('\b')
                msvcrt.putwch(' ')
                msvcrt.putwch('\b')
            pw = pw[:-1]
        else:
            pw = pw + c
            if echo_char:
                msvcrt.putwch(echo_char)
    msvcrt.putwch('\r')
    msvcrt.putwch('\n')
    return pw


def fallback_getpass(prompt='Password: ', stream=None, *, echo_char=None):
    _check_echo_char(echo_char)
    import warnings
    warnings.warn("Can not control echo on the terminal.", GetPassWarning,
                  stacklevel=2)
    if not stream:
        stream = sys.stderr
    print("Warning: Password input may be echoed.", file=stream)
    return _raw_input(prompt, stream, echo_char=echo_char)


def _check_echo_char(echo_char):
    # Single-character ASCII excluding control characters
    if echo_char is None:
        return
    if not isinstance(echo_char, str):
        raise TypeError("'echo_char' must be a str or None, not "
                        f"{type(echo_char).__name__}")
    if not (
        len(echo_char) == 1
        and echo_char.isprintable()
        and echo_char.isascii()
    ):
        raise ValueError("'echo_char' must be a single printable ASCII "
                         f"character, got: {echo_char!r}")


def _raw_input(prompt="", stream=None, input=None, echo_char=None,
               term_ctrl_chars=None):
    # This doesn't save the string in the GNU readline history.
    if not stream:
        stream = sys.stderr
    if not input:
        input = sys.stdin
    prompt = str(prompt)
    if prompt:
        try:
            stream.write(prompt)
        except UnicodeEncodeError:
            # Use replace error handler to get as much as possible printed.
            prompt = prompt.encode(stream.encoding, 'replace')
            prompt = prompt.decode(stream.encoding)
            stream.write(prompt)
        stream.flush()
    # NOTE: The Python C API calls flockfile() (and unlock) during readline.
    if echo_char:
        return _readline_with_echo_char(stream, input, echo_char,
                                        term_ctrl_chars)
    line = input.readline()
    if not line:
        raise EOFError
    if line[-1] == '\n':
        line = line[:-1]
    return line


class _PasswordLineEditor:
    """Handles line editing for password input with echo character."""

    def __init__(self, stream, echo_char, ctrl_chars):
        self.stream = stream
        self.echo_char = echo_char
        self.passwd = ""
        self.cursor_pos = 0
        self.eof_pressed = False
        self.literal_next = False
        self.ctrl = ctrl_chars
        self._dispatch = {
            ctrl_chars['SOH']: self._handle_move_start,     # Ctrl+A
            ctrl_chars['ENQ']: self._handle_move_end,       # Ctrl+E
            ctrl_chars['VT']: self._handle_kill_forward,    # Ctrl+K
            ctrl_chars['KILL']: self._handle_kill_line,     # Ctrl+U
            ctrl_chars['WERASE']: self._handle_erase_word,  # Ctrl+W
            ctrl_chars['ERASE']: self._handle_erase,        # DEL
            '\b': self._handle_erase,                       # Backspace
        }

    def _refresh_display(self):
        """Redraw the entire password line with *echo_char*."""
        self.stream.write('\r' + ' ' * len(self.passwd) + '\r')
        self.stream.write(self.echo_char * len(self.passwd))
        if self.cursor_pos < len(self.passwd):
            self.stream.write('\b' * (len(self.passwd) - self.cursor_pos))
        self.stream.flush()

    def _erase_chars(self, count):
        """Erase count echo characters from display."""
        self.stream.write("\b \b" * count)

    def _insert_char(self, char):
        """Insert character at cursor position."""
        self.passwd = self.passwd[:self.cursor_pos] + char + self.passwd[self.cursor_pos:]
        self.cursor_pos += 1
        # Only refresh if inserting in middle
        if self.cursor_pos < len(self.passwd):
            self._refresh_display()
        else:
            self.stream.write(self.echo_char)
            self.stream.flush()

    def _handle_move_start(self):
        """Move cursor to beginning (Ctrl+A)."""
        self.cursor_pos = 0

    def _handle_move_end(self):
        """Move cursor to end (Ctrl+E)."""
        self.cursor_pos = len(self.passwd)

    def _handle_erase(self):
        """Delete character before cursor (Backspace/DEL)."""
        if self.cursor_pos > 0:
            self.passwd = self.passwd[:self.cursor_pos-1] + self.passwd[self.cursor_pos:]
            self.cursor_pos -= 1
            # Only refresh if deleting from middle
            if self.cursor_pos < len(self.passwd):
                self._refresh_display()
            else:
                self.stream.write("\b \b")
                self.stream.flush()

    def _handle_kill_line(self):
        """Erase entire line (Ctrl+U)."""
        self._erase_chars(len(self.passwd))
        self.passwd = ""
        self.cursor_pos = 0
        self.stream.flush()

    def _handle_kill_forward(self):
        """Kill from cursor to end (Ctrl+K)."""
        chars_to_delete = len(self.passwd) - self.cursor_pos
        self.passwd = self.passwd[:self.cursor_pos]
        self._erase_chars(chars_to_delete)
        self.stream.flush()

    def _handle_erase_word(self):
        """Erase previous word (Ctrl+W)."""
        old_cursor = self.cursor_pos
        # Skip trailing spaces
        while self.cursor_pos > 0 and self.passwd[self.cursor_pos-1] == ' ':
            self.cursor_pos -= 1
        # Delete the word
        while self.cursor_pos > 0 and self.passwd[self.cursor_pos-1] != ' ':
            self.cursor_pos -= 1
        # Remove the deleted portion
        self.passwd = self.passwd[:self.cursor_pos] + self.passwd[old_cursor:]
        self._refresh_display()

    def handle(self, char):
        """Handle a single character input. Returns True if handled."""
        self.eof_pressed = False
        handler = self._dispatch.get(char)
        if handler:
            handler()
            return True
        return False


def _readline_with_echo_char(stream, input, echo_char, term_ctrl_chars=None):
    """Read password with echo character and line editing support."""
    if term_ctrl_chars is None:
        term_ctrl_chars = _POSIX_CTRL_CHARS.copy()

    editor = _PasswordLineEditor(stream, echo_char, term_ctrl_chars)

    while True:
        char = input.read(1)

        # Check for line terminators
        if char in ('\n', '\r'):
            break
        # Handle literal next mode FIRST (Ctrl+V quotes next char)
        elif editor.literal_next:
            editor._insert_char(char)
            editor.literal_next = False
            editor.eof_pressed = False
        # Check if it's the LNEXT character
        elif char == editor.ctrl['LNEXT']:
            editor.literal_next = True
            editor.eof_pressed = False
        # Check for special control characters
        elif char == editor.ctrl['INTR']:
            raise KeyboardInterrupt
        elif char == editor.ctrl['EOF']:
            if editor.eof_pressed:
                break
            editor.eof_pressed = True
        elif char == '\x00':
            pass
        # Dispatch to handler or insert as normal character
        elif not editor.handle(char):
            editor._insert_char(char)
            editor.eof_pressed = False

    return editor.passwd


def getuser():
    """Get the username from the environment or password database.

    First try various environment variables, then the password
    database.  This works on Windows as long as USERNAME is set.
    Any failure to find a username raises OSError.

    .. versionchanged:: 3.13
        Previously, various exceptions beyond just :exc:`OSError`
        were raised.
    """

    for name in ('LOGNAME', 'USER', 'LNAME', 'USERNAME'):
        user = os.environ.get(name)
        if user:
            return user

    try:
        import pwd
        return pwd.getpwuid(os.getuid())[0]
    except (ImportError, KeyError) as e:
        raise OSError('No username set in the environment') from e


# Bind the name getpass to the appropriate function
try:
    import termios
    # it's possible there is an incompatible termios from the
    # McMillan Installer, make sure we have a UNIX-compatible termios
    termios.tcgetattr, termios.tcsetattr
except (ImportError, AttributeError):
    try:
        import msvcrt
    except ImportError:
        getpass = fallback_getpass
    else:
        getpass = win_getpass
else:
    getpass = unix_getpass
