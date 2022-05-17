import copy
import os
import sys
import test.support
import unittest
import warnings
from test.support import os_helper
from test.support import warnings_helper


mailcap = warnings_helper.import_deprecated('mailcap')


# Location of mailcap file
MAILCAPFILE = test.support.findfile("mailcap.txt")

# Dict to act as mock mailcap entry for this test
# The keys and values should match the contents of MAILCAPFILE
MAILCAPDICT = {
    'application/x-movie':
        [{'compose': 'moviemaker %s',
          'x11-bitmap': '"/usr/lib/Zmail/bitmaps/movie.xbm"',
          'description': '"Movie"',
          'view': 'movieplayer %s',
          'lineno': 4}],
    'application/*':
        [{'copiousoutput': '',
          'view': 'echo "This is \\"%t\\" but        is 50 \\% Greek to me" \\; cat %s',
          'lineno': 5}],
    'audio/basic':
        [{'edit': 'audiocompose %s',
          'compose': 'audiocompose %s',
          'description': '"An audio fragment"',
          'view': 'showaudio %s',
          'lineno': 6}],
    'video/mpeg':
        [{'view': 'mpeg_play %s', 'lineno': 13}],
    'application/postscript':
        [{'needsterminal': '', 'view': 'ps-to-terminal %s', 'lineno': 1},
         {'compose': 'idraw %s', 'view': 'ps-to-terminal %s', 'lineno': 2}],
    'application/x-dvi':
        [{'view': 'xdvi %s', 'lineno': 3}],
    'message/external-body':
        [{'composetyped': 'extcompose %s',
          'description': '"A reference to data stored in an external location"',
          'needsterminal': '',
          'view': 'showexternal %s %{access-type} %{name} %{site}     %{directory} %{mode} %{server}',
          'lineno': 10}],
    'text/richtext':
        [{'test': 'test "`echo     %{charset} | tr \'[A-Z]\' \'[a-z]\'`"  = iso-8859-8',
          'copiousoutput': '',
          'view': 'shownonascii iso-8859-8 -e richtext -p %s',
          'lineno': 11}],
    'image/x-xwindowdump':
        [{'view': 'display %s', 'lineno': 9}],
    'audio/*':
        [{'view': '/usr/local/bin/showaudio %t', 'lineno': 7}],
    'video/*':
        [{'view': 'animate %s', 'lineno': 12}],
    'application/frame':
        [{'print': '"cat %s | lp"', 'view': 'showframe %s', 'lineno': 0}],
    'image/rgb':
        [{'view': 'display %s', 'lineno': 8}]
}

# For backwards compatibility, readmailcapfile() and lookup() still support
# the old version of mailcapdict without line numbers.
MAILCAPDICT_DEPRECATED = copy.deepcopy(MAILCAPDICT)
for entry_list in MAILCAPDICT_DEPRECATED.values():
    for entry in entry_list:
        entry.pop('lineno')


class HelperFunctionTest(unittest.TestCase):

    def test_listmailcapfiles(self):
        # The return value for listmailcapfiles() will vary by system.
        # So verify that listmailcapfiles() returns a list of strings that is of
        # non-zero length.
        mcfiles = mailcap.listmailcapfiles()
        self.assertIsInstance(mcfiles, list)
        for m in mcfiles:
            self.assertIsInstance(m, str)
        with os_helper.EnvironmentVarGuard() as env:
            # According to RFC 1524, if MAILCAPS env variable exists, use that
            # and only that.
            if "MAILCAPS" in env:
                env_mailcaps = env["MAILCAPS"].split(os.pathsep)
            else:
                env_mailcaps = ["/testdir1/.mailcap", "/testdir2/mailcap"]
                env["MAILCAPS"] = os.pathsep.join(env_mailcaps)
                mcfiles = mailcap.listmailcapfiles()
        self.assertEqual(env_mailcaps, mcfiles)

    def test_readmailcapfile(self):
        # Test readmailcapfile() using test file. It should match MAILCAPDICT.
        with open(MAILCAPFILE, 'r') as mcf:
            with self.assertWarns(DeprecationWarning):
                d = mailcap.readmailcapfile(mcf)
        self.assertDictEqual(d, MAILCAPDICT_DEPRECATED)

    def test_lookup(self):
        # Test without key
        expected = [{'view': 'animate %s', 'lineno': 12},
                    {'view': 'mpeg_play %s', 'lineno': 13}]
        actual = mailcap.lookup(MAILCAPDICT, 'video/mpeg')
        self.assertListEqual(expected, actual)

        # Test with key
        key = 'compose'
        expected = [{'edit': 'audiocompose %s',
                     'compose': 'audiocompose %s',
                     'description': '"An audio fragment"',
                     'view': 'showaudio %s',
                     'lineno': 6}]
        actual = mailcap.lookup(MAILCAPDICT, 'audio/basic', key)
        self.assertListEqual(expected, actual)

        # Test on user-defined dicts without line numbers
        expected = [{'view': 'mpeg_play %s'}, {'view': 'animate %s'}]
        actual = mailcap.lookup(MAILCAPDICT_DEPRECATED, 'video/mpeg')
        self.assertListEqual(expected, actual)

    def test_subst(self):
        plist = ['id=1', 'number=2', 'total=3']
        # test case: ([field, MIMEtype, filename, plist=[]], <expected string>)
        test_cases = [
            (["", "audio/*", "foo.txt"], ""),
            (["echo foo", "audio/*", "foo.txt"], "echo foo"),
            (["echo %s", "audio/*", "foo.txt"], "echo foo.txt"),
            (["echo %t", "audio/*", "foo.txt"], "echo audio/*"),
            (["echo \\%t", "audio/*", "foo.txt"], "echo %t"),
            (["echo foo", "audio/*", "foo.txt", plist], "echo foo"),
            (["echo %{total}", "audio/*", "foo.txt", plist], "echo 3")
        ]
        for tc in test_cases:
            self.assertEqual(mailcap.subst(*tc[0]), tc[1])


class GetcapsTest(unittest.TestCase):

    def test_mock_getcaps(self):
        # Test mailcap.getcaps() using mock mailcap file in this dir.
        # Temporarily override any existing system mailcap file by pointing the
        # MAILCAPS environment variable to our mock file.
        with os_helper.EnvironmentVarGuard() as env:
            env["MAILCAPS"] = MAILCAPFILE
            caps = mailcap.getcaps()
            self.assertDictEqual(caps, MAILCAPDICT)

    def test_system_mailcap(self):
        # Test mailcap.getcaps() with mailcap file(s) on system, if any.
        caps = mailcap.getcaps()
        self.assertIsInstance(caps, dict)
        mailcapfiles = mailcap.listmailcapfiles()
        existingmcfiles = [mcf for mcf in mailcapfiles if os.path.exists(mcf)]
        if existingmcfiles:
            # At least 1 mailcap file exists, so test that.
            for (k, v) in caps.items():
                self.assertIsInstance(k, str)
                self.assertIsInstance(v, list)
                for e in v:
                    self.assertIsInstance(e, dict)
        else:
            # No mailcap files on system. getcaps() should return empty dict.
            self.assertEqual({}, caps)


class FindmatchTest(unittest.TestCase):

    def test_findmatch(self):

        # default findmatch arguments
        c = MAILCAPDICT
        fname = "foo.txt"
        plist = ["access-type=default", "name=john", "site=python.org",
                 "directory=/tmp", "mode=foo", "server=bar"]
        audio_basic_entry = {
            'edit': 'audiocompose %s',
            'compose': 'audiocompose %s',
            'description': '"An audio fragment"',
            'view': 'showaudio %s',
            'lineno': 6
        }
        audio_entry = {"view": "/usr/local/bin/showaudio %t", 'lineno': 7}
        video_entry = {'view': 'animate %s', 'lineno': 12}
        message_entry = {
            'composetyped': 'extcompose %s',
            'description': '"A reference to data stored in an external location"', 'needsterminal': '',
            'view': 'showexternal %s %{access-type} %{name} %{site}     %{directory} %{mode} %{server}',
            'lineno': 10,
        }

        # test case: (findmatch args, findmatch keyword args, expected output)
        #   positional args: caps, MIMEtype
        #   keyword args: key="view", filename="/dev/null", plist=[]
        #   output: (command line, mailcap entry)
        cases = [
            ([{}, "video/mpeg"], {}, (None, None)),
            ([c, "foo/bar"], {}, (None, None)),
            ([c, "video/mpeg"], {}, ('animate /dev/null', video_entry)),
            ([c, "audio/basic", "edit"], {}, ("audiocompose /dev/null", audio_basic_entry)),
            ([c, "audio/basic", "compose"], {}, ("audiocompose /dev/null", audio_basic_entry)),
            ([c, "audio/basic", "description"], {}, ('"An audio fragment"', audio_basic_entry)),
            ([c, "audio/basic", "foobar"], {}, (None, None)),
            ([c, "video/*"], {"filename": fname}, ("animate %s" % fname, video_entry)),
            ([c, "audio/basic", "compose"],
             {"filename": fname},
             ("audiocompose %s" % fname, audio_basic_entry)),
            ([c, "audio/basic"],
             {"key": "description", "filename": fname},
             ('"An audio fragment"', audio_basic_entry)),
            ([c, "audio/*"],
             {"filename": fname},
             ("/usr/local/bin/showaudio audio/*", audio_entry)),
            ([c, "message/external-body"],
             {"plist": plist},
             ("showexternal /dev/null default john python.org     /tmp foo bar", message_entry))
        ]
        self._run_cases(cases)

    @unittest.skipUnless(os.name == "posix", "Requires 'test' command on system")
    @unittest.skipIf(sys.platform == "vxworks", "'test' command is not supported on VxWorks")
    def test_test(self):
        # findmatch() will automatically check any "test" conditions and skip
        # the entry if the check fails.
        caps = {"test/pass": [{"test": "test 1 -eq 1"}],
                "test/fail": [{"test": "test 1 -eq 0"}]}
        # test case: (findmatch args, findmatch keyword args, expected output)
        #   positional args: caps, MIMEtype, key ("test")
        #   keyword args: N/A
        #   output: (command line, mailcap entry)
        cases = [
            # findmatch will return the mailcap entry for test/pass because it evaluates to true
            ([caps, "test/pass", "test"], {}, ("test 1 -eq 1", {"test": "test 1 -eq 1"})),
            # findmatch will return None because test/fail evaluates to false
            ([caps, "test/fail", "test"], {}, (None, None))
        ]
        self._run_cases(cases)

    def _run_cases(self, cases):
        for c in cases:
            self.assertEqual(mailcap.findmatch(*c[0], **c[1]), c[2])


if __name__ == '__main__':
    unittest.main()
