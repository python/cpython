import errno
import os
import sysconfig
import unittest

from test.support import threading_helper
from test.support.threading_helper import run_concurrently


NTHREADS = 10


def _directory_scan_during_gc(directory, names):
    import gc
    import importlib.machinery
    import json
    import os
    import threading
    import time

    expected = set(names)
    barrier = threading.Barrier(5)
    stop = threading.Event()
    errors = []
    collections = []

    def collect():
        barrier.wait(timeout=15)
        while not stop.is_set() and len(collections) < 512:
            collections.append(gc.collect())
            time.sleep(0.0001)

    def scan():
        barrier.wait(timeout=15)
        for _ in range(256):
            try:
                assert set(os.listdir(directory)) == expected
                with os.scandir(directory) as entries:
                    assert {entry.name for entry in entries} == expected
                finder = importlib.machinery.FileFinder(
                    directory,
                    (importlib.machinery.SourceFileLoader,
                     importlib.machinery.SOURCE_SUFFIXES),
                )
                assert finder.find_spec('fixture') is not None
            except BaseException as error:
                errors.append(repr(error))

    collector = threading.Thread(target=collect)
    workers = [threading.Thread(target=scan) for _ in range(4)]
    collector.start()
    for worker in workers:
        worker.start()
    try:
        for worker in workers:
            worker.join(timeout=30)
    finally:
        stop.set()
        collector.join(timeout=15)
    assert not collector.is_alive()
    assert all(not worker.is_alive() for worker in workers)
    assert collections
    return json.dumps(errors)


@threading_helper.requires_working_threading()
class TestOs(unittest.TestCase):
    def test_directory_io_during_subinterpreter_gc(self):
        from concurrent import interpreters
        from concurrent.futures import ThreadPoolExecutor
        import json
        from pathlib import Path
        import tempfile

        names = ('fixture.py',) + tuple(f'file_{n:02d}.txt'
                                       for n in range(32))
        with tempfile.TemporaryDirectory() as directory:
            for name in names:
                Path(directory, name).write_text('VALUE = 73\n')
            for _ in range(16):
                children = [interpreters.create() for _ in range(4)]
                try:
                    with ThreadPoolExecutor(max_workers=4) as pool:
                        futures = [pool.submit(child.call,
                                               _directory_scan_during_gc,
                                               directory, names)
                                   for child in children]
                        results = [future.result(timeout=90)
                                   for future in futures]
                finally:
                    for child in children:
                        child.close()
                for result in results:
                    errors = json.loads(result)
                    self.assertFalse(errors, errors[:3])

    @unittest.skipUnless(sysconfig.get_config_var('_Py_HAVE_STRERROR_R'),
                         'need _Py_HAVE_STRERROR_R macro')
    def test_strerror(self):
        # gh-158893: os.strerror() is implemented with strerror_r() which is
        # thread safe. Well, check if it's actually the case.
        last_error = max([getattr(errno, name) for name in dir(errno)
                          if name.startswith('E')])
        test_errors = tuple(range(1, last_error + 1))
        loops = 20

        def worker():
            for _ in range(loops):
                for i in test_errors:
                    os.strerror(i)

        run_concurrently(
            worker_func=worker, nthreads=NTHREADS
        )


if __name__ == "__main__":
    unittest.main()
