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
    def test_no_race_for_set_is_set(self):
        import subprocess
        script = support.findfile("set_is_set.py", subdir="multiprocessingdata")
        for x in range(10):
            try:
                assert 0 == subprocess.call([sys.executable, script], timeout=60)
            except subprocess.TimeoutExpired:
                assert False, 'subprocess.Timeoutexpired for set_is_set.py'

    def test_no_race_set_clear(self):
        import subprocess
        script = support.findfile("set_clear_race.py", subdir="multiprocessingdata")
        assert 0 == subprocess.call([sys.executable, script])

    def test_wait_set_throws(self):
        import subprocess
        script = support.findfile("wait_set_throws.py", subdir="multiprocessingdata")
        assert 0 == subprocess.call([sys.executable, script])


if __name__ == '__main__':
    unittest.main()

