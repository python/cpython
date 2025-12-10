import unittest

from test.support import import_helper, threading_helper
from test.support.threading_helper import run_concurrently

bz2 = import_helper.import_module("bz2")
from bz2 import BZ2Compressor, BZ2Decompressor

from test.test_bz2 import ext_decompress, BaseTest


NTHREADS = 10
TEXT = BaseTest.TEXT


@threading_helper.requires_working_threading()
class TestBZ2(unittest.TestCase):
    def test_compressor(self):
        bz2c = BZ2Compressor()

        def worker():
            # it should return empty bytes as it buffers data internally
            data = bz2c.compress(TEXT)
            self.assertEqual(data, b"")

        run_concurrently(worker_func=worker, nthreads=NTHREADS)
        data = bz2c.flush()
        # The decompressed data should be TEXT repeated NTHREADS times
        decompressed = ext_decompress(data)
        self.assertEqual(decompressed, TEXT * NTHREADS)

    def test_decompressor(self):
        chunk_size = 128
        chunks = [bytes([ord("a") + i]) * chunk_size for i in range(NTHREADS)]
        input_data = b"".join(chunks)
        compressed = bz2.compress(input_data)

        bz2d = BZ2Decompressor()
        output = []

        def worker():
            data = bz2d.decompress(compressed, chunk_size)
            self.assertEqual(len(data), chunk_size)
            output.append(data)

        run_concurrently(worker_func=worker, nthreads=NTHREADS)
        self.assertEqual(len(output), NTHREADS)
        # Verify the expected chunks (order doesn't matter due to append race)
        self.assertEqual(set(output), set(chunks))


if __name__ == "__main__":
    unittest.main()
