import multiprocessing.shared_memory
from threading import Thread
import unittest

from test.support import threading_helper


class TestBase(unittest.TestCase):
    pass


def do_race():
    """Repeatly access the memoryview for racing."""
    n = 100

    obj = multiprocessing.shared_memory.ShareableList("Uq..SeDAmB+EBrkLl.SG.Z+Z.ZdsV..wT+zLxKwdN\b")
    threads = []
    for _ in range(n):
        threads.append(Thread(target=obj.count, args=(1,)))

    for t in threads:
        t.start()

    for t in threads:
        t.join()

    del obj


@threading_helper.requires_working_threading()
class TestMemoryView(TestBase):
    def test_racing_getbuf_and_releasebuf(self):
        do_race()

if __name__ == "__main__":
    unittest.main()
