import contextlib
import json
import os
import os.path
import sys
import threading
from textwrap import dedent
import unittest
import time

from test import support
from test.support import import_helper
from test.support import threading_helper
from test.support import os_helper
_interpreters = import_helper.import_module('_xxsubinterpreters')
_channels = import_helper.import_module('_xxinterpchannels')
from test.support import interpreters


def _captured_script(script):
    r, w = os.pipe()
    indented = script.replace('\n', '\n                ')
    wrapped = dedent(f"""
        import contextlib
        with open({w}, 'w', encoding='utf-8') as spipe:
            with contextlib.redirect_stdout(spipe):
                {indented}
        """)
    return wrapped, open(r, encoding='utf-8')


def clean_up_interpreters():
    for interp in interpreters.list_all():
        if interp.id == 0:  # main
            continue
        try:
            interp.close()
        except RuntimeError:
            pass  # already destroyed


def _run_output(interp, request, channels=None):
    script, rpipe = _captured_script(request)
    with rpipe:
        interp.run(script, channels=channels)
        return rpipe.read()


@contextlib.contextmanager
def _running(interp):
    r, w = os.pipe()
    def run():
        interp.run(dedent(f"""
            # wait for "signal"
            with open({r}) as rpipe:
                rpipe.read()
            """))

    t = threading.Thread(target=run)
    t.start()

    yield

    with open(w, 'w') as spipe:
        spipe.write('done')
    t.join()


class TestBase(unittest.TestCase):

    def os_pipe(self):
        r, w = os.pipe()
        def cleanup():
            try:
                os.close(w)
            except Exception:
                pass
            try:
                os.close(r)
            except Exception:
                pass
        self.addCleanup(cleanup)
        return r, w

    def tearDown(self):
        clean_up_interpreters()


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


class GetCurrentTests(TestBase):

    def test_main(self):
        main = interpreters.get_main()
        current = interpreters.get_current()
        self.assertEqual(current, main)

    def test_subinterpreter(self):
        main = _interpreters.get_main()
        interp = interpreters.create()
        out = _run_output(interp, dedent("""
            from test.support import interpreters
            cur = interpreters.get_current()
            print(cur.id)
            """))
        current = interpreters.Interpreter(int(out))
        self.assertNotEqual(current, main)


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


class TestInterpreterAttrs(TestBase):

    def test_id_type(self):
        main = interpreters.get_main()
        current = interpreters.get_current()
        interp = interpreters.create()
        self.assertIsInstance(main.id, _interpreters.InterpreterID)
        self.assertIsInstance(current.id, _interpreters.InterpreterID)
        self.assertIsInstance(interp.id, _interpreters.InterpreterID)

    def test_main_id(self):
        main = interpreters.get_main()
        self.assertEqual(main.id, 0)

    def test_custom_id(self):
        interp = interpreters.Interpreter(1)
        self.assertEqual(interp.id, 1)

        with self.assertRaises(TypeError):
            interpreters.Interpreter('1')

    def test_id_readonly(self):
        interp = interpreters.Interpreter(1)
        with self.assertRaises(AttributeError):
            interp.id = 2

    @unittest.skip('not ready yet (see bpo-32604)')
    def test_main_isolated(self):
        main = interpreters.get_main()
        self.assertFalse(main.isolated)

    @unittest.skip('not ready yet (see bpo-32604)')
    def test_subinterpreter_isolated_default(self):
        interp = interpreters.create()
        self.assertFalse(interp.isolated)

    def test_subinterpreter_isolated_explicit(self):
        interp1 = interpreters.create(isolated=True)
        interp2 = interpreters.create(isolated=False)
        self.assertTrue(interp1.isolated)
        self.assertFalse(interp2.isolated)

    @unittest.skip('not ready yet (see bpo-32604)')
    def test_custom_isolated_default(self):
        interp = interpreters.Interpreter(1)
        self.assertFalse(interp.isolated)

    def test_custom_isolated_explicit(self):
        interp1 = interpreters.Interpreter(1, isolated=True)
        interp2 = interpreters.Interpreter(1, isolated=False)
        self.assertTrue(interp1.isolated)
        self.assertFalse(interp2.isolated)

    def test_isolated_readonly(self):
        interp = interpreters.Interpreter(1)
        with self.assertRaises(AttributeError):
            interp.isolated = True

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
        r, w = self.os_pipe()
        interp = interpreters.create()
        interp.run(f"""if True:
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
        with self.assertRaises(RuntimeError):
            interp.is_running()

    def test_does_not_exist(self):
        interp = interpreters.Interpreter(1_000_000)
        with self.assertRaises(RuntimeError):
            interp.is_running()

    def test_bad_id(self):
        interp = interpreters.Interpreter(-1)
        with self.assertRaises(ValueError):
            interp.is_running()

    def test_with_only_background_threads(self):
        r_interp, w_interp = self.os_pipe()
        r_thread, w_thread = self.os_pipe()

        DONE = b'D'
        FINISHED = b'F'

        interp = interpreters.create()
        interp.run(f"""if True:
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
        interp.run('t.join()')
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
        with self.assertRaises(RuntimeError):
            interp.close()

    def test_does_not_exist(self):
        interp = interpreters.Interpreter(1_000_000)
        with self.assertRaises(RuntimeError):
            interp.close()

    def test_bad_id(self):
        interp = interpreters.Interpreter(-1)
        with self.assertRaises(ValueError):
            interp.close()

    def test_from_current(self):
        main, = interpreters.list_all()
        interp = interpreters.create()
        out = _run_output(interp, dedent(f"""
            from test.support import interpreters
            interp = interpreters.Interpreter({int(interp.id)})
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
        interp1.run(dedent(f"""
            from test.support import interpreters
            interp2 = interpreters.Interpreter(int({interp2.id}))
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
        r_interp, w_interp = self.os_pipe()
        r_thread, w_thread = self.os_pipe()

        FINISHED = b'F'

        interp = interpreters.create()
        interp.run(f"""if True:
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


class TestInterpreterRun(TestBase):

    def test_success(self):
        interp = interpreters.create()
        script, file = _captured_script('print("it worked!", end="")')
        with file:
            interp.run(script)
            out = file.read()

        self.assertEqual(out, 'it worked!')

    def test_in_thread(self):
        interp = interpreters.create()
        script, file = _captured_script('print("it worked!", end="")')
        with file:
            def f():
                interp.run(script)

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
            interp.run(script)

            file.seek(0)
            content = file.read()
            self.assertEqual(content, expected)

    @unittest.skip('Fails on FreeBSD')
    def test_already_running(self):
        interp = interpreters.create()
        with _running(interp):
            with self.assertRaises(RuntimeError):
                interp.run('print("spam")')

    def test_does_not_exist(self):
        interp = interpreters.Interpreter(1_000_000)
        with self.assertRaises(RuntimeError):
            interp.run('print("spam")')

    def test_bad_id(self):
        interp = interpreters.Interpreter(-1)
        with self.assertRaises(ValueError):
            interp.run('print("spam")')

    def test_bad_script(self):
        interp = interpreters.create()
        with self.assertRaises(TypeError):
            interp.run(10)

    def test_bytes_for_script(self):
        interp = interpreters.create()
        with self.assertRaises(TypeError):
            interp.run(b'print("spam")')

    def test_with_background_threads_still_running(self):
        r_interp, w_interp = self.os_pipe()
        r_thread, w_thread = self.os_pipe()

        RAN = b'R'
        DONE = b'D'
        FINISHED = b'F'

        interp = interpreters.create()
        interp.run(f"""if True:
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
        interp.run(f"""if True:
            os.write({w_interp}, {RAN!r})
            """)

        os.write(w_thread, DONE)
        interp.run('t.join()')
        self.assertEqual(os.read(r_interp, 1), RAN)
        self.assertEqual(os.read(r_interp, 1), RAN)
        self.assertEqual(os.read(r_interp, 1), FINISHED)

    # test_xxsubinterpreters covers the remaining Interpreter.run() behavior.


@unittest.skip('these are crashing, likely just due just to _xxsubinterpreters (see gh-105699)')
class StressTests(TestBase):

    # In these tests we generally want a lot of interpreters,
    # but not so many that any test takes too long.

    @support.requires_resource('cpu')
    def test_create_many_sequential(self):
        alive = []
        for _ in range(100):
            interp = interpreters.create()
            alive.append(interp)

    @support.requires_resource('cpu')
    def test_create_many_threaded(self):
        alive = []
        def task():
            interp = interpreters.create()
            alive.append(interp)
        threads = (threading.Thread(target=task) for _ in range(200))
        with threading_helper.start_threads(threads):
            pass


class StartupTests(TestBase):

    # We want to ensure the initial state of subinterpreters
    # matches expectations.

    _subtest_count = 0

    @contextlib.contextmanager
    def subTest(self, *args):
        with super().subTest(*args) as ctx:
            self._subtest_count += 1
            try:
                yield ctx
            finally:
                if self._debugged_in_subtest:
                    if self._subtest_count == 1:
                        # The first subtest adds a leading newline, so we
                        # compensate here by not printing a trailing newline.
                        print('### end subtest debug ###', end='')
                    else:
                        print('### end subtest debug ###')
                self._debugged_in_subtest = False

    def debug(self, msg, *, header=None):
        if header:
            self._debug(f'--- {header} ---')
            if msg:
                if msg.endswith(os.linesep):
                    self._debug(msg[:-len(os.linesep)])
                else:
                    self._debug(msg)
                    self._debug('<no newline>')
            self._debug('------')
        else:
            self._debug(msg)

    _debugged = False
    _debugged_in_subtest = False
    def _debug(self, msg):
        if not self._debugged:
            print()
            self._debugged = True
        if self._subtest is not None:
            if True:
                if not self._debugged_in_subtest:
                    self._debugged_in_subtest = True
                    print('### start subtest debug ###')
                print(msg)
        else:
            print(msg)

    def create_temp_dir(self):
        import tempfile
        tmp = tempfile.mkdtemp(prefix='test_interpreters_')
        tmp = os.path.realpath(tmp)
        self.addCleanup(os_helper.rmtree, tmp)
        return tmp

    def write_script(self, *path, text):
        filename = os.path.join(*path)
        dirname = os.path.dirname(filename)
        if dirname:
            os.makedirs(dirname, exist_ok=True)
        with open(filename, 'w', encoding='utf-8') as outfile:
            outfile.write(dedent(text))
        return filename

    @support.requires_subprocess()
    def run_python(self, argv, *, cwd=None):
        # This method is inspired by
        # EmbeddingTestsMixin.run_embedded_interpreter() in test_embed.py.
        import shlex
        import subprocess
        if isinstance(argv, str):
            argv = shlex.split(argv)
        argv = [sys.executable, *argv]
        try:
            proc = subprocess.run(
                argv,
                cwd=cwd,
                capture_output=True,
                text=True,
            )
        except Exception as exc:
            self.debug(f'# cmd: {shlex.join(argv)}')
            if isinstance(exc, FileNotFoundError) and not exc.filename:
                if os.path.exists(argv[0]):
                    exists = 'exists'
                else:
                    exists = 'does not exist'
                self.debug(f'{argv[0]} {exists}')
            raise  # re-raise
        assert proc.stderr == '' or proc.returncode != 0, proc.stderr
        if proc.returncode != 0 and support.verbose:
            self.debug(f'# python3 {shlex.join(argv[1:])} failed:')
            self.debug(proc.stdout, header='stdout')
            self.debug(proc.stderr, header='stderr')
        self.assertEqual(proc.returncode, 0)
        self.assertEqual(proc.stderr, '')
        return proc.stdout

    def test_sys_path_0(self):
        # The main interpreter's sys.path[0] should be used by subinterpreters.
        script = '''
            import sys
            from test.support import interpreters

            orig = sys.path[0]

            interp = interpreters.create()
            interp.run(f"""if True:
                import json
                import sys
                print(json.dumps({{
                    'main': {orig!r},
                    'sub': sys.path[0],
                }}, indent=4), flush=True)
                """)
            '''
        # <tmp>/
        #   pkg/
        #     __init__.py
        #     __main__.py
        #     script.py
        #   script.py
        cwd = self.create_temp_dir()
        self.write_script(cwd, 'pkg', '__init__.py', text='')
        self.write_script(cwd, 'pkg', '__main__.py', text=script)
        self.write_script(cwd, 'pkg', 'script.py', text=script)
        self.write_script(cwd, 'script.py', text=script)

        cases = [
            ('script.py', cwd),
            ('-m script', cwd),
            ('-m pkg', cwd),
            ('-m pkg.script', cwd),
            ('-c "import script"', ''),
        ]
        for argv, expected in cases:
            with self.subTest(f'python3 {argv}'):
                out = self.run_python(argv, cwd=cwd)
                data = json.loads(out)
                sp0_main, sp0_sub = data['main'], data['sub']
                self.assertEqual(sp0_sub, sp0_main)
                self.assertEqual(sp0_sub, expected)


class FinalizationTests(TestBase):

    def test_gh_109793(self):
        import subprocess
        argv = [sys.executable, '-c', '''if True:
            import _xxsubinterpreters as _interpreters
            interpid = _interpreters.create()
            raise Exception
            ''']
        proc = subprocess.run(argv, capture_output=True, text=True)
        self.assertIn('Traceback', proc.stderr)
        if proc.returncode == 0 and support.verbose:
            print()
            print("--- cmd unexpected succeeded ---")
            print(f"stdout:\n{proc.stdout}")
            print(f"stderr:\n{proc.stderr}")
            print("------")
        self.assertEqual(proc.returncode, 1)


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


class TestChannels(TestBase):

    def test_create(self):
        r, s = interpreters.create_channel()
        self.assertIsInstance(r, interpreters.RecvChannel)
        self.assertIsInstance(s, interpreters.SendChannel)

    def test_list_all(self):
        self.assertEqual(interpreters.list_all_channels(), [])
        created = set()
        for _ in range(3):
            ch = interpreters.create_channel()
            created.add(ch)
        after = set(interpreters.list_all_channels())
        self.assertEqual(after, created)


class TestRecvChannelAttrs(TestBase):

    def test_id_type(self):
        rch, _ = interpreters.create_channel()
        self.assertIsInstance(rch.id, _channels.ChannelID)

    def test_custom_id(self):
        rch = interpreters.RecvChannel(1)
        self.assertEqual(rch.id, 1)

        with self.assertRaises(TypeError):
            interpreters.RecvChannel('1')

    def test_id_readonly(self):
        rch = interpreters.RecvChannel(1)
        with self.assertRaises(AttributeError):
            rch.id = 2

    def test_equality(self):
        ch1, _ = interpreters.create_channel()
        ch2, _ = interpreters.create_channel()
        self.assertEqual(ch1, ch1)
        self.assertNotEqual(ch1, ch2)


class TestSendChannelAttrs(TestBase):

    def test_id_type(self):
        _, sch = interpreters.create_channel()
        self.assertIsInstance(sch.id, _channels.ChannelID)

    def test_custom_id(self):
        sch = interpreters.SendChannel(1)
        self.assertEqual(sch.id, 1)

        with self.assertRaises(TypeError):
            interpreters.SendChannel('1')

    def test_id_readonly(self):
        sch = interpreters.SendChannel(1)
        with self.assertRaises(AttributeError):
            sch.id = 2

    def test_equality(self):
        _, ch1 = interpreters.create_channel()
        _, ch2 = interpreters.create_channel()
        self.assertEqual(ch1, ch1)
        self.assertNotEqual(ch1, ch2)


class TestSendRecv(TestBase):

    def test_send_recv_main(self):
        r, s = interpreters.create_channel()
        orig = b'spam'
        s.send_nowait(orig)
        obj = r.recv()

        self.assertEqual(obj, orig)
        self.assertIsNot(obj, orig)

    def test_send_recv_same_interpreter(self):
        interp = interpreters.create()
        interp.run(dedent("""
            from test.support import interpreters
            r, s = interpreters.create_channel()
            orig = b'spam'
            s.send_nowait(orig)
            obj = r.recv()
            assert obj == orig, 'expected: obj == orig'
            assert obj is not orig, 'expected: obj is not orig'
            """))

    @unittest.skip('broken (see BPO-...)')
    def test_send_recv_different_interpreters(self):
        r1, s1 = interpreters.create_channel()
        r2, s2 = interpreters.create_channel()
        orig1 = b'spam'
        s1.send_nowait(orig1)
        out = _run_output(
            interpreters.create(),
            dedent(f"""
                obj1 = r.recv()
                assert obj1 == b'spam', 'expected: obj1 == orig1'
                # When going to another interpreter we get a copy.
                assert id(obj1) != {id(orig1)}, 'expected: obj1 is not orig1'
                orig2 = b'eggs'
                print(id(orig2))
                s.send_nowait(orig2)
                """),
            channels=dict(r=r1, s=s2),
            )
        obj2 = r2.recv()

        self.assertEqual(obj2, b'eggs')
        self.assertNotEqual(id(obj2), int(out))

    def test_send_recv_different_threads(self):
        r, s = interpreters.create_channel()

        def f():
            while True:
                try:
                    obj = r.recv()
                    break
                except interpreters.ChannelEmptyError:
                    time.sleep(0.1)
            s.send(obj)
        t = threading.Thread(target=f)
        t.start()

        orig = b'spam'
        s.send(orig)
        t.join()
        obj = r.recv()

        self.assertEqual(obj, orig)
        self.assertIsNot(obj, orig)

    def test_send_recv_nowait_main(self):
        r, s = interpreters.create_channel()
        orig = b'spam'
        s.send_nowait(orig)
        obj = r.recv_nowait()

        self.assertEqual(obj, orig)
        self.assertIsNot(obj, orig)

    def test_send_recv_nowait_main_with_default(self):
        r, _ = interpreters.create_channel()
        obj = r.recv_nowait(None)

        self.assertIsNone(obj)

    def test_send_recv_nowait_same_interpreter(self):
        interp = interpreters.create()
        interp.run(dedent("""
            from test.support import interpreters
            r, s = interpreters.create_channel()
            orig = b'spam'
            s.send_nowait(orig)
            obj = r.recv_nowait()
            assert obj == orig, 'expected: obj == orig'
            # When going back to the same interpreter we get the same object.
            assert obj is not orig, 'expected: obj is not orig'
            """))

    @unittest.skip('broken (see BPO-...)')
    def test_send_recv_nowait_different_interpreters(self):
        r1, s1 = interpreters.create_channel()
        r2, s2 = interpreters.create_channel()
        orig1 = b'spam'
        s1.send_nowait(orig1)
        out = _run_output(
            interpreters.create(),
            dedent(f"""
                obj1 = r.recv_nowait()
                assert obj1 == b'spam', 'expected: obj1 == orig1'
                # When going to another interpreter we get a copy.
                assert id(obj1) != {id(orig1)}, 'expected: obj1 is not orig1'
                orig2 = b'eggs'
                print(id(orig2))
                s.send_nowait(orig2)
                """),
            channels=dict(r=r1, s=s2),
            )
        obj2 = r2.recv_nowait()

        self.assertEqual(obj2, b'eggs')
        self.assertNotEqual(id(obj2), int(out))

    def test_recv_channel_does_not_exist(self):
        ch = interpreters.RecvChannel(1_000_000)
        with self.assertRaises(interpreters.ChannelNotFoundError):
            ch.recv()

    def test_send_channel_does_not_exist(self):
        ch = interpreters.SendChannel(1_000_000)
        with self.assertRaises(interpreters.ChannelNotFoundError):
            ch.send(b'spam')

    def test_recv_nowait_channel_does_not_exist(self):
        ch = interpreters.RecvChannel(1_000_000)
        with self.assertRaises(interpreters.ChannelNotFoundError):
            ch.recv_nowait()

    def test_send_nowait_channel_does_not_exist(self):
        ch = interpreters.SendChannel(1_000_000)
        with self.assertRaises(interpreters.ChannelNotFoundError):
            ch.send_nowait(b'spam')

    def test_recv_nowait_empty(self):
        ch, _ = interpreters.create_channel()
        with self.assertRaises(interpreters.ChannelEmptyError):
            ch.recv_nowait()

    def test_recv_nowait_default(self):
        default = object()
        rch, sch = interpreters.create_channel()
        obj1 = rch.recv_nowait(default)
        sch.send_nowait(None)
        sch.send_nowait(1)
        sch.send_nowait(b'spam')
        sch.send_nowait(b'eggs')
        obj2 = rch.recv_nowait(default)
        obj3 = rch.recv_nowait(default)
        obj4 = rch.recv_nowait()
        obj5 = rch.recv_nowait(default)
        obj6 = rch.recv_nowait(default)

        self.assertIs(obj1, default)
        self.assertIs(obj2, None)
        self.assertEqual(obj3, 1)
        self.assertEqual(obj4, b'spam')
        self.assertEqual(obj5, b'eggs')
        self.assertIs(obj6, default)


if __name__ == "__main__":
    unittest.main()
