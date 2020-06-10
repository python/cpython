import contextlib
import os
import threading
from textwrap import dedent
import unittest
import time

import _xxsubinterpreters as _interpreters
from test.support import interpreters


def _captured_script(script):
    r, w = os.pipe()
    indented = script.replace('\n', '\n                ')
    wrapped = dedent(f"""
        import contextlib
        with open({w}, 'w') as spipe:
            with contextlib.redirect_stdout(spipe):
                {indented}
        """)
    return wrapped, open(r)


def clean_up_interpreters():
    for interp in interpreters.list_all():
        if interp.id == 0:  # main
            continue
        try:
            interp.close()
        except RuntimeError:
            pass  # already destroyed


def _run_output(interp, request, shared=None):
    script, rpipe = _captured_script(request)
    with rpipe:
        interp.run(script)
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

    def tearDown(self):
        clean_up_interpreters()


class CreateTests(TestBase):

    def test_in_main(self):
        interp = interpreters.create()
        lst = interpreters.list_all()
        self.assertEqual(interp.id, lst[1].id)

    def test_in_thread(self):
        lock = threading.Lock()
        id = None
        interp = interpreters.create()
        lst = interpreters.list_all()
        def f():
            nonlocal id
            id = interp.id
            lock.acquire()
            lock.release()

        t = threading.Thread(target=f)
        with lock:
            t.start()
        t.join()
        self.assertEqual(interp.id, lst[1].id)

    def test_in_subinterpreter(self):
        main, = interpreters.list_all()
        interp = interpreters.create()
        out = _run_output(interp, dedent("""
            from test.support import interpreters
            interp = interpreters.create()
            print(interp)
            """))
        interp2 = out.strip()

        self.assertEqual(len(set(interpreters.list_all())), len({main, interp, interp2}))

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
        self.assertEqual(len(set(interpreters.list_all())), len(before | {interp}))

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
        self.assertEqual(len(set(interpreters.list_all())), len(before | {interp3, interp}))


class GetCurrentTests(TestBase):

    def test_main(self):
        main_interp_id = _interpreters.get_main()
        cur_interp_id =  interpreters.get_current().id
        self.assertEqual(cur_interp_id, main_interp_id)

    def test_subinterpreter(self):
        main = _interpreters.get_main()
        interp = interpreters.create()
        out = _run_output(interp, dedent("""
            from test.support import interpreters
            cur = interpreters.get_current()
            print(cur)
            """))
        cur = out.strip()
        self.assertNotEqual(cur, main)


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


class TestInterpreterId(TestBase):

    def test_in_main(self):
        main = interpreters.get_current()
        self.assertEqual(0, main.id)

    def test_with_custom_num(self):
        interp = interpreters.Interpreter(1)
        self.assertEqual(1, interp.id)

    def test_for_readonly_property(self):
        interp = interpreters.Interpreter(1)
        with self.assertRaises(AttributeError):
            interp.id = 2


class TestInterpreterIsRunning(TestBase):

    def test_main(self):
        main = interpreters.get_current()
        self.assertTrue(main.is_running())

    def test_subinterpreter(self):
        interp = interpreters.create()
        self.assertFalse(interp.is_running())

        with _running(interp):
            self.assertTrue(interp.is_running())
        self.assertFalse(interp.is_running())

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


class TestInterpreterDestroy(TestBase):

    def test_basic(self):
        interp1 = interpreters.create()
        interp2 = interpreters.create()
        interp3 = interpreters.create()
        self.assertEqual(4, len(interpreters.list_all()))
        interp2.close()
        self.assertEqual(3, len(interpreters.list_all()))

    def test_all(self):
        before = set(interpreters.list_all())
        interps = set()
        for _ in range(3):
            interp = interpreters.create()
            interps.add(interp)
        self.assertEqual(len(set(interpreters.list_all())), len(before | interps))
        for interp in interps:
            interp.close()
        self.assertEqual(len(set(interpreters.list_all())), len(before))

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

    def test_from_current(self):
        main, = interpreters.list_all()
        interp = interpreters.create()
        script = dedent(f"""
            from test.support import interpreters
            try:
                main = interpreters.get_current()
                main.close()
            except RuntimeError:
                pass
            """)

        interp.run(script)
        self.assertEqual(len(set(interpreters.list_all())), len({main, interp}))

    def test_from_sibling(self):
        main, = interpreters.list_all()
        interp1 = interpreters.create()
        script = dedent(f"""
            from test.support import interpreters
            interp2 = interpreters.create()
            interp2.close()
            """)
        interp1.run(script)

        self.assertEqual(len(set(interpreters.list_all())), len({main, interp1}))

    def test_from_other_thread(self):
        interp = interpreters.create()
        def f():
            interp.close()

        t = threading.Thread(target=f)
        t.start()
        t.join()

    def test_still_running(self):
        main, = interpreters.list_all()
        interp = interpreters.create()
        with _running(interp):
            with self.assertRaises(RuntimeError):
                interp.close()
            self.assertTrue(interp.is_running())


class TestInterpreterRun(TestBase):

    SCRIPT = dedent("""
        with open('{}', 'w') as out:
            out.write('{}')
        """)
    FILENAME = 'spam'

    def setUp(self):
        super().setUp()
        self.interp = interpreters.create()
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
            self.interp.run(script)
            out = file.read()

        self.assertEqual(out, 'it worked!')

    def test_in_thread(self):
        script, file = _captured_script('print("it worked!", end="")')
        with file:
            def f():
                self.interp.run(script)

            t = threading.Thread(target=f)
            t.start()
            t.join()
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
                import os
                try:
                    os.fork()
                except RuntimeError:
                    with open('{file.name}', 'w') as out:
                        out.write('{expected}')
                """)
            self.interp.run(script)

            file.seek(0)
            content = file.read()
            self.assertEqual(content, expected)

    def test_already_running(self):
        with _running(self.interp):
            with self.assertRaises(RuntimeError):
                self.interp.run('print("spam")')

    def test_bad_script(self):
        with self.assertRaises(TypeError):
            self.interp.run(10)

    def test_bytes_for_script(self):
        with self.assertRaises(TypeError):
            self.interp.run(b'print("spam")')


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


class TestChannel(TestBase):

    def test_create_cid(self):
        r, s = interpreters.create_channel()
        self.assertIsInstance(r, interpreters.RecvChannel)
        self.assertIsInstance(s, interpreters.SendChannel)

    def test_sequential_ids(self):
        before = interpreters.list_all_channels()
        channels1 = interpreters.create_channel()
        channels2 = interpreters.create_channel()
        channels3 = interpreters.create_channel()
        after = interpreters.list_all_channels()

        self.assertEqual(len(set(after) - set(before)),
                         len({channels1, channels2, channels3}))


class TestSendRecv(TestBase):

    def test_send_recv_main(self):
        r, s = interpreters.create_channel()
        orig = b'spam'
        s.send(orig)
        obj = r.recv()

        self.assertEqual(obj, orig)
        self.assertIsNot(obj, orig)

    def test_send_recv_same_interpreter(self):
        interp = interpreters.create()
        out = _run_output(interp, dedent("""
            from test.support import interpreters
            r, s = interpreters.create_channel()
            orig = b'spam'
            s.send(orig)
            obj = r.recv()
            assert obj is not orig
            assert obj == orig
            """))

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

        s.send(b'spam')
        t.join()
        obj = r.recv()

        self.assertEqual(obj, b'spam')

    def test_send_recv_nowait_main(self):
        r, s = interpreters.create_channel()
        orig = b'spam'
        s.send(orig)
        obj = r.recv_nowait()

        self.assertEqual(obj, orig)
        self.assertIsNot(obj, orig)

    def test_send_recv_nowait_same_interpreter(self):
        interp = interpreters.create()
        out = _run_output(interp, dedent("""
            from test.support import interpreters
            r, s = interpreters.create_channel()
            orig = b'spam'
            s.send(orig)
            obj = r.recv_nowait()
            assert obj is not orig
            assert obj == orig
            """))

        r, s = interpreters.create_channel()

        def f():
            while True:
                try:
                    obj = r.recv_nowait()
                    break
                except _interpreters.ChannelEmptyError:
                    time.sleep(0.1)
            s.send(obj)
