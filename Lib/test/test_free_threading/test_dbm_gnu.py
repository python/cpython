import unittest

from test.support import import_helper, os_helper, threading_helper
from test.support.threading_helper import run_concurrently

import threading

gdbm = import_helper.import_module("dbm.gnu")

NTHREADS = 10
KEY_PER_THREAD = 1000

gdbm_filename = "test_gdbm_file"


@threading_helper.requires_working_threading()
class TestGdbm(unittest.TestCase):
    def test_racing_dbm_gnu(self):
        def gdbm_multi_op_worker(db):
            # Each thread sets, gets, and iterates
            tid = threading.get_ident()

            # Insert keys
            for i in range(KEY_PER_THREAD):
                db[f"key_{tid}_{i}"] = f"value_{tid}_{i}"

            for i in range(KEY_PER_THREAD):
                # Keys and values are stored as bytes; encode values for
                # comparison
                key = f"key_{tid}_{i}"
                value = f"value_{tid}_{i}".encode()
                self.assertIn(key, db)
                self.assertEqual(db[key], value)
                self.assertEqual(db.get(key), value)
                self.assertIsNone(db.get("not_exist"))
                with self.assertRaises(KeyError):
                    db["not_exist"]

            # Iterate over the database keys and verify only those belonging
            # to this thread. Other threads may concurrently delete their keys.
            key_prefix = f"key_{tid}".encode()
            key = db.firstkey()
            key_count = 0
            while key:
                if key.startswith(key_prefix):
                    self.assertIn(key, db)
                    key_count += 1
                key = db.nextkey(key)

            # Can't assert key_count == KEY_PER_THREAD because concurrent
            # threads may insert or delete keys during iteration. This can
            # cause keys to be skipped or counted multiple times, making the
            # count unreliable.
            # See: https://www.gnu.org.ua/software/gdbm/manual/Sequential.html
            # self.assertEqual(key_count, KEY_PER_THREAD)

            # Delete this thread's keys
            for i in range(KEY_PER_THREAD):
                key = f"key_{tid}_{i}"
                del db[key]
                self.assertNotIn(key, db)
                with self.assertRaises(KeyError):
                    del db["not_exist"]

            # Re-insert keys
            for i in range(KEY_PER_THREAD):
                db[f"key_{tid}_{i}"] = f"value_{tid}_{i}"

        with os_helper.temp_dir() as tmpdirname:
            db = gdbm.open(f"{tmpdirname}/{gdbm_filename}", "c")
            run_concurrently(
                worker_func=gdbm_multi_op_worker, nthreads=NTHREADS, args=(db,)
            )
            self.assertEqual(len(db), NTHREADS * KEY_PER_THREAD)
            db.close()


if __name__ == "__main__":
    unittest.main()
