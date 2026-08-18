import unittest

from test.support import import_helper, threading_helper
from test.support.threading_helper import run_concurrently

import os
import string
import tempfile
import threading

from collections import Counter

mmap = import_helper.import_module("mmap")

NTHREADS = 10
ANONYMOUS_MEM = -1


@threading_helper.requires_working_threading()
class MmapTests(unittest.TestCase):
    def test_read_and_read_byte(self):
        ascii_uppercase = string.ascii_uppercase.encode()
        # Choose a total mmap size that evenly divides across threads and the
        # read pattern (3 bytes per loop).
        mmap_size = 3 * NTHREADS * len(ascii_uppercase)
        num_bytes_to_read_per_thread = mmap_size // NTHREADS
        bytes_read_from_mmap = []

        def read(mm_obj):
            nread = 0
            while nread < num_bytes_to_read_per_thread:
                b = mm_obj.read_byte()
                bytes_read_from_mmap.append(b)
                b = mm_obj.read(2)
                bytes_read_from_mmap.extend(b)
                nread += 3

        with mmap.mmap(ANONYMOUS_MEM, mmap_size) as mm_obj:
            for i in range(mmap_size // len(ascii_uppercase)):
                mm_obj.write(ascii_uppercase)

            mm_obj.seek(0)
            run_concurrently(
                worker_func=read,
                args=(mm_obj,),
                nthreads=NTHREADS,
            )

        self.assertEqual(len(bytes_read_from_mmap), mmap_size)
        # Count each letter/byte to verify read correctness
        counter = Counter(bytes_read_from_mmap)
        self.assertEqual(len(counter), len(ascii_uppercase))
        # Each letter/byte should be read (3 * NTHREADS) times
        for letter in ascii_uppercase:
            self.assertEqual(counter[letter], 3 * NTHREADS)

    def test_readline(self):
        num_lines = 1000
        lines_read_from_mmap = []
        expected_lines = []

        def readline(mm_obj):
            for i in range(num_lines // NTHREADS):
                line = mm_obj.readline()
                lines_read_from_mmap.append(line)

        # Allocate mmap enough for num_lines (max line 5 bytes including NL)
        with mmap.mmap(ANONYMOUS_MEM, num_lines * 5) as mm_obj:
            for i in range(num_lines):
                line = b"%d\n" % i
                mm_obj.write(line)
                expected_lines.append(line)

            mm_obj.seek(0)
            run_concurrently(
                worker_func=readline,
                args=(mm_obj,),
                nthreads=NTHREADS,
            )

        self.assertEqual(len(lines_read_from_mmap), num_lines)
        # Every line should be read once by threads; order is non-deterministic
        # Sort numerically by integer value
        lines_read_from_mmap.sort(key=lambda x: int(x))
        self.assertEqual(lines_read_from_mmap, expected_lines)

    def test_write_and_write_byte(self):
        thread_letters = list(string.ascii_uppercase)
        self.assertLessEqual(NTHREADS, len(thread_letters))
        per_thread_write_loop = 100

        def write(mm_obj):
            # Each thread picks a unique letter to write
            thread_letter = thread_letters.pop(0)
            thread_bytes = (thread_letter * 2).encode()
            for _ in range(per_thread_write_loop):
                mm_obj.write_byte(thread_bytes[0])
                mm_obj.write(thread_bytes)

        with mmap.mmap(
            ANONYMOUS_MEM, per_thread_write_loop * 3 * NTHREADS
        ) as mm_obj:
            run_concurrently(
                worker_func=write,
                args=(mm_obj,),
                nthreads=NTHREADS,
            )
            mm_obj.seek(0)
            data = mm_obj.read()
            self.assertEqual(len(data), NTHREADS * per_thread_write_loop * 3)
            counter = Counter(data)
            self.assertEqual(len(counter), NTHREADS)
            # Each thread letter should be written `per_thread_write_loop` * 3
            for letter in counter:
                self.assertEqual(counter[letter], per_thread_write_loop * 3)

    def test_move(self):
        ascii_uppercase = string.ascii_uppercase.encode()
        num_letters = len(ascii_uppercase)

        def move(mm_obj):
            for i in range(num_letters):
                # Move 1 byte from the first half to the second half
                mm_obj.move(0 + i, num_letters + i, 1)

        with mmap.mmap(ANONYMOUS_MEM, 2 * num_letters) as mm_obj:
            mm_obj.write(ascii_uppercase)
            run_concurrently(
                worker_func=move,
                args=(mm_obj,),
                nthreads=NTHREADS,
            )

    def test_seek_and_tell(self):
        seek_per_thread = 10

        def seek(mm_obj):
            self.assertTrue(mm_obj.seekable())
            for _ in range(seek_per_thread):
                before_seek = mm_obj.tell()
                mm_obj.seek(1, os.SEEK_CUR)
                self.assertLess(before_seek, mm_obj.tell())

        with mmap.mmap(ANONYMOUS_MEM, 1024) as mm_obj:
            run_concurrently(
                worker_func=seek,
                args=(mm_obj,),
                nthreads=NTHREADS,
            )
            # Each thread seeks from current position, the end position should
            # be the sum of all seeks from all threads.
            self.assertEqual(mm_obj.tell(), NTHREADS * seek_per_thread)

    def test_slice_update_and_slice_read(self):
        thread_letters = list(string.ascii_uppercase)
        self.assertLessEqual(NTHREADS, len(thread_letters))

        def slice_update_and_slice_read(mm_obj):
            # Each thread picks a unique letter to write
            thread_letter = thread_letters.pop(0)
            thread_bytes = (thread_letter * 1024).encode()
            for _ in range(100):
                mm_obj[:] = thread_bytes
                read_bytes = mm_obj[:]
                # Read bytes should be all the same letter, showing no
                # interleaving
                self.assertTrue(all_same(read_bytes))

        with mmap.mmap(ANONYMOUS_MEM, 1024) as mm_obj:
            run_concurrently(
                worker_func=slice_update_and_slice_read,
                args=(mm_obj,),
                nthreads=NTHREADS,
            )

    def test_item_update_and_item_read(self):
        thread_indexes = [i for i in range(NTHREADS)]

        def item_update_and_item_read(mm_obj):
            # Each thread picks a unique index to write
            thread_index = thread_indexes.pop()
            for i in range(100):
                mm_obj[thread_index] = i
                self.assertEqual(mm_obj[thread_index], i)

            # Read values set by other threads, all values
            # should be less than '100'
            for val in mm_obj:
                self.assertLess(int.from_bytes(val), 100)

        with mmap.mmap(ANONYMOUS_MEM, NTHREADS + 1) as mm_obj:
            run_concurrently(
                worker_func=item_update_and_item_read,
                args=(mm_obj,),
                nthreads=NTHREADS,
            )

    @unittest.skipUnless(os.name == "posix", "requires Posix")
    @unittest.skipUnless(hasattr(mmap.mmap, "resize"), "requires mmap.resize")
    def test_resize_and_size(self):
        thread_indexes = [i for i in range(NTHREADS)]

        def resize_and_item_update(mm_obj):
            # Each thread picks a unique index to write
            thread_index = thread_indexes.pop()
            mm_obj.resize(2048)
            self.assertEqual(mm_obj.size(), 2048)
            for i in range(100):
                mm_obj[thread_index] = i
                self.assertEqual(mm_obj[thread_index], i)

        with mmap.mmap(ANONYMOUS_MEM, 1024, flags=mmap.MAP_PRIVATE) as mm_obj:
            run_concurrently(
                worker_func=resize_and_item_update,
                args=(mm_obj,),
                nthreads=NTHREADS,
            )

    def test_close_and_closed(self):
        def close_mmap(mm_obj):
            mm_obj.close()
            self.assertTrue(mm_obj.closed)

        with mmap.mmap(ANONYMOUS_MEM, 1) as mm_obj:
            run_concurrently(
                worker_func=close_mmap,
                args=(mm_obj,),
                nthreads=NTHREADS,
            )

    def test_find_and_rfind(self):
        per_thread_loop = 10

        def find_and_rfind(mm_obj):
            pattern = b'Thread-Ident:"%d"' % threading.get_ident()
            mm_obj.write(pattern)
            for _ in range(per_thread_loop):
                found_at = mm_obj.find(pattern, 0)
                self.assertNotEqual(found_at, -1)
                # Should not find it after the `found_at`
                self.assertEqual(mm_obj.find(pattern, found_at + 1), -1)
                found_at_rev = mm_obj.rfind(pattern, 0)
                self.assertEqual(found_at, found_at_rev)
                # Should not find it after the `found_at`
                self.assertEqual(mm_obj.rfind(pattern, found_at + 1), -1)

        with mmap.mmap(ANONYMOUS_MEM, 1024) as mm_obj:
            run_concurrently(
                worker_func=find_and_rfind,
                args=(mm_obj,),
                nthreads=NTHREADS,
            )

    @unittest.skipUnless(os.name == "posix", "requires Posix")
    @unittest.skipUnless(hasattr(mmap.mmap, "resize"), "requires mmap.resize")
    def test_flush(self):
        mmap_filename = "test_mmap_file"
        resize_to = 1024

        def resize_and_flush(mm_obj):
            mm_obj.resize(resize_to)
            mm_obj.flush()

        with tempfile.TemporaryDirectory() as tmpdirname:
            file_path = f"{tmpdirname}/{mmap_filename}"
            with open(file_path, "wb+") as file:
                file.write(b"CPython")
                file.flush()
                with mmap.mmap(file.fileno(), 1) as mm_obj:
                    run_concurrently(
                        worker_func=resize_and_flush,
                        args=(mm_obj,),
                        nthreads=NTHREADS,
                    )

            self.assertEqual(os.path.getsize(file_path), resize_to)

    def test_mmap_export_as_memoryview(self):
        """
        Each thread creates a memoryview and updates the internal state of the
        mmap object.
        """
        buffer_size = 42

        def create_memoryview_from_mmap(mm_obj):
            memoryviews = []
            for _ in range(100):
                mv = memoryview(mm_obj)
                memoryviews.append(mv)
                self.assertEqual(len(mv), buffer_size)
                self.assertEqual(mv[:7], b"CPython")

            # Cannot close the mmap while it is exported as buffers
            with self.assertRaisesRegex(
                BufferError, "cannot close exported pointers exist"
            ):
                mm_obj.close()

        with mmap.mmap(ANONYMOUS_MEM, 42) as mm_obj:
            mm_obj.write(b"CPython")
            run_concurrently(
                worker_func=create_memoryview_from_mmap,
                args=(mm_obj,),
                nthreads=NTHREADS,
            )
            # Implicit mm_obj.close() verifies all exports (memoryviews) are
            # properly freed.


def all_same(lst):
    return all(item == lst[0] for item in lst)


if __name__ == "__main__":
    unittest.main()
