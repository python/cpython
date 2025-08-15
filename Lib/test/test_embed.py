# Run the tests in Programs/_testembed.c (tests for the CPython embedding APIs)
from test import support
from test.support import import_helper, os_helper, threading_helper, MS_WINDOWS
import unittest

from collections import namedtuple
import contextlib
import io
import json
import os
import os.path
import re
import shutil
import subprocess
import sys
import sysconfig
import tempfile
import textwrap

if not support.has_subprocess_support:
    raise unittest.SkipTest("test module requires subprocess")


try:
    import _testinternalcapi
except ImportError:
    _testinternalcapi = None


MACOS = (sys.platform == 'darwin')
PYMEM_ALLOCATOR_NOT_SET = 0
PYMEM_ALLOCATOR_DEBUG = 2
PYMEM_ALLOCATOR_MALLOC = 3
PYMEM_ALLOCATOR_MIMALLOC = 7
if support.Py_GIL_DISABLED:
    ALLOCATOR_FOR_CONFIG = PYMEM_ALLOCATOR_MIMALLOC
else:
    ALLOCATOR_FOR_CONFIG = PYMEM_ALLOCATOR_MALLOC

Py_STATS = hasattr(sys, '_stats_on')

# _PyCoreConfig_InitCompatConfig()
API_COMPAT = 1
# _PyCoreConfig_InitPythonConfig()
API_PYTHON = 2
# _PyCoreConfig_InitIsolatedConfig()
API_ISOLATED = 3

INIT_LOOPS = 4
MAX_HASH_SEED = 4294967295

ABI_THREAD = 't' if sysconfig.get_config_var('Py_GIL_DISABLED') else ''


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
        else:
            exepath = os.path.join(builddir, 'Programs')
        self.test_exe = exe = os.path.join(exepath, exename)
        if not os.path.exists(exe):
            self.skipTest("%r doesn't exist" % exe)
        # This is needed otherwise we get a fatal error:
        # "Py_Initialize: Unable to get the locale encoding
        # LookupError: no codec search functions registered: can't find encoding"
        self.oldcwd = os.getcwd()
        os.chdir(builddir)

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
            print("------")

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
            if support.verbose > 2:
                # 5 lines per pass is super-spammy, so limit that to -vvv
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
        if support.verbose > 1:
            print()
            print(out)
            print(err)
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
        if support.verbose > 1:
            print()
            print(out)
            print(err)
        expected_output = (
            "sys.warnoptions: ['once', 'module', 'default']\n"
            "sys._xoptions: {'not_an_option': '1', 'also_not_an_option': '2'}\n"
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

    def test_finalize_structseq(self):
        # bpo-46417: Py_Finalize() clears structseq static types. Check that
        # sys attributes using struct types still work when
        # Py_Finalize()/Py_Initialize() is called multiple times.
        # print() calls type->tp_repr(instance) and so checks that the types
        # are still working properly.
        script = support.findfile('_test_embed_structseq.py')
        with open(script, encoding="utf-8") as fp:
            code = fp.read()
        out, err = self.run_embedded_interpreter("test_repeated_init_exec", code)
        self.assertEqual(out, 'Tests passed\n' * INIT_LOOPS)

    def test_simple_initialization_api(self):
        # _testembed now uses Py_InitializeFromConfig by default
        # This case specifically checks Py_Initialize(Ex) still works
        out, err = self.run_embedded_interpreter("test_repeated_simple_init")
        self.assertEqual(out, 'Finalized\n' * INIT_LOOPS)

    @support.requires_specialization
    @unittest.skipUnless(support.TEST_MODULES_ENABLED, "requires test modules")
    def test_specialized_static_code_gets_unspecialized_at_Py_FINALIZE(self):
        # https://github.com/python/cpython/issues/92031

        code = textwrap.dedent("""\
            import dis
            import importlib._bootstrap
            import opcode
            import test.test_dis

            def is_specialized(f):
                for instruction in dis.get_instructions(f, adaptive=True):
                    opname = instruction.opname
                    if (
                        opname in opcode._specialized_opmap
                        # Exclude superinstructions:
                        and "__" not in opname
                    ):
                        return True
                return False

            func = importlib._bootstrap._handle_fromlist

            # "copy" the code to un-specialize it:
            func.__code__ = func.__code__.replace()

            assert not is_specialized(func), "specialized instructions found"

            for i in range(test.test_dis.ADAPTIVE_WARMUP_DELAY):
                func(importlib._bootstrap, ["x"], lambda *args: None)

            assert is_specialized(func), "no specialized instructions found"

            print("Tests passed")
        """)
        run = self.run_embedded_interpreter
        out, err = run("test_repeated_init_exec", code)
        self.assertEqual(out, 'Tests passed\n' * INIT_LOOPS)

    def test_ucnhash_capi_reset(self):
        # bpo-47182: unicodeobject.c:ucnhash_capi was not reset on shutdown.
        code = "print('\\N{digit nine}')"
        out, err = self.run_embedded_interpreter("test_repeated_init_exec", code)
        self.assertEqual(out, '9\n' * INIT_LOOPS)

    def test_datetime_reset_strptime(self):
        code = (
            "import datetime;"
            "d = datetime.datetime.strptime('2000-01-01', '%Y-%m-%d');"
            "print(d.strftime('%Y%m%d'))"
        )
        out, err = self.run_embedded_interpreter("test_repeated_init_exec", code)
        self.assertEqual(out, '20000101\n' * INIT_LOOPS)

    def test_static_types_inherited_slots(self):
        script = textwrap.dedent("""
            import test.support

            results = {}
            def add(cls, slot, own):
                value = getattr(cls, slot)
                try:
                    subresults = results[cls.__name__]
                except KeyError:
                    subresults = results[cls.__name__] = {}
                subresults[slot] = [repr(value), own]

            for cls in test.support.iter_builtin_types():
                for slot, own in test.support.iter_slot_wrappers(cls):
                    add(cls, slot, own)
            """)

        ns = {}
        exec(script, ns, ns)
        all_expected = ns['results']
        del ns

        script += textwrap.dedent("""
            import json
            import sys
            text = json.dumps(results)
            print(text, file=sys.stderr)
            """)
        out, err = self.run_embedded_interpreter(
                "test_repeated_init_exec", script, script)
        results = err.split('--- Loop #')[1:]
        results = [res.rpartition(' ---\n')[-1] for res in results]

        self.maxDiff = None
        for i, text in enumerate(results, start=1):
            result = json.loads(text)
            for classname, expected in all_expected.items():
                with self.subTest(loop=i, cls=classname):
                    slots = result.pop(classname)
                    self.assertEqual(slots, expected)
            self.assertEqual(result, {})
        self.assertEqual(out, '')

    def test_getargs_reset_static_parser(self):
        # Test _PyArg_Parser initializations via _PyArg_UnpackKeywords()
        # https://github.com/python/cpython/issues/122334
        code = textwrap.dedent("""
            try:
                import _ssl
            except ModuleNotFoundError:
                _ssl = None
            if _ssl is not None:
                _ssl.txt2obj(txt='1.3')
            print('1')

            import _queue
            _queue.SimpleQueue().put_nowait(item=None)
            print('2')

            import _zoneinfo
            _zoneinfo.ZoneInfo.clear_cache(only_keys=['Foo/Bar'])
            print('3')
        """)
        out, err = self.run_embedded_interpreter("test_repeated_init_exec", code)
        self.assertEqual(out, '1\n2\n3\n' * INIT_LOOPS)


@unittest.skipIf(_testinternalcapi is None, "requires _testinternalcapi")
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
        'isolated': False,
        'use_environment': True,
        'dev_mode': False,

        'install_signal_handlers': True,
        'use_hash_seed': False,
        'hash_seed': 0,
        'int_max_str_digits': sys.int_info.default_max_str_digits,
        'cpu_count': -1,
        'faulthandler': False,
        'tracemalloc': 0,
        'perf_profiling': False,
        'import_time': False,
        'code_debug_ranges': True,
        'show_ref_count': False,
        'dump_refs': False,
        'dump_refs_file': None,
        'malloc_stats': False,

        'filesystem_encoding': GET_DEFAULT_CONFIG,
        'filesystem_errors': GET_DEFAULT_CONFIG,

        'pycache_prefix': None,
        'program_name': GET_DEFAULT_CONFIG,
        'parse_argv': False,
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
        'module_search_paths_set': True,
        'platlibdir': sys.platlibdir,
        'stdlib_dir': GET_DEFAULT_CONFIG,

        'site_import': True,
        'bytes_warning': 0,
        'warn_default_encoding': False,
        'inspect': False,
        'interactive': False,
        'optimization_level': 0,
        'parser_debug': False,
        'write_bytecode': True,
        'verbose': 0,
        'quiet': False,
        'user_site_directory': True,
        'configure_c_stdio': False,
        'buffered_stdio': True,

        'stdio_encoding': GET_DEFAULT_CONFIG,
        'stdio_errors': GET_DEFAULT_CONFIG,

        'skip_source_first_line': False,
        'run_command': None,
        'run_module': None,
        'run_filename': None,
        'sys_path_0': None,

        '_install_importlib': True,
        'check_hash_pycs_mode': 'default',
        'pathconfig_warnings': True,
        '_init_main': True,
        'use_frozen_modules': not support.Py_DEBUG,
        'safe_path': False,
        '_is_python_build': IGNORE_CONFIG,
    }
    if Py_STATS:
        CONFIG_COMPAT['_pystats'] = 0
    if support.Py_DEBUG:
        CONFIG_COMPAT['run_presite'] = None
    if support.Py_GIL_DISABLED:
        CONFIG_COMPAT['enable_gil'] = -1
    if MS_WINDOWS:
        CONFIG_COMPAT.update({
            'legacy_windows_stdio': 0,
        })

    CONFIG_PYTHON = dict(CONFIG_COMPAT,
        _config_init=API_PYTHON,
        configure_c_stdio=True,
        parse_argv=True,
    )
    CONFIG_ISOLATED = dict(CONFIG_COMPAT,
        _config_init=API_ISOLATED,
        isolated=True,
        use_environment=False,
        user_site_directory=False,
        safe_path=True,
        dev_mode=False,
        install_signal_handlers=False,
        use_hash_seed=False,
        faulthandler=False,
        tracemalloc=0,
        perf_profiling=False,
        pathconfig_warnings=False,
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
                            env, api, modify_path_cb=None):
        configs = self._get_expected_config()

        pre_config = configs['pre_config']
        for key, value in expected_preconfig.items():
            if value is self.GET_DEFAULT_CONFIG:
                expected_preconfig[key] = pre_config[key]

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
        if MS_WINDOWS:
            value = config.get(key := 'program_name')
            if value and isinstance(value, str):
                value = value[:len(value.lower().removesuffix('.exe'))]
                if debug_build(sys.executable):
                    value = value[:len(value.lower().removesuffix('_d'))]
                config[key] = value
        for key, value in list(expected.items()):
            if value is self.IGNORE_CONFIG:
                config.pop(key, None)
                del expected[key]
            # Resolve bool/int mismatches to reduce noise in diffs
            if isinstance(value, (bool, int)) and isinstance(config.get(key), (bool, int)):
                expected[key] = type(config[key])(expected[key])
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

    def check_all_configs(self, testname, expected_config=None,
                          expected_preconfig=None,
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

        if api == API_PYTHON:
            default_config = self.CONFIG_PYTHON
        elif api == API_ISOLATED:
            default_config = self.CONFIG_ISOLATED
        else:
            default_config = self.CONFIG_COMPAT
        expected_config = dict(default_config, **expected_config)

        self.get_expected_config(expected_preconfig,
                                 expected_config,
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
            'allocator': ALLOCATOR_FOR_CONFIG,
            'utf8_mode': 1,
        }
        config = {
            'install_signal_handlers': False,
            'use_hash_seed': True,
            'hash_seed': 123,
            'tracemalloc': 2,
            'perf_profiling': False,
            'import_time': True,
            'code_debug_ranges': False,
            'show_ref_count': True,
            'malloc_stats': True,

            'stdio_encoding': 'iso8859-1',
            'stdio_errors': 'replace',

            'pycache_prefix': 'conf_pycache_prefix',
            'program_name': './conf_program_name',
            'argv': ['-c', 'arg2'],
            'orig_argv': ['python3',
                          '-W', 'cmdline_warnoption',
                          '-X', 'cmdline_xoption',
                          '-c', 'pass',
                          'arg2'],
            'parse_argv': True,
            'xoptions': [
                'config_xoption1=3',
                'config_xoption2=',
                'config_xoption3',
                'cmdline_xoption',
            ],
            'warnoptions': [
                'cmdline_warnoption',
                'default::BytesWarning',
                'config_warnoption',
            ],
            'run_command': 'pass\n',

            'site_import': False,
            'bytes_warning': 1,
            'inspect': True,
            'interactive': True,
            'optimization_level': 2,
            'write_bytecode': False,
            'verbose': 1,
            'quiet': True,
            'configure_c_stdio': True,
            'buffered_stdio': False,
            'user_site_directory': False,
            'faulthandler': True,
            'platlibdir': 'my_platlibdir',
            'module_search_paths': self.IGNORE_CONFIG,
            'safe_path': True,
            'int_max_str_digits': 31337,
            'cpu_count': 4321,

            'check_hash_pycs_mode': 'always',
            'pathconfig_warnings': False,
        }
        if Py_STATS:
            config['_pystats'] = 1
        self.check_all_configs("test_init_from_config", config, preconfig,
                               api=API_COMPAT)

    def test_init_compat_env(self):
        preconfig = {
            'allocator': ALLOCATOR_FOR_CONFIG,
        }
        config = {
            'use_hash_seed': True,
            'hash_seed': 42,
            'tracemalloc': 2,
            'perf_profiling': False,
            'import_time': True,
            'code_debug_ranges': False,
            'malloc_stats': True,
            'inspect': True,
            'optimization_level': 2,
            'pythonpath_env': '/my/path',
            'pycache_prefix': 'env_pycache_prefix',
            'write_bytecode': False,
            'verbose': 1,
            'buffered_stdio': False,
            'stdio_encoding': 'iso8859-1',
            'stdio_errors': 'replace',
            'user_site_directory': False,
            'faulthandler': True,
            'warnoptions': ['EnvVar'],
            'platlibdir': 'env_platlibdir',
            'module_search_paths': self.IGNORE_CONFIG,
            'safe_path': True,
            'int_max_str_digits': 4567,
        }
        if Py_STATS:
            config['_pystats'] = 1
        self.check_all_configs("test_init_compat_env", config, preconfig,
                               api=API_COMPAT)

    def test_init_python_env(self):
        preconfig = {
            'allocator': ALLOCATOR_FOR_CONFIG,
            'utf8_mode': 1,
        }
        config = {
            'use_hash_seed': True,
            'hash_seed': 42,
            'tracemalloc': 2,
            'perf_profiling': False,
            'import_time': True,
            'code_debug_ranges': False,
            'malloc_stats': True,
            'inspect': True,
            'optimization_level': 2,
            'pythonpath_env': '/my/path',
            'pycache_prefix': 'env_pycache_prefix',
            'write_bytecode': False,
            'verbose': 1,
            'buffered_stdio': False,
            'stdio_encoding': 'iso8859-1',
            'stdio_errors': 'replace',
            'user_site_directory': False,
            'faulthandler': True,
            'warnoptions': ['EnvVar'],
            'platlibdir': 'env_platlibdir',
            'module_search_paths': self.IGNORE_CONFIG,
            'safe_path': True,
            'int_max_str_digits': 4567,
        }
        if Py_STATS:
            config['_pystats'] = True
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
        preconfig = dict(allocator=ALLOCATOR_FOR_CONFIG)
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
            'faulthandler': True,
            'dev_mode': True,
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
            'orig_argv': ['python3', '-X', 'dev', '-P', 'script.py'],
            'run_filename': os.path.abspath('script.py'),
            'dev_mode': True,
            'faulthandler': True,
            'warnoptions': ['default'],
            'xoptions': ['dev'],
            'safe_path': True,
        }
        self.check_all_configs("test_preinit_parse_argv", config, preconfig,
                               api=API_PYTHON)

    def test_preinit_dont_parse_argv(self):
        # -X dev must be ignored by isolated preconfiguration
        preconfig = {
            'isolated': 0,
        }
        argv = ["python3",
               "-E", "-I", "-P",
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
            'isolated': True,
            'safe_path': True,
            'use_environment': False,
            'user_site_directory': False,
        }
        self.check_all_configs("test_init_isolated_flag", config, api=API_PYTHON)

    def test_preinit_isolated1(self):
        # _PyPreConfig.isolated=1, _PyCoreConfig.isolated not set
        config = {
            'isolated': True,
            'safe_path': True,
            'use_environment': False,
            'user_site_directory': False,
        }
        self.check_all_configs("test_preinit_isolated1", config, api=API_COMPAT)

    def test_preinit_isolated2(self):
        # _PyPreConfig.isolated=0, _PyCoreConfig.isolated=1
        config = {
            'isolated': True,
            'safe_path': True,
            'use_environment': False,
            'user_site_directory': False,
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

    @unittest.skip('as of 3.11 this test no longer works because '
                   'path calculations do not occur on read')
    def test_init_read_set(self):
        config = {
            'program_name': './init_read_set',
            'executable': 'my_executable',
            'base_executable': 'my_executable',
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
                'config_xoption',
                'cmdline_xoption',
                'sysadd_xoption',
                'faulthandler',
            ],
            'warnoptions': [
                'ignore:::cmdline_warnoption',
                'ignore:::sysadd_warnoption',
                'ignore:::config_warnoption',
            ],
            'orig_argv': ['python3',
                          '-W', 'ignore:::cmdline_warnoption',
                          '-X', 'cmdline_xoption'],
        }
        self.check_all_configs("test_init_sys_add", config, api=API_PYTHON)

    def test_init_run_main(self):
        code = ('import _testinternalcapi, json; '
                'print(json.dumps(_testinternalcapi.get_configs()))')
        config = {
            'argv': ['-c', 'arg2'],
            'orig_argv': ['python3', '-c', code, 'arg2'],
            'program_name': './python3',
            'run_command': code + '\n',
            'parse_argv': True,
            'sys_path_0': '',
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
            'parse_argv': True,
            '_init_main': 0,
            'sys_path_0': '',
        }
        self.check_all_configs("test_init_main", config,
                               api=API_PYTHON,
                               stderr="Run Python code before _Py_InitializeMain")

    def test_init_parse_argv(self):
        config = {
            'parse_argv': True,
            'argv': ['-c', 'arg1', '-v', 'arg3'],
            'orig_argv': ['./argv0', '-E', '-c', 'pass', 'arg1', '-v', 'arg3'],
            'program_name': './argv0',
            'run_command': 'pass\n',
            'use_environment': False,
        }
        self.check_all_configs("test_init_parse_argv", config, api=API_PYTHON)

    def test_init_dont_parse_argv(self):
        pre_config = {
            'parse_argv': 0,
        }
        config = {
            'parse_argv': False,
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
            'use_frozen_modules': not support.Py_DEBUG,
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
                             f'python{ver.major}{ver.minor}{ABI_THREAD}.zip'),
                os.path.join(prefix, sys.platlibdir,
                             f'python{ver.major}.{ver.minor}{ABI_THREAD}'),
                os.path.join(exec_prefix, sys.platlibdir,
                             f'python{ver.major}.{ver.minor}{ABI_THREAD}', 'lib-dynload'),
            ]

    @contextlib.contextmanager
    def tmpdir_with_python(self, subdir=None):
        # Temporary directory with a copy of the Python program
        with tempfile.TemporaryDirectory() as tmpdir:
            # bpo-38234: On macOS and FreeBSD, the temporary directory
            # can be symbolic link. For example, /tmp can be a symbolic link
            # to /var/tmp. Call realpath() to resolve all symbolic links.
            tmpdir = os.path.realpath(tmpdir)
            if subdir:
                tmpdir = os.path.normpath(os.path.join(tmpdir, subdir))
                os.makedirs(tmpdir)

            if MS_WINDOWS:
                # Copy pythonXY.dll (or pythonXY_d.dll)
                import fnmatch
                exedir = os.path.dirname(self.test_exe)
                for f in os.listdir(exedir):
                    if fnmatch.fnmatch(f, '*.dll'):
                        shutil.copyfile(os.path.join(exedir, f), os.path.join(tmpdir, f))

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
            stdlib = os.path.join(home, "Lib")
            # Because we are specifying 'home', module search paths
            # are fairly static
            expected_paths = [paths[0], os.path.join(home, 'DLLs'), stdlib]
        else:
            version = f'{sys.version_info.major}.{sys.version_info.minor}'
            stdlib = os.path.join(home, sys.platlibdir, f'python{version}{ABI_THREAD}')
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
        env = {'TESTHOME': home, 'PYTHONPATH': paths_str}
        self.check_all_configs("test_init_setpythonhome", config,
                               api=API_COMPAT, env=env)

    def test_init_is_python_build_with_home(self):
        # Test _Py_path_config._is_python_build configuration (gh-91985)
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
            stdlib = os.path.join(home, "Lib")
            # Because we are specifying 'home', module search paths
            # are fairly static
            expected_paths = [paths[0], os.path.join(home, 'DLLs'), stdlib]
        else:
            version = f'{sys.version_info.major}.{sys.version_info.minor}'
            stdlib = os.path.join(home, sys.platlibdir, f'python{version}{ABI_THREAD}')
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
        # The code above is taken from test_init_setpythonhome()
        env = {'TESTHOME': home, 'PYTHONPATH': paths_str}

        env['NEGATIVE_ISPYTHONBUILD'] = '1'
        config['_is_python_build'] = 0
        self.check_all_configs("test_init_is_python_build", config,
                               api=API_COMPAT, env=env)

        env['NEGATIVE_ISPYTHONBUILD'] = '0'
        config['_is_python_build'] = 1
        exedir = os.path.dirname(sys.executable)
        with open(os.path.join(exedir, 'pybuilddir.txt'), encoding='utf8') as f:
            expected_paths[1 if MS_WINDOWS else 2] = os.path.normpath(
                os.path.join(exedir, f'{f.read()}\n$'.splitlines()[0]))
        if not MS_WINDOWS:
            # PREFIX (default) is set when running in build directory
            prefix = exec_prefix = sys.prefix
            # stdlib calculation (/Lib) is not yet supported
            expected_paths[0] = self.module_search_paths(prefix=prefix)[0]
            config.update(prefix=prefix, base_prefix=prefix,
                          exec_prefix=exec_prefix, base_exec_prefix=exec_prefix)
        self.check_all_configs("test_init_is_python_build", config,
                               api=API_COMPAT, env=env)

    def copy_paths_by_env(self, config):
        all_configs = self._get_expected_config()
        paths = all_configs['config']['module_search_paths']
        paths_str = os.path.pathsep.join(paths)
        config['pythonpath_env'] = paths_str
        env = {'PYTHONPATH': paths_str}
        return env

    @unittest.skipIf(MS_WINDOWS, 'See test_init_pybuilddir_win32')
    def test_init_pybuilddir(self):
        # Test path configuration with pybuilddir.txt configuration file

        with self.tmpdir_with_python() as tmpdir:
            # pybuilddir.txt is a sub-directory relative to the current
            # directory (tmpdir)
            vpath = sysconfig.get_config_var("VPATH") or ''
            subdir = 'libdir'
            libdir = os.path.join(tmpdir, subdir)
            # The stdlib dir is dirname(executable) + VPATH + 'Lib'
            stdlibdir = os.path.normpath(os.path.join(tmpdir, vpath, 'Lib'))
            os.mkdir(libdir)

            filename = os.path.join(tmpdir, 'pybuilddir.txt')
            with open(filename, "w", encoding="utf8") as fp:
                fp.write(subdir)

            module_search_paths = self.module_search_paths()
            module_search_paths[-2] = stdlibdir
            module_search_paths[-1] = libdir

            executable = self.test_exe
            config = {
                'base_exec_prefix': sysconfig.get_config_var("exec_prefix"),
                'base_prefix': sysconfig.get_config_var("prefix"),
                'base_executable': executable,
                'executable': executable,
                'module_search_paths': module_search_paths,
                'stdlib_dir': stdlibdir,
            }
            env = self.copy_paths_by_env(config)
            self.check_all_configs("test_init_compat_config", config,
                                   api=API_COMPAT, env=env,
                                   ignore_stderr=True, cwd=tmpdir)

    @unittest.skipUnless(MS_WINDOWS, 'See test_init_pybuilddir')
    def test_init_pybuilddir_win32(self):
        # Test path configuration with pybuilddir.txt configuration file

        vpath = sysconfig.get_config_var("VPATH")
        subdir = r'PCbuild\arch'
        if os.path.normpath(vpath).count(os.sep) == 2:
            subdir = os.path.join(subdir, 'instrumented')

        with self.tmpdir_with_python(subdir) as tmpdir:
            # The prefix is dirname(executable) + VPATH
            prefix = os.path.normpath(os.path.join(tmpdir, vpath))
            # The stdlib dir is dirname(executable) + VPATH + 'Lib'
            stdlibdir = os.path.normpath(os.path.join(tmpdir, vpath, 'Lib'))

            filename = os.path.join(tmpdir, 'pybuilddir.txt')
            with open(filename, "w", encoding="utf8") as fp:
                fp.write(tmpdir)

            module_search_paths = self.module_search_paths()
            module_search_paths[-3] = os.path.join(tmpdir, os.path.basename(module_search_paths[-3]))
            module_search_paths[-2] = tmpdir
            module_search_paths[-1] = stdlibdir

            executable = self.test_exe
            config = {
                'base_exec_prefix': prefix,
                'base_prefix': prefix,
                'base_executable': executable,
                'executable': executable,
                'prefix': prefix,
                'exec_prefix': prefix,
                'module_search_paths': module_search_paths,
                'stdlib_dir': stdlibdir,
            }
            env = self.copy_paths_by_env(config)
            self.check_all_configs("test_init_compat_config", config,
                                   api=API_COMPAT, env=env,
                                   ignore_stderr=False, cwd=tmpdir)

    def test_init_pyvenv_cfg(self):
        # Test path configuration with pyvenv.cfg configuration file

        with self.tmpdir_with_python() as tmpdir, \
             tempfile.TemporaryDirectory() as pyvenv_home:
            ver = sys.version_info

            if not MS_WINDOWS:
                lib_dynload = os.path.join(pyvenv_home,
                                           sys.platlibdir,
                                           f'python{ver.major}.{ver.minor}{ABI_THREAD}',
                                           'lib-dynload')
                os.makedirs(lib_dynload)
            else:
                lib_folder = os.path.join(pyvenv_home, 'Lib')
                os.makedirs(lib_folder)
                # getpath.py uses Lib\os.py as the LANDMARK
                shutil.copyfile(
                    os.path.join(support.STDLIB_DIR, 'os.py'),
                    os.path.join(lib_folder, 'os.py'),
                )

            filename = os.path.join(tmpdir, 'pyvenv.cfg')
            with open(filename, "w", encoding="utf8") as fp:
                print("home = %s" % pyvenv_home, file=fp)
                print("include-system-site-packages = false", file=fp)

            paths = self.module_search_paths()
            if not MS_WINDOWS:
                paths[-1] = lib_dynload
            else:
                paths = [
                    os.path.join(tmpdir, os.path.basename(paths[0])),
                    pyvenv_home,
                    os.path.join(pyvenv_home, "Lib"),
                ]

            executable = self.test_exe
            base_executable = os.path.join(pyvenv_home, os.path.basename(executable))
            exec_prefix = pyvenv_home
            config = {
                'base_prefix': sysconfig.get_config_var("prefix"),
                'base_exec_prefix': exec_prefix,
                'exec_prefix': exec_prefix,
                'base_executable': base_executable,
                'executable': executable,
                'module_search_paths': paths,
            }
            if MS_WINDOWS:
                config['base_prefix'] = pyvenv_home
                config['prefix'] = pyvenv_home
                config['stdlib_dir'] = os.path.join(pyvenv_home, 'Lib')
                config['use_frozen_modules'] = int(not support.Py_DEBUG)
            else:
                # cannot reliably assume stdlib_dir here because it
                # depends too much on our build. But it ought to be found
                config['stdlib_dir'] = self.IGNORE_CONFIG
                config['use_frozen_modules'] = int(not support.Py_DEBUG)

            env = self.copy_paths_by_env(config)
            self.check_all_configs("test_init_compat_config", config,
                                   api=API_COMPAT, env=env,
                                   ignore_stderr=True, cwd=tmpdir)

    @unittest.skipUnless(MS_WINDOWS, 'specific to Windows')
    def test_getpath_abspath_win32(self):
        # Check _Py_abspath() is passed a backslashed path not to fall back to
        # GetFullPathNameW() on startup, which (re-)normalizes the path overly.
        # Currently, _Py_normpath() doesn't trim trailing dots and spaces.
        CASES = [
            ("C:/a. . .",  "C:\\a. . ."),
            ("C:\\a. . .", "C:\\a. . ."),
            ("\\\\?\\C:////a////b. . .", "\\\\?\\C:\\a\\b. . ."),
            ("//a/b/c. . .", "\\\\a\\b\\c. . ."),
            ("\\\\a\\b\\c. . .", "\\\\a\\b\\c. . ."),
            ("a. . .", f"{os.getcwd()}\\a"),  # relpath gets fully normalized
        ]
        out, err = self.run_embedded_interpreter(
            "test_init_initialize_config",
            env={**remove_python_envvars(),
                 "PYTHONPATH": os.path.pathsep.join(c[0] for c in CASES)}
        )
        self.assertEqual(err, "")
        try:
            out = json.loads(out)
        except json.JSONDecodeError:
            self.fail(f"fail to decode stdout: {out!r}")

        results = out['config']["module_search_paths"]
        for (_, expected), result in zip(CASES, results):
            self.assertEqual(result, expected)

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
            ('=on', True),
            ('=off', False),
            ('=', True),
            ('', True),
        }
        for raw, expected in tests:
            optval = f'frozen_modules{raw}'
            config = {
                'parse_argv': True,
                'argv': ['-c'],
                'orig_argv': ['./argv0', '-X', optval, '-c', 'pass'],
                'program_name': './argv0',
                'run_command': 'pass\n',
                'use_environment': True,
                'xoptions': [optval],
                'use_frozen_modules': expected,
            }
            env = {'TESTFROZEN': raw[1:]} if raw else None
            with self.subTest(repr(raw)):
                self.check_all_configs("test_init_use_frozen_modules", config,
                                       api=API_PYTHON, env=env)

    def test_init_main_interpreter_settings(self):
        OBMALLOC = 1<<5
        EXTENSIONS = 1<<8
        THREADS = 1<<10
        DAEMON_THREADS = 1<<11
        FORK = 1<<15
        EXEC = 1<<16
        expected = {
            # All optional features should be enabled.
            'feature_flags':
                OBMALLOC | FORK | EXEC | THREADS | DAEMON_THREADS,
            'own_gil': True,
        }
        out, err = self.run_embedded_interpreter(
            'test_init_main_interpreter_settings',
        )
        self.assertEqual(err, '')
        try:
            out = json.loads(out)
        except json.JSONDecodeError:
            self.fail(f'fail to decode stdout: {out!r}')

        self.assertEqual(out, expected)

    @threading_helper.requires_working_threading()
    def test_init_in_background_thread(self):
        # gh-123022: Check that running Py_Initialize() in a background
        # thread doesn't crash.
        out, err = self.run_embedded_interpreter("test_init_in_background_thread")
        self.assertEqual(err, "")


class SetConfigTests(unittest.TestCase):
    def test_set_config(self):
        # bpo-42260: Test _PyInterpreterState_SetConfig()
        import_helper.import_module('_testcapi')
        cmd = [sys.executable, '-X', 'utf8', '-I', '-m', 'test._test_embed_set_config']
        proc = subprocess.run(cmd,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE,
                              encoding='utf-8', errors='backslashreplace')
        if proc.returncode and support.verbose:
            print(proc.stdout)
            print(proc.stderr)
        self.assertEqual(proc.returncode, 0,
                         (proc.returncode, proc.stdout, proc.stderr))


class AuditingTests(EmbeddingTestsMixin, unittest.TestCase):
    def test_open_code_hook(self):
        self.run_embedded_interpreter("test_open_code_hook")

    def test_audit(self):
        self.run_embedded_interpreter("test_audit")

    def test_audit_tuple(self):
        self.run_embedded_interpreter("test_audit_tuple")

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

    def test_get_incomplete_frame(self):
        self.run_embedded_interpreter("test_get_incomplete_frame")


class MiscTests(EmbeddingTestsMixin, unittest.TestCase):
    def test_unicode_id_init(self):
        # bpo-42882: Test that _PyUnicode_FromId() works
        # when Python is initialized multiples times.
        self.run_embedded_interpreter("test_unicode_id_init")

    # See bpo-44133
    @unittest.skipIf(os.name == 'nt',
                     'Py_FrozenMain is not exported on Windows')
    @unittest.skipIf(_testinternalcapi is None, "requires _testinternalcapi")
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
            config use_environment: True
            config configure_c_stdio: True
            config buffered_stdio: False
        """).lstrip()
        self.assertEqual(out, expected)

    @unittest.skipUnless(support.Py_DEBUG,
                         '-X showrefcount requires a Python debug build')
    def test_no_memleak(self):
        # bpo-1635741: Python must release all memory at exit
        tests = (
            ('off', 'pass'),
            ('on', 'pass'),
            ('off', 'import __hello__'),
            ('on', 'import __hello__'),
        )
        for flag, stmt in tests:
            xopt = f"frozen_modules={flag}"
            cmd = [sys.executable, "-I", "-X", "showrefcount", "-X", xopt, "-c", stmt]
            proc = subprocess.run(cmd,
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT,
                                  text=True)
            self.assertEqual(proc.returncode, 0)
            out = proc.stdout.rstrip()
            match = re.match(r'^\[(-?\d+) refs, (-?\d+) blocks\]', out)
            if not match:
                self.fail(f"unexpected output: {out!a}")
            refs = int(match.group(1))
            blocks = int(match.group(2))
            with self.subTest(frozen_modules=flag, stmt=stmt):
                self.assertEqual(refs, 0, out)
                self.assertEqual(blocks, 0, out)

    @unittest.skipUnless(support.Py_DEBUG,
                         '-X presite requires a Python debug build')
    def test_presite(self):
        cmd = [sys.executable, "-I", "-X", "presite=test.reperf", "-c", "print('cmd')"]
        proc = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
        )
        self.assertEqual(proc.returncode, 0)
        out = proc.stdout.strip()
        self.assertIn("10 times sub", out)
        self.assertIn("CPU seconds", out)
        self.assertIn("cmd", out)


if __name__ == "__main__":
    unittest.main()
