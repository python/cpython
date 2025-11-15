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
                    # Get control characters from terminal settings
                    # Index 6 is cc (control characters array)
                    cc = old[6]
                    term_ctrl_chars = {
                        'ERASE': cc[termios.VERASE] if termios.VERASE < len(cc) else b'\x7f',
                        'KILL': cc[termios.VKILL] if termios.VKILL < len(cc) else b'\x15',
                        'WERASE': cc[termios.VWERASE] if termios.VWERASE < len(cc) else b'\x17',
                        'LNEXT': cc[termios.VLNEXT] if termios.VLNEXT < len(cc) else b'\x16',
                    }
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


def _readline_with_echo_char(stream, input, echo_char, term_ctrl_chars=None):
    passwd = ""
    eof_pressed = False
    literal_next = False  # For LNEXT (Ctrl+V)

    # Convert terminal control characters to strings for comparison
    # Default to standard POSIX values if not provided
    if term_ctrl_chars:
        # Control chars from termios are bytes, convert to str
        erase_char = term_ctrl_chars['ERASE'].decode('latin-1') if isinstance(term_ctrl_chars['ERASE'], bytes) else term_ctrl_chars['ERASE']
        kill_char = term_ctrl_chars['KILL'].decode('latin-1') if isinstance(term_ctrl_chars['KILL'], bytes) else term_ctrl_chars['KILL']
        werase_char = term_ctrl_chars['WERASE'].decode('latin-1') if isinstance(term_ctrl_chars['WERASE'], bytes) else term_ctrl_chars['WERASE']
        lnext_char = term_ctrl_chars['LNEXT'].decode('latin-1') if isinstance(term_ctrl_chars['LNEXT'], bytes) else term_ctrl_chars['LNEXT']
    else:
        # Standard POSIX defaults
        erase_char = '\x7f'  # DEL
        kill_char = '\x15'   # Ctrl+U
        werase_char = '\x17' # Ctrl+W
        lnext_char = '\x16'  # Ctrl+V

    while True:
        char = input.read(1)

        if char == '\n' or char == '\r':
            break
        elif char == '\x03':
            raise KeyboardInterrupt
        elif char == '\x04':
            if eof_pressed:
                break
            else:
                eof_pressed = True
        elif char == '\x00':
            continue
        # Handle LNEXT (Ctrl+V) - insert next character literally
        elif literal_next:
            passwd += char
            stream.write(echo_char)
            stream.flush()
            literal_next = False
            eof_pressed = False
        elif char == lnext_char:
            literal_next = True
            eof_pressed = False
        # Handle ERASE (Backspace/DEL) - delete one character
        elif char == erase_char or char == '\b':
            if passwd:
                stream.write("\b \b")
                stream.flush()
                passwd = passwd[:-1]
            eof_pressed = False
        # Handle KILL (Ctrl+U) - erase entire line
        elif char == kill_char:
            # Clear all echoed characters
            while passwd:
                stream.write("\b \b")
                passwd = passwd[:-1]
            stream.flush()
            eof_pressed = False
        # Handle WERASE (Ctrl+W) - erase previous word
        elif char == werase_char:
            # Delete backwards until we find a space or reach the beginning
            # First, skip any trailing spaces
            while passwd and passwd[-1] == ' ':
                stream.write("\b \b")
                passwd = passwd[:-1]
            # Then delete the word
            while passwd and passwd[-1] != ' ':
                stream.write("\b \b")
                passwd = passwd[:-1]
            stream.flush()
            eof_pressed = False
        else:
            passwd += char
            stream.write(echo_char)
            stream.flush()
            eof_pressed = False
    return passwd


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
