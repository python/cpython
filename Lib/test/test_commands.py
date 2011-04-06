'''
   Tests for commands module
   Nick Mathewson
'''
import unittest
import os, tempfile, re

from test.test_support import run_unittest, reap_children, import_module, \
                              check_warnings

# Silence Py3k warning
commands = import_module('commands', deprecated=True)

# The module says:
#   "NB This only works (and is only relevant) for UNIX."
#
# Actually, getoutput should work on any platform with an os.popen, but
# I'll take the comment as given, and skip this suite.

if os.name != 'posix':
    raise unittest.SkipTest('Not posix; skipping test_commands')


class CommandTests(unittest.TestCase):

    def test_getoutput(self):
        self.assertEqual(commands.getoutput('echo xyzzy'), 'xyzzy')
        self.assertEqual(commands.getstatusoutput('echo xyzzy'), (0, 'xyzzy'))

        # we use mkdtemp in the next line to create an empty directory
        # under our exclusive control; from that, we can invent a pathname
        # that we _know_ won't exist.  This is guaranteed to fail.
        dir = None
        try:
            dir = tempfile.mkdtemp()
            name = os.path.join(dir, "foo")

            status, output = commands.getstatusoutput('cat ' + name)
            self.assertNotEqual(status, 0)
        finally:
            if dir is not None:
                os.rmdir(dir)

    def test_getstatus(self):
        # This pattern should match 'ls -ld /.' on any posix
        # system, however perversely configured.  Even on systems
        # (e.g., Cygwin) where user and group names can have spaces:
        #     drwxr-xr-x   15 Administ Domain U     4096 Aug 12 12:50 /
        #     drwxr-xr-x   15 Joe User My Group     4096 Aug 12 12:50 /
        # Note that the first case above has a space in the group name
        # while the second one has a space in both names.
        # Special attributes supported:
        #   + = has ACLs
        #   @ = has Mac OS X extended attributes
        #   . = has a SELinux security context
        pat = r'''d.........   # It is a directory.
                  [.+@]?       # It may have special attributes.
                  \s+\d+       # It has some number of links.
                  [^/]*        # Skip user, group, size, and date.
                  /\.          # and end with the name of the file.
               '''

        with check_warnings((".*commands.getstatus.. is deprecated",
                             DeprecationWarning)):
            self.assertTrue(re.match(pat, commands.getstatus("/."), re.VERBOSE))


def test_main():
    run_unittest(CommandTests)
    reap_children()


if __name__ == "__main__":
    test_main()
