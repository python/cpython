"""Test script for the grp module."""

import unittest
from test import support

grp = support.import_module('grp')

class GroupDatabaseTestCase(unittest.TestCase):

    def check_value(self, value):
        # check that a grp tuple has the entries and
        # attributes promised by the docs
        self.assertEqual(len(value), 4)
        self.assertEqual(value[0], value.gr_name)
        self.assertIsInstance(value.gr_name, str)
        self.assertEqual(value[1], value.gr_passwd)
        self.assertIsInstance(value.gr_passwd, str)
        self.assertEqual(value[2], value.gr_gid)
        self.assertIsInstance(value.gr_gid, int)
        self.assertEqual(value[3], value.gr_mem)
        self.assertIsInstance(value.gr_mem, list)

    def test_values(self):
        entries = grp.getgrall()

        for e in entries:
            self.check_value(e)

    def test_values_extended(self):
        entries = grp.getgrall()
        if len(entries) > 1000:  # Huge group file (NIS?) -- skip the rest
            self.skipTest('huge group file, extended test skipped')

        for e in entries:
            e2 = grp.getgrgid(e.gr_gid)
            self.check_value(e2)
            self.assertEqual(e2.gr_gid, e.gr_gid)
            name = e.gr_name
            if name.startswith('+') or name.startswith('-'):
                # NIS-related entry
                continue
            e2 = grp.getgrnam(name)
            self.check_value(e2)
            # There are instances where getgrall() returns group names in
            # lowercase while getgrgid() returns proper casing.
            # Discovered on Ubuntu 5.04 (custom).
            self.assertEqual(e2.gr_name.lower(), name.lower())

    def test_errors(self):
        self.assertRaises(TypeError, grp.getgrgid)
        self.assertRaises(TypeError, grp.getgrnam)
        self.assertRaises(TypeError, grp.getgrall, 42)
        # embedded null character
        self.assertRaises(ValueError, grp.getgrnam, 'a\x00b')

        # try to get some errors
        bynames = {}
        bygids = {}
        for (n, p, g, mem) in grp.getgrall():
            if not n or n == '+':
                continue # skip NIS entries etc.
            bynames[n] = g
            bygids[g] = n

        allnames = list(bynames.keys())
        namei = 0
        # Start with a name that very likely does not exist. Typos deliberate.
        fakename = "should_noot_exxist"
        while fakename in bynames:
            chars = list(fakename)
            for i in range(len(chars)):
                if chars[i] == 'z':
                    chars[i] = 'A'
                    break
                elif chars[i] == 'Z':
                    continue
                else:
                    chars[i] = chr(ord(chars[i]) + 1)
                    break
            else:
                namei = namei + 1
                try:
                    fakename = allnames[namei]
                except IndexError:
                    # should never happen... if so, just forget it
                    break
            fakename = ''.join(chars)

        self.assertRaises(KeyError, grp.getgrnam, fakename)

        # Picking a nonexistent GID is hard, since getgrall() will not
        # necessarily report all existing groups (typical for LDAP based
        # directories in big organisations).
        # Try to find one in the range 1-99 as the lower IDs are typically
        # statically assigned and more likely to be reported accurately.
        nonexistent_gid = None
        candidates = list(range(1, 99))
        candidates = candidates[90:] + candidates[:90]
        for i in candidates:
            if i not in bygids:
                nonexistent_gid = i
                break

        if nonexistent_gid is not None:
            self.assertRaises(KeyError, grp.getgrgid, nonexistent_gid)

    def test_noninteger_gid(self):
        entries = grp.getgrall()
        if not entries:
            self.skipTest('no groups')
        # Choose an existent gid.
        gid = entries[0][2]
        self.assertWarns(DeprecationWarning, grp.getgrgid, float(gid))
        self.assertWarns(DeprecationWarning, grp.getgrgid, str(gid))


if __name__ == "__main__":
    unittest.main()
