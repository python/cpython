"""Test script for the grp module."""

import grp
import unittest
from test import test_support

class GroupDatabaseTestCase(unittest.TestCase):

    def check_value(self, value):
        # check that a grp tuple has the entries and
        # attributes promised by the docs
        self.assertEqual(len(value), 4)
        self.assertEqual(value[0], value.gr_name)
        self.assert_(isinstance(value.gr_name, basestring))
        self.assertEqual(value[1], value.gr_passwd)
        self.assert_(isinstance(value.gr_passwd, basestring))
        self.assertEqual(value[2], value.gr_gid)
        self.assert_(isinstance(value.gr_gid, int))
        self.assertEqual(value[3], value.gr_mem)
        self.assert_(isinstance(value.gr_mem, list))

    def valueseq(self, value1, value2):
        # are two grp tuples equal (don't compare passwords)
        return value1.gr_name==value2.gr_name and \
            value1.gr_gid==value2.gr_gid and value1.gr_mem==value2.gr_mem

    def test_values(self):
        entries = grp.getgrall()
        entriesbygid = {}
        entriesbyname = {}

        # we can't use the same strategy as in test_pwd, because
        # we can't compare gr_passwd (Mac OS X returns
        # "*" in getgrall() and "" in getgrgid())

        for e in entries:
            self.check_value(e)
            entriesbygid.setdefault(e.gr_gid, []).append(e)
            entriesbyname.setdefault(e.gr_name, []).append(e)

        for e in entries:
            e2 = grp.getgrgid(e.gr_gid)
            self.check_value(e2)
            # make sure that at least one of the entries
            # for this gid compares equal to e2
            self.assert_(max([self.valueseq(e2, x) \
                for x in entriesbygid[e.gr_gid]]))
            e2 = grp.getgrnam(e.gr_name)
            self.check_value(e2)
            # make sure that at least one of the entries
            # for this name compares equal to e2
            self.assert_(max([self.valueseq(e2, x) \
                for x in entriesbyname[e.gr_name]]))

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
