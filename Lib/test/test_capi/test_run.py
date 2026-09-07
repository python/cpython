import ast
import os
import sys
import tempfile
import textwrap
import unittest
from collections import UserDict
from test import support
from test.support import import_helper
from test.support.os_helper import unlink, TESTFN, TESTFN_ASCII, TESTFN_UNDECODABLE

_testcapi = import_helper.import_module('_testcapi')
_testlimitedcapi = import_helper.import_module('_testlimitedcapi')


NULL = None
Py_single_input = _testcapi.Py_single_input
Py_file_input = _testcapi.Py_file_input
Py_eval_input = _testcapi.Py_eval_input
INVALID_START = Py_single_input - 1
STDIN = '<stdin>'
STDERR_FD = 2
PyCF_ONLY_AST = _testcapi.PyCF_ONLY_AST
PyCF_IGNORE_COOKIE = _testcapi.PyCF_IGNORE_COOKIE

# Code raising a SyntaxError
SYNTAX_ERROR = 'True = 1'

# Raise a SystemExit with exit code 42
CODE_EXIT_42 = 'raise SystemExit(42)'


def create_text_file(filename, text):
    with open(filename, 'w', encoding='utf8') as fp:
        fp.write(text)


class DictSubclass(dict):
    pass


class capture_excepthook:
    def __init__(self):
        self.exc = None
        self._old_hook = None

    def _hook(self, exc_type, exc_value, exc_tb):
        # Storing the exception instance creates a reference cycle.
        self.exc = exc_value

    def __enter__(self):
        self._old_hook = sys.excepthook
        sys.excepthook = self._hook
        return self

    def __exit__(self, *exc_info):
        sys.excepthook = self._old_hook
        self.exc = None


class CAPITest(unittest.TestCase):
    def check_run_string(self, func):
        def run(s, *args):
            return func(s, Py_file_input, *args)

        ns = {}
        self.assertIsNone(run(b'x = 1', ns, ns))
        self.assertEqual(ns['x'], 1)

        with self.assertRaises(SyntaxError) as cm:
            run(SYNTAX_ERROR.encode(), {})
        self.assertEqual(cm.exception.filename, '<string>')

        with self.assertRaises(ValueError) as cm:
            run(b'raise ValueError("BUG")', {})
        self.assertEqual(str(cm.exception), 'BUG')

        with self.assertRaises(ValueError):
            func(b'x = 1', INVALID_START, {})

        self.assertIsNone(run(b'a\n', dict(a=1)))
        self.assertIsNone(run(b'a\n', dict(a=1), {}))
        self.assertIsNone(run(b'a\n', {}, dict(a=1)))
        self.assertIsNone(run(b'a\n', {}, UserDict(a=1)))

        self.assertRaises(NameError, run, b'a\n', {})
        self.assertRaises(NameError, run, b'a\n', {}, {})
        self.assertRaises(TypeError, run, b'a\n', dict(a=1), [])
        self.assertRaises(TypeError, run, b'a\n', dict(a=1), 1)

        self.assertIsNone(run(b'a\n', DictSubclass(a=1)))
        self.assertIsNone(run(b'a\n', DictSubclass(), dict(a=1)))
        self.assertRaises(NameError, run, b'a\n', DictSubclass())

        self.assertIsNone(run(b'\xc3\xa4\n', {'\xe4': 1}))
        self.assertRaises(SyntaxError, run, b'\xe4\n', {})

        self.assertRaises(SystemError, run, b'a\n', NULL)
        self.assertRaises(SystemError, run, b'a\n', NULL, {})
        self.assertRaises(SystemError, run, b'a\n', NULL, dict(a=1))
        self.assertRaises(SystemError, run, b'a\n', UserDict())
        self.assertRaises(SystemError, run, b'a\n', UserDict(), {})
        self.assertRaises(SystemError, run, b'a\n', UserDict(), dict(a=1))

        # CRASHES run(NULL, {})

    def test_run_string(self):
        # Test PyRun_String()
        self.check_run_string(_testcapi.run_string)

    def test_run_stringflags(self):
        # Test PyRun_StringFlags().
        self.check_run_string(_testcapi.run_stringflags)

    def check_run_file(self, func, support_closeit):
        # XXX: fopen() uses different path encoding than Python on Windows.
        filename = os.fsencode(TESTFN if os.name != 'nt' else TESTFN_ASCII)
        create_text_file(filename, 'a\n')
        self.addCleanup(unlink, filename)
        def run(*args):
            return func(filename, Py_file_input, *args)

        self.assertIsNone(run(dict(a=1)))
        self.assertIsNone(run(dict(a=1), {}))
        self.assertIsNone(run({}, dict(a=1)))
        self.assertIsNone(run({}, UserDict(a=1)))
        if support_closeit:
            closeit = 1
            self.assertIsNone(run(dict(a=1), {}, closeit))

        with self.assertRaises(ValueError):
            func(filename, INVALID_START, {})

        self.assertRaises(NameError, run, {})
        self.assertRaises(NameError, run, {}, {})
        self.assertRaises(TypeError, run, dict(a=1), [])
        self.assertRaises(TypeError, run, dict(a=1), 1)

        self.assertIsNone(run(DictSubclass(a=1)))
        self.assertIsNone(run(DictSubclass(), dict(a=1)))
        self.assertRaises(NameError, run, DictSubclass())

        self.assertRaises(SystemError, run, NULL)
        self.assertRaises(SystemError, run, NULL, {})
        self.assertRaises(SystemError, run, NULL, dict(a=1))
        self.assertRaises(SystemError, run, UserDict())
        self.assertRaises(SystemError, run, UserDict(), {})
        self.assertRaises(SystemError, run, UserDict(), dict(a=1))

        # syntax error
        create_text_file(filename, SYNTAX_ERROR)
        with self.assertRaises(SyntaxError) as cm:
            self.assertIsNone(run({}))
        self.assertEqual(cm.exception.filename, os.fsdecode(filename))

        # raise exception
        create_text_file(filename, 'raise ValueError("BUG")')
        with self.assertRaises(ValueError) as cm:
            self.assertIsNone(run({}))
        self.assertEqual(str(cm.exception), 'BUG')

        # Test undecodable filename
        if TESTFN_UNDECODABLE and os.name != 'nt':
            try:
                create_text_file(TESTFN_UNDECODABLE, 'a\n')
                self.addCleanup(unlink, TESTFN_UNDECODABLE)
            except OSError:
                # undecodable paths are not supported
                pass
            else:
                self.assertIsNone(func(TESTFN_UNDECODABLE, Py_file_input, dict(a=1)))

    def test_run_file(self):
        # Test PyRun_File().
        self.check_run_file(_testcapi.run_file, False)

    def test_run_fileex(self):
        # Test PyRun_FileEx().
        self.check_run_file(_testcapi.run_fileex, True)

    def test_run_fileflags(self):
        # Test PyRun_FileFlags().
        self.check_run_file(_testcapi.run_fileflags, True)

    def test_run_fileexflags(self):
        # Test PyRun_FileExFlags().
        self.check_run_file(_testcapi.run_fileexflags, True)

    def check_run_simplefile(self, func_name, have_closeit):
        run = getattr(_testcapi, func_name)

        open_filename = TESTFN
        with open(open_filename, 'w') as fp:
            print('import sys', file=fp)
            print('print(__file__)', file=fp)
            print('mod = sys.modules["__main__"]', file=fp)
            print('print(type(mod.__loader__).__name__)', file=fp)
        self.addCleanup(unlink, open_filename)

        class MockLoader:
            pass
        loader = MockLoader()

        for closeit in (0, 1):
            for set_file in (False, True):
                for filename in (open_filename, 'custom filename', STDIN):
                    set_loader = (filename != STDIN)
                    with self.subTest(closeit=closeit,
                                      set_file=set_file,
                                      filename=filename):
                        mod = sys.modules['__main__']
                        mod_file = mod.__file__
                        with (support.captured_stdout() as stdout,
                              support.swap_attr(mod, '__loader__', loader)):
                            try:
                                if set_file:
                                    del mod.__file__
                                filename_arg = os.fsencode(filename)
                                if have_closeit:
                                    res = run(open_filename, filename_arg, closeit)
                                else:
                                    res = run(open_filename, filename_arg)
                                if set_file:
                                    self.assertFalse(hasattr(mod, '__file__'))
                            finally:
                                mod.__file__ = mod_file

                        self.assertEqual(res, 0)
                        expected = [filename if set_file else mod_file]
                        if set_loader:
                            expected.append('SourceFileLoader')
                        else:
                            expected.append('MockLoader')
                        self.assertEqual(stdout.getvalue().splitlines(),
                                         expected)

    def test_run_simplefile(self):
        # Test PyRun_SimpleFile()
        self.check_run_simplefile('run_simplefile', False)

    def test_run_simplefileex(self):
        # Test PyRun_SimpleFileEx()
        self.check_run_simplefile('run_simplefileex', True)

    def test_run_simplefileexflags(self):
        # Test PyRun_SimpleFileExFlags()
        self.check_run_simplefile('run_simplefileexflags', True)

    def test_run_anyfile(self):
        # Test PyRun_AnyFile()
        self.check_run_simplefile('run_anyfile', False)

    def test_run_anyfileex(self):
        # Test PyRun_AnyFileEx()
        self.check_run_simplefile('run_anyfileex', True)

    def test_run_anyfileflags(self):
        # Test PyRun_AnyFileFlags()
        self.check_run_simplefile('run_anyfileflags', True)

    def _check_run_interactive(self, run, encode_filename, use_loop):
        open_filename = TESTFN
        if use_loop:
            with open(open_filename, 'w') as fp:
                print('welcome = "hello REPL"', file=fp)
                print('print(welcome)', file=fp)
        else:
            create_text_file(open_filename, 'print("hello REPL")')
        self.addCleanup(unlink, open_filename)

        for filename in ('filename', STDIN):
            with self.subTest(filename=filename):
                with support.captured_stdout() as stdout:
                    if encode_filename:
                        filename = filename.encode()
                    self.assertEqual(run(open_filename, filename), 0)
                self.assertEqual(stdout.getvalue(), 'hello REPL\n')

        create_text_file(open_filename, SYNTAX_ERROR)
        filename = 'custom filename'
        if encode_filename:
            filename_arg = filename.encode()
        else:
            filename_arg = filename
        with capture_excepthook() as excepthook:
            expected = (0 if use_loop else -1)
            self.assertEqual(run(open_filename, filename_arg), expected)
            self.assertIsInstance(excepthook.exc, SyntaxError)
            self.assertEqual(excepthook.exc.filename, filename)

        create_text_file(open_filename, 'raise ValueError("BUG")')
        with capture_excepthook() as excepthook:
            expected = (0 if use_loop else -1)
            self.assertEqual(run(open_filename, filename_arg), expected)
            self.assertIsInstance(excepthook.exc, ValueError)
            self.assertEqual(str(excepthook.exc), 'BUG')

        if not encode_filename:
            # wrong type for the second parameter (filename)
            with capture_excepthook() as excepthook:
                self.assertEqual(run(open_filename, b'bytes'), -1)
                self.assertIsInstance(excepthook.exc, TypeError)

    def check_run_interactive(self, run, encode_filename, use_loop=False):
        # Redirect stderr to a temporary file to hide '>>> ' from the REPL
        try:
            stderr_copy = os.dup(STDERR_FD)
        except OSError:
            # On WASI, dup(STDERR_FD) fails with "OSError: [Errno 58] Not
            # supported". In this case, run the test without redirecting
            # stderr to a temporary file.
            self._check_run_interactive(run, encode_filename, use_loop)
            return

        with tempfile.TemporaryFile() as tmp:
            try:
                os.dup2(tmp.fileno(), STDERR_FD)
                self._check_run_interactive(run, encode_filename, use_loop)
            finally:
                os.dup2(stderr_copy, STDERR_FD)
                os.close(stderr_copy)

    def test_run_interactiveone(self):
        # Test PyRun_InteractiveOne()
        run = _testcapi.run_interactiveone
        self.check_run_interactive(run, True)

    def test_run_interactiveoneflags(self):
        # Test PyRun_InteractiveOneFlags()
        run = _testcapi.run_interactiveoneflags
        self.check_run_interactive(run, True)

    def test_run_interactiveoneobject(self):
        # Test PyRun_InteractiveOneObject()
        run = _testcapi.run_interactiveoneobject
        self.check_run_interactive(run, False)

    def test_run_interactiveloop(self):
        # Test PyRun_InteractiveLoop()
        run = _testcapi.run_interactiveloop
        self.check_run_interactive(run, True, use_loop=True)

    def test_run_interactiveloopflags(self):
        # Test PyRun_InteractiveLoopFlags()
        run = _testcapi.run_interactiveloopflags
        self.check_run_interactive(run, True, use_loop=True)

    def test_run_anyfileexflags(self):
        # Test PyRun_AnyFileExFlags()
        self.check_run_simplefile('run_anyfileexflags', True)

    def check_run_simplestring(self, run):
        with support.captured_stdout() as stdout:
            run(b'print("simple hello")')
        self.assertEqual(stdout.getvalue(), 'simple hello\n')

        with capture_excepthook() as excepthook:
            self.assertEqual(run(SYNTAX_ERROR.encode()), -1)
            self.assertIsInstance(excepthook.exc, SyntaxError)
            self.assertEqual(excepthook.exc.filename, '<string>')

        with capture_excepthook() as excepthook:
            self.assertEqual(run(b'raise ValueError("BUG")'), -1)
            self.assertIsInstance(excepthook.exc, ValueError)
            self.assertEqual(str(excepthook.exc), 'BUG')

    def test_run_simplestring(self):
        # Test PyRun_SimpleString()
        self.check_run_simplestring(_testcapi.run_simplestring)

    def test_run_simplestringflags(self):
        # Test PyRun_SimpleStringFlags()
        self.check_run_simplestring(_testcapi.run_simplestringflags)

    def check_compilestring(self, compilestring, has_flags, encode_filename=True):
        filename_str = TESTFN
        if encode_filename:
            filename = os.fsencode(filename_str)
        else:
            filename = filename_str

        def check_code(co, name, value):
            ns = {}
            exec(co, ns, ns)
            self.assertEqual(ns[name], value)

        co = compilestring(b'x = 1', filename, Py_file_input)
        self.assertEqual(co.co_filename, filename_str)
        check_code(co, 'x', 1)

        if has_flags:
            code = textwrap.dedent("""
                # encoding: latin1
                x = 'a\xe9'
            """)
            co = compilestring(code.encode(), filename, Py_file_input, PyCF_IGNORE_COOKIE)
            self.assertEqual(co.co_filename, filename_str)
            check_code(co, 'x', 'a\xe9')

            co = compilestring(code.encode(), filename, Py_file_input)
            self.assertEqual(co.co_filename, filename_str)
            check_code(co, 'x', 'a\xc3\xa9')

            tree = compilestring(b'x = 1', filename, Py_file_input, PyCF_ONLY_AST)
            self.assertIsInstance(tree, ast.AST)

        co = compilestring(b'raise ValueError("BUG")', filename, Py_file_input)
        with self.assertRaises(ValueError):
            exec(co, {})

        with self.assertRaises(SyntaxError) as cm:
            compilestring(SYNTAX_ERROR.encode(), filename, Py_file_input)

        with self.assertRaises(ValueError):
            compilestring(b'x = 1', filename, INVALID_START)

    def test_compilestring(self):
        # Test Py_CompileString()
        self.check_compilestring(_testlimitedcapi.run_compilestring, False)

    def test_compilestringflags(self):
        # Test Py_CompileStringFlags()
        self.check_compilestring(_testcapi.run_compilestringflags, True)

    def test_compilestringexflags(self):
        # Test Py_CompileStringExFlags()
        self.check_compilestring(_testcapi.run_compilestringexflags, True)

    def test_compilestringobject(self):
        # Test Py_CompileStringObject()
        self.check_compilestring(_testcapi.run_compilestringobject, True,
                                 encode_filename=False)


if __name__ == '__main__':
    unittest.main()
