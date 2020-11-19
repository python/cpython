import netrc, os, unittest, sys, tempfile, textwrap
from test import support
from test.support import os_helper


class NetrcTestCase(unittest.TestCase):

    def make_nrc(self, test_data):
        test_data = textwrap.dedent(test_data)
        mode = 'w'
        if sys.platform != 'cygwin':
            mode += 't'
        temp_fd, temp_filename = tempfile.mkstemp()
        with os.fdopen(temp_fd, mode=mode) as fp:
            fp.write(test_data)
        self.addCleanup(os.unlink, temp_filename)
        return netrc.netrc(temp_filename)

    def test_default(self):
        nrc = self.make_nrc("""\
            machine host1.domain.com login log1 password pass1 account acct1
            default login log2 password pass2
            """)
        self.assertEqual(nrc.hosts['host1.domain.com'],
                         ('log1', 'acct1', 'pass1'))
        self.assertEqual(nrc.hosts['default'], ('log2', None, 'pass2'))

        nrc2 = self.make_nrc(nrc.__repr__())
        self.assertEqual(nrc.hosts, nrc2.hosts)

    def test_macros(self):
        nrc = self.make_nrc("""\
            macdef macro1
            line1
            line2

            macdef macro2
            line3
            line4
            """)
        self.assertEqual(nrc.macros, {'macro1': ['line1\n', 'line2\n'],
                                      'macro2': ['line3\n', 'line4\n']})

    def _test_passwords(self, nrc, passwd):
        nrc = self.make_nrc(nrc)
        self.assertEqual(nrc.hosts['host.domain.com'], ('log', 'acct', passwd))

    def test_password_with_leading_hash(self):
        self._test_passwords("""\
            machine host.domain.com login log password #pass account acct
            """, '#pass')

    def test_password_with_trailing_hash(self):
        self._test_passwords("""\
            machine host.domain.com login log password pass# account acct
            """, 'pass#')

    def test_password_with_internal_hash(self):
        self._test_passwords("""\
            machine host.domain.com login log password pa#ss account acct
            """, 'pa#ss')

    def _test_comment(self, nrc, passwd='pass'):
        nrc = self.make_nrc(nrc)
        self.assertEqual(nrc.hosts['foo.domain.com'], ('bar', None, passwd))
        self.assertEqual(nrc.hosts['bar.domain.com'], ('foo', None, 'pass'))

    def test_comment_before_machine_line(self):
        self._test_comment("""\
            # comment
            machine foo.domain.com login bar password pass
            machine bar.domain.com login foo password pass
            """)

    def test_comment_before_machine_line_no_space(self):
        self._test_comment("""\
            #comment
            machine foo.domain.com login bar password pass
            machine bar.domain.com login foo password pass
            """)

    def test_comment_before_machine_line_hash_only(self):
        self._test_comment("""\
            #
            machine foo.domain.com login bar password pass
            machine bar.domain.com login foo password pass
            """)

    def test_comment_at_end_of_machine_line(self):
        self._test_comment("""\
            machine foo.domain.com login bar password pass # comment
            machine bar.domain.com login foo password pass
            """)

    def test_comment_at_end_of_machine_line_no_space(self):
        self._test_comment("""\
            machine foo.domain.com login bar password pass #comment
            machine bar.domain.com login foo password pass
            """)

    def test_comment_at_end_of_machine_line_pass_has_hash(self):
        self._test_comment("""\
            machine foo.domain.com login bar password #pass #comment
            machine bar.domain.com login foo password pass
            """, '#pass')


    @unittest.skipUnless(os.name == 'posix', 'POSIX only test')
    def test_security(self):
        # This test is incomplete since we are normally not run as root and
        # therefore can't test the file ownership being wrong.
        d = os_helper.TESTFN
        os.mkdir(d)
        self.addCleanup(os_helper.rmtree, d)
        fn = os.path.join(d, '.netrc')
        with open(fn, 'wt') as f:
            f.write("""\
                machine foo.domain.com login bar password pass
                default login foo password pass
                """)
        with os_helper.EnvironmentVarGuard() as environ:
            environ.set('HOME', d)
            os.chmod(fn, 0o600)
            nrc = netrc.netrc()
            self.assertEqual(nrc.hosts['foo.domain.com'],
                             ('bar', None, 'pass'))
            os.chmod(fn, 0o622)
            self.assertRaises(netrc.NetrcParseError, netrc.netrc)

    def test_file_not_found_in_home(self):
        d = os_helper.TESTFN
        os.mkdir(d)
        self.addCleanup(os_helper.rmtree, d)
        with os_helper.EnvironmentVarGuard() as environ:
            environ.set('HOME', d)
            self.assertRaises(FileNotFoundError, netrc.netrc)

    def test_file_not_found_explicit(self):
        self.assertRaises(FileNotFoundError, netrc.netrc,
                          file='unlikely_netrc')

    def test_home_not_set(self):
        fake_home = os_helper.TESTFN
        os.mkdir(fake_home)
        self.addCleanup(os_helper.rmtree, fake_home)
        fake_netrc_path = os.path.join(fake_home, '.netrc')
        with open(fake_netrc_path, 'w') as f:
            f.write('machine foo.domain.com login bar password pass')
        os.chmod(fake_netrc_path, 0o600)

        orig_expanduser = os.path.expanduser
        called = []

        def fake_expanduser(s):
            called.append(s)
            with os_helper.EnvironmentVarGuard() as environ:
                environ.set('HOME', fake_home)
                environ.set('USERPROFILE', fake_home)
                result = orig_expanduser(s)
                return result

        with support.swap_attr(os.path, 'expanduser', fake_expanduser):
            nrc = netrc.netrc()
            login, account, password = nrc.authenticators('foo.domain.com')
            self.assertEqual(login, 'bar')

        self.assertTrue(called)


if __name__ == "__main__":
    unittest.main()
