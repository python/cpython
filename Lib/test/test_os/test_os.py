# As a test suite for the os module, this is woefully inadequate, but this
# does add tests for a few functions which have been determined to be more
# portable than they had been thought to be.

import os
import unittest
from test import support
from test.support import os_helper
from test.support.os_helper import FakePath


@unittest.skipIf(support.is_wasi, "WASI has no /dev/null")
class DevNullTests(unittest.TestCase):
    def test_devnull(self):
        with open(os.devnull, 'wb', 0) as f:
            f.write(b'hello')
            f.close()
        with open(os.devnull, 'rb') as f:
            self.assertEqual(f.read(), b'')


class OSErrorTests(unittest.TestCase):
    def setUp(self):
        class Str(str):
            pass

        self.bytes_filenames = []
        self.unicode_filenames = []
        if os_helper.TESTFN_UNENCODABLE is not None:
            decoded = os_helper.TESTFN_UNENCODABLE
        else:
            decoded = os_helper.TESTFN
        self.unicode_filenames.append(decoded)
        self.unicode_filenames.append(Str(decoded))
        if os_helper.TESTFN_UNDECODABLE is not None:
            encoded = os_helper.TESTFN_UNDECODABLE
        else:
            encoded = os.fsencode(os_helper.TESTFN)
        self.bytes_filenames.append(encoded)

        self.filenames = self.bytes_filenames + self.unicode_filenames

    def test_oserror_filename(self):
        funcs = [
            (self.filenames, os.chdir,),
            (self.filenames, os.lstat,),
            (self.filenames, os.open, os.O_RDONLY),
            (self.filenames, os.rmdir,),
            (self.filenames, os.stat,),
            (self.filenames, os.unlink,),
            (self.filenames, os.listdir,),
            (self.filenames, os.rename, "dst"),
            (self.filenames, os.replace, "dst"),
        ]
        if os_helper.can_chmod():
            funcs.append((self.filenames, os.chmod, 0o777))
        if hasattr(os, "chown"):
            funcs.append((self.filenames, os.chown, 0, 0))
        if hasattr(os, "lchown"):
            funcs.append((self.filenames, os.lchown, 0, 0))
        if hasattr(os, "truncate"):
            funcs.append((self.filenames, os.truncate, 0))
        if hasattr(os, "chflags"):
            funcs.append((self.filenames, os.chflags, 0))
        if hasattr(os, "lchflags"):
            funcs.append((self.filenames, os.lchflags, 0))
        if hasattr(os, "chroot"):
            funcs.append((self.filenames, os.chroot,))
        if hasattr(os, "link"):
            funcs.append((self.filenames, os.link, "dst"))
        if hasattr(os, "listxattr"):
            funcs.extend((
                (self.filenames, os.listxattr,),
                (self.filenames, os.getxattr, "user.test"),
                (self.filenames, os.setxattr, "user.test", b'user'),
                (self.filenames, os.removexattr, "user.test"),
            ))
        if hasattr(os, "lchmod"):
            funcs.append((self.filenames, os.lchmod, 0o777))
        if hasattr(os, "readlink"):
            funcs.append((self.filenames, os.readlink,))

        for filenames, func, *func_args in funcs:
            for name in filenames:
                try:
                    func(name, *func_args)
                except OSError as err:
                    self.assertIs(err.filename, name, str(func))
                except UnicodeDecodeError:
                    pass
                else:
                    self.fail(f"No exception thrown by {func}")


class PathTConverterTests(unittest.TestCase):
    # tuples of (function name, allows fd arguments, additional arguments to
    # function, cleanup function)
    functions = [
        ('stat', True, (), None),
        ('lstat', False, (), None),
        ('access', False, (os.F_OK,), None),
        ('chflags', False, (0,), None),
        ('lchflags', False, (0,), None),
        ('open', False, (os.O_RDONLY,), getattr(os, 'close', None)),
    ]

    def test_path_t_converter(self):
        str_filename = os_helper.TESTFN
        if os.name == 'nt':
            bytes_fspath = bytes_filename = None
        else:
            bytes_filename = os.fsencode(os_helper.TESTFN)
            bytes_fspath = FakePath(bytes_filename)
        fd = os.open(FakePath(str_filename), os.O_WRONLY|os.O_CREAT)
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        self.addCleanup(os.close, fd)

        int_fspath = FakePath(fd)
        str_fspath = FakePath(str_filename)

        for name, allow_fd, extra_args, cleanup_fn in self.functions:
            with self.subTest(name=name):
                try:
                    fn = getattr(os, name)
                except AttributeError:
                    continue

                for path in (str_filename, bytes_filename, str_fspath,
                             bytes_fspath):
                    if path is None:
                        continue
                    with self.subTest(name=name, path=path):
                        result = fn(path, *extra_args)
                        if cleanup_fn is not None:
                            cleanup_fn(result)

                with self.assertRaisesRegex(
                        TypeError, 'to return str or bytes'):
                    fn(int_fspath, *extra_args)

                if allow_fd:
                    result = fn(fd, *extra_args)  # should not fail
                    if cleanup_fn is not None:
                        cleanup_fn(result)
                else:
                    with self.assertRaisesRegex(
                            TypeError,
                            'os.PathLike'):
                        fn(fd, *extra_args)

    def test_path_t_converter_and_custom_class(self):
        msg = r'__fspath__\(\) to return str or bytes, not %s'
        with self.assertRaisesRegex(TypeError, msg % r'int'):
            os.stat(FakePath(2))
        with self.assertRaisesRegex(TypeError, msg % r'float'):
            os.stat(FakePath(2.34))
        with self.assertRaisesRegex(TypeError, msg % r'object'):
            os.stat(FakePath(object()))


class ExportsTests(unittest.TestCase):
    def test_os_all(self):
        self.assertIn('open', os.__all__)
        self.assertIn('walk', os.__all__)


if __name__ == "__main__":
    unittest.main()
