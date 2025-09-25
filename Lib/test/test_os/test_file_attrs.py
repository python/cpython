"""
Test file attributes: chmod(), chown(), access(), setxattr(), chflags(), etc.
"""

import decimal
import errno
import fractions
import os
import platform
import posix
import stat
import sys
import tempfile
import unittest
from test import support
from test.support import os_helper
from .utils import create_file

try:
    import pwd
    all_users = [u.pw_uid for u in pwd.getpwall()]
except (ImportError, AttributeError):
    all_users = []


_DUMMY_SYMLINK = os.path.join(tempfile.gettempdir(),
                              os_helper.TESTFN + '-dummy-symlink')

root_in_posix = False
if hasattr(os, 'geteuid'):
    root_in_posix = (os.geteuid() == 0)


@unittest.skipUnless(hasattr(os, "chown"), "requires os.chown()")
class ChownFileTests(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        os.mkdir(os_helper.TESTFN)

    def test_chown_uid_gid_arguments_must_be_index(self):
        stat = os.stat(os_helper.TESTFN)
        uid = stat.st_uid
        gid = stat.st_gid
        for value in (-1.0, -1j, decimal.Decimal(-1), fractions.Fraction(-2, 2)):
            self.assertRaises(TypeError, os.chown, os_helper.TESTFN, value, gid)
            self.assertRaises(TypeError, os.chown, os_helper.TESTFN, uid, value)
        self.assertIsNone(os.chown(os_helper.TESTFN, uid, gid))
        self.assertIsNone(os.chown(os_helper.TESTFN, -1, -1))

    @unittest.skipUnless(hasattr(os, 'getgroups'), 'need os.getgroups')
    def test_chown_gid(self):
        groups = os.getgroups()
        if len(groups) < 2:
            self.skipTest("test needs at least 2 groups")

        gid_1, gid_2 = groups[:2]
        uid = os.stat(os_helper.TESTFN).st_uid

        os.chown(os_helper.TESTFN, uid, gid_1)
        gid = os.stat(os_helper.TESTFN).st_gid
        self.assertEqual(gid, gid_1)

        os.chown(os_helper.TESTFN, uid, gid_2)
        gid = os.stat(os_helper.TESTFN).st_gid
        self.assertEqual(gid, gid_2)

    @unittest.skipUnless(root_in_posix and len(all_users) > 1,
                         "test needs root privilege and more than one user")
    def test_chown_with_root(self):
        uid_1, uid_2 = all_users[:2]
        gid = os.stat(os_helper.TESTFN).st_gid
        os.chown(os_helper.TESTFN, uid_1, gid)
        uid = os.stat(os_helper.TESTFN).st_uid
        self.assertEqual(uid, uid_1)
        os.chown(os_helper.TESTFN, uid_2, gid)
        uid = os.stat(os_helper.TESTFN).st_uid
        self.assertEqual(uid, uid_2)

    @unittest.skipUnless(not root_in_posix and len(all_users) > 1,
                         "test needs non-root account and more than one user")
    def test_chown_without_permission(self):
        uid_1, uid_2 = all_users[:2]
        gid = os.stat(os_helper.TESTFN).st_gid
        with self.assertRaises(PermissionError):
            os.chown(os_helper.TESTFN, uid_1, gid)
            os.chown(os_helper.TESTFN, uid_2, gid)

    @classmethod
    def tearDownClass(cls):
        os.rmdir(os_helper.TESTFN)


class PosixTester(unittest.TestCase):

    def setUp(self):
        # create empty file
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        create_file(os_helper.TESTFN, b'')

    def _test_all_chown_common(self, chown_func, first_param, stat_func):
        """Common code for chown, fchown and lchown tests."""
        def check_stat(uid, gid):
            if stat_func is not None:
                stat = stat_func(first_param)
                self.assertEqual(stat.st_uid, uid)
                self.assertEqual(stat.st_gid, gid)
        uid = os.getuid()
        gid = os.getgid()
        # test a successful chown call
        chown_func(first_param, uid, gid)
        check_stat(uid, gid)
        chown_func(first_param, -1, gid)
        check_stat(uid, gid)
        chown_func(first_param, uid, -1)
        check_stat(uid, gid)

        if sys.platform == "vxworks":
            # On VxWorks, root user id is 1 and 0 means no login user:
            # both are super users.
            is_root = (uid in (0, 1))
        else:
            is_root = (uid == 0)
        if support.is_emscripten:
            # Emscripten getuid() / geteuid() always return 0 (root), but
            # cannot chown uid/gid to random value.
            pass
        elif is_root:
            # Try an amusingly large uid/gid to make sure we handle
            # large unsigned values.  (chown lets you use any
            # uid/gid you like, even if they aren't defined.)
            #
            # On VxWorks uid_t is defined as unsigned short. A big
            # value greater than 65535 will result in underflow error.
            #
            # This problem keeps coming up:
            #   http://bugs.python.org/issue1747858
            #   http://bugs.python.org/issue4591
            #   http://bugs.python.org/issue15301
            # Hopefully the fix in 4591 fixes it for good!
            #
            # This part of the test only runs when run as root.
            # Only scary people run their tests as root.

            big_value = (2**31 if sys.platform != "vxworks" else 2**15)
            chown_func(first_param, big_value, big_value)
            check_stat(big_value, big_value)
            chown_func(first_param, -1, -1)
            check_stat(big_value, big_value)
            chown_func(first_param, uid, gid)
            check_stat(uid, gid)
        elif platform.system() in ('HP-UX', 'SunOS'):
            # HP-UX and Solaris can allow a non-root user to chown() to root
            # (issue #5113)
            raise unittest.SkipTest("Skipping because of non-standard chown() "
                                    "behavior")
        else:
            # non-root cannot chown to root, raises OSError
            self.assertRaises(OSError, chown_func, first_param, 0, 0)
            check_stat(uid, gid)
            self.assertRaises(OSError, chown_func, first_param, 0, -1)
            check_stat(uid, gid)
            if hasattr(os, 'getgroups'):
                if 0 not in os.getgroups():
                    self.assertRaises(OSError, chown_func, first_param, -1, 0)
                    check_stat(uid, gid)
        # test illegal types
        for t in str, float:
            self.assertRaises(TypeError, chown_func, first_param, t(uid), gid)
            check_stat(uid, gid)
            self.assertRaises(TypeError, chown_func, first_param, uid, t(gid))
            check_stat(uid, gid)

    @unittest.skipUnless(hasattr(os, "chown"), "requires os.chown()")
    @unittest.skipIf(support.is_emscripten, "getgid() is a stub")
    def test_chown(self):
        # raise an OSError if the file does not exist
        os.unlink(os_helper.TESTFN)
        self.assertRaises(OSError, posix.chown, os_helper.TESTFN, -1, -1)

        # re-create the file
        os_helper.create_empty_file(os_helper.TESTFN)
        self._test_all_chown_common(posix.chown, os_helper.TESTFN, posix.stat)

    @os_helper.skip_unless_working_chmod
    @unittest.skipUnless(hasattr(posix, 'fchown'), "test needs os.fchown()")
    @unittest.skipIf(support.is_emscripten, "getgid() is a stub")
    def test_fchown(self):
        os.unlink(os_helper.TESTFN)

        # re-create the file
        test_file = open(os_helper.TESTFN, 'w')
        try:
            fd = test_file.fileno()
            self._test_all_chown_common(posix.fchown, fd,
                                        getattr(posix, 'fstat', None))
        finally:
            test_file.close()

    @os_helper.skip_unless_working_chmod
    @unittest.skipUnless(hasattr(posix, 'lchown'), "test needs os.lchown()")
    def test_lchown(self):
        os.unlink(os_helper.TESTFN)
        # create a symlink
        os.symlink(_DUMMY_SYMLINK, os_helper.TESTFN)
        self._test_all_chown_common(posix.lchown, os_helper.TESTFN,
                                    getattr(posix, 'lstat', None))

    @unittest.skipUnless(hasattr(posix, 'access'), 'test needs posix.access()')
    def test_access(self):
        self.assertTrue(posix.access(os_helper.TESTFN, os.R_OK))

    @unittest.skipUnless(hasattr(posix, 'umask'), 'test needs posix.umask()')
    def test_umask(self):
        old_mask = posix.umask(0)
        self.assertIsInstance(old_mask, int)
        posix.umask(old_mask)

    def check_chmod(self, chmod_func, target, **kwargs):
        closefd = not isinstance(target, int)
        mode = os.stat(target).st_mode
        try:
            new_mode = mode & ~(stat.S_IWOTH | stat.S_IWGRP | stat.S_IWUSR)
            chmod_func(target, new_mode, **kwargs)
            self.assertEqual(os.stat(target).st_mode, new_mode)
            if stat.S_ISREG(mode):
                try:
                    with open(target, 'wb+', closefd=closefd):
                        pass
                except PermissionError:
                    pass
            new_mode = mode | (stat.S_IWOTH | stat.S_IWGRP | stat.S_IWUSR)
            chmod_func(target, new_mode, **kwargs)
            self.assertEqual(os.stat(target).st_mode, new_mode)
            if stat.S_ISREG(mode):
                with open(target, 'wb+', closefd=closefd):
                    pass
        finally:
            chmod_func(target, mode)

    @os_helper.skip_unless_working_chmod
    def test_chmod_file(self):
        self.check_chmod(posix.chmod, os_helper.TESTFN)

    def tempdir(self):
        target = os_helper.TESTFN + 'd'
        posix.mkdir(target)
        self.addCleanup(posix.rmdir, target)
        return target

    @os_helper.skip_unless_working_chmod
    def test_chmod_dir(self):
        target = self.tempdir()
        self.check_chmod(posix.chmod, target)

    @os_helper.skip_unless_working_chmod
    def test_fchmod_file(self):
        with open(os_helper.TESTFN, 'wb+') as f:
            self.check_chmod(posix.fchmod, f.fileno())
            self.check_chmod(posix.chmod, f.fileno())

    @unittest.skipUnless(hasattr(posix, 'lchmod'), 'test needs os.lchmod()')
    def test_lchmod_file(self):
        self.check_chmod(posix.lchmod, os_helper.TESTFN)
        self.check_chmod(posix.chmod, os_helper.TESTFN, follow_symlinks=False)

    @unittest.skipUnless(hasattr(posix, 'lchmod'), 'test needs os.lchmod()')
    def test_lchmod_dir(self):
        target = self.tempdir()
        self.check_chmod(posix.lchmod, target)
        self.check_chmod(posix.chmod, target, follow_symlinks=False)

    def check_chmod_link(self, chmod_func, target, link, **kwargs):
        target_mode = os.stat(target).st_mode
        link_mode = os.lstat(link).st_mode
        try:
            new_mode = target_mode & ~(stat.S_IWOTH | stat.S_IWGRP | stat.S_IWUSR)
            chmod_func(link, new_mode, **kwargs)
            self.assertEqual(os.stat(target).st_mode, new_mode)
            self.assertEqual(os.lstat(link).st_mode, link_mode)
            new_mode = target_mode | (stat.S_IWOTH | stat.S_IWGRP | stat.S_IWUSR)
            chmod_func(link, new_mode, **kwargs)
            self.assertEqual(os.stat(target).st_mode, new_mode)
            self.assertEqual(os.lstat(link).st_mode, link_mode)
        finally:
            posix.chmod(target, target_mode)

    def check_lchmod_link(self, chmod_func, target, link, **kwargs):
        target_mode = os.stat(target).st_mode
        link_mode = os.lstat(link).st_mode
        new_mode = link_mode & ~(stat.S_IWOTH | stat.S_IWGRP | stat.S_IWUSR)
        chmod_func(link, new_mode, **kwargs)
        self.assertEqual(os.stat(target).st_mode, target_mode)
        self.assertEqual(os.lstat(link).st_mode, new_mode)
        new_mode = link_mode | (stat.S_IWOTH | stat.S_IWGRP | stat.S_IWUSR)
        chmod_func(link, new_mode, **kwargs)
        self.assertEqual(os.stat(target).st_mode, target_mode)
        self.assertEqual(os.lstat(link).st_mode, new_mode)

    @os_helper.skip_unless_symlink
    def test_chmod_file_symlink(self):
        target = os_helper.TESTFN
        link = os_helper.TESTFN + '-link'
        os.symlink(target, link)
        self.addCleanup(posix.unlink, link)
        if os.name == 'nt':
            self.check_lchmod_link(posix.chmod, target, link)
        else:
            self.check_chmod_link(posix.chmod, target, link)
        self.check_chmod_link(posix.chmod, target, link, follow_symlinks=True)

    @os_helper.skip_unless_symlink
    def test_chmod_dir_symlink(self):
        target = self.tempdir()
        link = os_helper.TESTFN + '-link'
        os.symlink(target, link, target_is_directory=True)
        self.addCleanup(posix.unlink, link)
        if os.name == 'nt':
            self.check_lchmod_link(posix.chmod, target, link)
        else:
            self.check_chmod_link(posix.chmod, target, link)
        self.check_chmod_link(posix.chmod, target, link, follow_symlinks=True)

    @unittest.skipUnless(hasattr(posix, 'lchmod'), 'test needs os.lchmod()')
    @os_helper.skip_unless_symlink
    def test_lchmod_file_symlink(self):
        target = os_helper.TESTFN
        link = os_helper.TESTFN + '-link'
        os.symlink(target, link)
        self.addCleanup(posix.unlink, link)
        self.check_lchmod_link(posix.chmod, target, link, follow_symlinks=False)
        self.check_lchmod_link(posix.lchmod, target, link)

    @unittest.skipUnless(hasattr(posix, 'lchmod'), 'test needs os.lchmod()')
    @os_helper.skip_unless_symlink
    def test_lchmod_dir_symlink(self):
        target = self.tempdir()
        link = os_helper.TESTFN + '-link'
        os.symlink(target, link)
        self.addCleanup(posix.unlink, link)
        self.check_lchmod_link(posix.chmod, target, link, follow_symlinks=False)
        self.check_lchmod_link(posix.lchmod, target, link)

    def _test_chflags_regular_file(self, chflags_func, target_file, **kwargs):
        st = os.stat(target_file)
        self.assertHasAttr(st, 'st_flags')

        # ZFS returns EOPNOTSUPP when attempting to set flag UF_IMMUTABLE.
        flags = st.st_flags | stat.UF_IMMUTABLE
        try:
            chflags_func(target_file, flags, **kwargs)
        except OSError as err:
            if err.errno != errno.EOPNOTSUPP:
                raise
            msg = 'chflag UF_IMMUTABLE not supported by underlying fs'
            self.skipTest(msg)

        try:
            new_st = os.stat(target_file)
            self.assertEqual(st.st_flags | stat.UF_IMMUTABLE, new_st.st_flags)
            try:
                fd = open(target_file, 'w+')
            except OSError as e:
                self.assertEqual(e.errno, errno.EPERM)
        finally:
            posix.chflags(target_file, st.st_flags)

    @unittest.skipUnless(hasattr(posix, 'chflags'), 'test needs os.chflags()')
    def test_chflags(self):
        self._test_chflags_regular_file(posix.chflags, os_helper.TESTFN)

    @unittest.skipUnless(hasattr(posix, 'lchflags'), 'test needs os.lchflags()')
    def test_lchflags_regular_file(self):
        self._test_chflags_regular_file(posix.lchflags, os_helper.TESTFN)
        self._test_chflags_regular_file(posix.chflags, os_helper.TESTFN,
                                        follow_symlinks=False)

    @unittest.skipUnless(hasattr(posix, 'lchflags'), 'test needs os.lchflags()')
    def test_lchflags_symlink(self):
        testfn_st = os.stat(os_helper.TESTFN)

        self.assertHasAttr(testfn_st, 'st_flags')

        self.addCleanup(os_helper.unlink, _DUMMY_SYMLINK)
        os.symlink(os_helper.TESTFN, _DUMMY_SYMLINK)
        dummy_symlink_st = os.lstat(_DUMMY_SYMLINK)

        def chflags_nofollow(path, flags):
            return posix.chflags(path, flags, follow_symlinks=False)

        for fn in (posix.lchflags, chflags_nofollow):
            # ZFS returns EOPNOTSUPP when attempting to set flag UF_IMMUTABLE.
            flags = dummy_symlink_st.st_flags | stat.UF_IMMUTABLE
            try:
                fn(_DUMMY_SYMLINK, flags)
            except OSError as err:
                if err.errno != errno.EOPNOTSUPP:
                    raise
                msg = 'chflag UF_IMMUTABLE not supported by underlying fs'
                self.skipTest(msg)
            try:
                new_testfn_st = os.stat(os_helper.TESTFN)
                new_dummy_symlink_st = os.lstat(_DUMMY_SYMLINK)

                self.assertEqual(testfn_st.st_flags, new_testfn_st.st_flags)
                self.assertEqual(dummy_symlink_st.st_flags | stat.UF_IMMUTABLE,
                                 new_dummy_symlink_st.st_flags)
            finally:
                fn(_DUMMY_SYMLINK, dummy_symlink_st.st_flags)


def supports_extended_attributes():
    if not hasattr(os, "setxattr"):
        return False

    try:
        with open(os_helper.TESTFN, "xb", 0) as fp:
            try:
                os.setxattr(fp.fileno(), b"user.test", b"")
            except OSError:
                return False
    finally:
        os_helper.unlink(os_helper.TESTFN)

    return True


@unittest.skipUnless(supports_extended_attributes(),
                     "no non-broken extended attribute support")
# Kernels < 2.6.39 don't respect setxattr flags.
@support.requires_linux_version(2, 6, 39)
class ExtendedAttributeTests(unittest.TestCase):

    def _check_xattrs_str(self, s, getxattr, setxattr, removexattr, listxattr, **kwargs):
        fn = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, fn)
        create_file(fn)

        with self.assertRaises(OSError) as cm:
            getxattr(fn, s("user.test"), **kwargs)
        self.assertEqual(cm.exception.errno, errno.ENODATA)

        init_xattr = listxattr(fn)
        self.assertIsInstance(init_xattr, list)

        setxattr(fn, s("user.test"), b"", **kwargs)
        xattr = set(init_xattr)
        xattr.add("user.test")
        self.assertEqual(set(listxattr(fn)), xattr)
        self.assertEqual(getxattr(fn, b"user.test", **kwargs), b"")
        setxattr(fn, s("user.test"), b"hello", os.XATTR_REPLACE, **kwargs)
        self.assertEqual(getxattr(fn, b"user.test", **kwargs), b"hello")

        with self.assertRaises(OSError) as cm:
            setxattr(fn, s("user.test"), b"bye", os.XATTR_CREATE, **kwargs)
        self.assertEqual(cm.exception.errno, errno.EEXIST)

        with self.assertRaises(OSError) as cm:
            setxattr(fn, s("user.test2"), b"bye", os.XATTR_REPLACE, **kwargs)
        self.assertEqual(cm.exception.errno, errno.ENODATA)

        setxattr(fn, s("user.test2"), b"foo", os.XATTR_CREATE, **kwargs)
        xattr.add("user.test2")
        self.assertEqual(set(listxattr(fn)), xattr)
        removexattr(fn, s("user.test"), **kwargs)

        with self.assertRaises(OSError) as cm:
            getxattr(fn, s("user.test"), **kwargs)
        self.assertEqual(cm.exception.errno, errno.ENODATA)

        xattr.remove("user.test")
        self.assertEqual(set(listxattr(fn)), xattr)
        self.assertEqual(getxattr(fn, s("user.test2"), **kwargs), b"foo")
        setxattr(fn, s("user.test"), b"a"*256, **kwargs)
        self.assertEqual(getxattr(fn, s("user.test"), **kwargs), b"a"*256)
        removexattr(fn, s("user.test"), **kwargs)
        many = sorted("user.test{}".format(i) for i in range(32))
        for thing in many:
            setxattr(fn, thing, b"x", **kwargs)
        self.assertEqual(set(listxattr(fn)), set(init_xattr) | set(many))

    def _check_xattrs(self, *args, **kwargs):
        self._check_xattrs_str(str, *args, **kwargs)
        os_helper.unlink(os_helper.TESTFN)

        self._check_xattrs_str(os.fsencode, *args, **kwargs)
        os_helper.unlink(os_helper.TESTFN)

    def test_simple(self):
        self._check_xattrs(os.getxattr, os.setxattr, os.removexattr,
                           os.listxattr)

    def test_lpath(self):
        self._check_xattrs(os.getxattr, os.setxattr, os.removexattr,
                           os.listxattr, follow_symlinks=False)

    def test_fds(self):
        def getxattr(path, *args):
            with open(path, "rb") as fp:
                return os.getxattr(fp.fileno(), *args)
        def setxattr(path, *args):
            with open(path, "wb", 0) as fp:
                os.setxattr(fp.fileno(), *args)
        def removexattr(path, *args):
            with open(path, "wb", 0) as fp:
                os.removexattr(fp.fileno(), *args)
        def listxattr(path, *args):
            with open(path, "rb") as fp:
                return os.listxattr(fp.fileno(), *args)
        self._check_xattrs(getxattr, setxattr, removexattr, listxattr)


if __name__ == "__main__":
    unittest.main()
