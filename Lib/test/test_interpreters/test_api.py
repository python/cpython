import os
import threading
from textwrap import dedent
import unittest

from test import support
from test.support import import_helper
# Raise SkipTest if subinterpreters not supported.
import_helper.import_module('_xxsubinterpreters')
from test.support import interpreters
from test.support.interpreters import InterpreterNotFoundError
from .utils import _captured_script, _run_output, _running, TestBase


class ModuleTests(TestBase):

    def test_queue_aliases(self):
        first = [
            interpreters.create_queue,
            interpreters.Queue,
            interpreters.QueueEmpty,
            interpreters.QueueFull,
        ]
        second = [
            interpreters.create_queue,
            interpreters.Queue,
            interpreters.QueueEmpty,
            interpreters.QueueFull,
        ]
        self.assertEqual(second, first)


class CreateTests(TestBase):

    def test_in_main(self):
        interp = interpreters.create()
        self.assertIsInstance(interp, interpreters.Interpreter)
        self.assertIn(interp, interpreters.list_all())

    def test_in_thread(self):
        lock = threading.Lock()
        interp = None
        def f():
            nonlocal interp
            interp = interpreters.create()
            lock.acquire()
            lock.release()
        t = threading.Thread(target=f)
        with lock:
            t.start()
        t.join()
        self.assertIn(interp, interpreters.list_all())

    def test_in_subinterpreter(self):
        main, = interpreters.list_all()
        interp = interpreters.create()
        out = _run_output(interp, dedent("""
            from test.support import interpreters
            interp = interpreters.create()
            print(interp.id)
            """))
        interp2 = interpreters.Interpreter(int(out))
        self.assertEqual(interpreters.list_all(), [main, interp, interp2])

    def test_after_destroy_all(self):
        before = set(interpreters.list_all())
        # Create 3 subinterpreters.
        interp_lst = []
        for _ in range(3):
            interps = interpreters.create()
            interp_lst.append(interps)
        # Now destroy them.
        for interp in interp_lst:
            interp.close()
        # Finally, create another.
        interp = interpreters.create()
        self.assertEqual(set(interpreters.list_all()), before | {interp})

    def test_after_destroy_some(self):
        before = set(interpreters.list_all())
        # Create 3 subinterpreters.
        interp1 = interpreters.create()
        interp2 = interpreters.create()
        interp3 = interpreters.create()
        # Now destroy 2 of them.
        interp1.close()
        interp2.close()
        # Finally, create another.
        interp = interpreters.create()
        self.assertEqual(set(interpreters.list_all()), before | {interp3, interp})


class GetMainTests(TestBase):

    def test_id(self):
        main = interpreters.get_main()
        self.assertEqual(main.id, 0)

    def test_current(self):
        main = interpreters.get_main()
        current = interpreters.get_current()
        self.assertIs(main, current)

    def test_idempotent(self):
        main1 = interpreters.get_main()
        main2 = interpreters.get_main()
        self.assertIs(main1, main2)


class GetCurrentTests(TestBase):

    def test_main(self):
        main = interpreters.get_main()
        current = interpreters.get_current()
        self.assertEqual(current, main)

    def test_subinterpreter(self):
        main = interpreters.get_main()
        interp = interpreters.create()
        out = _run_output(interp, dedent("""
            from test.support import interpreters
            cur = interpreters.get_current()
            print(cur.id)
            """))
        current = interpreters.Interpreter(int(out))
        self.assertEqual(current, interp)
        self.assertNotEqual(current, main)

    def test_idempotent(self):
        with self.subTest('main'):
            cur1 = interpreters.get_current()
            cur2 = interpreters.get_current()
            self.assertIs(cur1, cur2)

        with self.subTest('subinterpreter'):
            interp = interpreters.create()
            out = _run_output(interp, dedent("""
                from test.support import interpreters
                cur = interpreters.get_current()
                print(id(cur))
                cur = interpreters.get_current()
                print(id(cur))
                """))
            objid1, objid2 = (int(v) for v in out.splitlines())
            self.assertEqual(objid1, objid2)

        with self.subTest('per-interpreter'):
            interp = interpreters.create()
            out = _run_output(interp, dedent("""
                from test.support import interpreters
                cur = interpreters.get_current()
                print(id(cur))
                """))
            id1 = int(out)
            id2 = id(interp)
            self.assertNotEqual(id1, id2)


class ListAllTests(TestBase):

    def test_initial(self):
        interps = interpreters.list_all()
        self.assertEqual(1, len(interps))

    def test_after_creating(self):
        main = interpreters.get_current()
        first = interpreters.create()
        second = interpreters.create()

        ids = []
        for interp in interpreters.list_all():
            ids.append(interp.id)

        self.assertEqual(ids, [main.id, first.id, second.id])

    def test_after_destroying(self):
        main = interpreters.get_current()
        first = interpreters.create()
        second = interpreters.create()
        first.close()

        ids = []
        for interp in interpreters.list_all():
            ids.append(interp.id)

        self.assertEqual(ids, [main.id, second.id])

    def test_idempotent(self):
        main = interpreters.get_current()
        first = interpreters.create()
        second = interpreters.create()
        expected = [main, first, second]

        actual = interpreters.list_all()

        self.assertEqual(actual, expected)
        for interp1, interp2 in zip(actual, expected):
            self.assertIs(interp1, interp2)


class InterpreterObjectTests(TestBase):

    def test_init_int(self):
        interpid = interpreters.get_current().id
        interp = interpreters.Interpreter(interpid)
        self.assertEqual(interp.id, interpid)

    def test_init_interpreter_id(self):
        interpid = interpreters.get_current()._id
        interp = interpreters.Interpreter(interpid)
        self.assertEqual(interp._id, interpid)

    def test_init_unsupported(self):
        actualid = interpreters.get_current().id
        for interpid in [
            str(actualid),
            float(actualid),
            object(),
            None,
            '',
        ]:
            with self.subTest(repr(interpid)):
                with self.assertRaises(TypeError):
                    interpreters.Interpreter(interpid)

    def test_idempotent(self):
        main = interpreters.get_main()
        interp = interpreters.Interpreter(main.id)
        self.assertIs(interp, main)

    def test_init_does_not_exist(self):
        with self.assertRaises(InterpreterNotFoundError):
            interpreters.Interpreter(1_000_000)

    def test_init_bad_id(self):
        with self.assertRaises(ValueError):
            interpreters.Interpreter(-1)

    def test_id_type(self):
        main = interpreters.get_main()
        current = interpreters.get_current()
        interp = interpreters.create()
        self.assertIsInstance(main.id, int)
        self.assertIsInstance(current.id, int)
        self.assertIsInstance(interp.id, int)

    def test_id_readonly(self):
        interp = interpreters.create()
        with self.assertRaises(AttributeError):
            interp.id = 1_000_000

    def test_hashable(self):
        interp = interpreters.create()
        expected = hash(interp.id)
        actual = hash(interp)
        self.assertEqual(actual, expected)

    def test_equality(self):
        interp1 = interpreters.create()
        interp2 = interpreters.create()
        self.assertEqual(interp1, interp1)
        self.assertNotEqual(interp1, interp2)


class TestInterpreterIsRunning(TestBase):

    def test_main(self):
        main = interpreters.get_main()
        self.assertTrue(main.is_running())

    @unittest.skip('Fails on FreeBSD')
    def test_subinterpreter(self):
        interp = interpreters.create()
        self.assertFalse(interp.is_running())

        with _running(interp):
            self.assertTrue(interp.is_running())
        self.assertFalse(interp.is_running())

    def test_finished(self):
        r, w = self.pipe()
        interp = interpreters.create()
        interp.exec_sync(f"""if True:
            import os
            os.write({w}, b'x')
            """)
        self.assertFalse(interp.is_running())
        self.assertEqual(os.read(r, 1), b'x')

    def test_from_subinterpreter(self):
        interp = interpreters.create()
        out = _run_output(interp, dedent(f"""
            import _xxsubinterpreters as _interpreters
            if _interpreters.is_running({interp.id}):
                print(True)
            else:
                print(False)
            """))
        self.assertEqual(out.strip(), 'True')

    def test_already_destroyed(self):
        interp = interpreters.create()
        interp.close()
        with self.assertRaises(InterpreterNotFoundError):
            interp.is_running()

    def test_with_only_background_threads(self):
        r_interp, w_interp = self.pipe()
        r_thread, w_thread = self.pipe()

        DONE = b'D'
        FINISHED = b'F'

        interp = interpreters.create()
        interp.exec_sync(f"""if True:
            import os
            import threading

            def task():
                v = os.read({r_thread}, 1)
                assert v == {DONE!r}
                os.write({w_interp}, {FINISHED!r})
            t = threading.Thread(target=task)
            t.start()
            """)
        self.assertFalse(interp.is_running())

        os.write(w_thread, DONE)
        interp.exec_sync('t.join()')
        self.assertEqual(os.read(r_interp, 1), FINISHED)


class TestInterpreterClose(TestBase):

    def test_basic(self):
        main = interpreters.get_main()
        interp1 = interpreters.create()
        interp2 = interpreters.create()
        interp3 = interpreters.create()
        self.assertEqual(set(interpreters.list_all()),
                         {main, interp1, interp2, interp3})
        interp2.close()
        self.assertEqual(set(interpreters.list_all()),
                         {main, interp1, interp3})

    def test_all(self):
        before = set(interpreters.list_all())
        interps = set()
        for _ in range(3):
            interp = interpreters.create()
            interps.add(interp)
        self.assertEqual(set(interpreters.list_all()), before | interps)
        for interp in interps:
            interp.close()
        self.assertEqual(set(interpreters.list_all()), before)

    def test_main(self):
        main, = interpreters.list_all()
        with self.assertRaises(RuntimeError):
            main.close()

        def f():
            with self.assertRaises(RuntimeError):
                main.close()

        t = threading.Thread(target=f)
        t.start()
        t.join()

    def test_already_destroyed(self):
        interp = interpreters.create()
        interp.close()
        with self.assertRaises(InterpreterNotFoundError):
            interp.close()

    def test_from_current(self):
        main, = interpreters.list_all()
        interp = interpreters.create()
        out = _run_output(interp, dedent(f"""
            from test.support import interpreters
            interp = interpreters.Interpreter({interp.id})
            try:
                interp.close()
            except RuntimeError:
                print('failed')
            """))
        self.assertEqual(out.strip(), 'failed')
        self.assertEqual(set(interpreters.list_all()), {main, interp})

    def test_from_sibling(self):
        main, = interpreters.list_all()
        interp1 = interpreters.create()
        interp2 = interpreters.create()
        self.assertEqual(set(interpreters.list_all()),
                         {main, interp1, interp2})
        interp1.exec_sync(dedent(f"""
            from test.support import interpreters
            interp2 = interpreters.Interpreter({interp2.id})
            interp2.close()
            interp3 = interpreters.create()
            interp3.close()
            """))
        self.assertEqual(set(interpreters.list_all()), {main, interp1})

    def test_from_other_thread(self):
        interp = interpreters.create()
        def f():
            interp.close()

        t = threading.Thread(target=f)
        t.start()
        t.join()

    @unittest.skip('Fails on FreeBSD')
    def test_still_running(self):
        main, = interpreters.list_all()
        interp = interpreters.create()
        with _running(interp):
            with self.assertRaises(RuntimeError):
                interp.close()
            self.assertTrue(interp.is_running())

    def test_subthreads_still_running(self):
        r_interp, w_interp = self.pipe()
        r_thread, w_thread = self.pipe()

        FINISHED = b'F'

        interp = interpreters.create()
        interp.exec_sync(f"""if True:
            import os
            import threading
            import time

            done = False

            def notify_fini():
                global done
                done = True
                t.join()
            threading._register_atexit(notify_fini)

            def task():
                while not done:
                    time.sleep(0.1)
                os.write({w_interp}, {FINISHED!r})
            t = threading.Thread(target=task)
            t.start()
            """)
        interp.close()

        self.assertEqual(os.read(r_interp, 1), FINISHED)


class TestInterpreterPrepareMain(TestBase):

    def test_empty(self):
        interp = interpreters.create()
        with self.assertRaises(ValueError):
            interp.prepare_main()

    def test_dict(self):
        values = {'spam': 42, 'eggs': 'ham'}
        interp = interpreters.create()
        interp.prepare_main(values)
        out = _run_output(interp, dedent("""
            print(spam, eggs)
            """))
        self.assertEqual(out.strip(), '42 ham')

    def test_tuple(self):
        values = {'spam': 42, 'eggs': 'ham'}
        values = tuple(values.items())
        interp = interpreters.create()
        interp.prepare_main(values)
        out = _run_output(interp, dedent("""
            print(spam, eggs)
            """))
        self.assertEqual(out.strip(), '42 ham')

    def test_kwargs(self):
        values = {'spam': 42, 'eggs': 'ham'}
        interp = interpreters.create()
        interp.prepare_main(**values)
        out = _run_output(interp, dedent("""
            print(spam, eggs)
            """))
        self.assertEqual(out.strip(), '42 ham')

    def test_dict_and_kwargs(self):
        values = {'spam': 42, 'eggs': 'ham'}
        interp = interpreters.create()
        interp.prepare_main(values, foo='bar')
        out = _run_output(interp, dedent("""
            print(spam, eggs, foo)
            """))
        self.assertEqual(out.strip(), '42 ham bar')

    def test_not_shareable(self):
        interp = interpreters.create()
        # XXX TypeError?
        with self.assertRaises(ValueError):
            interp.prepare_main(spam={'spam': 'eggs', 'foo': 'bar'})

        # Make sure neither was actually bound.
        with self.assertRaises(interpreters.ExecFailure):
            interp.exec_sync('print(foo)')
        with self.assertRaises(interpreters.ExecFailure):
            interp.exec_sync('print(spam)')


class TestInterpreterExecSync(TestBase):

    def test_success(self):
        interp = interpreters.create()
        script, file = _captured_script('print("it worked!", end="")')
        with file:
            interp.exec_sync(script)
            out = file.read()

        self.assertEqual(out, 'it worked!')

    def test_failure(self):
        interp = interpreters.create()
        with self.assertRaises(interpreters.ExecFailure):
            interp.exec_sync('raise Exception')

    def test_display_preserved_exception(self):
        tempdir = self.temp_dir()
        modfile = self.make_module('spam', tempdir, text="""
            def ham():
                raise RuntimeError('uh-oh!')

            def eggs():
                ham()
            """)
        scriptfile = self.make_script('script.py', tempdir, text="""
            from test.support import interpreters

            def script():
                import spam
                spam.eggs()

            interp = interpreters.create()
            interp.exec_sync(script)
            """)

        stdout, stderr = self.assert_python_failure(scriptfile)
        self.maxDiff = None
        interpmod_line, = (l for l in stderr.splitlines() if ' exec_sync' in l)
        #      File "{interpreters.__file__}", line 179, in exec_sync
        self.assertEqual(stderr, dedent(f"""\
            Traceback (most recent call last):
              File "{scriptfile}", line 9, in <module>
                interp.exec_sync(script)
                ~~~~~~~~~~~~~~~~^^^^^^^^
              {interpmod_line.strip()}
                raise ExecFailure(excinfo)
            test.support.interpreters.ExecFailure: RuntimeError: uh-oh!

            Uncaught in the interpreter:

            Traceback (most recent call last):
              File "{scriptfile}", line 6, in script
                spam.eggs()
                ~~~~~~~~~^^
              File "{modfile}", line 6, in eggs
                ham()
                ~~~^^
              File "{modfile}", line 3, in ham
                raise RuntimeError('uh-oh!')
            RuntimeError: uh-oh!
            """))
        self.assertEqual(stdout, '')

    def test_in_thread(self):
        interp = interpreters.create()
        script, file = _captured_script('print("it worked!", end="")')
        with file:
            def f():
                interp.exec_sync(script)

            t = threading.Thread(target=f)
            t.start()
            t.join()
            out = file.read()

        self.assertEqual(out, 'it worked!')

    @support.requires_fork()
    def test_fork(self):
        interp = interpreters.create()
        import tempfile
        with tempfile.NamedTemporaryFile('w+', encoding='utf-8') as file:
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
            interp.exec_sync(script)

            file.seek(0)
            content = file.read()
            self.assertEqual(content, expected)

    @unittest.skip('Fails on FreeBSD')
    def test_already_running(self):
        interp = interpreters.create()
        with _running(interp):
            with self.assertRaises(RuntimeError):
                interp.exec_sync('print("spam")')

    def test_bad_script(self):
        interp = interpreters.create()
        with self.assertRaises(TypeError):
            interp.exec_sync(10)

    def test_bytes_for_script(self):
        interp = interpreters.create()
        with self.assertRaises(TypeError):
            interp.exec_sync(b'print("spam")')

    def test_with_background_threads_still_running(self):
        r_interp, w_interp = self.pipe()
        r_thread, w_thread = self.pipe()

        RAN = b'R'
        DONE = b'D'
        FINISHED = b'F'

        interp = interpreters.create()
        interp.exec_sync(f"""if True:
            import os
            import threading

            def task():
                v = os.read({r_thread}, 1)
                assert v == {DONE!r}
                os.write({w_interp}, {FINISHED!r})
            t = threading.Thread(target=task)
            t.start()
            os.write({w_interp}, {RAN!r})
            """)
        interp.exec_sync(f"""if True:
            os.write({w_interp}, {RAN!r})
            """)

        os.write(w_thread, DONE)
        interp.exec_sync('t.join()')
        self.assertEqual(os.read(r_interp, 1), RAN)
        self.assertEqual(os.read(r_interp, 1), RAN)
        self.assertEqual(os.read(r_interp, 1), FINISHED)

    # test_xxsubinterpreters covers the remaining
    # Interpreter.exec_sync() behavior.


class TestInterpreterRun(TestBase):

    def test_success(self):
        interp = interpreters.create()
        script, file = _captured_script('print("it worked!", end="")')
        with file:
            t = interp.run(script)
            t.join()
            out = file.read()

        self.assertEqual(out, 'it worked!')

    def test_failure(self):
        caught = False
        def excepthook(args):
            nonlocal caught
            caught = True
        threading.excepthook = excepthook
        try:
            interp = interpreters.create()
            t = interp.run('raise Exception')
            t.join()

            self.assertTrue(caught)
        except BaseException:
            threading.excepthook = threading.__excepthook__


class TestIsShareable(TestBase):

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
                (),
                (1, ('spam', 'eggs'), True),
                ]
        for obj in shareables:
            with self.subTest(obj):
                shareable = interpreters.is_shareable(obj)
                self.assertTrue(shareable)

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
                    interpreters.is_shareable(obj))


if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    unittest.main()
