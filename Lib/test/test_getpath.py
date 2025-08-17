import copy
import ntpath
import pathlib
import posixpath
import unittest

from test.support import verbose

try:
    # If we are in a source tree, use the original source file for tests
    SOURCE = (pathlib.Path(__file__).absolute().parent.parent.parent / "Modules/getpath.py").read_bytes()
except FileNotFoundError:
    # Try from _testcapimodule instead
    from _testinternalcapi import get_getpath_codeobject
    SOURCE = get_getpath_codeobject()


class MockGetPathTests(unittest.TestCase):
    def __init__(self, *a, **kw):
        super().__init__(*a, **kw)
        self.maxDiff = None

    def test_normal_win32(self):
        "Test a 'standard' install layout on Windows."
        ns = MockNTNamespace(
            argv0=r"C:\Python\python.exe",
            real_executable=r"C:\Python\python.exe",
        )
        ns.add_known_xfile(r"C:\Python\python.exe")
        ns.add_known_file(r"C:\Python\Lib\os.py")
        ns.add_known_dir(r"C:\Python\DLLs")
        expected = dict(
            executable=r"C:\Python\python.exe",
            base_executable=r"C:\Python\python.exe",
            prefix=r"C:\Python",
            exec_prefix=r"C:\Python",
            module_search_paths_set=1,
            module_search_paths=[
                r"C:\Python\python98.zip",
                r"C:\Python\DLLs",
                r"C:\Python\Lib",
                r"C:\Python",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_buildtree_win32(self):
        "Test an in-build-tree layout on Windows."
        ns = MockNTNamespace(
            argv0=r"C:\CPython\PCbuild\amd64\python.exe",
            real_executable=r"C:\CPython\PCbuild\amd64\python.exe",
        )
        ns.add_known_xfile(r"C:\CPython\PCbuild\amd64\python.exe")
        ns.add_known_file(r"C:\CPython\Lib\os.py")
        ns.add_known_file(r"C:\CPython\PCbuild\amd64\pybuilddir.txt", [""])
        expected = dict(
            executable=r"C:\CPython\PCbuild\amd64\python.exe",
            base_executable=r"C:\CPython\PCbuild\amd64\python.exe",
            prefix=r"C:\CPython",
            exec_prefix=r"C:\CPython",
            build_prefix=r"C:\CPython",
            _is_python_build=1,
            module_search_paths_set=1,
            module_search_paths=[
                r"C:\CPython\PCbuild\amd64\python98.zip",
                r"C:\CPython\PCbuild\amd64",
                r"C:\CPython\Lib",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_venv_win32(self):
        """Test a venv layout on Windows.

        This layout is discovered by the presence of %__PYVENV_LAUNCHER__%,
        specifying the original launcher executable. site.py is responsible
        for updating prefix and exec_prefix.
        """
        ns = MockNTNamespace(
            argv0=r"C:\Python\python.exe",
            ENV___PYVENV_LAUNCHER__=r"C:\venv\Scripts\python.exe",
            real_executable=r"C:\Python\python.exe",
        )
        ns.add_known_xfile(r"C:\Python\python.exe")
        ns.add_known_xfile(r"C:\venv\Scripts\python.exe")
        ns.add_known_file(r"C:\Python\Lib\os.py")
        ns.add_known_dir(r"C:\Python\DLLs")
        ns.add_known_file(r"C:\venv\pyvenv.cfg", [
            r"home = C:\Python"
        ])
        expected = dict(
            executable=r"C:\venv\Scripts\python.exe",
            prefix=r"C:\venv",
            exec_prefix=r"C:\venv",
            base_executable=r"C:\Python\python.exe",
            base_prefix=r"C:\Python",
            base_exec_prefix=r"C:\Python",
            module_search_paths_set=1,
            module_search_paths=[
                r"C:\Python\python98.zip",
                r"C:\Python\DLLs",
                r"C:\Python\Lib",
                r"C:\Python",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_registry_win32(self):
        """Test registry lookup on Windows.

        On Windows there are registry entries that are intended for other
        applications to register search paths.
        """
        hkey = rf"HKLM\Software\Python\PythonCore\9.8-XY\PythonPath"
        winreg = MockWinreg({
            hkey: None,
            f"{hkey}\\Path1": "path1-dir",
            f"{hkey}\\Path1\\Subdir": "not-subdirs",
        })
        ns = MockNTNamespace(
            argv0=r"C:\Python\python.exe",
            real_executable=r"C:\Python\python.exe",
            winreg=winreg,
        )
        ns.add_known_xfile(r"C:\Python\python.exe")
        ns.add_known_file(r"C:\Python\Lib\os.py")
        ns.add_known_dir(r"C:\Python\DLLs")
        expected = dict(
            module_search_paths_set=1,
            module_search_paths=[
                r"C:\Python\python98.zip",
                "path1-dir",
                # should not contain not-subdirs
                r"C:\Python\DLLs",
                r"C:\Python\Lib",
                r"C:\Python",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

        ns["config"]["use_environment"] = 0
        ns["config"]["module_search_paths_set"] = 0
        ns["config"]["module_search_paths"] = None
        expected = dict(
            module_search_paths_set=1,
            module_search_paths=[
                r"C:\Python\python98.zip",
                r"C:\Python\DLLs",
                r"C:\Python\Lib",
                r"C:\Python",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_symlink_normal_win32(self):
        "Test a 'standard' install layout via symlink on Windows."
        ns = MockNTNamespace(
            argv0=r"C:\LinkedFrom\python.exe",
            real_executable=r"C:\Python\python.exe",
        )
        ns.add_known_xfile(r"C:\LinkedFrom\python.exe")
        ns.add_known_xfile(r"C:\Python\python.exe")
        ns.add_known_link(r"C:\LinkedFrom\python.exe", r"C:\Python\python.exe")
        ns.add_known_file(r"C:\Python\Lib\os.py")
        ns.add_known_dir(r"C:\Python\DLLs")
        expected = dict(
            executable=r"C:\LinkedFrom\python.exe",
            base_executable=r"C:\LinkedFrom\python.exe",
            prefix=r"C:\Python",
            exec_prefix=r"C:\Python",
            module_search_paths_set=1,
            module_search_paths=[
                r"C:\Python\python98.zip",
                r"C:\Python\DLLs",
                r"C:\Python\Lib",
                r"C:\Python",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_symlink_buildtree_win32(self):
        "Test an in-build-tree layout via symlink on Windows."
        ns = MockNTNamespace(
            argv0=r"C:\LinkedFrom\python.exe",
            real_executable=r"C:\CPython\PCbuild\amd64\python.exe",
        )
        ns.add_known_xfile(r"C:\LinkedFrom\python.exe")
        ns.add_known_xfile(r"C:\CPython\PCbuild\amd64\python.exe")
        ns.add_known_link(r"C:\LinkedFrom\python.exe", r"C:\CPython\PCbuild\amd64\python.exe")
        ns.add_known_file(r"C:\CPython\Lib\os.py")
        ns.add_known_file(r"C:\CPython\PCbuild\amd64\pybuilddir.txt", [""])
        expected = dict(
            executable=r"C:\LinkedFrom\python.exe",
            base_executable=r"C:\LinkedFrom\python.exe",
            prefix=r"C:\CPython",
            exec_prefix=r"C:\CPython",
            build_prefix=r"C:\CPython",
            _is_python_build=1,
            module_search_paths_set=1,
            module_search_paths=[
                r"C:\CPython\PCbuild\amd64\python98.zip",
                r"C:\CPython\PCbuild\amd64",
                r"C:\CPython\Lib",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_buildtree_pythonhome_win32(self):
        "Test an out-of-build-tree layout on Windows with PYTHONHOME override."
        ns = MockNTNamespace(
            argv0=r"C:\Out\python.exe",
            real_executable=r"C:\Out\python.exe",
            ENV_PYTHONHOME=r"C:\CPython",
        )
        ns.add_known_xfile(r"C:\Out\python.exe")
        ns.add_known_file(r"C:\CPython\Lib\os.py")
        ns.add_known_file(r"C:\Out\pybuilddir.txt", [""])
        expected = dict(
            executable=r"C:\Out\python.exe",
            base_executable=r"C:\Out\python.exe",
            prefix=r"C:\CPython",
            exec_prefix=r"C:\CPython",
            # This build_prefix is a miscalculation, because we have
            # moved the output direction out of the prefix.
            # Specify PYTHONHOME to get the correct prefix/exec_prefix
            build_prefix="C:\\",
            _is_python_build=1,
            module_search_paths_set=1,
            module_search_paths=[
                r"C:\Out\python98.zip",
                r"C:\Out",
                r"C:\CPython\Lib",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_no_dlls_win32(self):
        "Test a layout on Windows with no DLLs directory."
        ns = MockNTNamespace(
            argv0=r"C:\Python\python.exe",
            real_executable=r"C:\Python\python.exe",
        )
        ns.add_known_xfile(r"C:\Python\python.exe")
        ns.add_known_file(r"C:\Python\Lib\os.py")
        expected = dict(
            executable=r"C:\Python\python.exe",
            base_executable=r"C:\Python\python.exe",
            prefix=r"C:\Python",
            exec_prefix=r"C:\Python",
            module_search_paths_set=1,
            module_search_paths=[
                r"C:\Python\python98.zip",
                r"C:\Python",
                r"C:\Python\Lib",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_normal_posix(self):
        "Test a 'standard' install layout on *nix"
        ns = MockPosixNamespace(
            PREFIX="/usr",
            argv0="python",
            ENV_PATH="/usr/bin",
        )
        ns.add_known_xfile("/usr/bin/python")
        ns.add_known_file("/usr/lib/python9.8/os.py")
        ns.add_known_dir("/usr/lib/python9.8/lib-dynload")
        expected = dict(
            executable="/usr/bin/python",
            base_executable="/usr/bin/python",
            prefix="/usr",
            exec_prefix="/usr",
            module_search_paths_set=1,
            module_search_paths=[
                "/usr/lib/python98.zip",
                "/usr/lib/python9.8",
                "/usr/lib/python9.8/lib-dynload",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_buildpath_posix(self):
        """Test an in-build-tree layout on POSIX.

        This layout is discovered from the presence of pybuilddir.txt, which
        contains the relative path from the executable's directory to the
        platstdlib path.
        """
        ns = MockPosixNamespace(
            argv0=r"/home/cpython/python",
            PREFIX="/usr/local",
        )
        ns.add_known_xfile("/home/cpython/python")
        ns.add_known_xfile("/usr/local/bin/python")
        ns.add_known_file("/home/cpython/pybuilddir.txt", ["build/lib.linux-x86_64-9.8"])
        ns.add_known_file("/home/cpython/Lib/os.py")
        ns.add_known_dir("/home/cpython/lib-dynload")
        expected = dict(
            executable="/home/cpython/python",
            prefix="/usr/local",
            exec_prefix="/usr/local",
            base_executable="/home/cpython/python",
            build_prefix="/home/cpython",
            _is_python_build=1,
            module_search_paths_set=1,
            module_search_paths=[
                "/usr/local/lib/python98.zip",
                "/home/cpython/Lib",
                "/home/cpython/build/lib.linux-x86_64-9.8",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_venv_posix(self):
        "Test a venv layout on *nix."
        ns = MockPosixNamespace(
            argv0="python",
            PREFIX="/usr",
            ENV_PATH="/venv/bin:/usr/bin",
        )
        ns.add_known_xfile("/usr/bin/python")
        ns.add_known_xfile("/venv/bin/python")
        ns.add_known_file("/usr/lib/python9.8/os.py")
        ns.add_known_dir("/usr/lib/python9.8/lib-dynload")
        ns.add_known_file("/venv/pyvenv.cfg", [
            r"home = /usr/bin"
        ])
        expected = dict(
            executable="/venv/bin/python",
            prefix="/venv",
            exec_prefix="/venv",
            base_executable="/usr/bin/python",
            base_prefix="/usr",
            base_exec_prefix="/usr",
            module_search_paths_set=1,
            module_search_paths=[
                "/usr/lib/python98.zip",
                "/usr/lib/python9.8",
                "/usr/lib/python9.8/lib-dynload",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_venv_posix_without_home_key(self):
        ns = MockPosixNamespace(
            argv0="/venv/bin/python3",
            PREFIX="/usr",
            ENV_PATH="/usr/bin",
        )
        # Setup the bare minimum venv
        ns.add_known_xfile("/usr/bin/python3")
        ns.add_known_xfile("/venv/bin/python3")
        ns.add_known_link("/venv/bin/python3", "/usr/bin/python3")
        ns.add_known_file("/venv/pyvenv.cfg", [
            # home = key intentionally omitted
        ])
        expected = dict(
            executable="/venv/bin/python3",
            prefix="/venv",
            base_prefix="/usr",
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_venv_changed_name_posix(self):
        "Test a venv layout on *nix."
        ns = MockPosixNamespace(
            argv0="python",
            PREFIX="/usr",
            ENV_PATH="/venv/bin:/usr/bin",
        )
        ns.add_known_xfile("/usr/bin/python3")
        ns.add_known_xfile("/venv/bin/python")
        ns.add_known_link("/venv/bin/python", "/usr/bin/python3")
        ns.add_known_file("/usr/lib/python9.8/os.py")
        ns.add_known_dir("/usr/lib/python9.8/lib-dynload")
        ns.add_known_file("/venv/pyvenv.cfg", [
            r"home = /usr/bin"
        ])
        expected = dict(
            executable="/venv/bin/python",
            prefix="/venv",
            exec_prefix="/venv",
            base_executable="/usr/bin/python3",
            base_prefix="/usr",
            base_exec_prefix="/usr",
            module_search_paths_set=1,
            module_search_paths=[
                "/usr/lib/python98.zip",
                "/usr/lib/python9.8",
                "/usr/lib/python9.8/lib-dynload",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_venv_non_installed_zip_path_posix(self):
        "Test a venv created from non-installed python has correct zip path."""
        ns = MockPosixNamespace(
            argv0="/venv/bin/python",
            PREFIX="/usr",
            ENV_PATH="/venv/bin:/usr/bin",
        )
        ns.add_known_xfile("/path/to/non-installed/bin/python")
        ns.add_known_xfile("/venv/bin/python")
        ns.add_known_link("/venv/bin/python",
                          "/path/to/non-installed/bin/python")
        ns.add_known_file("/path/to/non-installed/lib/python9.8/os.py")
        ns.add_known_dir("/path/to/non-installed/lib/python9.8/lib-dynload")
        ns.add_known_file("/venv/pyvenv.cfg", [
            r"home = /path/to/non-installed"
        ])
        expected = dict(
            executable="/venv/bin/python",
            prefix="/venv",
            exec_prefix="/venv",
            base_executable="/path/to/non-installed/bin/python",
            base_prefix="/path/to/non-installed",
            base_exec_prefix="/path/to/non-installed",
            module_search_paths_set=1,
            module_search_paths=[
                "/path/to/non-installed/lib/python98.zip",
                "/path/to/non-installed/lib/python9.8",
                "/path/to/non-installed/lib/python9.8/lib-dynload",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_venv_changed_name_copy_posix(self):
        "Test a venv --copies layout on *nix that lacks a distributed 'python'"
        ns = MockPosixNamespace(
            argv0="python",
            PREFIX="/usr",
            ENV_PATH="/venv/bin:/usr/bin",
        )
        ns.add_known_xfile("/usr/bin/python9")
        ns.add_known_xfile("/venv/bin/python")
        ns.add_known_file("/usr/lib/python9.8/os.py")
        ns.add_known_dir("/usr/lib/python9.8/lib-dynload")
        ns.add_known_file("/venv/pyvenv.cfg", [
            r"home = /usr/bin"
        ])
        expected = dict(
            executable="/venv/bin/python",
            prefix="/venv",
            exec_prefix="/venv",
            base_executable="/usr/bin/python9",
            base_prefix="/usr",
            base_exec_prefix="/usr",
            module_search_paths_set=1,
            module_search_paths=[
                "/usr/lib/python98.zip",
                "/usr/lib/python9.8",
                "/usr/lib/python9.8/lib-dynload",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_symlink_normal_posix(self):
        "Test a 'standard' install layout via symlink on *nix"
        ns = MockPosixNamespace(
            PREFIX="/usr",
            argv0="/linkfrom/python",
        )
        ns.add_known_xfile("/linkfrom/python")
        ns.add_known_xfile("/usr/bin/python")
        ns.add_known_link("/linkfrom/python", "/usr/bin/python")
        ns.add_known_file("/usr/lib/python9.8/os.py")
        ns.add_known_dir("/usr/lib/python9.8/lib-dynload")
        expected = dict(
            executable="/linkfrom/python",
            base_executable="/linkfrom/python",
            prefix="/usr",
            exec_prefix="/usr",
            module_search_paths_set=1,
            module_search_paths=[
                "/usr/lib/python98.zip",
                "/usr/lib/python9.8",
                "/usr/lib/python9.8/lib-dynload",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_symlink_buildpath_posix(self):
        """Test an in-build-tree layout on POSIX.

        This layout is discovered from the presence of pybuilddir.txt, which
        contains the relative path from the executable's directory to the
        platstdlib path.
        """
        ns = MockPosixNamespace(
            argv0=r"/linkfrom/python",
            PREFIX="/usr/local",
        )
        ns.add_known_xfile("/linkfrom/python")
        ns.add_known_xfile("/home/cpython/python")
        ns.add_known_link("/linkfrom/python", "/home/cpython/python")
        ns.add_known_xfile("/usr/local/bin/python")
        ns.add_known_file("/home/cpython/pybuilddir.txt", ["build/lib.linux-x86_64-9.8"])
        ns.add_known_file("/home/cpython/Lib/os.py")
        ns.add_known_dir("/home/cpython/lib-dynload")
        expected = dict(
            executable="/linkfrom/python",
            prefix="/usr/local",
            exec_prefix="/usr/local",
            base_executable="/linkfrom/python",
            build_prefix="/home/cpython",
            _is_python_build=1,
            module_search_paths_set=1,
            module_search_paths=[
                "/usr/local/lib/python98.zip",
                "/home/cpython/Lib",
                "/home/cpython/build/lib.linux-x86_64-9.8",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_custom_platlibdir_posix(self):
        "Test an install with custom platlibdir on *nix"
        ns = MockPosixNamespace(
            PREFIX="/usr",
            argv0="/linkfrom/python",
            PLATLIBDIR="lib64",
        )
        ns.add_known_xfile("/usr/bin/python")
        ns.add_known_file("/usr/lib64/python9.8/os.py")
        ns.add_known_dir("/usr/lib64/python9.8/lib-dynload")
        expected = dict(
            executable="/linkfrom/python",
            base_executable="/linkfrom/python",
            prefix="/usr",
            exec_prefix="/usr",
            module_search_paths_set=1,
            module_search_paths=[
                "/usr/lib64/python98.zip",
                "/usr/lib64/python9.8",
                "/usr/lib64/python9.8/lib-dynload",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_framework_macos(self):
        """ Test framework layout on macOS

        This layout is primarily detected using a compile-time option
        (WITH_NEXT_FRAMEWORK).
        """
        ns = MockPosixNamespace(
            os_name="darwin",
            argv0="/Library/Frameworks/Python.framework/Versions/9.8/Resources/Python.app/Contents/MacOS/Python",
            WITH_NEXT_FRAMEWORK=1,
            PREFIX="/Library/Frameworks/Python.framework/Versions/9.8",
            EXEC_PREFIX="/Library/Frameworks/Python.framework/Versions/9.8",
            ENV___PYVENV_LAUNCHER__="/Library/Frameworks/Python.framework/Versions/9.8/bin/python9.8",
            real_executable="/Library/Frameworks/Python.framework/Versions/9.8/Resources/Python.app/Contents/MacOS/Python",
            library="/Library/Frameworks/Python.framework/Versions/9.8/Python",
        )
        ns.add_known_xfile("/Library/Frameworks/Python.framework/Versions/9.8/Resources/Python.app/Contents/MacOS/Python")
        ns.add_known_xfile("/Library/Frameworks/Python.framework/Versions/9.8/bin/python9.8")
        ns.add_known_dir("/Library/Frameworks/Python.framework/Versions/9.8/lib/python9.8/lib-dynload")
        ns.add_known_file("/Library/Frameworks/Python.framework/Versions/9.8/lib/python9.8/os.py")

        # This is definitely not the stdlib (see discussion in bpo-46890)
        #ns.add_known_file("/Library/Frameworks/lib/python98.zip")

        expected = dict(
            executable="/Library/Frameworks/Python.framework/Versions/9.8/bin/python9.8",
            prefix="/Library/Frameworks/Python.framework/Versions/9.8",
            exec_prefix="/Library/Frameworks/Python.framework/Versions/9.8",
            base_executable="/Library/Frameworks/Python.framework/Versions/9.8/bin/python9.8",
            base_prefix="/Library/Frameworks/Python.framework/Versions/9.8",
            base_exec_prefix="/Library/Frameworks/Python.framework/Versions/9.8",
            module_search_paths_set=1,
            module_search_paths=[
                "/Library/Frameworks/Python.framework/Versions/9.8/lib/python98.zip",
                "/Library/Frameworks/Python.framework/Versions/9.8/lib/python9.8",
                "/Library/Frameworks/Python.framework/Versions/9.8/lib/python9.8/lib-dynload",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_alt_framework_macos(self):
        """ Test framework layout on macOS with alternate framework name

        ``--with-framework-name=DebugPython``

        This layout is primarily detected using a compile-time option
        (WITH_NEXT_FRAMEWORK).
        """
        ns = MockPosixNamespace(
            argv0="/Library/Frameworks/DebugPython.framework/Versions/9.8/Resources/Python.app/Contents/MacOS/DebugPython",
            os_name="darwin",
            WITH_NEXT_FRAMEWORK=1,
            PREFIX="/Library/Frameworks/DebugPython.framework/Versions/9.8",
            EXEC_PREFIX="/Library/Frameworks/DebugPython.framework/Versions/9.8",
            ENV___PYVENV_LAUNCHER__="/Library/Frameworks/DebugPython.framework/Versions/9.8/bin/python9.8",
            real_executable="/Library/Frameworks/DebugPython.framework/Versions/9.8/Resources/Python.app/Contents/MacOS/DebugPython",
            library="/Library/Frameworks/DebugPython.framework/Versions/9.8/DebugPython",
            PYTHONPATH=None,
            ENV_PYTHONHOME=None,
            ENV_PYTHONEXECUTABLE=None,
            executable_dir=None,
            py_setpath=None,
        )
        ns.add_known_xfile("/Library/Frameworks/DebugPython.framework/Versions/9.8/Resources/Python.app/Contents/MacOS/DebugPython")
        ns.add_known_xfile("/Library/Frameworks/DebugPython.framework/Versions/9.8/bin/python9.8")
        ns.add_known_dir("/Library/Frameworks/DebugPython.framework/Versions/9.8/lib/python9.8/lib-dynload")
        ns.add_known_xfile("/Library/Frameworks/DebugPython.framework/Versions/9.8/lib/python9.8/os.py")

        # This is definitely not the stdlib (see discussion in bpo-46890)
        #ns.add_known_xfile("/Library/lib/python98.zip")
        expected = dict(
            executable="/Library/Frameworks/DebugPython.framework/Versions/9.8/bin/python9.8",
            prefix="/Library/Frameworks/DebugPython.framework/Versions/9.8",
            exec_prefix="/Library/Frameworks/DebugPython.framework/Versions/9.8",
            base_executable="/Library/Frameworks/DebugPython.framework/Versions/9.8/bin/python9.8",
            base_prefix="/Library/Frameworks/DebugPython.framework/Versions/9.8",
            base_exec_prefix="/Library/Frameworks/DebugPython.framework/Versions/9.8",
            module_search_paths_set=1,
            module_search_paths=[
                "/Library/Frameworks/DebugPython.framework/Versions/9.8/lib/python98.zip",
                "/Library/Frameworks/DebugPython.framework/Versions/9.8/lib/python9.8",
                "/Library/Frameworks/DebugPython.framework/Versions/9.8/lib/python9.8/lib-dynload",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_venv_framework_macos(self):
        """Test a venv layout on macOS using a framework build
        """
        venv_path = "/tmp/workdir/venv"
        ns = MockPosixNamespace(
            os_name="darwin",
            argv0="/Library/Frameworks/Python.framework/Versions/9.8/Resources/Python.app/Contents/MacOS/Python",
            WITH_NEXT_FRAMEWORK=1,
            PREFIX="/Library/Frameworks/Python.framework/Versions/9.8",
            EXEC_PREFIX="/Library/Frameworks/Python.framework/Versions/9.8",
            ENV___PYVENV_LAUNCHER__=f"{venv_path}/bin/python",
            real_executable="/Library/Frameworks/Python.framework/Versions/9.8/Resources/Python.app/Contents/MacOS/Python",
            library="/Library/Frameworks/Python.framework/Versions/9.8/Python",
        )
        ns.add_known_dir(venv_path)
        ns.add_known_dir(f"{venv_path}/bin")
        ns.add_known_dir(f"{venv_path}/lib")
        ns.add_known_dir(f"{venv_path}/lib/python9.8")
        ns.add_known_xfile(f"{venv_path}/bin/python")
        ns.add_known_xfile("/Library/Frameworks/Python.framework/Versions/9.8/Resources/Python.app/Contents/MacOS/Python")
        ns.add_known_xfile("/Library/Frameworks/Python.framework/Versions/9.8/bin/python9.8")
        ns.add_known_dir("/Library/Frameworks/Python.framework/Versions/9.8/lib/python9.8/lib-dynload")
        ns.add_known_xfile("/Library/Frameworks/Python.framework/Versions/9.8/lib/python9.8/os.py")
        ns.add_known_file(f"{venv_path}/pyvenv.cfg", [
            "home = /Library/Frameworks/Python.framework/Versions/9.8/bin"
        ])
        expected = dict(
            executable=f"{venv_path}/bin/python",
            prefix=venv_path,
            exec_prefix=venv_path,
            base_executable="/Library/Frameworks/Python.framework/Versions/9.8/bin/python9.8",
            base_prefix="/Library/Frameworks/Python.framework/Versions/9.8",
            base_exec_prefix="/Library/Frameworks/Python.framework/Versions/9.8",
            module_search_paths_set=1,
            module_search_paths=[
                "/Library/Frameworks/Python.framework/Versions/9.8/lib/python98.zip",
                "/Library/Frameworks/Python.framework/Versions/9.8/lib/python9.8",
                "/Library/Frameworks/Python.framework/Versions/9.8/lib/python9.8/lib-dynload",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_venv_alt_framework_macos(self):
        """Test a venv layout on macOS using a framework build

        ``--with-framework-name=DebugPython``
        """
        venv_path = "/tmp/workdir/venv"
        ns = MockPosixNamespace(
            os_name="darwin",
            argv0="/Library/Frameworks/DebugPython.framework/Versions/9.8/Resources/Python.app/Contents/MacOS/DebugPython",
            WITH_NEXT_FRAMEWORK=1,
            PREFIX="/Library/Frameworks/DebugPython.framework/Versions/9.8",
            EXEC_PREFIX="/Library/Frameworks/DebugPython.framework/Versions/9.8",
            ENV___PYVENV_LAUNCHER__=f"{venv_path}/bin/python",
            real_executable="/Library/Frameworks/DebugPython.framework/Versions/9.8/Resources/Python.app/Contents/MacOS/DebugPython",
            library="/Library/Frameworks/DebugPython.framework/Versions/9.8/DebugPython",
        )
        ns.add_known_dir(venv_path)
        ns.add_known_dir(f"{venv_path}/bin")
        ns.add_known_dir(f"{venv_path}/lib")
        ns.add_known_dir(f"{venv_path}/lib/python9.8")
        ns.add_known_xfile(f"{venv_path}/bin/python")
        ns.add_known_xfile("/Library/Frameworks/DebugPython.framework/Versions/9.8/Resources/Python.app/Contents/MacOS/DebugPython")
        ns.add_known_xfile("/Library/Frameworks/DebugPython.framework/Versions/9.8/bin/python9.8")
        ns.add_known_dir("/Library/Frameworks/DebugPython.framework/Versions/9.8/lib/python9.8/lib-dynload")
        ns.add_known_xfile("/Library/Frameworks/DebugPython.framework/Versions/9.8/lib/python9.8/os.py")
        ns.add_known_file(f"{venv_path}/pyvenv.cfg", [
            "home = /Library/Frameworks/DebugPython.framework/Versions/9.8/bin"
        ])
        expected = dict(
            executable=f"{venv_path}/bin/python",
            prefix=venv_path,
            exec_prefix=venv_path,
            base_executable="/Library/Frameworks/DebugPython.framework/Versions/9.8/bin/python9.8",
            base_prefix="/Library/Frameworks/DebugPython.framework/Versions/9.8",
            base_exec_prefix="/Library/Frameworks/DebugPython.framework/Versions/9.8",
            module_search_paths_set=1,
            module_search_paths=[
                "/Library/Frameworks/DebugPython.framework/Versions/9.8/lib/python98.zip",
                "/Library/Frameworks/DebugPython.framework/Versions/9.8/lib/python9.8",
                "/Library/Frameworks/DebugPython.framework/Versions/9.8/lib/python9.8/lib-dynload",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_venv_macos(self):
        """Test a venv layout on macOS.

        This layout is discovered when 'executable' and 'real_executable' match,
        but $__PYVENV_LAUNCHER__ has been set to the original process.
        """
        ns = MockPosixNamespace(
            os_name="darwin",
            argv0="/usr/bin/python",
            PREFIX="/usr",
            ENV___PYVENV_LAUNCHER__="/framework/Python9.8/python",
            real_executable="/usr/bin/python",
        )
        ns.add_known_xfile("/usr/bin/python")
        ns.add_known_xfile("/framework/Python9.8/python")
        ns.add_known_file("/usr/lib/python9.8/os.py")
        ns.add_known_dir("/usr/lib/python9.8/lib-dynload")
        ns.add_known_file("/framework/Python9.8/pyvenv.cfg", [
            "home = /usr/bin"
        ])
        expected = dict(
            executable="/framework/Python9.8/python",
            prefix="/framework/Python9.8",
            exec_prefix="/framework/Python9.8",
            base_executable="/usr/bin/python",
            base_prefix="/usr",
            base_exec_prefix="/usr",
            module_search_paths_set=1,
            module_search_paths=[
                "/usr/lib/python98.zip",
                "/usr/lib/python9.8",
                "/usr/lib/python9.8/lib-dynload",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_symlink_normal_macos(self):
        "Test a 'standard' install layout via symlink on macOS"
        ns = MockPosixNamespace(
            os_name="darwin",
            PREFIX="/usr",
            argv0="python",
            ENV_PATH="/linkfrom:/usr/bin",
            # real_executable on macOS matches the invocation path
            real_executable="/linkfrom/python",
        )
        ns.add_known_xfile("/linkfrom/python")
        ns.add_known_xfile("/usr/bin/python")
        ns.add_known_link("/linkfrom/python", "/usr/bin/python")
        ns.add_known_file("/usr/lib/python9.8/os.py")
        ns.add_known_dir("/usr/lib/python9.8/lib-dynload")
        expected = dict(
            executable="/linkfrom/python",
            base_executable="/linkfrom/python",
            prefix="/usr",
            exec_prefix="/usr",
            module_search_paths_set=1,
            module_search_paths=[
                "/usr/lib/python98.zip",
                "/usr/lib/python9.8",
                "/usr/lib/python9.8/lib-dynload",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_symlink_buildpath_macos(self):
        """Test an in-build-tree layout via symlink on macOS.

        This layout is discovered from the presence of pybuilddir.txt, which
        contains the relative path from the executable's directory to the
        platstdlib path.
        """
        ns = MockPosixNamespace(
            os_name="darwin",
            argv0=r"python",
            ENV_PATH="/linkfrom:/usr/bin",
            PREFIX="/usr/local",
            # real_executable on macOS matches the invocation path
            real_executable="/linkfrom/python",
        )
        ns.add_known_xfile("/linkfrom/python")
        ns.add_known_xfile("/home/cpython/python")
        ns.add_known_link("/linkfrom/python", "/home/cpython/python")
        ns.add_known_xfile("/usr/local/bin/python")
        ns.add_known_file("/home/cpython/pybuilddir.txt", ["build/lib.macos-9.8"])
        ns.add_known_file("/home/cpython/Lib/os.py")
        ns.add_known_dir("/home/cpython/lib-dynload")
        expected = dict(
            executable="/linkfrom/python",
            prefix="/usr/local",
            exec_prefix="/usr/local",
            base_executable="/linkfrom/python",
            build_prefix="/home/cpython",
            _is_python_build=1,
            module_search_paths_set=1,
            module_search_paths=[
                "/usr/local/lib/python98.zip",
                "/home/cpython/Lib",
                "/home/cpython/build/lib.macos-9.8",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_explicitly_set_stdlib_dir(self):
        """Test the explicitly set stdlib_dir in the config is respected."""
        ns = MockPosixNamespace(
            PREFIX="/usr",
            argv0="python",
            ENV_PATH="/usr/bin",
        )
        ns["config"]["stdlib_dir"] = "/custom_stdlib_dir"
        expected = dict(
            stdlib_dir="/custom_stdlib_dir",
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

    def test_PYTHONHOME_in_venv(self):
        "Make sure prefix/exec_prefix still point to the venv if PYTHONHOME was used."
        ns = MockPosixNamespace(
            argv0="/venv/bin/python",
            PREFIX="/usr",
            ENV_PYTHONHOME="/pythonhome",
        )
        # Setup venv
        ns.add_known_xfile("/venv/bin/python")
        ns.add_known_file("/venv/pyvenv.cfg", [
            r"home = /usr/bin"
        ])
        # Seutup PYTHONHOME
        ns.add_known_file("/pythonhome/lib/python9.8/os.py")
        ns.add_known_dir("/pythonhome/lib/python9.8/lib-dynload")

        expected = dict(
            executable="/venv/bin/python",
            prefix="/venv",
            exec_prefix="/venv",
            base_prefix="/pythonhome",
            base_exec_prefix="/pythonhome",
            module_search_paths_set=1,
            module_search_paths=[
                "/pythonhome/lib/python98.zip",
                "/pythonhome/lib/python9.8",
                "/pythonhome/lib/python9.8/lib-dynload",
            ],
        )
        actual = getpath(ns, expected)
        self.assertEqual(expected, actual)

# ******************************************************************************

DEFAULT_NAMESPACE = dict(
    PREFIX="",
    EXEC_PREFIX="",
    PYTHONPATH="",
    VPATH="",
    PLATLIBDIR="",
    PYDEBUGEXT="",
    VERSION_MAJOR=9,    # fixed version number for ease
    VERSION_MINOR=8,    # of testing
    ABI_THREAD="",
    PYWINVER=None,
    EXE_SUFFIX=None,

    ENV_PATH="",
    ENV_PYTHONHOME="",
    ENV_PYTHONEXECUTABLE="",
    ENV___PYVENV_LAUNCHER__="",
    argv0="",
    py_setpath="",
    real_executable="",
    executable_dir="",
    library="",
    winreg=None,
    build_prefix=None,
    venv_prefix=None,
)

DEFAULT_CONFIG = dict(
    home=None,
    platlibdir=None,
    pythonpath=None,
    program_name=None,
    prefix=None,
    exec_prefix=None,
    base_prefix=None,
    base_exec_prefix=None,
    executable=None,
    base_executable="",
    stdlib_dir=None,
    platstdlib_dir=None,
    module_search_paths=None,
    module_search_paths_set=0,
    pythonpath_env=None,
    argv=None,
    orig_argv=None,

    isolated=0,
    use_environment=1,
    use_site=1,
)

class MockNTNamespace(dict):
    def __init__(self, *a, argv0=None, config=None, **kw):
        self.update(DEFAULT_NAMESPACE)
        self["config"] = DEFAULT_CONFIG.copy()
        self["os_name"] = "nt"
        self["PLATLIBDIR"] = "DLLs"
        self["PYWINVER"] = "9.8-XY"
        self["VPATH"] = r"..\.."
        super().__init__(*a, **kw)
        if argv0:
            self["config"]["orig_argv"] = [argv0]
        if config:
            self["config"].update(config)
        self._files = {}
        self._links = {}
        self._dirs = set()
        self._warnings = []

    def add_known_file(self, path, lines=None):
        self._files[path.casefold()] = list(lines or ())
        self.add_known_dir(path.rpartition("\\")[0])

    def add_known_xfile(self, path):
        self.add_known_file(path)

    def add_known_link(self, path, target):
        self._links[path.casefold()] = target

    def add_known_dir(self, path):
        p = path.rstrip("\\").casefold()
        while p:
            self._dirs.add(p)
            p = p.rpartition("\\")[0]

    def __missing__(self, key):
        try:
            return getattr(self, key)
        except AttributeError:
            raise KeyError(key) from None

    def abspath(self, path):
        if self.isabs(path):
            return path
        return self.joinpath("C:\\Absolute", path)

    def basename(self, path):
        return path.rpartition("\\")[2]

    def dirname(self, path):
        name = path.rstrip("\\").rpartition("\\")[0]
        if name[1:] == ":":
            return name + "\\"
        return name

    def hassuffix(self, path, suffix):
        return path.casefold().endswith(suffix.casefold())

    def isabs(self, path):
        return path[1:3] == ":\\"

    def isdir(self, path):
        if verbose:
            print("Check if", path, "is a dir")
        return path.casefold() in self._dirs

    def isfile(self, path):
        if verbose:
            print("Check if", path, "is a file")
        return path.casefold() in self._files

    def ismodule(self, path):
        if verbose:
            print("Check if", path, "is a module")
        path = path.casefold()
        return path in self._files and path.rpartition(".")[2] == "py".casefold()

    def isxfile(self, path):
        if verbose:
            print("Check if", path, "is a executable")
        path = path.casefold()
        return path in self._files and path.rpartition(".")[2] == "exe".casefold()

    def joinpath(self, *path):
        return ntpath.normpath(ntpath.join(*path))

    def readlines(self, path):
        try:
            return self._files[path.casefold()]
        except KeyError:
            raise FileNotFoundError(path) from None

    def realpath(self, path, _trail=None):
        if verbose:
            print("Read link from", path)
        try:
            link = self._links[path.casefold()]
        except KeyError:
            return path
        if _trail is None:
            _trail = set()
        elif link.casefold() in _trail:
            raise OSError("circular link")
        _trail.add(link.casefold())
        return self.realpath(link, _trail)

    def warn(self, message):
        self._warnings.append(message)
        if verbose:
            print(message)


class MockWinreg:
    HKEY_LOCAL_MACHINE = "HKLM"
    HKEY_CURRENT_USER = "HKCU"

    def __init__(self, keys):
        self.keys = {k.casefold(): v for k, v in keys.items()}
        self.open = {}

    def __repr__(self):
        return "<MockWinreg>"

    def __eq__(self, other):
        return isinstance(other, type(self))

    def open_keys(self):
        return list(self.open)

    def OpenKeyEx(self, hkey, subkey):
        if verbose:
            print(f"OpenKeyEx({hkey}, {subkey})")
        key = f"{hkey}\\{subkey}".casefold()
        if key in self.keys:
            self.open[key] = self.open.get(key, 0) + 1
            return key
        raise FileNotFoundError()

    def CloseKey(self, hkey):
        if verbose:
            print(f"CloseKey({hkey})")
        hkey = hkey.casefold()
        if hkey not in self.open:
            raise RuntimeError("key is not open")
        self.open[hkey] -= 1
        if not self.open[hkey]:
            del self.open[hkey]

    def EnumKey(self, hkey, i):
        if verbose:
            print(f"EnumKey({hkey}, {i})")
        hkey = hkey.casefold()
        if hkey not in self.open:
            raise RuntimeError("key is not open")
        prefix = f'{hkey}\\'
        subkeys = [k[len(prefix):] for k in sorted(self.keys) if k.startswith(prefix)]
        subkeys[:] = [k for k in subkeys if '\\' not in k]
        for j, n in enumerate(subkeys):
            if j == i:
                return n.removeprefix(prefix)
        raise OSError("end of enumeration")

    def QueryValue(self, hkey, subkey):
        if verbose:
            print(f"QueryValue({hkey}, {subkey})")
        hkey = hkey.casefold()
        if hkey not in self.open:
            raise RuntimeError("key is not open")
        if subkey:
            subkey = subkey.casefold()
            hkey = f'{hkey}\\{subkey}'
        try:
            return self.keys[hkey]
        except KeyError:
            raise OSError()


class MockPosixNamespace(dict):
    def __init__(self, *a, argv0=None, config=None, **kw):
        self.update(DEFAULT_NAMESPACE)
        self["config"] = DEFAULT_CONFIG.copy()
        self["os_name"] = "posix"
        self["PLATLIBDIR"] = "lib"
        self["WITH_NEXT_FRAMEWORK"] = 0
        super().__init__(*a, **kw)
        if argv0:
            self["config"]["orig_argv"] = [argv0]
        if config:
            self["config"].update(config)
        self._files = {}
        self._xfiles = set()
        self._links = {}
        self._dirs = set()
        self._warnings = []

    def add_known_file(self, path, lines=None):
        self._files[path] = list(lines or ())
        self.add_known_dir(path.rpartition("/")[0])

    def add_known_xfile(self, path):
        self.add_known_file(path)
        self._xfiles.add(path)

    def add_known_link(self, path, target):
        self._links[path] = target

    def add_known_dir(self, path):
        p = path.rstrip("/")
        while p:
            self._dirs.add(p)
            p = p.rpartition("/")[0]

    def __missing__(self, key):
        try:
            return getattr(self, key)
        except AttributeError:
            raise KeyError(key) from None

    def abspath(self, path):
        if self.isabs(path):
            return path
        return self.joinpath("/Absolute", path)

    def basename(self, path):
        return path.rpartition("/")[2]

    def dirname(self, path):
        return path.rstrip("/").rpartition("/")[0]

    def hassuffix(self, path, suffix):
        return path.endswith(suffix)

    def isabs(self, path):
        return path[0:1] == "/"

    def isdir(self, path):
        if verbose:
            print("Check if", path, "is a dir")
        return path in self._dirs

    def isfile(self, path):
        if verbose:
            print("Check if", path, "is a file")
        return path in self._files

    def ismodule(self, path):
        if verbose:
            print("Check if", path, "is a module")
        return path in self._files and path.rpartition(".")[2] == "py"

    def isxfile(self, path):
        if verbose:
            print("Check if", path, "is an xfile")
        return path in self._xfiles

    def joinpath(self, *path):
        return posixpath.normpath(posixpath.join(*path))

    def readlines(self, path):
        try:
            return self._files[path]
        except KeyError:
            raise FileNotFoundError(path) from None

    def realpath(self, path, _trail=None):
        if verbose:
            print("Read link from", path)
        try:
            link = self._links[path]
        except KeyError:
            return path
        if _trail is None:
            _trail = set()
        elif link in _trail:
            raise OSError("circular link")
        _trail.add(link)
        return self.realpath(link, _trail)

    def warn(self, message):
        self._warnings.append(message)
        if verbose:
            print(message)


def diff_dict(before, after, prefix="global"):
    diff = []
    for k in sorted(before):
        if k[:2] == "__":
            continue
        if k == "config":
            diff_dict(before[k], after[k], prefix="config")
            continue
        if k in after and after[k] != before[k]:
            diff.append((k, before[k], after[k]))
    if not diff:
        return
    max_k = max(len(k) for k, _, _ in diff)
    indent = " " * (len(prefix) + 1 + max_k)
    if verbose:
        for k, b, a in diff:
            if b:
                print("{}.{} -{!r}\n{} +{!r}".format(prefix, k.ljust(max_k), b, indent, a))
            else:
                print("{}.{} +{!r}".format(prefix, k.ljust(max_k), a))


def dump_dict(before, after, prefix="global"):
    if not verbose or not after:
        return
    max_k = max(len(k) for k in after)
    for k, v in sorted(after.items(), key=lambda i: i[0]):
        if k[:2] == "__":
            continue
        if k == "config":
            dump_dict(before[k], after[k], prefix="config")
            continue
        try:
            if v != before[k]:
                print("{}.{} {!r} (was {!r})".format(prefix, k.ljust(max_k), v, before[k]))
                continue
        except KeyError:
            pass
        print("{}.{} {!r}".format(prefix, k.ljust(max_k), v))


def getpath(ns, keys):
    before = copy.deepcopy(ns)
    failed = True
    try:
        exec(SOURCE, ns)
        failed = False
    finally:
        if failed:
            dump_dict(before, ns)
        else:
            diff_dict(before, ns)
    return {
        k: ns['config'].get(k, ns.get(k, ...))
        for k in keys
    }
