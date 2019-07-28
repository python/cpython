from __future__ import unicode_literals

import os
import sys
import shutil
import tempfile
import textwrap
import contextlib

try:
    from contextlib import ExitStack
except ImportError:
    from contextlib2 import ExitStack

try:
    import pathlib
except ImportError:
    import pathlib2 as pathlib


__metaclass__ = type


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


class SiteDir:
    def setUp(self):
        self.fixtures = ExitStack()
        self.addCleanup(self.fixtures.close)
        self.site_dir = self.fixtures.enter_context(tempdir())


class OnSysPath:
    @staticmethod
    @contextlib.contextmanager
    def add_sys_path(dir):
        sys.path[:0] = [str(dir)]
        try:
            yield
        finally:
            sys.path.remove(str(dir))

    def setUp(self):
        super(OnSysPath, self).setUp()
        self.fixtures.enter_context(self.add_sys_path(self.site_dir))


class DistInfoPkg(OnSysPath, SiteDir):
    files = {
        "distinfo_pkg-1.0.0.dist-info": {
            "METADATA": """
                Name: distinfo-pkg
                Author: Steven Ma
                Version: 1.0.0
                Requires-Dist: wheel >= 1.0
                Requires-Dist: pytest; extra == 'test'
                """,
            "RECORD": "mod.py,sha256=abc,20\n",
            "entry_points.txt": """
                [entries]
                main = mod:main
                ns:sub = mod:main
            """
            },
        "mod.py": """
            def main():
                print("hello world")
            """,
        }

    def setUp(self):
        super(DistInfoPkg, self).setUp()
        build_files(DistInfoPkg.files, self.site_dir)


class DistInfoPkgOffPath(SiteDir):
    def setUp(self):
        super(DistInfoPkgOffPath, self).setUp()
        build_files(DistInfoPkg.files, self.site_dir)


class EggInfoPkg(OnSysPath, SiteDir):
    files = {
        "egginfo_pkg.egg-info": {
            "PKG-INFO": """
                Name: egginfo-pkg
                Author: Steven Ma
                License: Unknown
                Version: 1.0.0
                Classifier: Intended Audience :: Developers
                Classifier: Topic :: Software Development :: Libraries
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
            "top_level.txt": "mod\n"
            },
        "mod.py": """
            def main():
                print("hello world")
            """,
        }

    def setUp(self):
        super(EggInfoPkg, self).setUp()
        build_files(EggInfoPkg.files, prefix=self.site_dir)


class EggInfoFile(OnSysPath, SiteDir):
    files = {
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
        super(EggInfoFile, self).setUp()
        build_files(EggInfoFile.files, prefix=self.site_dir)


def build_files(file_defs, prefix=pathlib.Path()):
    """Build a set of files/directories, as described by the

    file_defs dictionary.  Each key/value pair in the dictionary is
    interpreted as a filename/contents pair.  If the contents value is a
    dictionary, a directory is created, and the dictionary interpreted
    as the files within it, recursively.

    For example:

    {"README.txt": "A README file",
     "foo": {
        "__init__.py": "",
        "bar": {
            "__init__.py": "",
        },
        "baz.py": "# Some code",
     }
    }
    """
    for name, contents in file_defs.items():
        full_name = prefix / name
        if isinstance(contents, dict):
            full_name.mkdir()
            build_files(contents, prefix=full_name)
        else:
            if isinstance(contents, bytes):
                with full_name.open('wb') as f:
                    f.write(contents)
            else:
                with full_name.open('w') as f:
                    f.write(DALS(contents))


def DALS(str):
    "Dedent and left-strip"
    return textwrap.dedent(str).lstrip()
