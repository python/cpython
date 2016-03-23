import os
import unittest
from test import support

spwd = support.import_module('spwd')


@unittest.skipUnless(hasattr(os, 'geteuid') and os.geteuid() == 0,
                     'root privileges required')
class TestSpwdRoot(unittest.TestCase):

    def test_getspall(self):
        entries = spwd.getspall()
        self.assertIsInstance(entries, list)
        for entry in entries:
            self.assertIsInstance(entry, spwd.struct_spwd)

    def test_getspnam(self):
        entries = spwd.getspall()
        if not entries:
            self.skipTest('empty shadow password database')
        random_name = entries[0].sp_namp
        entry = spwd.getspnam(random_name)
        self.assertIsInstance(entry, spwd.struct_spwd)
        self.assertEqual(entry.sp_namp, random_name)
        self.assertEqual(entry.sp_namp, entry[0])
        self.assertEqual(entry.sp_namp, entry.sp_nam)
        self.assertIsInstance(entry.sp_pwdp, str)
        self.assertEqual(entry.sp_pwdp, entry[1])
        self.assertEqual(entry.sp_pwdp, entry.sp_pwd)
        self.assertIsInstance(entry.sp_lstchg, int)
        self.assertEqual(entry.sp_lstchg, entry[2])
        self.assertIsInstance(entry.sp_min, int)
        self.assertEqual(entry.sp_min, entry[3])
        self.assertIsInstance(entry.sp_max, int)
        self.assertEqual(entry.sp_max, entry[4])
        self.assertIsInstance(entry.sp_warn, int)
        self.assertEqual(entry.sp_warn, entry[5])
        self.assertIsInstance(entry.sp_inact, int)
        self.assertEqual(entry.sp_inact, entry[6])
        self.assertIsInstance(entry.sp_expire, int)
        self.assertEqual(entry.sp_expire, entry[7])
        self.assertIsInstance(entry.sp_flag, int)
        self.assertEqual(entry.sp_flag, entry[8])
        with self.assertRaises(KeyError) as cx:
            spwd.getspnam('invalid user name')
        self.assertEqual(str(cx.exception), "'getspnam(): name not found'")
        self.assertRaises(TypeError, spwd.getspnam)
        self.assertRaises(TypeError, spwd.getspnam, 0)
        self.assertRaises(TypeError, spwd.getspnam, random_name, 0)
        try:
            bytes_name = os.fsencode(random_name)
        except UnicodeEncodeError:
            pass
        else:
            self.assertRaises(TypeError, spwd.getspnam, bytes_name)


@unittest.skipUnless(hasattr(os, 'geteuid') and os.geteuid() != 0,
                     'non-root user required')
class TestSpwdNonRoot(unittest.TestCase):

    def test_getspnam_exception(self):
        name = 'bin'
        try:
            with self.assertRaises(PermissionError) as cm:
                spwd.getspnam(name)
        except KeyError as exc:
            self.skipTest("spwd entry %r doesn't exist: %s" % (name, exc))
        else:
            self.assertEqual(str(cm.exception), '[Errno 13] Permission denied')


if __name__ == "__main__":
    unittest.main()
