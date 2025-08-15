import contextlib
import itertools
import os
import re
import shutil
import subprocess
import sys
import sysconfig
import tempfile
import unittest
from pathlib import Path
from test import support

if sys.platform != "win32":
    raise unittest.SkipTest("test only applies to Windows")

# Get winreg after the platform check
import winreg


PY_EXE = "py.exe"
DEBUG_BUILD = False
if sys.executable.casefold().endswith("_d.exe".casefold()):
    PY_EXE = "py_d.exe"
    DEBUG_BUILD = True

# Registry data to create. On removal, everything beneath top-level names will
# be deleted.
TEST_DATA = {
    "PythonTestSuite": {
        "DisplayName": "Python Test Suite",
        "SupportUrl": "https://www.python.org/",
        "3.100": {
            "DisplayName": "X.Y version",
            "InstallPath": {
                None: sys.prefix,
                "ExecutablePath": "X.Y.exe",
            }
        },
        "3.100-32": {
            "DisplayName": "X.Y-32 version",
            "InstallPath": {
                None: sys.prefix,
                "ExecutablePath": "X.Y-32.exe",
            }
        },
        "3.100-arm64": {
            "DisplayName": "X.Y-arm64 version",
            "InstallPath": {
                None: sys.prefix,
                "ExecutablePath": "X.Y-arm64.exe",
                "ExecutableArguments": "-X fake_arg_for_test",
            }
        },
        "ignored": {
            "DisplayName": "Ignored because no ExecutablePath",
            "InstallPath": {
                None: sys.prefix,
            }
        },
    },
    "PythonTestSuite1": {
        "DisplayName": "Python Test Suite Single",
        "3.100": {
            "DisplayName": "Single Interpreter",
            "InstallPath": {
                None: sys.prefix,
                "ExecutablePath": sys.executable,
            }
        }
    },
}


TEST_PY_ENV = dict(
    PY_PYTHON="PythonTestSuite/3.100",
    PY_PYTHON2="PythonTestSuite/3.100-32",
    PY_PYTHON3="PythonTestSuite/3.100-arm64",
)


TEST_PY_DEFAULTS = "\n".join([
    "[defaults]",
    *[f"{k[3:].lower()}={v}" for k, v in TEST_PY_ENV.items()],
])


TEST_PY_COMMANDS = "\n".join([
    "[commands]",
    "test-command=TEST_EXE.exe",
])


def quote(s):
    s = str(s)
    return f'"{s}"' if " " in s else s


def create_registry_data(root, data):
    def _create_registry_data(root, key, value):
        if isinstance(value, dict):
            # For a dict, we recursively create keys
            with winreg.CreateKeyEx(root, key) as hkey:
                for k, v in value.items():
                    _create_registry_data(hkey, k, v)
        elif isinstance(value, str):
            # For strings, we set values. 'key' may be None in this case
            winreg.SetValueEx(root, key, None, winreg.REG_SZ, value)
        else:
            raise TypeError("don't know how to create data for '{}'".format(value))

    for k, v in data.items():
        _create_registry_data(root, k, v)


def enum_keys(root):
    for i in itertools.count():
        try:
            yield winreg.EnumKey(root, i)
        except OSError as ex:
            if ex.winerror == 259:
                break
            raise


def delete_registry_data(root, keys):
    ACCESS = winreg.KEY_WRITE | winreg.KEY_ENUMERATE_SUB_KEYS
    for key in list(keys):
        with winreg.OpenKey(root, key, access=ACCESS) as hkey:
            delete_registry_data(hkey, enum_keys(hkey))
        winreg.DeleteKey(root, key)


def is_installed(tag):
    key = rf"Software\Python\PythonCore\{tag}\InstallPath"
    for root, flag in [
        (winreg.HKEY_CURRENT_USER, 0),
        (winreg.HKEY_LOCAL_MACHINE, winreg.KEY_WOW64_64KEY),
        (winreg.HKEY_LOCAL_MACHINE, winreg.KEY_WOW64_32KEY),
    ]:
        try:
            winreg.CloseKey(winreg.OpenKey(root, key, access=winreg.KEY_READ | flag))
            return True
        except OSError:
            pass
    return False


class PreservePyIni:
    def __init__(self, path, content):
        self.path = Path(path)
        self.content = content
        self._preserved = None

    def __enter__(self):
        try:
            self._preserved = self.path.read_bytes()
        except FileNotFoundError:
            self._preserved = None
        self.path.write_text(self.content, encoding="utf-16")

    def __exit__(self, *exc_info):
        if self._preserved is None:
            self.path.unlink()
        else:
            self.path.write_bytes(self._preserved)


class RunPyMixin:
    py_exe = None

    @classmethod
    def find_py(cls):
        py_exe = None
        if sysconfig.is_python_build():
            py_exe = Path(sys.executable).parent / PY_EXE
        else:
            for p in os.getenv("PATH").split(";"):
                if p:
                    py_exe = Path(p) / PY_EXE
                    if py_exe.is_file():
                        break
            else:
                py_exe = None

        # Test launch and check version, to exclude installs of older
        # releases when running outside of a source tree
        if py_exe:
            try:
                with subprocess.Popen(
                    [py_exe, "-h"],
                    stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    encoding="ascii",
                    errors="ignore",
                ) as p:
                    p.stdin.close()
                    version = next(p.stdout, "\n").splitlines()[0].rpartition(" ")[2]
                    p.stdout.read()
                    p.wait(10)
                if not sys.version.startswith(version):
                    py_exe = None
            except OSError:
                py_exe = None

        if not py_exe:
            raise unittest.SkipTest(
                "cannot locate '{}' for test".format(PY_EXE)
            )
        return py_exe

    def get_py_exe(self):
        if not self.py_exe:
            self.py_exe = self.find_py()
        return self.py_exe

    def run_py(self, args, env=None, allow_fail=False, expect_returncode=0, argv=None):
        if not self.py_exe:
            self.py_exe = self.find_py()

        ignore = {"VIRTUAL_ENV", "PY_PYTHON", "PY_PYTHON2", "PY_PYTHON3"}
        env = {
            **{k.upper(): v for k, v in os.environ.items() if k.upper() not in ignore},
            "PYLAUNCHER_DEBUG": "1",
            "PYLAUNCHER_DRYRUN": "1",
            "PYLAUNCHER_LIMIT_TO_COMPANY": "",
            **{k.upper(): v for k, v in (env or {}).items()},
        }
        if not argv:
            argv = [self.py_exe, *args]
        with subprocess.Popen(
            argv,
            env=env,
            executable=self.py_exe,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        ) as p:
            p.stdin.close()
            p.wait(10)
            out = p.stdout.read().decode("utf-8", "replace")
            err = p.stderr.read().decode("ascii", "replace").replace("\uFFFD", "?")
        if p.returncode != expect_returncode and support.verbose and not allow_fail:
            print("++ COMMAND ++")
            print([self.py_exe, *args])
            print("++ STDOUT ++")
            print(out)
            print("++ STDERR ++")
            print(err)
        if allow_fail and p.returncode != expect_returncode:
            raise subprocess.CalledProcessError(p.returncode, [self.py_exe, *args], out, err)
        else:
            self.assertEqual(expect_returncode, p.returncode)
        data = {
            s.partition(":")[0]: s.partition(":")[2].lstrip()
            for s in err.splitlines()
            if not s.startswith("#") and ":" in s
        }
        data["stdout"] = out
        data["stderr"] = err
        return data

    def py_ini(self, content):
        local_appdata = os.environ.get("LOCALAPPDATA")
        if not local_appdata:
            raise unittest.SkipTest("LOCALAPPDATA environment variable is "
                                    "missing or empty")
        return PreservePyIni(Path(local_appdata) / "py.ini", content)

    @contextlib.contextmanager
    def script(self, content, encoding="utf-8"):
        file = Path(tempfile.mktemp(dir=os.getcwd()) + ".py")
        if isinstance(content, bytes):
            file.write_bytes(content)
        else:
            file.write_text(content, encoding=encoding)
        try:
            yield file
        finally:
            file.unlink()

    @contextlib.contextmanager
    def fake_venv(self):
        venv = Path.cwd() / "Scripts"
        venv.mkdir(exist_ok=True, parents=True)
        venv_exe = (venv / ("python_d.exe" if DEBUG_BUILD else "python.exe"))
        venv_exe.touch()
        try:
            yield venv_exe, {"VIRTUAL_ENV": str(venv.parent)}
        finally:
            shutil.rmtree(venv)


class TestLauncher(unittest.TestCase, RunPyMixin):
    @classmethod
    def setUpClass(cls):
        with winreg.CreateKey(winreg.HKEY_CURRENT_USER, rf"Software\Python") as key:
            create_registry_data(key, TEST_DATA)

        if support.verbose:
            p = subprocess.check_output("reg query HKCU\\Software\\Python /s")
            #print(p.decode('mbcs'))


    @classmethod
    def tearDownClass(cls):
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, rf"Software\Python", access=winreg.KEY_WRITE | winreg.KEY_ENUMERATE_SUB_KEYS) as key:
            delete_registry_data(key, TEST_DATA)


    def test_version(self):
        data = self.run_py(["-0"])
        self.assertEqual(self.py_exe, Path(data["argv0"]))
        self.assertEqual(sys.version.partition(" ")[0], data["version"])

    def test_help_option(self):
        data = self.run_py(["-h"])
        self.assertEqual("True", data["SearchInfo.help"])

    def test_list_option(self):
        for opt, v1, v2 in [
            ("-0", "True", "False"),
            ("-0p", "False", "True"),
            ("--list", "True", "False"),
            ("--list-paths", "False", "True"),
        ]:
            with self.subTest(opt):
                data = self.run_py([opt])
                self.assertEqual(v1, data["SearchInfo.list"])
                self.assertEqual(v2, data["SearchInfo.listPaths"])

    def test_list(self):
        data = self.run_py(["--list"])
        found = {}
        expect = {}
        for line in data["stdout"].splitlines():
            m = re.match(r"\s*(.+?)\s+?(\*\s+)?(.+)$", line)
            if m:
                found[m.group(1)] = m.group(3)
        for company in TEST_DATA:
            company_data = TEST_DATA[company]
            tags = [t for t in company_data if isinstance(company_data[t], dict)]
            for tag in tags:
                arg = f"-V:{company}/{tag}"
                expect[arg] = company_data[tag]["DisplayName"]
            expect.pop(f"-V:{company}/ignored", None)

        actual = {k: v for k, v in found.items() if k in expect}
        try:
            self.assertDictEqual(expect, actual)
        except:
            if support.verbose:
                print("*** STDOUT ***")
                print(data["stdout"])
            raise

    def test_list_paths(self):
        data = self.run_py(["--list-paths"])
        found = {}
        expect = {}
        for line in data["stdout"].splitlines():
            m = re.match(r"\s*(.+?)\s+?(\*\s+)?(.+)$", line)
            if m:
                found[m.group(1)] = m.group(3)
        for company in TEST_DATA:
            company_data = TEST_DATA[company]
            tags = [t for t in company_data if isinstance(company_data[t], dict)]
            for tag in tags:
                arg = f"-V:{company}/{tag}"
                install = company_data[tag]["InstallPath"]
                try:
                    expect[arg] = install["ExecutablePath"]
                    try:
                        expect[arg] += " " + install["ExecutableArguments"]
                    except KeyError:
                        pass
                except KeyError:
                    expect[arg] = str(Path(install[None]) / Path(sys.executable).name)

            expect.pop(f"-V:{company}/ignored", None)

        actual = {k: v for k, v in found.items() if k in expect}
        try:
            self.assertDictEqual(expect, actual)
        except:
            if support.verbose:
                print("*** STDOUT ***")
                print(data["stdout"])
            raise

    def test_filter_to_company(self):
        company = "PythonTestSuite"
        data = self.run_py([f"-V:{company}/"])
        self.assertEqual("X.Y.exe", data["LaunchCommand"])
        self.assertEqual(company, data["env.company"])
        self.assertEqual("3.100", data["env.tag"])

    def test_filter_to_company_with_default(self):
        company = "PythonTestSuite"
        data = self.run_py([f"-V:{company}/"], env=dict(PY_PYTHON="3.0"))
        self.assertEqual("X.Y.exe", data["LaunchCommand"])
        self.assertEqual(company, data["env.company"])
        self.assertEqual("3.100", data["env.tag"])

    def test_filter_to_tag(self):
        company = "PythonTestSuite"
        data = self.run_py(["-V:3.100"])
        self.assertEqual("X.Y.exe", data["LaunchCommand"])
        self.assertEqual(company, data["env.company"])
        self.assertEqual("3.100", data["env.tag"])

        data = self.run_py(["-V:3.100-32"])
        self.assertEqual("X.Y-32.exe", data["LaunchCommand"])
        self.assertEqual(company, data["env.company"])
        self.assertEqual("3.100-32", data["env.tag"])

        data = self.run_py(["-V:3.100-arm64"])
        self.assertEqual("X.Y-arm64.exe -X fake_arg_for_test", data["LaunchCommand"])
        self.assertEqual(company, data["env.company"])
        self.assertEqual("3.100-arm64", data["env.tag"])

    def test_filter_to_company_and_tag(self):
        company = "PythonTestSuite"
        data = self.run_py([f"-V:{company}/3.1"], expect_returncode=103)

        data = self.run_py([f"-V:{company}/3.100"])
        self.assertEqual("X.Y.exe", data["LaunchCommand"])
        self.assertEqual(company, data["env.company"])
        self.assertEqual("3.100", data["env.tag"])

    def test_filter_with_single_install(self):
        company = "PythonTestSuite1"
        data = self.run_py(
            ["-V:Nonexistent"],
            env={"PYLAUNCHER_LIMIT_TO_COMPANY": company},
            expect_returncode=103,
        )

    def test_search_major_3(self):
        try:
            data = self.run_py(["-3"], allow_fail=True)
        except subprocess.CalledProcessError:
            raise unittest.SkipTest("requires at least one Python 3.x install")
        self.assertEqual("PythonCore", data["env.company"])
        self.assertTrue(data["env.tag"].startswith("3."), data["env.tag"])

    def test_search_major_3_32(self):
        try:
            data = self.run_py(["-3-32"], allow_fail=True)
        except subprocess.CalledProcessError:
            if not any(is_installed(f"3.{i}-32") for i in range(5, 11)):
                raise unittest.SkipTest("requires at least one 32-bit Python 3.x install")
            raise
        self.assertEqual("PythonCore", data["env.company"])
        self.assertTrue(data["env.tag"].startswith("3."), data["env.tag"])
        self.assertTrue(data["env.tag"].endswith("-32"), data["env.tag"])

    def test_search_major_2(self):
        try:
            data = self.run_py(["-2"], allow_fail=True)
        except subprocess.CalledProcessError:
            if not is_installed("2.7"):
                raise unittest.SkipTest("requires at least one Python 2.x install")
        self.assertEqual("PythonCore", data["env.company"])
        self.assertTrue(data["env.tag"].startswith("2."), data["env.tag"])

    def test_py_default(self):
        with self.py_ini(TEST_PY_DEFAULTS):
            data = self.run_py(["-arg"])
        self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
        self.assertEqual("3.100", data["SearchInfo.tag"])
        self.assertEqual("X.Y.exe -arg", data["stdout"].strip())

    def test_py2_default(self):
        with self.py_ini(TEST_PY_DEFAULTS):
            data = self.run_py(["-2", "-arg"])
        self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
        self.assertEqual("3.100-32", data["SearchInfo.tag"])
        self.assertEqual("X.Y-32.exe -arg", data["stdout"].strip())

    def test_py3_default(self):
        with self.py_ini(TEST_PY_DEFAULTS):
            data = self.run_py(["-3", "-arg"])
        self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
        self.assertEqual("3.100-arm64", data["SearchInfo.tag"])
        self.assertEqual("X.Y-arm64.exe -X fake_arg_for_test -arg", data["stdout"].strip())

    def test_py_default_env(self):
        data = self.run_py(["-arg"], env=TEST_PY_ENV)
        self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
        self.assertEqual("3.100", data["SearchInfo.tag"])
        self.assertEqual("X.Y.exe -arg", data["stdout"].strip())

    def test_py2_default_env(self):
        data = self.run_py(["-2", "-arg"], env=TEST_PY_ENV)
        self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
        self.assertEqual("3.100-32", data["SearchInfo.tag"])
        self.assertEqual("X.Y-32.exe -arg", data["stdout"].strip())

    def test_py3_default_env(self):
        data = self.run_py(["-3", "-arg"], env=TEST_PY_ENV)
        self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
        self.assertEqual("3.100-arm64", data["SearchInfo.tag"])
        self.assertEqual("X.Y-arm64.exe -X fake_arg_for_test -arg", data["stdout"].strip())

    def test_py_default_short_argv0(self):
        with self.py_ini(TEST_PY_DEFAULTS):
            for argv0 in ['"py.exe"', 'py.exe', '"py"', 'py']:
                with self.subTest(argv0):
                    data = self.run_py(["--version"], argv=f'{argv0} --version')
                    self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
                    self.assertEqual("3.100", data["SearchInfo.tag"])
                    self.assertEqual("X.Y.exe --version", data["stdout"].strip())

    def test_py_default_in_list(self):
        data = self.run_py(["-0"], env=TEST_PY_ENV)
        default = None
        for line in data["stdout"].splitlines():
            m = re.match(r"\s*-V:(.+?)\s+?\*\s+(.+)$", line)
            if m:
                default = m.group(1)
                break
        self.assertEqual("PythonTestSuite/3.100", default)

    def test_virtualenv_in_list(self):
        with self.fake_venv() as (venv_exe, env):
            data = self.run_py(["-0p"], env=env)
            for line in data["stdout"].splitlines():
                m = re.match(r"\s*\*\s+(.+)$", line)
                if m:
                    self.assertEqual(str(venv_exe), m.group(1))
                    break
            else:
                if support.verbose:
                    print(data["stdout"])
                    print(data["stderr"])
                self.fail("did not find active venv path")

            data = self.run_py(["-0"], env=env)
            for line in data["stdout"].splitlines():
                m = re.match(r"\s*\*\s+(.+)$", line)
                if m:
                    self.assertEqual("Active venv", m.group(1))
                    break
            else:
                self.fail("did not find active venv entry")

    def test_virtualenv_with_env(self):
        with self.fake_venv() as (venv_exe, env):
            data1 = self.run_py([], env={**env, "PY_PYTHON": "PythonTestSuite/3"})
            data2 = self.run_py(["-V:PythonTestSuite/3"], env={**env, "PY_PYTHON": "PythonTestSuite/3"})
        # Compare stdout, because stderr goes via ascii
        self.assertEqual(data1["stdout"].strip(), quote(venv_exe))
        self.assertEqual(data1["SearchInfo.lowPriorityTag"], "True")
        # Ensure passing the argument doesn't trigger the same behaviour
        self.assertNotEqual(data2["stdout"].strip(), quote(venv_exe))
        self.assertNotEqual(data2["SearchInfo.lowPriorityTag"], "True")

    def test_py_shebang(self):
        with self.py_ini(TEST_PY_DEFAULTS):
            with self.script("#! /usr/bin/python -prearg") as script:
                data = self.run_py([script, "-postarg"])
        self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
        self.assertEqual("3.100", data["SearchInfo.tag"])
        self.assertEqual(f"X.Y.exe -prearg {quote(script)} -postarg", data["stdout"].strip())

    def test_python_shebang(self):
        with self.py_ini(TEST_PY_DEFAULTS):
            with self.script("#! python -prearg") as script:
                data = self.run_py([script, "-postarg"])
        self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
        self.assertEqual("3.100", data["SearchInfo.tag"])
        self.assertEqual(f"X.Y.exe -prearg {quote(script)} -postarg", data["stdout"].strip())

    def test_py2_shebang(self):
        with self.py_ini(TEST_PY_DEFAULTS):
            with self.script("#! /usr/bin/python2 -prearg") as script:
                data = self.run_py([script, "-postarg"])
        self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
        self.assertEqual("3.100-32", data["SearchInfo.tag"])
        self.assertEqual(f"X.Y-32.exe -prearg {quote(script)} -postarg",
                         data["stdout"].strip())

    def test_py3_shebang(self):
        with self.py_ini(TEST_PY_DEFAULTS):
            with self.script("#! /usr/bin/python3 -prearg") as script:
                data = self.run_py([script, "-postarg"])
        self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
        self.assertEqual("3.100-arm64", data["SearchInfo.tag"])
        self.assertEqual(f"X.Y-arm64.exe -X fake_arg_for_test -prearg {quote(script)} -postarg",
                         data["stdout"].strip())

    def test_py_shebang_nl(self):
        with self.py_ini(TEST_PY_DEFAULTS):
            with self.script("#! /usr/bin/python -prearg\n") as script:
                data = self.run_py([script, "-postarg"])
        self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
        self.assertEqual("3.100", data["SearchInfo.tag"])
        self.assertEqual(f"X.Y.exe -prearg {quote(script)} -postarg",
                         data["stdout"].strip())

    def test_py2_shebang_nl(self):
        with self.py_ini(TEST_PY_DEFAULTS):
            with self.script("#! /usr/bin/python2 -prearg\n") as script:
                data = self.run_py([script, "-postarg"])
        self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
        self.assertEqual("3.100-32", data["SearchInfo.tag"])
        self.assertEqual(f"X.Y-32.exe -prearg {quote(script)} -postarg",
                         data["stdout"].strip())

    def test_py3_shebang_nl(self):
        with self.py_ini(TEST_PY_DEFAULTS):
            with self.script("#! /usr/bin/python3 -prearg\n") as script:
                data = self.run_py([script, "-postarg"])
        self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
        self.assertEqual("3.100-arm64", data["SearchInfo.tag"])
        self.assertEqual(f"X.Y-arm64.exe -X fake_arg_for_test -prearg {quote(script)} -postarg",
                         data["stdout"].strip())

    def test_py_shebang_short_argv0(self):
        with self.py_ini(TEST_PY_DEFAULTS):
            with self.script("#! /usr/bin/python -prearg") as script:
                # Override argv to only pass "py.exe" as the command
                data = self.run_py([script, "-postarg"], argv=f'"py.exe" "{script}" -postarg')
        self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
        self.assertEqual("3.100", data["SearchInfo.tag"])
        self.assertEqual(f'X.Y.exe -prearg "{script}" -postarg', data["stdout"].strip())

    def test_py_shebang_valid_bom(self):
        with self.py_ini(TEST_PY_DEFAULTS):
            content = "#! /usr/bin/python -prearg".encode("utf-8")
            with self.script(b"\xEF\xBB\xBF" + content) as script:
                data = self.run_py([script, "-postarg"])
        self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
        self.assertEqual("3.100", data["SearchInfo.tag"])
        self.assertEqual(f"X.Y.exe -prearg {quote(script)} -postarg", data["stdout"].strip())

    def test_py_shebang_invalid_bom(self):
        with self.py_ini(TEST_PY_DEFAULTS):
            content = "#! /usr/bin/python3 -prearg".encode("utf-8")
            with self.script(b"\xEF\xAA\xBF" + content) as script:
                data = self.run_py([script, "-postarg"])
        self.assertIn("Invalid BOM", data["stderr"])
        self.assertEqual("PythonTestSuite", data["SearchInfo.company"])
        self.assertEqual("3.100", data["SearchInfo.tag"])
        self.assertEqual(f"X.Y.exe {quote(script)} -postarg", data["stdout"].strip())

    def test_py_handle_64_in_ini(self):
        with self.py_ini("\n".join(["[defaults]", "python=3.999-64"])):
            # Expect this to fail, but should get oldStyleTag flipped on
            data = self.run_py([], allow_fail=True, expect_returncode=103)
        self.assertEqual("3.999-64", data["SearchInfo.tag"])
        self.assertEqual("True", data["SearchInfo.oldStyleTag"])

    def test_search_path(self):
        exe = Path("arbitrary-exe-name.exe").absolute()
        exe.touch()
        self.addCleanup(exe.unlink)
        with self.py_ini(TEST_PY_DEFAULTS):
            with self.script(f"#! /usr/bin/env {exe.stem} -prearg") as script:
                data = self.run_py(
                    [script, "-postarg"],
                    env={"PATH": f"{exe.parent};{os.getenv('PATH')}"},
                )
        self.assertEqual(f"{quote(exe)} -prearg {quote(script)} -postarg",
                         data["stdout"].strip())

    def test_search_path_exe(self):
        # Leave the .exe on the name to ensure we don't add it a second time
        exe = Path("arbitrary-exe-name.exe").absolute()
        exe.touch()
        self.addCleanup(exe.unlink)
        with self.py_ini(TEST_PY_DEFAULTS):
            with self.script(f"#! /usr/bin/env {exe.name} -prearg") as script:
                data = self.run_py(
                    [script, "-postarg"],
                    env={"PATH": f"{exe.parent};{os.getenv('PATH')}"},
                )
        self.assertEqual(f"{quote(exe)} -prearg {quote(script)} -postarg",
                         data["stdout"].strip())

    def test_recursive_search_path(self):
        stem = self.get_py_exe().stem
        with self.py_ini(TEST_PY_DEFAULTS):
            with self.script(f"#! /usr/bin/env {stem}") as script:
                data = self.run_py(
                    [script],
                    env={"PATH": f"{self.get_py_exe().parent};{os.getenv('PATH')}"},
                )
        # The recursive search is ignored and we get normal "py" behavior
        self.assertEqual(f"X.Y.exe {quote(script)}", data["stdout"].strip())

    def test_install(self):
        data = self.run_py(["-V:3.10"], env={"PYLAUNCHER_ALWAYS_INSTALL": "1"}, expect_returncode=111)
        cmd = data["stdout"].strip()
        # If winget is runnable, we should find it. Otherwise, we'll be trying
        # to open the Store.
        try:
            subprocess.check_call(["winget.exe", "--version"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        except FileNotFoundError:
            self.assertIn("ms-windows-store://", cmd)
        else:
            self.assertIn("winget.exe", cmd)
        # Both command lines include the store ID
        self.assertIn("9PJPW5LDXLZ5", cmd)

    def test_literal_shebang_absolute(self):
        with self.script("#! C:/some_random_app -witharg") as script:
            data = self.run_py([script])
        self.assertEqual(
            f"C:\\some_random_app -witharg {quote(script)}",
            data["stdout"].strip(),
        )

    def test_literal_shebang_relative(self):
        with self.script("#! ..\\some_random_app -witharg") as script:
            data = self.run_py([script])
        self.assertEqual(
            f"{quote(script.parent.parent / 'some_random_app')} -witharg {quote(script)}",
            data["stdout"].strip(),
        )

    def test_literal_shebang_quoted(self):
        with self.script('#! "some random app" -witharg') as script:
            data = self.run_py([script])
        self.assertEqual(
            f"{quote(script.parent / 'some random app')} -witharg {quote(script)}",
            data["stdout"].strip(),
        )

        with self.script('#! some" random "app -witharg') as script:
            data = self.run_py([script])
        self.assertEqual(
            f"{quote(script.parent / 'some random app')} -witharg {quote(script)}",
            data["stdout"].strip(),
        )

    def test_literal_shebang_quoted_escape(self):
        with self.script('#! some\\" random "app -witharg') as script:
            data = self.run_py([script])
        self.assertEqual(
            f"{quote(script.parent / 'some/ random app')} -witharg {quote(script)}",
            data["stdout"].strip(),
        )

    def test_literal_shebang_command(self):
        with self.py_ini(TEST_PY_COMMANDS):
            with self.script('#! test-command arg1') as script:
                data = self.run_py([script])
        self.assertEqual(
            f"TEST_EXE.exe arg1 {quote(script)}",
            data["stdout"].strip(),
        )

    def test_literal_shebang_invalid_template(self):
        with self.script('#! /usr/bin/not-python arg1') as script:
            data = self.run_py([script])
        expect = script.parent / "/usr/bin/not-python"
        self.assertEqual(
            f"{quote(expect)} arg1 {quote(script)}",
            data["stdout"].strip(),
        )

    def test_shebang_command_in_venv(self):
        stem = "python-that-is-not-on-path"

        # First ensure that our test name doesn't exist, and the launcher does
        # not match any installed env
        with self.script(f'#! /usr/bin/env {stem} arg1') as script:
            data = self.run_py([script], expect_returncode=103)

        with self.fake_venv() as (venv_exe, env):
            # Put a "normal" Python on PATH as a distraction.
            # The active VIRTUAL_ENV should be preferred when the name isn't an
            # exact match.
            exe = Path(Path(venv_exe).name).absolute()
            exe.touch()
            self.addCleanup(exe.unlink)
            env["PATH"] = f"{exe.parent};{os.environ['PATH']}"

            with self.script(f'#! /usr/bin/env {stem} arg1') as script:
                data = self.run_py([script], env=env)
            self.assertEqual(data["stdout"].strip(), f"{quote(venv_exe)} arg1 {quote(script)}")

            with self.script(f'#! /usr/bin/env {exe.stem} arg1') as script:
                data = self.run_py([script], env=env)
            self.assertEqual(data["stdout"].strip(), f"{quote(exe)} arg1 {quote(script)}")

    def test_shebang_executable_extension(self):
        with self.script('#! /usr/bin/env python3.99') as script:
            data = self.run_py([script], expect_returncode=103)
        expect = "# Search PATH for python3.99.exe"
        actual = [line.strip() for line in data["stderr"].splitlines()
                  if line.startswith("# Search PATH")]
        self.assertEqual([expect], actual)
