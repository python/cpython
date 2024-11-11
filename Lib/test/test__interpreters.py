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
from test.support import os_helper
from test.support import script_helper


_interpreters = import_helper.import_module('_interpreters')
_testinternalcapi = import_helper.import_module('_testinternalcapi')
from _interpreters import InterpreterNotFoundError


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


def _run_output(interp, request):
    script, rpipe = _captured_script(request)
    with rpipe:
        _interpreters.run_string(interp, script)
        return rpipe.read()


def _wait_for_interp_to_run(interp, timeout=None):
    # bpo-37224: Running this test file in multiprocesses will fail randomly.
    # The failure reason is that the thread can't acquire the cpu to
    # run subinterpreter eariler than the main thread in multiprocess.
    if timeout is None:
        timeout = support.SHORT_TIMEOUT
    for _ in support.sleeping_retry(timeout, error=False):
        if _interpreters.is_running(interp):
            break
    else:
        raise RuntimeError('interp is not running')


@contextlib.contextmanager
def _running(interp):
    r, w = os.pipe()
    def run():
        _interpreters.run_string(interp, dedent(f"""
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
    for id, *_ in _interpreters.list_all():
        if id == 0:  # main
            continue
        try:
            _interpreters.destroy(id)
        except _interpreters.InterpreterError:
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
                True,
                False,
                100.0,
                (1, ('spam', 'eggs')),
                ]
        for obj in shareables:
            with self.subTest(obj):
                self.assertTrue(
                    _interpreters.is_shareable(obj))

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
                NotImplemented,
                ...,
                # builtin types and objects
                type,
                object,
                object(),
                Exception(),
                # user-defined types and objects
                Cheese,
                Cheese('Wensleydale'),
                SubBytes(b'spam'),
                ]
        for obj in not_shareables:
            with self.subTest(repr(obj)):
                self.assertFalse(
                    _interpreters.is_shareable(obj))


class ShareableTypeTests(unittest.TestCase):

    def _assert_values(self, values):
        for obj in values:
            with self.subTest(obj):
                xid = _testinternalcapi.get_crossinterp_data(obj)
                got = _testinternalcapi.restore_crossinterp_data(xid)

                self.assertEqual(got, obj)
                self.assertIs(type(got), type(obj))

    def test_singletons(self):
        for obj in [None]:
            with self.subTest(obj):
                xid = _testinternalcapi.get_crossinterp_data(obj)
                got = _testinternalcapi.restore_crossinterp_data(xid)

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
                    _testinternalcapi.get_crossinterp_data(i)

    def test_bool(self):
        self._assert_values([True, False])

    def test_float(self):
        self._assert_values([0.0, 1.1, -1.0, 0.12345678, -0.12345678])

    def test_tuple(self):
        self._assert_values([(), (1,), ("hello", "world", ), (1, True, "hello")])
        # Test nesting
        self._assert_values([
            ((1,),),
            ((1, 2), (3, 4)),
            ((1, 2), (3, 4), (5, 6)),
        ])

    def test_tuples_containing_non_shareable_types(self):
        non_shareables = [
                Exception(),
                object(),
        ]
        for s in non_shareables:
            value = tuple([0, 1.0, s])
            with self.subTest(repr(value)):
                # XXX Assert the NotShareableError when it is exported
                with self.assertRaises(ValueError):
                    _testinternalcapi.get_crossinterp_data(value)
            # Check nested as well
            value = tuple([0, 1., (s,)])
            with self.subTest("nested " + repr(value)):
                # XXX Assert the NotShareableError when it is exported
                with self.assertRaises(ValueError):
                    _testinternalcapi.get_crossinterp_data(value)


class ModuleTests(TestBase):

    def test_import_in_interpreter(self):
        _run_output(
            _interpreters.create(),
            'import _interpreters',
        )


##################################
# interpreter tests

class ListAllTests(TestBase):

    def test_initial(self):
        main, *_ = _interpreters.get_main()
        ids = [id for id, *_ in _interpreters.list_all()]
        self.assertEqual(ids, [main])

    def test_after_creating(self):
        main, *_ = _interpreters.get_main()
        first = _interpreters.create()
        second = _interpreters.create()
        ids = [id for id, *_ in _interpreters.list_all()]
        self.assertEqual(ids, [main, first, second])

    def test_after_destroying(self):
        main, *_ = _interpreters.get_main()
        first = _interpreters.create()
        second = _interpreters.create()
        _interpreters.destroy(first)
        ids = [id for id, *_ in _interpreters.list_all()]
        self.assertEqual(ids, [main, second])


class GetCurrentTests(TestBase):

    def test_main(self):
        main, *_ = _interpreters.get_main()
        cur, *_ = _interpreters.get_current()
        self.assertEqual(cur, main)
        self.assertIsInstance(cur, int)

    def test_subinterpreter(self):
        main, *_ = _interpreters.get_main()
        interp = _interpreters.create()
        out = _run_output(interp, dedent("""
            import _interpreters
            cur, *_ = _interpreters.get_current()
            print(cur)
            assert isinstance(cur, int)
            """))
        cur = int(out.strip())
        _, expected = [id for id, *_ in _interpreters.list_all()]
        self.assertEqual(cur, expected)
        self.assertNotEqual(cur, main)


class GetMainTests(TestBase):

    def test_from_main(self):
        [expected] = [id for id, *_ in _interpreters.list_all()]
        main, *_ = _interpreters.get_main()
        self.assertEqual(main, expected)
        self.assertIsInstance(main, int)

    def test_from_subinterpreter(self):
        [expected] = [id for id, *_ in _interpreters.list_all()]
        interp = _interpreters.create()
        out = _run_output(interp, dedent("""
            import _interpreters
            main, *_ = _interpreters.get_main()
            print(main)
            assert isinstance(main, int)
            """))
        main = int(out.strip())
        self.assertEqual(main, expected)


class IsRunningTests(TestBase):

    def test_main(self):
        main, *_ = _interpreters.get_main()
        self.assertTrue(_interpreters.is_running(main))

    @unittest.skip('Fails on FreeBSD')
    def test_subinterpreter(self):
        interp = _interpreters.create()
        self.assertFalse(_interpreters.is_running(interp))

        with _running(interp):
            self.assertTrue(_interpreters.is_running(interp))
        self.assertFalse(_interpreters.is_running(interp))

    def test_from_subinterpreter(self):
        interp = _interpreters.create()
        out = _run_output(interp, dedent(f"""
            import _interpreters
            if _interpreters.is_running({interp}):
                print(True)
            else:
                print(False)
            """))
        self.assertEqual(out.strip(), 'True')

    def test_already_destroyed(self):
        interp = _interpreters.create()
        _interpreters.destroy(interp)
        with self.assertRaises(InterpreterNotFoundError):
            _interpreters.is_running(interp)

    def test_does_not_exist(self):
        with self.assertRaises(InterpreterNotFoundError):
            _interpreters.is_running(1_000_000)

    def test_bad_id(self):
        with self.assertRaises(ValueError):
            _interpreters.is_running(-1)


class CreateTests(TestBase):

    def test_in_main(self):
        id = _interpreters.create()
        self.assertIsInstance(id, int)

        after = [id for id, *_ in _interpreters.list_all()]
        self.assertIn(id, after)

    @unittest.skip('enable this test when working on pystate.c')
    def test_unique_id(self):
        seen = set()
        for _ in range(100):
            id = _interpreters.create()
            _interpreters.destroy(id)
            seen.add(id)

        self.assertEqual(len(seen), 100)

    def test_in_thread(self):
        lock = threading.Lock()
        id = None
        def f():
            nonlocal id
            id = _interpreters.create()
            lock.acquire()
            lock.release()

        t = threading.Thread(target=f)
        with lock:
            t.start()
        t.join()
        after = set(id for id, *_ in _interpreters.list_all())
        self.assertIn(id, after)

    def test_in_subinterpreter(self):
        main, = [id for id, *_ in _interpreters.list_all()]
        id1 = _interpreters.create()
        out = _run_output(id1, dedent("""
            import _interpreters
            id = _interpreters.create()
            print(id)
            assert isinstance(id, int)
            """))
        id2 = int(out.strip())

        after = set(id for id, *_ in _interpreters.list_all())
        self.assertEqual(after, {main, id1, id2})

    def test_in_threaded_subinterpreter(self):
        main, = [id for id, *_ in _interpreters.list_all()]
        id1 = _interpreters.create()
        id2 = None
        def f():
            nonlocal id2
            out = _run_output(id1, dedent("""
                import _interpreters
                id = _interpreters.create()
                print(id)
                """))
            id2 = int(out.strip())

        t = threading.Thread(target=f)
        t.start()
        t.join()

        after = set(id for id, *_ in _interpreters.list_all())
        self.assertEqual(after, {main, id1, id2})

    def test_after_destroy_all(self):
        before = set(id for id, *_ in _interpreters.list_all())
        # Create 3 subinterpreters.
        ids = []
        for _ in range(3):
            id = _interpreters.create()
            ids.append(id)
        # Now destroy them.
        for id in ids:
            _interpreters.destroy(id)
        # Finally, create another.
        id = _interpreters.create()
        after = set(id for id, *_ in _interpreters.list_all())
        self.assertEqual(after, before | {id})

    def test_after_destroy_some(self):
        before = set(id for id, *_ in _interpreters.list_all())
        # Create 3 subinterpreters.
        id1 = _interpreters.create()
        id2 = _interpreters.create()
        id3 = _interpreters.create()
        # Now destroy 2 of them.
        _interpreters.destroy(id1)
        _interpreters.destroy(id3)
        # Finally, create another.
        id = _interpreters.create()
        after = set(id for id, *_ in _interpreters.list_all())
        self.assertEqual(after, before | {id, id2})


class DestroyTests(TestBase):

    def test_one(self):
        id1 = _interpreters.create()
        id2 = _interpreters.create()
        id3 = _interpreters.create()
        before = set(id for id, *_ in _interpreters.list_all())
        self.assertIn(id2, before)

        _interpreters.destroy(id2)

        after = set(id for id, *_ in _interpreters.list_all())
        self.assertNotIn(id2, after)
        self.assertIn(id1, after)
        self.assertIn(id3, after)

    def test_all(self):
        initial = set(id for id, *_ in _interpreters.list_all())
        ids = set()
        for _ in range(3):
            id = _interpreters.create()
            ids.add(id)
        before = set(id for id, *_ in _interpreters.list_all())
        self.assertEqual(before, initial | ids)
        for id in ids:
            _interpreters.destroy(id)
        after = set(id for id, *_ in _interpreters.list_all())
        self.assertEqual(after, initial)

    def test_main(self):
        main, = [id for id, *_ in _interpreters.list_all()]
        with self.assertRaises(_interpreters.InterpreterError):
            _interpreters.destroy(main)

        def f():
            with self.assertRaises(_interpreters.InterpreterError):
                _interpreters.destroy(main)

        t = threading.Thread(target=f)
        t.start()
        t.join()

    def test_already_destroyed(self):
        id = _interpreters.create()
        _interpreters.destroy(id)
        with self.assertRaises(InterpreterNotFoundError):
            _interpreters.destroy(id)

    def test_does_not_exist(self):
        with self.assertRaises(InterpreterNotFoundError):
            _interpreters.destroy(1_000_000)

    def test_bad_id(self):
        with self.assertRaises(ValueError):
            _interpreters.destroy(-1)

    def test_from_current(self):
        main, = [id for id, *_ in _interpreters.list_all()]
        id = _interpreters.create()
        script = dedent(f"""
            import _interpreters
            try:
                _interpreters.destroy({id})
            except _interpreters.InterpreterError:
                pass
            """)

        _interpreters.run_string(id, script)
        after = set(id for id, *_ in _interpreters.list_all())
        self.assertEqual(after, {main, id})

    def test_from_sibling(self):
        main, = [id for id, *_ in _interpreters.list_all()]
        id1 = _interpreters.create()
        id2 = _interpreters.create()
        script = dedent(f"""
            import _interpreters
            _interpreters.destroy({id2})
            """)
        _interpreters.run_string(id1, script)

        after = set(id for id, *_ in _interpreters.list_all())
        self.assertEqual(after, {main, id1})

    def test_from_other_thread(self):
        id = _interpreters.create()
        def f():
            _interpreters.destroy(id)

        t = threading.Thread(target=f)
        t.start()
        t.join()

    def test_still_running(self):
        main, = [id for id, *_ in _interpreters.list_all()]
        interp = _interpreters.create()
        with _running(interp):
            self.assertTrue(_interpreters.is_running(interp),
                            msg=f"Interp {interp} should be running before destruction.")

            with self.assertRaises(_interpreters.InterpreterError,
                                   msg=f"Should not be able to destroy interp {interp} while it's still running."):
                _interpreters.destroy(interp)
            self.assertTrue(_interpreters.is_running(interp))


class CommonTests(TestBase):
    def setUp(self):
        super().setUp()
        self.id = _interpreters.create()

    def test_signatures(self):
        # for method in ['exec', 'run_string', 'run_func']:
        msg = "expected 'shared' to be a dict"
        with self.assertRaisesRegex(TypeError, msg):
            _interpreters.exec(self.id, 'a', 1)
        with self.assertRaisesRegex(TypeError, msg):
            _interpreters.exec(self.id, 'a', shared=1)
        with self.assertRaisesRegex(TypeError, msg):
            _interpreters.run_string(self.id, 'a', shared=1)
        with self.assertRaisesRegex(TypeError, msg):
            _interpreters.run_func(self.id, lambda: None, shared=1)


class RunStringTests(TestBase):

    def setUp(self):
        super().setUp()
        self.id = _interpreters.create()

    def test_success(self):
        script, file = _captured_script('print("it worked!", end="")')
        with file:
            _interpreters.run_string(self.id, script)
            out = file.read()

        self.assertEqual(out, 'it worked!')

    def test_in_thread(self):
        script, file = _captured_script('print("it worked!", end="")')
        with file:
            def f():
                _interpreters.run_string(self.id, script)

            t = threading.Thread(target=f)
            t.start()
            t.join()
            out = file.read()

        self.assertEqual(out, 'it worked!')

    def test_create_thread(self):
        subinterp = _interpreters.create()
        script, file = _captured_script("""
            import threading
            def f():
                print('it worked!', end='')

            t = threading.Thread(target=f)
            t.start()
            t.join()
            """)
        with file:
            _interpreters.run_string(subinterp, script)
            out = file.read()

        self.assertEqual(out, 'it worked!')

    def test_create_daemon_thread(self):
        with self.subTest('isolated'):
            expected = 'spam spam spam spam spam'
            subinterp = _interpreters.create('isolated')
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
                _interpreters.run_string(subinterp, script)
                out = file.read()

            self.assertEqual(out, expected)

        with self.subTest('not isolated'):
            subinterp = _interpreters.create('legacy')
            script, file = _captured_script("""
                import threading
                def f():
                    print('it worked!', end='')

                t = threading.Thread(target=f, daemon=True)
                t.start()
                t.join()
                """)
            with file:
                _interpreters.run_string(subinterp, script)
                out = file.read()

            self.assertEqual(out, 'it worked!')

    def test_shareable_types(self):
        interp = _interpreters.create()
        objects = [
            None,
            'spam',
            b'spam',
            42,
        ]
        for obj in objects:
            with self.subTest(obj):
                _interpreters.set___main___attrs(interp, dict(obj=obj))
                _interpreters.run_string(
                    interp,
                    f'assert(obj == {obj!r})',
                )

    def test_os_exec(self):
        expected = 'spam spam spam spam spam'
        subinterp = _interpreters.create()
        script, file = _captured_script(f"""
            import os, sys
            try:
                os.execl(sys.executable)
            except RuntimeError:
                print('{expected}', end='')
            """)
        with file:
            _interpreters.run_string(subinterp, script)
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
            _interpreters.run_string(self.id, script)

            file.seek(0)
            content = file.read()
            self.assertEqual(content, expected)

    def test_already_running(self):
        with _running(self.id):
            with self.assertRaises(_interpreters.InterpreterError):
                _interpreters.run_string(self.id, 'print("spam")')

    def test_does_not_exist(self):
        id = 0
        while id in set(id for id, *_ in _interpreters.list_all()):
            id += 1
        with self.assertRaises(InterpreterNotFoundError):
            _interpreters.run_string(id, 'print("spam")')

    def test_error_id(self):
        with self.assertRaises(ValueError):
            _interpreters.run_string(-1, 'print("spam")')

    def test_bad_id(self):
        with self.assertRaises(TypeError):
            _interpreters.run_string('spam', 'print("spam")')

    def test_bad_script(self):
        with self.assertRaises(TypeError):
            _interpreters.run_string(self.id, 10)

    def test_bytes_for_script(self):
        with self.assertRaises(TypeError):
            _interpreters.run_string(self.id, b'print("spam")')

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
        _interpreters.set___main___attrs(self.id, shared)
        _interpreters.run_string(self.id, script)
        with open(r, 'rb') as chan:
            ns = pickle.load(chan)

        self.assertEqual(ns['spam'], 42)
        self.assertEqual(ns['eggs'], -1)
        self.assertEqual(ns['result'], 41)
        self.assertIsNone(ns['cheddar'])

    def test_shared_overwrites(self):
        _interpreters.run_string(self.id, dedent("""
            spam = 'eggs'
            ns1 = dict(vars())
            del ns1['__builtins__']
            """))

        shared = {'spam': b'ham'}
        script = dedent("""
            ns2 = dict(vars())
            del ns2['__builtins__']
        """)
        _interpreters.set___main___attrs(self.id, shared)
        _interpreters.run_string(self.id, script)

        r, w = os.pipe()
        script = dedent(f"""
            ns = dict(vars())
            del ns['__builtins__']
            import pickle
            with open({w}, 'wb') as chan:
                pickle.dump(ns, chan)
            """)
        _interpreters.run_string(self.id, script)
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
        _interpreters.set___main___attrs(self.id, shared)
        _interpreters.run_string(self.id, script)
        with open(r, 'rb') as chan:
            ns = pickle.load(chan)

        self.assertEqual(ns['__name__'], b'not __main__')

    def test_main_reused(self):
        r, w = os.pipe()
        _interpreters.run_string(self.id, dedent(f"""
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
        _interpreters.run_string(self.id, dedent(f"""
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
        _interpreters.run_string(self.id, script)
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
        import _interpreters
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


class RunFailedTests(TestBase):

    def setUp(self):
        super().setUp()
        self.id = _interpreters.create()

    def add_module(self, modname, text):
        import tempfile
        tempdir = tempfile.mkdtemp()
        self.addCleanup(lambda: os_helper.rmtree(tempdir))
        _interpreters.run_string(self.id, dedent(f"""
            import sys
            sys.path.insert(0, {tempdir!r})
            """))
        return script_helper.make_script(tempdir, modname, text)

    def run_script(self, text, *, fails=False):
        r, w = os.pipe()
        try:
            script = dedent(f"""
                import os, sys
                os.write({w}, b'0')

                # This raises an exception:
                {{}}

                # Nothing from here down should ever run.
                os.write({w}, b'1')
                class NeverError(Exception): pass
                raise NeverError  # never raised
                """).format(dedent(text))
            if fails:
                err = _interpreters.run_string(self.id, script)
                self.assertIsNot(err, None)
                return err
            else:
                err = _interpreters.run_string(self.id, script)
                self.assertIs(err, None)
                return None
        except:
            raise  # re-raise
        else:
            msg = os.read(r, 100)
            self.assertEqual(msg, b'0')
        finally:
            os.close(r)
            os.close(w)

    def _assert_run_failed(self, exctype, msg, script):
        if isinstance(exctype, str):
            exctype_name = exctype
            exctype = None
        else:
            exctype_name = exctype.__name__

        # Run the script.
        excinfo = self.run_script(script, fails=True)

        # Check the wrapper exception.
        self.assertEqual(excinfo.type.__name__, exctype_name)
        if msg is None:
            self.assertEqual(excinfo.formatted.split(':')[0],
                             exctype_name)
        else:
            self.assertEqual(excinfo.formatted,
                             '{}: {}'.format(exctype_name, msg))

        return excinfo

    def assert_run_failed(self, exctype, script):
        self._assert_run_failed(exctype, None, script)

    def assert_run_failed_msg(self, exctype, msg, script):
        self._assert_run_failed(exctype, msg, script)

    def test_exit(self):
        with self.subTest('sys.exit(0)'):
            # XXX Should an unhandled SystemExit(0) be handled as not-an-error?
            self.assert_run_failed(SystemExit, """
                sys.exit(0)
                """)

        with self.subTest('sys.exit()'):
            self.assert_run_failed(SystemExit, """
                import sys
                sys.exit()
                """)

        with self.subTest('sys.exit(42)'):
            self.assert_run_failed_msg(SystemExit, '42', """
                import sys
                sys.exit(42)
                """)

        with self.subTest('SystemExit'):
            self.assert_run_failed_msg(SystemExit, '42', """
                raise SystemExit(42)
                """)

        # XXX Also check os._exit() (via a subprocess)?

    def test_plain_exception(self):
        self.assert_run_failed_msg(Exception, 'spam', """
            raise Exception("spam")
            """)

    def test_invalid_syntax(self):
        script = dedent("""
            x = 1 + 2
            y = 2 + 4
            z = 4 + 8

            # missing close paren
            print("spam"

            if x + y + z < 20:
                ...
            """)

        with self.subTest('script'):
            self.assert_run_failed(SyntaxError, script)

        with self.subTest('module'):
            modname = 'spam_spam_spam'
            filename = self.add_module(modname, script)
            self.assert_run_failed(SyntaxError, f"""
                import {modname}
                """)

    def test_NameError(self):
        self.assert_run_failed(NameError, """
            res = spam + eggs
            """)
        # XXX check preserved suggestions

    def test_AttributeError(self):
        self.assert_run_failed(AttributeError, """
            object().spam
            """)
        # XXX check preserved suggestions

    def test_ExceptionGroup(self):
        self.assert_run_failed(ExceptionGroup, """
            raise ExceptionGroup('exceptions', [
                Exception('spam'),
                ImportError('eggs'),
            ])
            """)

    def test_user_defined_exception(self):
        self.assert_run_failed_msg('MyError', 'spam', """
            class MyError(Exception):
                pass
            raise MyError('spam')
            """)


class RunFuncTests(TestBase):

    def setUp(self):
        super().setUp()
        self.id = _interpreters.create()

    def test_success(self):
        r, w = os.pipe()
        def script():
            global w
            import contextlib
            with open(w, 'w', encoding="utf-8") as spipe:
                with contextlib.redirect_stdout(spipe):
                    print('it worked!', end='')
        _interpreters.set___main___attrs(self.id, dict(w=w))
        _interpreters.run_func(self.id, script)

        with open(r, encoding="utf-8") as outfile:
            out = outfile.read()

        self.assertEqual(out, 'it worked!')

    def test_in_thread(self):
        r, w = os.pipe()
        def script():
            global w
            import contextlib
            with open(w, 'w', encoding="utf-8") as spipe:
                with contextlib.redirect_stdout(spipe):
                    print('it worked!', end='')
        def f():
            _interpreters.set___main___attrs(self.id, dict(w=w))
            _interpreters.run_func(self.id, script)
        t = threading.Thread(target=f)
        t.start()
        t.join()

        with open(r, encoding="utf-8") as outfile:
            out = outfile.read()

        self.assertEqual(out, 'it worked!')

    def test_code_object(self):
        r, w = os.pipe()

        def script():
            global w
            import contextlib
            with open(w, 'w', encoding="utf-8") as spipe:
                with contextlib.redirect_stdout(spipe):
                    print('it worked!', end='')
        code = script.__code__
        _interpreters.set___main___attrs(self.id, dict(w=w))
        _interpreters.run_func(self.id, code)

        with open(r, encoding="utf-8") as outfile:
            out = outfile.read()

        self.assertEqual(out, 'it worked!')

    def test_closure(self):
        spam = True
        def script():
            assert spam

        with self.assertRaises(ValueError):
            _interpreters.run_func(self.id, script)

    # XXX This hasn't been fixed yet.
    @unittest.expectedFailure
    def test_return_value(self):
        def script():
            return 'spam'
        with self.assertRaises(ValueError):
            _interpreters.run_func(self.id, script)

    def test_args(self):
        with self.subTest('args'):
            def script(a, b=0):
                assert a == b
            with self.assertRaises(ValueError):
                _interpreters.run_func(self.id, script)

        with self.subTest('*args'):
            def script(*args):
                assert not args
            with self.assertRaises(ValueError):
                _interpreters.run_func(self.id, script)

        with self.subTest('**kwargs'):
            def script(**kwargs):
                assert not kwargs
            with self.assertRaises(ValueError):
                _interpreters.run_func(self.id, script)

        with self.subTest('kwonly'):
            def script(*, spam=True):
                assert spam
            with self.assertRaises(ValueError):
                _interpreters.run_func(self.id, script)

        with self.subTest('posonly'):
            def script(spam, /):
                assert spam
            with self.assertRaises(ValueError):
                _interpreters.run_func(self.id, script)


if __name__ == '__main__':
    unittest.main()
