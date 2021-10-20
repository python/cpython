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
        if parts:
            # 2 min 1 sec
            parts.append('%s sec' % seconds)
        else:
            # 1.0 sec
            parts.append('%.1f sec' % (seconds + ms / 1000))
    if not parts:
        return '%s ms' % ms

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
    support.print_warning(msg)


orig_unraisablehook = None


def regrtest_unraisable_hook(unraisable):
    global orig_unraisablehook
    support.environment_altered = True
    support.print_warning("Unraisable exception")
    old_stderr = sys.stderr
    try:
        support.flush_std_streams()
        sys.stderr = support.print_warning.orig_stderr
        orig_unraisablehook(unraisable)
        sys.stderr.flush()
    finally:
        sys.stderr = old_stderr


def setup_unraisable_hook():
    global orig_unraisablehook
    orig_unraisablehook = sys.unraisablehook
    sys.unraisablehook = regrtest_unraisable_hook


orig_threading_excepthook = None


def regrtest_threading_excepthook(args):
    global orig_threading_excepthook
    support.environment_altered = True
    support.print_warning(f"Uncaught thread exception: {args.exc_type.__name__}")
    old_stderr = sys.stderr
    try:
        support.flush_std_streams()
        sys.stderr = support.print_warning.orig_stderr
        orig_threading_excepthook(args)
        sys.stderr.flush()
    finally:
        sys.stderr = old_stderr


def setup_threading_excepthook():
    global orig_threading_excepthook
    import threading
    orig_threading_excepthook = threading.excepthook
    threading.excepthook = regrtest_threading_excepthook


def clear_caches():
    # Clear the warnings registry, so they can be displayed again
    for mod in sys.modules.values():
        if hasattr(mod, '__warningregistry__'):
            del mod.__warningregistry__

    # Flush standard output, so that buffered data is sent to the OS and
    # associated Python objects are reclaimed.
    for stream in (sys.stdout, sys.stderr, sys.__stdout__, sys.__stderr__):
        if stream is not None:
            stream.flush()

    # Clear assorted module caches.
    # Don't worry about resetting the cache if the module is not loaded
    try:
        distutils_dir_util = sys.modules['distutils.dir_util']
    except KeyError:
        pass
    else:
        distutils_dir_util._path_created.clear()

    try:
        re = sys.modules['re']
    except KeyError:
        pass
    else:
        re.purge()

    try:
        _strptime = sys.modules['_strptime']
    except KeyError:
        pass
    else:
        _strptime._regex_cache.clear()

    try:
        urllib_parse = sys.modules['urllib.parse']
    except KeyError:
        pass
    else:
        urllib_parse.clear_cache()

    try:
        urllib_request = sys.modules['urllib.request']
    except KeyError:
        pass
    else:
        urllib_request.urlcleanup()

    try:
        linecache = sys.modules['linecache']
    except KeyError:
        pass
    else:
        linecache.clearcache()

    try:
        mimetypes = sys.modules['mimetypes']
    except KeyError:
        pass
    else:
        mimetypes._default_mime_types()

    try:
        filecmp = sys.modules['filecmp']
    except KeyError:
        pass
    else:
        filecmp._cache.clear()

    try:
        struct = sys.modules['struct']
    except KeyError:
        pass
    else:
        struct._clearcache()

    try:
        doctest = sys.modules['doctest']
    except KeyError:
        pass
    else:
        doctest.master = None

    try:
        ctypes = sys.modules['ctypes']
    except KeyError:
        pass
    else:
        ctypes._reset_cache()

    try:
        typing = sys.modules['typing']
    except KeyError:
        pass
    else:
        for f in typing._cleanups:
            f()
