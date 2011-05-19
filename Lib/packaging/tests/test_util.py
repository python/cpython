"""Tests for packaging.util."""
import os
import sys
import time
import logging
import tempfile
import subprocess
from io import StringIO

from packaging.tests import support, unittest
from packaging.errors import (
    PackagingPlatformError, PackagingByteCompileError, PackagingFileError,
    PackagingExecError, InstallationException)
from packaging import util
from packaging.util import (
    convert_path, change_root, split_quoted, strtobool, rfc822_escape,
    get_compiler_versions, _MAC_OS_X_LD_VERSION, byte_compile, find_packages,
    spawn, get_pypirc_path, generate_pypirc, read_pypirc, resolve_name, iglob,
    RICH_GLOB, egginfo_to_distinfo, is_setuptools, is_distutils, is_packaging,
    get_install_method)


PYPIRC = """\
[distutils]
index-servers =
    pypi
    server1

[pypi]
username:me
password:xxxx

[server1]
repository:http://example.com
username:tarek
password:secret
"""

PYPIRC_OLD = """\
[server-login]
username:tarek
password:secret
"""

WANTED = """\
[distutils]
index-servers =
    pypi

[pypi]
username:tarek
password:xxx
"""


class FakePopen:
    test_class = None

    def __init__(self, cmd, shell, stdout, stderr):
        self.cmd = cmd.split()[0]
        exes = self.test_class._exes
        if self.cmd not in exes:
            # we don't want to call the system, returning an empty
            # output so it doesn't match
            self.stdout = StringIO()
            self.stderr = StringIO()
        else:
            self.stdout = StringIO(exes[self.cmd])
            self.stderr = StringIO()


class UtilTestCase(support.EnvironRestorer,
                   support.TempdirManager,
                   support.LoggingCatcher,
                   unittest.TestCase):

    restore_environ = ['HOME']

    def setUp(self):
        super(UtilTestCase, self).setUp()
        self.tmp_dir = self.mkdtemp()
        self.rc = os.path.join(self.tmp_dir, '.pypirc')
        os.environ['HOME'] = self.tmp_dir
        # saving the environment
        self.name = os.name
        self.platform = sys.platform
        self.version = sys.version
        self.sep = os.sep
        self.join = os.path.join
        self.isabs = os.path.isabs
        self.splitdrive = os.path.splitdrive
        #self._config_vars = copy(sysconfig._config_vars)

        # patching os.uname
        if hasattr(os, 'uname'):
            self.uname = os.uname
            self._uname = os.uname()
        else:
            self.uname = None
            self._uname = None
        os.uname = self._get_uname

        # patching POpen
        self.old_find_executable = util.find_executable
        util.find_executable = self._find_executable
        self._exes = {}
        self.old_popen = subprocess.Popen
        self.old_stdout = sys.stdout
        self.old_stderr = sys.stderr
        FakePopen.test_class = self
        subprocess.Popen = FakePopen

    def tearDown(self):
        # getting back the environment
        os.name = self.name
        sys.platform = self.platform
        sys.version = self.version
        os.sep = self.sep
        os.path.join = self.join
        os.path.isabs = self.isabs
        os.path.splitdrive = self.splitdrive
        if self.uname is not None:
            os.uname = self.uname
        else:
            del os.uname
        #sysconfig._config_vars = copy(self._config_vars)
        util.find_executable = self.old_find_executable
        subprocess.Popen = self.old_popen
        sys.old_stdout = self.old_stdout
        sys.old_stderr = self.old_stderr
        super(UtilTestCase, self).tearDown()

    def _set_uname(self, uname):
        self._uname = uname

    def _get_uname(self):
        return self._uname

    def test_convert_path(self):
        # linux/mac
        os.sep = '/'

        def _join(path):
            return '/'.join(path)
        os.path.join = _join

        self.assertEqual(convert_path('/home/to/my/stuff'),
                         '/home/to/my/stuff')

        # win
        os.sep = '\\'

        def _join(*path):
            return '\\'.join(path)
        os.path.join = _join

        self.assertRaises(ValueError, convert_path, '/home/to/my/stuff')
        self.assertRaises(ValueError, convert_path, 'home/to/my/stuff/')

        self.assertEqual(convert_path('home/to/my/stuff'),
                         'home\\to\\my\\stuff')
        self.assertEqual(convert_path('.'),
                         os.curdir)

    def test_change_root(self):
        # linux/mac
        os.name = 'posix'

        def _isabs(path):
            return path[0] == '/'
        os.path.isabs = _isabs

        def _join(*path):
            return '/'.join(path)
        os.path.join = _join

        self.assertEqual(change_root('/root', '/old/its/here'),
                         '/root/old/its/here')
        self.assertEqual(change_root('/root', 'its/here'),
                         '/root/its/here')

        # windows
        os.name = 'nt'

        def _isabs(path):
            return path.startswith('c:\\')
        os.path.isabs = _isabs

        def _splitdrive(path):
            if path.startswith('c:'):
                return '', path.replace('c:', '')
            return '', path
        os.path.splitdrive = _splitdrive

        def _join(*path):
            return '\\'.join(path)
        os.path.join = _join

        self.assertEqual(change_root('c:\\root', 'c:\\old\\its\\here'),
                         'c:\\root\\old\\its\\here')
        self.assertEqual(change_root('c:\\root', 'its\\here'),
                         'c:\\root\\its\\here')

        # BugsBunny os (it's a great os)
        os.name = 'BugsBunny'
        self.assertRaises(PackagingPlatformError,
                          change_root, 'c:\\root', 'its\\here')

        # XXX platforms to be covered: os2, mac

    def test_split_quoted(self):
        self.assertEqual(split_quoted('""one"" "two" \'three\' \\four'),
                         ['one', 'two', 'three', 'four'])

    def test_strtobool(self):
        yes = ('y', 'Y', 'yes', 'True', 't', 'true', 'True', 'On', 'on', '1')
        no = ('n', 'no', 'f', 'false', 'off', '0', 'Off', 'No', 'N')

        for y in yes:
            self.assertTrue(strtobool(y))

        for n in no:
            self.assertFalse(strtobool(n))

    def test_rfc822_escape(self):
        header = 'I am a\npoor\nlonesome\nheader\n'
        res = rfc822_escape(header)
        wanted = ('I am a%(8s)spoor%(8s)slonesome%(8s)s'
                  'header%(8s)s') % {'8s': '\n' + 8 * ' '}
        self.assertEqual(res, wanted)

    def test_find_exe_version(self):
        # the ld version scheme under MAC OS is:
        #   ^@(#)PROGRAM:ld  PROJECT:ld64-VERSION
        #
        # where VERSION is a 2-digit number for major
        # revisions. For instance under Leopard, it's
        # currently 77
        #
        # Dots are used when branching is done.
        #
        # The SnowLeopard ld64 is currently 95.2.12

        for output, version in (('@(#)PROGRAM:ld  PROJECT:ld64-77', '77'),
                                ('@(#)PROGRAM:ld  PROJECT:ld64-95.2.12',
                                 '95.2.12')):
            result = _MAC_OS_X_LD_VERSION.search(output)
            self.assertEqual(result.group(1), version)

    def _find_executable(self, name):
        if name in self._exes:
            return name
        return None

    def test_get_compiler_versions(self):
        # get_versions calls distutils.spawn.find_executable on
        # 'gcc', 'ld' and 'dllwrap'
        self.assertEqual(get_compiler_versions(), (None, None, None))

        # Let's fake we have 'gcc' and it returns '3.4.5'
        self._exes['gcc'] = 'gcc (GCC) 3.4.5 (mingw special)\nFSF'
        res = get_compiler_versions()
        self.assertEqual(str(res[0]), '3.4.5')

        # and let's see what happens when the version
        # doesn't match the regular expression
        # (\d+\.\d+(\.\d+)*)
        self._exes['gcc'] = 'very strange output'
        res = get_compiler_versions()
        self.assertEqual(res[0], None)

        # same thing for ld
        if sys.platform != 'darwin':
            self._exes['ld'] = 'GNU ld version 2.17.50 20060824'
            res = get_compiler_versions()
            self.assertEqual(str(res[1]), '2.17.50')
            self._exes['ld'] = '@(#)PROGRAM:ld  PROJECT:ld64-77'
            res = get_compiler_versions()
            self.assertEqual(res[1], None)
        else:
            self._exes['ld'] = 'GNU ld version 2.17.50 20060824'
            res = get_compiler_versions()
            self.assertEqual(res[1], None)
            self._exes['ld'] = '@(#)PROGRAM:ld  PROJECT:ld64-77'
            res = get_compiler_versions()
            self.assertEqual(str(res[1]), '77')

        # and dllwrap
        self._exes['dllwrap'] = 'GNU dllwrap 2.17.50 20060824\nFSF'
        res = get_compiler_versions()
        self.assertEqual(str(res[2]), '2.17.50')
        self._exes['dllwrap'] = 'Cheese Wrap'
        res = get_compiler_versions()
        self.assertEqual(res[2], None)

    @unittest.skipUnless(hasattr(sys, 'dont_write_bytecode'),
                         'sys.dont_write_bytecode not supported')
    def test_dont_write_bytecode(self):
        # makes sure byte_compile raise a PackagingError
        # if sys.dont_write_bytecode is True
        old_dont_write_bytecode = sys.dont_write_bytecode
        sys.dont_write_bytecode = True
        try:
            self.assertRaises(PackagingByteCompileError, byte_compile, [])
        finally:
            sys.dont_write_bytecode = old_dont_write_bytecode

    def test_newer(self):
        self.assertRaises(PackagingFileError, util.newer, 'xxx', 'xxx')
        self.newer_f1 = self.mktempfile()
        time.sleep(1)
        self.newer_f2 = self.mktempfile()
        self.assertTrue(util.newer(self.newer_f2.name, self.newer_f1.name))

    def test_find_packages(self):
        # let's create a structure we want to scan:
        #
        #   pkg1
        #     __init__
        #     pkg2
        #       __init__
        #     pkg3
        #       __init__
        #       pkg6
        #           __init__
        #     pkg4    <--- not a pkg
        #       pkg8
        #          __init__
        #   pkg5
        #     __init__
        #
        root = self.mkdtemp()
        pkg1 = os.path.join(root, 'pkg1')
        os.mkdir(pkg1)
        self.write_file(os.path.join(pkg1, '__init__.py'))
        os.mkdir(os.path.join(pkg1, 'pkg2'))
        self.write_file(os.path.join(pkg1, 'pkg2', '__init__.py'))
        os.mkdir(os.path.join(pkg1, 'pkg3'))
        self.write_file(os.path.join(pkg1, 'pkg3', '__init__.py'))
        os.mkdir(os.path.join(pkg1, 'pkg3', 'pkg6'))
        self.write_file(os.path.join(pkg1, 'pkg3', 'pkg6', '__init__.py'))
        os.mkdir(os.path.join(pkg1, 'pkg4'))
        os.mkdir(os.path.join(pkg1, 'pkg4', 'pkg8'))
        self.write_file(os.path.join(pkg1, 'pkg4', 'pkg8', '__init__.py'))
        pkg5 = os.path.join(root, 'pkg5')
        os.mkdir(pkg5)
        self.write_file(os.path.join(pkg5, '__init__.py'))

        res = find_packages([root], ['pkg1.pkg2'])
        self.assertEqual(set(res), set(['pkg1', 'pkg5', 'pkg1.pkg3',
                                         'pkg1.pkg3.pkg6']))

    def test_resolve_name(self):
        self.assertIs(str, resolve_name('builtins.str'))
        self.assertEqual(
            UtilTestCase.__name__,
            resolve_name("packaging.tests.test_util.UtilTestCase").__name__)
        self.assertEqual(
            UtilTestCase.test_resolve_name.__name__,
            resolve_name("packaging.tests.test_util.UtilTestCase."
                         "test_resolve_name").__name__)

        self.assertRaises(ImportError, resolve_name,
                          "packaging.tests.test_util.UtilTestCaseNot")
        self.assertRaises(ImportError, resolve_name,
                          "packaging.tests.test_util.UtilTestCase."
                          "nonexistent_attribute")

    def test_import_nested_first_time(self):
        tmp_dir = self.mkdtemp()
        os.makedirs(os.path.join(tmp_dir, 'a', 'b'))
        self.write_file(os.path.join(tmp_dir, 'a', '__init__.py'), '')
        self.write_file(os.path.join(tmp_dir, 'a', 'b', '__init__.py'), '')
        self.write_file(os.path.join(tmp_dir, 'a', 'b', 'c.py'),
                                    'class Foo: pass')

        try:
            sys.path.append(tmp_dir)
            resolve_name("a.b.c.Foo")
            # assert nothing raised
        finally:
            sys.path.remove(tmp_dir)

    @unittest.skipIf(sys.version < '2.6', 'requires Python 2.6 or higher')
    def test_run_2to3_on_code(self):
        content = "print 'test'"
        converted_content = "print('test')"
        file_handle = self.mktempfile()
        file_name = file_handle.name
        file_handle.write(content)
        file_handle.flush()
        file_handle.seek(0)
        from packaging.util import run_2to3
        run_2to3([file_name])
        new_content = "".join(file_handle.read())
        file_handle.close()
        self.assertEqual(new_content, converted_content)

    @unittest.skipIf(sys.version < '2.6', 'requires Python 2.6 or higher')
    def test_run_2to3_on_doctests(self):
        # to check if text files containing doctests only get converted.
        content = ">>> print 'test'\ntest\n"
        converted_content = ">>> print('test')\ntest\n\n"
        file_handle = self.mktempfile()
        file_name = file_handle.name
        file_handle.write(content)
        file_handle.flush()
        file_handle.seek(0)
        from packaging.util import run_2to3
        run_2to3([file_name], doctests_only=True)
        new_content = "".join(file_handle.readlines())
        file_handle.close()
        self.assertEqual(new_content, converted_content)

    @unittest.skipUnless(os.name in ('nt', 'posix'),
                         'runs only under posix or nt')
    def test_spawn(self):
        # Do not patch subprocess on unix because
        # packaging.util._spawn_posix uses it
        if os.name in 'posix':
            subprocess.Popen = self.old_popen
        tmpdir = self.mkdtemp()

        # creating something executable
        # through the shell that returns 1
        if os.name == 'posix':
            exe = os.path.join(tmpdir, 'foo.sh')
            self.write_file(exe, '#!/bin/sh\nexit 1')
            os.chmod(exe, 0o777)
        else:
            exe = os.path.join(tmpdir, 'foo.bat')
            self.write_file(exe, 'exit 1')

        os.chmod(exe, 0o777)
        self.assertRaises(PackagingExecError, spawn, [exe])

        # now something that works
        if os.name == 'posix':
            exe = os.path.join(tmpdir, 'foo.sh')
            self.write_file(exe, '#!/bin/sh\nexit 0')
            os.chmod(exe, 0o777)
        else:
            exe = os.path.join(tmpdir, 'foo.bat')
            self.write_file(exe, 'exit 0')

        os.chmod(exe, 0o777)
        spawn([exe])  # should work without any error

    def test_server_registration(self):
        # This test makes sure we know how to:
        # 1. handle several sections in .pypirc
        # 2. handle the old format

        # new format
        self.write_file(self.rc, PYPIRC)
        config = read_pypirc()

        config = sorted(config.items())
        expected = [('password', 'xxxx'), ('realm', 'pypi'),
                    ('repository', 'http://pypi.python.org/pypi'),
                    ('server', 'pypi'), ('username', 'me')]
        self.assertEqual(config, expected)

        # old format
        self.write_file(self.rc, PYPIRC_OLD)
        config = read_pypirc()
        config = sorted(config.items())
        expected = [('password', 'secret'), ('realm', 'pypi'),
                    ('repository', 'http://pypi.python.org/pypi'),
                    ('server', 'server-login'), ('username', 'tarek')]
        self.assertEqual(config, expected)

    def test_server_empty_registration(self):
        rc = get_pypirc_path()
        self.assertFalse(os.path.exists(rc))
        generate_pypirc('tarek', 'xxx')
        self.assertTrue(os.path.exists(rc))
        with open(rc) as f:
            content = f.read()
        self.assertEqual(content, WANTED)


class GlobTestCaseBase(support.TempdirManager,
                       support.LoggingCatcher,
                       unittest.TestCase):

    def build_files_tree(self, files):
        tempdir = self.mkdtemp()
        for filepath in files:
            is_dir = filepath.endswith('/')
            filepath = os.path.join(tempdir, *filepath.split('/'))
            if is_dir:
                dirname = filepath
            else:
                dirname = os.path.dirname(filepath)
            if dirname and not os.path.exists(dirname):
                os.makedirs(dirname)
            if not is_dir:
                self.write_file(filepath, 'babar')
        return tempdir

    @staticmethod
    def os_dependent_path(path):
        path = path.rstrip('/').split('/')
        return os.path.join(*path)

    def clean_tree(self, spec):
        files = []
        for path, includes in spec.items():
            if includes:
                files.append(self.os_dependent_path(path))
        return files


class GlobTestCase(GlobTestCaseBase):

    def assertGlobMatch(self, glob, spec):
        """"""
        tempdir = self.build_files_tree(spec)
        expected = self.clean_tree(spec)
        self.addCleanup(os.chdir, os.getcwd())
        os.chdir(tempdir)
        result = list(iglob(glob))
        self.assertCountEqual(expected, result)

    def test_regex_rich_glob(self):
        matches = RICH_GLOB.findall(
                                r"babar aime les {fraises} est les {huitres}")
        self.assertEqual(["fraises", "huitres"], matches)

    def test_simple_glob(self):
        glob = '*.tp?'
        spec = {'coucou.tpl': True,
                 'coucou.tpj': True,
                 'Donotwant': False}
        self.assertGlobMatch(glob, spec)

    def test_simple_glob_in_dir(self):
        glob = 'babar/*.tp?'
        spec = {'babar/coucou.tpl': True,
                 'babar/coucou.tpj': True,
                 'babar/toto.bin': False,
                 'Donotwant': False}
        self.assertGlobMatch(glob, spec)

    def test_recursive_glob_head(self):
        glob = '**/tip/*.t?l'
        spec = {'babar/zaza/zuzu/tip/coucou.tpl': True,
                 'babar/z/tip/coucou.tpl': True,
                 'babar/tip/coucou.tpl': True,
                 'babar/zeop/tip/babar/babar.tpl': False,
                 'babar/z/tip/coucou.bin': False,
                 'babar/toto.bin': False,
                 'zozo/zuzu/tip/babar.tpl': True,
                 'zozo/tip/babar.tpl': True,
                 'Donotwant': False}
        self.assertGlobMatch(glob, spec)

    def test_recursive_glob_tail(self):
        glob = 'babar/**'
        spec = {'babar/zaza/': True,
                'babar/zaza/zuzu/': True,
                'babar/zaza/zuzu/babar.xml': True,
                'babar/zaza/zuzu/toto.xml': True,
                'babar/zaza/zuzu/toto.csv': True,
                'babar/zaza/coucou.tpl': True,
                'babar/bubu.tpl': True,
                'zozo/zuzu/tip/babar.tpl': False,
                'zozo/tip/babar.tpl': False,
                'Donotwant': False}
        self.assertGlobMatch(glob, spec)

    def test_recursive_glob_middle(self):
        glob = 'babar/**/tip/*.t?l'
        spec = {'babar/zaza/zuzu/tip/coucou.tpl': True,
                 'babar/z/tip/coucou.tpl': True,
                 'babar/tip/coucou.tpl': True,
                 'babar/zeop/tip/babar/babar.tpl': False,
                 'babar/z/tip/coucou.bin': False,
                 'babar/toto.bin': False,
                 'zozo/zuzu/tip/babar.tpl': False,
                 'zozo/tip/babar.tpl': False,
                 'Donotwant': False}
        self.assertGlobMatch(glob, spec)

    def test_glob_set_tail(self):
        glob = 'bin/*.{bin,sh,exe}'
        spec = {'bin/babar.bin': True,
                 'bin/zephir.sh': True,
                 'bin/celestine.exe': True,
                 'bin/cornelius.bat': False,
                 'bin/cornelius.xml': False,
                 'toto/yurg': False,
                 'Donotwant': False}
        self.assertGlobMatch(glob, spec)

    def test_glob_set_middle(self):
        glob = 'xml/{babar,toto}.xml'
        spec = {'xml/babar.xml': True,
                 'xml/toto.xml': True,
                 'xml/babar.xslt': False,
                 'xml/cornelius.sgml': False,
                 'xml/zephir.xml': False,
                 'toto/yurg.xml': False,
                 'Donotwant': False}
        self.assertGlobMatch(glob, spec)

    def test_glob_set_head(self):
        glob = '{xml,xslt}/babar.*'
        spec = {'xml/babar.xml': True,
                 'xml/toto.xml': False,
                 'xslt/babar.xslt': True,
                 'xslt/toto.xslt': False,
                 'toto/yurg.xml': False,
                 'Donotwant': False}
        self.assertGlobMatch(glob, spec)

    def test_glob_all(self):
        glob = '{xml/*,xslt/**}/babar.xml'
        spec = {'xml/a/babar.xml': True,
                 'xml/b/babar.xml': True,
                 'xml/a/c/babar.xml': False,
                 'xslt/a/babar.xml': True,
                 'xslt/b/babar.xml': True,
                 'xslt/a/c/babar.xml': True,
                 'toto/yurg.xml': False,
                 'Donotwant': False}
        self.assertGlobMatch(glob, spec)

    def test_invalid_glob_pattern(self):
        invalids = [
            'ppooa**',
            'azzaeaz4**/',
            '/**ddsfs',
            '**##1e"&e',
            'DSFb**c009',
            '{',
            '{aaQSDFa',
            '}',
            'aQSDFSaa}',
            '{**a,',
            ',**a}',
            '{a**,',
            ',b**}',
            '{a**a,babar}',
            '{bob,b**z}',
        ]
        msg = "%r is not supposed to be a valid pattern"
        for pattern in invalids:
            try:
                iglob(pattern)
            except ValueError:
                continue
            else:
                self.fail(msg % pattern)


class EggInfoToDistInfoTestCase(support.TempdirManager,
                                support.LoggingCatcher,
                                unittest.TestCase):

    def get_metadata_file_paths(self, distinfo_path):
        req_metadata_files = ['METADATA', 'RECORD', 'INSTALLER']
        metadata_file_paths = []
        for metadata_file in req_metadata_files:
            path = os.path.join(distinfo_path, metadata_file)
            metadata_file_paths.append(path)
        return metadata_file_paths

    def test_egginfo_to_distinfo_setuptools(self):
        distinfo = 'hello-0.1.1-py3.3.dist-info'
        egginfo = 'hello-0.1.1-py3.3.egg-info'
        dirs = [egginfo]
        files = ['hello.py', 'hello.pyc']
        extra_metadata = ['dependency_links.txt', 'entry_points.txt',
                          'not-zip-safe', 'PKG-INFO', 'top_level.txt',
                          'SOURCES.txt']
        for f in extra_metadata:
            files.append(os.path.join(egginfo, f))

        tempdir, record_file = self.build_dist_tree(files, dirs)
        distinfo_path = os.path.join(tempdir, distinfo)
        egginfo_path = os.path.join(tempdir, egginfo)
        metadata_file_paths = self.get_metadata_file_paths(distinfo_path)

        egginfo_to_distinfo(record_file)
        # test that directories and files get created
        self.assertTrue(os.path.isdir(distinfo_path))
        self.assertTrue(os.path.isdir(egginfo_path))

        for mfile in metadata_file_paths:
            self.assertTrue(os.path.isfile(mfile))

    def test_egginfo_to_distinfo_distutils(self):
        distinfo = 'hello-0.1.1-py3.3.dist-info'
        egginfo = 'hello-0.1.1-py3.3.egg-info'
        # egginfo is a file in distutils which contains the metadata
        files = ['hello.py', 'hello.pyc', egginfo]

        tempdir, record_file = self.build_dist_tree(files, dirs=[])
        distinfo_path = os.path.join(tempdir, distinfo)
        egginfo_path = os.path.join(tempdir, egginfo)
        metadata_file_paths = self.get_metadata_file_paths(distinfo_path)

        egginfo_to_distinfo(record_file)
        # test that directories and files get created
        self.assertTrue(os.path.isdir(distinfo_path))
        self.assertTrue(os.path.isfile(egginfo_path))

        for mfile in metadata_file_paths:
            self.assertTrue(os.path.isfile(mfile))

    def build_dist_tree(self, files, dirs):
        tempdir = self.mkdtemp()
        record_file_path = os.path.join(tempdir, 'RECORD')
        file_paths, dir_paths = ([], [])
        for d in dirs:
            path = os.path.join(tempdir, d)
            os.makedirs(path)
            dir_paths.append(path)
        for f in files:
            path = os.path.join(tempdir, f)
            with open(path, 'w') as _f:
                _f.write(f)
            file_paths.append(path)

        with open(record_file_path, 'w') as record_file:
            for fpath in file_paths:
                record_file.write(fpath + '\n')
            for dpath in dir_paths:
                record_file.write(dpath + '\n')

        return (tempdir, record_file_path)


class PackagingLibChecks(support.TempdirManager,
                         support.LoggingCatcher,
                         unittest.TestCase):

    def setUp(self):
        super(PackagingLibChecks, self).setUp()
        self._empty_dir = self.mkdtemp()

    def test_empty_package_is_not_based_on_anything(self):
        self.assertFalse(is_setuptools(self._empty_dir))
        self.assertFalse(is_distutils(self._empty_dir))
        self.assertFalse(is_packaging(self._empty_dir))

    def test_setup_py_importing_setuptools_is_setuptools_based(self):
        self.assertTrue(is_setuptools(self._setuptools_setup_py_pkg()))

    def test_egg_info_dir_and_setup_py_is_setuptools_based(self):
        self.assertTrue(is_setuptools(self._setuptools_egg_info_pkg()))

    def test_egg_info_and_non_setuptools_setup_py_is_setuptools_based(self):
        self.assertTrue(is_setuptools(self._egg_info_with_no_setuptools()))

    def test_setup_py_not_importing_setuptools_is_not_setuptools_based(self):
        self.assertFalse(is_setuptools(self._random_setup_py_pkg()))

    def test_setup_py_importing_distutils_is_distutils_based(self):
        self.assertTrue(is_distutils(self._distutils_setup_py_pkg()))

    def test_pkg_info_file_and_setup_py_is_distutils_based(self):
        self.assertTrue(is_distutils(self._distutils_pkg_info()))

    def test_pkg_info_and_non_distutils_setup_py_is_distutils_based(self):
        self.assertTrue(is_distutils(self._pkg_info_with_no_distutils()))

    def test_setup_py_not_importing_distutils_is_not_distutils_based(self):
        self.assertFalse(is_distutils(self._random_setup_py_pkg()))

    def test_setup_cfg_with_no_metadata_section_is_not_packaging_based(self):
        self.assertFalse(is_packaging(self._setup_cfg_with_no_metadata_pkg()))

    def test_setup_cfg_with_valid_metadata_section_is_packaging_based(self):
        self.assertTrue(is_packaging(self._valid_setup_cfg_pkg()))

    def test_setup_cfg_and_invalid_setup_cfg_is_not_packaging_based(self):
        self.assertFalse(is_packaging(self._invalid_setup_cfg_pkg()))

    def test_get_install_method_with_setuptools_pkg(self):
        path = self._setuptools_setup_py_pkg()
        self.assertEqual("setuptools", get_install_method(path))

    def test_get_install_method_with_distutils_pkg(self):
        path = self._distutils_pkg_info()
        self.assertEqual("distutils", get_install_method(path))

    def test_get_install_method_with_packaging_pkg(self):
        path = self._valid_setup_cfg_pkg()
        self.assertEqual("packaging", get_install_method(path))

    def test_get_install_method_with_unknown_pkg(self):
        path = self._invalid_setup_cfg_pkg()
        self.assertRaises(InstallationException, get_install_method, path)

    def test_is_setuptools_logs_setup_py_text_found(self):
        is_setuptools(self._setuptools_setup_py_pkg())
        expected = ['setup.py file found', 'found setuptools text in setup.py']
        self.assertEqual(expected, self.get_logs(logging.INFO))

    def test_is_setuptools_logs_setup_py_text_not_found(self):
        directory = self._random_setup_py_pkg()
        is_setuptools(directory)
        info_expected = ['setup.py file found']
        warn_expected = ['no egg-info directory found',
                         'no setuptools text found in setup.py']
        self.assertEqual(info_expected, self.get_logs(logging.INFO))
        self.assertEqual(warn_expected, self.get_logs(logging.WARN))

    def test_is_setuptools_logs_egg_info_dir_found(self):
        is_setuptools(self._setuptools_egg_info_pkg())
        expected = ['setup.py file found', 'found egg-info directory']
        self.assertEqual(expected, self.get_logs(logging.INFO))

    def test_is_distutils_logs_setup_py_text_found(self):
        is_distutils(self._distutils_setup_py_pkg())
        expected = ['setup.py file found', 'found distutils text in setup.py']
        self.assertEqual(expected, self.get_logs(logging.INFO))

    def test_is_distutils_logs_setup_py_text_not_found(self):
        directory = self._random_setup_py_pkg()
        is_distutils(directory)
        info_expected = ['setup.py file found']
        warn_expected = ['no PKG-INFO file found',
                         'no distutils text found in setup.py']
        self.assertEqual(info_expected, self.get_logs(logging.INFO))
        self.assertEqual(warn_expected, self.get_logs(logging.WARN))

    def test_is_distutils_logs_pkg_info_file_found(self):
        is_distutils(self._distutils_pkg_info())
        expected = ['setup.py file found', 'PKG-INFO file found']
        self.assertEqual(expected, self.get_logs(logging.INFO))

    def test_is_packaging_logs_setup_cfg_found(self):
        is_packaging(self._valid_setup_cfg_pkg())
        expected = ['setup.cfg file found']
        self.assertEqual(expected, self.get_logs(logging.INFO))

    def test_is_packaging_logs_setup_cfg_not_found(self):
        is_packaging(self._empty_dir)
        expected = ['no setup.cfg file found']
        self.assertEqual(expected, self.get_logs(logging.WARN))

    def _write_setuptools_setup_py(self, directory):
        self.write_file((directory, 'setup.py'),
                "from setuptools import setup")

    def _write_distutils_setup_py(self, directory):
        self.write_file([directory, 'setup.py'],
                "from distutils.core import setup")

    def _write_packaging_setup_cfg(self, directory):
        self.write_file([directory, 'setup.cfg'],
                        ("[metadata]\n"
                         "name = mypackage\n"
                         "version = 0.1.0\n"))

    def _setuptools_setup_py_pkg(self):
        tmp = self.mkdtemp()
        self._write_setuptools_setup_py(tmp)
        return tmp

    def _distutils_setup_py_pkg(self):
        tmp = self.mkdtemp()
        self._write_distutils_setup_py(tmp)
        return tmp

    def _valid_setup_cfg_pkg(self):
        tmp = self.mkdtemp()
        self._write_packaging_setup_cfg(tmp)
        return tmp

    def _setuptools_egg_info_pkg(self):
        tmp = self.mkdtemp()
        self._write_setuptools_setup_py(tmp)
        tempfile.mkdtemp(suffix='.egg-info', dir=tmp)
        return tmp

    def _distutils_pkg_info(self):
        tmp = self._distutils_setup_py_pkg()
        self.write_file([tmp, 'PKG-INFO'], '')
        return tmp

    def _setup_cfg_with_no_metadata_pkg(self):
        tmp = self.mkdtemp()
        self.write_file([tmp, 'setup.cfg'],
                        ("[othersection]\n"
                         "foo = bar\n"))
        return tmp

    def _invalid_setup_cfg_pkg(self):
        tmp = self.mkdtemp()
        self.write_file([tmp, 'setup.cfg'],
                        ("[metadata]\n"
                         "name = john\n"
                         "last_name = doe\n"))
        return tmp

    def _egg_info_with_no_setuptools(self):
        tmp = self._random_setup_py_pkg()
        tempfile.mkdtemp(suffix='.egg-info', dir=tmp)
        return tmp

    def _pkg_info_with_no_distutils(self):
        tmp = self._random_setup_py_pkg()
        self.write_file([tmp, 'PKG-INFO'], '')
        return tmp

    def _random_setup_py_pkg(self):
        tmp = self.mkdtemp()
        self.write_file((tmp, 'setup.py'), "from mypackage import setup")
        return tmp


def test_suite():
    suite = unittest.makeSuite(UtilTestCase)
    suite.addTest(unittest.makeSuite(GlobTestCase))
    suite.addTest(unittest.makeSuite(EggInfoToDistInfoTestCase))
    suite.addTest(unittest.makeSuite(PackagingLibChecks))
    return suite


if __name__ == "__main__":
    unittest.main(defaultTest="test_suite")
