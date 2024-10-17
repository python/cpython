from concurrent.futures import ProcessPoolExecutor, ThreadPoolExecutor
from multiprocessing import Manager
import time
import unittest

from .util import BaseTestCase, setup_module


class PoolExecutorTest(BaseTestCase):
    def test_map_buffersize(self):
        manager = Manager()
        for ExecutorType in (ThreadPoolExecutor, ProcessPoolExecutor):
            with ExecutorType(max_workers=1) as pool:
                with self.assertRaisesRegex(
                    ValueError, "buffersize must be None or >= 1."
                ):
                    pool.map(bool, [], buffersize=0)
            with ExecutorType(max_workers=1) as pool:
                with self.assertRaisesRegex(
                    ValueError, "cannot specify both buffersize and timeout."
                ):
                    pool.map(bool, [], timeout=1, buffersize=1)

            for buffersize, iterable_size in [
                (1, 5),
                (5, 5),
                (10, 5),
            ]:
                iterable = range(iterable_size)
                processed_elements = manager.list()
                with ExecutorType(max_workers=1) as pool:
                    iterator = pool.map(
                        processed_elements.append, iterable, buffersize=buffersize
                    )
                    time.sleep(1)  # wait for buffered futures to finish
                    self.assertSetEqual(
                        set(processed_elements),
                        set(range(min(buffersize, iterable_size))),
                    )
                    next(iterator)
                    time.sleep(1)  # wait for the created future to finish
                    self.assertSetEqual(
                        set(processed_elements),
                        set(range(min(buffersize + 1, iterable_size))),
                    )


def setUpModule():
    setup_module()


if __name__ == "__main__":
    unittest.main()
