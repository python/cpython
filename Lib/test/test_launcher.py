import itertools
import os
import re
import subprocess
import sys
import sysconfig
import unittest
from pathlib import Path
from test import support

if sys.platform != "win32":
    raise unittest.SkipTest("test only applies to Windows")

# Get winreg after the platform check
import winreg


PY_EXE = "py.exe"
if sys.executable.casefold().endswith("_d.exe".casefold()):
    PY_EXE = "py_d.exe"

# Registry data to create. On removal, everything beneath top-level names will
# be deleted.
TEST_DATA = {
    "PythonTestSuite": {
        "DisplayName": "Python Test Suite",
        "SupportUrl": "https://www.python.org/",
        "{0.major}.{0.minor}".format(sys.version_info): {
            "DisplayName": "X.Y version",
            "InstallPath": {
                None: sys.prefix,
            }
        },
        "{0.major}.{0.minor}-32".format(sys.version_info): {
            "DisplayName": "X.Y-32 version",
            "InstallPath": {
                None: sys.prefix,
            }
        },
        "{0.major}.{0.minor}-arm64".format(sys.version_info): {
            "DisplayName": "X.Y-arm64 version",
            "InstallPath": {
                None: sys.prefix,
                "ExecutablePath": sys.executable,
                "ExecutableArguments": "-X fake_arg_for_test",
            }
        },
    }
}


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


class TestLauncher(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.py_exe = None

    def setUp(self):
        py_exe = None
        if sysconfig.is_python_build(True):
            py_exe = Path(sys.executable).parent / PY_EXE
        else:
            for p in os.getenv("PATH").split(";"):
                if p:
                    py_exe = Path(p) / PY_EXE
                    if py_exe.is_file():
                        break
        if not py_exe:
            raise unittest.SkipTest(
                "cannot locate '{}' for test".format(PY_EXE)
            )
        self.py_exe = py_exe

        with winreg.CreateKey(winreg.HKEY_CURRENT_USER, rf"Software\Python") as key:
            create_registry_data(key, TEST_DATA)


    def tearDown(self):
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, rf"Software\Python", access=winreg.KEY_WRITE | winreg.KEY_ENUMERATE_SUB_KEYS) as key:
            delete_registry_data(key, TEST_DATA)


    def run_py(self, args, env=None):
        env = {**os.environ, **(env or {}), "PYLAUNCHER_DEBUG": "1", "PYLAUNCHER_DRYRUN": "1"}
        with subprocess.Popen(
            [self.py_exe, *args],
            env=env,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        ) as p:
            p.stdin.close()
            p.wait(10)
            out = p.stdout.read().decode("ascii", "replace")
            err = p.stderr.read().decode("ascii", "replace")
        if p.returncode and support.verbose:
            print("++ COMMAND ++")
            print([self.py_exe, *args])
            print("++ STDOUT ++")
            print(out)
            print("++ STDERR ++")
            print(err)
        self.assertEqual(0, p.returncode)
        data = {
            s.partition(":")[0]: s.partition(":")[2].lstrip()
            for s in err.splitlines()
            if not s.startswith("#") and ":" in s
        }
        data["stdout"] = out
        data["stderr"] = err
        return data


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
        data = self.run_py(["--list-paths"])
        found = {}
        expect = {}
        for line in data["stdout"].splitlines():
            m = re.match(r"\s*(.+?)\s+(.+)$", line)
            if m:
                found[m.group(1)] = m.group(2)
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

        actual = {k: v for k, v in found.items() if k in expect}
        self.assertDictEqual(expect, actual)
