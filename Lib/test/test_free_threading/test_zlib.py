import itertools
import unittest

from test.support import import_helper, threading_helper
from test.support.threading_helper import run_concurrently

zlib = import_helper.import_module("zlib")

from test.test_zlib import HAMLET_SCENE


NTHREADS = 10


@threading_helper.requires_working_threading()
class TestZlib(unittest.TestCase):
    def test_compressor(self):
        comp = zlib.compressobj()

        # First compress() outputs zlib header
        header = comp.compress(HAMLET_SCENE)
        self.assertGreater(len(header), 0)

        def worker():
            # it should return empty bytes as it buffers data internally
            data = comp.compress(HAMLET_SCENE)
            self.assertEqual(data, b"")

        run_concurrently(worker_func=worker, nthreads=NTHREADS - 1)
        full_compressed = header + comp.flush()
        decompressed = zlib.decompress(full_compressed)
        # The decompressed data should be HAMLET_SCENE repeated NTHREADS times
        self.assertEqual(decompressed, HAMLET_SCENE * NTHREADS)

    def test_decompressor_concurrent_attribute_reads(self):
        input_data = HAMLET_SCENE * NTHREADS
        compressed = zlib.compress(input_data)

        decomp = zlib.decompressobj()
        decomp_size_per_loop = len(input_data) // 1000
        decompressed_parts = []

        def decomp_worker():
            # Decompress in chunks, which updates eof, unused_data, unconsumed_tail
            decompressed_parts.append(
                decomp.decompress(compressed, decomp_size_per_loop)
            )
            while decomp.unconsumed_tail:
                decompressed_parts.append(
                    decomp.decompress(
                        decomp.unconsumed_tail, decomp_size_per_loop
                    )
                )

        def decomp_attr_reader():
            # Read attributes concurrently while another thread decompresses
            for _ in range(1000):
                _ = decomp.unused_data
                _ = decomp.unconsumed_tail
                _ = decomp.eof

        counter = itertools.count()

        def worker():
            # First thread decompresses, others read attributes
            if next(counter) == 0:
                decomp_worker()
            else:
                decomp_attr_reader()

        run_concurrently(worker_func=worker, nthreads=NTHREADS)

        self.assertTrue(decomp.eof)
        self.assertEqual(decomp.unused_data, b"")
        decompressed = b"".join(decompressed_parts)
        self.assertEqual(decompressed, HAMLET_SCENE * NTHREADS)


if __name__ == "__main__":
    unittest.main()
