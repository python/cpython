import os
import unittest

from test.support import import_helper, threading_helper
from test.support.threading_helper import run_concurrently
from uuid import SafeUUID

c_uuid = import_helper.import_module("_uuid")

NTHREADS = 10
UUID_PER_THREAD = 1000


@threading_helper.requires_working_threading()
class UUIDTests(unittest.TestCase):
    @unittest.skipUnless(os.name == "posix", "POSIX only")
    def test_generate_time_safe(self):
        uuids = []

        def worker():
            local_uuids = []
            for _ in range(UUID_PER_THREAD):
                uuid, is_safe = c_uuid.generate_time_safe()
                self.assertIs(type(uuid), bytes)
                self.assertEqual(len(uuid), 16)
                # Collect the UUID only if it is safe. If not, we cannot ensure
                # UUID uniqueness. According to uuid_generate_time_safe() man
                # page, it is theoretically possible for two concurrently
                # running processes to generate the same UUID(s) if the return
                # value is not 0.
                if is_safe == SafeUUID.safe:
                    local_uuids.append(uuid)

            # Merge all safe uuids
            uuids.extend(local_uuids)

        run_concurrently(worker_func=worker, nthreads=NTHREADS)
        self.assertEqual(len(uuids), len(set(uuids)))

    @unittest.skipUnless(os.name == "nt", "Windows only")
    def test_UuidCreate(self):
        uuids = []

        def worker():
            local_uuids = []
            for _ in range(UUID_PER_THREAD):
                uuid = c_uuid.UuidCreate()
                self.assertIs(type(uuid), bytes)
                self.assertEqual(len(uuid), 16)
                local_uuids.append(uuid)

            # Merge all uuids
            uuids.extend(local_uuids)

        run_concurrently(worker_func=worker, nthreads=NTHREADS)
        self.assertEqual(len(uuids), len(set(uuids)))


if __name__ == "__main__":
    unittest.main()
