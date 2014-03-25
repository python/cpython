# tempfile.py unit tests.
import tempfile
import errno
import io
import os
import signal
import sys
import re
import warnings
import contextlib
import weakref
from unittest import mock

import unittest
from test import support, script_helper


if hasattr(os, 'stat'):
    import stat
    has_stat = 1
else:
    has_stat = 0

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

# Common functionality.
class BaseTestCase(unittest.TestCase):

    str_check = re.compile(r"^[a-z0-9_-]{8}$")

    def setUp(self):
        self._warnings_manager = support.check_warnings()
        self._warnings_manager.__enter__()
        warnings.filterwarnings("ignore", category=RuntimeWarning,
                                message="mktemp", module=__name__)

    def tearDown(self):
        self._warnings_manager.__exit__(None, None, None)


    def nameCheck(self, name, dir, pre, suf):
        (ndir, nbase) = os.path.split(name)
        npre  = nbase[:len(pre)]
        nsuf  = nbase[len(nbase)-len(suf):]

        # check for equality of the absolute paths!
        self.assertEqual(os.path.abspath(ndir), os.path.abspath(dir),
                         "file '%s' not in directory '%s'" % (name, dir))
        self.assertEqual(npre, pre,
                         "file '%s' does not begin with '%s'" % (nbase, pre))
        self.assertEqual(nsuf, suf,
                         "file '%s' does not end with '%s'" % (nbase, suf))

        nbase = nbase[len(pre):len(nbase)-len(suf)]
        self.assertTrue(self.str_check.match(nbase),
                     "random string '%s' does not match ^[a-z0-9_-]{8}$"
                     % nbase)


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
            "gettempdir" : 1,
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

    def test_get_six_char_str(self):
        # _RandomNameSequence returns a six-character string
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

    @unittest.skipUnless(hasattr(os, 'fork'),
        "os.fork is required for this test")
    def test_process_awareness(self):
        # ensure that the random source differs between
        # child and parent.
        read_fd, write_fd = os.pipe()
        pid = None
        try:
            pid = os.fork()
            if not pid:
                os.close(read_fd)
                os.write(write_fd, next(self.r).encode("ascii"))
                os.close(write_fd)
                # bypass the normal exit handlers- leave those to
                # the parent.
                os._exit(0)
            parent_value = next(self.r)
            child_value = os.read(read_fd, len(parent_value)).decode("ascii")
        finally:
            if pid:
                # best effort to ensure the process can't bleed out
                # via any bugs above
                try:
                    os.kill(pid, signal.SIGKILL)
                except OSError:
                    pass
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
        with support.EnvironmentVarGuard() as env:
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

                with support.swap_attr(io, "open", raise_OSError):
                    # test again with failing io.open()
                    with self.assertRaises(FileNotFoundError):
                        tempfile._get_default_tempdir()
                    self.assertEqual(os.listdir(our_temp_directory), [])

                open = io.open
                def bad_writer(*args, **kwargs):
                    fp = open(*args, **kwargs)
                    fp.write = raise_OSError
                    return fp

                with support.swap_attr(io, "open", bad_writer):
                    # test again with failing write()
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
        support.rmtree(dir)


def _mock_candidate_names(*names):
    return support.swap_attr(tempfile,
                             '_get_candidate_names',
                             lambda: iter(names))


class TestMkstempInner(BaseTestCase):
    """Test the internal function _mkstemp_inner."""

    class mkstemped:
        _bflags = tempfile._bin_openflags
        _tflags = tempfile._text_openflags
        _close = os.close
        _unlink = os.unlink

        def __init__(self, dir, pre, suf, bin):
            if bin: flags = self._bflags
            else:   flags = self._tflags

            (self.fd, self.name) = tempfile._mkstemp_inner(dir, pre, suf, flags)

        def write(self, str):
            os.write(self.fd, str)

        def __del__(self):
            self._close(self.fd)
            self._unlink(self.name)

    def do_create(self, dir=None, pre="", suf="", bin=1):
        if dir is None:
            dir = tempfile.gettempdir()
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
        finally:
            os.rmdir(dir)

    @unittest.skipUnless(has_stat, 'os.stat not available')
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

    def default_mkstemp_inner(self):
        return tempfile._mkstemp_inner(tempfile.gettempdir(),
                                       tempfile.template,
                                       '',
                                       tempfile._bin_openflags)

    def test_collision_with_existing_file(self):
        # _mkstemp_inner tries another name when a file with
        # the chosen name already exists
        with _inside_empty_temp_dir(), \
             _mock_candidate_names('aaa', 'aaa', 'bbb'):
            (fd1, name1) = self.default_mkstemp_inner()
            os.close(fd1)
            self.assertTrue(name1.endswith('aaa'))

            (fd2, name2) = self.default_mkstemp_inner()
            os.close(fd2)
            self.assertTrue(name2.endswith('bbb'))

    def test_collision_with_existing_directory(self):
        # _mkstemp_inner tries another name when a directory with
        # the chosen name already exists
        with _inside_empty_temp_dir(), \
             _mock_candidate_names('aaa', 'aaa', 'bbb'):
            dir = tempfile.mkdtemp()
            self.assertTrue(dir.endswith('aaa'))

            (fd, name) = self.default_mkstemp_inner()
            os.close(fd)
            self.assertTrue(name.endswith('bbb'))


class TestGetTempPrefix(BaseTestCase):
    """Test gettempprefix()."""

    def test_sane_template(self):
        # gettempprefix returns a nonempty prefix string
        p = tempfile.gettempprefix()

        self.assertIsInstance(p, str)
        self.assertTrue(len(p) > 0)

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

        dir = tempfile.gettempdir()
        self.assertTrue(os.path.isabs(dir) or dir == os.curdir,
                     "%s is not an absolute path" % dir)
        self.assertTrue(os.path.isdir(dir),
                     "%s is not a directory" % dir)

    def test_directory_writable(self):
        # gettempdir returns a directory writable by the user

        # sneaky: just instantiate a NamedTemporaryFile, which
        # defaults to writing into the directory returned by
        # gettempdir.
        file = tempfile.NamedTemporaryFile()
        file.write(b"blat")
        file.close()

    def test_same_thing(self):
        # gettempdir always returns the same object
        a = tempfile.gettempdir()
        b = tempfile.gettempdir()

        self.assertTrue(a is b)

    def test_case_sensitive(self):
        # gettempdir should not flatten its case
        # even on a case-insensitive file system
        case_sensitive_tempdir = tempfile.mkdtemp("-Temp")
        _tempdir, tempfile.tempdir = tempfile.tempdir, None
        try:
            with support.EnvironmentVarGuard() as env:
                # Fake the first env var which is checked as a candidate
                env["TMPDIR"] = case_sensitive_tempdir
                self.assertEqual(tempfile.gettempdir(), case_sensitive_tempdir)
        finally:
            tempfile.tempdir = _tempdir
            support.rmdir(case_sensitive_tempdir)


class TestMkstemp(BaseTestCase):
    """Test mkstemp()."""

    def do_create(self, dir=None, pre="", suf=""):
        if dir is None:
            dir = tempfile.gettempdir()
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

    def test_choose_directory(self):
        # mkstemp can create directories in a user-selected directory
        dir = tempfile.mkdtemp()
        try:
            self.do_create(dir=dir)
        finally:
            os.rmdir(dir)


class TestMkdtemp(BaseTestCase):
    """Test mkdtemp()."""

    def do_create(self, dir=None, pre="", suf=""):
        if dir is None:
            dir = tempfile.gettempdir()
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
        finally:
            os.rmdir(dir)

    @unittest.skipUnless(has_stat, 'os.stat not available')
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

    def test_creates_named(self):
        # NamedTemporaryFile creates files with names
        f = tempfile.NamedTemporaryFile()
        self.assertTrue(os.path.exists(f.name),
                        "NamedTemporaryFile %s does not exist" % f.name)

    def test_del_on_close(self):
        # A NamedTemporaryFile is deleted when closed
        dir = tempfile.mkdtemp()
        try:
            f = tempfile.NamedTemporaryFile(dir=dir)
            f.write(b'blat')
            f.close()
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

    def test_no_leak_fd(self):
        # Issue #21058: don't leak file descriptor when io.open() fails
        closed = []
        def close(fd):
            closed.append(fd)

        with mock.patch('os.close', side_effect=close):
            with mock.patch('io.open', side_effect=ValueError):
                self.assertRaises(ValueError, tempfile.NamedTemporaryFile)
                self.assertEqual(len(closed), 1)

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
            self.assertFalse(isinstance(filename, str) and os.path.exists(filename),
                        "SpooledTemporaryFile %s exists after close" % filename)
        finally:
            os.rmdir(dir)

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
        f.seek(0)
        buf = f.read()
        self.assertEqual(buf, b'xyz')

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
        f.seek(100, 0)
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

        f.write(b'x')
        self.assertTrue(f._rolled)
        self.assertEqual(f.mode, 'rb+')
        self.assertIsNotNone(f.name)
        with self.assertRaises(AttributeError):
            f.newlines
        with self.assertRaises(AttributeError):
            f.encoding

    def test_text_mode(self):
        # Creating a SpooledTemporaryFile with a text mode should produce
        # a file object reading and writing (Unicode) text strings.
        f = tempfile.SpooledTemporaryFile(mode='w+', max_size=10)
        f.write("abc\n")
        f.seek(0)
        self.assertEqual(f.read(), "abc\n")
        f.write("def\n")
        f.seek(0)
        self.assertEqual(f.read(), "abc\ndef\n")
        self.assertFalse(f._rolled)
        self.assertEqual(f.mode, 'w+')
        self.assertIsNone(f.name)
        self.assertIsNone(f.newlines)
        self.assertIsNone(f.encoding)

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
        self.assertIsNotNone(f.encoding)

    def test_text_newline_and_encoding(self):
        f = tempfile.SpooledTemporaryFile(mode='w+', max_size=10,
                                          newline='', encoding='utf-8')
        f.write("\u039B\r\n")
        f.seek(0)
        self.assertEqual(f.read(), "\u039B\r\n")
        self.assertFalse(f._rolled)
        self.assertEqual(f.mode, 'w+')
        self.assertIsNone(f.name)
        self.assertIsNone(f.newlines)
        self.assertIsNone(f.encoding)

        f.write("\u039B" * 20 + "\r\n")
        f.seek(0)
        self.assertEqual(f.read(), "\u039B\r\n" + ("\u039B" * 20) + "\r\n")
        self.assertTrue(f._rolled)
        self.assertEqual(f.mode, 'w+')
        self.assertIsNotNone(f.name)
        self.assertIsNotNone(f.newlines)
        self.assertEqual(f.encoding, 'utf-8')

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
        if has_stat:
            self.assertEqual(os.fstat(f.fileno()).st_size, 20)


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

        def test_no_leak_fd(self):
            # Issue #21058: don't leak file descriptor when io.open() fails
            closed = []
            def close(fd):
                closed.append(fd)

            with mock.patch('os.close', side_effect=close):
                with mock.patch('io.open', side_effect=ValueError):
                    self.assertRaises(ValueError, tempfile.TemporaryFile)
                    self.assertEqual(len(closed), 1)



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

    def do_create(self, dir=None, pre="", suf="", recurse=1):
        if dir is None:
            dir = tempfile.gettempdir()
        tmp = tempfile.TemporaryDirectory(dir=dir, prefix=pre, suffix=suf)
        self.nameCheck(tmp.name, dir, pre, suf)
        # Create a subdirectory and some files
        if recurse:
            d1 = self.do_create(tmp.name, pre, suf, recurse-1)
            d1.name = None
        with open(os.path.join(tmp.name, "test.txt"), "wb") as f:
            f.write(b"Hello world!")
        return tmp

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

    @support.skip_unless_symlink
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
        self.assertEqual(os.listdir(d2.name), ['test.txt'],
                         "Contents of the directory pointed to by a symlink "
                         "were deleted")
        d2.cleanup()

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
                    with open(os.path.join(tmp2, "test.txt"), "w") as f:
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

    def test_warnings_on_cleanup(self):
        # ResourceWarning will be triggered by __del__
        with self.do_create() as dir:
            d = self.do_create(dir=dir, recurse=3)
            name = d.name

            # Check for the resource warning
            with support.check_warnings(('Implicitly', ResourceWarning), quiet=False):
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


def test_main():
    support.run_unittest(__name__)

if __name__ == "__main__":
    test_main()
