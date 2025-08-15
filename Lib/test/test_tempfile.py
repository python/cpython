# tempfile.py unit tests.
import tempfile
import errno
import io
import os
import pathlib
import sys
import re
import warnings
import contextlib
import stat
import types
import weakref
import gc
import shutil
import subprocess
from unittest import mock

import unittest
from test import support
from test.support import os_helper
from test.support import script_helper
from test.support import warnings_helper


has_textmode = (tempfile._text_openflags != tempfile._bin_openflags)
has_spawnl = hasattr(os, 'spawnl')

# TEST_FILES may need to be tweaked for systems depending on the maximum
# number of files that can be opened at one time (see ulimit -n)
if sys.platform.startswith('openbsd'):
    TEST_FILES = 48
else:
    TEST_FILES = 100

# This is organized as one test for each chunk of code in tempfile.py,
# in order of their appearance in the file.  Testing which requires
# threads is not done here.

class TestLowLevelInternals(unittest.TestCase):
    def test_infer_return_type_singles(self):
        self.assertIs(str, tempfile._infer_return_type(''))
        self.assertIs(bytes, tempfile._infer_return_type(b''))
        self.assertIs(str, tempfile._infer_return_type(None))

    def test_infer_return_type_multiples(self):
        self.assertIs(str, tempfile._infer_return_type('', ''))
        self.assertIs(bytes, tempfile._infer_return_type(b'', b''))
        with self.assertRaises(TypeError):
            tempfile._infer_return_type('', b'')
        with self.assertRaises(TypeError):
            tempfile._infer_return_type(b'', '')

    def test_infer_return_type_multiples_and_none(self):
        self.assertIs(str, tempfile._infer_return_type(None, ''))
        self.assertIs(str, tempfile._infer_return_type('', None))
        self.assertIs(str, tempfile._infer_return_type(None, None))
        self.assertIs(bytes, tempfile._infer_return_type(b'', None))
        self.assertIs(bytes, tempfile._infer_return_type(None, b''))
        with self.assertRaises(TypeError):
            tempfile._infer_return_type('', None, b'')
        with self.assertRaises(TypeError):
            tempfile._infer_return_type(b'', None, '')

    def test_infer_return_type_pathlib(self):
        self.assertIs(str, tempfile._infer_return_type(os_helper.FakePath('/')))

    def test_infer_return_type_pathlike(self):
        Path = os_helper.FakePath
        self.assertIs(str, tempfile._infer_return_type(Path('/')))
        self.assertIs(bytes, tempfile._infer_return_type(Path(b'/')))
        self.assertIs(str, tempfile._infer_return_type('', Path('')))
        self.assertIs(bytes, tempfile._infer_return_type(b'', Path(b'')))
        self.assertIs(bytes, tempfile._infer_return_type(None, Path(b'')))
        self.assertIs(str, tempfile._infer_return_type(None, Path('')))

        with self.assertRaises(TypeError):
            tempfile._infer_return_type('', Path(b''))
        with self.assertRaises(TypeError):
            tempfile._infer_return_type(b'', Path(''))

# Common functionality.

class BaseTestCase(unittest.TestCase):

    str_check = re.compile(r"^[a-z0-9_-]{8}$")
    b_check = re.compile(br"^[a-z0-9_-]{8}$")

    def setUp(self):
        self.enterContext(warnings_helper.check_warnings())
        warnings.filterwarnings("ignore", category=RuntimeWarning,
                                message="mktemp", module=__name__)

    def nameCheck(self, name, dir, pre, suf):
        (ndir, nbase) = os.path.split(name)
        npre  = nbase[:len(pre)]
        nsuf  = nbase[len(nbase)-len(suf):]

        if dir is not None:
            self.assertIs(
                type(name),
                str
                if type(dir) is str or isinstance(dir, os.PathLike) else
                bytes,
                "unexpected return type",
            )
        if pre is not None:
            self.assertIs(type(name), str if type(pre) is str else bytes,
                          "unexpected return type")
        if suf is not None:
            self.assertIs(type(name), str if type(suf) is str else bytes,
                          "unexpected return type")
        if (dir, pre, suf) == (None, None, None):
            self.assertIs(type(name), str, "default return type must be str")

        # check for equality of the absolute paths!
        self.assertEqual(os.path.abspath(ndir), os.path.abspath(dir),
                         "file %r not in directory %r" % (name, dir))
        self.assertEqual(npre, pre,
                         "file %r does not begin with %r" % (nbase, pre))
        self.assertEqual(nsuf, suf,
                         "file %r does not end with %r" % (nbase, suf))

        nbase = nbase[len(pre):len(nbase)-len(suf)]
        check = self.str_check if isinstance(nbase, str) else self.b_check
        self.assertTrue(check.match(nbase),
                        "random characters %r do not match %r"
                        % (nbase, check.pattern))


class TestExports(BaseTestCase):
    def test_exports(self):
        # There are no surprising symbols in the tempfile module
        dict = tempfile.__dict__

        expected = {
            "NamedTemporaryFile" : 1,
            "TemporaryFile" : 1,
            "mkstemp" : 1,
            "mkdtemp" : 1,
            "mktemp" : 1,
            "TMP_MAX" : 1,
            "gettempprefix" : 1,
            "gettempprefixb" : 1,
            "gettempdir" : 1,
            "gettempdirb" : 1,
            "tempdir" : 1,
            "template" : 1,
            "SpooledTemporaryFile" : 1,
            "TemporaryDirectory" : 1,
        }

        unexp = []
        for key in dict:
            if key[0] != '_' and key not in expected:
                unexp.append(key)
        self.assertTrue(len(unexp) == 0,
                        "unexpected keys: %s" % unexp)


class TestRandomNameSequence(BaseTestCase):
    """Test the internal iterator object _RandomNameSequence."""

    def setUp(self):
        self.r = tempfile._RandomNameSequence()
        super().setUp()

    def test_get_eight_char_str(self):
        # _RandomNameSequence returns a eight-character string
        s = next(self.r)
        self.nameCheck(s, '', '', '')

    def test_many(self):
        # _RandomNameSequence returns no duplicate strings (stochastic)

        dict = {}
        r = self.r
        for i in range(TEST_FILES):
            s = next(r)
            self.nameCheck(s, '', '', '')
            self.assertNotIn(s, dict)
            dict[s] = 1

    def supports_iter(self):
        # _RandomNameSequence supports the iterator protocol

        i = 0
        r = self.r
        for s in r:
            i += 1
            if i == 20:
                break

    @support.requires_fork()
    def test_process_awareness(self):
        # ensure that the random source differs between
        # child and parent.
        read_fd, write_fd = os.pipe()
        pid = None
        try:
            pid = os.fork()
            if not pid:
                # child process
                os.close(read_fd)
                os.write(write_fd, next(self.r).encode("ascii"))
                os.close(write_fd)
                # bypass the normal exit handlers- leave those to
                # the parent.
                os._exit(0)

            # parent process
            parent_value = next(self.r)
            child_value = os.read(read_fd, len(parent_value)).decode("ascii")
        finally:
            if pid:
                support.wait_process(pid, exitcode=0)

            os.close(read_fd)
            os.close(write_fd)
        self.assertNotEqual(child_value, parent_value)



class TestCandidateTempdirList(BaseTestCase):
    """Test the internal function _candidate_tempdir_list."""

    def test_nonempty_list(self):
        # _candidate_tempdir_list returns a nonempty list of strings

        cand = tempfile._candidate_tempdir_list()

        self.assertFalse(len(cand) == 0)
        for c in cand:
            self.assertIsInstance(c, str)

    def test_wanted_dirs(self):
        # _candidate_tempdir_list contains the expected directories

        # Make sure the interesting environment variables are all set.
        with os_helper.EnvironmentVarGuard() as env:
            for envname in 'TMPDIR', 'TEMP', 'TMP':
                dirname = os.getenv(envname)
                if not dirname:
                    env[envname] = os.path.abspath(envname)

            cand = tempfile._candidate_tempdir_list()

            for envname in 'TMPDIR', 'TEMP', 'TMP':
                dirname = os.getenv(envname)
                if not dirname: raise ValueError
                self.assertIn(dirname, cand)

            try:
                dirname = os.getcwd()
            except (AttributeError, OSError):
                dirname = os.curdir

            self.assertIn(dirname, cand)

            # Not practical to try to verify the presence of OS-specific
            # paths in this list.


# We test _get_default_tempdir some more by testing gettempdir.

class TestGetDefaultTempdir(BaseTestCase):
    """Test _get_default_tempdir()."""

    def test_no_files_left_behind(self):
        # use a private empty directory
        with tempfile.TemporaryDirectory() as our_temp_directory:
            # force _get_default_tempdir() to consider our empty directory
            def our_candidate_list():
                return [our_temp_directory]

            with support.swap_attr(tempfile, "_candidate_tempdir_list",
                                   our_candidate_list):
                # verify our directory is empty after _get_default_tempdir()
                tempfile._get_default_tempdir()
                self.assertEqual(os.listdir(our_temp_directory), [])

                def raise_OSError(*args, **kwargs):
                    raise OSError()

                with support.swap_attr(os, "open", raise_OSError):
                    # test again with failing os.open()
                    with self.assertRaises(FileNotFoundError):
                        tempfile._get_default_tempdir()
                    self.assertEqual(os.listdir(our_temp_directory), [])

                with support.swap_attr(os, "write", raise_OSError):
                    # test again with failing os.write()
                    with self.assertRaises(FileNotFoundError):
                        tempfile._get_default_tempdir()
                    self.assertEqual(os.listdir(our_temp_directory), [])


class TestGetCandidateNames(BaseTestCase):
    """Test the internal function _get_candidate_names."""

    def test_retval(self):
        # _get_candidate_names returns a _RandomNameSequence object
        obj = tempfile._get_candidate_names()
        self.assertIsInstance(obj, tempfile._RandomNameSequence)

    def test_same_thing(self):
        # _get_candidate_names always returns the same object
        a = tempfile._get_candidate_names()
        b = tempfile._get_candidate_names()

        self.assertTrue(a is b)


@contextlib.contextmanager
def _inside_empty_temp_dir():
    dir = tempfile.mkdtemp()
    try:
        with support.swap_attr(tempfile, 'tempdir', dir):
            yield
    finally:
        os_helper.rmtree(dir)


def _mock_candidate_names(*names):
    return support.swap_attr(tempfile,
                             '_get_candidate_names',
                             lambda: iter(names))


class TestBadTempdir:

    @unittest.skipIf(
        support.is_emscripten, "Emscripten cannot remove write bits."
    )
    def test_read_only_directory(self):
        with _inside_empty_temp_dir():
            oldmode = mode = os.stat(tempfile.tempdir).st_mode
            mode &= ~(stat.S_IWUSR | stat.S_IWGRP | stat.S_IWOTH)
            os.chmod(tempfile.tempdir, mode)
            try:
                if os.access(tempfile.tempdir, os.W_OK):
                    self.skipTest("can't set the directory read-only")
                with self.assertRaises(PermissionError):
                    self.make_temp()
                self.assertEqual(os.listdir(tempfile.tempdir), [])
            finally:
                os.chmod(tempfile.tempdir, oldmode)

    def test_nonexisting_directory(self):
        with _inside_empty_temp_dir():
            tempdir = os.path.join(tempfile.tempdir, 'nonexistent')
            with support.swap_attr(tempfile, 'tempdir', tempdir):
                with self.assertRaises(FileNotFoundError):
                    self.make_temp()

    def test_non_directory(self):
        with _inside_empty_temp_dir():
            tempdir = os.path.join(tempfile.tempdir, 'file')
            open(tempdir, 'wb').close()
            with support.swap_attr(tempfile, 'tempdir', tempdir):
                with self.assertRaises((NotADirectoryError, FileNotFoundError)):
                    self.make_temp()


class TestMkstempInner(TestBadTempdir, BaseTestCase):
    """Test the internal function _mkstemp_inner."""

    class mkstemped:
        _bflags = tempfile._bin_openflags
        _tflags = tempfile._text_openflags
        _close = os.close
        _unlink = os.unlink

        def __init__(self, dir, pre, suf, bin):
            if bin: flags = self._bflags
            else:   flags = self._tflags

            output_type = tempfile._infer_return_type(dir, pre, suf)
            (self.fd, self.name) = tempfile._mkstemp_inner(dir, pre, suf, flags, output_type)

        def write(self, str):
            os.write(self.fd, str)

        def __del__(self):
            self._close(self.fd)
            self._unlink(self.name)

    def do_create(self, dir=None, pre=None, suf=None, bin=1):
        output_type = tempfile._infer_return_type(dir, pre, suf)
        if dir is None:
            if output_type is str:
                dir = tempfile.gettempdir()
            else:
                dir = tempfile.gettempdirb()
        if pre is None:
            pre = output_type()
        if suf is None:
            suf = output_type()
        file = self.mkstemped(dir, pre, suf, bin)

        self.nameCheck(file.name, dir, pre, suf)
        return file

    def test_basic(self):
        # _mkstemp_inner can create files
        self.do_create().write(b"blat")
        self.do_create(pre="a").write(b"blat")
        self.do_create(suf="b").write(b"blat")
        self.do_create(pre="a", suf="b").write(b"blat")
        self.do_create(pre="aa", suf=".txt").write(b"blat")

    def test_basic_with_bytes_names(self):
        # _mkstemp_inner can create files when given name parts all
        # specified as bytes.
        dir_b = tempfile.gettempdirb()
        self.do_create(dir=dir_b, suf=b"").write(b"blat")
        self.do_create(dir=dir_b, pre=b"a").write(b"blat")
        self.do_create(dir=dir_b, suf=b"b").write(b"blat")
        self.do_create(dir=dir_b, pre=b"a", suf=b"b").write(b"blat")
        self.do_create(dir=dir_b, pre=b"aa", suf=b".txt").write(b"blat")
        # Can't mix str & binary types in the args.
        with self.assertRaises(TypeError):
            self.do_create(dir="", suf=b"").write(b"blat")
        with self.assertRaises(TypeError):
            self.do_create(dir=dir_b, pre="").write(b"blat")
        with self.assertRaises(TypeError):
            self.do_create(dir=dir_b, pre=b"", suf="").write(b"blat")

    def test_basic_many(self):
        # _mkstemp_inner can create many files (stochastic)
        extant = list(range(TEST_FILES))
        for i in extant:
            extant[i] = self.do_create(pre="aa")

    def test_choose_directory(self):
        # _mkstemp_inner can create files in a user-selected directory
        dir = tempfile.mkdtemp()
        try:
            self.do_create(dir=dir).write(b"blat")
            self.do_create(dir=os_helper.FakePath(dir)).write(b"blat")
        finally:
            support.gc_collect()  # For PyPy or other GCs.
            os.rmdir(dir)

    @os_helper.skip_unless_working_chmod
    def test_file_mode(self):
        # _mkstemp_inner creates files with the proper mode

        file = self.do_create()
        mode = stat.S_IMODE(os.stat(file.name).st_mode)
        expected = 0o600
        if sys.platform == 'win32':
            # There's no distinction among 'user', 'group' and 'world';
            # replicate the 'user' bits.
            user = expected >> 6
            expected = user * (1 + 8 + 64)
        self.assertEqual(mode, expected)

    @unittest.skipUnless(has_spawnl, 'os.spawnl not available')
    @support.requires_subprocess()
    def test_noinherit(self):
        # _mkstemp_inner file handles are not inherited by child processes

        if support.verbose:
            v="v"
        else:
            v="q"

        file = self.do_create()
        self.assertEqual(os.get_inheritable(file.fd), False)
        fd = "%d" % file.fd

        try:
            me = __file__
        except NameError:
            me = sys.argv[0]

        # We have to exec something, so that FD_CLOEXEC will take
        # effect.  The core of this test is therefore in
        # tf_inherit_check.py, which see.
        tester = os.path.join(os.path.dirname(os.path.abspath(me)),
                              "tf_inherit_check.py")

        # On Windows a spawn* /path/ with embedded spaces shouldn't be quoted,
        # but an arg with embedded spaces should be decorated with double
        # quotes on each end
        if sys.platform == 'win32':
            decorated = '"%s"' % sys.executable
            tester = '"%s"' % tester
        else:
            decorated = sys.executable

        retval = os.spawnl(os.P_WAIT, sys.executable, decorated, tester, v, fd)
        self.assertFalse(retval < 0,
                    "child process caught fatal signal %d" % -retval)
        self.assertFalse(retval > 0, "child process reports failure %d"%retval)

    @unittest.skipUnless(has_textmode, "text mode not available")
    def test_textmode(self):
        # _mkstemp_inner can create files in text mode

        # A text file is truncated at the first Ctrl+Z byte
        f = self.do_create(bin=0)
        f.write(b"blat\x1a")
        f.write(b"extra\n")
        os.lseek(f.fd, 0, os.SEEK_SET)
        self.assertEqual(os.read(f.fd, 20), b"blat")

    def make_temp(self):
        return tempfile._mkstemp_inner(tempfile.gettempdir(),
                                       tempfile.gettempprefix(),
                                       '',
                                       tempfile._bin_openflags,
                                       str)

    def test_collision_with_existing_file(self):
        # _mkstemp_inner tries another name when a file with
        # the chosen name already exists
        with _inside_empty_temp_dir(), \
             _mock_candidate_names('aaa', 'aaa', 'bbb'):
            (fd1, name1) = self.make_temp()
            os.close(fd1)
            self.assertTrue(name1.endswith('aaa'))

            (fd2, name2) = self.make_temp()
            os.close(fd2)
            self.assertTrue(name2.endswith('bbb'))

    def test_collision_with_existing_directory(self):
        # _mkstemp_inner tries another name when a directory with
        # the chosen name already exists
        with _inside_empty_temp_dir(), \
             _mock_candidate_names('aaa', 'aaa', 'bbb'):
            dir = tempfile.mkdtemp()
            self.assertTrue(dir.endswith('aaa'))

            (fd, name) = self.make_temp()
            os.close(fd)
            self.assertTrue(name.endswith('bbb'))


class TestGetTempPrefix(BaseTestCase):
    """Test gettempprefix()."""

    def test_sane_template(self):
        # gettempprefix returns a nonempty prefix string
        p = tempfile.gettempprefix()

        self.assertIsInstance(p, str)
        self.assertGreater(len(p), 0)

        pb = tempfile.gettempprefixb()

        self.assertIsInstance(pb, bytes)
        self.assertGreater(len(pb), 0)

    def test_usable_template(self):
        # gettempprefix returns a usable prefix string

        # Create a temp directory, avoiding use of the prefix.
        # Then attempt to create a file whose name is
        # prefix + 'xxxxxx.xxx' in that directory.
        p = tempfile.gettempprefix() + "xxxxxx.xxx"
        d = tempfile.mkdtemp(prefix="")
        try:
            p = os.path.join(d, p)
            fd = os.open(p, os.O_RDWR | os.O_CREAT)
            os.close(fd)
            os.unlink(p)
        finally:
            os.rmdir(d)


class TestGetTempDir(BaseTestCase):
    """Test gettempdir()."""

    def test_directory_exists(self):
        # gettempdir returns a directory which exists

        for d in (tempfile.gettempdir(), tempfile.gettempdirb()):
            self.assertTrue(os.path.isabs(d) or d == os.curdir,
                            "%r is not an absolute path" % d)
            self.assertTrue(os.path.isdir(d),
                            "%r is not a directory" % d)

    def test_directory_writable(self):
        # gettempdir returns a directory writable by the user

        # sneaky: just instantiate a NamedTemporaryFile, which
        # defaults to writing into the directory returned by
        # gettempdir.
        with tempfile.NamedTemporaryFile() as file:
            file.write(b"blat")

    def test_same_thing(self):
        # gettempdir always returns the same object
        a = tempfile.gettempdir()
        b = tempfile.gettempdir()
        c = tempfile.gettempdirb()

        self.assertTrue(a is b)
        self.assertNotEqual(type(a), type(c))
        self.assertEqual(a, os.fsdecode(c))

    def test_case_sensitive(self):
        # gettempdir should not flatten its case
        # even on a case-insensitive file system
        case_sensitive_tempdir = tempfile.mkdtemp("-Temp")
        _tempdir, tempfile.tempdir = tempfile.tempdir, None
        try:
            with os_helper.EnvironmentVarGuard() as env:
                # Fake the first env var which is checked as a candidate
                env["TMPDIR"] = case_sensitive_tempdir
                self.assertEqual(tempfile.gettempdir(), case_sensitive_tempdir)
        finally:
            tempfile.tempdir = _tempdir
            os_helper.rmdir(case_sensitive_tempdir)


class TestMkstemp(BaseTestCase):
    """Test mkstemp()."""

    def do_create(self, dir=None, pre=None, suf=None):
        output_type = tempfile._infer_return_type(dir, pre, suf)
        if dir is None:
            if output_type is str:
                dir = tempfile.gettempdir()
            else:
                dir = tempfile.gettempdirb()
        if pre is None:
            pre = output_type()
        if suf is None:
            suf = output_type()
        (fd, name) = tempfile.mkstemp(dir=dir, prefix=pre, suffix=suf)
        (ndir, nbase) = os.path.split(name)
        adir = os.path.abspath(dir)
        self.assertEqual(adir, ndir,
            "Directory '%s' incorrectly returned as '%s'" % (adir, ndir))

        try:
            self.nameCheck(name, dir, pre, suf)
        finally:
            os.close(fd)
            os.unlink(name)

    def test_basic(self):
        # mkstemp can create files
        self.do_create()
        self.do_create(pre="a")
        self.do_create(suf="b")
        self.do_create(pre="a", suf="b")
        self.do_create(pre="aa", suf=".txt")
        self.do_create(dir=".")

    def test_basic_with_bytes_names(self):
        # mkstemp can create files when given name parts all
        # specified as bytes.
        d = tempfile.gettempdirb()
        self.do_create(dir=d, suf=b"")
        self.do_create(dir=d, pre=b"a")
        self.do_create(dir=d, suf=b"b")
        self.do_create(dir=d, pre=b"a", suf=b"b")
        self.do_create(dir=d, pre=b"aa", suf=b".txt")
        self.do_create(dir=b".")
        with self.assertRaises(TypeError):
            self.do_create(dir=".", pre=b"aa", suf=b".txt")
        with self.assertRaises(TypeError):
            self.do_create(dir=b".", pre="aa", suf=b".txt")
        with self.assertRaises(TypeError):
            self.do_create(dir=b".", pre=b"aa", suf=".txt")


    def test_choose_directory(self):
        # mkstemp can create directories in a user-selected directory
        dir = tempfile.mkdtemp()
        try:
            self.do_create(dir=dir)
            self.do_create(dir=os_helper.FakePath(dir))
        finally:
            os.rmdir(dir)

    def test_for_tempdir_is_bytes_issue40701_api_warts(self):
        orig_tempdir = tempfile.tempdir
        self.assertIsInstance(tempfile.tempdir, (str, type(None)))
        try:
            fd, path = tempfile.mkstemp()
            os.close(fd)
            os.unlink(path)
            self.assertIsInstance(path, str)
            tempfile.tempdir = tempfile.gettempdirb()
            self.assertIsInstance(tempfile.tempdir, bytes)
            self.assertIsInstance(tempfile.gettempdir(), str)
            self.assertIsInstance(tempfile.gettempdirb(), bytes)
            fd, path = tempfile.mkstemp()
            os.close(fd)
            os.unlink(path)
            self.assertIsInstance(path, bytes)
            fd, path = tempfile.mkstemp(suffix='.txt')
            os.close(fd)
            os.unlink(path)
            self.assertIsInstance(path, str)
            fd, path = tempfile.mkstemp(prefix='test-temp-')
            os.close(fd)
            os.unlink(path)
            self.assertIsInstance(path, str)
            fd, path = tempfile.mkstemp(dir=tempfile.gettempdir())
            os.close(fd)
            os.unlink(path)
            self.assertIsInstance(path, str)
        finally:
            tempfile.tempdir = orig_tempdir


class TestMkdtemp(TestBadTempdir, BaseTestCase):
    """Test mkdtemp()."""

    def make_temp(self):
        return tempfile.mkdtemp()

    def do_create(self, dir=None, pre=None, suf=None):
        output_type = tempfile._infer_return_type(dir, pre, suf)
        if dir is None:
            if output_type is str:
                dir = tempfile.gettempdir()
            else:
                dir = tempfile.gettempdirb()
        if pre is None:
            pre = output_type()
        if suf is None:
            suf = output_type()
        name = tempfile.mkdtemp(dir=dir, prefix=pre, suffix=suf)

        try:
            self.nameCheck(name, dir, pre, suf)
            return name
        except:
            os.rmdir(name)
            raise

    def test_basic(self):
        # mkdtemp can create directories
        os.rmdir(self.do_create())
        os.rmdir(self.do_create(pre="a"))
        os.rmdir(self.do_create(suf="b"))
        os.rmdir(self.do_create(pre="a", suf="b"))
        os.rmdir(self.do_create(pre="aa", suf=".txt"))

    def test_basic_with_bytes_names(self):
        # mkdtemp can create directories when given all binary parts
        d = tempfile.gettempdirb()
        os.rmdir(self.do_create(dir=d))
        os.rmdir(self.do_create(dir=d, pre=b"a"))
        os.rmdir(self.do_create(dir=d, suf=b"b"))
        os.rmdir(self.do_create(dir=d, pre=b"a", suf=b"b"))
        os.rmdir(self.do_create(dir=d, pre=b"aa", suf=b".txt"))
        with self.assertRaises(TypeError):
            os.rmdir(self.do_create(dir=d, pre="aa", suf=b".txt"))
        with self.assertRaises(TypeError):
            os.rmdir(self.do_create(dir=d, pre=b"aa", suf=".txt"))
        with self.assertRaises(TypeError):
            os.rmdir(self.do_create(dir="", pre=b"aa", suf=b".txt"))

    def test_basic_many(self):
        # mkdtemp can create many directories (stochastic)
        extant = list(range(TEST_FILES))
        try:
            for i in extant:
                extant[i] = self.do_create(pre="aa")
        finally:
            for i in extant:
                if(isinstance(i, str)):
                    os.rmdir(i)

    def test_choose_directory(self):
        # mkdtemp can create directories in a user-selected directory
        dir = tempfile.mkdtemp()
        try:
            os.rmdir(self.do_create(dir=dir))
            os.rmdir(self.do_create(dir=os_helper.FakePath(dir)))
        finally:
            os.rmdir(dir)

    @os_helper.skip_unless_working_chmod
    def test_mode(self):
        # mkdtemp creates directories with the proper mode

        dir = self.do_create()
        try:
            mode = stat.S_IMODE(os.stat(dir).st_mode)
            mode &= 0o777 # Mask off sticky bits inherited from /tmp
            expected = 0o700
            if sys.platform == 'win32':
                # There's no distinction among 'user', 'group' and 'world';
                # replicate the 'user' bits.
                user = expected >> 6
                expected = user * (1 + 8 + 64)
            self.assertEqual(mode, expected)
        finally:
            os.rmdir(dir)

    @unittest.skipUnless(os.name == "nt", "Only on Windows.")
    def test_mode_win32(self):
        # Use icacls.exe to extract the users with some level of access
        # Main thing we are testing is that the BUILTIN\Users group has
        # no access. The exact ACL is going to vary based on which user
        # is running the test.
        dir = self.do_create()
        try:
            out = subprocess.check_output(["icacls.exe", dir], encoding="oem").casefold()
        finally:
            os.rmdir(dir)

        dir = dir.casefold()
        users = set()
        found_user = False
        for line in out.strip().splitlines():
            acl = None
            # First line of result includes our directory
            if line.startswith(dir):
                acl = line.removeprefix(dir).strip()
            elif line and line[:1].isspace():
                acl = line.strip()
            if acl:
                users.add(acl.partition(":")[0])

        self.assertNotIn(r"BUILTIN\Users".casefold(), users)

    def test_collision_with_existing_file(self):
        # mkdtemp tries another name when a file with
        # the chosen name already exists
        with _inside_empty_temp_dir(), \
             _mock_candidate_names('aaa', 'aaa', 'bbb'):
            file = tempfile.NamedTemporaryFile(delete=False)
            file.close()
            self.assertTrue(file.name.endswith('aaa'))
            dir = tempfile.mkdtemp()
            self.assertTrue(dir.endswith('bbb'))

    def test_collision_with_existing_directory(self):
        # mkdtemp tries another name when a directory with
        # the chosen name already exists
        with _inside_empty_temp_dir(), \
             _mock_candidate_names('aaa', 'aaa', 'bbb'):
            dir1 = tempfile.mkdtemp()
            self.assertTrue(dir1.endswith('aaa'))
            dir2 = tempfile.mkdtemp()
            self.assertTrue(dir2.endswith('bbb'))

    def test_for_tempdir_is_bytes_issue40701_api_warts(self):
        orig_tempdir = tempfile.tempdir
        self.assertIsInstance(tempfile.tempdir, (str, type(None)))
        try:
            path = tempfile.mkdtemp()
            os.rmdir(path)
            self.assertIsInstance(path, str)
            tempfile.tempdir = tempfile.gettempdirb()
            self.assertIsInstance(tempfile.tempdir, bytes)
            self.assertIsInstance(tempfile.gettempdir(), str)
            self.assertIsInstance(tempfile.gettempdirb(), bytes)
            path = tempfile.mkdtemp()
            os.rmdir(path)
            self.assertIsInstance(path, bytes)
            path = tempfile.mkdtemp(suffix='-dir')
            os.rmdir(path)
            self.assertIsInstance(path, str)
            path = tempfile.mkdtemp(prefix='test-mkdtemp-')
            os.rmdir(path)
            self.assertIsInstance(path, str)
            path = tempfile.mkdtemp(dir=tempfile.gettempdir())
            os.rmdir(path)
            self.assertIsInstance(path, str)
        finally:
            tempfile.tempdir = orig_tempdir

    def test_path_is_absolute(self):
        # Test that the path returned by mkdtemp with a relative `dir`
        # argument is absolute
        try:
            path = tempfile.mkdtemp(dir=".")
            self.assertTrue(os.path.isabs(path))
        finally:
            os.rmdir(path)


class TestMktemp(BaseTestCase):
    """Test mktemp()."""

    # For safety, all use of mktemp must occur in a private directory.
    # We must also suppress the RuntimeWarning it generates.
    def setUp(self):
        self.dir = tempfile.mkdtemp()
        super().setUp()

    def tearDown(self):
        if self.dir:
            os.rmdir(self.dir)
            self.dir = None
        super().tearDown()

    class mktemped:
        _unlink = os.unlink
        _bflags = tempfile._bin_openflags

        def __init__(self, dir, pre, suf):
            self.name = tempfile.mktemp(dir=dir, prefix=pre, suffix=suf)
            # Create the file.  This will raise an exception if it's
            # mysteriously appeared in the meanwhile.
            os.close(os.open(self.name, self._bflags, 0o600))

        def __del__(self):
            self._unlink(self.name)

    def do_create(self, pre="", suf=""):
        file = self.mktemped(self.dir, pre, suf)

        self.nameCheck(file.name, self.dir, pre, suf)
        return file

    def test_basic(self):
        # mktemp can choose usable file names
        self.do_create()
        self.do_create(pre="a")
        self.do_create(suf="b")
        self.do_create(pre="a", suf="b")
        self.do_create(pre="aa", suf=".txt")

    def test_many(self):
        # mktemp can choose many usable file names (stochastic)
        extant = list(range(TEST_FILES))
        for i in extant:
            extant[i] = self.do_create(pre="aa")
        del extant
        support.gc_collect()  # For PyPy or other GCs.

##     def test_warning(self):
##         # mktemp issues a warning when used
##         warnings.filterwarnings("error",
##                                 category=RuntimeWarning,
##                                 message="mktemp")
##         self.assertRaises(RuntimeWarning,
##                           tempfile.mktemp, dir=self.dir)


# We test _TemporaryFileWrapper by testing NamedTemporaryFile.


class TestNamedTemporaryFile(BaseTestCase):
    """Test NamedTemporaryFile()."""

    def do_create(self, dir=None, pre="", suf="", delete=True):
        if dir is None:
            dir = tempfile.gettempdir()
        file = tempfile.NamedTemporaryFile(dir=dir, prefix=pre, suffix=suf,
                                           delete=delete)

        self.nameCheck(file.name, dir, pre, suf)
        return file


    def test_basic(self):
        # NamedTemporaryFile can create files
        self.do_create()
        self.do_create(pre="a")
        self.do_create(suf="b")
        self.do_create(pre="a", suf="b")
        self.do_create(pre="aa", suf=".txt")

    def test_method_lookup(self):
        # Issue #18879: Looking up a temporary file method should keep it
        # alive long enough.
        f = self.do_create()
        wr = weakref.ref(f)
        write = f.write
        write2 = f.write
        del f
        write(b'foo')
        del write
        write2(b'bar')
        del write2
        if support.check_impl_detail(cpython=True):
            # No reference cycle was created.
            self.assertIsNone(wr())

    def test_iter(self):
        # Issue #23700: getting iterator from a temporary file should keep
        # it alive as long as it's being iterated over
        lines = [b'spam\n', b'eggs\n', b'beans\n']
        def make_file():
            f = tempfile.NamedTemporaryFile(mode='w+b')
            f.write(b''.join(lines))
            f.seek(0)
            return f
        for i, l in enumerate(make_file()):
            self.assertEqual(l, lines[i])
        self.assertEqual(i, len(lines) - 1)

    def test_creates_named(self):
        # NamedTemporaryFile creates files with names
        f = tempfile.NamedTemporaryFile()
        self.assertTrue(os.path.exists(f.name),
                        "NamedTemporaryFile %s does not exist" % f.name)

    def test_del_on_close(self):
        # A NamedTemporaryFile is deleted when closed
        dir = tempfile.mkdtemp()
        try:
            with tempfile.NamedTemporaryFile(dir=dir) as f:
                f.write(b'blat')
            self.assertEqual(os.listdir(dir), [])
            self.assertFalse(os.path.exists(f.name),
                        "NamedTemporaryFile %s exists after close" % f.name)
        finally:
            os.rmdir(dir)

    def test_dis_del_on_close(self):
        # Tests that delete-on-close can be disabled
        dir = tempfile.mkdtemp()
        tmp = None
        try:
            f = tempfile.NamedTemporaryFile(dir=dir, delete=False)
            tmp = f.name
            f.write(b'blat')
            f.close()
            self.assertTrue(os.path.exists(f.name),
                        "NamedTemporaryFile %s missing after close" % f.name)
        finally:
            if tmp is not None:
                os.unlink(tmp)
            os.rmdir(dir)

    def test_multiple_close(self):
        # A NamedTemporaryFile can be closed many times without error
        f = tempfile.NamedTemporaryFile()
        f.write(b'abc\n')
        f.close()
        f.close()
        f.close()

    def test_context_manager(self):
        # A NamedTemporaryFile can be used as a context manager
        with tempfile.NamedTemporaryFile() as f:
            self.assertTrue(os.path.exists(f.name))
        self.assertFalse(os.path.exists(f.name))
        def use_closed():
            with f:
                pass
        self.assertRaises(ValueError, use_closed)

    def test_context_man_not_del_on_close_if_delete_on_close_false(self):
        # Issue gh-58451: tempfile.NamedTemporaryFile is not particularly useful
        # on Windows
        # A NamedTemporaryFile is NOT deleted when closed if
        # delete_on_close=False, but is deleted on context manager exit
        dir = tempfile.mkdtemp()
        try:
            with tempfile.NamedTemporaryFile(dir=dir,
                                             delete=True,
                                             delete_on_close=False) as f:
                f.write(b'blat')
                f_name = f.name
                f.close()
                with self.subTest():
                    # Testing that file is not deleted on close
                    self.assertTrue(os.path.exists(f.name),
                            f"NamedTemporaryFile {f.name!r} is incorrectly "
                            f"deleted on closure when delete_on_close=False")

            with self.subTest():
                # Testing that file is deleted on context manager exit
                self.assertFalse(os.path.exists(f.name),
                                 f"NamedTemporaryFile {f.name!r} exists "
                                 f"after context manager exit")

        finally:
            os.rmdir(dir)

    def test_context_man_ok_to_delete_manually(self):
        # In the case of delete=True, a NamedTemporaryFile can be manually
        # deleted in a with-statement context without causing an error.
        dir = tempfile.mkdtemp()
        try:
            with tempfile.NamedTemporaryFile(dir=dir,
                                             delete=True,
                                             delete_on_close=False) as f:
                f.write(b'blat')
                f.close()
                os.unlink(f.name)

        finally:
            os.rmdir(dir)

    def test_context_man_not_del_if_delete_false(self):
        # A NamedTemporaryFile is not deleted if delete = False
        dir = tempfile.mkdtemp()
        f_name = ""
        try:
            # Test that delete_on_close=True has no effect if delete=False.
            with tempfile.NamedTemporaryFile(dir=dir, delete=False,
                                             delete_on_close=True) as f:
                f.write(b'blat')
                f_name = f.name
            self.assertTrue(os.path.exists(f.name),
                        f"NamedTemporaryFile {f.name!r} exists after close")
        finally:
            os.unlink(f_name)
            os.rmdir(dir)

    def test_del_by_finalizer(self):
        # A NamedTemporaryFile is deleted when finalized in the case of
        # delete=True, delete_on_close=False, and no with-statement is used.
        def my_func(dir):
            f = tempfile.NamedTemporaryFile(dir=dir, delete=True,
                                            delete_on_close=False)
            tmp_name = f.name
            f.write(b'blat')
            # Testing extreme case, where the file is not explicitly closed
            # f.close()
            return tmp_name
        # Make sure that the garbage collector has finalized the file object.
        gc.collect()
        dir = tempfile.mkdtemp()
        try:
            tmp_name = my_func(dir)
            self.assertFalse(os.path.exists(tmp_name),
                        f"NamedTemporaryFile {tmp_name!r} "
                        f"exists after finalizer ")
        finally:
            os.rmdir(dir)

    def test_correct_finalizer_work_if_already_deleted(self):
        # There should be no error in the case of delete=True,
        # delete_on_close=False, no with-statement is used, and the file is
        # deleted manually.
        def my_func(dir)->str:
            f = tempfile.NamedTemporaryFile(dir=dir, delete=True,
                                            delete_on_close=False)
            tmp_name = f.name
            f.write(b'blat')
            f.close()
            os.unlink(tmp_name)
            return tmp_name
        # Make sure that the garbage collector has finalized the file object.
        gc.collect()

    def test_bad_mode(self):
        dir = tempfile.mkdtemp()
        self.addCleanup(os_helper.rmtree, dir)
        with self.assertRaises(ValueError):
            tempfile.NamedTemporaryFile(mode='wr', dir=dir)
        with self.assertRaises(TypeError):
            tempfile.NamedTemporaryFile(mode=2, dir=dir)
        self.assertEqual(os.listdir(dir), [])

    def test_bad_encoding(self):
        dir = tempfile.mkdtemp()
        self.addCleanup(os_helper.rmtree, dir)
        with self.assertRaises(LookupError):
            tempfile.NamedTemporaryFile('w', encoding='bad-encoding', dir=dir)
        self.assertEqual(os.listdir(dir), [])

    def test_unexpected_error(self):
        dir = tempfile.mkdtemp()
        self.addCleanup(os_helper.rmtree, dir)
        with mock.patch('tempfile._TemporaryFileWrapper') as mock_ntf, \
             mock.patch('io.open', mock.mock_open()) as mock_open:
            mock_ntf.side_effect = KeyboardInterrupt()
            with self.assertRaises(KeyboardInterrupt):
                tempfile.NamedTemporaryFile(dir=dir)
        mock_open().close.assert_called()
        self.assertEqual(os.listdir(dir), [])

    # How to test the mode and bufsize parameters?

class TestSpooledTemporaryFile(BaseTestCase):
    """Test SpooledTemporaryFile()."""

    def do_create(self, max_size=0, dir=None, pre="", suf=""):
        if dir is None:
            dir = tempfile.gettempdir()
        file = tempfile.SpooledTemporaryFile(max_size=max_size, dir=dir, prefix=pre, suffix=suf)

        return file


    def test_basic(self):
        # SpooledTemporaryFile can create files
        f = self.do_create()
        self.assertFalse(f._rolled)
        f = self.do_create(max_size=100, pre="a", suf=".txt")
        self.assertFalse(f._rolled)

    def test_is_iobase(self):
        # SpooledTemporaryFile should implement io.IOBase
        self.assertIsInstance(self.do_create(), io.IOBase)

    def test_iobase_interface(self):
        # SpooledTemporaryFile should implement the io.IOBase interface.
        # Ensure it has all the required methods and properties.
        iobase_attrs = {
            # From IOBase
            'fileno', 'seek', 'truncate', 'close', 'closed', '__enter__',
            '__exit__', 'flush', 'isatty', '__iter__', '__next__', 'readable',
            'readline', 'readlines', 'seekable', 'tell', 'writable',
            'writelines',
            # From BufferedIOBase (binary mode) and TextIOBase (text mode)
            'detach', 'read', 'read1', 'write', 'readinto', 'readinto1',
            'encoding', 'errors', 'newlines',
        }
        spooledtempfile_attrs = set(dir(tempfile.SpooledTemporaryFile))
        missing_attrs = iobase_attrs - spooledtempfile_attrs
        self.assertFalse(
            missing_attrs,
            'SpooledTemporaryFile missing attributes from '
            'IOBase/BufferedIOBase/TextIOBase'
        )

    def test_del_on_close(self):
        # A SpooledTemporaryFile is deleted when closed
        dir = tempfile.mkdtemp()
        try:
            f = tempfile.SpooledTemporaryFile(max_size=10, dir=dir)
            self.assertFalse(f._rolled)
            f.write(b'blat ' * 5)
            self.assertTrue(f._rolled)
            filename = f.name
            f.close()
            self.assertEqual(os.listdir(dir), [])
            if not isinstance(filename, int):
                self.assertFalse(os.path.exists(filename),
                    "SpooledTemporaryFile %s exists after close" % filename)
        finally:
            os.rmdir(dir)

    def test_del_unrolled_file(self):
        # The unrolled SpooledTemporaryFile should raise a ResourceWarning
        # when deleted since the file was not explicitly closed.
        f = self.do_create(max_size=10)
        f.write(b'foo')
        self.assertEqual(f.name, None)  # Unrolled so no filename/fd
        with self.assertWarns(ResourceWarning):
            f.__del__()

    @unittest.skipIf(
        support.is_emscripten, "Emscripten cannot fstat renamed files."
    )
    def test_del_rolled_file(self):
        # The rolled file should be deleted when the SpooledTemporaryFile
        # object is deleted. This should raise a ResourceWarning since the file
        # was not explicitly closed.
        f = self.do_create(max_size=2)
        f.write(b'foo')
        name = f.name  # This is a fd on posix+cygwin, a filename everywhere else
        self.assertTrue(os.path.exists(name))
        with self.assertWarns(ResourceWarning):
            f.__del__()
        self.assertFalse(
            os.path.exists(name),
            "Rolled SpooledTemporaryFile (name=%s) exists after delete" % name
        )

    def test_rewrite_small(self):
        # A SpooledTemporaryFile can be written to multiple within the max_size
        f = self.do_create(max_size=30)
        self.assertFalse(f._rolled)
        for i in range(5):
            f.seek(0, 0)
            f.write(b'x' * 20)
        self.assertFalse(f._rolled)

    def test_write_sequential(self):
        # A SpooledTemporaryFile should hold exactly max_size bytes, and roll
        # over afterward
        f = self.do_create(max_size=30)
        self.assertFalse(f._rolled)
        f.write(b'x' * 20)
        self.assertFalse(f._rolled)
        f.write(b'x' * 10)
        self.assertFalse(f._rolled)
        f.write(b'x')
        self.assertTrue(f._rolled)

    def test_writelines(self):
        # Verify writelines with a SpooledTemporaryFile
        f = self.do_create()
        f.writelines((b'x', b'y', b'z'))
        pos = f.seek(0)
        self.assertEqual(pos, 0)
        buf = f.read()
        self.assertEqual(buf, b'xyz')

    def test_writelines_rollover(self):
        # Verify writelines rolls over before exhausting the iterator
        f = self.do_create(max_size=2)

        def it():
            yield b'xy'
            self.assertFalse(f._rolled)
            yield b'z'
            self.assertTrue(f._rolled)

        f.writelines(it())
        pos = f.seek(0)
        self.assertEqual(pos, 0)
        buf = f.read()
        self.assertEqual(buf, b'xyz')

    def test_writelines_fast_path(self):
        f = self.do_create(max_size=2)
        f.write(b'abc')
        self.assertTrue(f._rolled)

        f.writelines([b'd', b'e', b'f'])
        pos = f.seek(0)
        self.assertEqual(pos, 0)
        buf = f.read()
        self.assertEqual(buf, b'abcdef')


    def test_writelines_sequential(self):
        # A SpooledTemporaryFile should hold exactly max_size bytes, and roll
        # over afterward
        f = self.do_create(max_size=35)
        f.writelines((b'x' * 20, b'x' * 10, b'x' * 5))
        self.assertFalse(f._rolled)
        f.write(b'x')
        self.assertTrue(f._rolled)

    def test_sparse(self):
        # A SpooledTemporaryFile that is written late in the file will extend
        # when that occurs
        f = self.do_create(max_size=30)
        self.assertFalse(f._rolled)
        pos = f.seek(100, 0)
        self.assertEqual(pos, 100)
        self.assertFalse(f._rolled)
        f.write(b'x')
        self.assertTrue(f._rolled)

    def test_fileno(self):
        # A SpooledTemporaryFile should roll over to a real file on fileno()
        f = self.do_create(max_size=30)
        self.assertFalse(f._rolled)
        self.assertTrue(f.fileno() > 0)
        self.assertTrue(f._rolled)

    def test_multiple_close_before_rollover(self):
        # A SpooledTemporaryFile can be closed many times without error
        f = tempfile.SpooledTemporaryFile()
        f.write(b'abc\n')
        self.assertFalse(f._rolled)
        f.close()
        f.close()
        f.close()

    def test_multiple_close_after_rollover(self):
        # A SpooledTemporaryFile can be closed many times without error
        f = tempfile.SpooledTemporaryFile(max_size=1)
        f.write(b'abc\n')
        self.assertTrue(f._rolled)
        f.close()
        f.close()
        f.close()

    def test_bound_methods(self):
        # It should be OK to steal a bound method from a SpooledTemporaryFile
        # and use it independently; when the file rolls over, those bound
        # methods should continue to function
        f = self.do_create(max_size=30)
        read = f.read
        write = f.write
        seek = f.seek

        write(b"a" * 35)
        write(b"b" * 35)
        seek(0, 0)
        self.assertEqual(read(70), b'a'*35 + b'b'*35)

    def test_properties(self):
        f = tempfile.SpooledTemporaryFile(max_size=10)
        f.write(b'x' * 10)
        self.assertFalse(f._rolled)
        self.assertEqual(f.mode, 'w+b')
        self.assertIsNone(f.name)
        with self.assertRaises(AttributeError):
            f.newlines
        with self.assertRaises(AttributeError):
            f.encoding
        with self.assertRaises(AttributeError):
            f.errors

        f.write(b'x')
        self.assertTrue(f._rolled)
        self.assertEqual(f.mode, 'rb+')
        self.assertIsNotNone(f.name)
        with self.assertRaises(AttributeError):
            f.newlines
        with self.assertRaises(AttributeError):
            f.encoding
        with self.assertRaises(AttributeError):
            f.errors

    def test_text_mode(self):
        # Creating a SpooledTemporaryFile with a text mode should produce
        # a file object reading and writing (Unicode) text strings.
        f = tempfile.SpooledTemporaryFile(mode='w+', max_size=10,
                                          encoding="utf-8")
        f.write("abc\n")
        f.seek(0)
        self.assertEqual(f.read(), "abc\n")
        f.write("def\n")
        f.seek(0)
        self.assertEqual(f.read(), "abc\ndef\n")
        self.assertFalse(f._rolled)
        self.assertEqual(f.mode, 'w+')
        self.assertIsNone(f.name)
        self.assertEqual(f.newlines, os.linesep)
        self.assertEqual(f.encoding, "utf-8")
        self.assertEqual(f.errors, "strict")

        f.write("xyzzy\n")
        f.seek(0)
        self.assertEqual(f.read(), "abc\ndef\nxyzzy\n")
        # Check that Ctrl+Z doesn't truncate the file
        f.write("foo\x1abar\n")
        f.seek(0)
        self.assertEqual(f.read(), "abc\ndef\nxyzzy\nfoo\x1abar\n")
        self.assertTrue(f._rolled)
        self.assertEqual(f.mode, 'w+')
        self.assertIsNotNone(f.name)
        self.assertEqual(f.newlines, os.linesep)
        self.assertEqual(f.encoding, "utf-8")
        self.assertEqual(f.errors, "strict")

    def test_text_newline_and_encoding(self):
        f = tempfile.SpooledTemporaryFile(mode='w+', max_size=10,
                                          newline='', encoding='utf-8',
                                          errors='ignore')
        f.write("\u039B\r\n")
        f.seek(0)
        self.assertEqual(f.read(), "\u039B\r\n")
        self.assertFalse(f._rolled)
        self.assertEqual(f.mode, 'w+')
        self.assertIsNone(f.name)
        self.assertIsNotNone(f.newlines)
        self.assertEqual(f.encoding, "utf-8")
        self.assertEqual(f.errors, "ignore")

        f.write("\u039C" * 10 + "\r\n")
        f.write("\u039D" * 20)
        f.seek(0)
        self.assertEqual(f.read(),
                "\u039B\r\n" + ("\u039C" * 10) + "\r\n" + ("\u039D" * 20))
        self.assertTrue(f._rolled)
        self.assertEqual(f.mode, 'w+')
        self.assertIsNotNone(f.name)
        self.assertIsNotNone(f.newlines)
        self.assertEqual(f.encoding, 'utf-8')
        self.assertEqual(f.errors, 'ignore')

    def test_context_manager_before_rollover(self):
        # A SpooledTemporaryFile can be used as a context manager
        with tempfile.SpooledTemporaryFile(max_size=1) as f:
            self.assertFalse(f._rolled)
            self.assertFalse(f.closed)
        self.assertTrue(f.closed)
        def use_closed():
            with f:
                pass
        self.assertRaises(ValueError, use_closed)

    def test_context_manager_during_rollover(self):
        # A SpooledTemporaryFile can be used as a context manager
        with tempfile.SpooledTemporaryFile(max_size=1) as f:
            self.assertFalse(f._rolled)
            f.write(b'abc\n')
            f.flush()
            self.assertTrue(f._rolled)
            self.assertFalse(f.closed)
        self.assertTrue(f.closed)
        def use_closed():
            with f:
                pass
        self.assertRaises(ValueError, use_closed)

    def test_context_manager_after_rollover(self):
        # A SpooledTemporaryFile can be used as a context manager
        f = tempfile.SpooledTemporaryFile(max_size=1)
        f.write(b'abc\n')
        f.flush()
        self.assertTrue(f._rolled)
        with f:
            self.assertFalse(f.closed)
        self.assertTrue(f.closed)
        def use_closed():
            with f:
                pass
        self.assertRaises(ValueError, use_closed)

    @unittest.skipIf(
        support.is_emscripten, "Emscripten cannot fstat renamed files."
    )
    def test_truncate_with_size_parameter(self):
        # A SpooledTemporaryFile can be truncated to zero size
        f = tempfile.SpooledTemporaryFile(max_size=10)
        f.write(b'abcdefg\n')
        f.seek(0)
        f.truncate()
        self.assertFalse(f._rolled)
        self.assertEqual(f._file.getvalue(), b'')
        # A SpooledTemporaryFile can be truncated to a specific size
        f = tempfile.SpooledTemporaryFile(max_size=10)
        f.write(b'abcdefg\n')
        f.truncate(4)
        self.assertFalse(f._rolled)
        self.assertEqual(f._file.getvalue(), b'abcd')
        # A SpooledTemporaryFile rolls over if truncated to large size
        f = tempfile.SpooledTemporaryFile(max_size=10)
        f.write(b'abcdefg\n')
        f.truncate(20)
        self.assertTrue(f._rolled)
        self.assertEqual(os.fstat(f.fileno()).st_size, 20)

    def test_class_getitem(self):
        self.assertIsInstance(tempfile.SpooledTemporaryFile[bytes],
                      types.GenericAlias)

if tempfile.NamedTemporaryFile is not tempfile.TemporaryFile:

    class TestTemporaryFile(BaseTestCase):
        """Test TemporaryFile()."""

        def test_basic(self):
            # TemporaryFile can create files
            # No point in testing the name params - the file has no name.
            tempfile.TemporaryFile()

        def test_has_no_name(self):
            # TemporaryFile creates files with no names (on this system)
            dir = tempfile.mkdtemp()
            f = tempfile.TemporaryFile(dir=dir)
            f.write(b'blat')

            # Sneaky: because this file has no name, it should not prevent
            # us from removing the directory it was created in.
            try:
                os.rmdir(dir)
            except:
                # cleanup
                f.close()
                os.rmdir(dir)
                raise

        def test_multiple_close(self):
            # A TemporaryFile can be closed many times without error
            f = tempfile.TemporaryFile()
            f.write(b'abc\n')
            f.close()
            f.close()
            f.close()

        # How to test the mode and bufsize parameters?
        def test_mode_and_encoding(self):

            def roundtrip(input, *args, **kwargs):
                with tempfile.TemporaryFile(*args, **kwargs) as fileobj:
                    fileobj.write(input)
                    fileobj.seek(0)
                    self.assertEqual(input, fileobj.read())

            roundtrip(b"1234", "w+b")
            roundtrip("abdc\n", "w+")
            roundtrip("\u039B", "w+", encoding="utf-16")
            roundtrip("foo\r\n", "w+", newline="")

        def test_bad_mode(self):
            dir = tempfile.mkdtemp()
            self.addCleanup(os_helper.rmtree, dir)
            with self.assertRaises(ValueError):
                tempfile.TemporaryFile(mode='wr', dir=dir)
            with self.assertRaises(TypeError):
                tempfile.TemporaryFile(mode=2, dir=dir)
            self.assertEqual(os.listdir(dir), [])

        def test_bad_encoding(self):
            dir = tempfile.mkdtemp()
            self.addCleanup(os_helper.rmtree, dir)
            with self.assertRaises(LookupError):
                tempfile.TemporaryFile('w', encoding='bad-encoding', dir=dir)
            self.assertEqual(os.listdir(dir), [])

        def test_unexpected_error(self):
            dir = tempfile.mkdtemp()
            self.addCleanup(os_helper.rmtree, dir)
            with mock.patch('tempfile._O_TMPFILE_WORKS', False), \
                 mock.patch('os.unlink') as mock_unlink, \
                 mock.patch('os.open') as mock_open, \
                 mock.patch('os.close') as mock_close:
                mock_unlink.side_effect = KeyboardInterrupt()
                with self.assertRaises(KeyboardInterrupt):
                    tempfile.TemporaryFile(dir=dir)
            mock_close.assert_called()
            self.assertEqual(os.listdir(dir), [])


# Helper for test_del_on_shutdown
class NulledModules:
    def __init__(self, *modules):
        self.refs = [mod.__dict__ for mod in modules]
        self.contents = [ref.copy() for ref in self.refs]

    def __enter__(self):
        for d in self.refs:
            for key in d:
                d[key] = None

    def __exit__(self, *exc_info):
        for d, c in zip(self.refs, self.contents):
            d.clear()
            d.update(c)


class TestTemporaryDirectory(BaseTestCase):
    """Test TemporaryDirectory()."""

    def do_create(self, dir=None, pre="", suf="", recurse=1, dirs=1, files=1,
                  ignore_cleanup_errors=False):
        if dir is None:
            dir = tempfile.gettempdir()
        tmp = tempfile.TemporaryDirectory(
            dir=dir, prefix=pre, suffix=suf,
            ignore_cleanup_errors=ignore_cleanup_errors)
        self.nameCheck(tmp.name, dir, pre, suf)
        self.do_create2(tmp.name, recurse, dirs, files)
        return tmp

    def do_create2(self, path, recurse=1, dirs=1, files=1):
        # Create subdirectories and some files
        if recurse:
            for i in range(dirs):
                name = os.path.join(path, "dir%d" % i)
                os.mkdir(name)
                self.do_create2(name, recurse-1, dirs, files)
        for i in range(files):
            with open(os.path.join(path, "test%d.txt" % i), "wb") as f:
                f.write(b"Hello world!")

    def test_mkdtemp_failure(self):
        # Check no additional exception if mkdtemp fails
        # Previously would raise AttributeError instead
        # (noted as part of Issue #10188)
        with tempfile.TemporaryDirectory() as nonexistent:
            pass
        with self.assertRaises(FileNotFoundError) as cm:
            tempfile.TemporaryDirectory(dir=nonexistent)
        self.assertEqual(cm.exception.errno, errno.ENOENT)

    def test_explicit_cleanup(self):
        # A TemporaryDirectory is deleted when cleaned up
        dir = tempfile.mkdtemp()
        try:
            d = self.do_create(dir=dir)
            self.assertTrue(os.path.exists(d.name),
                            "TemporaryDirectory %s does not exist" % d.name)
            d.cleanup()
            self.assertFalse(os.path.exists(d.name),
                        "TemporaryDirectory %s exists after cleanup" % d.name)
        finally:
            os.rmdir(dir)

    def test_explicit_cleanup_ignore_errors(self):
        """Test that cleanup doesn't return an error when ignoring them."""
        with tempfile.TemporaryDirectory() as working_dir:
            temp_dir = self.do_create(
                dir=working_dir, ignore_cleanup_errors=True)
            temp_path = pathlib.Path(temp_dir.name)
            self.assertTrue(temp_path.exists(),
                            f"TemporaryDirectory {temp_path!s} does not exist")
            with open(temp_path / "a_file.txt", "w+t") as open_file:
                open_file.write("Hello world!\n")
                temp_dir.cleanup()
            self.assertEqual(len(list(temp_path.glob("*"))),
                             int(sys.platform.startswith("win")),
                             "Unexpected number of files in "
                             f"TemporaryDirectory {temp_path!s}")
            self.assertEqual(
                temp_path.exists(),
                sys.platform.startswith("win"),
                f"TemporaryDirectory {temp_path!s} existence state unexpected")
            temp_dir.cleanup()
            self.assertFalse(
                temp_path.exists(),
                f"TemporaryDirectory {temp_path!s} exists after cleanup")

    @unittest.skipUnless(os.name == "nt", "Only on Windows.")
    def test_explicit_cleanup_correct_error(self):
        with tempfile.TemporaryDirectory() as working_dir:
            temp_dir = self.do_create(dir=working_dir)
            with open(os.path.join(temp_dir.name, "example.txt"), 'wb'):
                # Previously raised NotADirectoryError on some OSes
                # (e.g. Windows). See bpo-43153.
                with self.assertRaises(PermissionError):
                    temp_dir.cleanup()

    @unittest.skipUnless(os.name == "nt", "Only on Windows.")
    def test_cleanup_with_used_directory(self):
        with tempfile.TemporaryDirectory() as working_dir:
            temp_dir = self.do_create(dir=working_dir)
            subdir = os.path.join(temp_dir.name, "subdir")
            os.mkdir(subdir)
            with os_helper.change_cwd(subdir):
                # Previously raised RecursionError on some OSes
                # (e.g. Windows). See bpo-35144.
                with self.assertRaises(PermissionError):
                    temp_dir.cleanup()

    @os_helper.skip_unless_symlink
    def test_cleanup_with_symlink_to_a_directory(self):
        # cleanup() should not follow symlinks to directories (issue #12464)
        d1 = self.do_create()
        d2 = self.do_create(recurse=0)

        # Symlink d1/foo -> d2
        os.symlink(d2.name, os.path.join(d1.name, "foo"))

        # This call to cleanup() should not follow the "foo" symlink
        d1.cleanup()

        self.assertFalse(os.path.exists(d1.name),
                         "TemporaryDirectory %s exists after cleanup" % d1.name)
        self.assertTrue(os.path.exists(d2.name),
                        "Directory pointed to by a symlink was deleted")
        self.assertEqual(os.listdir(d2.name), ['test0.txt'],
                         "Contents of the directory pointed to by a symlink "
                         "were deleted")
        d2.cleanup()

    @os_helper.skip_unless_symlink
    def test_cleanup_with_symlink_modes(self):
        # cleanup() should not follow symlinks when fixing mode bits (#91133)
        with self.do_create(recurse=0) as d2:
            file1 = os.path.join(d2, 'file1')
            open(file1, 'wb').close()
            dir1 = os.path.join(d2, 'dir1')
            os.mkdir(dir1)
            for mode in range(8):
                mode <<= 6
                with self.subTest(mode=format(mode, '03o')):
                    def test(target, target_is_directory):
                        d1 = self.do_create(recurse=0)
                        symlink = os.path.join(d1.name, 'symlink')
                        os.symlink(target, symlink,
                                target_is_directory=target_is_directory)
                        try:
                            os.chmod(symlink, mode, follow_symlinks=False)
                        except NotImplementedError:
                            pass
                        try:
                            os.chmod(symlink, mode)
                        except FileNotFoundError:
                            pass
                        os.chmod(d1.name, mode)
                        d1.cleanup()
                        self.assertFalse(os.path.exists(d1.name))

                    with self.subTest('nonexisting file'):
                        test('nonexisting', target_is_directory=False)
                    with self.subTest('nonexisting dir'):
                        test('nonexisting', target_is_directory=True)

                    with self.subTest('existing file'):
                        os.chmod(file1, mode)
                        old_mode = os.stat(file1).st_mode
                        test(file1, target_is_directory=False)
                        new_mode = os.stat(file1).st_mode
                        self.assertEqual(new_mode, old_mode,
                                         '%03o != %03o' % (new_mode, old_mode))

                    with self.subTest('existing dir'):
                        os.chmod(dir1, mode)
                        old_mode = os.stat(dir1).st_mode
                        test(dir1, target_is_directory=True)
                        new_mode = os.stat(dir1).st_mode
                        self.assertEqual(new_mode, old_mode,
                                         '%03o != %03o' % (new_mode, old_mode))

    @unittest.skipUnless(hasattr(os, 'chflags'), 'requires os.chflags')
    @os_helper.skip_unless_symlink
    def test_cleanup_with_symlink_flags(self):
        # cleanup() should not follow symlinks when fixing flags (#91133)
        flags = stat.UF_IMMUTABLE | stat.UF_NOUNLINK
        self.check_flags(flags)

        with self.do_create(recurse=0) as d2:
            file1 = os.path.join(d2, 'file1')
            open(file1, 'wb').close()
            dir1 = os.path.join(d2, 'dir1')
            os.mkdir(dir1)
            def test(target, target_is_directory):
                d1 = self.do_create(recurse=0)
                symlink = os.path.join(d1.name, 'symlink')
                os.symlink(target, symlink,
                           target_is_directory=target_is_directory)
                try:
                    os.chflags(symlink, flags, follow_symlinks=False)
                except NotImplementedError:
                    pass
                try:
                    os.chflags(symlink, flags)
                except FileNotFoundError:
                    pass
                os.chflags(d1.name, flags)
                d1.cleanup()
                self.assertFalse(os.path.exists(d1.name))

            with self.subTest('nonexisting file'):
                test('nonexisting', target_is_directory=False)
            with self.subTest('nonexisting dir'):
                test('nonexisting', target_is_directory=True)

            with self.subTest('existing file'):
                os.chflags(file1, flags)
                old_flags = os.stat(file1).st_flags
                test(file1, target_is_directory=False)
                new_flags = os.stat(file1).st_flags
                self.assertEqual(new_flags, old_flags)

            with self.subTest('existing dir'):
                os.chflags(dir1, flags)
                old_flags = os.stat(dir1).st_flags
                test(dir1, target_is_directory=True)
                new_flags = os.stat(dir1).st_flags
                self.assertEqual(new_flags, old_flags)

    @support.cpython_only
    def test_del_on_collection(self):
        # A TemporaryDirectory is deleted when garbage collected
        dir = tempfile.mkdtemp()
        try:
            d = self.do_create(dir=dir)
            name = d.name
            del d # Rely on refcounting to invoke __del__
            self.assertFalse(os.path.exists(name),
                        "TemporaryDirectory %s exists after __del__" % name)
        finally:
            os.rmdir(dir)

    @support.cpython_only
    def test_del_on_collection_ignore_errors(self):
        """Test that ignoring errors works when TemporaryDirectory is gced."""
        with tempfile.TemporaryDirectory() as working_dir:
            temp_dir = self.do_create(
                dir=working_dir, ignore_cleanup_errors=True)
            temp_path = pathlib.Path(temp_dir.name)
            self.assertTrue(temp_path.exists(),
                            f"TemporaryDirectory {temp_path!s} does not exist")
            with open(temp_path / "a_file.txt", "w+t") as open_file:
                open_file.write("Hello world!\n")
                del temp_dir
            self.assertEqual(len(list(temp_path.glob("*"))),
                             int(sys.platform.startswith("win")),
                             "Unexpected number of files in "
                             f"TemporaryDirectory {temp_path!s}")
            self.assertEqual(
                temp_path.exists(),
                sys.platform.startswith("win"),
                f"TemporaryDirectory {temp_path!s} existence state unexpected")

    def test_del_on_shutdown(self):
        # A TemporaryDirectory may be cleaned up during shutdown
        with self.do_create() as dir:
            for mod in ('builtins', 'os', 'shutil', 'sys', 'tempfile', 'warnings'):
                code = """if True:
                    import builtins
                    import os
                    import shutil
                    import sys
                    import tempfile
                    import warnings

                    tmp = tempfile.TemporaryDirectory(dir={dir!r})
                    sys.stdout.buffer.write(tmp.name.encode())

                    tmp2 = os.path.join(tmp.name, 'test_dir')
                    os.mkdir(tmp2)
                    with open(os.path.join(tmp2, "test0.txt"), "w") as f:
                        f.write("Hello world!")

                    {mod}.tmp = tmp

                    warnings.filterwarnings("always", category=ResourceWarning)
                    """.format(dir=dir, mod=mod)
                rc, out, err = script_helper.assert_python_ok("-c", code)
                tmp_name = out.decode().strip()
                self.assertFalse(os.path.exists(tmp_name),
                            "TemporaryDirectory %s exists after cleanup" % tmp_name)
                err = err.decode('utf-8', 'backslashreplace')
                self.assertNotIn("Exception ", err)
                self.assertIn("ResourceWarning: Implicitly cleaning up", err)

    def test_del_on_shutdown_ignore_errors(self):
        """Test ignoring errors works when a tempdir is gc'ed on shutdown."""
        with tempfile.TemporaryDirectory() as working_dir:
            code = """if True:
                import pathlib
                import sys
                import tempfile
                import warnings

                temp_dir = tempfile.TemporaryDirectory(
                    dir={working_dir!r}, ignore_cleanup_errors=True)
                sys.stdout.buffer.write(temp_dir.name.encode())

                temp_dir_2 = pathlib.Path(temp_dir.name) / "test_dir"
                temp_dir_2.mkdir()
                with open(temp_dir_2 / "test0.txt", "w") as test_file:
                    test_file.write("Hello world!")
                open_file = open(temp_dir_2 / "open_file.txt", "w")
                open_file.write("Hello world!")

                warnings.filterwarnings("always", category=ResourceWarning)
                """.format(working_dir=working_dir)
            __, out, err = script_helper.assert_python_ok("-c", code)
            temp_path = pathlib.Path(out.decode().strip())
            self.assertEqual(len(list(temp_path.glob("*"))),
                             int(sys.platform.startswith("win")),
                             "Unexpected number of files in "
                             f"TemporaryDirectory {temp_path!s}")
            self.assertEqual(
                temp_path.exists(),
                sys.platform.startswith("win"),
                f"TemporaryDirectory {temp_path!s} existence state unexpected")
            err = err.decode('utf-8', 'backslashreplace')
            self.assertNotIn("Exception", err)
            self.assertNotIn("Error", err)
            self.assertIn("ResourceWarning: Implicitly cleaning up", err)

    def test_exit_on_shutdown(self):
        # Issue #22427
        with self.do_create() as dir:
            code = """if True:
                import sys
                import tempfile
                import warnings

                def generator():
                    with tempfile.TemporaryDirectory(dir={dir!r}) as tmp:
                        yield tmp
                g = generator()
                sys.stdout.buffer.write(next(g).encode())

                warnings.filterwarnings("always", category=ResourceWarning)
                """.format(dir=dir)
            rc, out, err = script_helper.assert_python_ok("-c", code)
            tmp_name = out.decode().strip()
            self.assertFalse(os.path.exists(tmp_name),
                        "TemporaryDirectory %s exists after cleanup" % tmp_name)
            err = err.decode('utf-8', 'backslashreplace')
            self.assertNotIn("Exception ", err)
            self.assertIn("ResourceWarning: Implicitly cleaning up", err)

    def test_warnings_on_cleanup(self):
        # ResourceWarning will be triggered by __del__
        with self.do_create() as dir:
            d = self.do_create(dir=dir, recurse=3)
            name = d.name

            # Check for the resource warning
            with warnings_helper.check_warnings(('Implicitly',
                                                 ResourceWarning),
                                                quiet=False):
                warnings.filterwarnings("always", category=ResourceWarning)
                del d
                support.gc_collect()
            self.assertFalse(os.path.exists(name),
                        "TemporaryDirectory %s exists after __del__" % name)

    def test_multiple_close(self):
        # Can be cleaned-up many times without error
        d = self.do_create()
        d.cleanup()
        d.cleanup()
        d.cleanup()

    def test_context_manager(self):
        # Can be used as a context manager
        d = self.do_create()
        with d as name:
            self.assertTrue(os.path.exists(name))
            self.assertEqual(name, d.name)
        self.assertFalse(os.path.exists(name))

    def test_modes(self):
        for mode in range(8):
            mode <<= 6
            with self.subTest(mode=format(mode, '03o')):
                d = self.do_create(recurse=3, dirs=2, files=2)
                with d:
                    # Change files and directories mode recursively.
                    for root, dirs, files in os.walk(d.name, topdown=False):
                        for name in files:
                            os.chmod(os.path.join(root, name), mode)
                        os.chmod(root, mode)
                    d.cleanup()
                self.assertFalse(os.path.exists(d.name))

    def check_flags(self, flags):
        # skip the test if these flags are not supported (ex: FreeBSD 13)
        filename = os_helper.TESTFN
        try:
            open(filename, "w").close()
            try:
                os.chflags(filename, flags)
            except OSError as exc:
                # "OSError: [Errno 45] Operation not supported"
                self.skipTest(f"chflags() doesn't support flags "
                              f"{flags:#b}: {exc}")
            else:
                os.chflags(filename, 0)
        finally:
            os_helper.unlink(filename)

    @unittest.skipUnless(hasattr(os, 'chflags'), 'requires os.chflags')
    def test_flags(self):
        flags = stat.UF_IMMUTABLE | stat.UF_NOUNLINK
        self.check_flags(flags)

        d = self.do_create(recurse=3, dirs=2, files=2)
        with d:
            # Change files and directories flags recursively.
            for root, dirs, files in os.walk(d.name, topdown=False):
                for name in files:
                    os.chflags(os.path.join(root, name), flags)
                os.chflags(root, flags)
            d.cleanup()
        self.assertFalse(os.path.exists(d.name))

    def test_delete_false(self):
        with tempfile.TemporaryDirectory(delete=False) as working_dir:
            pass
        self.assertTrue(os.path.exists(working_dir))
        shutil.rmtree(working_dir)

if __name__ == "__main__":
    unittest.main()
