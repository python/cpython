import unittest

from test.support import import_helper, threading_helper
from test.support.threading_helper import run_concurrently

winreg = import_helper.import_module("winreg")

NTHREADS = 4
NKEYS = 1000


def open_keys():
    return [winreg.OpenKey(winreg.HKEY_CURRENT_USER, "Software")
            for _ in range(NKEYS)]


@threading_helper.requires_working_threading()
class WinregTests(unittest.TestCase):
    def test_close(self):
        keys = open_keys()

        def close(close_func):
            for key in keys:
                close_func(key)

        run_concurrently(close, nthreads=NTHREADS,
                         args=(winreg.HKEYType.Close,))
        keys = open_keys()
        run_concurrently(close, nthreads=NTHREADS, args=(winreg.CloseKey,))

    def test_detach_and_close(self):
        keys = open_keys()
        detached = []

        def detach():
            for key in keys:
                detached.append(key.Detach())

        def close():
            for key in keys:
                key.Close()

        run_concurrently([detach, close])
        for handle in detached:
            if handle:
                winreg.CloseKey(handle)


if __name__ == "__main__":
    unittest.main()
