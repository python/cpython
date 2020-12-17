import sys
import threading
from contextlib import contextmanager
from tempfile import TemporaryFile

if sys.version_info >= (3, 8):
    from importlib import metadata as importlib_metadata  # noqa
else:
    import importlib_metadata  # noqa


def is_main_thread():
    """returns true if we are within the main thread"""
    cur_thread = threading.current_thread()
    if sys.version_info >= (3, 4):
        return cur_thread is threading.main_thread()
    else:
        # noinspection PyUnresolvedReferences
        return isinstance(cur_thread, threading._MainThread)


# noinspection PyPep8Naming
@contextmanager
def suppress_output():
    """suppress both stdout and stderr outputs"""
    if sys.version_info >= (3, 5):
        from contextlib import redirect_stderr, redirect_stdout
    else:

        class _RedirectStream(object):

            _stream = None

            def __init__(self, new_target):
                self._new_target = new_target
                self._old_targets = []

            def __enter__(self):
                self._old_targets.append(getattr(sys, self._stream))
                setattr(sys, self._stream, self._new_target)
                return self._new_target

            def __exit__(self, exctype, excinst, exctb):
                setattr(sys, self._stream, self._old_targets.pop())

        class redirect_stdout(_RedirectStream):
            _stream = "stdout"

        class redirect_stderr(_RedirectStream):
            _stream = "stderr"

    with TemporaryFile("wt") as file:
        with redirect_stdout(file):
            with redirect_stderr(file):
                yield
