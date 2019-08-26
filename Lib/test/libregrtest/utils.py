import math
import os.path
import sys
import textwrap
from test import support


def format_duration(seconds):
    ms = math.ceil(seconds * 1e3)
    seconds, ms = divmod(ms, 1000)
    minutes, seconds = divmod(seconds, 60)
    hours, minutes = divmod(minutes, 60)

    parts = []
    if hours:
        parts.append('%s hour' % hours)
    if minutes:
        parts.append('%s min' % minutes)
    if seconds:
        parts.append('%s sec' % seconds)
    if ms:
        parts.append('%s ms' % ms)
    if not parts:
        return '0 ms'

    parts = parts[:2]
    return ' '.join(parts)


def removepy(names):
    if not names:
        return
    for idx, name in enumerate(names):
        basename, ext = os.path.splitext(name)
        if ext == '.py':
            names[idx] = basename


def count(n, word):
    if n == 1:
        return "%d %s" % (n, word)
    else:
        return "%d %ss" % (n, word)


def printlist(x, width=70, indent=4, file=None):
    """Print the elements of iterable x to stdout.

    Optional arg width (default 70) is the maximum line length.
    Optional arg indent (default 4) is the number of blanks with which to
    begin each line.
    """

    blanks = ' ' * indent
    # Print the sorted list: 'x' may be a '--random' list or a set()
    print(textwrap.fill(' '.join(str(elt) for elt in sorted(x)), width,
                        initial_indent=blanks, subsequent_indent=blanks),
          file=file)


def print_warning(msg):
    print(f"Warning -- {msg}", file=sys.stderr, flush=True)


orig_unraisablehook = None


def regrtest_unraisable_hook(unraisable):
    global orig_unraisablehook
    support.environment_altered = True
    print_warning("Unraisable exception")
    orig_unraisablehook(unraisable)


def setup_unraisable_hook():
    global orig_unraisablehook
    orig_unraisablehook = sys.unraisablehook
    sys.unraisablehook = regrtest_unraisable_hook
