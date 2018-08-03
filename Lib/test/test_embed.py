# Run the tests in Programs/_testembed.c (tests for the CPython embedding APIs)
from test import support
import unittest

from collections import namedtuple
import os
import re
import subprocess
import sys


class EmbeddingTestsMixin:
    def setUp(self):
        here = os.path.abspath(__file__)
        basepath = os.path.dirname(os.path.dirname(os.path.dirname(here)))
        exename = "_testembed"
        if sys.platform.startswith("win"):
            ext = ("_d" if "_d" in sys.executable else "") + ".exe"
            exename += ext
            exepath = os.path.dirname(sys.executable)
        else:
            exepath = os.path.join(basepath, "Programs")
        self.test_exe = exe = os.path.join(exepath, exename)
        if not os.path.exists(exe):
            self.skipTest("%r doesn't exist" % exe)
        # This is needed otherwise we get a fatal error:
        # "Py_Initialize: Unable to get the locale encoding
        # LookupError: no codec search functions registered: can't find encoding"
        self.oldcwd = os.getcwd()
        os.chdir(basepath)

    def tearDown(self):
        os.chdir(self.oldcwd)

    def run_embedded_interpreter(self, *args, env=None):
        """Runs a test in the embedded interpreter"""
        cmd = [self.test_exe]
        cmd.extend(args)
        if env is not None and sys.platform == 'win32':
            # Windows requires at least the SYSTEMROOT environment variable to
            # start Python.
            env = env.copy()
            env['SYSTEMROOT'] = os.environ['SYSTEMROOT']

        p = subprocess.Popen(cmd,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE,
                             universal_newlines=True,
                             env=env)
        (out, err) = p.communicate()
        if p.returncode != 0 and support.verbose:
            print(f"--- {cmd} failed ---")
            print(f"stdout:\n{out}")
            print(f"stderr:\n{err}")
            print(f"------")

        self.assertEqual(p.returncode, 0,
                         "bad returncode %d, stderr is %r" %
                         (p.returncode, err))
        return out, err

    def run_repeated_init_and_subinterpreters(self):
        out, err = self.run_embedded_interpreter("repeated_init_and_subinterpreters")
        self.assertEqual(err, "")

        # The output from _testembed looks like this:
        # --- Pass 0 ---
        # interp 0 <0x1cf9330>, thread state <0x1cf9700>: id(modules) = 139650431942728
        # interp 1 <0x1d4f690>, thread state <0x1d35350>: id(modules) = 139650431165784
        # interp 2 <0x1d5a690>, thread state <0x1d99ed0>: id(modules) = 139650413140368
        # interp 3 <0x1d4f690>, thread state <0x1dc3340>: id(modules) = 139650412862200
        # interp 0 <0x1cf9330>, thread state <0x1cf9700>: id(modules) = 139650431942728
        # --- Pass 1 ---
        # ...

        interp_pat = (r"^interp (\d+) <(0x[\dA-F]+)>, "
                      r"thread state <(0x[\dA-F]+)>: "
                      r"id\(modules\) = ([\d]+)$")
        Interp = namedtuple("Interp", "id interp tstate modules")

        numloops = 0
        current_run = []
        for line in out.splitlines():
            if line == "--- Pass {} ---".format(numloops):
                self.assertEqual(len(current_run), 0)
                if support.verbose > 1:
                    print(line)
                numloops += 1
                continue

            self.assertLess(len(current_run), 5)
            match = re.match(interp_pat, line)
            if match is None:
                self.assertRegex(line, interp_pat)

            # Parse the line from the loop.  The first line is the main
            # interpreter and the 3 afterward are subinterpreters.
            interp = Interp(*match.groups())
            if support.verbose > 1:
                print(interp)
            self.assertTrue(interp.interp)
            self.assertTrue(interp.tstate)
            self.assertTrue(interp.modules)
            current_run.append(interp)

            # The last line in the loop should be the same as the first.
            if len(current_run) == 5:
                main = current_run[0]
                self.assertEqual(interp, main)
                yield current_run
                current_run = []


class EmbeddingTests(EmbeddingTestsMixin, unittest.TestCase):
    def test_subinterps_main(self):
        for run in self.run_repeated_init_and_subinterpreters():
            main = run[0]

            self.assertEqual(main.id, '0')

    def test_subinterps_different_ids(self):
        for run in self.run_repeated_init_and_subinterpreters():
            main, *subs, _ = run

            mainid = int(main.id)
            for i, sub in enumerate(subs):
                self.assertEqual(sub.id, str(mainid + i + 1))

    def test_subinterps_distinct_state(self):
        for run in self.run_repeated_init_and_subinterpreters():
            main, *subs, _ = run

            if '0x0' in main:
                # XXX Fix on Windows (and other platforms): something
                # is going on with the pointers in Programs/_testembed.c.
                # interp.interp is 0x0 and interp.modules is the same
                # between interpreters.
                raise unittest.SkipTest('platform prints pointers as 0x0')

            for sub in subs:
                # A new subinterpreter may have the same
                # PyInterpreterState pointer as a previous one if
                # the earlier one has already been destroyed.  So
                # we compare with the main interpreter.  The same
                # applies to tstate.
                self.assertNotEqual(sub.interp, main.interp)
                self.assertNotEqual(sub.tstate, main.tstate)
                self.assertNotEqual(sub.modules, main.modules)

    def test_forced_io_encoding(self):
        # Checks forced configuration of embedded interpreter IO streams
        env = dict(os.environ, PYTHONIOENCODING="utf-8:surrogateescape")
        out, err = self.run_embedded_interpreter("forced_io_encoding", env=env)
        if support.verbose > 1:
            print()
            print(out)
            print(err)
        expected_stream_encoding = "utf-8"
        expected_errors = "surrogateescape"
        expected_output = '\n'.join([
        "--- Use defaults ---",
        "Expected encoding: default",
        "Expected errors: default",
        "stdin: {in_encoding}:{errors}",
        "stdout: {out_encoding}:{errors}",
        "stderr: {out_encoding}:backslashreplace",
        "--- Set errors only ---",
        "Expected encoding: default",
        "Expected errors: ignore",
        "stdin: {in_encoding}:ignore",
        "stdout: {out_encoding}:ignore",
        "stderr: {out_encoding}:backslashreplace",
        "--- Set encoding only ---",
        "Expected encoding: latin-1",
        "Expected errors: default",
        "stdin: latin-1:{errors}",
        "stdout: latin-1:{errors}",
        "stderr: latin-1:backslashreplace",
        "--- Set encoding and errors ---",
        "Expected encoding: latin-1",
        "Expected errors: replace",
        "stdin: latin-1:replace",
        "stdout: latin-1:replace",
        "stderr: latin-1:backslashreplace"])
        expected_output = expected_output.format(
                                in_encoding=expected_stream_encoding,
                                out_encoding=expected_stream_encoding,
                                errors=expected_errors)
        # This is useful if we ever trip over odd platform behaviour
        self.maxDiff = None
        self.assertEqual(out.strip(), expected_output)

    def test_pre_initialization_api(self):
        """
        Checks some key parts of the C-API that need to work before the runtine
        is initialized (via Py_Initialize()).
        """
        env = dict(os.environ, PYTHONPATH=os.pathsep.join(sys.path))
        out, err = self.run_embedded_interpreter("pre_initialization_api", env=env)
        if sys.platform == "win32":
            expected_path = self.test_exe
        else:
            expected_path = os.path.join(os.getcwd(), "spam")
        expected_output = f"sys.executable: {expected_path}\n"
        self.assertIn(expected_output, out)
        self.assertEqual(err, '')

    def test_pre_initialization_sys_options(self):
        """
        Checks that sys.warnoptions and sys._xoptions can be set before the
        runtime is initialized (otherwise they won't be effective).
        """
        env = dict(os.environ, PYTHONPATH=os.pathsep.join(sys.path))
        out, err = self.run_embedded_interpreter(
                        "pre_initialization_sys_options", env=env)
        expected_output = (
            "sys.warnoptions: ['once', 'module', 'default']\n"
            "sys._xoptions: {'not_an_option': '1', 'also_not_an_option': '2'}\n"
            "warnings.filters[:3]: ['default', 'module', 'once']\n"
        )
        self.assertIn(expected_output, out)
        self.assertEqual(err, '')

    def test_bpo20891(self):
        """
        bpo-20891: Calling PyGILState_Ensure in a non-Python thread before
        calling PyEval_InitThreads() must not crash. PyGILState_Ensure() must
        call PyEval_InitThreads() for us in this case.
        """
        out, err = self.run_embedded_interpreter("bpo20891")
        self.assertEqual(out, '')
        self.assertEqual(err, '')

    def test_initialize_twice(self):
        """
        bpo-33932: Calling Py_Initialize() twice should do nothing (and not
        crash!).
        """
        out, err = self.run_embedded_interpreter("initialize_twice")
        self.assertEqual(out, '')
        self.assertEqual(err, '')

    def test_initialize_pymain(self):
        """
        bpo-34008: Calling Py_Main() after Py_Initialize() must not fail.
        """
        out, err = self.run_embedded_interpreter("initialize_pymain")
        self.assertEqual(out.rstrip(), "Py_Main() after Py_Initialize: sys.argv=['-c', 'arg2']")
        self.assertEqual(err, '')


class InitConfigTests(EmbeddingTestsMixin, unittest.TestCase):
    maxDiff = 4096
    DEFAULT_CONFIG = {
        'install_signal_handlers': 1,
        'Py_IgnoreEnvironmentFlag': 0,
        'use_hash_seed': 0,
        'hash_seed': 0,
        'allocator': '(null)',
        'dev_mode': 0,
        'faulthandler': 0,
        'tracemalloc': 0,
        'import_time': 0,
        'show_ref_count': 0,
        'show_alloc_count': 0,
        'dump_refs': 0,
        'malloc_stats': 0,
        'utf8_mode': 0,

        'coerce_c_locale': 0,
        'coerce_c_locale_warn': 0,

        'program_name': './_testembed',
        'argc': 0,
        'argv': '[]',
        'program': '(null)',

        'Py_IsolatedFlag': 0,
        'Py_NoSiteFlag': 0,
        'Py_BytesWarningFlag': 0,
        'Py_InspectFlag': 0,
        'Py_InteractiveFlag': 0,
        'Py_OptimizeFlag': 0,
        'Py_DebugFlag': 0,
        'Py_DontWriteBytecodeFlag': 0,
        'Py_VerboseFlag': 0,
        'Py_QuietFlag': 0,
        'Py_NoUserSiteDirectory': 0,
        'Py_UnbufferedStdioFlag': 0,

        '_disable_importlib': 0,
        'Py_FrozenFlag': 0,
    }

    def check_config(self, testname, expected):
        env = dict(os.environ)
        for key in list(env):
            if key.startswith('PYTHON'):
                del env[key]
        # Disable C locale coercion and UTF-8 mode to not depend
        # on the current locale
        env['PYTHONCOERCECLOCALE'] = '0'
        env['PYTHONUTF8'] = '0'
        out, err = self.run_embedded_interpreter(testname, env=env)
        # Ignore err

        expected = dict(self.DEFAULT_CONFIG, **expected)
        for key, value in expected.items():
            expected[key] = str(value)

        config = {}
        for line in out.splitlines():
            key, value = line.split(' = ', 1)
            config[key] = value
        self.assertEqual(config, expected)

    def test_init_default_config(self):
        self.check_config("init_default_config", {})

    def test_init_global_config(self):
        config = {
            'program_name': './globalvar',
            'Py_NoSiteFlag': 1,
            'Py_BytesWarningFlag': 1,
            'Py_InspectFlag': 1,
            'Py_InteractiveFlag': 1,
            'Py_OptimizeFlag': 2,
            'Py_DontWriteBytecodeFlag': 1,
            'Py_VerboseFlag': 1,
            'Py_QuietFlag': 1,
            'Py_UnbufferedStdioFlag': 1,
            'utf8_mode': 1,
            'Py_NoUserSiteDirectory': 1,
            'Py_FrozenFlag': 1,
        }
        self.check_config("init_global_config", config)

    def test_init_from_config(self):
        config = {
            'install_signal_handlers': 0,
            'use_hash_seed': 1,
            'hash_seed': 123,
            'allocator': 'malloc_debug',
            'tracemalloc': 2,
            'import_time': 1,
            'show_ref_count': 1,
            'show_alloc_count': 1,
            'malloc_stats': 1,

            'utf8_mode': 1,

            'program_name': './conf_program_name',
            'program': 'conf_program',

            'faulthandler': 1,
        }
        self.check_config("init_from_config", config)

    def test_init_env(self):
        config = {
            'use_hash_seed': 1,
            'hash_seed': 42,
            'allocator': 'malloc_debug',
            'tracemalloc': 2,
            'import_time': 1,
            'malloc_stats': 1,
            'utf8_mode': 1,
            'Py_InspectFlag': 1,
            'Py_OptimizeFlag': 2,
            'Py_DontWriteBytecodeFlag': 1,
            'Py_VerboseFlag': 1,
            'Py_UnbufferedStdioFlag': 1,
            'Py_NoUserSiteDirectory': 1,
            'faulthandler': 1,
            'dev_mode': 1,
        }
        self.check_config("init_env", config)

    def test_init_dev_mode(self):
        config = {
            'dev_mode': 1,
            'faulthandler': 1,
            'allocator': 'debug',
        }
        self.check_config("init_dev_mode", config)

    def test_init_isolated(self):
        config = {
            'Py_IsolatedFlag': 1,
            'Py_IgnoreEnvironmentFlag': 1,
            'Py_NoUserSiteDirectory': 1,
        }
        self.check_config("init_isolated", config)


if __name__ == "__main__":
    unittest.main()
