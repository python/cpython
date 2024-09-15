import os
import sys
import copy
import shutil
import pathlib
import tempfile
import textwrap
import functools
import contextlib

from test.support.os_helper import FS_NONASCII
from test.support import requires_zlib

from . import _path
from ._path import FilesSpec


try:
    from importlib import resources  # type: ignore

    getattr(resources, 'files')
    getattr(resources, 'as_file')
except (ImportError, AttributeError):
    import importlib_resources as resources  # type: ignore


@contextlib.contextmanager
def tempdir():
    tmpdir = tempfile.mkdtemp()
    try:
        yield pathlib.Path(tmpdir)
    finally:
        shutil.rmtree(tmpdir)


@contextlib.contextmanager
def save_cwd():
    orig = os.getcwd()
    try:
        yield
    finally:
        os.chdir(orig)


@contextlib.contextmanager
def tempdir_as_cwd():
    with tempdir() as tmp:
        with save_cwd():
            os.chdir(str(tmp))
            yield tmp


@contextlib.contextmanager
def install_finder(finder):
    sys.meta_path.append(finder)
    try:
        yield
    finally:
        sys.meta_path.remove(finder)


class Fixtures:
    def setUp(self):
        self.fixtures = contextlib.ExitStack()
        self.addCleanup(self.fixtures.close)


class SiteDir(Fixtures):
    def setUp(self):
        super().setUp()
        self.site_dir = self.fixtures.enter_context(tempdir())


class OnSysPath(Fixtures):
    @staticmethod
    @contextlib.contextmanager
    def add_sys_path(dir):
        sys.path[:0] = [str(dir)]
        try:
            yield
        finally:
            sys.path.remove(str(dir))

    def setUp(self):
        super().setUp()
        self.fixtures.enter_context(self.add_sys_path(self.site_dir))


class DistInfoPkg(OnSysPath, SiteDir):
    files: FilesSpec = {
        "distinfo_pkg-1.0.0.dist-info": {
            "METADATA": """
                Name: distinfo-pkg
                Author: Steven Ma
                Version: 1.0.0
                Requires-Dist: wheel >= 1.0
                Requires-Dist: pytest; extra == 'test'
                Keywords: sample package

                Once upon a time
                There was a distinfo pkg
                """,
            "RECORD": "mod.py,sha256=abc,20\n",
            "entry_points.txt": """
                [entries]
                main = mod:main
                ns:sub = mod:main
            """,
        },
        "mod.py": """
            def main():
                print("hello world")
            """,
    }

    def setUp(self):
        super().setUp()
        build_files(DistInfoPkg.files, self.site_dir)

    def make_uppercase(self):
        """
        Rewrite metadata with everything uppercase.
        """
        shutil.rmtree(self.site_dir / "distinfo_pkg-1.0.0.dist-info")
        files = copy.deepcopy(DistInfoPkg.files)
        info = files["distinfo_pkg-1.0.0.dist-info"]
        info["METADATA"] = info["METADATA"].upper()
        build_files(files, self.site_dir)


class DistInfoPkgWithDot(OnSysPath, SiteDir):
    files: FilesSpec = {
        "pkg_dot-1.0.0.dist-info": {
            "METADATA": """
                Name: pkg.dot
                Version: 1.0.0
                """,
        },
    }

    def setUp(self):
        super().setUp()
        build_files(DistInfoPkgWithDot.files, self.site_dir)


class DistInfoPkgWithDotLegacy(OnSysPath, SiteDir):
    files: FilesSpec = {
        "pkg.dot-1.0.0.dist-info": {
            "METADATA": """
                Name: pkg.dot
                Version: 1.0.0
                """,
        },
        "pkg.lot.egg-info": {
            "METADATA": """
                Name: pkg.lot
                Version: 1.0.0
                """,
        },
    }

    def setUp(self):
        super().setUp()
        build_files(DistInfoPkgWithDotLegacy.files, self.site_dir)


class DistInfoPkgOffPath(SiteDir):
    def setUp(self):
        super().setUp()
        build_files(DistInfoPkg.files, self.site_dir)


class EggInfoPkg(OnSysPath, SiteDir):
    files: FilesSpec = {
        "egginfo_pkg.egg-info": {
            "PKG-INFO": """
                Name: egginfo-pkg
                Author: Steven Ma
                License: Unknown
                Version: 1.0.0
                Classifier: Intended Audience :: Developers
                Classifier: Topic :: Software Development :: Libraries
                Keywords: sample package
                Description: Once upon a time
                        There was an egginfo package
                """,
            "SOURCES.txt": """
                mod.py
                egginfo_pkg.egg-info/top_level.txt
            """,
            "entry_points.txt": """
                [entries]
                main = mod:main
            """,
            "requires.txt": """
                wheel >= 1.0; python_version >= "2.7"
                [test]
                pytest
            """,
            "top_level.txt": "mod\n",
        },
        "mod.py": """
            def main():
                print("hello world")
            """,
    }

    def setUp(self):
        super().setUp()
        build_files(EggInfoPkg.files, prefix=self.site_dir)


class EggInfoPkgPipInstalledNoToplevel(OnSysPath, SiteDir):
    files: FilesSpec = {
        "egg_with_module_pkg.egg-info": {
            "PKG-INFO": "Name: egg_with_module-pkg",
            # SOURCES.txt is made from the source archive, and contains files
            # (setup.py) that are not present after installation.
            "SOURCES.txt": """
                egg_with_module.py
                setup.py
                egg_with_module_pkg.egg-info/PKG-INFO
                egg_with_module_pkg.egg-info/SOURCES.txt
                egg_with_module_pkg.egg-info/top_level.txt
            """,
            # installed-files.txt is written by pip, and is a strictly more
            # accurate source than SOURCES.txt as to the installed contents of
            # the package.
            "installed-files.txt": """
                ../egg_with_module.py
                PKG-INFO
                SOURCES.txt
                top_level.txt
            """,
            # missing top_level.txt (to trigger fallback to installed-files.txt)
        },
        "egg_with_module.py": """
            def main():
                print("hello world")
            """,
    }

    def setUp(self):
        super().setUp()
        build_files(EggInfoPkgPipInstalledNoToplevel.files, prefix=self.site_dir)


class EggInfoPkgPipInstalledExternalDataFiles(OnSysPath, SiteDir):
    files: FilesSpec = {
        "egg_with_module_pkg.egg-info": {
            "PKG-INFO": "Name: egg_with_module-pkg",
            # SOURCES.txt is made from the source archive, and contains files
            # (setup.py) that are not present after installation.
            "SOURCES.txt": """
                egg_with_module.py
                setup.py
                egg_with_module.json
                egg_with_module_pkg.egg-info/PKG-INFO
                egg_with_module_pkg.egg-info/SOURCES.txt
                egg_with_module_pkg.egg-info/top_level.txt
            """,
            # installed-files.txt is written by pip, and is a strictly more
            # accurate source than SOURCES.txt as to the installed contents of
            # the package.
            "installed-files.txt": """
                ../../../etc/jupyter/jupyter_notebook_config.d/relative.json
                /etc/jupyter/jupyter_notebook_config.d/absolute.json
                ../egg_with_module.py
                PKG-INFO
                SOURCES.txt
                top_level.txt
            """,
            # missing top_level.txt (to trigger fallback to installed-files.txt)
        },
        "egg_with_module.py": """
            def main():
                print("hello world")
            """,
    }

    def setUp(self):
        super().setUp()
        build_files(EggInfoPkgPipInstalledExternalDataFiles.files, prefix=self.site_dir)


class EggInfoPkgPipInstalledNoModules(OnSysPath, SiteDir):
    files: FilesSpec = {
        "egg_with_no_modules_pkg.egg-info": {
            "PKG-INFO": "Name: egg_with_no_modules-pkg",
            # SOURCES.txt is made from the source archive, and contains files
            # (setup.py) that are not present after installation.
            "SOURCES.txt": """
                setup.py
                egg_with_no_modules_pkg.egg-info/PKG-INFO
                egg_with_no_modules_pkg.egg-info/SOURCES.txt
                egg_with_no_modules_pkg.egg-info/top_level.txt
            """,
            # installed-files.txt is written by pip, and is a strictly more
            # accurate source than SOURCES.txt as to the installed contents of
            # the package.
            "installed-files.txt": """
                PKG-INFO
                SOURCES.txt
                top_level.txt
            """,
            # top_level.txt correctly reflects that no modules are installed
            "top_level.txt": b"\n",
        },
    }

    def setUp(self):
        super().setUp()
        build_files(EggInfoPkgPipInstalledNoModules.files, prefix=self.site_dir)


class EggInfoPkgSourcesFallback(OnSysPath, SiteDir):
    files: FilesSpec = {
        "sources_fallback_pkg.egg-info": {
            "PKG-INFO": "Name: sources_fallback-pkg",
            # SOURCES.txt is made from the source archive, and contains files
            # (setup.py) that are not present after installation.
            "SOURCES.txt": """
                sources_fallback.py
                setup.py
                sources_fallback_pkg.egg-info/PKG-INFO
                sources_fallback_pkg.egg-info/SOURCES.txt
            """,
            # missing installed-files.txt (i.e. not installed by pip) and
            # missing top_level.txt (to trigger fallback to SOURCES.txt)
        },
        "sources_fallback.py": """
            def main():
                print("hello world")
            """,
    }

    def setUp(self):
        super().setUp()
        build_files(EggInfoPkgSourcesFallback.files, prefix=self.site_dir)


class EggInfoFile(OnSysPath, SiteDir):
    files: FilesSpec = {
        "egginfo_file.egg-info": """
            Metadata-Version: 1.0
            Name: egginfo_file
            Version: 0.1
            Summary: An example package
            Home-page: www.example.com
            Author: Eric Haffa-Vee
            Author-email: eric@example.coms
            License: UNKNOWN
            Description: UNKNOWN
            Platform: UNKNOWN
            """,
    }

    def setUp(self):
        super().setUp()
        build_files(EggInfoFile.files, prefix=self.site_dir)


# dedent all text strings before writing
orig = _path.create.registry[str]
_path.create.register(str, lambda content, path: orig(DALS(content), path))


build_files = _path.build


def build_record(file_defs):
    return ''.join(f'{name},,\n' for name in record_names(file_defs))


def record_names(file_defs):
    recording = _path.Recording()
    _path.build(file_defs, recording)
    return recording.record


class FileBuilder:
    def unicode_filename(self):
        return FS_NONASCII or self.skip("File system does not support non-ascii.")


def DALS(str):
    "Dedent and left-strip"
    return textwrap.dedent(str).lstrip()


@requires_zlib()
class ZipFixtures:
    root = 'test.test_importlib.data'

    def _fixture_on_path(self, filename):
        pkg_file = resources.files(self.root).joinpath(filename)
        file = self.resources.enter_context(resources.as_file(pkg_file))
        assert file.name.startswith('example'), file.name
        sys.path.insert(0, str(file))
        self.resources.callback(sys.path.pop, 0)

    def setUp(self):
        # Add self.zip_name to the front of sys.path.
        self.resources = contextlib.ExitStack()
        self.addCleanup(self.resources.close)


def parameterize(*args_set):
    """Run test method with a series of parameters."""

    def wrapper(func):
        @functools.wraps(func)
        def _inner(self):
            for args in args_set:
                with self.subTest(**args):
                    func(self, **args)

        return _inner

    return wrapper
