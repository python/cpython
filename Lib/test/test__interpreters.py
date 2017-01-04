import contextlib
import os
import os.path
import shutil
import tempfile
from textwrap import dedent
import threading
import unittest

from test import support
from test.support import script_helper

interpreters = support.import_module('_interpreters')


SCRIPT_THREADED_INTERP = """\
import threading
import _interpreters
def f():
    _interpreters.run_string(id, {!r})

t = threading.Thread(target=f)
t.start()
"""


@contextlib.contextmanager
def _blocked(dirname):
    filename = os.path.join(dirname, '.lock')
    wait_script = dedent("""
        import os.path
        import time
        while not os.path.exists('{}'):
            time.sleep(0.1)
        """).format(filename)
    try:
        yield wait_script
    finally:
        support.create_empty_file(filename)


class InterpreterTests(unittest.TestCase):

    def setUp(self):
        self.dirname = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self.dirname)

    def test_still_running_at_exit(self):
        script = SCRIPT_THREADED_INTERP.format("""if True:
            import time
            # Give plenty of time for the main interpreter to finish.
            time.sleep(1_000_000)
            """)
        filename = script_helper.make_script(self.dirname, 'interp', script)
        proc = script_helper.spawn_python(filename)
        retcode = proc.wait()

        self.assertEqual(retcode, 0)


class TestBase(unittest.TestCase):

    def tearDown(self):
        for id in interpreters._enumerate():
            if id == 0:  # main
                continue
            try:
                interpreters.destroy(id)
            except RuntimeError:
                pass  # already destroyed


class CreateTests(TestBase):

    def test_in_main(self):
        id = interpreters.create()

        self.assertIn(id, interpreters._enumerate())

    def test_unique_id(self):
        seen = set()
        for _ in range(100):
            id = interpreters.create()
            interpreters.destroy(id)
            seen.add(id)

        self.assertEqual(len(seen), 100)

    def test_in_thread(self):
        lock = threading.Lock()
        id = None
        def f():
            nonlocal id
            id = interpreters.create()
            lock.acquire()
            lock.release()

        t = threading.Thread(target=f)
        with lock:
            t.start()
        t.join()
        self.assertIn(id, interpreters._enumerate())

    def test_in_subinterpreter(self):
        main, = interpreters._enumerate()
        id1 = interpreters.create()
        ns = interpreters.run_string_unrestricted(id1, dedent("""
            import _interpreters
            id = _interpreters.create()
            """))
        id2 = ns['id']

        self.assertEqual(set(interpreters._enumerate()), {main, id1, id2})

    def test_in_threaded_subinterpreter(self):
        main, = interpreters._enumerate()
        id1 = interpreters.create()
        ns = None
        script = dedent("""
            import _interpreters
            id = _interpreters.create()
            """)
        def f():
            nonlocal ns
            ns = interpreters.run_string_unrestricted(id1, script)

        t = threading.Thread(target=f)
        t.start()
        t.join()
        id2 = ns['id']

        self.assertEqual(set(interpreters._enumerate()), {main, id1, id2})


    def test_after_destroy_all(self):
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
        self.assertEqual(set(interpreters._enumerate()), before | {id})

    def test_after_destroy_some(self):
        before = set(interpreters._enumerate())
        # Create 3 subinterpreters.
        id1 = interpreters.create()
        id2 = interpreters.create()
        id3 = interpreters.create()
        # Now destroy 2 of them.
        interpreters.destroy(id1)
        interpreters.destroy(id3)
        # Finally, create another.
        id = interpreters.create()
        self.assertEqual(set(interpreters._enumerate()), before | {id, id2})


class DestroyTests(TestBase):

    def test_one(self):
        id1 = interpreters.create()
        id2 = interpreters.create()
        id3 = interpreters.create()
        self.assertIn(id2, interpreters._enumerate())
        interpreters.destroy(id2)
        self.assertNotIn(id2, interpreters._enumerate())
        self.assertIn(id1, interpreters._enumerate())
        self.assertIn(id3, interpreters._enumerate())

    def test_all(self):
        before = set(interpreters._enumerate())
        ids = set()
        for _ in range(3):
            id = interpreters.create()
            ids.add(id)
        self.assertEqual(set(interpreters._enumerate()), before | ids)
        for id in ids:
            interpreters.destroy(id)
        self.assertEqual(set(interpreters._enumerate()), before)

    def test_main(self):
        main, = interpreters._enumerate()
        with self.assertRaises(RuntimeError):
            interpreters.destroy(main)

        def f():
            with self.assertRaises(RuntimeError):
                interpreters.destroy(main)

        t = threading.Thread(target=f)
        t.start()
        t.join()

    def test_already_destroyed(self):
        id = interpreters.create()
        interpreters.destroy(id)
        with self.assertRaises(RuntimeError):
            interpreters.destroy(id)

    def test_does_not_exist(self):
        with self.assertRaises(RuntimeError):
            interpreters.destroy(1_000_000)

    def test_bad_id(self):
        with self.assertRaises(RuntimeError):
            interpreters.destroy(-1)

    def test_from_current(self):
        main, = interpreters._enumerate()
        id = interpreters.create()
        script = dedent("""
            import _interpreters
            _interpreters.destroy({})
            """).format(id)

        with self.assertRaises(RuntimeError):
            interpreters.run_string(id, script)
        self.assertEqual(set(interpreters._enumerate()), {main, id})

    def test_from_sibling(self):
        main, = interpreters._enumerate()
        id1 = interpreters.create()
        id2 = interpreters.create()
        script = dedent("""
            import _interpreters
            _interpreters.destroy({})
            """).format(id2)
        interpreters.run_string(id1, script)

        self.assertEqual(set(interpreters._enumerate()), {main, id1})

    def test_from_other_thread(self):
        id = interpreters.create()
        def f():
            interpreters.destroy(id)

        t = threading.Thread(target=f)
        t.start()
        t.join()

    def test_still_running(self):
        # XXX Rewrite this test without files by using
        # run_string_unrestricted().
        main, = interpreters._enumerate()
        id = interpreters.create()
        def f():
            interpreters.run_string(id, wait_script)

        dirname = tempfile.mkdtemp()
        t = threading.Thread(target=f)
        with _blocked(dirname) as wait_script:
            t.start()
            with self.assertRaises(RuntimeError):
                interpreters.destroy(id)

        t.join()
        self.assertEqual(set(interpreters._enumerate()), {main, id})


class RunStringTests(TestBase):

    SCRIPT = dedent("""
        with open('{}', 'w') as out:
            out.write('{}')
        """)
    FILENAME = 'spam'

    def setUp(self):
        self.id = interpreters.create()
        self.dirname = None
        self.filename = None

    def tearDown(self):
        if self.dirname is not None:
            try:
                shutil.rmtree(self.dirname)
            except FileNotFoundError:
                pass  # already deleted
        super().tearDown()

    def _resolve_filename(self, name=None):
        if name is None:
            name = self.FILENAME
        if self.dirname is None:
            self.dirname = tempfile.mkdtemp()
        return os.path.join(self.dirname, name)

    def _empty_file(self):
        self.filename = self._resolve_filename()
        support.create_empty_file(self.filename)
        return self.filename

    def assert_file_contains(self, expected, filename=None):
        if filename is None:
            filename = self.filename
        self.assertIsNot(filename, None)
        with open(filename) as out:
            content = out.read()
        self.assertEqual(content, expected)

    def test_success(self):
        filename = self._empty_file()
        expected = 'spam spam spam spam spam'
        script = self.SCRIPT.format(filename, expected)
        interpreters.run_string(self.id, script)

        self.assert_file_contains(expected)

    def test_in_thread(self):
        filename = self._empty_file()
        expected = 'spam spam spam spam spam'
        script = self.SCRIPT.format(filename, expected)
        def f():
            interpreters.run_string(self.id, script)

        t = threading.Thread(target=f)
        t.start()
        t.join()

        self.assert_file_contains(expected)

    def test_create_thread(self):
        filename = self._empty_file()
        expected = 'spam spam spam spam spam'
        script = dedent("""
            import threading
            def f():
                with open('{}', 'w') as out:
                    out.write('{}')

            t = threading.Thread(target=f)
            t.start()
            t.join()
            """).format(filename, expected)
        interpreters.run_string(self.id, script)

        self.assert_file_contains(expected)

    @unittest.skipUnless(hasattr(os, 'fork'), "test needs os.fork()")
    def test_fork(self):
        filename = self._empty_file()
        expected = 'spam spam spam spam spam'
        script = dedent("""
            import os
            r, w = os.pipe()
            pid = os.fork()
            if pid == 0:  # child
                import sys
                filename = '{}'
                with open(filename, 'w') as out:
                    out.write('{}')
                os.write(w, b'done!')

                # Kill the unittest runner in the child process.
                os._exit(1)
            else:
                import select
                try:
                    select.select([r], [], [])
                finally:
                    os.close(r)
                    os.close(w)
            """).format(filename, expected)
        interpreters.run_string(self.id, script)
        self.assert_file_contains(expected)

    def test_already_running(self):
        def f():
            interpreters.run_string(self.id, wait_script)

        t = threading.Thread(target=f)
        dirname = tempfile.mkdtemp()
        with _blocked(dirname) as wait_script:
            t.start()
            with self.assertRaises(RuntimeError):
                interpreters.run_string(self.id, 'print("spam")')
        t.join()

    def test_does_not_exist(self):
        id = 0
        while id in interpreters._enumerate():
            id += 1
        with self.assertRaises(RuntimeError):
            interpreters.run_string(id, 'print("spam")')

    def test_error_id(self):
        with self.assertRaises(RuntimeError):
            interpreters.run_string(-1, 'print("spam")')

    def test_bad_id(self):
        with self.assertRaises(TypeError):
            interpreters.run_string('spam', 'print("spam")')

    def test_bad_code(self):
        with self.assertRaises(TypeError):
            interpreters.run_string(self.id, 10)

    def test_bytes_for_code(self):
        with self.assertRaises(TypeError):
            interpreters.run_string(self.id, b'print("spam")')

    def test_invalid_syntax(self):
        with self.assertRaises(SyntaxError):
            # missing close paren
            interpreters.run_string(self.id, 'print("spam"')

    def test_failure(self):
        with self.assertRaises(Exception) as caught:
            interpreters.run_string(self.id, 'raise Exception("spam")')
        self.assertEqual(str(caught.exception), 'spam')

    def test_sys_exit(self):
        with self.assertRaises(SystemExit) as cm:
            interpreters.run_string(self.id, dedent("""
                import sys
                sys.exit()
                """))
        self.assertIsNone(cm.exception.code)

        with self.assertRaises(SystemExit) as cm:
            interpreters.run_string(self.id, dedent("""
                import sys
                sys.exit(42)
                """))
        self.assertEqual(cm.exception.code, 42)

    def test_SystemError(self):
        with self.assertRaises(SystemExit) as cm:
            interpreters.run_string(self.id, 'raise SystemExit(42)')
        self.assertEqual(cm.exception.code, 42)


class RunStringUnrestrictedTests(TestBase):

    def setUp(self):
        self.id = interpreters.create()

    def test_without_ns(self):
        script = dedent("""
            spam = 42
            """)
        ns = interpreters.run_string_unrestricted(self.id, script)

        self.assertEqual(ns['spam'], 42)

    def test_with_ns(self):
        updates = {'spam': 'ham', 'eggs': -1}
        script = dedent("""
            spam = 42
            result = spam + eggs
            """)
        ns = interpreters.run_string_unrestricted(self.id, script, updates)

        self.assertEqual(ns['spam'], 42)
        self.assertEqual(ns['eggs'], -1)
        self.assertEqual(ns['result'], 41)

    def test_ns_does_not_overwrite(self):
        updates = {'__name__': 'not __main__'}
        script = dedent("""
            spam = 42
            """)
        ns = interpreters.run_string_unrestricted(self.id, script, updates)

        self.assertEqual(ns['__name__'], '__main__')

    def test_return_execution_namespace(self):
        script = dedent("""
            spam = 42
            """)
        ns = interpreters.run_string_unrestricted(self.id, script)

        ns.pop('__builtins__')
        ns.pop('__loader__')
        self.assertEqual(ns, {
            '__name__': '__main__',
            '__annotations__': {},
            '__doc__': None,
            '__package__': None,
            '__spec__': None,
            'spam': 42,
            })


if __name__ == '__main__':
    unittest.main()
