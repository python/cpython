from contextlib import contextmanager

try:
    from _sqlite3 import SQLITE_KEYWORDS
except ImportError:
    SQLITE_KEYWORDS = ()

_completion_matches = []


def _complete(text, state):
    global _completion_matches

    if state == 0:
        text_upper = text.upper()
        _completion_matches = [c for c in SQLITE_KEYWORDS if c.startswith(text_upper)]
    try:
        return _completion_matches[state] + " "
    except IndexError:
        return None


@contextmanager
def completer():
    try:
        import readline
    except ImportError:
        yield
        return

    old_completer = readline.get_completer()
    try:
        readline.set_completer(_complete)
        if readline.backend == "editline":
            # libedit uses "^I" instead of "tab"
            command_string = "bind ^I rl_complete"
        else:
            command_string = "tab: complete"
        readline.parse_and_bind(command_string)
        yield
    finally:
        readline.set_completer(old_completer)
