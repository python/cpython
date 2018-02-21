import contextlib
import os
import pickle
from textwrap import dedent, indent
import threading
import time
import unittest

from test import support
from test.support import script_helper

interpreters = support.import_module('_xxsubinterpreters')


def _captured_script(script):
    r, w = os.pipe()
    indented = script.replace('\n', '\n                ')
    wrapped = dedent(f"""
        import contextlib
        with open({w}, 'w') as chan:
            with contextlib.redirect_stdout(chan):
                {indented}
        """)
    return wrapped, open(r)


def _run_output(interp, request, shared=None):
    script, chan = _captured_script(request)
    with chan:
        interpreters.run_string(interp, script, shared)
        return chan.read()


@contextlib.contextmanager
def _running(interp):
    r, w = os.pipe()
    def run():
        interpreters.run_string(interp, dedent(f"""
            # wait for "signal"
            with open({r}) as chan:
                chan.read()
            """))

    t = threading.Thread(target=run)
    t.start()

    yield

    with open(w, 'w') as chan:
        chan.write('done')
    t.join()


class IsShareableTests(unittest.TestCase):

    def test_default_shareables(self):
        shareables = [
                # singletons
                None,
                # builtin objects
                b'spam',
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
                42,
                100.0,
                'spam',
                # user-defined types and objects
                Cheese,
                Cheese('Wensleydale'),
                SubBytes(b'spam'),
                ]
        for obj in not_shareables:
            with self.subTest(obj):
                self.assertFalse(
                    interpreters.is_shareable(obj))


class TestBase(unittest.TestCase):

    def tearDown(self):
        for id in interpreters.list_all():
            if id == 0:  # main
                continue
            try:
                interpreters.destroy(id)
            except RuntimeError:
                pass  # already destroyed

        for cid in interpreters.channel_list_all():
            try:
                interpreters.channel_destroy(cid)
            except interpreters.ChannelNotFoundError:
                pass  # already destroyed


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

    def test_subinterpreter(self):
        main = interpreters.get_main()
        interp = interpreters.create()
        out = _run_output(interp, dedent("""
            import _xxsubinterpreters as _interpreters
            print(int(_interpreters.get_current()))
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

    def test_from_subinterpreter(self):
        [expected] = interpreters.list_all()
        interp = interpreters.create()
        out = _run_output(interp, dedent("""
            import _xxsubinterpreters as _interpreters
            print(int(_interpreters.get_main()))
            """))
        main = int(out.strip())
        self.assertEqual(main, expected)


class IsRunningTests(TestBase):

    def test_main(self):
        main = interpreters.get_main()
        self.assertTrue(interpreters.is_running(main))

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
            if _interpreters.is_running({int(interp)}):
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
        with self.assertRaises(RuntimeError):
            interpreters.is_running(-1)


class InterpreterIDTests(TestBase):

    def test_with_int(self):
        id = interpreters.InterpreterID(10, force=True)

        self.assertEqual(int(id), 10)

    def test_coerce_id(self):
        id = interpreters.InterpreterID('10', force=True)
        self.assertEqual(int(id), 10)

        id = interpreters.InterpreterID(10.0, force=True)
        self.assertEqual(int(id), 10)

        class Int(str):
            def __init__(self, value):
                self._value = value
            def __int__(self):
                return self._value

        id = interpreters.InterpreterID(Int(10), force=True)
        self.assertEqual(int(id), 10)

    def test_bad_id(self):
        for id in [-1, 'spam']:
            with self.subTest(id):
                with self.assertRaises(ValueError):
                    interpreters.InterpreterID(id)
        with self.assertRaises(OverflowError):
            interpreters.InterpreterID(2**64)
        with self.assertRaises(TypeError):
            interpreters.InterpreterID(object())

    def test_does_not_exist(self):
        id = interpreters.channel_create()
        with self.assertRaises(RuntimeError):
            interpreters.InterpreterID(int(id) + 1)  # unforced

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
        self.assertFalse(id1 == id3)

        self.assertFalse(id1 != id1)
        self.assertFalse(id1 != id2)
        self.assertTrue(id1 != id3)


class CreateTests(TestBase):

    def test_in_main(self):
        id = interpreters.create()

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
            print(int(id))
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
                print(int(id))
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
        with self.assertRaises(RuntimeError):
            interpreters.destroy(-1)

    def test_from_current(self):
        main, = interpreters.list_all()
        id = interpreters.create()
        script = dedent(f"""
            import _xxsubinterpreters as _interpreters
            try:
                _interpreters.destroy({int(id)})
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
            _interpreters.destroy({int(id2)})
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
            with self.assertRaises(RuntimeError):
                interpreters.destroy(interp)
            self.assertTrue(interpreters.is_running(interp))


class RunStringTests(TestBase):

    SCRIPT = dedent("""
        with open('{}', 'w') as out:
            out.write('{}')
        """)
    FILENAME = 'spam'

    def setUp(self):
        super().setUp()
        self.id = interpreters.create()
        self._fs = None

    def tearDown(self):
        if self._fs is not None:
            self._fs.close()
        super().tearDown()

    @property
    def fs(self):
        if self._fs is None:
            self._fs = FSFixture(self)
        return self._fs

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
        script, file = _captured_script("""
            import threading
            def f():
                print('it worked!', end='')

            t = threading.Thread(target=f)
            t.start()
            t.join()
            """)
        with file:
            interpreters.run_string(self.id, script)
            out = file.read()

        self.assertEqual(out, 'it worked!')

    @unittest.skipUnless(hasattr(os, 'fork'), "test needs os.fork()")
    def test_fork(self):
        import tempfile
        with tempfile.NamedTemporaryFile('w+') as file:
            file.write('')
            file.flush()

            expected = 'spam spam spam spam spam'
            script = dedent(f"""
                # (inspired by Lib/test/test_fork.py)
                import os
                pid = os.fork()
                if pid == 0:  # child
                    with open('{file.name}', 'w') as out:
                        out.write('{expected}')
                    # Kill the unittest runner in the child process.
                    os._exit(1)
                else:
                    SHORT_SLEEP = 0.1
                    import time
                    for _ in range(10):
                        spid, status = os.waitpid(pid, os.WNOHANG)
                        if spid == pid:
                            break
                        time.sleep(SHORT_SLEEP)
                    assert(spid == pid)
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
        with self.assertRaises(RuntimeError):
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
        script = dedent(f"""
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
        script = dedent(f"""
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


class ChannelIDTests(TestBase):

    def test_default_kwargs(self):
        cid = interpreters._channel_id(10, force=True)

        self.assertEqual(int(cid), 10)
        self.assertEqual(cid.end, 'both')

    def test_with_kwargs(self):
        cid = interpreters._channel_id(10, send=True, force=True)
        self.assertEqual(cid.end, 'send')

        cid = interpreters._channel_id(10, send=True, recv=False, force=True)
        self.assertEqual(cid.end, 'send')

        cid = interpreters._channel_id(10, recv=True, force=True)
        self.assertEqual(cid.end, 'recv')

        cid = interpreters._channel_id(10, recv=True, send=False, force=True)
        self.assertEqual(cid.end, 'recv')

        cid = interpreters._channel_id(10, send=True, recv=True, force=True)
        self.assertEqual(cid.end, 'both')

    def test_coerce_id(self):
        cid = interpreters._channel_id('10', force=True)
        self.assertEqual(int(cid), 10)

        cid = interpreters._channel_id(10.0, force=True)
        self.assertEqual(int(cid), 10)

        class Int(str):
            def __init__(self, value):
                self._value = value
            def __int__(self):
                return self._value

        cid = interpreters._channel_id(Int(10), force=True)
        self.assertEqual(int(cid), 10)

    def test_bad_id(self):
        for cid in [-1, 'spam']:
            with self.subTest(cid):
                with self.assertRaises(ValueError):
                    interpreters._channel_id(cid)
        with self.assertRaises(OverflowError):
            interpreters._channel_id(2**64)
        with self.assertRaises(TypeError):
            interpreters._channel_id(object())

    def test_bad_kwargs(self):
        with self.assertRaises(ValueError):
            interpreters._channel_id(10, send=False, recv=False)

    def test_does_not_exist(self):
        cid = interpreters.channel_create()
        with self.assertRaises(interpreters.ChannelNotFoundError):
            interpreters._channel_id(int(cid) + 1)  # unforced

    def test_repr(self):
        cid = interpreters._channel_id(10, force=True)
        self.assertEqual(repr(cid), 'ChannelID(10)')

        cid = interpreters._channel_id(10, send=True, force=True)
        self.assertEqual(repr(cid), 'ChannelID(10, send=True)')

        cid = interpreters._channel_id(10, recv=True, force=True)
        self.assertEqual(repr(cid), 'ChannelID(10, recv=True)')

        cid = interpreters._channel_id(10, send=True, recv=True, force=True)
        self.assertEqual(repr(cid), 'ChannelID(10)')

    def test_equality(self):
        cid1 = interpreters.channel_create()
        cid2 = interpreters._channel_id(int(cid1))
        cid3 = interpreters.channel_create()

        self.assertTrue(cid1 == cid1)
        self.assertTrue(cid1 == cid2)
        self.assertTrue(cid1 == int(cid1))
        self.assertFalse(cid1 == cid3)

        self.assertFalse(cid1 != cid1)
        self.assertFalse(cid1 != cid2)
        self.assertTrue(cid1 != cid3)


class ChannelTests(TestBase):

    def test_sequential_ids(self):
        before = interpreters.channel_list_all()
        id1 = interpreters.channel_create()
        id2 = interpreters.channel_create()
        id3 = interpreters.channel_create()
        after = interpreters.channel_list_all()

        self.assertEqual(id2, int(id1) + 1)
        self.assertEqual(id3, int(id2) + 1)
        self.assertEqual(set(after) - set(before), {id1, id2, id3})

    def test_ids_global(self):
        id1 = interpreters.create()
        out = _run_output(id1, dedent("""
            import _xxsubinterpreters as _interpreters
            cid = _interpreters.channel_create()
            print(int(cid))
            """))
        cid1 = int(out.strip())

        id2 = interpreters.create()
        out = _run_output(id2, dedent("""
            import _xxsubinterpreters as _interpreters
            cid = _interpreters.channel_create()
            print(int(cid))
            """))
        cid2 = int(out.strip())

        self.assertEqual(cid2, int(cid1) + 1)

    ####################

    def test_drop_single_user(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_recv(cid)
        interpreters.channel_drop_interpreter(cid, send=True, recv=True)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_drop_multiple_users(self):
        cid = interpreters.channel_create()
        id1 = interpreters.create()
        id2 = interpreters.create()
        interpreters.run_string(id1, dedent(f"""
            import _xxsubinterpreters as _interpreters
            _interpreters.channel_send({int(cid)}, b'spam')
            """))
        out = _run_output(id2, dedent(f"""
            import _xxsubinterpreters as _interpreters
            obj = _interpreters.channel_recv({int(cid)})
            _interpreters.channel_drop_interpreter({int(cid)})
            print(repr(obj))
            """))
        interpreters.run_string(id1, dedent(f"""
            _interpreters.channel_drop_interpreter({int(cid)})
            """))

        self.assertEqual(out.strip(), "b'spam'")

    def test_drop_no_kwargs(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_recv(cid)
        interpreters.channel_drop_interpreter(cid)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_drop_multiple_times(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_recv(cid)
        interpreters.channel_drop_interpreter(cid, send=True, recv=True)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_drop_interpreter(cid, send=True, recv=True)

    def test_drop_with_unused_items(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_send(cid, b'ham')
        interpreters.channel_drop_interpreter(cid, send=True, recv=True)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_drop_never_used(self):
        cid = interpreters.channel_create()
        interpreters.channel_drop_interpreter(cid)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'spam')
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_drop_by_unassociated_interp(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interp = interpreters.create()
        interpreters.run_string(interp, dedent(f"""
            import _xxsubinterpreters as _interpreters
            _interpreters.channel_drop_interpreter({int(cid)})
            """))
        obj = interpreters.channel_recv(cid)
        interpreters.channel_drop_interpreter(cid)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
        self.assertEqual(obj, b'spam')

    def test_drop_close_if_unassociated(self):
        cid = interpreters.channel_create()
        interp = interpreters.create()
        interpreters.run_string(interp, dedent(f"""
            import _xxsubinterpreters as _interpreters
            obj = _interpreters.channel_send({int(cid)}, b'spam')
            _interpreters.channel_drop_interpreter({int(cid)})
            """))

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_drop_partially(self):
        # XXX Is partial close too wierd/confusing?
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, None)
        interpreters.channel_recv(cid)
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_drop_interpreter(cid, send=True)
        obj = interpreters.channel_recv(cid)

        self.assertEqual(obj, b'spam')

    def test_drop_used_multiple_times_by_single_user(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_recv(cid)
        interpreters.channel_drop_interpreter(cid, send=True, recv=True)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    ####################

    def test_close_single_user(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_recv(cid)
        interpreters.channel_close(cid)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_close_multiple_users(self):
        cid = interpreters.channel_create()
        id1 = interpreters.create()
        id2 = interpreters.create()
        interpreters.run_string(id1, dedent(f"""
            import _xxsubinterpreters as _interpreters
            _interpreters.channel_send({int(cid)}, b'spam')
            """))
        interpreters.run_string(id2, dedent(f"""
            import _xxsubinterpreters as _interpreters
            _interpreters.channel_recv({int(cid)})
            """))
        interpreters.channel_close(cid)
        with self.assertRaises(interpreters.RunFailedError) as cm:
            interpreters.run_string(id1, dedent(f"""
                _interpreters.channel_send({int(cid)}, b'spam')
                """))
        self.assertIn('ChannelClosedError', str(cm.exception))
        with self.assertRaises(interpreters.RunFailedError) as cm:
            interpreters.run_string(id2, dedent(f"""
                _interpreters.channel_send({int(cid)}, b'spam')
                """))
        self.assertIn('ChannelClosedError', str(cm.exception))

    def test_close_multiple_times(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_recv(cid)
        interpreters.channel_close(cid)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_close(cid)

    def test_close_with_unused_items(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_send(cid, b'ham')
        interpreters.channel_close(cid)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_close_never_used(self):
        cid = interpreters.channel_create()
        interpreters.channel_close(cid)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'spam')
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_close_by_unassociated_interp(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interp = interpreters.create()
        interpreters.run_string(interp, dedent(f"""
            import _xxsubinterpreters as _interpreters
            _interpreters.channel_close({int(cid)})
            """))
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_close(cid)

    def test_close_used_multiple_times_by_single_user(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_recv(cid)
        interpreters.channel_close(cid)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    ####################

    def test_send_recv_main(self):
        cid = interpreters.channel_create()
        orig = b'spam'
        interpreters.channel_send(cid, orig)
        obj = interpreters.channel_recv(cid)

        self.assertEqual(obj, orig)
        self.assertIsNot(obj, orig)

    def test_send_recv_same_interpreter(self):
        id1 = interpreters.create()
        out = _run_output(id1, dedent("""
            import _xxsubinterpreters as _interpreters
            cid = _interpreters.channel_create()
            orig = b'spam'
            _interpreters.channel_send(cid, orig)
            obj = _interpreters.channel_recv(cid)
            assert obj is not orig
            assert obj == orig
            """))

    def test_send_recv_different_interpreters(self):
        cid = interpreters.channel_create()
        id1 = interpreters.create()
        out = _run_output(id1, dedent(f"""
            import _xxsubinterpreters as _interpreters
            _interpreters.channel_send({int(cid)}, b'spam')
            """))
        obj = interpreters.channel_recv(cid)

        self.assertEqual(obj, b'spam')

    def test_send_recv_different_threads(self):
        cid = interpreters.channel_create()

        def f():
            while True:
                try:
                    obj = interpreters.channel_recv(cid)
                    break
                except interpreters.ChannelEmptyError:
                    time.sleep(0.1)
            interpreters.channel_send(cid, obj)
        t = threading.Thread(target=f)
        t.start()

        interpreters.channel_send(cid, b'spam')
        t.join()
        obj = interpreters.channel_recv(cid)

        self.assertEqual(obj, b'spam')

    def test_send_recv_different_interpreters_and_threads(self):
        cid = interpreters.channel_create()
        id1 = interpreters.create()
        out = None

        def f():
            nonlocal out
            out = _run_output(id1, dedent(f"""
                import time
                import _xxsubinterpreters as _interpreters
                while True:
                    try:
                        obj = _interpreters.channel_recv({int(cid)})
                        break
                    except _interpreters.ChannelEmptyError:
                        time.sleep(0.1)
                assert(obj == b'spam')
                _interpreters.channel_send({int(cid)}, b'eggs')
                """))
        t = threading.Thread(target=f)
        t.start()

        interpreters.channel_send(cid, b'spam')
        t.join()
        obj = interpreters.channel_recv(cid)

        self.assertEqual(obj, b'eggs')

    def test_send_not_found(self):
        with self.assertRaises(interpreters.ChannelNotFoundError):
            interpreters.channel_send(10, b'spam')

    def test_recv_not_found(self):
        with self.assertRaises(interpreters.ChannelNotFoundError):
            interpreters.channel_recv(10)

    def test_recv_empty(self):
        cid = interpreters.channel_create()
        with self.assertRaises(interpreters.ChannelEmptyError):
            interpreters.channel_recv(cid)

    def test_run_string_arg(self):
        cid = interpreters.channel_create()
        interp = interpreters.create()

        out = _run_output(interp, dedent("""
            import _xxsubinterpreters as _interpreters
            print(cid.end)
            _interpreters.channel_send(cid, b'spam')
            """),
            dict(cid=cid.send))
        obj = interpreters.channel_recv(cid)

        self.assertEqual(obj, b'spam')
        self.assertEqual(out.strip(), 'send')


if __name__ == '__main__':
    unittest.main()
