import random
import string
import sys
import unittest
from test.support import import_helper

pwd = import_helper.import_module('pwd')

@unittest.skipUnless(hasattr(pwd, 'getpwall'), 'Does not have getpwall()')
class PwdTest(unittest.TestCase):

    def test_values(self):
        entries = pwd.getpwall()

        for e in entries:
            self.assertEqual(len(e), 7)
            self.assertEqual(e[0], e.pw_name)
            self.assertIsInstance(e.pw_name, str)
            self.assertEqual(e[1], e.pw_passwd)
            self.assertIsInstance(e.pw_passwd, str)
            self.assertEqual(e[2], e.pw_uid)
            self.assertIsInstance(e.pw_uid, int)
            self.assertEqual(e[3], e.pw_gid)
            self.assertIsInstance(e.pw_gid, int)
            self.assertEqual(e[4], e.pw_gecos)
            self.assertIn(type(e.pw_gecos), (str, type(None)))
            self.assertEqual(e[5], e.pw_dir)
            self.assertIsInstance(e.pw_dir, str)
            self.assertEqual(e[6], e.pw_shell)
            self.assertIsInstance(e.pw_shell, str)

            # The following won't work, because of duplicate entries
            # for one uid
            #    self.assertEqual(pwd.getpwuid(e.pw_uid), e)
            # instead of this collect all entries for one uid
            # and check afterwards (done in test_values_extended)

    def test_values_extended(self):
        entries = pwd.getpwall()
        entriesbyname = {}
        entriesbyuid = {}

        if len(entries) > 1000:  # Huge passwd file (NIS?) -- skip this test
            self.skipTest('passwd file is huge; extended test skipped')

        for e in entries:
            entriesbyname.setdefault(e.pw_name, []).append(e)
            entriesbyuid.setdefault(e.pw_uid, []).append(e)

        # check whether the entry returned by getpwuid()
        # for each uid is among those from getpwall() for this uid
        for e in entries:
            if not e[0] or e[0] == '+':
                continue # skip NIS entries etc.
            self.assertIn(pwd.getpwnam(e.pw_name), entriesbyname[e.pw_name])
            self.assertIn(pwd.getpwuid(e.pw_uid), entriesbyuid[e.pw_uid])

    def test_errors(self):
        self.assertRaises(TypeError, pwd.getpwuid)
        self.assertRaises(TypeError, pwd.getpwuid, 3.14)
        self.assertRaises(TypeError, pwd.getpwuid, 0.0)
        self.assertRaises(TypeError, pwd.getpwuid, 0, 0)
        # should be out of uid_t range
        self.assertRaises(KeyError, pwd.getpwuid, 2**128)
        self.assertRaises(KeyError, pwd.getpwuid, -2**128)
        self.assertRaises(TypeError, pwd.getpwnam)
        self.assertRaises(TypeError, pwd.getpwnam, 42)
        self.assertRaises(TypeError, pwd.getpwnam, b'root')
        self.assertRaises(TypeError, pwd.getpwnam, 'root', 0)
        # embedded null character
        self.assertRaisesRegex(ValueError, 'null', pwd.getpwnam, 'a\x00b')
        self.assertRaisesRegex(ValueError, 'null', pwd.getpwnam, 'root\x00')
        self.assertRaises(UnicodeEncodeError, pwd.getpwnam, 'roo\udc74')
        self.assertRaises(KeyError, pwd.getpwnam, '')
        self.assertRaises(TypeError, pwd.getpwall, 42)

        # Find a non-existent user name.
        # getpwall() will not necessarily report all existing users
        # (typical for LDAP based directories in big organizations).
        for _ in range(30):
            fakename = ''.join(random.choices(string.ascii_lowercase, k=6))
            try:
                pwd.getpwnam(fakename)
            except KeyError:
                break
        else:
            self.fail('Cannot find non-existent user name')

        # Find a non-existent uid.
        maxuid = max(e.pw_uid for e in pwd.getpwall())
        if maxuid < 2**15:
            maxuid = 2**15
        elif maxuid < 2**16:
            maxuid = 2**16-1
        else:
            maxuid = 2**31
        for _ in range(30):
            fakeuid = random.randrange(maxuid)
            try:
                pwd.getpwuid(fakeuid)
            except KeyError:
                break
        else:
            self.fail('Cannot find non-existent uid')

        # On Cygwin, getpwuid(-1) returns 'Unknown+User' user
        if sys.platform != 'cygwin':
            # -1 shouldn't be a valid uid because it has a special meaning in many
            # uid-related functions
            self.assertRaises(KeyError, pwd.getpwuid, -1)


if __name__ == "__main__":
    unittest.main()
