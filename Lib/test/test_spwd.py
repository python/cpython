import os
import unittest
from test import test_support

spwd = test_support.import_module('spwd')


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
        random_name = entries[0].sp_nam
        entry = spwd.getspnam(random_name)
        self.assertIsInstance(entry, spwd.struct_spwd)
        self.assertEqual(entry.sp_nam, random_name)
        self.assertEqual(entry.sp_nam, entry[0])
        self.assertIsInstance(entry.sp_pwd, str)
        self.assertEqual(entry.sp_pwd, entry[1])
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
        if test_support.have_unicode:
            try:
                unicode_name = unicode(random_name)
            except UnicodeDecodeError:
                pass
            else:
                self.assertEqual(spwd.getspnam(unicode_name), entry)


def test_main():
    test_support.run_unittest(TestSpwdRoot)

if __name__ == "__main__":
    test_main()
