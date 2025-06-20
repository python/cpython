import contextlib
import os
import pickle
import sys
from textwrap import dedent
import threading
import types
import unittest

from test import support
from test.support import os_helper
from test.support import script_helper
from test.support import import_helper
# Raise SkipTest if subinterpreters not supported.
_interpreters = import_helper.import_module('_interpreters')
from concurrent import interpreters
from test.support import Py_GIL_DISABLED
from test.support import force_not_colorized
import test._crossinterp_definitions as defs
from concurrent.interpreters import (
    InterpreterError, InterpreterNotFoundError, ExecutionFailed,
)
from .utils import (
    _captured_script, _run_output, _running, TestBase,
    requires_test_modules, _testinternalcapi,
)


WHENCE_STR_UNKNOWN = 'unknown'
WHENCE_STR_RUNTIME = 'runtime init'
WHENCE_STR_LEGACY_CAPI = 'legacy C-API'
WHENCE_STR_CAPI = 'C-API'
WHENCE_STR_XI = 'cross-interpreter C-API'
WHENCE_STR_STDLIB = '_interpreters module'


def is_pickleable(obj):
    try:
        pickle.dumps(obj)
    except Exception:
        return False
    return True


@contextlib.contextmanager
def defined_in___main__(name, script, *, remove=False):
    import __main__ as mainmod
    mainns = vars(mainmod)
    assert name not in mainns
    exec(script, mainns, mainns)
    if remove:
        yield mainns.pop(name)
    else:
        try:
            yield mainns[name]
        finally:
            mainns.pop(name, None)


def build_excinfo(exctype, msg=None, formatted=None, errdisplay=None):
    if isinstance(exctype, type):
        assert issubclass(exctype, BaseException), exctype
        exctype = types.SimpleNamespace(
            __name__=exctype.__name__,
            __qualname__=exctype.__qualname__,
            __module__=exctype.__module__,
        )
    elif isinstance(exctype, str):
        module, _, name = exctype.rpartition(exctype)
        if not module and name in __builtins__:
            module = 'builtins'
        exctype = types.SimpleNamespace(
            __name__=name,
            __qualname__=exctype,
            __module__=module or None,
        )
    else:
        assert isinstance(exctype, types.SimpleNamespace)
    assert msg is None or isinstance(msg, str), msg
    assert formatted  is None or isinstance(formatted, str), formatted
    assert errdisplay is None or isinstance(errdisplay, str), errdisplay
    return types.SimpleNamespace(
        type=exctype,
        msg=msg,
        formatted=formatted,
        errdisplay=errdisplay,
    )


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

        # GH-126221: Passing an invalid Unicode character used to cause a SystemError
        self.assertRaises(UnicodeEncodeError, _interpreters.create, '\udc80')

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
            from concurrent import interpreters
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
            from concurrent import interpreters
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
                from concurrent import interpreters
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
                from concurrent import interpreters
                cur = interpreters.get_current()
                print(id(cur))
                """))
            id1 = int(out)
            id2 = id(interp)
            self.assertNotEqual(id1, id2)

    @requires_test_modules
    def test_created_with_capi(self):
        expected = _testinternalcapi.next_interpreter_id()
        text = self.run_temp_from_capi(f"""
            import {interpreters.__name__} as interpreters
            interp = interpreters.get_current()
            print((interp.id, interp.whence))
            """)
        interpid, whence = eval(text)
        self.assertEqual(interpid, expected)
        self.assertEqual(whence, WHENCE_STR_CAPI)


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

    def test_created_with_capi(self):
        mainid, *_ = _interpreters.get_main()
        interpid1 = _interpreters.create()
        interpid2 = _interpreters.create()
        interpid3 = _interpreters.create()
        interpid4 = interpid3 + 1
        interpid5 = interpid4 + 1
        expected = [
            (mainid, WHENCE_STR_RUNTIME),
            (interpid1, WHENCE_STR_STDLIB),
            (interpid2, WHENCE_STR_STDLIB),
            (interpid3, WHENCE_STR_STDLIB),
            (interpid4, WHENCE_STR_CAPI),
            (interpid5, WHENCE_STR_STDLIB),
        ]
        expected2 = expected[:-2]
        text = self.run_temp_from_capi(f"""
            import {interpreters.__name__} as interpreters
            interp = interpreters.create()
            print(
                [(i.id, i.whence) for i in interpreters.list_all()])
            """)
        res = eval(text)
        res2 = [(i.id, i.whence) for i in interpreters.list_all()]
        self.assertEqual(res, expected)
        self.assertEqual(res2, expected2)


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

    def test_whence(self):
        main = interpreters.get_main()
        interp = interpreters.create()

        with self.subTest('main'):
            self.assertEqual(main.whence, WHENCE_STR_RUNTIME)

        with self.subTest('from _interpreters'):
            self.assertEqual(interp.whence, WHENCE_STR_STDLIB)

        with self.subTest('from C-API'):
            text = self.run_temp_from_capi(f"""
                import {interpreters.__name__} as interpreters
                interp = interpreters.get_current()
                print(repr(interp.whence))
                """)
            whence = eval(text)
            self.assertEqual(whence, WHENCE_STR_CAPI)

        with self.subTest('readonly'):
            for value in [
                None,
                WHENCE_STR_UNKNOWN,
                WHENCE_STR_RUNTIME,
                WHENCE_STR_STDLIB,
                WHENCE_STR_CAPI,
            ]:
                with self.assertRaises(AttributeError):
                    interp.whence = value
                with self.assertRaises(AttributeError):
                    main.whence = value

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

    def test_pickle(self):
        interp = interpreters.create()
        data = pickle.dumps(interp)
        unpickled = pickle.loads(data)
        self.assertEqual(unpickled, interp)


class TestInterpreterIsRunning(TestBase):

    def test_main(self):
        main = interpreters.get_main()
        self.assertTrue(main.is_running())

    # XXX Is this still true?
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
        interp.exec(f"""if True:
            import os
            os.write({w}, b'x')
            """)
        self.assertFalse(interp.is_running())
        self.assertEqual(os.read(r, 1), b'x')

    def test_from_subinterpreter(self):
        interp = interpreters.create()
        out = _run_output(interp, dedent(f"""
            import _interpreters
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
        interp.exec(f"""if True:
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
        interp.exec('t.join()')
        self.assertEqual(os.read(r_interp, 1), FINISHED)

    def test_created_with_capi(self):
        script = dedent(f"""
            import {interpreters.__name__} as interpreters
            interp = interpreters.get_current()
            print(interp.is_running())
            """)
        def parse_results(text):
            self.assertNotEqual(text, "")
            try:
                return eval(text)
            except Exception:
                raise Exception(repr(text))

        with self.subTest('running __main__ (from self)'):
            with self.interpreter_from_capi() as interpid:
                text = self.run_from_capi(interpid, script, main=True)
            running = parse_results(text)
            self.assertTrue(running)

        with self.subTest('running, but not __main__ (from self)'):
            text = self.run_temp_from_capi(script)
            running = parse_results(text)
            self.assertFalse(running)

        with self.subTest('running __main__ (from other)'):
            with self.interpreter_obj_from_capi() as (interp, interpid):
                before = interp.is_running()
                with self.running_from_capi(interpid, main=True):
                    during = interp.is_running()
                after = interp.is_running()
            self.assertFalse(before)
            self.assertTrue(during)
            self.assertFalse(after)

        with self.subTest('running, but not __main__ (from other)'):
            with self.interpreter_obj_from_capi() as (interp, interpid):
                before = interp.is_running()
                with self.running_from_capi(interpid, main=False):
                    during = interp.is_running()
                after = interp.is_running()
            self.assertFalse(before)
            self.assertFalse(during)
            self.assertFalse(after)

        with self.subTest('not running (from other)'):
            with self.interpreter_obj_from_capi() as (interp, _):
                running = interp.is_running()
            self.assertFalse(running)


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
        with self.assertRaises(InterpreterError):
            main.close()

        def f():
            with self.assertRaises(InterpreterError):
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
            from concurrent import interpreters
            interp = interpreters.Interpreter({interp.id})
            try:
                interp.close()
            except interpreters.InterpreterError:
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
        interp1.exec(dedent(f"""
            from concurrent import interpreters
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

    # XXX Is this still true?
    @unittest.skip('Fails on FreeBSD')
    def test_still_running(self):
        main, = interpreters.list_all()
        interp = interpreters.create()
        with _running(interp):
            with self.assertRaises(InterpreterError):
                interp.close()
            self.assertTrue(interp.is_running())

    def test_subthreads_still_running(self):
        r_interp, w_interp = self.pipe()
        r_thread, w_thread = self.pipe()

        FINISHED = b'F'

        interp = interpreters.create()
        interp.exec(f"""if True:
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

    def test_created_with_capi(self):
        script = dedent(f"""
            import {interpreters.__name__} as interpreters
            interp = interpreters.get_current()
            interp.close()
            """)

        with self.subTest('running __main__ (from self)'):
            with self.interpreter_from_capi() as interpid:
                with self.assertRaisesRegex(ExecutionFailed,
                                            'InterpreterError.*unrecognized'):
                    self.run_from_capi(interpid, script, main=True)

        with self.subTest('running, but not __main__ (from self)'):
            with self.assertRaisesRegex(ExecutionFailed,
                                        'InterpreterError.*unrecognized'):
                self.run_temp_from_capi(script)

        with self.subTest('running __main__ (from other)'):
            with self.interpreter_obj_from_capi() as (interp, interpid):
                with self.running_from_capi(interpid, main=True):
                    with self.assertRaisesRegex(InterpreterError, 'unrecognized'):
                        interp.close()
                    # Make sure it wssn't closed.
                    self.assertTrue(
                        self.interp_exists(interpid))

        # The rest would be skipped until we deal with running threads when
        # interp.close() is called.  However, the "whence" restrictions
        # trigger first.

        with self.subTest('running, but not __main__ (from other)'):
            with self.interpreter_obj_from_capi() as (interp, interpid):
                with self.running_from_capi(interpid, main=False):
                    with self.assertRaisesRegex(InterpreterError, 'unrecognized'):
                        interp.close()
                    # Make sure it wssn't closed.
                    self.assertTrue(
                        self.interp_exists(interpid))

        with self.subTest('not running (from other)'):
            with self.interpreter_obj_from_capi() as (interp, interpid):
                with self.assertRaisesRegex(InterpreterError, 'unrecognized'):
                    interp.close()
                self.assertTrue(
                    self.interp_exists(interpid))


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
        with self.assertRaises(interpreters.NotShareableError):
            interp.prepare_main(spam={'spam': 'eggs', 'foo': 'bar'})

        # Make sure neither was actually bound.
        with self.assertRaises(ExecutionFailed):
            interp.exec('print(foo)')
        with self.assertRaises(ExecutionFailed):
            interp.exec('print(spam)')

    def test_running(self):
        interp = interpreters.create()
        interp.prepare_main({'spam': True})
        with self.running(interp):
            with self.assertRaisesRegex(InterpreterError, 'running'):
                interp.prepare_main({'spam': False})
        interp.exec('assert spam is True')

    @requires_test_modules
    def test_created_with_capi(self):
        with self.interpreter_obj_from_capi() as (interp, interpid):
            with self.assertRaisesRegex(InterpreterError, 'unrecognized'):
                interp.prepare_main({'spam': True})
            with self.assertRaisesRegex(ExecutionFailed, 'NameError'):
                self.run_from_capi(interpid, 'assert spam is True')


class TestInterpreterExec(TestBase):

    def test_success(self):
        interp = interpreters.create()
        script, results = _captured_script('print("it worked!", end="")')
        with results:
            interp.exec(script)
        results = results.final()
        results.raise_if_failed()
        out = results.stdout

        self.assertEqual(out, 'it worked!')

    def test_failure(self):
        interp = interpreters.create()
        with self.assertRaises(ExecutionFailed):
            interp.exec('raise Exception')

    @force_not_colorized
    def test_display_preserved_exception(self):
        tempdir = self.temp_dir()
        modfile = self.make_module('spam', tempdir, text="""
            def ham():
                raise RuntimeError('uh-oh!')

            def eggs():
                ham()
            """)
        scriptfile = self.make_script('script.py', tempdir, text="""
            from concurrent import interpreters

            def script():
                import spam
                spam.eggs()

            interp = interpreters.create()
            interp.exec(script)
            """)

        stdout, stderr = self.assert_python_failure(scriptfile)
        self.maxDiff = None
        interpmod_line, = (l for l in stderr.splitlines() if ' exec' in l)
        #      File "{interpreters.__file__}", line 179, in exec
        self.assertEqual(stderr, dedent(f"""\
            Traceback (most recent call last):
              File "{scriptfile}", line 9, in <module>
                interp.exec(script)
                ~~~~~~~~~~~^^^^^^^^
              {interpmod_line.strip()}
                raise ExecutionFailed(excinfo)
            concurrent.interpreters.ExecutionFailed: RuntimeError: uh-oh!

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
        script, results = _captured_script('print("it worked!", end="")')
        with results:
            def f():
                interp.exec(script)

            t = threading.Thread(target=f)
            t.start()
            t.join()
        results = results.final()
        results.raise_if_failed()
        out = results.stdout

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
            interp.exec(script)

            file.seek(0)
            content = file.read()
            self.assertEqual(content, expected)

    # XXX Is this still true?
    @unittest.skip('Fails on FreeBSD')
    def test_already_running(self):
        interp = interpreters.create()
        with _running(interp):
            with self.assertRaises(RuntimeError):
                interp.exec('print("spam")')

    def test_bad_script(self):
        interp = interpreters.create()
        with self.assertRaises(TypeError):
            interp.exec(10)

    def test_bytes_for_script(self):
        r, w = self.pipe()
        RAN = b'R'
        DONE = b'D'
        interp = interpreters.create()
        interp.exec(f"""if True:
            import os
            os.write({w}, {RAN!r})
            """)
        os.write(w, DONE)
        self.assertEqual(os.read(r, 1), RAN)

    def test_with_background_threads_still_running(self):
        r_interp, w_interp = self.pipe()
        r_thread, w_thread = self.pipe()

        RAN = b'R'
        DONE = b'D'
        FINISHED = b'F'

        interp = interpreters.create()
        interp.exec(f"""if True:
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
        interp.exec(f"""if True:
            os.write({w_interp}, {RAN!r})
            """)

        os.write(w_thread, DONE)
        interp.exec('t.join()')
        self.assertEqual(os.read(r_interp, 1), RAN)
        self.assertEqual(os.read(r_interp, 1), RAN)
        self.assertEqual(os.read(r_interp, 1), FINISHED)

    def test_created_with_capi(self):
        with self.interpreter_obj_from_capi() as (interp, _):
            with self.assertRaisesRegex(InterpreterError, 'unrecognized'):
                interp.exec('raise Exception("it worked!")')

    def test_list_comprehension(self):
        # gh-135450: List comprehensions caused an assertion failure
        # in _PyCode_CheckNoExternalState()
        import string
        r_interp, w_interp = self.pipe()

        interp = interpreters.create()
        interp.exec(f"""if True:
            import os
            comp = [str(i) for i in range(10)]
            os.write({w_interp}, ''.join(comp).encode())
        """)
        self.assertEqual(os.read(r_interp, 10).decode(), string.digits)
        interp.close()


    # test__interpreters covers the remaining
    # Interpreter.exec() behavior.


call_func_noop = defs.spam_minimal
call_func_ident = defs.spam_returns_arg
call_func_failure = defs.spam_raises


def call_func_return_shareable():
    return (1, None)


def call_func_return_stateless_func():
    return (lambda x: x)


def call_func_return_pickleable():
    return [1, 2, 3]


def call_func_return_unpickleable():
    x = 42
    return (lambda: x)


def get_call_func_closure(value):
    def call_func_closure():
        return value
    return call_func_closure


def call_func_exec_wrapper(script, ns):
    res = exec(script, ns, ns)
    return res, ns, id(ns)


class Spam:

    @staticmethod
    def noop():
        pass

    @classmethod
    def from_values(cls, *values):
        return cls(values)

    def __init__(self, value):
        self.value = value

    def __call__(self, *args, **kwargs):
        return (self.value, args, kwargs)

    def __eq__(self, other):
        if not isinstance(other, Spam):
            return NotImplemented
        return self.value == other.value

    def run(self, *args, **kwargs):
        return (self.value, args, kwargs)


def call_func_complex(op, /, value=None, *args, exc=None, **kwargs):
    if exc is not None:
        raise exc
    if op == '':
        raise ValueError('missing op')
    elif op == 'ident':
        if args or kwargs:
            raise Exception((args, kwargs))
        return value
    elif op == 'full-ident':
        return (value, args, kwargs)
    elif op == 'globals':
        if value is not None or args or kwargs:
            raise Exception((value, args, kwargs))
        return __name__
    elif op == 'interpid':
        if value is not None or args or kwargs:
            raise Exception((value, args, kwargs))
        return interpreters.get_current().id
    elif op == 'closure':
        if args or kwargs:
            raise Exception((args, kwargs))
        return get_call_func_closure(value)
    elif op == 'custom':
        if args or kwargs:
            raise Exception((args, kwargs))
        return Spam(value)
    elif op == 'custom-inner':
        if args or kwargs:
            raise Exception((args, kwargs))
        class Eggs(Spam):
            pass
        return Eggs(value)
    elif not isinstance(op, str):
        raise TypeError(op)
    else:
        raise NotImplementedError(op)


class TestInterpreterCall(TestBase):

    # signature
    #  - blank
    #  - args
    #  - kwargs
    #  - args, kwargs
    # return
    #  - nothing (None)
    #  - simple
    #  - closure
    #  - custom
    # ops:
    #  - do nothing
    #  - fail
    #  - echo
    #  - do complex, relative to interpreter
    # scope
    #  - global func
    #  - local closure
    #  - returned closure
    #  - callable type instance
    #  - type
    #  - classmethod
    #  - staticmethod
    #  - instance method
    # exception
    #  - builtin
    #  - custom
    #  - preserves info (e.g. SyntaxError)
    #  - matching error display

    @contextlib.contextmanager
    def assert_fails(self, expected):
        with self.assertRaises(ExecutionFailed) as cm:
            yield cm
        uncaught = cm.exception.excinfo
        self.assertEqual(uncaught.type.__name__, expected.__name__)

    def assert_fails_not_shareable(self):
        return self.assert_fails(interpreters.NotShareableError)

    def assert_code_equal(self, code1, code2):
        if code1 == code2:
            return
        self.assertEqual(code1.co_name, code2.co_name)
        self.assertEqual(code1.co_flags, code2.co_flags)
        self.assertEqual(code1.co_consts, code2.co_consts)
        self.assertEqual(code1.co_varnames, code2.co_varnames)
        self.assertEqual(code1.co_cellvars, code2.co_cellvars)
        self.assertEqual(code1.co_freevars, code2.co_freevars)
        self.assertEqual(code1.co_names, code2.co_names)
        self.assertEqual(
            _testinternalcapi.get_code_var_counts(code1),
            _testinternalcapi.get_code_var_counts(code2),
        )
        self.assertEqual(code1.co_code, code2.co_code)

    def assert_funcs_equal(self, func1, func2):
        if func1 == func2:
            return
        self.assertIs(type(func1), type(func2))
        self.assertEqual(func1.__name__, func2.__name__)
        self.assertEqual(func1.__defaults__, func2.__defaults__)
        self.assertEqual(func1.__kwdefaults__, func2.__kwdefaults__)
        self.assertEqual(func1.__closure__, func2.__closure__)
        self.assert_code_equal(func1.__code__, func2.__code__)
        self.assertEqual(
            _testinternalcapi.get_code_var_counts(func1),
            _testinternalcapi.get_code_var_counts(func2),
        )

    def assert_exceptions_equal(self, exc1, exc2):
        assert isinstance(exc1, Exception)
        assert isinstance(exc2, Exception)
        if exc1 == exc2:
            return
        self.assertIs(type(exc1), type(exc2))
        self.assertEqual(exc1.args, exc2.args)

    def test_stateless_funcs(self):
        interp = interpreters.create()

        func = call_func_noop
        with self.subTest('no args, no return'):
            res = interp.call(func)
            self.assertIsNone(res)

        func = call_func_return_shareable
        with self.subTest('no args, returns shareable'):
            res = interp.call(func)
            self.assertEqual(res, (1, None))

        func = call_func_return_stateless_func
        expected = (lambda x: x)
        with self.subTest('no args, returns stateless func'):
            res = interp.call(func)
            self.assert_funcs_equal(res, expected)

        func = call_func_return_pickleable
        with self.subTest('no args, returns pickleable'):
            res = interp.call(func)
            self.assertEqual(res, [1, 2, 3])

        func = call_func_return_unpickleable
        with self.subTest('no args, returns unpickleable'):
            with self.assertRaises(interpreters.NotShareableError):
                interp.call(func)

    def test_stateless_func_returns_arg(self):
        interp = interpreters.create()

        for arg in [
            None,
            10,
            'spam!',
            b'spam!',
            (1, 2, 'spam!'),
            memoryview(b'spam!'),
        ]:
            with self.subTest(f'shareable {arg!r}'):
                assert _interpreters.is_shareable(arg)
                res = interp.call(defs.spam_returns_arg, arg)
                self.assertEqual(res, arg)

        for arg in defs.STATELESS_FUNCTIONS:
            with self.subTest(f'stateless func {arg!r}'):
                res = interp.call(defs.spam_returns_arg, arg)
                self.assert_funcs_equal(res, arg)

        for arg in defs.TOP_FUNCTIONS:
            if arg in defs.STATELESS_FUNCTIONS:
                continue
            with self.subTest(f'stateful func {arg!r}'):
                res = interp.call(defs.spam_returns_arg, arg)
                self.assert_funcs_equal(res, arg)
                assert is_pickleable(arg)

        for arg in [
            Ellipsis,
            NotImplemented,
            object(),
            2**1000,
            [1, 2, 3],
            {'a': 1, 'b': 2},
            types.SimpleNamespace(x=42),
            # builtin types
            object,
            type,
            Exception,
            ModuleNotFoundError,
            # builtin exceptions
            Exception('uh-oh!'),
            ModuleNotFoundError('mymodule'),
            # builtin fnctions
            len,
            sys.exit,
            # user classes
            *defs.TOP_CLASSES,
            *(c(*a) for c, a in defs.TOP_CLASSES.items()
              if c not in defs.CLASSES_WITHOUT_EQUALITY),
        ]:
            with self.subTest(f'pickleable {arg!r}'):
                res = interp.call(defs.spam_returns_arg, arg)
                if type(arg) is object:
                    self.assertIs(type(res), object)
                elif isinstance(arg, BaseException):
                    self.assert_exceptions_equal(res, arg)
                else:
                    self.assertEqual(res, arg)
                assert is_pickleable(arg)

        for arg in [
            types.MappingProxyType({}),
            *(f for f in defs.NESTED_FUNCTIONS
              if f not in defs.STATELESS_FUNCTIONS),
        ]:
            with self.subTest(f'unpickleable {arg!r}'):
                assert not _interpreters.is_shareable(arg)
                assert not is_pickleable(arg)
                with self.assertRaises(interpreters.NotShareableError):
                    interp.call(defs.spam_returns_arg, arg)

    def test_full_args(self):
        interp = interpreters.create()
        expected = (1, 2, 3, 4, 5, 6, ('?',), {'g': 7, 'h': 8})
        func = defs.spam_full_args
        res = interp.call(func, 1, 2, 3, 4, '?', e=5, f=6, g=7, h=8)
        self.assertEqual(res, expected)

    def test_full_defaults(self):
        # pickleable, but not stateless
        interp = interpreters.create()
        expected = (-1, -2, -3, -4, -5, -6, (), {'g': 8, 'h': 9})
        res = interp.call(defs.spam_full_args_with_defaults, g=8, h=9)
        self.assertEqual(res, expected)

    def test_modified_arg(self):
        interp = interpreters.create()
        script = dedent("""
            a = 7
            b = 2
            c = a ** b
            """)
        ns = {}
        expected = {'a': 7, 'b': 2, 'c': 49}
        res = interp.call(call_func_exec_wrapper, script, ns)
        obj, resns, resid = res
        del resns['__builtins__']
        self.assertIsNone(obj)
        self.assertEqual(ns, {})
        self.assertEqual(resns, expected)
        self.assertNotEqual(resid, id(ns))
        self.assertNotEqual(resid, id(resns))

    def test_func_in___main___valid(self):
        # pickleable, already there'

        with os_helper.temp_dir() as tempdir:
            def new_mod(name, text):
                script_helper.make_script(tempdir, name, dedent(text))

            def run(text):
                name = 'myscript'
                text = dedent(f"""
                import sys
                sys.path.insert(0, {tempdir!r})

                """) + dedent(text)
                filename = script_helper.make_script(tempdir, name, text)
                res = script_helper.assert_python_ok(filename)
                return res.out.decode('utf-8').strip()

            # no module indirection
            with self.subTest('no indirection'):
                text = run(f"""
                    from concurrent import interpreters

                    def spam():
                        # This a global var...
                        return __name__

                    if __name__ == '__main__':
                        interp = interpreters.create()
                        res = interp.call(spam)
                        print(res)
                    """)
                self.assertEqual(text, '<fake __main__>')

            # indirect as func, direct interp
            new_mod('mymod', f"""
                def run(interp, func):
                    return interp.call(func)
                """)
            with self.subTest('indirect as func, direct interp'):
                text = run(f"""
                    from concurrent import interpreters
                    import mymod

                    def spam():
                        # This a global var...
                        return __name__

                    if __name__ == '__main__':
                        interp = interpreters.create()
                        res = mymod.run(interp, spam)
                        print(res)
                    """)
                self.assertEqual(text, '<fake __main__>')

            # indirect as func, indirect interp
            new_mod('mymod', f"""
                from concurrent import interpreters
                def run(func):
                    interp = interpreters.create()
                    return interp.call(func)
                """)
            with self.subTest('indirect as func, indirect interp'):
                text = run(f"""
                    import mymod

                    def spam():
                        # This a global var...
                        return __name__

                    if __name__ == '__main__':
                        res = mymod.run(spam)
                        print(res)
                    """)
                self.assertEqual(text, '<fake __main__>')

    def test_func_in___main___invalid(self):
        interp = interpreters.create()

        funcname = f'{__name__.replace(".", "_")}_spam_okay'
        script = dedent(f"""
            def {funcname}():
                # This a global var...
                return __name__
            """)

        with self.subTest('pickleable, added dynamically'):
            with defined_in___main__(funcname, script) as arg:
                with self.assertRaises(interpreters.NotShareableError):
                    interp.call(defs.spam_returns_arg, arg)

        with self.subTest('lying about __main__'):
            with defined_in___main__(funcname, script, remove=True) as arg:
                with self.assertRaises(interpreters.NotShareableError):
                    interp.call(defs.spam_returns_arg, arg)

    def test_func_in___main___hidden(self):
        # When a top-level function that uses global variables is called
        # through Interpreter.call(), it will be pickled, sent over,
        # and unpickled.  That requires that it be found in the other
        # interpreter's __main__ module.  However, the original script
        # that defined the function is only run in the main interpreter,
        # so pickle.loads() would normally fail.
        #
        # We work around this by running the script in the other
        # interpreter.  However, this is a one-off solution for the sake
        # of unpickling, so we avoid modifying that interpreter's
        # __main__ module by running the script in a hidden module.
        #
        # In this test we verify that the function runs with the hidden
        # module as its __globals__ when called in the other interpreter,
        # and that the interpreter's __main__ module is unaffected.
        text = dedent("""
            eggs = True

            def spam(*, explicit=False):
                if explicit:
                    import __main__
                    ns = __main__.__dict__
                else:
                    # For now we have to have a LOAD_GLOBAL in the
                    # function in order for globals() to actually return
                    # spam.__globals__.  Maybe it doesn't go through pickle?
                    # XXX We will fix this later.
                    spam
                    ns = globals()

                func = ns.get('spam')
                return [
                    id(ns),
                    ns.get('__name__'),
                    ns.get('__file__'),
                    id(func),
                    None if func is None else repr(func),
                    ns.get('eggs'),
                    ns.get('ham'),
                ]

            if __name__ == "__main__":
                from concurrent import interpreters
                interp = interpreters.create()

                ham = True
                print([
                    [
                        spam(explicit=True),
                        spam(),
                    ],
                    [
                        interp.call(spam, explicit=True),
                        interp.call(spam),
                    ],
                ])
           """)
        with os_helper.temp_dir() as tempdir:
            filename = script_helper.make_script(tempdir, 'my-script', text)
            res = script_helper.assert_python_ok(filename)
        stdout = res.out.decode('utf-8').strip()
        local, remote = eval(stdout)

        # In the main interpreter.
        main, unpickled = local
        nsid, _, _, funcid, func, _, _ = main
        self.assertEqual(main, [
            nsid,
            '__main__',
            filename,
            funcid,
            func,
            True,
            True,
        ])
        self.assertIsNot(func, None)
        self.assertRegex(func, '^<function spam at 0x.*>$')
        self.assertEqual(unpickled, main)

        # In the subinterpreter.
        main, unpickled = remote
        nsid1, _, _, funcid1, _, _, _ = main
        self.assertEqual(main, [
            nsid1,
            '__main__',
            None,
            funcid1,
            None,
            None,
            None,
        ])
        nsid2, _, _, funcid2, func, _, _ = unpickled
        self.assertEqual(unpickled, [
            nsid2,
            '<fake __main__>',
            filename,
            funcid2,
            func,
            True,
            None,
        ])
        self.assertIsNot(func, None)
        self.assertRegex(func, '^<function spam at 0x.*>$')
        self.assertNotEqual(nsid2, nsid1)
        self.assertNotEqual(funcid2, funcid1)

    def test_func_in___main___uses_globals(self):
        # See the note in test_func_in___main___hidden about pickle
        # and the __main__ module.
        #
        # Additionally, the solution to that problem must provide
        # for global variables on which a pickled function might rely.
        #
        # To check that, we run a script that has two global functions
        # and a global variable in the __main__ module.  One of the
        # functions sets the global variable and the other returns
        # the value.
        #
        # The script calls those functions multiple times in another
        # interpreter, to verify the following:
        #
        #  * the global variable is properly initialized
        #  * the global variable retains state between calls
        #  * the setter modifies that persistent variable
        #  * the getter uses the variable
        #  * the calls in the other interpreter do not modify
        #    the main interpreter
        #  * those calls don't modify the interpreter's __main__ module
        #  * the functions and variable do not actually show up in the
        #    other interpreter's __main__ module
        text = dedent("""
            count = 0

            def inc(x=1):
                global count
                count += x

            def get_count():
                return count

            if __name__ == "__main__":
                counts = []
                results = [count, counts]

                from concurrent import interpreters
                interp = interpreters.create()

                val = interp.call(get_count)
                counts.append(val)

                interp.call(inc)
                val = interp.call(get_count)
                counts.append(val)

                interp.call(inc, 3)
                val = interp.call(get_count)
                counts.append(val)

                results.append(count)

                modified = {name: interp.call(eval, f'{name!r} in vars()')
                            for name in ('count', 'inc', 'get_count')}
                results.append(modified)

                print(results)
           """)
        with os_helper.temp_dir() as tempdir:
            filename = script_helper.make_script(tempdir, 'my-script', text)
            res = script_helper.assert_python_ok(filename)
        stdout = res.out.decode('utf-8').strip()
        before, counts, after, modified = eval(stdout)
        self.assertEqual(modified, {
            'count': False,
            'inc': False,
            'get_count': False,
        })
        self.assertEqual(before, 0)
        self.assertEqual(after, 0)
        self.assertEqual(counts, [0, 1, 4])

    def test_raises(self):
        interp = interpreters.create()
        with self.assertRaises(ExecutionFailed):
            interp.call(call_func_failure)

        with self.assert_fails(ValueError):
            interp.call(call_func_complex, '???', exc=ValueError('spam'))

    def test_call_valid(self):
        interp = interpreters.create()

        for i, (callable, args, kwargs, expected) in enumerate([
            (call_func_noop, (), {}, None),
            (call_func_ident, ('spamspamspam',), {}, 'spamspamspam'),
            (call_func_return_shareable, (), {}, (1, None)),
            (call_func_return_pickleable, (), {}, [1, 2, 3]),
            (Spam.noop, (), {}, None),
            (Spam.from_values, (), {}, Spam(())),
            (Spam.from_values, (1, 2, 3), {}, Spam((1, 2, 3))),
            (Spam, ('???',), {}, Spam('???')),
            (Spam(101), (), {}, (101, (), {})),
            (Spam(10101).run, (), {}, (10101, (), {})),
            (call_func_complex, ('ident', 'spam'), {}, 'spam'),
            (call_func_complex, ('full-ident', 'spam'), {}, ('spam', (), {})),
            (call_func_complex, ('full-ident', 'spam', 'ham'), {'eggs': '!!!'},
             ('spam', ('ham',), {'eggs': '!!!'})),
            (call_func_complex, ('globals',), {}, __name__),
            (call_func_complex, ('interpid',), {}, interp.id),
            (call_func_complex, ('custom', 'spam!'), {}, Spam('spam!')),
        ]):
            with self.subTest(f'success case #{i+1}'):
                res = interp.call(callable, *args, **kwargs)
                self.assertEqual(res, expected)

    def test_call_invalid(self):
        interp = interpreters.create()

        func = get_call_func_closure
        with self.subTest(func):
            with self.assertRaises(interpreters.NotShareableError):
                interp.call(func, 42)

        func = get_call_func_closure(42)
        with self.subTest(func):
            with self.assertRaises(interpreters.NotShareableError):
                interp.call(func)

        func = call_func_complex
        op = 'closure'
        with self.subTest(f'{func} ({op})'):
            with self.assertRaises(interpreters.NotShareableError):
                interp.call(func, op, value='~~~')

        op = 'custom-inner'
        with self.subTest(f'{func} ({op})'):
            with self.assertRaises(interpreters.NotShareableError):
                interp.call(func, op, 'eggs!')

    def test_callable_requires_frame(self):
        # There are various functions that require a current frame.
        interp = interpreters.create()
        for call, expected in [
            ((eval, '[1, 2, 3]'),
                [1, 2, 3]),
            ((eval, 'sum([1, 2, 3])'),
                6),
            ((exec, '...'),
                None),
        ]:
            with self.subTest(str(call)):
                res = interp.call(*call)
                self.assertEqual(res, expected)

        result_not_pickleable = [
            globals,
            locals,
            vars,
        ]
        for func, expectedtype in {
            globals: dict,
            locals: dict,
            vars: dict,
            dir: list,
        }.items():
            with self.subTest(str(func)):
                if func in result_not_pickleable:
                    with self.assertRaises(interpreters.NotShareableError):
                        interp.call(func)
                else:
                    res = interp.call(func)
                    self.assertIsInstance(res, expectedtype)
                    self.assertIn('__builtins__', res)

    def test_globals_from_builtins(self):
        # The builtins  exec(), eval(), globals(), locals(), vars(),
        # and dir() each runs relative to the target interpreter's
        # __main__ module, when called directly.  However,
        # globals(), locals(), and vars() don't work when called
        # directly so we don't check them.
        from _frozen_importlib import BuiltinImporter
        interp = interpreters.create()

        names = interp.call(dir)
        self.assertEqual(names, [
            '__builtins__',
            '__doc__',
            '__loader__',
            '__name__',
            '__package__',
            '__spec__',
        ])

        values = {name: interp.call(eval, name)
                  for name in names if name != '__builtins__'}
        self.assertEqual(values, {
            '__name__': '__main__',
            '__doc__': None,
            '__spec__': None,  # It wasn't imported, so no module spec?
            '__package__': None,
            '__loader__': BuiltinImporter,
        })
        with self.assertRaises(ExecutionFailed):
            interp.call(eval, 'spam'),

        interp.call(exec, f'assert dir() == {names}')

        # Update the interpreter's __main__.
        interp.prepare_main(spam=42)
        expected = names + ['spam']

        names = interp.call(dir)
        self.assertEqual(names, expected)

        value = interp.call(eval, 'spam')
        self.assertEqual(value, 42)

        interp.call(exec, f'assert dir() == {expected}, dir()')

    def test_globals_from_stateless_func(self):
        # A stateless func, which doesn't depend on any globals,
        # doesn't go through pickle, so it runs in __main__.
        def set_global(name, value):
            globals()[name] = value

        def get_global(name):
            return globals().get(name)

        interp = interpreters.create()

        modname = interp.call(get_global, '__name__')
        self.assertEqual(modname, '__main__')

        res = interp.call(get_global, 'spam')
        self.assertIsNone(res)

        interp.exec('spam = True')
        res = interp.call(get_global, 'spam')
        self.assertTrue(res)

        interp.call(set_global, 'spam', 42)
        res = interp.call(get_global, 'spam')
        self.assertEqual(res, 42)

        interp.exec('assert spam == 42, repr(spam)')

    def test_call_in_thread(self):
        interp = interpreters.create()

        for i, (callable, args, kwargs) in enumerate([
            (call_func_noop, (), {}),
            (call_func_return_shareable, (), {}),
            (call_func_return_pickleable, (), {}),
            (Spam.from_values, (), {}),
            (Spam.from_values, (1, 2, 3), {}),
            (Spam(101), (), {}),
            (Spam(10101).run, (), {}),
            (Spam.noop, (), {}),
            (call_func_complex, ('ident', 'spam'), {}),
            (call_func_complex, ('full-ident', 'spam'), {}),
            (call_func_complex, ('full-ident', 'spam', 'ham'), {'eggs': '!!!'}),
            (call_func_complex, ('globals',), {}),
            (call_func_complex, ('interpid',), {}),
            (call_func_complex, ('custom', 'spam!'), {}),
        ]):
            with self.subTest(f'success case #{i+1}'):
                with self.captured_thread_exception() as ctx:
                    t = interp.call_in_thread(callable, *args, **kwargs)
                    t.join()
                self.assertIsNone(ctx.caught)

        for i, (callable, args, kwargs) in enumerate([
            (get_call_func_closure, (42,), {}),
            (get_call_func_closure(42), (), {}),
        ]):
            with self.subTest(f'invalid case #{i+1}'):
                with self.captured_thread_exception() as ctx:
                    t = interp.call_in_thread(callable, *args, **kwargs)
                    t.join()
                self.assertIsNotNone(ctx.caught)

        with self.captured_thread_exception() as ctx:
            t = interp.call_in_thread(call_func_failure)
            t.join()
        self.assertIsNotNone(ctx.caught)


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


class LowLevelTests(TestBase):

    # The behaviors in the low-level module are important in as much
    # as they are exercised by the high-level module.  Therefore the
    # most important testing happens in the high-level tests.
    # These low-level tests cover corner cases that are not
    # encountered by the high-level module, thus they
    # mostly shouldn't matter as much.

    def test_new_config(self):
        # This test overlaps with
        # test.test_capi.test_misc.InterpreterConfigTests.

        default = _interpreters.new_config('isolated')
        with self.subTest('no arg'):
            config = _interpreters.new_config()
            self.assert_ns_equal(config, default)
            self.assertIsNot(config, default)

        with self.subTest('default'):
            config1 = _interpreters.new_config('default')
            self.assert_ns_equal(config1, default)
            self.assertIsNot(config1, default)

            config2 = _interpreters.new_config('default')
            self.assert_ns_equal(config2, config1)
            self.assertIsNot(config2, config1)

        for arg in ['', 'default']:
            with self.subTest(f'default ({arg!r})'):
                config = _interpreters.new_config(arg)
                self.assert_ns_equal(config, default)
                self.assertIsNot(config, default)

        supported = {
            'isolated': types.SimpleNamespace(
                use_main_obmalloc=False,
                allow_fork=False,
                allow_exec=False,
                allow_threads=True,
                allow_daemon_threads=False,
                check_multi_interp_extensions=True,
                gil='own',
            ),
            'legacy': types.SimpleNamespace(
                use_main_obmalloc=True,
                allow_fork=True,
                allow_exec=True,
                allow_threads=True,
                allow_daemon_threads=True,
                check_multi_interp_extensions=bool(Py_GIL_DISABLED),
                gil='shared',
            ),
            'empty': types.SimpleNamespace(
                use_main_obmalloc=False,
                allow_fork=False,
                allow_exec=False,
                allow_threads=False,
                allow_daemon_threads=False,
                check_multi_interp_extensions=False,
                gil='default',
            ),
        }
        gil_supported = ['default', 'shared', 'own']

        for name, vanilla in supported.items():
            with self.subTest(f'supported ({name})'):
                expected = vanilla
                config1 = _interpreters.new_config(name)
                self.assert_ns_equal(config1, expected)
                self.assertIsNot(config1, expected)

                config2 = _interpreters.new_config(name)
                self.assert_ns_equal(config2, config1)
                self.assertIsNot(config2, config1)

            with self.subTest(f'noop override ({name})'):
                expected = vanilla
                overrides = vars(vanilla)
                config = _interpreters.new_config(name, **overrides)
                self.assert_ns_equal(config, expected)

            with self.subTest(f'override all ({name})'):
                overrides = {k: not v for k, v in vars(vanilla).items()}
                for gil in gil_supported:
                    if vanilla.gil == gil:
                        continue
                    overrides['gil'] = gil
                    expected = types.SimpleNamespace(**overrides)
                    config = _interpreters.new_config(name, **overrides)
                    self.assert_ns_equal(config, expected)

            # Override individual fields.
            for field, old in vars(vanilla).items():
                if field == 'gil':
                    values = [v for v in gil_supported if v != old]
                else:
                    values = [not old]
                for val in values:
                    with self.subTest(f'{name}.{field} ({old!r} -> {val!r})'):
                        overrides = {field: val}
                        expected = types.SimpleNamespace(
                            **dict(vars(vanilla), **overrides),
                        )
                        config = _interpreters.new_config(name, **overrides)
                        self.assert_ns_equal(config, expected)

        with self.subTest('extra override'):
            with self.assertRaises(ValueError):
                _interpreters.new_config(spam=True)

        # Bad values for bool fields.
        for field, value in vars(supported['empty']).items():
            if field == 'gil':
                continue
            assert isinstance(value, bool)
            for value in [1, '', 'spam', 1.0, None, object()]:
                with self.subTest(f'bad override ({field}={value!r})'):
                    with self.assertRaises(TypeError):
                        _interpreters.new_config(**{field: value})

        # Bad values for .gil.
        for value in [True, 1, 1.0, None, object()]:
            with self.subTest(f'bad override (gil={value!r})'):
                with self.assertRaises(TypeError):
                    _interpreters.new_config(gil=value)
        for value in ['', 'spam']:
            with self.subTest(f'bad override (gil={value!r})'):
                with self.assertRaises(ValueError):
                    _interpreters.new_config(gil=value)

    def test_get_main(self):
        interpid, whence = _interpreters.get_main()
        self.assertEqual(interpid, 0)
        self.assertEqual(whence, _interpreters.WHENCE_RUNTIME)
        self.assertEqual(
            _interpreters.whence(interpid),
            _interpreters.WHENCE_RUNTIME)

    def test_get_current(self):
        with self.subTest('main'):
            main, *_ = _interpreters.get_main()
            interpid, whence = _interpreters.get_current()
            self.assertEqual(interpid, main)
            self.assertEqual(whence, _interpreters.WHENCE_RUNTIME)

        script = f"""
            import _interpreters
            interpid, whence = _interpreters.get_current()
            print((interpid, whence))
            """
        def parse_stdout(text):
            interpid, whence = eval(text)
            return interpid, whence

        with self.subTest('from _interpreters'):
            orig = _interpreters.create()
            text = self.run_and_capture(orig, script)
            interpid, whence = parse_stdout(text)
            self.assertEqual(interpid, orig)
            self.assertEqual(whence, _interpreters.WHENCE_STDLIB)

        with self.subTest('from C-API'):
            last = 0
            for id, *_ in _interpreters.list_all():
                last = max(last, id)
            expected = last + 1
            text = self.run_temp_from_capi(script)
            interpid, whence = parse_stdout(text)
            self.assertEqual(interpid, expected)
            self.assertEqual(whence, _interpreters.WHENCE_CAPI)

    def test_list_all(self):
        mainid, *_ = _interpreters.get_main()
        interpid1 = _interpreters.create()
        interpid2 = _interpreters.create()
        interpid3 = _interpreters.create()
        expected = [
            (mainid, _interpreters.WHENCE_RUNTIME),
            (interpid1, _interpreters.WHENCE_STDLIB),
            (interpid2, _interpreters.WHENCE_STDLIB),
            (interpid3, _interpreters.WHENCE_STDLIB),
        ]

        with self.subTest('main'):
            res = _interpreters.list_all()
            self.assertEqual(res, expected)

        with self.subTest('via interp from _interpreters'):
            text = self.run_and_capture(interpid2, f"""
                import _interpreters
                print(
                    _interpreters.list_all())
                """)

            res = eval(text)
            self.assertEqual(res, expected)

        with self.subTest('via interp from C-API'):
            interpid4 = interpid3 + 1
            interpid5 = interpid4 + 1
            expected2 = expected + [
                (interpid4, _interpreters.WHENCE_CAPI),
                (interpid5, _interpreters.WHENCE_STDLIB),
            ]
            expected3 = expected + [
                (interpid5, _interpreters.WHENCE_STDLIB),
            ]
            text = self.run_temp_from_capi(f"""
                import _interpreters
                _interpreters.create()
                print(
                    _interpreters.list_all())
                """)
            res2 = eval(text)
            res3 = _interpreters.list_all()
            self.assertEqual(res2, expected2)
            self.assertEqual(res3, expected3)

    def test_create(self):
        isolated = _interpreters.new_config('isolated')
        legacy = _interpreters.new_config('legacy')
        default = isolated

        with self.subTest('no args'):
            interpid = _interpreters.create()
            config = _interpreters.get_config(interpid)
            self.assert_ns_equal(config, default)

        with self.subTest('config: None'):
            interpid = _interpreters.create(None)
            config = _interpreters.get_config(interpid)
            self.assert_ns_equal(config, default)

        with self.subTest('config: \'empty\''):
            with self.assertRaises(InterpreterError):
                # The "empty" config isn't viable on its own.
                _interpreters.create('empty')

        for arg, expected in {
            '': default,
            'default': default,
            'isolated': isolated,
            'legacy': legacy,
        }.items():
            with self.subTest(f'str arg: {arg!r}'):
                interpid = _interpreters.create(arg)
                config = _interpreters.get_config(interpid)
                self.assert_ns_equal(config, expected)

        with self.subTest('custom'):
            orig = _interpreters.new_config('empty')
            orig.use_main_obmalloc = True
            orig.check_multi_interp_extensions = bool(Py_GIL_DISABLED)
            orig.gil = 'shared'
            interpid = _interpreters.create(orig)
            config = _interpreters.get_config(interpid)
            self.assert_ns_equal(config, orig)

        with self.subTest('missing fields'):
            orig = _interpreters.new_config()
            del orig.gil
            with self.assertRaises(ValueError):
                _interpreters.create(orig)

        with self.subTest('extra fields'):
            orig = _interpreters.new_config()
            orig.spam = True
            with self.assertRaises(ValueError):
                _interpreters.create(orig)

        with self.subTest('whence'):
            interpid = _interpreters.create()
            self.assertEqual(
                _interpreters.whence(interpid),
                _interpreters.WHENCE_STDLIB)

    @requires_test_modules
    def test_destroy(self):
        with self.subTest('from _interpreters'):
            interpid = _interpreters.create()
            before = [id for id, *_ in _interpreters.list_all()]
            _interpreters.destroy(interpid)
            after = [id for id, *_ in _interpreters.list_all()]

            self.assertIn(interpid, before)
            self.assertNotIn(interpid, after)
            self.assertFalse(
                self.interp_exists(interpid))

        with self.subTest('main'):
            interpid, *_ = _interpreters.get_main()
            with self.assertRaises(InterpreterError):
                # It is the current interpreter.
                _interpreters.destroy(interpid)

        with self.subTest('from C-API'):
            interpid = _testinternalcapi.create_interpreter()
            with self.assertRaisesRegex(InterpreterError, 'unrecognized'):
                _interpreters.destroy(interpid, restrict=True)
            self.assertTrue(
                self.interp_exists(interpid))
            _interpreters.destroy(interpid)
            self.assertFalse(
                self.interp_exists(interpid))

        with self.subTest('basic C-API'):
            interpid = _testinternalcapi.create_interpreter()
            self.assertTrue(
                self.interp_exists(interpid))
            _testinternalcapi.destroy_interpreter(interpid, basic=True)
            self.assertFalse(
                self.interp_exists(interpid))

    def test_get_config(self):
        # This test overlaps with
        # test.test_capi.test_misc.InterpreterConfigTests.

        with self.subTest('main'):
            expected = _interpreters.new_config('legacy')
            expected.gil = 'own'
            if Py_GIL_DISABLED:
                expected.check_multi_interp_extensions = False
            interpid, *_ = _interpreters.get_main()
            config = _interpreters.get_config(interpid)
            self.assert_ns_equal(config, expected)

        with self.subTest('isolated'):
            expected = _interpreters.new_config('isolated')
            interpid = _interpreters.create('isolated')
            config = _interpreters.get_config(interpid)
            self.assert_ns_equal(config, expected)

        with self.subTest('legacy'):
            expected = _interpreters.new_config('legacy')
            interpid = _interpreters.create('legacy')
            config = _interpreters.get_config(interpid)
            self.assert_ns_equal(config, expected)

        with self.subTest('from C-API'):
            orig = _interpreters.new_config('isolated')
            with self.interpreter_from_capi(orig) as interpid:
                with self.assertRaisesRegex(InterpreterError, 'unrecognized'):
                    _interpreters.get_config(interpid, restrict=True)
                config = _interpreters.get_config(interpid)
            self.assert_ns_equal(config, orig)

    @requires_test_modules
    def test_whence(self):
        with self.subTest('main'):
            interpid, *_ = _interpreters.get_main()
            whence = _interpreters.whence(interpid)
            self.assertEqual(whence, _interpreters.WHENCE_RUNTIME)

        with self.subTest('stdlib'):
            interpid = _interpreters.create()
            whence = _interpreters.whence(interpid)
            self.assertEqual(whence, _interpreters.WHENCE_STDLIB)

        for orig, name in {
            _interpreters.WHENCE_UNKNOWN: 'not ready',
            _interpreters.WHENCE_LEGACY_CAPI: 'legacy C-API',
            _interpreters.WHENCE_CAPI: 'C-API',
            _interpreters.WHENCE_XI: 'cross-interpreter C-API',
        }.items():
            with self.subTest(f'from C-API ({orig}: {name})'):
                with self.interpreter_from_capi(whence=orig) as interpid:
                    whence = _interpreters.whence(interpid)
                self.assertEqual(whence, orig)

        with self.subTest('from C-API, running'):
            text = self.run_temp_from_capi(dedent(f"""
                import _interpreters
                interpid, *_ = _interpreters.get_current()
                print(_interpreters.whence(interpid))
                """),
                config=True)
            whence = eval(text)
            self.assertEqual(whence, _interpreters.WHENCE_CAPI)

        with self.subTest('from legacy C-API, running'):
            ...
            text = self.run_temp_from_capi(dedent(f"""
                import _interpreters
                interpid, *_ = _interpreters.get_current()
                print(_interpreters.whence(interpid))
                """),
                config=False)
            whence = eval(text)
            self.assertEqual(whence, _interpreters.WHENCE_LEGACY_CAPI)

    def test_is_running(self):
        def check(interpid, expected):
            with self.assertRaisesRegex(InterpreterError, 'unrecognized'):
                _interpreters.is_running(interpid, restrict=True)
            running = _interpreters.is_running(interpid)
            self.assertIs(running, expected)

        with self.subTest('from _interpreters (running)'):
            interpid = _interpreters.create()
            with self.running(interpid):
                running = _interpreters.is_running(interpid)
                self.assertTrue(running)

        with self.subTest('from _interpreters (not running)'):
            interpid = _interpreters.create()
            running = _interpreters.is_running(interpid)
            self.assertFalse(running)

        with self.subTest('main'):
            interpid, *_ = _interpreters.get_main()
            check(interpid, True)

        with self.subTest('from C-API (running __main__)'):
            with self.interpreter_from_capi() as interpid:
                with self.running_from_capi(interpid, main=True):
                    check(interpid, True)

        with self.subTest('from C-API (running, but not __main__)'):
            with self.interpreter_from_capi() as interpid:
                with self.running_from_capi(interpid, main=False):
                    check(interpid, False)

        with self.subTest('from C-API (not running)'):
            with self.interpreter_from_capi() as interpid:
                check(interpid, False)

    def test_exec(self):
        with self.subTest('run script'):
            interpid = _interpreters.create()
            script, results = _captured_script('print("it worked!", end="")')
            with results:
                exc = _interpreters.exec(interpid, script)
            results = results.final()
            results.raise_if_failed()
            out = results.stdout
            self.assertEqual(out, 'it worked!')

        with self.subTest('uncaught exception'):
            interpid = _interpreters.create()
            script, results = _captured_script("""
                raise Exception('uh-oh!')
                print("it worked!", end="")
                """)
            with results:
                exc = _interpreters.exec(interpid, script)
                out = results.stdout()
            expected = build_excinfo(
                Exception, 'uh-oh!',
                # We check these in other tests.
                formatted=exc.formatted,
                errdisplay=exc.errdisplay,
            )
            self.assertEqual(out, '')
            self.assert_ns_equal(exc, expected)

        with self.subTest('from C-API'):
            with self.interpreter_from_capi() as interpid:
                with self.assertRaisesRegex(InterpreterError, 'unrecognized'):
                    _interpreters.exec(interpid, 'raise Exception("it worked!")',
                                       restrict=True)
                exc = _interpreters.exec(interpid, 'raise Exception("it worked!")')
            self.assertIsNot(exc, None)
            self.assertEqual(exc.msg, 'it worked!')

    def test_call(self):
        interpid = _interpreters.create()

        # Here we focus on basic args and return values.
        # See TestInterpreterCall for full operational coverage,
        # including supported callables.

        with self.subTest('no args, return None'):
            func = defs.spam_minimal
            res, exc = _interpreters.call(interpid, func)
            self.assertIsNone(exc)
            self.assertIsNone(res)

        with self.subTest('empty args, return None'):
            func = defs.spam_minimal
            res, exc = _interpreters.call(interpid, func, (), {})
            self.assertIsNone(exc)
            self.assertIsNone(res)

        with self.subTest('no args, return non-None'):
            func = defs.script_with_return
            res, exc = _interpreters.call(interpid, func)
            self.assertIsNone(exc)
            self.assertIs(res, True)

        with self.subTest('full args, return non-None'):
            expected = (1, 2, 3, 4, 5, 6, (7, 8), {'g': 9, 'h': 0})
            func = defs.spam_full_args
            args = (1, 2, 3, 4, 7, 8)
            kwargs = dict(e=5, f=6, g=9, h=0)
            res, exc = _interpreters.call(interpid, func, args, kwargs)
            self.assertIsNone(exc)
            self.assertEqual(res, expected)

        with self.subTest('uncaught exception'):
            func = defs.spam_raises
            res, exc = _interpreters.call(interpid, func)
            expected = build_excinfo(
                Exception, 'spam!',
                # We check these in other tests.
                formatted=exc.formatted,
                errdisplay=exc.errdisplay,
            )
            self.assertIsNone(res)
            self.assertEqual(exc, expected)

    @requires_test_modules
    def test_set___main___attrs(self):
        with self.subTest('from _interpreters'):
            interpid = _interpreters.create()
            before1 = _interpreters.exec(interpid, 'assert spam == \'eggs\'')
            before2 = _interpreters.exec(interpid, 'assert ham == 42')
            self.assertEqual(before1.type.__name__, 'NameError')
            self.assertEqual(before2.type.__name__, 'NameError')

            _interpreters.set___main___attrs(interpid, dict(
                spam='eggs',
                ham=42,
            ))
            after1 = _interpreters.exec(interpid, 'assert spam == \'eggs\'')
            after2 = _interpreters.exec(interpid, 'assert ham == 42')
            after3 = _interpreters.exec(interpid, 'assert spam == 42')
            self.assertIs(after1, None)
            self.assertIs(after2, None)
            self.assertEqual(after3.type.__name__, 'AssertionError')

            with self.assertRaises(ValueError):
                # GH-127165: Embedded NULL characters broke the lookup
                _interpreters.set___main___attrs(interpid, {"\x00": 1})

        with self.subTest('from C-API'):
            with self.interpreter_from_capi() as interpid:
                with self.assertRaisesRegex(InterpreterError, 'unrecognized'):
                    _interpreters.set___main___attrs(interpid, {'spam': True},
                                                     restrict=True)
                _interpreters.set___main___attrs(interpid, {'spam': True})
                rc = _testinternalcapi.exec_interpreter(
                    interpid,
                    'assert spam is True',
                )
            self.assertEqual(rc, 0)


if __name__ == '__main__':
    # Test needs to be a package, so we can do relative imports.
    unittest.main()
