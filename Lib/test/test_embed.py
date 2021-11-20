# Run the tests in Programs/_testembed.c (tests for the CPython embedding APIs)
from test import support
from test.support import import_helper
from test.support import os_helper
import unittest

from collections import namedtuple
import contextlib
import json
import os
import os.path
import re
import shutil
import subprocess
import sys
import tempfile
import textwrap


MS_WINDOWS = (os.name == 'nt')
MACOS = (sys.platform == 'darwin')

PYMEM_ALLOCATOR_NOT_SET = 0
PYMEM_ALLOCATOR_DEBUG = 2
PYMEM_ALLOCATOR_MALLOC = 3

# _PyCoreConfig_InitCompatConfig()
API_COMPAT = 1
# _PyCoreConfig_InitPythonConfig()
API_PYTHON = 2
# _PyCoreConfig_InitIsolatedConfig()
API_ISOLATED = 3

INIT_LOOPS = 16
MAX_HASH_SEED = 4294967295


# If we are running from a build dir, but the stdlib has been installed,
# some tests need to expect different results.
STDLIB_INSTALL = os.path.join(sys.prefix, sys.platlibdir,
    f'python{sys.version_info.major}.{sys.version_info.minor}')
if not os.path.isfile(os.path.join(STDLIB_INSTALL, 'os.py')):
    STDLIB_INSTALL = None

def debug_build(program):
    program = os.path.basename(program)
    name = os.path.splitext(program)[0]
    return name.casefold().endswith("_d".casefold())


def remove_python_envvars():
    env = dict(os.environ)
    # Remove PYTHON* environment variables to get deterministic environment
    for key in list(env):
        if key.startswith('PYTHON'):
            del env[key]
    return env


class EmbeddingTestsMixin:
    def setUp(self):
        exename = "_testembed"
        builddir = os.path.dirname(sys.executable)
        if MS_WINDOWS:
            ext = ("_d" if debug_build(sys.executable) else "") + ".exe"
            exename += ext
            exepath = builddir
            expecteddir = support.REPO_ROOT
        else:
            exepath = os.path.join(builddir, 'Programs')
            expecteddir = os.path.join(support.REPO_ROOT, 'Programs')
        self.test_exe = exe = os.path.join(exepath, exename)
        if exepath != expecteddir or not os.path.exists(exe):
            self.skipTest("%r doesn't exist" % exe)
        # This is needed otherwise we get a fatal error:
        # "Py_Initialize: Unable to get the locale encoding
        # LookupError: no codec search functions registered: can't find encoding"
        self.oldcwd = os.getcwd()
        os.chdir(support.REPO_ROOT)

    def tearDown(self):
        os.chdir(self.oldcwd)

    def run_embedded_interpreter(self, *args, env=None,
                                 timeout=None, returncode=0, input=None,
                                 cwd=None):
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
                             env=env,
                             cwd=cwd)
        try:
            (out, err) = p.communicate(input=input, timeout=timeout)
        except:
            p.terminate()
            p.wait()
            raise
        if p.returncode != returncode and support.verbose:
            print(f"--- {cmd} failed ---")
            print(f"stdout:\n{out}")
            print(f"stderr:\n{err}")
            print(f"------")

        self.assertEqual(p.returncode, returncode,
                         "bad returncode %d, stderr is %r" %
                         (p.returncode, err))
        return out, err

    def run_repeated_init_and_subinterpreters(self):
        out, err = self.run_embedded_interpreter("test_repeated_init_and_subinterpreters")
        self.assertEqual(err, "")

        # The output from _testembed looks like this:
        # --- Pass 1 ---
        # interp 0 <0x1cf9330>, thread state <0x1cf9700>: id(modules) = 139650431942728
        # interp 1 <0x1d4f690>, thread state <0x1d35350>: id(modules) = 139650431165784
        # interp 2 <0x1d5a690>, thread state <0x1d99ed0>: id(modules) = 139650413140368
        # interp 3 <0x1d4f690>, thread state <0x1dc3340>: id(modules) = 139650412862200
        # interp 0 <0x1cf9330>, thread state <0x1cf9700>: id(modules) = 139650431942728
        # --- Pass 2 ---
        # ...

        interp_pat = (r"^interp (\d+) <(0x[\dA-F]+)>, "
                      r"thread state <(0x[\dA-F]+)>: "
                      r"id\(modules\) = ([\d]+)$")
        Interp = namedtuple("Interp", "id interp tstate modules")

        numloops = 1
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
    maxDiff = 100 * 50

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

    def test_repeated_init_and_inittab(self):
        out, err = self.run_embedded_interpreter("test_repeated_init_and_inittab")
        self.assertEqual(err, "")

        lines = [f"--- Pass {i} ---" for i in range(1, INIT_LOOPS+1)]
        lines = "\n".join(lines) + "\n"
        self.assertEqual(out, lines)

    def test_forced_io_encoding(self):
        # Checks forced configuration of embedded interpreter IO streams
        env = dict(os.environ, PYTHONIOENCODING="utf-8:surrogateescape")
        out, err = self.run_embedded_interpreter("test_forced_io_encoding", env=env)
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
        Checks some key parts of the C-API that need to work before the runtime
        is initialized (via Py_Initialize()).
        """
        env = dict(os.environ, PYTHONPATH=os.pathsep.join(sys.path))
        out, err = self.run_embedded_interpreter("test_pre_initialization_api", env=env)
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
        env = remove_python_envvars()
        env['PYTHONPATH'] = os.pathsep.join(sys.path)
        out, err = self.run_embedded_interpreter(
                        "test_pre_initialization_sys_options", env=env)
        expected_output = (
            "sys.warnoptions: ['once', 'module', 'default']\n"
            "sys._xoptions: {'dev': '2', 'utf8': '1'}\n"
            "warnings.filters[:3]: ['default', 'module', 'once']\n"
        )
        self.assertIn(expected_output, out)
        self.assertEqual(err, '')

    def test_bpo20891(self):
        """
        bpo-20891: Calling PyGILState_Ensure in a non-Python thread must not
        crash.
        """
        out, err = self.run_embedded_interpreter("test_bpo20891")
        self.assertEqual(out, '')
        self.assertEqual(err, '')

    def test_initialize_twice(self):
        """
        bpo-33932: Calling Py_Initialize() twice should do nothing (and not
        crash!).
        """
        out, err = self.run_embedded_interpreter("test_initialize_twice")
        self.assertEqual(out, '')
        self.assertEqual(err, '')

    def test_initialize_pymain(self):
        """
        bpo-34008: Calling Py_Main() after Py_Initialize() must not fail.
        """
        out, err = self.run_embedded_interpreter("test_initialize_pymain")
        self.assertEqual(out.rstrip(), "Py_Main() after Py_Initialize: sys.argv=['-c', 'arg2']")
        self.assertEqual(err, '')

    def test_run_main(self):
        out, err = self.run_embedded_interpreter("test_run_main")
        self.assertEqual(out.rstrip(), "Py_RunMain(): sys.argv=['-c', 'arg2']")
        self.assertEqual(err, '')

    def test_run_main_loop(self):
        # bpo-40413: Calling Py_InitializeFromConfig()+Py_RunMain() multiple
        # times must not crash.
        nloop = 5
        out, err = self.run_embedded_interpreter("test_run_main_loop")
        self.assertEqual(out, "Py_RunMain(): sys.argv=['-c', 'arg2']\n" * nloop)
        self.assertEqual(err, '')


class InitConfigTests(EmbeddingTestsMixin, unittest.TestCase):
    maxDiff = 4096
    UTF8_MODE_ERRORS = ('surrogatepass' if MS_WINDOWS else 'surrogateescape')

    # Marker to read the default configuration: get_default_config()
    GET_DEFAULT_CONFIG = object()

    # Marker to ignore a configuration parameter
    IGNORE_CONFIG = object()

    PRE_CONFIG_COMPAT = {
        '_config_init': API_COMPAT,
        'allocator': PYMEM_ALLOCATOR_NOT_SET,
        'parse_argv': 0,
        'configure_locale': 1,
        'coerce_c_locale': 0,
        'coerce_c_locale_warn': 0,
        'utf8_mode': 0,
    }
    if MS_WINDOWS:
        PRE_CONFIG_COMPAT.update({
            'legacy_windows_fs_encoding': 0,
        })
    PRE_CONFIG_PYTHON = dict(PRE_CONFIG_COMPAT,
        _config_init=API_PYTHON,
        parse_argv=1,
        coerce_c_locale=GET_DEFAULT_CONFIG,
        utf8_mode=GET_DEFAULT_CONFIG,
    )
    PRE_CONFIG_ISOLATED = dict(PRE_CONFIG_COMPAT,
        _config_init=API_ISOLATED,
        configure_locale=0,
        isolated=1,
        use_environment=0,
        utf8_mode=0,
        dev_mode=0,
        coerce_c_locale=0,
    )

    COPY_PRE_CONFIG = [
        'dev_mode',
        'isolated',
        'use_environment',
    ]

    CONFIG_COMPAT = {
        '_config_init': API_COMPAT,
        'isolated': 0,
        'use_environment': 1,
        'dev_mode': 0,

        'install_signal_handlers': 1,
        'use_hash_seed': 0,
        'hash_seed': 0,
        'faulthandler': 0,
        'tracemalloc': 0,
        'import_time': 0,
        'no_debug_ranges': 0,
        'show_ref_count': 0,
        'dump_refs': 0,
        'malloc_stats': 0,

        'filesystem_encoding': GET_DEFAULT_CONFIG,
        'filesystem_errors': GET_DEFAULT_CONFIG,

        'pycache_prefix': None,
        'program_name': GET_DEFAULT_CONFIG,
        'parse_argv': 0,
        'argv': [""],
        'orig_argv': [],

        'xoptions': [],
        'warnoptions': [],

        'pythonpath_env': None,
        'home': None,
        'executable': GET_DEFAULT_CONFIG,
        'base_executable': GET_DEFAULT_CONFIG,

        'prefix': GET_DEFAULT_CONFIG,
        'base_prefix': GET_DEFAULT_CONFIG,
        'exec_prefix': GET_DEFAULT_CONFIG,
        'base_exec_prefix': GET_DEFAULT_CONFIG,
        'module_search_paths': GET_DEFAULT_CONFIG,
        'module_search_paths_set': 1,
        'platlibdir': sys.platlibdir,
        'stdlib_dir': GET_DEFAULT_CONFIG,

        'site_import': 1,
        'bytes_warning': 0,
        'warn_default_encoding': 0,
        'inspect': 0,
        'interactive': 0,
        'optimization_level': 0,
        'parser_debug': 0,
        'write_bytecode': 1,
        'verbose': 0,
        'quiet': 0,
        'user_site_directory': 1,
        'configure_c_stdio': 0,
        'buffered_stdio': 1,

        'stdio_encoding': GET_DEFAULT_CONFIG,
        'stdio_errors': GET_DEFAULT_CONFIG,

        'skip_source_first_line': 0,
        'run_command': None,
        'run_module': None,
        'run_filename': None,

        '_install_importlib': 1,
        'check_hash_pycs_mode': 'default',
        'pathconfig_warnings': 1,
        '_init_main': 1,
        '_isolated_interpreter': 0,
        'use_frozen_modules': 1,
    }
    if MS_WINDOWS:
        CONFIG_COMPAT.update({
            'legacy_windows_stdio': 0,
        })

    CONFIG_PYTHON = dict(CONFIG_COMPAT,
        _config_init=API_PYTHON,
        configure_c_stdio=1,
        parse_argv=2,
    )
    CONFIG_ISOLATED = dict(CONFIG_COMPAT,
        _config_init=API_ISOLATED,
        isolated=1,
        use_environment=0,
        user_site_directory=0,
        dev_mode=0,
        install_signal_handlers=0,
        use_hash_seed=0,
        faulthandler=0,
        tracemalloc=0,
        pathconfig_warnings=0,
    )
    if MS_WINDOWS:
        CONFIG_ISOLATED['legacy_windows_stdio'] = 0

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
        ('Py_FrozenFlag', 'pathconfig_warnings', True),
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

    # path config
    if MS_WINDOWS:
        PATH_CONFIG = {
            'isolated': -1,
            'site_import': -1,
            'python3_dll': GET_DEFAULT_CONFIG,
        }
    else:
        PATH_CONFIG = {}
    # other keys are copied by COPY_PATH_CONFIG

    COPY_PATH_CONFIG = [
        # Copy core config to global config for expected values
        'prefix',
        'exec_prefix',
        'program_name',
        'home',
        'stdlib_dir',
        # program_full_path and module_search_path are copied indirectly from
        # the core configuration in check_path_config().
    ]
    if MS_WINDOWS:
        COPY_PATH_CONFIG.extend((
            'base_executable',
        ))

    EXPECTED_CONFIG = None

    @classmethod
    def tearDownClass(cls):
        # clear cache
        cls.EXPECTED_CONFIG = None

    def main_xoptions(self, xoptions_list):
        xoptions = {}
        for opt in xoptions_list:
            if '=' in opt:
                key, value = opt.split('=', 1)
                xoptions[key] = value
            else:
                xoptions[opt] = True
        return xoptions

    def _get_expected_config_impl(self):
        env = remove_python_envvars()
        code = textwrap.dedent('''
            import json
            import sys
            import _testinternalcapi

            configs = _testinternalcapi.get_configs()

            data = json.dumps(configs)
            data = data.encode('utf-8')
            sys.stdout.buffer.write(data)
            sys.stdout.buffer.flush()
        ''')

        # Use -S to not import the site module: get the proper configuration
        # when test_embed is run from a venv (bpo-35313)
        args = [sys.executable, '-S', '-c', code]
        proc = subprocess.run(args, env=env,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)
        if proc.returncode:
            raise Exception(f"failed to get the default config: "
                            f"stdout={proc.stdout!r} stderr={proc.stderr!r}")
        stdout = proc.stdout.decode('utf-8')
        # ignore stderr
        try:
            return json.loads(stdout)
        except json.JSONDecodeError:
            self.fail(f"fail to decode stdout: {stdout!r}")

    def _get_expected_config(self):
        cls = InitConfigTests
        if cls.EXPECTED_CONFIG is None:
            cls.EXPECTED_CONFIG = self._get_expected_config_impl()

        # get a copy
        configs = {}
        for config_key, config_value in cls.EXPECTED_CONFIG.items():
            config = {}
            for key, value in config_value.items():
                if isinstance(value, list):
                    value = value.copy()
                config[key] = value
            configs[config_key] = config
        return configs

    def get_expected_config(self, expected_preconfig, expected,
                            expected_pathconfig, env, api,
                            modify_path_cb=None):
        configs = self._get_expected_config()

        pre_config = configs['pre_config']
        for key, value in expected_preconfig.items():
            if value is self.GET_DEFAULT_CONFIG:
                expected_preconfig[key] = pre_config[key]

        path_config = configs['path_config']
        for key, value in expected_pathconfig.items():
            if value is self.GET_DEFAULT_CONFIG:
                expected_pathconfig[key] = path_config[key]

        if not expected_preconfig['configure_locale'] or api == API_COMPAT:
            # there is no easy way to get the locale encoding before
            # setlocale(LC_CTYPE, "") is called: don't test encodings
            for key in ('filesystem_encoding', 'filesystem_errors',
                        'stdio_encoding', 'stdio_errors'):
                expected[key] = self.IGNORE_CONFIG

        if not expected_preconfig['configure_locale']:
            # UTF-8 Mode depends on the locale. There is no easy way
            # to guess if UTF-8 Mode will be enabled or not if the locale
            # is not configured.
            expected_preconfig['utf8_mode'] = self.IGNORE_CONFIG

        if expected_preconfig['utf8_mode'] == 1:
            if expected['filesystem_encoding'] is self.GET_DEFAULT_CONFIG:
                expected['filesystem_encoding'] = 'utf-8'
            if expected['filesystem_errors'] is self.GET_DEFAULT_CONFIG:
                expected['filesystem_errors'] = self.UTF8_MODE_ERRORS
            if expected['stdio_encoding'] is self.GET_DEFAULT_CONFIG:
                expected['stdio_encoding'] = 'utf-8'
            if expected['stdio_errors'] is self.GET_DEFAULT_CONFIG:
                expected['stdio_errors'] = 'surrogateescape'

        if MS_WINDOWS:
            default_executable = self.test_exe
        elif expected['program_name'] is not self.GET_DEFAULT_CONFIG:
            default_executable = os.path.abspath(expected['program_name'])
        else:
            default_executable = os.path.join(os.getcwd(), '_testembed')
        if expected['executable'] is self.GET_DEFAULT_CONFIG:
            expected['executable'] = default_executable
        if expected['base_executable'] is self.GET_DEFAULT_CONFIG:
            expected['base_executable'] = default_executable
        if expected['program_name'] is self.GET_DEFAULT_CONFIG:
            expected['program_name'] = './_testembed'

        config = configs['config']
        for key, value in expected.items():
            if value is self.GET_DEFAULT_CONFIG:
                expected[key] = config[key]

        if expected['module_search_paths'] is not self.IGNORE_CONFIG:
            pythonpath_env = expected['pythonpath_env']
            if pythonpath_env is not None:
                paths = pythonpath_env.split(os.path.pathsep)
                expected['module_search_paths'] = [*paths, *expected['module_search_paths']]
            if modify_path_cb is not None:
                expected['module_search_paths'] = expected['module_search_paths'].copy()
                modify_path_cb(expected['module_search_paths'])

        for key in self.COPY_PRE_CONFIG:
            if key not in expected_preconfig:
                expected_preconfig[key] = expected[key]

    def check_pre_config(self, configs, expected):
        pre_config = dict(configs['pre_config'])
        for key, value in list(expected.items()):
            if value is self.IGNORE_CONFIG:
                pre_config.pop(key, None)
                del expected[key]
        self.assertEqual(pre_config, expected)

    def check_config(self, configs, expected):
        config = dict(configs['config'])
        for key, value in list(expected.items()):
            if value is self.IGNORE_CONFIG:
                config.pop(key, None)
                del expected[key]
        self.assertEqual(config, expected)

    def check_global_config(self, configs):
        pre_config = configs['pre_config']
        config = configs['config']

        expected = dict(self.DEFAULT_GLOBAL_CONFIG)
        for item in self.COPY_GLOBAL_CONFIG:
            if len(item) == 3:
                global_key, core_key, opposite = item
                expected[global_key] = 0 if config[core_key] else 1
            else:
                global_key, core_key = item
                expected[global_key] = config[core_key]
        for item in self.COPY_GLOBAL_PRE_CONFIG:
            if len(item) == 3:
                global_key, core_key, opposite = item
                expected[global_key] = 0 if pre_config[core_key] else 1
            else:
                global_key, core_key = item
                expected[global_key] = pre_config[core_key]

        self.assertEqual(configs['global_config'], expected)

    def check_path_config(self, configs, expected):
        config = configs['config']

        for key in self.COPY_PATH_CONFIG:
            expected[key] = config[key]
        expected['module_search_path'] = os.path.pathsep.join(config['module_search_paths'])
        expected['program_full_path'] = config['executable']

        self.assertEqual(configs['path_config'], expected)

    def check_all_configs(self, testname, expected_config=None,
                          expected_preconfig=None, expected_pathconfig=None,
                          modify_path_cb=None,
                          stderr=None, *, api, preconfig_api=None,
                          env=None, ignore_stderr=False, cwd=None):
        new_env = remove_python_envvars()
        if env is not None:
            new_env.update(env)
        env = new_env

        if preconfig_api is None:
            preconfig_api = api
        if preconfig_api == API_ISOLATED:
            default_preconfig = self.PRE_CONFIG_ISOLATED
        elif preconfig_api == API_PYTHON:
            default_preconfig = self.PRE_CONFIG_PYTHON
        else:
            default_preconfig = self.PRE_CONFIG_COMPAT
        if expected_preconfig is None:
            expected_preconfig = {}
        expected_preconfig = dict(default_preconfig, **expected_preconfig)

        if expected_config is None:
            expected_config = {}

        if expected_pathconfig is None:
            expected_pathconfig = {}
        expected_pathconfig = dict(self.PATH_CONFIG, **expected_pathconfig)

        if api == API_PYTHON:
            default_config = self.CONFIG_PYTHON
        elif api == API_ISOLATED:
            default_config = self.CONFIG_ISOLATED
        else:
            default_config = self.CONFIG_COMPAT
        expected_config = dict(default_config, **expected_config)

        self.get_expected_config(expected_preconfig,
                                 expected_config,
                                 expected_pathconfig,
                                 env,
                                 api, modify_path_cb)

        out, err = self.run_embedded_interpreter(testname,
                                                 env=env, cwd=cwd)
        if stderr is None and not expected_config['verbose']:
            stderr = ""
        if stderr is not None and not ignore_stderr:
            self.assertEqual(err.rstrip(), stderr)
        try:
            configs = json.loads(out)
        except json.JSONDecodeError:
            self.fail(f"fail to decode stdout: {out!r}")

        self.check_pre_config(configs, expected_preconfig)
        self.check_config(configs, expected_config)
        self.check_global_config(configs)
        self.check_path_config(configs, expected_pathconfig)
        return configs

    def test_init_default_config(self):
        self.check_all_configs("test_init_initialize_config", api=API_COMPAT)

    def test_preinit_compat_config(self):
        self.check_all_configs("test_preinit_compat_config", api=API_COMPAT)

    def test_init_compat_config(self):
        self.check_all_configs("test_init_compat_config", api=API_COMPAT)

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

            'user_site_directory': 0,
            'pathconfig_warnings': 0,
        }
        self.check_all_configs("test_init_global_config", config, preconfig,
                               api=API_COMPAT)

    def test_init_from_config(self):
        preconfig = {
            'allocator': PYMEM_ALLOCATOR_MALLOC,
            'utf8_mode': 1,
        }
        config = {
            'install_signal_handlers': 0,
            'use_hash_seed': 1,
            'hash_seed': 123,
            'tracemalloc': 2,
            'import_time': 1,
            'no_debug_ranges': 1,
            'show_ref_count': 1,
            'malloc_stats': 1,

            'stdio_encoding': 'iso8859-1',
            'stdio_errors': 'replace',

            'pycache_prefix': 'conf_pycache_prefix',
            'program_name': './conf_program_name',
            'argv': ['-c', 'arg2'],
            'orig_argv': ['python3',
                          '-W', 'cmdline_warnoption',
                          '-X', 'dev',
                          '-c', 'pass',
                          'arg2'],
            'parse_argv': 2,
            'xoptions': [
                'dev=3',
                'utf8',
                'dev',
            ],
            'warnoptions': [
                'cmdline_warnoption',
                'default::BytesWarning',
                'config_warnoption',
            ],
            'run_command': 'pass\n',

            'site_import': 0,
            'bytes_warning': 1,
            'inspect': 1,
            'interactive': 1,
            'optimization_level': 2,
            'write_bytecode': 0,
            'verbose': 1,
            'quiet': 1,
            'configure_c_stdio': 1,
            'buffered_stdio': 0,
            'user_site_directory': 0,
            'faulthandler': 1,
            'platlibdir': 'my_platlibdir',
            'module_search_paths': self.IGNORE_CONFIG,

            'check_hash_pycs_mode': 'always',
            'pathconfig_warnings': 0,

            '_isolated_interpreter': 1,
        }
        self.check_all_configs("test_init_from_config", config, preconfig,
                               api=API_COMPAT)

    def test_init_compat_env(self):
        preconfig = {
            'allocator': PYMEM_ALLOCATOR_MALLOC,
        }
        config = {
            'use_hash_seed': 1,
            'hash_seed': 42,
            'tracemalloc': 2,
            'import_time': 1,
            'no_debug_ranges': 1,
            'malloc_stats': 1,
            'inspect': 1,
            'optimization_level': 2,
            'pythonpath_env': '/my/path',
            'pycache_prefix': 'env_pycache_prefix',
            'write_bytecode': 0,
            'verbose': 1,
            'buffered_stdio': 0,
            'stdio_encoding': 'iso8859-1',
            'stdio_errors': 'replace',
            'user_site_directory': 0,
            'faulthandler': 1,
            'warnoptions': ['EnvVar'],
            'platlibdir': 'env_platlibdir',
            'module_search_paths': self.IGNORE_CONFIG,
        }
        self.check_all_configs("test_init_compat_env", config, preconfig,
                               api=API_COMPAT)

    def test_init_python_env(self):
        preconfig = {
            'allocator': PYMEM_ALLOCATOR_MALLOC,
            'utf8_mode': 1,
        }
        config = {
            'use_hash_seed': 1,
            'hash_seed': 42,
            'tracemalloc': 2,
            'import_time': 1,
            'no_debug_ranges': 1,
            'malloc_stats': 1,
            'inspect': 1,
            'optimization_level': 2,
            'pythonpath_env': '/my/path',
            'pycache_prefix': 'env_pycache_prefix',
            'write_bytecode': 0,
            'verbose': 1,
            'buffered_stdio': 0,
            'stdio_encoding': 'iso8859-1',
            'stdio_errors': 'replace',
            'user_site_directory': 0,
            'faulthandler': 1,
            'warnoptions': ['EnvVar'],
            'platlibdir': 'env_platlibdir',
            'module_search_paths': self.IGNORE_CONFIG,
        }
        self.check_all_configs("test_init_python_env", config, preconfig,
                               api=API_PYTHON)

    def test_init_env_dev_mode(self):
        preconfig = dict(allocator=PYMEM_ALLOCATOR_DEBUG)
        config = dict(dev_mode=1,
                      faulthandler=1,
                      warnoptions=['default'])
        self.check_all_configs("test_init_env_dev_mode", config, preconfig,
                               api=API_COMPAT)

    def test_init_env_dev_mode_alloc(self):
        preconfig = dict(allocator=PYMEM_ALLOCATOR_MALLOC)
        config = dict(dev_mode=1,
                      faulthandler=1,
                      warnoptions=['default'])
        self.check_all_configs("test_init_env_dev_mode_alloc", config, preconfig,
                               api=API_COMPAT)

    def test_init_dev_mode(self):
        preconfig = {
            'allocator': PYMEM_ALLOCATOR_DEBUG,
        }
        config = {
            'faulthandler': 1,
            'dev_mode': 1,
            'warnoptions': ['default'],
        }
        self.check_all_configs("test_init_dev_mode", config, preconfig,
                               api=API_PYTHON)

    def test_preinit_parse_argv(self):
        # Pre-initialize implicitly using argv: make sure that -X dev
        # is used to configure the allocation in preinitialization
        preconfig = {
            'allocator': PYMEM_ALLOCATOR_DEBUG,
        }
        config = {
            'argv': ['script.py'],
            'orig_argv': ['python3', '-X', 'dev', 'script.py'],
            'run_filename': os.path.abspath('script.py'),
            'dev_mode': 1,
            'faulthandler': 1,
            'warnoptions': ['default'],
            'xoptions': ['dev'],
        }
        self.check_all_configs("test_preinit_parse_argv", config, preconfig,
                               api=API_PYTHON)

    def test_preinit_dont_parse_argv(self):
        # -X dev must be ignored by isolated preconfiguration
        preconfig = {
            'isolated': 0,
        }
        argv = ["python3",
               "-E", "-I",
               "-X", "dev",
               "-X", "utf8",
               "script.py"]
        config = {
            'argv': argv,
            'orig_argv': argv,
            'isolated': 0,
        }
        self.check_all_configs("test_preinit_dont_parse_argv", config, preconfig,
                               api=API_ISOLATED)

    def test_init_isolated_flag(self):
        config = {
            'isolated': 1,
            'use_environment': 0,
            'user_site_directory': 0,
        }
        self.check_all_configs("test_init_isolated_flag", config, api=API_PYTHON)

    def test_preinit_isolated1(self):
        # _PyPreConfig.isolated=1, _PyCoreConfig.isolated not set
        config = {
            'isolated': 1,
            'use_environment': 0,
            'user_site_directory': 0,
        }
        self.check_all_configs("test_preinit_isolated1", config, api=API_COMPAT)

    def test_preinit_isolated2(self):
        # _PyPreConfig.isolated=0, _PyCoreConfig.isolated=1
        config = {
            'isolated': 1,
            'use_environment': 0,
            'user_site_directory': 0,
        }
        self.check_all_configs("test_preinit_isolated2", config, api=API_COMPAT)

    def test_preinit_isolated_config(self):
        self.check_all_configs("test_preinit_isolated_config", api=API_ISOLATED)

    def test_init_isolated_config(self):
        self.check_all_configs("test_init_isolated_config", api=API_ISOLATED)

    def test_preinit_python_config(self):
        self.check_all_configs("test_preinit_python_config", api=API_PYTHON)

    def test_init_python_config(self):
        self.check_all_configs("test_init_python_config", api=API_PYTHON)

    def test_init_dont_configure_locale(self):
        # _PyPreConfig.configure_locale=0
        preconfig = {
            'configure_locale': 0,
            'coerce_c_locale': 0,
        }
        self.check_all_configs("test_init_dont_configure_locale", {}, preconfig,
                               api=API_PYTHON)

    def test_init_read_set(self):
        config = {
            'program_name': './init_read_set',
            'executable': 'my_executable',
        }
        def modify_path(path):
            path.insert(1, "test_path_insert1")
            path.append("test_path_append")
        self.check_all_configs("test_init_read_set", config,
                               api=API_PYTHON,
                               modify_path_cb=modify_path)

    def test_init_sys_add(self):
        config = {
            'faulthandler': 1,
            'xoptions': [
                'dev',
                'utf8',
                'faulthandler',
            ],
            'warnoptions': [
                'ignore:::cmdline_warnoption',
                'ignore:::sysadd_warnoption',
                'ignore:::config_warnoption',
            ],
            'orig_argv': ['python3',
                          '-W', 'ignore:::cmdline_warnoption',
                          '-X', 'utf8'],
        }
        preconfig = {'utf8_mode': 1}
        self.check_all_configs("test_init_sys_add", config,
                               expected_preconfig=preconfig,
                               api=API_PYTHON)

    def test_init_run_main(self):
        code = ('import _testinternalcapi, json; '
                'print(json.dumps(_testinternalcapi.get_configs()))')
        config = {
            'argv': ['-c', 'arg2'],
            'orig_argv': ['python3', '-c', code, 'arg2'],
            'program_name': './python3',
            'run_command': code + '\n',
            'parse_argv': 2,
        }
        self.check_all_configs("test_init_run_main", config, api=API_PYTHON)

    def test_init_main(self):
        code = ('import _testinternalcapi, json; '
                'print(json.dumps(_testinternalcapi.get_configs()))')
        config = {
            'argv': ['-c', 'arg2'],
            'orig_argv': ['python3',
                          '-c', code,
                          'arg2'],
            'program_name': './python3',
            'run_command': code + '\n',
            'parse_argv': 2,
            '_init_main': 0,
        }
        self.check_all_configs("test_init_main", config,
                               api=API_PYTHON,
                               stderr="Run Python code before _Py_InitializeMain")

    def test_init_parse_argv(self):
        config = {
            'parse_argv': 2,
            'argv': ['-c', 'arg1', '-v', 'arg3'],
            'orig_argv': ['./argv0', '-E', '-c', 'pass', 'arg1', '-v', 'arg3'],
            'program_name': './argv0',
            'run_command': 'pass\n',
            'use_environment': 0,
        }
        self.check_all_configs("test_init_parse_argv", config, api=API_PYTHON)

    def test_init_dont_parse_argv(self):
        pre_config = {
            'parse_argv': 0,
        }
        config = {
            'parse_argv': 0,
            'argv': ['./argv0', '-E', '-c', 'pass', 'arg1', '-v', 'arg3'],
            'orig_argv': ['./argv0', '-E', '-c', 'pass', 'arg1', '-v', 'arg3'],
            'program_name': './argv0',
        }
        self.check_all_configs("test_init_dont_parse_argv", config, pre_config,
                               api=API_PYTHON)

    def default_program_name(self, config):
        if MS_WINDOWS:
            program_name = 'python'
            executable = self.test_exe
        else:
            program_name = 'python3'
            if MACOS:
                executable = self.test_exe
            else:
                executable = shutil.which(program_name) or ''
        config.update({
            'program_name': program_name,
            'base_executable': executable,
            'executable': executable,
        })

    def test_init_setpath(self):
        # Test Py_SetPath()
        config = self._get_expected_config()
        paths = config['config']['module_search_paths']

        config = {
            'module_search_paths': paths,
            'prefix': '',
            'base_prefix': '',
            'exec_prefix': '',
            'base_exec_prefix': '',
             # The current getpath.c doesn't determine the stdlib dir
             # in this case.
            'stdlib_dir': '',
            'use_frozen_modules': -1,
        }
        self.default_program_name(config)
        env = {'TESTPATH': os.path.pathsep.join(paths)}

        self.check_all_configs("test_init_setpath", config,
                               api=API_COMPAT, env=env,
                               ignore_stderr=True)

    def test_init_setpath_config(self):
        # Test Py_SetPath() with PyConfig
        config = self._get_expected_config()
        paths = config['config']['module_search_paths']

        config = {
            # set by Py_SetPath()
            'module_search_paths': paths,
            'prefix': '',
            'base_prefix': '',
            'exec_prefix': '',
            'base_exec_prefix': '',
             # The current getpath.c doesn't determine the stdlib dir
             # in this case.
            'stdlib_dir': '',
            'use_frozen_modules': -1,
            # overridden by PyConfig
            'program_name': 'conf_program_name',
            'base_executable': 'conf_executable',
            'executable': 'conf_executable',
        }
        env = {'TESTPATH': os.path.pathsep.join(paths)}
        self.check_all_configs("test_init_setpath_config", config,
                               api=API_PYTHON, env=env, ignore_stderr=True)

    def module_search_paths(self, prefix=None, exec_prefix=None):
        config = self._get_expected_config()
        if prefix is None:
            prefix = config['config']['prefix']
        if exec_prefix is None:
            exec_prefix = config['config']['prefix']
        if MS_WINDOWS:
            return config['config']['module_search_paths']
        else:
            ver = sys.version_info
            return [
                os.path.join(prefix, sys.platlibdir,
                             f'python{ver.major}{ver.minor}.zip'),
                os.path.join(prefix, sys.platlibdir,
                             f'python{ver.major}.{ver.minor}'),
                os.path.join(exec_prefix, sys.platlibdir,
                             f'python{ver.major}.{ver.minor}', 'lib-dynload'),
            ]

    @contextlib.contextmanager
    def tmpdir_with_python(self):
        # Temporary directory with a copy of the Python program
        with tempfile.TemporaryDirectory() as tmpdir:
            # bpo-38234: On macOS and FreeBSD, the temporary directory
            # can be symbolic link. For example, /tmp can be a symbolic link
            # to /var/tmp. Call realpath() to resolve all symbolic links.
            tmpdir = os.path.realpath(tmpdir)

            if MS_WINDOWS:
                # Copy pythonXY.dll (or pythonXY_d.dll)
                ver = sys.version_info
                dll = f'python{ver.major}{ver.minor}'
                dll3 = f'python{ver.major}'
                if debug_build(sys.executable):
                    dll += '_d'
                    dll3 += '_d'
                dll += '.dll'
                dll3 += '.dll'
                dll = os.path.join(os.path.dirname(self.test_exe), dll)
                dll3 = os.path.join(os.path.dirname(self.test_exe), dll3)
                dll_copy = os.path.join(tmpdir, os.path.basename(dll))
                dll3_copy = os.path.join(tmpdir, os.path.basename(dll3))
                shutil.copyfile(dll, dll_copy)
                shutil.copyfile(dll3, dll3_copy)

            # Copy Python program
            exec_copy = os.path.join(tmpdir, os.path.basename(self.test_exe))
            shutil.copyfile(self.test_exe, exec_copy)
            shutil.copystat(self.test_exe, exec_copy)
            self.test_exe = exec_copy

            yield tmpdir

    def test_init_setpythonhome(self):
        # Test Py_SetPythonHome(home) with PYTHONPATH env var
        config = self._get_expected_config()
        paths = config['config']['module_search_paths']
        paths_str = os.path.pathsep.join(paths)

        for path in paths:
            if not os.path.isdir(path):
                continue
            if os.path.exists(os.path.join(path, 'os.py')):
                home = os.path.dirname(path)
                break
        else:
            self.fail(f"Unable to find home in {paths!r}")

        prefix = exec_prefix = home
        if MS_WINDOWS:
            stdlib = os.path.join(home, sys.platlibdir)
        else:
            version = f'{sys.version_info.major}.{sys.version_info.minor}'
            stdlib = os.path.join(home, sys.platlibdir, f'python{version}')
        expected_paths = self.module_search_paths(prefix=home, exec_prefix=home)

        config = {
            'home': home,
            'module_search_paths': expected_paths,
            'prefix': prefix,
            'base_prefix': prefix,
            'exec_prefix': exec_prefix,
            'base_exec_prefix': exec_prefix,
            'pythonpath_env': paths_str,
            'stdlib_dir': stdlib,
        }
        self.default_program_name(config)
        if not config['executable']:
            config['use_frozen_modules'] = -1
        env = {'TESTHOME': home, 'PYTHONPATH': paths_str}
        self.check_all_configs("test_init_setpythonhome", config,
                               api=API_COMPAT, env=env)

    def copy_paths_by_env(self, config):
        all_configs = self._get_expected_config()
        paths = all_configs['config']['module_search_paths']
        paths_str = os.path.pathsep.join(paths)
        config['pythonpath_env'] = paths_str
        env = {'PYTHONPATH': paths_str}
        return env

    @unittest.skipIf(MS_WINDOWS, 'Windows does not use pybuilddir.txt')
    def test_init_pybuilddir(self):
        # Test path configuration with pybuilddir.txt configuration file

        with self.tmpdir_with_python() as tmpdir:
            # pybuilddir.txt is a sub-directory relative to the current
            # directory (tmpdir)
            subdir = 'libdir'
            libdir = os.path.join(tmpdir, subdir)
            os.mkdir(libdir)

            filename = os.path.join(tmpdir, 'pybuilddir.txt')
            with open(filename, "w", encoding="utf8") as fp:
                fp.write(subdir)

            module_search_paths = self.module_search_paths()
            module_search_paths[-1] = libdir

            executable = self.test_exe
            config = {
                'base_executable': executable,
                'executable': executable,
                'module_search_paths': module_search_paths,
                'stdlib_dir': STDLIB_INSTALL,
                'use_frozen_modules': 1 if STDLIB_INSTALL else -1,
            }
            env = self.copy_paths_by_env(config)
            self.check_all_configs("test_init_compat_config", config,
                                   api=API_COMPAT, env=env,
                                   ignore_stderr=True, cwd=tmpdir)

    def test_init_pyvenv_cfg(self):
        # Test path configuration with pyvenv.cfg configuration file

        with self.tmpdir_with_python() as tmpdir, \
             tempfile.TemporaryDirectory() as pyvenv_home:
            ver = sys.version_info

            if not MS_WINDOWS:
                lib_dynload = os.path.join(pyvenv_home,
                                           sys.platlibdir,
                                           f'python{ver.major}.{ver.minor}',
                                           'lib-dynload')
                os.makedirs(lib_dynload)
            else:
                lib_dynload = os.path.join(pyvenv_home, 'lib')
                os.makedirs(lib_dynload)
                # getpathp.c uses Lib\os.py as the LANDMARK
                shutil.copyfile(
                    os.path.join(support.STDLIB_DIR, 'os.py'),
                    os.path.join(lib_dynload, 'os.py'),
                )

            filename = os.path.join(tmpdir, 'pyvenv.cfg')
            with open(filename, "w", encoding="utf8") as fp:
                print("home = %s" % pyvenv_home, file=fp)
                print("include-system-site-packages = false", file=fp)

            paths = self.module_search_paths()
            if not MS_WINDOWS:
                paths[-1] = lib_dynload
            else:
                for index, path in enumerate(paths):
                    if index == 0:
                        paths[index] = os.path.join(tmpdir, os.path.basename(path))
                    else:
                        paths[index] = os.path.join(pyvenv_home, os.path.basename(path))
                paths[-1] = pyvenv_home

            executable = self.test_exe
            exec_prefix = pyvenv_home
            config = {
                'base_exec_prefix': exec_prefix,
                'exec_prefix': exec_prefix,
                'base_executable': executable,
                'executable': executable,
                'module_search_paths': paths,
            }
            path_config = {}
            if MS_WINDOWS:
                config['base_prefix'] = pyvenv_home
                config['prefix'] = pyvenv_home
                config['stdlib_dir'] = os.path.join(pyvenv_home, 'lib')
                config['use_frozen_modules'] = 1

                ver = sys.version_info
                dll = f'python{ver.major}'
                if debug_build(executable):
                    dll += '_d'
                dll += '.DLL'
                dll = os.path.join(os.path.dirname(executable), dll)
                path_config['python3_dll'] = dll
            else:
                # The current getpath.c doesn't determine the stdlib dir
                # in this case.
                config['stdlib_dir'] = STDLIB_INSTALL
                config['use_frozen_modules'] = 1 if STDLIB_INSTALL else -1

            env = self.copy_paths_by_env(config)
            self.check_all_configs("test_init_compat_config", config,
                                   expected_pathconfig=path_config,
                                   api=API_COMPAT, env=env,
                                   ignore_stderr=True, cwd=tmpdir)

    def test_global_pathconfig(self):
        # Test C API functions getting the path configuration:
        #
        # - Py_GetExecPrefix()
        # - Py_GetPath()
        # - Py_GetPrefix()
        # - Py_GetProgramFullPath()
        # - Py_GetProgramName()
        # - Py_GetPythonHome()
        #
        # The global path configuration (_Py_path_config) must be a copy
        # of the path configuration of PyInterpreter.config (PyConfig).
        ctypes = import_helper.import_module('ctypes')
        _testinternalcapi = import_helper.import_module('_testinternalcapi')

        def get_func(name):
            func = getattr(ctypes.pythonapi, name)
            func.argtypes = ()
            func.restype = ctypes.c_wchar_p
            return func

        Py_GetPath = get_func('Py_GetPath')
        Py_GetPrefix = get_func('Py_GetPrefix')
        Py_GetExecPrefix = get_func('Py_GetExecPrefix')
        Py_GetProgramName = get_func('Py_GetProgramName')
        Py_GetProgramFullPath = get_func('Py_GetProgramFullPath')
        Py_GetPythonHome = get_func('Py_GetPythonHome')

        config = _testinternalcapi.get_configs()['config']

        self.assertEqual(Py_GetPath().split(os.path.pathsep),
                         config['module_search_paths'])
        self.assertEqual(Py_GetPrefix(), config['prefix'])
        self.assertEqual(Py_GetExecPrefix(), config['exec_prefix'])
        self.assertEqual(Py_GetProgramName(), config['program_name'])
        self.assertEqual(Py_GetProgramFullPath(), config['executable'])
        self.assertEqual(Py_GetPythonHome(), config['home'])

    def test_init_warnoptions(self):
        # lowest to highest priority
        warnoptions = [
            'ignore:::PyConfig_Insert0',      # PyWideStringList_Insert(0)
            'default',                        # PyConfig.dev_mode=1
            'ignore:::env1',                  # PYTHONWARNINGS env var
            'ignore:::env2',                  # PYTHONWARNINGS env var
            'ignore:::cmdline1',              # -W opt command line option
            'ignore:::cmdline2',              # -W opt command line option
            'default::BytesWarning',          # PyConfig.bytes_warnings=1
            'ignore:::PySys_AddWarnOption1',  # PySys_AddWarnOption()
            'ignore:::PySys_AddWarnOption2',  # PySys_AddWarnOption()
            'ignore:::PyConfig_BeforeRead',   # PyConfig.warnoptions
            'ignore:::PyConfig_AfterRead']    # PyWideStringList_Append()
        preconfig = dict(allocator=PYMEM_ALLOCATOR_DEBUG)
        config = {
            'dev_mode': 1,
            'faulthandler': 1,
            'bytes_warning': 1,
            'warnoptions': warnoptions,
            'orig_argv': ['python3',
                          '-Wignore:::cmdline1',
                          '-Wignore:::cmdline2'],
        }
        self.check_all_configs("test_init_warnoptions", config, preconfig,
                               api=API_PYTHON)

    def test_init_set_config(self):
        config = {
            '_init_main': 0,
            'bytes_warning': 2,
            'warnoptions': ['error::BytesWarning'],
        }
        self.check_all_configs("test_init_set_config", config,
                               api=API_ISOLATED)

    def test_get_argc_argv(self):
        self.run_embedded_interpreter("test_get_argc_argv")
        # ignore output

    def test_init_use_frozen_modules(self):
        tests = {
            ('=on', 1),
            ('=off', 0),
            ('=', 1),
            ('', 1),
        }
        for raw, expected in tests:
            optval = f'frozen_modules{raw}'
            config = {
                'parse_argv': 2,
                'argv': ['-c'],
                'orig_argv': ['./argv0', '-X', optval, '-c', 'pass'],
                'program_name': './argv0',
                'run_command': 'pass\n',
                'use_environment': 1,
                'xoptions': [optval],
                'use_frozen_modules': expected,
            }
            env = {'TESTFROZEN': raw[1:]} if raw else None
            with self.subTest(repr(raw)):
                self.check_all_configs("test_init_use_frozen_modules", config,
                                       api=API_PYTHON, env=env)


class SetConfigTests(unittest.TestCase):
    def test_set_config(self):
        # bpo-42260: Test _PyInterpreterState_SetConfig()
        import_helper.import_module('_testcapi')
        cmd = [sys.executable, '-I', '-m', 'test._test_embed_set_config']
        proc = subprocess.run(cmd,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)
        self.assertEqual(proc.returncode, 0,
                         (proc.returncode, proc.stdout, proc.stderr))


class AuditingTests(EmbeddingTestsMixin, unittest.TestCase):
    def test_open_code_hook(self):
        self.run_embedded_interpreter("test_open_code_hook")

    def test_audit(self):
        self.run_embedded_interpreter("test_audit")

    def test_audit_subinterpreter(self):
        self.run_embedded_interpreter("test_audit_subinterpreter")

    def test_audit_run_command(self):
        self.run_embedded_interpreter("test_audit_run_command",
                                      timeout=support.SHORT_TIMEOUT,
                                      returncode=1)

    def test_audit_run_file(self):
        self.run_embedded_interpreter("test_audit_run_file",
                                      timeout=support.SHORT_TIMEOUT,
                                      returncode=1)

    def test_audit_run_interactivehook(self):
        startup = os.path.join(self.oldcwd, os_helper.TESTFN) + ".py"
        with open(startup, "w", encoding="utf-8") as f:
            print("import sys", file=f)
            print("sys.__interactivehook__ = lambda: None", file=f)
        try:
            env = {**remove_python_envvars(), "PYTHONSTARTUP": startup}
            self.run_embedded_interpreter("test_audit_run_interactivehook",
                                          timeout=support.SHORT_TIMEOUT,
                                          returncode=10, env=env)
        finally:
            os.unlink(startup)

    def test_audit_run_startup(self):
        startup = os.path.join(self.oldcwd, os_helper.TESTFN) + ".py"
        with open(startup, "w", encoding="utf-8") as f:
            print("pass", file=f)
        try:
            env = {**remove_python_envvars(), "PYTHONSTARTUP": startup}
            self.run_embedded_interpreter("test_audit_run_startup",
                                          timeout=support.SHORT_TIMEOUT,
                                          returncode=10, env=env)
        finally:
            os.unlink(startup)

    def test_audit_run_stdin(self):
        self.run_embedded_interpreter("test_audit_run_stdin",
                                      timeout=support.SHORT_TIMEOUT,
                                      returncode=1)


class MiscTests(EmbeddingTestsMixin, unittest.TestCase):
    def test_unicode_id_init(self):
        # bpo-42882: Test that _PyUnicode_FromId() works
        # when Python is initialized multiples times.
        self.run_embedded_interpreter("test_unicode_id_init")

    # See bpo-44133
    @unittest.skipIf(os.name == 'nt',
                     'Py_FrozenMain is not exported on Windows')
    def test_frozenmain(self):
        env = dict(os.environ)
        env['PYTHONUNBUFFERED'] = '1'
        out, err = self.run_embedded_interpreter("test_frozenmain", env=env)
        executable = os.path.realpath('./argv0')
        expected = textwrap.dedent(f"""
            Frozen Hello World
            sys.argv ['./argv0', '-E', 'arg1', 'arg2']
            config program_name: ./argv0
            config executable: {executable}
            config use_environment: 1
            config configure_c_stdio: 1
            config buffered_stdio: 0
        """).lstrip()
        self.assertEqual(out, expected)


class StdPrinterTests(EmbeddingTestsMixin, unittest.TestCase):
    # Test PyStdPrinter_Type which is used by _PySys_SetPreliminaryStderr():
    #   "Set up a preliminary stderr printer until we have enough
    #    infrastructure for the io module in place."

    STDOUT_FD = 1

    def create_printer(self, fd):
        ctypes = import_helper.import_module('ctypes')
        PyFile_NewStdPrinter = ctypes.pythonapi.PyFile_NewStdPrinter
        PyFile_NewStdPrinter.argtypes = (ctypes.c_int,)
        PyFile_NewStdPrinter.restype = ctypes.py_object
        return PyFile_NewStdPrinter(fd)

    def test_write(self):
        message = "unicode:\xe9-\u20ac-\udc80!\n"

        stdout_fd = self.STDOUT_FD
        stdout_fd_copy = os.dup(stdout_fd)
        self.addCleanup(os.close, stdout_fd_copy)

        rfd, wfd = os.pipe()
        self.addCleanup(os.close, rfd)
        self.addCleanup(os.close, wfd)
        try:
            # PyFile_NewStdPrinter() only accepts fileno(stdout)
            # or fileno(stderr) file descriptor.
            os.dup2(wfd, stdout_fd)

            printer = self.create_printer(stdout_fd)
            printer.write(message)
        finally:
            os.dup2(stdout_fd_copy, stdout_fd)

        data = os.read(rfd, 100)
        self.assertEqual(data, message.encode('utf8', 'backslashreplace'))

    def test_methods(self):
        fd = self.STDOUT_FD
        printer = self.create_printer(fd)
        self.assertEqual(printer.fileno(), fd)
        self.assertEqual(printer.isatty(), os.isatty(fd))
        printer.flush()  # noop
        printer.close()  # noop

    def test_disallow_instantiation(self):
        fd = self.STDOUT_FD
        printer = self.create_printer(fd)
        support.check_disallow_instantiation(self, type(printer))


if __name__ == "__main__":
    unittest.main()
