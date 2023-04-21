from collections import namedtuple
import contextlib
import itertools
import os
import pickle
import sys
from textwrap import dedent
import threading
import time
import unittest

from test import support
from test.support import script_helper


interpreters = support.import_module('_xxsubinterpreters')


##################################
# helpers

def powerset(*sets):
    return itertools.chain.from_iterable(
        combinations(sets, r)
        for r in range(len(sets)+1))


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


def _run_output(interp, request, shared=None):
    script, rpipe = _captured_script(request)
    with rpipe:
        interpreters.run_string(interp, script, shared)
        return rpipe.read()


@contextlib.contextmanager
def _running(interp):
    r, w = os.pipe()
    def run():
        interpreters.run_string(interp, dedent(f"""
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


#@contextmanager
#def run_threaded(id, source, **shared):
#    def run():
#        run_interp(id, source, **shared)
#    t = threading.Thread(target=run)
#    t.start()
#    yield
#    t.join()


def run_interp(id, source, **shared):
    _run_interp(id, source, shared)


def _run_interp(id, source, shared, _mainns={}):
    source = dedent(source)
    main = interpreters.get_main()
    if main == id:
        if interpreters.get_current() != main:
            raise RuntimeError
        # XXX Run a func?
        exec(source, _mainns)
    else:
        interpreters.run_string(id, source, shared)


def run_interp_threaded(id, source, **shared):
    def run():
        _run(id, source, shared)
    t = threading.Thread(target=run)
    t.start()
    t.join()


class Interpreter(namedtuple('Interpreter', 'name id')):

    @classmethod
    def from_raw(cls, raw):
        if isinstance(raw, cls):
            return raw
        elif isinstance(raw, str):
            return cls(raw)
        else:
            raise NotImplementedError

    def __new__(cls, name=None, id=None):
        main = interpreters.get_main()
        if id == main:
            if not name:
                name = 'main'
            elif name != 'main':
                raise ValueError(
                    'name mismatch (expected "main", got "{}")'.format(name))
            id = main
        elif id is not None:
            if not name:
                name = 'interp'
            elif name == 'main':
                raise ValueError('name mismatch (unexpected "main")')
            if not isinstance(id, interpreters.InterpreterID):
                id = interpreters.InterpreterID(id)
        elif not name or name == 'main':
            name = 'main'
            id = main
        else:
            id = interpreters.create()
        self = super().__new__(cls, name, id)
        return self


# XXX expect_channel_closed() is unnecessary once we improve exc propagation.

@contextlib.contextmanager
def expect_channel_closed():
    try:
        yield
    except interpreters.ChannelClosedError:
        pass
    else:
        assert False, 'channel not closed'


class ChannelAction(namedtuple('ChannelAction', 'action end interp')):

    def __new__(cls, action, end=None, interp=None):
        if not end:
            end = 'both'
        if not interp:
            interp = 'main'
        self = super().__new__(cls, action, end, interp)
        return self

    def __init__(self, *args, **kwargs):
        if self.action == 'use':
            if self.end not in ('same', 'opposite', 'send', 'recv'):
                raise ValueError(self.end)
        elif self.action in ('close', 'force-close'):
            if self.end not in ('both', 'same', 'opposite', 'send', 'recv'):
                raise ValueError(self.end)
        else:
            raise ValueError(self.action)
        if self.interp not in ('main', 'same', 'other', 'extra'):
            raise ValueError(self.interp)

    def resolve_end(self, end):
        if self.end == 'same':
            return end
        elif self.end == 'opposite':
            return 'recv' if end == 'send' else 'send'
        else:
            return self.end

    def resolve_interp(self, interp, other, extra):
        if self.interp == 'same':
            return interp
        elif self.interp == 'other':
            if other is None:
                raise RuntimeError
            return other
        elif self.interp == 'extra':
            if extra is None:
                raise RuntimeError
            return extra
        elif self.interp == 'main':
            if interp.name == 'main':
                return interp
            elif other and other.name == 'main':
                return other
            else:
                raise RuntimeError
        # Per __init__(), there aren't any others.


class ChannelState(namedtuple('ChannelState', 'pending closed')):

    def __new__(cls, pending=0, *, closed=False):
        self = super().__new__(cls, pending, closed)
        return self

    def incr(self):
        return type(self)(self.pending + 1, closed=self.closed)

    def decr(self):
        return type(self)(self.pending - 1, closed=self.closed)

    def close(self, *, force=True):
        if self.closed:
            if not force or self.pending == 0:
                return self
        return type(self)(0 if force else self.pending, closed=True)


def run_action(cid, action, end, state, *, hideclosed=True):
    if state.closed:
        if action == 'use' and end == 'recv' and state.pending:
            expectfail = False
        else:
            expectfail = True
    else:
        expectfail = False

    try:
        result = _run_action(cid, action, end, state)
    except interpreters.ChannelClosedError:
        if not hideclosed and not expectfail:
            raise
        result = state.close()
    else:
        if expectfail:
            raise ...  # XXX
    return result


def _run_action(cid, action, end, state):
    if action == 'use':
        if end == 'send':
            interpreters.channel_send(cid, b'spam')
            return state.incr()
        elif end == 'recv':
            if not state.pending:
                try:
                    interpreters.channel_recv(cid)
                except interpreters.ChannelEmptyError:
                    return state
                else:
                    raise Exception('expected ChannelEmptyError')
            else:
                interpreters.channel_recv(cid)
                return state.decr()
        else:
            raise ValueError(end)
    elif action == 'close':
        kwargs = {}
        if end in ('recv', 'send'):
            kwargs[end] = True
        interpreters.channel_close(cid, **kwargs)
        return state.close()
    elif action == 'force-close':
        kwargs = {
            'force': True,
            }
        if end in ('recv', 'send'):
            kwargs[end] = True
        interpreters.channel_close(cid, **kwargs)
        return state.close(force=True)
    else:
        raise ValueError(action)


def clean_up_interpreters():
    for id in interpreters.list_all():
        if id == 0:  # main
            continue
        try:
            interpreters.destroy(id)
        except RuntimeError:
            pass  # already destroyed


def clean_up_channels():
    for cid in interpreters.channel_list_all():
        try:
            interpreters.channel_destroy(cid)
        except interpreters.ChannelNotFoundError:
            pass  # already destroyed


class TestBase(unittest.TestCase):

    def tearDown(self):
        clean_up_interpreters()
        clean_up_channels()


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

    def setUp(self):
        super().setUp()
        self.cid = interpreters.channel_create()

    def tearDown(self):
        interpreters.channel_destroy(self.cid)
        super().tearDown()

    def _assert_values(self, values):
        for obj in values:
            with self.subTest(obj):
                interpreters.channel_send(self.cid, obj)
                got = interpreters.channel_recv(self.cid)

                self.assertEqual(got, obj)
                self.assertIs(type(got), type(obj))
                # XXX Check the following in the channel tests?
                #self.assertIsNot(got, obj)

    def test_singletons(self):
        for obj in [None]:
            with self.subTest(obj):
                interpreters.channel_send(self.cid, obj)
                got = interpreters.channel_recv(self.cid)

                # XXX What about between interpreters?
                self.assertIs(got, obj)

    def test_types(self):
        self._assert_values([
            b'spam',
            9999,
            self.cid,
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
                    interpreters.channel_send(self.cid, i)


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
        id = interpreters.channel_create()
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
                import os
                try:
                    os.fork()
                except RuntimeError:
                    with open('{file.name}', 'w') as out:
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


##################################
# channel tests

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
        class Int(str):
            def __index__(self):
                return 10

        cid = interpreters._channel_id(Int(), force=True)
        self.assertEqual(int(cid), 10)

    def test_bad_id(self):
        self.assertRaises(TypeError, interpreters._channel_id, object())
        self.assertRaises(TypeError, interpreters._channel_id, 10.0)
        self.assertRaises(TypeError, interpreters._channel_id, '10')
        self.assertRaises(TypeError, interpreters._channel_id, b'10')
        self.assertRaises(ValueError, interpreters._channel_id, -1)
        self.assertRaises(OverflowError, interpreters._channel_id, 2**64)

    def test_bad_kwargs(self):
        with self.assertRaises(ValueError):
            interpreters._channel_id(10, send=False, recv=False)

    def test_does_not_exist(self):
        cid = interpreters.channel_create()
        with self.assertRaises(interpreters.ChannelNotFoundError):
            interpreters._channel_id(int(cid) + 1)  # unforced

    def test_str(self):
        cid = interpreters._channel_id(10, force=True)
        self.assertEqual(str(cid), '10')

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
        self.assertTrue(int(cid1) == cid1)
        self.assertTrue(cid1 == float(int(cid1)))
        self.assertTrue(float(int(cid1)) == cid1)
        self.assertFalse(cid1 == float(int(cid1)) + 0.1)
        self.assertFalse(cid1 == str(int(cid1)))
        self.assertFalse(cid1 == 2**1000)
        self.assertFalse(cid1 == float('inf'))
        self.assertFalse(cid1 == 'spam')
        self.assertFalse(cid1 == cid3)

        self.assertFalse(cid1 != cid1)
        self.assertFalse(cid1 != cid2)
        self.assertTrue(cid1 != cid3)


class ChannelTests(TestBase):

    def test_create_cid(self):
        cid = interpreters.channel_create()
        self.assertIsInstance(cid, interpreters.ChannelID)

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
            print(cid)
            """))
        cid1 = int(out.strip())

        id2 = interpreters.create()
        out = _run_output(id2, dedent("""
            import _xxsubinterpreters as _interpreters
            cid = _interpreters.channel_create()
            print(cid)
            """))
        cid2 = int(out.strip())

        self.assertEqual(cid2, int(cid1) + 1)

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
            _interpreters.channel_send({cid}, b'spam')
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
                        obj = _interpreters.channel_recv({cid})
                        break
                    except _interpreters.ChannelEmptyError:
                        time.sleep(0.1)
                assert(obj == b'spam')
                _interpreters.channel_send({cid}, b'eggs')
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

    def test_run_string_arg_unresolved(self):
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

    # XXX For now there is no high-level channel into which the
    # sent channel ID can be converted...
    # Note: this test caused crashes on some buildbots (bpo-33615).
    @unittest.skip('disabled until high-level channels exist')
    def test_run_string_arg_resolved(self):
        cid = interpreters.channel_create()
        cid = interpreters._channel_id(cid, _resolve=True)
        interp = interpreters.create()

        out = _run_output(interp, dedent("""
            import _xxsubinterpreters as _interpreters
            print(chan.id.end)
            _interpreters.channel_send(chan.id, b'spam')
            """),
            dict(chan=cid.send))
        obj = interpreters.channel_recv(cid)

        self.assertEqual(obj, b'spam')
        self.assertEqual(out.strip(), 'send')

    # close

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
            _interpreters.channel_send({cid}, b'spam')
            """))
        interpreters.run_string(id2, dedent(f"""
            import _xxsubinterpreters as _interpreters
            _interpreters.channel_recv({cid})
            """))
        interpreters.channel_close(cid)
        with self.assertRaises(interpreters.RunFailedError) as cm:
            interpreters.run_string(id1, dedent(f"""
                _interpreters.channel_send({cid}, b'spam')
                """))
        self.assertIn('ChannelClosedError', str(cm.exception))
        with self.assertRaises(interpreters.RunFailedError) as cm:
            interpreters.run_string(id2, dedent(f"""
                _interpreters.channel_send({cid}, b'spam')
                """))
        self.assertIn('ChannelClosedError', str(cm.exception))

    def test_close_multiple_times(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_recv(cid)
        interpreters.channel_close(cid)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_close(cid)

    def test_close_empty(self):
        tests = [
            (False, False),
            (True, False),
            (False, True),
            (True, True),
            ]
        for send, recv in tests:
            with self.subTest((send, recv)):
                cid = interpreters.channel_create()
                interpreters.channel_send(cid, b'spam')
                interpreters.channel_recv(cid)
                interpreters.channel_close(cid, send=send, recv=recv)

                with self.assertRaises(interpreters.ChannelClosedError):
                    interpreters.channel_send(cid, b'eggs')
                with self.assertRaises(interpreters.ChannelClosedError):
                    interpreters.channel_recv(cid)

    def test_close_defaults_with_unused_items(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_send(cid, b'ham')

        with self.assertRaises(interpreters.ChannelNotEmptyError):
            interpreters.channel_close(cid)
        interpreters.channel_recv(cid)
        interpreters.channel_send(cid, b'eggs')

    def test_close_recv_with_unused_items_unforced(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_send(cid, b'ham')

        with self.assertRaises(interpreters.ChannelNotEmptyError):
            interpreters.channel_close(cid, recv=True)
        interpreters.channel_recv(cid)
        interpreters.channel_send(cid, b'eggs')
        interpreters.channel_recv(cid)
        interpreters.channel_recv(cid)
        interpreters.channel_close(cid, recv=True)

    def test_close_send_with_unused_items_unforced(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_send(cid, b'ham')
        interpreters.channel_close(cid, send=True)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
        interpreters.channel_recv(cid)
        interpreters.channel_recv(cid)
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_close_both_with_unused_items_unforced(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_send(cid, b'ham')

        with self.assertRaises(interpreters.ChannelNotEmptyError):
            interpreters.channel_close(cid, recv=True, send=True)
        interpreters.channel_recv(cid)
        interpreters.channel_send(cid, b'eggs')
        interpreters.channel_recv(cid)
        interpreters.channel_recv(cid)
        interpreters.channel_close(cid, recv=True)

    def test_close_recv_with_unused_items_forced(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_send(cid, b'ham')
        interpreters.channel_close(cid, recv=True, force=True)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_close_send_with_unused_items_forced(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_send(cid, b'ham')
        interpreters.channel_close(cid, send=True, force=True)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_close_both_with_unused_items_forced(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_send(cid, b'ham')
        interpreters.channel_close(cid, send=True, recv=True, force=True)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
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
            _interpreters.channel_close({cid}, force=True)
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
        interpreters.channel_close(cid, force=True)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)


class ChannelReleaseTests(TestBase):

    # XXX Add more test coverage a la the tests for close().

    """
    - main / interp / other
    - run in: current thread / new thread / other thread / different threads
    - end / opposite
    - force / no force
    - used / not used  (associated / not associated)
    - empty / emptied / never emptied / partly emptied
    - closed / not closed
    - released / not released
    - creator (interp) / other
    - associated interpreter not running
    - associated interpreter destroyed
    """

    """
    use
    pre-release
    release
    after
    check
    """

    """
    release in:         main, interp1
    creator:            same, other (incl. interp2)

    use:                None,send,recv,send/recv in None,same,other(incl. interp2),same+other(incl. interp2),all
    pre-release:        None,send,recv,both in None,same,other(incl. interp2),same+other(incl. interp2),all
    pre-release forced: None,send,recv,both in None,same,other(incl. interp2),same+other(incl. interp2),all

    release:            same
    release forced:     same

    use after:          None,send,recv,send/recv in None,same,other(incl. interp2),same+other(incl. interp2),all
    release after:      None,send,recv,send/recv in None,same,other(incl. interp2),same+other(incl. interp2),all
    check released:     send/recv for same/other(incl. interp2)
    check closed:       send/recv for same/other(incl. interp2)
    """

    def test_single_user(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_recv(cid)
        interpreters.channel_release(cid, send=True, recv=True)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_multiple_users(self):
        cid = interpreters.channel_create()
        id1 = interpreters.create()
        id2 = interpreters.create()
        interpreters.run_string(id1, dedent(f"""
            import _xxsubinterpreters as _interpreters
            _interpreters.channel_send({cid}, b'spam')
            """))
        out = _run_output(id2, dedent(f"""
            import _xxsubinterpreters as _interpreters
            obj = _interpreters.channel_recv({cid})
            _interpreters.channel_release({cid})
            print(repr(obj))
            """))
        interpreters.run_string(id1, dedent(f"""
            _interpreters.channel_release({cid})
            """))

        self.assertEqual(out.strip(), "b'spam'")

    def test_no_kwargs(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_recv(cid)
        interpreters.channel_release(cid)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_multiple_times(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_recv(cid)
        interpreters.channel_release(cid, send=True, recv=True)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_release(cid, send=True, recv=True)

    def test_with_unused_items(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_send(cid, b'ham')
        interpreters.channel_release(cid, send=True, recv=True)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_never_used(self):
        cid = interpreters.channel_create()
        interpreters.channel_release(cid)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'spam')
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_by_unassociated_interp(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interp = interpreters.create()
        interpreters.run_string(interp, dedent(f"""
            import _xxsubinterpreters as _interpreters
            _interpreters.channel_release({cid})
            """))
        obj = interpreters.channel_recv(cid)
        interpreters.channel_release(cid)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
        self.assertEqual(obj, b'spam')

    def test_close_if_unassociated(self):
        # XXX Something's not right with this test...
        cid = interpreters.channel_create()
        interp = interpreters.create()
        interpreters.run_string(interp, dedent(f"""
            import _xxsubinterpreters as _interpreters
            obj = _interpreters.channel_send({cid}, b'spam')
            _interpreters.channel_release({cid})
            """))

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)

    def test_partially(self):
        # XXX Is partial close too weird/confusing?
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, None)
        interpreters.channel_recv(cid)
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_release(cid, send=True)
        obj = interpreters.channel_recv(cid)

        self.assertEqual(obj, b'spam')

    def test_used_multiple_times_by_single_user(self):
        cid = interpreters.channel_create()
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_send(cid, b'spam')
        interpreters.channel_recv(cid)
        interpreters.channel_release(cid, send=True, recv=True)

        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_send(cid, b'eggs')
        with self.assertRaises(interpreters.ChannelClosedError):
            interpreters.channel_recv(cid)


class ChannelCloseFixture(namedtuple('ChannelCloseFixture',
                                     'end interp other extra creator')):

    # Set this to True to avoid creating interpreters, e.g. when
    # scanning through test permutations without running them.
    QUICK = False

    def __new__(cls, end, interp, other, extra, creator):
        assert end in ('send', 'recv')
        if cls.QUICK:
            known = {}
        else:
            interp = Interpreter.from_raw(interp)
            other = Interpreter.from_raw(other)
            extra = Interpreter.from_raw(extra)
            known = {
                interp.name: interp,
                other.name: other,
                extra.name: extra,
                }
        if not creator:
            creator = 'same'
        self = super().__new__(cls, end, interp, other, extra, creator)
        self._prepped = set()
        self._state = ChannelState()
        self._known = known
        return self

    @property
    def state(self):
        return self._state

    @property
    def cid(self):
        try:
            return self._cid
        except AttributeError:
            creator = self._get_interpreter(self.creator)
            self._cid = self._new_channel(creator)
            return self._cid

    def get_interpreter(self, interp):
        interp = self._get_interpreter(interp)
        self._prep_interpreter(interp)
        return interp

    def expect_closed_error(self, end=None):
        if end is None:
            end = self.end
        if end == 'recv' and self.state.closed == 'send':
            return False
        return bool(self.state.closed)

    def prep_interpreter(self, interp):
        self._prep_interpreter(interp)

    def record_action(self, action, result):
        self._state = result

    def clean_up(self):
        clean_up_interpreters()
        clean_up_channels()

    # internal methods

    def _new_channel(self, creator):
        if creator.name == 'main':
            return interpreters.channel_create()
        else:
            ch = interpreters.channel_create()
            run_interp(creator.id, f"""
                import _xxsubinterpreters
                cid = _xxsubinterpreters.channel_create()
                # We purposefully send back an int to avoid tying the
                # channel to the other interpreter.
                _xxsubinterpreters.channel_send({ch}, int(cid))
                del _xxsubinterpreters
                """)
            self._cid = interpreters.channel_recv(ch)
        return self._cid

    def _get_interpreter(self, interp):
        if interp in ('same', 'interp'):
            return self.interp
        elif interp == 'other':
            return self.other
        elif interp == 'extra':
            return self.extra
        else:
            name = interp
            try:
                interp = self._known[name]
            except KeyError:
                interp = self._known[name] = Interpreter(name)
            return interp

    def _prep_interpreter(self, interp):
        if interp.id in self._prepped:
            return
        self._prepped.add(interp.id)
        if interp.name == 'main':
            return
        run_interp(interp.id, f"""
            import _xxsubinterpreters as interpreters
            import test.test__xxsubinterpreters as helpers
            ChannelState = helpers.ChannelState
            try:
                cid
            except NameError:
                cid = interpreters._channel_id({self.cid})
            """)


@unittest.skip('these tests take several hours to run')
class ExhaustiveChannelTests(TestBase):

    """
    - main / interp / other
    - run in: current thread / new thread / other thread / different threads
    - end / opposite
    - force / no force
    - used / not used  (associated / not associated)
    - empty / emptied / never emptied / partly emptied
    - closed / not closed
    - released / not released
    - creator (interp) / other
    - associated interpreter not running
    - associated interpreter destroyed

    - close after unbound
    """

    """
    use
    pre-close
    close
    after
    check
    """

    """
    close in:         main, interp1
    creator:          same, other, extra

    use:              None,send,recv,send/recv in None,same,other,same+other,all
    pre-close:        None,send,recv in None,same,other,same+other,all
    pre-close forced: None,send,recv in None,same,other,same+other,all

    close:            same
    close forced:     same

    use after:        None,send,recv,send/recv in None,same,other,extra,same+other,all
    close after:      None,send,recv,send/recv in None,same,other,extra,same+other,all
    check closed:     send/recv for same/other(incl. interp2)
    """

    def iter_action_sets(self):
        # - used / not used  (associated / not associated)
        # - empty / emptied / never emptied / partly emptied
        # - closed / not closed
        # - released / not released

        # never used
        yield []

        # only pre-closed (and possible used after)
        for closeactions in self._iter_close_action_sets('same', 'other'):
            yield closeactions
            for postactions in self._iter_post_close_action_sets():
                yield closeactions + postactions
        for closeactions in self._iter_close_action_sets('other', 'extra'):
            yield closeactions
            for postactions in self._iter_post_close_action_sets():
                yield closeactions + postactions

        # used
        for useactions in self._iter_use_action_sets('same', 'other'):
            yield useactions
            for closeactions in self._iter_close_action_sets('same', 'other'):
                actions = useactions + closeactions
                yield actions
                for postactions in self._iter_post_close_action_sets():
                    yield actions + postactions
            for closeactions in self._iter_close_action_sets('other', 'extra'):
                actions = useactions + closeactions
                yield actions
                for postactions in self._iter_post_close_action_sets():
                    yield actions + postactions
        for useactions in self._iter_use_action_sets('other', 'extra'):
            yield useactions
            for closeactions in self._iter_close_action_sets('same', 'other'):
                actions = useactions + closeactions
                yield actions
                for postactions in self._iter_post_close_action_sets():
                    yield actions + postactions
            for closeactions in self._iter_close_action_sets('other', 'extra'):
                actions = useactions + closeactions
                yield actions
                for postactions in self._iter_post_close_action_sets():
                    yield actions + postactions

    def _iter_use_action_sets(self, interp1, interp2):
        interps = (interp1, interp2)

        # only recv end used
        yield [
            ChannelAction('use', 'recv', interp1),
            ]
        yield [
            ChannelAction('use', 'recv', interp2),
            ]
        yield [
            ChannelAction('use', 'recv', interp1),
            ChannelAction('use', 'recv', interp2),
            ]

        # never emptied
        yield [
            ChannelAction('use', 'send', interp1),
            ]
        yield [
            ChannelAction('use', 'send', interp2),
            ]
        yield [
            ChannelAction('use', 'send', interp1),
            ChannelAction('use', 'send', interp2),
            ]

        # partially emptied
        for interp1 in interps:
            for interp2 in interps:
                for interp3 in interps:
                    yield [
                        ChannelAction('use', 'send', interp1),
                        ChannelAction('use', 'send', interp2),
                        ChannelAction('use', 'recv', interp3),
                        ]

        # fully emptied
        for interp1 in interps:
            for interp2 in interps:
                for interp3 in interps:
                    for interp4 in interps:
                        yield [
                            ChannelAction('use', 'send', interp1),
                            ChannelAction('use', 'send', interp2),
                            ChannelAction('use', 'recv', interp3),
                            ChannelAction('use', 'recv', interp4),
                            ]

    def _iter_close_action_sets(self, interp1, interp2):
        ends = ('recv', 'send')
        interps = (interp1, interp2)
        for force in (True, False):
            op = 'force-close' if force else 'close'
            for interp in interps:
                for end in ends:
                    yield [
                        ChannelAction(op, end, interp),
                        ]
        for recvop in ('close', 'force-close'):
            for sendop in ('close', 'force-close'):
                for recv in interps:
                    for send in interps:
                        yield [
                            ChannelAction(recvop, 'recv', recv),
                            ChannelAction(sendop, 'send', send),
                            ]

    def _iter_post_close_action_sets(self):
        for interp in ('same', 'extra', 'other'):
            yield [
                ChannelAction('use', 'recv', interp),
                ]
            yield [
                ChannelAction('use', 'send', interp),
                ]

    def run_actions(self, fix, actions):
        for action in actions:
            self.run_action(fix, action)

    def run_action(self, fix, action, *, hideclosed=True):
        end = action.resolve_end(fix.end)
        interp = action.resolve_interp(fix.interp, fix.other, fix.extra)
        fix.prep_interpreter(interp)
        if interp.name == 'main':
            result = run_action(
                fix.cid,
                action.action,
                end,
                fix.state,
                hideclosed=hideclosed,
                )
            fix.record_action(action, result)
        else:
            _cid = interpreters.channel_create()
            run_interp(interp.id, f"""
                result = helpers.run_action(
                    {fix.cid},
                    {repr(action.action)},
                    {repr(end)},
                    {repr(fix.state)},
                    hideclosed={hideclosed},
                    )
                interpreters.channel_send({_cid}, result.pending.to_bytes(1, 'little'))
                interpreters.channel_send({_cid}, b'X' if result.closed else b'')
                """)
            result = ChannelState(
                pending=int.from_bytes(interpreters.channel_recv(_cid), 'little'),
                closed=bool(interpreters.channel_recv(_cid)),
                )
            fix.record_action(action, result)

    def iter_fixtures(self):
        # XXX threads?
        interpreters = [
            ('main', 'interp', 'extra'),
            ('interp', 'main', 'extra'),
            ('interp1', 'interp2', 'extra'),
            ('interp1', 'interp2', 'main'),
        ]
        for interp, other, extra in interpreters:
            for creator in ('same', 'other', 'creator'):
                for end in ('send', 'recv'):
                    yield ChannelCloseFixture(end, interp, other, extra, creator)

    def _close(self, fix, *, force):
        op = 'force-close' if force else 'close'
        close = ChannelAction(op, fix.end, 'same')
        if not fix.expect_closed_error():
            self.run_action(fix, close, hideclosed=False)
        else:
            with self.assertRaises(interpreters.ChannelClosedError):
                self.run_action(fix, close, hideclosed=False)

    def _assert_closed_in_interp(self, fix, interp=None):
        if interp is None or interp.name == 'main':
            with self.assertRaises(interpreters.ChannelClosedError):
                interpreters.channel_recv(fix.cid)
            with self.assertRaises(interpreters.ChannelClosedError):
                interpreters.channel_send(fix.cid, b'spam')
            with self.assertRaises(interpreters.ChannelClosedError):
                interpreters.channel_close(fix.cid)
            with self.assertRaises(interpreters.ChannelClosedError):
                interpreters.channel_close(fix.cid, force=True)
        else:
            run_interp(interp.id, f"""
                with helpers.expect_channel_closed():
                    interpreters.channel_recv(cid)
                """)
            run_interp(interp.id, f"""
                with helpers.expect_channel_closed():
                    interpreters.channel_send(cid, b'spam')
                """)
            run_interp(interp.id, f"""
                with helpers.expect_channel_closed():
                    interpreters.channel_close(cid)
                """)
            run_interp(interp.id, f"""
                with helpers.expect_channel_closed():
                    interpreters.channel_close(cid, force=True)
                """)

    def _assert_closed(self, fix):
        self.assertTrue(fix.state.closed)

        for _ in range(fix.state.pending):
            interpreters.channel_recv(fix.cid)
        self._assert_closed_in_interp(fix)

        for interp in ('same', 'other'):
            interp = fix.get_interpreter(interp)
            if interp.name == 'main':
                continue
            self._assert_closed_in_interp(fix, interp)

        interp = fix.get_interpreter('fresh')
        self._assert_closed_in_interp(fix, interp)

    def _iter_close_tests(self, verbose=False):
        i = 0
        for actions in self.iter_action_sets():
            print()
            for fix in self.iter_fixtures():
                i += 1
                if i > 1000:
                    return
                if verbose:
                    if (i - 1) % 6 == 0:
                        print()
                    print(i, fix, '({} actions)'.format(len(actions)))
                else:
                    if (i - 1) % 6 == 0:
                        print(' ', end='')
                    print('.', end=''); sys.stdout.flush()
                yield i, fix, actions
            if verbose:
                print('---')
        print()

    # This is useful for scanning through the possible tests.
    def _skim_close_tests(self):
        ChannelCloseFixture.QUICK = True
        for i, fix, actions in self._iter_close_tests():
            pass

    def test_close(self):
        for i, fix, actions in self._iter_close_tests():
            with self.subTest('{} {}  {}'.format(i, fix, actions)):
                fix.prep_interpreter(fix.interp)
                self.run_actions(fix, actions)

                self._close(fix, force=False)

                self._assert_closed(fix)
            # XXX Things slow down if we have too many interpreters.
            fix.clean_up()

    def test_force_close(self):
        for i, fix, actions in self._iter_close_tests():
            with self.subTest('{} {}  {}'.format(i, fix, actions)):
                fix.prep_interpreter(fix.interp)
                self.run_actions(fix, actions)

                self._close(fix, force=True)

                self._assert_closed(fix)
            # XXX Things slow down if we have too many interpreters.
            fix.clean_up()


if __name__ == '__main__':
    unittest.main()
