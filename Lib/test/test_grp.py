"""Test script for the grp module."""

import grp
import unittest
from test import test_support


class GroupDatabaseTestCase(unittest.TestCase):

    def test_values(self):
        entries = grp.getgrall()
        entriesbygid = {}

        for e in entries:
            self.assertEqual(len(e), 4)
            self.assertEqual(e[0], e.gr_name)
            self.assert_(isinstance(e.gr_name, basestring))
            self.assertEqual(e[1], e.gr_passwd)
            self.assert_(isinstance(e.gr_passwd, basestring))
            self.assertEqual(e[2], e.gr_gid)
            self.assert_(isinstance(e.gr_gid, int))
            self.assertEqual(e[3], e.gr_mem)
            self.assert_(isinstance(e.gr_mem, list))

            self.assertEqual(grp.getgrnam(e.gr_name), e)
            # The following won't work, because of duplicate entries
            # for one gid
            #    self.assertEqual(grp.getgrgid(e.gr_gid), e)
            # instead of this collect all entries for one uid
            # and check afterwards
            entriesbygid.setdefault(e.gr_gid, []).append(e)

        # check whether the entry returned by getgrgid()
        # for each uid is among those from getgrall() for this uid
        for e in entries:
            self.assert_(grp.getgrgid(e.gr_gid) in entriesbygid[e.gr_gid])

    def test_errors(self):
        self.assertRaises(TypeError, grp.getgrgid)
        self.assertRaises(TypeError, grp.getgrnam)
        self.assertRaises(TypeError, grp.getgrall, 42)

        # try to get some errors
        bynames = {}
        bygids = {}
        for (n, p, g, mem) in grp.getgrall():
            bynames[n] = g
            bygids[g] = n

        allnames = bynames.keys()
        namei = 0
        fakename = allnames[namei]
        while fakename in bynames:
            chars = map(None, fakename)
            for i in xrange(len(chars)):
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
            fakename = ''.join(map(None, chars))

        self.assertRaises(KeyError, grp.getgrnam, fakename)

        # Choose a non-existent gid.
        fakegid = 4127
        while fakegid in bygids:
            fakegid = (fakegid * 3) % 0x10000

        self.assertRaises(KeyError, grp.getgrgid, fakegid)

def test_main():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(GroupDatabaseTestCase))
    test_support.run_suite(suite)

if __name__ == "__main__":
    test_main()
