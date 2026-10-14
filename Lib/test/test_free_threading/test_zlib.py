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
        # Also exercise intermediate output with large uncompressed blocks.
        for level, input_data in (
            (zlib.Z_DEFAULT_COMPRESSION, HAMLET_SCENE),
            (zlib.Z_NO_COMPRESSION, HAMLET_SCENE * 100),
        ):
            with self.subTest(level=level):
                comp = zlib.compressobj(level)
                compressed_parts = []

                def worker():
                    compressed_parts.append(comp.compress(input_data))

                # Calls are serialized by the compressor, but threads may
                # append their output in a different order. Use two threads
                # so that both possible output orders can be checked.
                nthreads = 2
                run_concurrently(worker_func=worker, nthreads=nthreads)
                self.assertEqual(len(compressed_parts), nthreads)
                final_part = comp.flush()
                expected_data = input_data * nthreads

                for parts in itertools.permutations(compressed_parts):
                    full_compressed = b"".join(parts) + final_part
                    decomp = zlib.decompressobj()
                    try:
                        decompressed = decomp.decompress(full_compressed)
                    except zlib.error:
                        continue
                    if (decomp.eof and not decomp.unused_data
                            and decompressed == expected_data):
                        break
                else:
                    self.fail("No output order reproduces the input")

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
