# Run the tests in Programs/_testembed.c (tests for the CPython embedding APIs)
from test import support
import unittest

from collections import namedtuple
import json
import os
import platform
import re
import subprocess
import sys
import textwrap


MS_WINDOWS = (os.name == 'nt')


class EmbeddingTestsMixin:
    def setUp(self):
        here = os.path.abspath(__file__)
        basepath = os.path.dirname(os.path.dirname(os.path.dirname(here)))
        exename = "_testembed"
        if MS_WINDOWS:
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
        if env is not None and MS_WINDOWS:
            # Windows requires at least the SYSTEMROOT environment variable to
            # start Python.
            env = env.copy()
            env['SYSTEMROOT'] = os.environ['SYSTEMROOT']

        p = subprocess.Popen(cmd,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE,
                             universal_newlines=True,
                             env=env)
        try:
            (out, err) = p.communicate()
        except:
            p.terminate()
            p.wait()
            raise
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
        "Expected encoding: iso8859-1",
        "Expected errors: default",
        "stdin: iso8859-1:{errors}",
        "stdout: iso8859-1:{errors}",
        "stderr: iso8859-1:backslashreplace",
        "--- Set encoding and errors ---",
        "Expected encoding: iso8859-1",
        "Expected errors: replace",
        "stdin: iso8859-1:replace",
        "stdout: iso8859-1:replace",
        "stderr: iso8859-1:backslashreplace"])
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
        if MS_WINDOWS:
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

    def test_run_main(self):
        out, err = self.run_embedded_interpreter("run_main")
        self.assertEqual(out.rstrip(), "_Py_RunMain(): sys.argv=['-c', 'arg2']")
        self.assertEqual(err, '')


class InitConfigTests(EmbeddingTestsMixin, unittest.TestCase):
    maxDiff = 4096
    UTF8_MODE_ERRORS = ('surrogatepass' if MS_WINDOWS else 'surrogateescape')

    # core config
    UNTESTED_CORE_CONFIG = (
        # FIXME: untested core configuration variables
        'dll_path',
        'executable',
        'module_search_paths',
    )
    # Mark config which should be get by get_default_config()
    GET_DEFAULT_CONFIG = object()
    DEFAULT_PRE_CONFIG = {
        'allocator': None,
        'coerce_c_locale': 0,
        'coerce_c_locale_warn': 0,
        'utf8_mode': 0,
    }
    COPY_PRE_CONFIG = [
        'dev_mode',
        'isolated',
        'use_environment',
    ]

    DEFAULT_CORE_CONFIG = {
        'isolated': 0,
        'use_environment': 1,
        'dev_mode': 0,

        'install_signal_handlers': 1,
        'use_hash_seed': 0,
        'hash_seed': 0,
        'faulthandler': 0,
        'tracemalloc': 0,
        'import_time': 0,
        'show_ref_count': 0,
        'show_alloc_count': 0,
        'dump_refs': 0,
        'malloc_stats': 0,

        'filesystem_encoding': GET_DEFAULT_CONFIG,
        'filesystem_errors': GET_DEFAULT_CONFIG,

        'pycache_prefix': None,
        'program_name': './_testembed',
        'argv': [""],
        'program': '',

        'xoptions': [],
        'warnoptions': [],

        'module_search_path_env': None,
        'home': None,

        'prefix': GET_DEFAULT_CONFIG,
        'base_prefix': GET_DEFAULT_CONFIG,
        'exec_prefix': GET_DEFAULT_CONFIG,
        'base_exec_prefix': GET_DEFAULT_CONFIG,

        'site_import': 1,
        'bytes_warning': 0,
        'inspect': 0,
        'interactive': 0,
        'optimization_level': 0,
        'parser_debug': 0,
        'write_bytecode': 1,
        'verbose': 0,
        'quiet': 0,
        'user_site_directory': 1,
        'buffered_stdio': 1,

        'stdio_encoding': GET_DEFAULT_CONFIG,
        'stdio_errors': GET_DEFAULT_CONFIG,

        'skip_source_first_line': 0,
        'run_command': None,
        'run_module': None,
        'run_filename': None,

        '_install_importlib': 1,
        'check_hash_pycs_mode': 'default',
        '_frozen': 0,
    }
    if MS_WINDOWS:
        DEFAULT_PRE_CONFIG.update({
            'legacy_windows_fs_encoding': 0,
        })
        DEFAULT_CORE_CONFIG.update({
            'legacy_windows_stdio': 0,
        })

    # global config
    DEFAULT_GLOBAL_CONFIG = {
        'Py_HasFileSystemDefaultEncoding': 0,
        'Py_HashRandomizationFlag': 1,
        '_Py_HasFileSystemDefaultEncodeErrors': 0,
    }
    COPY_GLOBAL_PRE_CONFIG = [
        ('Py_UTF8Mode', 'utf8_mode'),
    ]
    COPY_GLOBAL_CONFIG = [
        # Copy core config to global config for expected values
        # True means that the core config value is inverted (0 => 1 and 1 => 0)
        ('Py_BytesWarningFlag', 'bytes_warning'),
        ('Py_DebugFlag', 'parser_debug'),
        ('Py_DontWriteBytecodeFlag', 'write_bytecode', True),
        ('Py_FileSystemDefaultEncodeErrors', 'filesystem_errors'),
        ('Py_FileSystemDefaultEncoding', 'filesystem_encoding'),
        ('Py_FrozenFlag', '_frozen'),
        ('Py_IgnoreEnvironmentFlag', 'use_environment', True),
        ('Py_InspectFlag', 'inspect'),
        ('Py_InteractiveFlag', 'interactive'),
        ('Py_IsolatedFlag', 'isolated'),
        ('Py_NoSiteFlag', 'site_import', True),
        ('Py_NoUserSiteDirectory', 'user_site_directory', True),
        ('Py_OptimizeFlag', 'optimization_level'),
        ('Py_QuietFlag', 'quiet'),
        ('Py_UnbufferedStdioFlag', 'buffered_stdio', True),
        ('Py_VerboseFlag', 'verbose'),
    ]
    if MS_WINDOWS:
        COPY_GLOBAL_PRE_CONFIG.extend((
            ('Py_LegacyWindowsFSEncodingFlag', 'legacy_windows_fs_encoding'),
        ))
        COPY_GLOBAL_CONFIG.extend((
            ('Py_LegacyWindowsStdioFlag', 'legacy_windows_stdio'),
        ))

    def main_xoptions(self, xoptions_list):
        xoptions = {}
        for opt in xoptions_list:
            if '=' in opt:
                key, value = opt.split('=', 1)
                xoptions[key] = value
            else:
                xoptions[opt] = True
        return xoptions

    def get_expected_config(self, expected, env):
        expected = dict(self.DEFAULT_CORE_CONFIG, **expected)

        code = textwrap.dedent('''
            import json
            import sys

            data = {
                'stdio_encoding': sys.stdout.encoding,
                'stdio_errors': sys.stdout.errors,
                'prefix': sys.prefix,
                'base_prefix': sys.base_prefix,
                'exec_prefix': sys.exec_prefix,
                'base_exec_prefix': sys.base_exec_prefix,
                'filesystem_encoding': sys.getfilesystemencoding(),
                'filesystem_errors': sys.getfilesystemencodeerrors(),
            }

            data = json.dumps(data)
            data = data.encode('utf-8')
            sys.stdout.buffer.write(data)
            sys.stdout.buffer.flush()
        ''')

        # Use -S to not import the site module: get the proper configuration
        # when test_embed is run from a venv (bpo-35313)
        args = (sys.executable, '-S', '-c', code)
        env = dict(env)
        if not expected['isolated']:
            env['PYTHONCOERCECLOCALE'] = '0'
            env['PYTHONUTF8'] = '0'
        proc = subprocess.run(args, env=env,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT)
        if proc.returncode:
            raise Exception(f"failed to get the default config: "
                            f"stdout={proc.stdout!r} stderr={proc.stderr!r}")
        stdout = proc.stdout.decode('utf-8')
        try:
            config = json.loads(stdout)
        except json.JSONDecodeError:
            self.fail(f"fail to decode stdout: {stdout!r}")

        for key, value in expected.items():
            if value is self.GET_DEFAULT_CONFIG:
                expected[key] = config[key]
        return expected

    def check_pre_config(self, config, expected):
        pre_config = dict(config['pre_config'])
        core_config = dict(config['core_config'])
        self.assertEqual(pre_config, expected)

    def check_core_config(self, config, expected):
        core_config = dict(config['core_config'])
        for key in self.UNTESTED_CORE_CONFIG:
            core_config.pop(key, None)
        self.assertEqual(core_config, expected)

    def check_global_config(self, config):
        pre_config = config['pre_config']
        core_config = config['core_config']

        expected = dict(self.DEFAULT_GLOBAL_CONFIG)
        for item in self.COPY_GLOBAL_CONFIG:
            if len(item) == 3:
                global_key, core_key, opposite = item
                expected[global_key] = 0 if core_config[core_key] else 1
            else:
                global_key, core_key = item
                expected[global_key] = core_config[core_key]
        for item in self.COPY_GLOBAL_PRE_CONFIG:
            if len(item) == 3:
                global_key, core_key, opposite = item
                expected[global_key] = 0 if pre_config[core_key] else 1
            else:
                global_key, core_key = item
                expected[global_key] = pre_config[core_key]

        self.assertEqual(config['global_config'], expected)

    def check_config(self, testname, expected_config, expected_preconfig):
        env = dict(os.environ)
        # Remove PYTHON* environment variables to get deterministic environment
        for key in list(env):
            if key.startswith('PYTHON'):
                del env[key]
        # Disable C locale coercion and UTF-8 mode to not depend
        # on the current locale
        env['PYTHONCOERCECLOCALE'] = '0'
        env['PYTHONUTF8'] = '0'

        out, err = self.run_embedded_interpreter(testname, env=env)
        # Ignore err
        try:
            config = json.loads(out)
        except json.JSONDecodeError:
            self.fail(f"fail to decode stdout: {out!r}")

        expected_preconfig = dict(self.DEFAULT_PRE_CONFIG, **expected_preconfig)
        expected_config = self.get_expected_config(expected_config, env)
        for key in self.COPY_PRE_CONFIG:
            if key not in expected_preconfig:
                expected_preconfig[key] = expected_config[key]

        self.check_pre_config(config, expected_preconfig)
        self.check_core_config(config, expected_config)
        self.check_global_config(config)

    def test_init_default_config(self):
        self.check_config("init_default_config", {}, {})

    def test_init_global_config(self):
        preconfig = {
            'utf8_mode': 1,
        }
        config = {
            'program_name': './globalvar',
            'site_import': 0,
            'bytes_warning': 1,
            'warnoptions': ['default::BytesWarning'],
            'inspect': 1,
            'interactive': 1,
            'optimization_level': 2,
            'write_bytecode': 0,
            'verbose': 1,
            'quiet': 1,
            'buffered_stdio': 0,

            'stdio_encoding': 'utf-8',
            'stdio_errors': 'surrogateescape',
            'filesystem_encoding': 'utf-8',
            'filesystem_errors': self.UTF8_MODE_ERRORS,
            'user_site_directory': 0,
        }
        self.check_config("init_global_config", config, preconfig)

    def test_init_from_config(self):
        preconfig = {
            'allocator': 'malloc',
            'utf8_mode': 1,
        }
        config = {
            'install_signal_handlers': 0,
            'use_hash_seed': 1,
            'hash_seed': 123,
            'tracemalloc': 2,
            'import_time': 1,
            'show_ref_count': 1,
            'show_alloc_count': 1,
            'malloc_stats': 1,

            'stdio_encoding': 'iso8859-1',
            'stdio_errors': 'replace',
            'filesystem_encoding': 'utf-8',
            'filesystem_errors': self.UTF8_MODE_ERRORS,

            'pycache_prefix': 'conf_pycache_prefix',
            'program_name': './conf_program_name',
            'argv': ['-c', 'arg2'],
            'program': 'conf_program',
            'xoptions': ['core_xoption1=3', 'core_xoption2=', 'core_xoption3'],
            'warnoptions': ['error::ResourceWarning', 'default::BytesWarning'],
            'run_command': 'pass\n',

            'site_import': 0,
            'bytes_warning': 1,
            'inspect': 1,
            'interactive': 1,
            'optimization_level': 2,
            'write_bytecode': 0,
            'verbose': 1,
            'quiet': 1,
            'buffered_stdio': 0,
            'user_site_directory': 0,
            'faulthandler': 1,

            'check_hash_pycs_mode': 'always',
        }
        self.check_config("init_from_config", config, preconfig)

    INIT_ENV_PRECONFIG = {
        'allocator': 'malloc',
    }
    INIT_ENV_CONFIG = {
        'use_hash_seed': 1,
        'hash_seed': 42,
        'tracemalloc': 2,
        'import_time': 1,
        'malloc_stats': 1,
        'inspect': 1,
        'optimization_level': 2,
        'pycache_prefix': 'env_pycache_prefix',
        'write_bytecode': 0,
        'verbose': 1,
        'buffered_stdio': 0,
        'stdio_encoding': 'iso8859-1',
        'stdio_errors': 'replace',
        'user_site_directory': 0,
        'faulthandler': 1,
    }

    def test_init_env(self):
        self.check_config("init_env", self.INIT_ENV_CONFIG, self.INIT_ENV_PRECONFIG)

    def test_init_env_dev_mode(self):
        preconfig = dict(self.INIT_ENV_PRECONFIG,
                      allocator='debug')
        config = dict(self.INIT_ENV_CONFIG,
                      dev_mode=1,
                      warnoptions=['default'])
        self.check_config("init_env_dev_mode", config, preconfig)

    def test_init_env_dev_mode_alloc(self):
        preconfig = dict(self.INIT_ENV_PRECONFIG,
                         allocator='malloc')
        config = dict(self.INIT_ENV_CONFIG,
                      dev_mode=1,
                      warnoptions=['default'])
        self.check_config("init_env_dev_mode_alloc", config, preconfig)

    def test_init_dev_mode(self):
        preconfig = {
            'allocator': 'debug',
        }
        config = {
            'faulthandler': 1,
            'dev_mode': 1,
            'warnoptions': ['default'],
        }
        self.check_config("init_dev_mode", config, preconfig)

    def test_init_isolated(self):
        preconfig = {}
        config = {
            'isolated': 1,
            'use_environment': 0,
            'user_site_directory': 0,
        }
        self.check_config("init_isolated", config, preconfig)

    def test_preinit_isolated1(self):
        # _PyPreConfig.isolated=1, _PyCoreConfig.isolated not set
        preconfig = {}
        config = {
            'isolated': 1,
            'use_environment': 0,
            'user_site_directory': 0,
        }
        self.check_config("preinit_isolated1", config, preconfig)

    def test_preinit_isolated2(self):
        # _PyPreConfig.isolated=0, _PyCoreConfig.isolated=1
        preconfig = {}
        config = {
            'isolated': 1,
            'use_environment': 0,
            'user_site_directory': 0,
        }
        self.check_config("preinit_isolated2", config, preconfig)


if __name__ == "__main__":
    unittest.main()
