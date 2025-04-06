import contextlib
import itertools
import os
import pickle
import sys
from textwrap import dedent
import threading
import unittest

from test import support
from test.support import import_helper
from test.support import script_helper


interpreters = import_helper.import_module('_xxsubinterpreters')
_testcapi = import_helper.import_module('_testcapi')

##################################
# helpers

def _captured_script(script):
    r, w = os.pipe()
    indented = script.replace('\n', '\n                ')
    wrapped = dedent(f"""
        import contextlib
        with open({w}, 'w', encoding="utf-8") as spipe:
            with contextlib.redirect_stdout(spipe):
                {indented}
        """)
    return wrapped, open(r, encoding="utf-8")


def _run_output(interp, request, shared=None):
    script, rpipe = _captured_script(request)
    with rpipe:
        interpreters.run_string(interp, script, shared)
        return rpipe.read()


def _wait_for_interp_to_run(interp, timeout=None):
    # bpo-37224: Running this test file in multiprocesses will fail randomly.
    # The failure reason is that the thread can't acquire the cpu to
    # run subinterpreter eariler than the main thread in multiprocess.
    if timeout is None:
        timeout = support.SHORT_TIMEOUT
    for _ in support.sleeping_retry(timeout, error=False):
        if interpreters.is_running(interp):
            break
    else:
        raise RuntimeError('interp is not running')


@contextlib.contextmanager
def _running(interp):
    r, w = os.pipe()
    def run():
        interpreters.run_string(interp, dedent(f"""
            # wait for "signal"
            with open({r}, encoding="utf-8") as rpipe:
                rpipe.read()
            """))

    t = threading.Thread(target=run)
    t.start()
    _wait_for_interp_to_run(interp)

    yield

    with open(w, 'w', encoding="utf-8") as spipe:
        spipe.write('done')
    t.join()


def clean_up_interpreters():
    for id in interpreters.list_all():
        if id == 0:  # main
            continue
        try:
            interpreters.destroy(id)
        except RuntimeError:
            pass  # already destroyed


class TestBase(unittest.TestCase):

    def tearDown(self):
        clean_up_interpreters()


##################################
# misc. tests

class IsShareableTests(unittest.TestCase):

    def test_default_shareables(self):
        shareables = [
                # singletons
                None,
                # builtin objects
                b'spam',
                'spam',
                10,
                -10,
                ]
        for obj in shareables:
            with self.subTest(obj):
                self.assertTrue(
                    interpreters.is_shareable(obj))

    def test_not_shareable(self):
        class Cheese:
            def __init__(self, name):
                self.name = name
            def __str__(self):
                return self.name

        class SubBytes(bytes):
            """A subclass of a shareable type."""

        not_shareables = [
                # singletons
                True,
                False,
                NotImplemented,
                ...,
                # builtin types and objects
                type,
                object,
                object(),
                Exception(),
                100.0,
                # user-defined types and objects
                Cheese,
                Cheese('Wensleydale'),
                SubBytes(b'spam'),
                ]
        for obj in not_shareables:
            with self.subTest(repr(obj)):
                self.assertFalse(
                    interpreters.is_shareable(obj))


class ShareableTypeTests(unittest.TestCase):

    def _assert_values(self, values):
        for obj in values:
            with self.subTest(obj):
                xid = _testcapi.get_crossinterp_data(obj)
                got = _testcapi.restore_crossinterp_data(xid)

                self.assertEqual(got, obj)
                self.assertIs(type(got), type(obj))

    def test_singletons(self):
        for obj in [None]:
            with self.subTest(obj):
                xid = _testcapi.get_crossinterp_data(obj)
                got = _testcapi.restore_crossinterp_data(xid)

                # XXX What about between interpreters?
                self.assertIs(got, obj)

    def test_types(self):
        self._assert_values([
            b'spam',
            9999,
            ])

    def test_bytes(self):
        self._assert_values(i.to_bytes(2, 'little', signed=True)
                            for i in range(-1, 258))

    def test_strs(self):
        self._assert_values(['hello world', '你好世界', ''])

    def test_int(self):
        self._assert_values(itertools.chain(range(-1, 258),
                                            [sys.maxsize, -sys.maxsize - 1]))

    def test_non_shareable_int(self):
        ints = [
            sys.maxsize + 1,
            -sys.maxsize - 2,
            2**1000,
        ]
        for i in ints:
            with self.subTest(i):
                with self.assertRaises(OverflowError):
                    _testcapi.get_crossinterp_data(i)


class ModuleTests(TestBase):

    def test_import_in_interpreter(self):
        _run_output(
            interpreters.create(),
            'import _xxsubinterpreters as _interpreters',
        )


##################################
# interpreter tests

class ListAllTests(TestBase):

    def test_initial(self):
        main = interpreters.get_main()
        ids = interpreters.list_all()
        self.assertEqual(ids, [main])

    def test_after_creating(self):
        main = interpreters.get_main()
        first = interpreters.create()
        second = interpreters.create()
        ids = interpreters.list_all()
        self.assertEqual(ids, [main, first, second])

    def test_after_destroying(self):
        main = interpreters.get_main()
        first = interpreters.create()
        second = interpreters.create()
        interpreters.destroy(first)
        ids = interpreters.list_all()
        self.assertEqual(ids, [main, second])


class GetCurrentTests(TestBase):

    def test_main(self):
        main = interpreters.get_main()
        cur = interpreters.get_current()
        self.assertEqual(cur, main)
        self.assertIsInstance(cur, interpreters.InterpreterID)

    def test_subinterpreter(self):
        main = interpreters.get_main()
        interp = interpreters.create()
        out = _run_output(interp, dedent("""
            import _xxsubinterpreters as _interpreters
            cur = _interpreters.get_current()
            print(cur)
            assert isinstance(cur, _interpreters.InterpreterID)
            """))
        cur = int(out.strip())
        _, expected = interpreters.list_all()
        self.assertEqual(cur, expected)
        self.assertNotEqual(cur, main)


class GetMainTests(TestBase):

    def test_from_main(self):
        [expected] = interpreters.list_all()
        main = interpreters.get_main()
        self.assertEqual(main, expected)
        self.assertIsInstance(main, interpreters.InterpreterID)

    def test_from_subinterpreter(self):
        [expected] = interpreters.list_all()
        interp = interpreters.create()
        out = _run_output(interp, dedent("""
            import _xxsubinterpreters as _interpreters
            main = _interpreters.get_main()
            print(main)
            assert isinstance(main, _interpreters.InterpreterID)
            """))
        main = int(out.strip())
        self.assertEqual(main, expected)


class IsRunningTests(TestBase):

    def test_main(self):
        main = interpreters.get_main()
        self.assertTrue(interpreters.is_running(main))

    @unittest.skip('Fails on FreeBSD')
    def test_subinterpreter(self):
        interp = interpreters.create()
        self.assertFalse(interpreters.is_running(interp))

        with _running(interp):
            self.assertTrue(interpreters.is_running(interp))
        self.assertFalse(interpreters.is_running(interp))

    def test_from_subinterpreter(self):
        interp = interpreters.create()
        out = _run_output(interp, dedent(f"""
            import _xxsubinterpreters as _interpreters
            if _interpreters.is_running({interp}):
                print(True)
            else:
                print(False)
            """))
        self.assertEqual(out.strip(), 'True')

    def test_already_destroyed(self):
        interp = interpreters.create()
        interpreters.destroy(interp)
        with self.assertRaises(RuntimeError):
            interpreters.is_running(interp)

    def test_does_not_exist(self):
        with self.assertRaises(RuntimeError):
            interpreters.is_running(1_000_000)

    def test_bad_id(self):
        with self.assertRaises(ValueError):
            interpreters.is_running(-1)


class InterpreterIDTests(TestBase):

    def test_with_int(self):
        id = interpreters.InterpreterID(10, force=True)

        self.assertEqual(int(id), 10)

    def test_coerce_id(self):
        class Int(str):
            def __index__(self):
                return 10

        id = interpreters.InterpreterID(Int(), force=True)
        self.assertEqual(int(id), 10)

    def test_bad_id(self):
        self.assertRaises(TypeError, interpreters.InterpreterID, object())
        self.assertRaises(TypeError, interpreters.InterpreterID, 10.0)
        self.assertRaises(TypeError, interpreters.InterpreterID, '10')
        self.assertRaises(TypeError, interpreters.InterpreterID, b'10')
        self.assertRaises(ValueError, interpreters.InterpreterID, -1)
        self.assertRaises(OverflowError, interpreters.InterpreterID, 2**64)

    def test_does_not_exist(self):
        id = interpreters.create()
        with self.assertRaises(RuntimeError):
            interpreters.InterpreterID(int(id) + 1)  # unforced

    def test_str(self):
        id = interpreters.InterpreterID(10, force=True)
        self.assertEqual(str(id), '10')

    def test_repr(self):
        id = interpreters.InterpreterID(10, force=True)
        self.assertEqual(repr(id), 'InterpreterID(10)')

    def test_equality(self):
        id1 = interpreters.create()
        id2 = interpreters.InterpreterID(int(id1))
        id3 = interpreters.create()

        self.assertTrue(id1 == id1)
        self.assertTrue(id1 == id2)
        self.assertTrue(id1 == int(id1))
        self.assertTrue(int(id1) == id1)
        self.assertTrue(id1 == float(int(id1)))
        self.assertTrue(float(int(id1)) == id1)
        self.assertFalse(id1 == float(int(id1)) + 0.1)
        self.assertFalse(id1 == str(int(id1)))
        self.assertFalse(id1 == 2**1000)
        self.assertFalse(id1 == float('inf'))
        self.assertFalse(id1 == 'spam')
        self.assertFalse(id1 == id3)

        self.assertFalse(id1 != id1)
        self.assertFalse(id1 != id2)
        self.assertTrue(id1 != id3)


class CreateTests(TestBase):

    def test_in_main(self):
        id = interpreters.create()
        self.assertIsInstance(id, interpreters.InterpreterID)

        self.assertIn(id, interpreters.list_all())

    @unittest.skip('enable this test when working on pystate.c')
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
        self.assertIn(id, interpreters.list_all())

    def test_in_subinterpreter(self):
        main, = interpreters.list_all()
        id1 = interpreters.create()
        out = _run_output(id1, dedent("""
            import _xxsubinterpreters as _interpreters
            id = _interpreters.create()
            print(id)
            assert isinstance(id, _interpreters.InterpreterID)
            """))
        id2 = int(out.strip())

        self.assertEqual(set(interpreters.list_all()), {main, id1, id2})

    def test_in_threaded_subinterpreter(self):
        main, = interpreters.list_all()
        id1 = interpreters.create()
        id2 = None
        def f():
            nonlocal id2
            out = _run_output(id1, dedent("""
                import _xxsubinterpreters as _interpreters
                id = _interpreters.create()
                print(id)
                """))
            id2 = int(out.strip())

        t = threading.Thread(target=f)
        t.start()
        t.join()

        self.assertEqual(set(interpreters.list_all()), {main, id1, id2})

    def test_after_destroy_all(self):
        before = set(interpreters.list_all())
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
        self.assertEqual(set(interpreters.list_all()), before | {id})

    def test_after_destroy_some(self):
        before = set(interpreters.list_all())
        # Create 3 subinterpreters.
        id1 = interpreters.create()
        id2 = interpreters.create()
        id3 = interpreters.create()
        # Now destroy 2 of them.
        interpreters.destroy(id1)
        interpreters.destroy(id3)
        # Finally, create another.
        id = interpreters.create()
        self.assertEqual(set(interpreters.list_all()), before | {id, id2})


class DestroyTests(TestBase):

    def test_one(self):
        id1 = interpreters.create()
        id2 = interpreters.create()
        id3 = interpreters.create()
        self.assertIn(id2, interpreters.list_all())
        interpreters.destroy(id2)
        self.assertNotIn(id2, interpreters.list_all())
        self.assertIn(id1, interpreters.list_all())
        self.assertIn(id3, interpreters.list_all())

    def test_all(self):
        before = set(interpreters.list_all())
        ids = set()
        for _ in range(3):
            id = interpreters.create()
            ids.add(id)
        self.assertEqual(set(interpreters.list_all()), before | ids)
        for id in ids:
            interpreters.destroy(id)
        self.assertEqual(set(interpreters.list_all()), before)

    def test_main(self):
        main, = interpreters.list_all()
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
        with self.assertRaises(ValueError):
            interpreters.destroy(-1)

    def test_from_current(self):
        main, = interpreters.list_all()
        id = interpreters.create()
        script = dedent(f"""
            import _xxsubinterpreters as _interpreters
            try:
                _interpreters.destroy({id})
            except RuntimeError:
                pass
            """)

        interpreters.run_string(id, script)
        self.assertEqual(set(interpreters.list_all()), {main, id})

    def test_from_sibling(self):
        main, = interpreters.list_all()
        id1 = interpreters.create()
        id2 = interpreters.create()
        script = dedent(f"""
            import _xxsubinterpreters as _interpreters
            _interpreters.destroy({id2})
            """)
        interpreters.run_string(id1, script)

        self.assertEqual(set(interpreters.list_all()), {main, id1})

    def test_from_other_thread(self):
        id = interpreters.create()
        def f():
            interpreters.destroy(id)

        t = threading.Thread(target=f)
        t.start()
        t.join()

    def test_still_running(self):
        main, = interpreters.list_all()
        interp = interpreters.create()
        with _running(interp):
            self.assertTrue(interpreters.is_running(interp),
                            msg=f"Interp {interp} should be running before destruction.")

            with self.assertRaises(RuntimeError,
                                   msg=f"Should not be able to destroy interp {interp} while it's still running."):
                interpreters.destroy(interp)
            self.assertTrue(interpreters.is_running(interp))


class RunStringTests(TestBase):

    def setUp(self):
        super().setUp()
        self.id = interpreters.create()

    def test_success(self):
        script, file = _captured_script('print("it worked!", end="")')
        with file:
            interpreters.run_string(self.id, script)
            out = file.read()

        self.assertEqual(out, 'it worked!')

    def test_in_thread(self):
        script, file = _captured_script('print("it worked!", end="")')
        with file:
            def f():
                interpreters.run_string(self.id, script)

            t = threading.Thread(target=f)
            t.start()
            t.join()
            out = file.read()

        self.assertEqual(out, 'it worked!')

    def test_create_thread(self):
        subinterp = interpreters.create()
        script, file = _captured_script("""
            import threading
            def f():
                print('it worked!', end='')

            t = threading.Thread(target=f)
            t.start()
            t.join()
            """)
        with file:
            interpreters.run_string(subinterp, script)
            out = file.read()

        self.assertEqual(out, 'it worked!')

    def test_create_daemon_thread(self):
        with self.subTest('isolated'):
            expected = 'spam spam spam spam spam'
            subinterp = interpreters.create(isolated=True)
            script, file = _captured_script(f"""
                import threading
                def f():
                    print('it worked!', end='')

                try:
                    t = threading.Thread(target=f, daemon=True)
                    t.start()
                    t.join()
                except RuntimeError:
                    print('{expected}', end='')
                """)
            with file:
                interpreters.run_string(subinterp, script)
                out = file.read()

            self.assertEqual(out, expected)

        with self.subTest('not isolated'):
            subinterp = interpreters.create(isolated=False)
            script, file = _captured_script("""
                import threading
                def f():
                    print('it worked!', end='')

                t = threading.Thread(target=f, daemon=True)
                t.start()
                t.join()
                """)
            with file:
                interpreters.run_string(subinterp, script)
                out = file.read()

            self.assertEqual(out, 'it worked!')

    def test_shareable_types(self):
        interp = interpreters.create()
        objects = [
            None,
            'spam',
            b'spam',
            42,
        ]
        for obj in objects:
            with self.subTest(obj):
                interpreters.run_string(
                    interp,
                    f'assert(obj == {obj!r})',
                    shared=dict(obj=obj),
                )

    def test_os_exec(self):
        expected = 'spam spam spam spam spam'
        subinterp = interpreters.create()
        script, file = _captured_script(f"""
            import os, sys
            try:
                os.execl(sys.executable)
            except RuntimeError:
                print('{expected}', end='')
            """)
        with file:
            interpreters.run_string(subinterp, script)
            out = file.read()

        self.assertEqual(out, expected)

    @support.requires_fork()
    def test_fork(self):
        import tempfile
        with tempfile.NamedTemporaryFile('w+', encoding="utf-8") as file:
            file.write('')
            file.flush()

            expected = 'spam spam spam spam spam'
            script = dedent(f"""
                import os
                try:
                    os.fork()
                except RuntimeError:
                    with open('{file.name}', 'w', encoding='utf-8') as out:
                        out.write('{expected}')
                """)
            interpreters.run_string(self.id, script)

            file.seek(0)
            content = file.read()
            self.assertEqual(content, expected)

    def test_already_running(self):
        with _running(self.id):
            with self.assertRaises(RuntimeError):
                interpreters.run_string(self.id, 'print("spam")')

    def test_does_not_exist(self):
        id = 0
        while id in interpreters.list_all():
            id += 1
        with self.assertRaises(RuntimeError):
            interpreters.run_string(id, 'print("spam")')

    def test_error_id(self):
        with self.assertRaises(ValueError):
            interpreters.run_string(-1, 'print("spam")')

    def test_bad_id(self):
        with self.assertRaises(TypeError):
            interpreters.run_string('spam', 'print("spam")')

    def test_bad_script(self):
        with self.assertRaises(TypeError):
            interpreters.run_string(self.id, 10)

    def test_bytes_for_script(self):
        with self.assertRaises(TypeError):
            interpreters.run_string(self.id, b'print("spam")')

    @contextlib.contextmanager
    def assert_run_failed(self, exctype, msg=None):
        with self.assertRaises(interpreters.RunFailedError) as caught:
            yield
        if msg is None:
            self.assertEqual(str(caught.exception).split(':')[0],
                             str(exctype))
        else:
            self.assertEqual(str(caught.exception),
                             "{}: {}".format(exctype, msg))

    def test_invalid_syntax(self):
        with self.assert_run_failed(SyntaxError):
            # missing close paren
            interpreters.run_string(self.id, 'print("spam"')

    def test_failure(self):
        with self.assert_run_failed(Exception, 'spam'):
            interpreters.run_string(self.id, 'raise Exception("spam")')

    def test_SystemExit(self):
        with self.assert_run_failed(SystemExit, '42'):
            interpreters.run_string(self.id, 'raise SystemExit(42)')

    def test_sys_exit(self):
        with self.assert_run_failed(SystemExit):
            interpreters.run_string(self.id, dedent("""
                import sys
                sys.exit()
                """))

        with self.assert_run_failed(SystemExit, '42'):
            interpreters.run_string(self.id, dedent("""
                import sys
                sys.exit(42)
                """))

    def test_with_shared(self):
        r, w = os.pipe()

        shared = {
                'spam': b'ham',
                'eggs': b'-1',
                'cheddar': None,
                }
        script = dedent(f"""
            eggs = int(eggs)
            spam = 42
            result = spam + eggs

            ns = dict(vars())
            del ns['__builtins__']
            import pickle
            with open({w}, 'wb') as chan:
                pickle.dump(ns, chan)
            """)
        interpreters.run_string(self.id, script, shared)
        with open(r, 'rb') as chan:
            ns = pickle.load(chan)

        self.assertEqual(ns['spam'], 42)
        self.assertEqual(ns['eggs'], -1)
        self.assertEqual(ns['result'], 41)
        self.assertIsNone(ns['cheddar'])

    def test_shared_overwrites(self):
        interpreters.run_string(self.id, dedent("""
            spam = 'eggs'
            ns1 = dict(vars())
            del ns1['__builtins__']
            """))

        shared = {'spam': b'ham'}
        script = dedent("""
            ns2 = dict(vars())
            del ns2['__builtins__']
        """)
        interpreters.run_string(self.id, script, shared)

        r, w = os.pipe()
        script = dedent(f"""
            ns = dict(vars())
            del ns['__builtins__']
            import pickle
            with open({w}, 'wb') as chan:
                pickle.dump(ns, chan)
            """)
        interpreters.run_string(self.id, script)
        with open(r, 'rb') as chan:
            ns = pickle.load(chan)

        self.assertEqual(ns['ns1']['spam'], 'eggs')
        self.assertEqual(ns['ns2']['spam'], b'ham')
        self.assertEqual(ns['spam'], b'ham')

    def test_shared_overwrites_default_vars(self):
        r, w = os.pipe()

        shared = {'__name__': b'not __main__'}
        script = dedent(f"""
            spam = 42

            ns = dict(vars())
            del ns['__builtins__']
            import pickle
            with open({w}, 'wb') as chan:
                pickle.dump(ns, chan)
            """)
        interpreters.run_string(self.id, script, shared)
        with open(r, 'rb') as chan:
            ns = pickle.load(chan)

        self.assertEqual(ns['__name__'], b'not __main__')

    def test_main_reused(self):
        r, w = os.pipe()
        interpreters.run_string(self.id, dedent(f"""
            spam = True

            ns = dict(vars())
            del ns['__builtins__']
            import pickle
            with open({w}, 'wb') as chan:
                pickle.dump(ns, chan)
            del ns, pickle, chan
            """))
        with open(r, 'rb') as chan:
            ns1 = pickle.load(chan)

        r, w = os.pipe()
        interpreters.run_string(self.id, dedent(f"""
            eggs = False

            ns = dict(vars())
            del ns['__builtins__']
            import pickle
            with open({w}, 'wb') as chan:
                pickle.dump(ns, chan)
            """))
        with open(r, 'rb') as chan:
            ns2 = pickle.load(chan)

        self.assertIn('spam', ns1)
        self.assertNotIn('eggs', ns1)
        self.assertIn('eggs', ns2)
        self.assertIn('spam', ns2)

    def test_execution_namespace_is_main(self):
        r, w = os.pipe()

        script = dedent(f"""
            spam = 42

            ns = dict(vars())
            ns['__builtins__'] = str(ns['__builtins__'])
            import pickle
            with open({w}, 'wb') as chan:
                pickle.dump(ns, chan)
            """)
        interpreters.run_string(self.id, script)
        with open(r, 'rb') as chan:
            ns = pickle.load(chan)

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

    # XXX Fix this test!
    @unittest.skip('blocking forever')
    def test_still_running_at_exit(self):
        script = dedent("""
        from textwrap import dedent
        import threading
        import _xxsubinterpreters as _interpreters
        id = _interpreters.create()
        def f():
            _interpreters.run_string(id, dedent('''
                import time
                # Give plenty of time for the main interpreter to finish.
                time.sleep(1_000_000)
                '''))

        t = threading.Thread(target=f)
        t.start()
        """)
        with support.temp_dir() as dirname:
            filename = script_helper.make_script(dirname, 'interp', script)
            with script_helper.spawn_python(filename) as proc:
                retcode = proc.wait()

        self.assertEqual(retcode, 0)


if __name__ == '__main__':
    unittest.main()
