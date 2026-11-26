import csv
import io
import unittest

from test.support import threading_helper
from test.support.threading_helper import run_concurrently


NTHREADS = 10


@threading_helper.requires_working_threading()
class TestCSV(unittest.TestCase):
    def test_concurrent_reader_next(self):
        input_rows = [f"{i},{i},{i}" for i in range(50)]
        input_stream = io.StringIO("\n".join(input_rows))
        reader = csv.reader(input_stream)
        output_rows = []

        def read_row():
            for row in reader:
                self.assertEqual(len(row), 3)
                output_rows.append(",".join(row))

        run_concurrently(worker_func=read_row, nthreads=NTHREADS)
        self.assertSetEqual(set(input_rows), set(output_rows))

    def test_concurrent_writer_writerow(self):
        output_stream = io.StringIO()
        writer = csv.writer(output_stream)
        row_per_thread = 10
        expected_rows = []

        def write_row():
            for i in range(row_per_thread):
                writer.writerow([i, i, i])
                expected_rows.append(f"{i},{i},{i}")

        run_concurrently(worker_func=write_row, nthreads=NTHREADS)

        # Rewind to the start of the stream and parse the rows
        output_stream.seek(0)
        output_rows = [line.strip() for line in output_stream.readlines()]

        self.assertEqual(len(output_rows), NTHREADS * row_per_thread)
        self.assertListEqual(sorted(output_rows), sorted(expected_rows))


if __name__ == "__main__":
    unittest.main()
