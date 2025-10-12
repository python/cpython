import contextlib
import copy
import functools
import json
import pathlib
import shutil
import sys
import textwrap

from test.support import import_helper
from test.support import os_helper
from test.support import requires_zlib

from . import _path
from ._path import FilesSpec

if sys.version_info >= (3, 9):
    from importlib import resources
else:
    import importlib_resources as resources


@contextlib.contextmanager
def tmp_path():
    """
    Like os_helper.temp_dir, but yields a pathlib.Path.
    """
    with os_helper.temp_dir() as path:
        yield pathlib.Path(path)


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
        self.site_dir = self.fixtures.enter_context(tmp_path())


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
        self.fixtures.enter_context(import_helper.isolated_modules())


class SiteBuilder(SiteDir):
    def setUp(self):
        super().setUp()
        for cls in self.__class__.mro():
            with contextlib.suppress(AttributeError):
                build_files(cls.files, prefix=self.site_dir)


class DistInfoPkg(OnSysPath, SiteBuilder):
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

    def make_uppercase(self):
        """
        Rewrite metadata with everything uppercase.
        """
        shutil.rmtree(self.site_dir / "distinfo_pkg-1.0.0.dist-info")
        files = copy.deepcopy(DistInfoPkg.files)
        info = files["distinfo_pkg-1.0.0.dist-info"]
        info["METADATA"] = info["METADATA"].upper()
        build_files(files, self.site_dir)


class DistInfoPkgEditable(DistInfoPkg):
    """
    Package with a PEP 660 direct_url.json.
    """

    some_hash = '524127ce937f7cb65665130c695abd18ca386f60bb29687efb976faa1596fdcc'
    files: FilesSpec = {
        'distinfo_pkg-1.0.0.dist-info': {
            'direct_url.json': json.dumps({
                "archive_info": {
                    "hash": f"sha256={some_hash}",
                    "hashes": {"sha256": f"{some_hash}"},
                },
                "url": "file:///path/to/distinfo_pkg-1.0.0.editable-py3-none-any.whl",
            })
        },
    }


class DistInfoPkgWithDot(OnSysPath, SiteBuilder):
    files: FilesSpec = {
        "pkg_dot-1.0.0.dist-info": {
            "METADATA": """
                Name: pkg.dot
                Version: 1.0.0
                """,
        },
    }


class DistInfoPkgWithDotLegacy(OnSysPath, SiteBuilder):
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


class DistInfoPkgOffPath(SiteBuilder):
    files = DistInfoPkg.files


class EggInfoPkg(OnSysPath, SiteBuilder):
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


class EggInfoPkgPipInstalledNoToplevel(OnSysPath, SiteBuilder):
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


class EggInfoPkgPipInstalledExternalDataFiles(OnSysPath, SiteBuilder):
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


class EggInfoPkgPipInstalledNoModules(OnSysPath, SiteBuilder):
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


class EggInfoPkgSourcesFallback(OnSysPath, SiteBuilder):
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


class EggInfoFile(OnSysPath, SiteBuilder):
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
        return os_helper.FS_NONASCII or self.skip(
            "File system does not support non-ascii."
        )


def DALS(str):
    "Dedent and left-strip"
    return textwrap.dedent(str).lstrip()


@requires_zlib()
class ZipFixtures:
    root = 'test.test_importlib.metadata.data'

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
        # workaround for #138313
        self.addCleanup(lambda: None)


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
