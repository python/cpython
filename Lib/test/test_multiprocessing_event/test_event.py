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


class TestEventSignalHandling(unittest.TestCase):
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
    def test_event_signal_handling(self):
        import subprocess
        script = support.findfile("event_signal.py", subdir="multiprocessingdata")
        for x in range(10):
            try:
                exit_code = subprocess.call([sys.executable, script], stdout=subprocess.DEVNULL, timeout=30)
                assert exit_code == 0
            except subprocess.TimeoutExpired:
                assert False, 'subprocess.Timeoutexpired for event_signal.py'


if __name__ == '__main__':
    unittest.main()

