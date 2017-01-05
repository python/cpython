import ast
import os.path
import shutil
import tempfile
from textwrap import dedent
import threading
import unittest

from test import support

_interpreters = support.import_module('_interpreters')
interpreters = support.import_module('interpreters')


class TestBase(unittest.TestCase):

    def setUp(self):
        self.dirname = None

    def tearDown(self):
        if self.dirname is not None:
            try:
                shutil.rmtree(self.dirname)
            except FileNotFoundError:
                pass  # already deleted

        for id in _interpreters.enumerate():
            if id == 0:  # main
                continue
            try:
                _interpreters.destroy(id)
            except RuntimeError:
                pass  # already destroyed

    def _resolve_filename(self, name):
        if self.dirname is None:
            self.dirname = tempfile.mkdtemp()
        return os.path.join(self.dirname, name)

    def _empty_file(self, name):
        filename = self._resolve_filename(name)
        support.create_empty_file(filename)
        return filename

    def assert_file_contains(self, filename, expected):
        with open(filename) as out:
            content = out.read()
        self.assertEqual(content, expected)


class ModuleTests(TestBase):

    def test_enumerate(self):
        main_id = _interpreters.get_main()
        got = interpreters.enumerate()
        self.assertEqual(set(i.id for i in got), {main_id})

        id1 = _interpreters.create()
        id2 = _interpreters.create()
        got = interpreters.enumerate()
        self.assertEqual(set(i.id for i in got), {main_id, id1, id2})

    def test_current(self):
        main_id = _interpreters.get_main()
        current = interpreters.current()
        self.assertEqual(current.id, main_id)

        id = _interpreters.create()
        script = dedent("""
            import interpreters
            interp = interpreters.current()
            """)
        ns = _interpreters.run_string_unrestricted(id, script)
        current = ns['interp']
        self.assertEqual(current.id, id)

    def test_main(self):
        expected = _interpreters.get_main()
        main = interpreters.main()

        self.assertEqual(main.id, expected)

    def test_create(self):
        main_id = _interpreters.get_main()
        interp = interpreters.create()

        self.assertIsInstance(interp, interpreters.Interpreter)
        self.assertGreater(interp.id, main_id)


class InterpreterTests(TestBase):

    def test_repr(self):
        interp = interpreters.Interpreter(10)
        result = repr(interp)

        self.assertEqual(result, 'Interpreter(id=10)')

    def test_equality(self):
        interp1 = interpreters.Interpreter(0)
        interp2 = interpreters.Interpreter(10)
        interp3 = interpreters.Interpreter(10)
        different = (interp1 == interp2)
        same = (interp2 == interp3)
        identity = (interp1 == interp1)

        self.assertFalse(different)
        self.assertTrue(same)
        self.assertTrue(identity)

    def test_id(self):
        interp = interpreters.Interpreter(10)
        id = interp.id

        self.assertEqual(id, 10)

    def test_is_running(self):
        interp = interpreters.create()
        is_running = interp.is_running()
        self.assertFalse(is_running)

        script = 'is_running = interp.is_running()'
        ns = _interpreters.run_string_unrestricted(interp.id, script, {
                'interp': interp,
                })
        is_running = ns['is_running']
        self.assertTrue(is_running)

    def test_destroy(self):
        before = set(_interpreters.enumerate())
        interp = interpreters.create()
        self.assertEqual(set(_interpreters.enumerate()), before | {interp.id})

        interp.destroy()
        after = set(_interpreters.enumerate())

        self.assertEqual(before, after)

    def test_run_string(self):
        filename = self._empty_file('spam')
        data = 'success!'
        script = dedent(f"""
            with open('{filename}', 'w') as out:
                out.write('{data}')
            """)
        interp = interpreters.create()

        interp.run(script)
        self.assert_file_contains(filename, data)

    def test_run_unsupported(self):
        script = 'raise Exception'
        interp = interpreters.create()

        with self.assertRaises(NotImplementedError):
            interp.run(None)
        with self.assertRaises(NotImplementedError):
            interp.run(10)
        node = compile(script, '<script>', 'exec', ast.PyCF_ONLY_AST)
        with self.assertRaises(NotImplementedError):
            interp.run(node)
        code = compile(script, '<script>', 'exec')
        with self.assertRaises(NotImplementedError):
            interp.run(code)
        f = lambda: None
        with self.assertRaises(NotImplementedError):
            interp.run(f)
