import unittest

from test.support import import_helper, threading_helper
from test.support.threading_helper import run_concurrently

lzma = import_helper.import_module("lzma")
from lzma import LZMACompressor, LZMADecompressor

from test.test_lzma import INPUT


NTHREADS = 10


@threading_helper.requires_working_threading()
class TestLZMA(unittest.TestCase):
    def test_compressor(self):
        lzc = LZMACompressor()

        # First compress() outputs LZMA header
        header = lzc.compress(INPUT)
        self.assertGreater(len(header), 0)

        def worker():
            # it should return empty bytes as it buffers data internally
            data = lzc.compress(INPUT)
            self.assertEqual(data, b"")

        run_concurrently(worker_func=worker, nthreads=NTHREADS - 1)
        full_compressed = header + lzc.flush()
        decompressed = lzma.decompress(full_compressed)
        # The decompressed data should be INPUT repeated NTHREADS times
        self.assertEqual(decompressed, INPUT * NTHREADS)

    def test_decompressor(self):
        chunk_size = 128
        chunks = [bytes([ord("a") + i]) * chunk_size for i in range(NTHREADS)]
        input_data = b"".join(chunks)
        compressed = lzma.compress(input_data)

        lzd = LZMADecompressor()
        output = []

        def worker():
            data = lzd.decompress(compressed, chunk_size)
            self.assertEqual(len(data), chunk_size)
            output.append(data)

        run_concurrently(worker_func=worker, nthreads=NTHREADS)
        self.assertEqual(len(output), NTHREADS)
        # Verify the expected chunks (order doesn't matter due to append race)
        self.assertSetEqual(set(output), set(chunks))


if __name__ == "__main__":
    unittest.main()
