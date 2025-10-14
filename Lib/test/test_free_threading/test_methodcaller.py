import unittest
from threading import Thread
from operator import methodcaller


class TestMethodcaller(unittest.TestCase):
    def test_methodcaller_threading(self):
        number_of_threads = 10
        size = 4_000

        mc = methodcaller("append", 2)

        def work(mc, l, ii):
            for _ in range(ii):
                mc(l)

        worker_threads = []
        lists = []
        for ii in range(number_of_threads):
            l = []
            lists.append(l)
            worker_threads.append(Thread(target=work, args=[mc, l, size]))
        for t in worker_threads:
            t.start()
        for t in worker_threads:
            t.join()
        for l in lists:
            assert len(l) == size


if __name__ == "__main__":
    unittest.main()
