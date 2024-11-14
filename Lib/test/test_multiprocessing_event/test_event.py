import unittest
from test import support
import sys
import signal
import os


try:
    import multiprocessing
    from concurrent.futures import ProcessPoolExecutor
    _have_multiprocessing = True
except (NotImplementedError, ModuleNotFoundError):
    _have_multiprocessing = False


@unittest.skipUnless(_have_multiprocessing,
                     "requires multiprocessing")
@unittest.skipUnless(hasattr(signal, 'signal'),
                     "Requires signal.signal")
@unittest.skipUnless(hasattr(signal, 'SIGINT'),
                     "Requires signal.SIGINT")
@unittest.skipUnless(hasattr(os, 'kill'),
                     "Requires os.kill")
@unittest.skipUnless(hasattr(os, 'getppid'),
                     "Requires os.getppid")
@support.requires_subprocess()
class TestEventSignalHandling(unittest.TestCase):
    def test_no_race_for_is_set_set(self):
        import subprocess
        script = support.findfile("is_set_set.py", subdir="multiprocessingdata")
        for x in range(10):
            try:
                assert 0 == subprocess.call([sys.executable, script], timeout=60)
            except subprocess.TimeoutExpired:
                assert False, 'subprocess.Timeoutexpired for is_set_set.py'

    def test_no_race_set_clear(self):
        import subprocess
        script = support.findfile("set_clear_race.py", subdir="multiprocessingdata")
        assert 0 == subprocess.call([sys.executable, script])

    def test_wait_set_no_deadlock(self):
        import subprocess
        script = support.findfile("wait_set_no_deadlock.py", subdir="multiprocessingdata")
        assert 0 == subprocess.call([sys.executable, script])

    def test_wait_timeout(self):
        event = multiprocessing.Event()
        # https://docs.python.org/3/library/multiprocessing.html#multiprocessing.Event
        # multiprocessing.Event: A clone of threading.Event.

        # threading.Event: https://docs.python.org/3/library/threading.html#threading.Event

        # threading.Event.wait(): https://docs.python.org/3/library/threading.html#threading.Event.wait
        # Block as long as the internal flag is false and the timeout, if given, has not expired.
        # The return value represents the reason that this blocking method returned;
        # True if returning because the internal flag is set to true, or
        # False if a timeout is given and the internal flag did not become true within the given wait time.

        # When the timeout argument is present and not None, it should be a floating-point number
        # specifying a timeout for the operation in seconds, or fractions thereof.

        # wait() supports both integer and float:
        flag_set = event.wait(1)
        assert flag_set == False

        flag_set = event.wait(0.1)
        assert flag_set == False


if __name__ == '__main__':
    unittest.main()

