import netrc, os, unittest, sys, textwrap, tempfile

from test import support
from unittest import mock

try:
    import pwd
except ImportError:
    pwd = None


def generate_netrc(directory, filename, data):
    data = textwrap.dedent(data)
    mode = 'w'
    mode = 'w'
    if sys.platform != 'cygwin':
        mode += 't'
    with open(os.path.join(directory, filename), mode, encoding="utf-8") as fp:
        fp.write(data)


class NetrcTestCase(unittest.TestCase):

    @staticmethod
    def home_netrc(data):
        with support.os_helper.EnvironmentVarGuard() as environ, \
            tempfile.TemporaryDirectory() as tmpdir:
                environ.unset('NETRC')
                environ.unset('HOME')

                generate_netrc(tmpdir, ".netrc", data)
                os.chmod(os.path.join(tmpdir, ".netrc"), 0o600)

                with mock.patch("os.path.expanduser"):
                    os.path.expanduser.return_value = tmpdir
                    nrc = netrc.netrc()

        return nrc

    @staticmethod
    def envvar_netrc(data):
        with support.os_helper.EnvironmentVarGuard() as environ:
            with tempfile.TemporaryDirectory() as tmpdir:
                environ.set('NETRC', os.path.join(tmpdir, ".netrc"))
                environ.unset('HOME')

                generate_netrc(tmpdir, ".netrc", data)
                os.chmod(os.path.join(tmpdir, ".netrc"), 0o600)

                nrc = netrc.netrc()

        return nrc
        
    @staticmethod
    def file_argument(data):
        with support.os_helper.EnvironmentVarGuard() as environ:
            with tempfile.TemporaryDirectory() as tmpdir:
                environ.set('NETRC', 'not-a-file.random')

                generate_netrc(tmpdir, ".netrc", data)
                os.chmod(os.path.join(tmpdir, ".netrc"), 0o600)

                nrc = netrc.netrc(os.path.join(tmpdir, ".netrc"))

        return nrc

    def make_nrc(self, test_data):
        return self.file_argument(test_data)
    
    @support.subTests('nrc_builder', (home_netrc, envvar_netrc, file_argument))
    def test_toplevel_non_ordered_tokens(self, nrc_builder):
        nrc = nrc_builder("""\
            machine host.domain.com password pass1 login log1 account acct1
            default login log2 password pass2 account acct2
            """)
        self.assertEqual(nrc.hosts['host.domain.com'], ('log1', 'acct1', 'pass1'))
        self.assertEqual(nrc.hosts['default'], ('log2', 'acct2', 'pass2'))

    @support.subTests('nrc_builder', (home_netrc, envvar_netrc, file_argument))
    def test_toplevel_tokens(self, nrc_builder):
        nrc = nrc_builder("""\
            machine host.domain.com login log1 password pass1 account acct1
            default login log2 password pass2 account acct2
            """)
        self.assertEqual(nrc.hosts['host.domain.com'], ('log1', 'acct1', 'pass1'))
        self.assertEqual(nrc.hosts['default'], ('log2', 'acct2', 'pass2'))

    @support.subTests('nrc_builder', (home_netrc, envvar_netrc, file_argument))
    def test_macros(self, nrc_builder):
        data = """\
            macdef macro1
            line1
            line2

            macdef macro2
            line3
            line4

        """
        nrc = nrc_builder(data)
        self.assertEqual(nrc.macros, {'macro1': ['line1\n', 'line2\n'],
                                      'macro2': ['line3\n', 'line4\n']})
        # strip the last \n
        self.assertRaises(netrc.NetrcParseError, self.make_nrc,
                          data.rstrip(' ')[:-1])

    @support.subTests('nrc_builder', (home_netrc, envvar_netrc, file_argument))
    def test_optional_tokens(self, nrc_builder):
        data = (
            "machine host.domain.com",
            "machine host.domain.com login",
            "machine host.domain.com account",
            "machine host.domain.com password",
            "machine host.domain.com login \"\" account",
            "machine host.domain.com login \"\" password",
            "machine host.domain.com account \"\" password"
        )
        for item in data:
            nrc = nrc_builder(item)
            self.assertEqual(nrc.hosts['host.domain.com'], ('', '', ''))
        data = (
            "default",
            "default login",
            "default account",
            "default password",
            "default login \"\" account",
            "default login \"\" password",
            "default account \"\" password"
        )
        for item in data:
            nrc = self.make_nrc(item)
            self.assertEqual(nrc.hosts['default'], ('', '', ''))

    @support.subTests('nrc_builder', (home_netrc, envvar_netrc, file_argument))
    def test_invalid_tokens(self, nrc_builder):
        data = (
            "invalid host.domain.com",
            "machine host.domain.com invalid",
            "machine host.domain.com login log password pass account acct invalid",
            "default host.domain.com invalid",
            "default host.domain.com login log password pass account acct invalid"
        )
        for item in data:
            self.assertRaises(netrc.NetrcParseError, nrc_builder, item)

    def _test_token_x(self, nrc, token, value):
        nrc = self.make_nrc(nrc)
        if token == 'login':
            self.assertEqual(nrc.hosts['host.domain.com'], (value, 'acct', 'pass'))
        elif token == 'account':
            self.assertEqual(nrc.hosts['host.domain.com'], ('log', value, 'pass'))
        elif token == 'password':
            self.assertEqual(nrc.hosts['host.domain.com'], ('log', 'acct', value))
    
    def test_token_value_quotes(self):
        self._test_token_x("""\
            machine host.domain.com login "log" password pass account acct
            """, 'login', 'log')
        self._test_token_x("""\
            machine host.domain.com login log password pass account "acct"
            """, 'account', 'acct')
        self._test_token_x("""\
            machine host.domain.com login log password "pass" account acct
            """, 'password', 'pass')

    def test_token_value_escape(self):
        self._test_token_x("""\
            machine host.domain.com login \\"log password pass account acct
            """, 'login', '"log')
        self._test_token_x("""\
            machine host.domain.com login "\\"log" password pass account acct
            """, 'login', '"log')
        self._test_token_x("""\
            machine host.domain.com login log password pass account \\"acct
            """, 'account', '"acct')
        self._test_token_x("""\
            machine host.domain.com login log password pass account "\\"acct"
            """, 'account', '"acct')
        self._test_token_x("""\
            machine host.domain.com login log password \\"pass account acct
            """, 'password', '"pass')
        self._test_token_x("""\
            machine host.domain.com login log password "\\"pass" account acct
            """, 'password', '"pass')

    def test_token_value_whitespace(self):
        self._test_token_x("""\
            machine host.domain.com login "lo g" password pass account acct
            """, 'login', 'lo g')
        self._test_token_x("""\
            machine host.domain.com login log password "pas s" account acct
            """, 'password', 'pas s')
        self._test_token_x("""\
            machine host.domain.com login log password pass account "acc t"
            """, 'account', 'acc t')

    def test_token_value_non_ascii(self):
        self._test_token_x("""\
            machine host.domain.com login \xa1\xa2 password pass account acct
            """, 'login', '\xa1\xa2')
        self._test_token_x("""\
            machine host.domain.com login log password pass account \xa1\xa2
            """, 'account', '\xa1\xa2')
        self._test_token_x("""\
            machine host.domain.com login log password \xa1\xa2 account acct
            """, 'password', '\xa1\xa2')

    def test_token_value_leading_hash(self):
        self._test_token_x("""\
            machine host.domain.com login #log password pass account acct
            """, 'login', '#log')
        self._test_token_x("""\
            machine host.domain.com login log password pass account #acct
            """, 'account', '#acct')
        self._test_token_x("""\
            machine host.domain.com login log password #pass account acct
            """, 'password', '#pass')

    def test_token_value_trailing_hash(self):
        self._test_token_x("""\
            machine host.domain.com login log# password pass account acct
            """, 'login', 'log#')
        self._test_token_x("""\
            machine host.domain.com login log password pass account acct#
            """, 'account', 'acct#')
        self._test_token_x("""\
            machine host.domain.com login log password pass# account acct
            """, 'password', 'pass#')

    def test_token_value_internal_hash(self):
        self._test_token_x("""\
            machine host.domain.com login lo#g password pass account acct
            """, 'login', 'lo#g')
        self._test_token_x("""\
            machine host.domain.com login log password pass account ac#ct
            """, 'account', 'ac#ct')
        self._test_token_x("""\
            machine host.domain.com login log password pa#ss account acct
            """, 'password', 'pa#ss')

    def _test_comment(self, nrc, passwd='pass'):
        nrc = self.make_nrc(nrc)
        self.assertEqual(nrc.hosts['foo.domain.com'], ('bar', '', passwd))
        self.assertEqual(nrc.hosts['bar.domain.com'], ('foo', '', 'pass'))

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

    def test_comment_after_machine_line(self):
        self._test_comment("""\
            machine foo.domain.com login bar password pass
            # comment
            machine bar.domain.com login foo password pass
            """)
        self._test_comment("""\
            machine foo.domain.com login bar password pass
            machine bar.domain.com login foo password pass
            # comment
            """)

    def test_comment_after_machine_line_no_space(self):
        self._test_comment("""\
            machine foo.domain.com login bar password pass
            #comment
            machine bar.domain.com login foo password pass
            """)
        self._test_comment("""\
            machine foo.domain.com login bar password pass
            machine bar.domain.com login foo password pass
            #comment
            """)

    def test_comment_after_machine_line_hash_only(self):
        self._test_comment("""\
            machine foo.domain.com login bar password pass
            #
            machine bar.domain.com login foo password pass
            """)
        self._test_comment("""\
            machine foo.domain.com login bar password pass
            machine bar.domain.com login foo password pass
            #
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
    @unittest.skipIf(pwd is None, 'security check requires pwd module')
    @support.os_helper.skip_unless_working_chmod
    def test_security(self):
        # This test is incomplete since we are normally not run as root and
        # therefore can't test the file ownership being wrong.
        d = support.os_helper.TESTFN
        os.mkdir(d)
        self.addCleanup(support.os_helper.rmtree, d)
        fn = os.path.join(d, '.netrc')
        with open(fn, 'wt') as f:
            f.write("""\
                machine foo.domain.com login bar password pass
                default login foo password pass
                """)
        with support.os_helper.EnvironmentVarGuard() as environ:
            environ.set('HOME', d)
            os.chmod(fn, 0o600)
            nrc = netrc.netrc()
            self.assertEqual(nrc.hosts['foo.domain.com'],
                             ('bar', '', 'pass'))
            os.chmod(fn, 0o622)
            self.assertRaises(netrc.NetrcParseError, netrc.netrc)
        with open(fn, 'wt') as f:
            f.write("""\
                machine foo.domain.com login anonymous password pass
                default login foo password pass
                """)
        with support.os_helper.EnvironmentVarGuard() as environ:
            environ.set('HOME', d)
            os.chmod(fn, 0o600)
            nrc = netrc.netrc()
            self.assertEqual(nrc.hosts['foo.domain.com'],
                             ('anonymous', '', 'pass'))
            os.chmod(fn, 0o622)
            self.assertEqual(nrc.hosts['foo.domain.com'],
                             ('anonymous', '', 'pass'))


if __name__ == "__main__":
    unittest.main()
