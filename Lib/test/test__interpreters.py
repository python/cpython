import threading
import unittest

from test import support
interpreters = support.import_module('_interpreters')


class InterpretersTests(unittest.TestCase):

    def setUp(self):
        self.ids = []
        self.lock = threading.Lock()

    def tearDown(self):
        for id in self.ids:
            try:
                interpreters.destroy(id)
            except RuntimeError:
                pass  # already destroyed

    def _create(self):
        id = interpreters.create()
        self.ids.append(id)
        return id

    def test_create_in_main(self):
        id = interpreters.create()
        self.ids.append(id)

        self.assertIn(id, interpreters._enumerate())

    def test_create_unique_id(self):
        seen = set()
        for _ in range(100):
            id = self._create()
            interpreters.destroy(id)
            seen.add(id)

        self.assertEqual(len(seen), 100)

    def test_create_in_thread(self):
        id = None
        def f():
            nonlocal id
            id = interpreters.create()
            self.ids.append(id)
            self.lock.acquire()
            self.lock.release()

        t = threading.Thread(target=f)
        with self.lock:
            t.start()
        t.join()
        self.assertIn(id, interpreters._enumerate())

    @unittest.skip('waiting for run_string()')
    def test_create_in_subinterpreter(self):
        raise NotImplementedError

    @unittest.skip('waiting for run_string()')
    def test_create_in_threaded_subinterpreter(self):
        raise NotImplementedError

    def test_create_after_destroy_all(self):
        before = set(interpreters._enumerate())
        # Create 3 subinterpreters.
        ids = []
        for _ in range(3):
            id = interpreters.create()
            ids.append(id)
        # Now destroy them.
        for id in ids:
            interpreters.destroy(id)
        # Finally, create another.
        id = interpreters.create()
        self.ids.append(id)
        self.assertEqual(set(interpreters._enumerate()), before | {id})

    def test_create_after_destroy_some(self):
        before = set(interpreters._enumerate())
        # Create 3 subinterpreters.
        id1 = interpreters.create()
        id2 = interpreters.create()
        self.ids.append(id2)
        id3 = interpreters.create()
        # Now destroy 2 of them.
        interpreters.destroy(id1)
        interpreters.destroy(id3)
        # Finally, create another.
        id = interpreters.create()
        self.ids.append(id)
        self.assertEqual(set(interpreters._enumerate()), before | {id, id2})

    def test_destroy_one(self):
        id1 = self._create()
        id2 = self._create()
        id3 = self._create()
        self.assertIn(id2, interpreters._enumerate())
        interpreters.destroy(id2)
        self.assertNotIn(id2, interpreters._enumerate())
        self.assertIn(id1, interpreters._enumerate())
        self.assertIn(id3, interpreters._enumerate())

    def test_destroy_all(self):
        before = set(interpreters._enumerate())
        ids = set()
        for _ in range(3):
            id = self._create()
            ids.add(id)
        self.assertEqual(set(interpreters._enumerate()), before | ids)
        for id in ids:
            interpreters.destroy(id)
        self.assertEqual(set(interpreters._enumerate()), before)

    def test_destroy_main(self):
        main, = interpreters._enumerate()
        with self.assertRaises(RuntimeError):
            interpreters.destroy(main)

        def f():
            with self.assertRaises(RuntimeError):
                interpreters.destroy(main)

        t = threading.Thread(target=f)
        t.start()
        t.join()

    def test_destroy_already_destroyed(self):
        id = interpreters.create()
        interpreters.destroy(id)
        with self.assertRaises(RuntimeError):
            interpreters.destroy(id)

    def test_destroy_does_not_exist(self):
        with self.assertRaises(RuntimeError):
            interpreters.destroy(1_000_000)

    def test_destroy_bad_id(self):
        with self.assertRaises(RuntimeError):
            interpreters.destroy(-1)

    @unittest.skip('waiting for run_string()')
    def test_destroy_from_current(self):
        raise NotImplementedError

    @unittest.skip('waiting for run_string()')
    def test_destroy_from_sibling(self):
        raise NotImplementedError

    def test_destroy_from_other_thread(self):
        id = interpreters.create()
        self.ids.append(id)
        def f():
            interpreters.destroy(id)

        t = threading.Thread(target=f)
        t.start()
        t.join()

    @unittest.skip('waiting for run_string()')
    def test_destroy_still_running(self):
        raise NotImplementedError


if __name__ == "__main__":
    unittest.main()
