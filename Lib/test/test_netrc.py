import netrc, os, unittest, sys, textwrap, tempfile

from test import support
from unittest import mock
from contextlib import ExitStack

try:
    import pwd
except ImportError:
    pwd = None


class NetrcEnvironment:
    def __enter__(self):
        self.stack = ExitStack()
        self.environ = self.stack.enter_context(support.os_helper.EnvironmentVarGuard())
        self.tmpdir = self.stack.enter_context(tempfile.TemporaryDirectory())
        return self

    def __exit__(self, *ignore_exc):
        self.stack.close()

    def generate_netrc(self, content, filename=".netrc", mode=0o600, encoding="utf-8"):
        write_mode = "w"
        if sys.platform != "cygwin":
            write_mode += "t"

        netrc_file = os.path.join(self.tmpdir, filename)
        with open(netrc_file, mode=write_mode, encoding=encoding) as fp:
            fp.write(textwrap.dedent(content))

        os.chmod(netrc_file, mode=mode)

        return netrc_file


class NetrcBuilder:
    @staticmethod
    def use_default_netrc_in_home(*args, **kwargs):
        with NetrcEnvironment() as helper:
            helper.environ.unset("NETRC")
            helper.environ.unset("HOME")

            helper.generate_netrc(*args, **kwargs)

            with mock.patch("os.path.expanduser"):
                os.path.expanduser.return_value = helper.tmpdir
                return netrc.netrc()

    @staticmethod
    def use_netrc_envvar(*args, **kwargs):
        with NetrcEnvironment() as helper:
            netrc_file = helper.generate_netrc(*args, **kwargs)

            helper.environ.set("NETRC", netrc_file)
            return netrc.netrc()

    @staticmethod
    def use_file_argument(*args, **kwargs):
        with NetrcEnvironment() as helper:
            helper.environ.set("NETRC", "not-a-file.netrc")

            netrc_file = helper.generate_netrc(*args, **kwargs)
            return netrc.netrc(netrc_file)

    @staticmethod
    def use_all_strategies():
        return (NetrcBuilder.use_default_netrc_in_home,
                NetrcBuilder.use_netrc_envvar,
                NetrcBuilder.use_file_argument)


class NetrcTestCase(unittest.TestCase):
    ALL_STRATEGIES = NetrcBuilder.use_all_strategies()

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_toplevel_non_ordered_tokens(self, make_nrc):
        nrc = make_nrc("""\
            machine host.domain.com password pass1 login log1 account acct1
            default login log2 password pass2 account acct2
            """)
        self.assertEqual(nrc.hosts['host.domain.com'], ('log1', 'acct1', 'pass1'))
        self.assertEqual(nrc.hosts['default'], ('log2', 'acct2', 'pass2'))

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_toplevel_tokens(self, make_nrc):
        nrc = make_nrc("""\
            machine host.domain.com login log1 password pass1 account acct1
            default login log2 password pass2 account acct2
            """)
        self.assertEqual(nrc.hosts['host.domain.com'], ('log1', 'acct1', 'pass1'))
        self.assertEqual(nrc.hosts['default'], ('log2', 'acct2', 'pass2'))

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_macros(self, make_nrc):
        data = """\
            macdef macro1
            line1
            line2

            macdef macro2
            line3
            line4

        """
        nrc = make_nrc(data)
        self.assertEqual(nrc.macros, {'macro1': ['line1\n', 'line2\n'],
                                      'macro2': ['line3\n', 'line4\n']})
        # strip the last \n
        self.assertRaises(netrc.NetrcParseError, make_nrc,
                          data.rstrip(' ')[:-1])

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_optional_tokens(self, make_nrc):
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
            nrc = make_nrc(item)
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
            nrc = make_nrc(item)
            self.assertEqual(nrc.hosts['default'], ('', '', ''))

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_invalid_tokens(self, make_nrc):
        data = (
            "invalid host.domain.com",
            "machine host.domain.com invalid",
            "machine host.domain.com login log password pass account acct invalid",
            "default host.domain.com invalid",
            "default host.domain.com login log password pass account acct invalid"
        )
        for item in data:
            self.assertRaises(netrc.NetrcParseError, make_nrc, item)

    def _test_token_x(self, make_nrc, content, token, value):
        nrc = make_nrc(content)
        if token == 'login':
            self.assertEqual(nrc.hosts['host.domain.com'], (value, 'acct', 'pass'))
        elif token == 'account':
            self.assertEqual(nrc.hosts['host.domain.com'], ('log', value, 'pass'))
        elif token == 'password':
            self.assertEqual(nrc.hosts['host.domain.com'], ('log', 'acct', value))

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_token_value_quotes(self, make_nrc):
        self._test_token_x(make_nrc, """\
            machine host.domain.com login "log" password pass account acct
            """, 'login', 'log')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log password pass account "acct"
            """, 'account', 'acct')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log password "pass" account acct
            """, 'password', 'pass')

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_token_value_escape(self, make_nrc):
        self._test_token_x(make_nrc, """\
            machine host.domain.com login \\"log password pass account acct
            """, 'login', '"log')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login "\\"log" password pass account acct
            """, 'login', '"log')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log password pass account \\"acct
            """, 'account', '"acct')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log password pass account "\\"acct"
            """, 'account', '"acct')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log password \\"pass account acct
            """, 'password', '"pass')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log password "\\"pass" account acct
            """, 'password', '"pass')

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_token_value_whitespace(self, make_nrc):
        self._test_token_x(make_nrc, """\
            machine host.domain.com login "lo g" password pass account acct
            """, 'login', 'lo g')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log password "pas s" account acct
            """, 'password', 'pas s')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log password pass account "acc t"
            """, 'account', 'acc t')

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_token_value_non_ascii(self, make_nrc):
        self._test_token_x(make_nrc, """\
            machine host.domain.com login \xa1\xa2 password pass account acct
            """, 'login', '\xa1\xa2')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log password pass account \xa1\xa2
            """, 'account', '\xa1\xa2')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log password \xa1\xa2 account acct
            """, 'password', '\xa1\xa2')

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_token_value_leading_hash(self, make_nrc):
        self._test_token_x(make_nrc, """\
            machine host.domain.com login #log password pass account acct
            """, 'login', '#log')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log password pass account #acct
            """, 'account', '#acct')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log password #pass account acct
            """, 'password', '#pass')

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_token_value_trailing_hash(self, make_nrc):
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log# password pass account acct
            """, 'login', 'log#')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log password pass account acct#
            """, 'account', 'acct#')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log password pass# account acct
            """, 'password', 'pass#')

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_token_value_internal_hash(self, make_nrc):
        self._test_token_x(make_nrc, """\
            machine host.domain.com login lo#g password pass account acct
            """, 'login', 'lo#g')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log password pass account ac#ct
            """, 'account', 'ac#ct')
        self._test_token_x(make_nrc, """\
            machine host.domain.com login log password pa#ss account acct
            """, 'password', 'pa#ss')

    def _test_comment(self, make_nrc, content, passwd='pass'):
        nrc = make_nrc(content)
        self.assertEqual(nrc.hosts['foo.domain.com'], ('bar', '', passwd))
        self.assertEqual(nrc.hosts['bar.domain.com'], ('foo', '', 'pass'))

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_comment_before_machine_line(self, make_nrc):
        self._test_comment(make_nrc, """\
            # comment
            machine foo.domain.com login bar password pass
            machine bar.domain.com login foo password pass
            """)

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_comment_before_machine_line_no_space(self, make_nrc):
        self._test_comment(make_nrc, """\
            #comment
            machine foo.domain.com login bar password pass
            machine bar.domain.com login foo password pass
            """)

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_comment_before_machine_line_hash_only(self, make_nrc):
        self._test_comment(make_nrc, """\
            #
            machine foo.domain.com login bar password pass
            machine bar.domain.com login foo password pass
            """)

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_comment_after_machine_line(self, make_nrc):
        self._test_comment(make_nrc, """\
            machine foo.domain.com login bar password pass
            # comment
            machine bar.domain.com login foo password pass
            """)
        self._test_comment(make_nrc, """\
            machine foo.domain.com login bar password pass
            machine bar.domain.com login foo password pass
            # comment
            """)

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_comment_after_machine_line_no_space(self, make_nrc):
        self._test_comment(make_nrc, """\
            machine foo.domain.com login bar password pass
            #comment
            machine bar.domain.com login foo password pass
            """)
        self._test_comment(make_nrc, """\
            machine foo.domain.com login bar password pass
            machine bar.domain.com login foo password pass
            #comment
            """)

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_comment_after_machine_line_hash_only(self, make_nrc):
        self._test_comment(make_nrc, """\
            machine foo.domain.com login bar password pass
            #
            machine bar.domain.com login foo password pass
            """)
        self._test_comment(make_nrc, """\
            machine foo.domain.com login bar password pass
            machine bar.domain.com login foo password pass
            #
            """)

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_comment_at_end_of_machine_line(self, make_nrc):
        self._test_comment(make_nrc, """\
            machine foo.domain.com login bar password pass # comment
            machine bar.domain.com login foo password pass
            """)

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_comment_at_end_of_machine_line_no_space(self, make_nrc):
        self._test_comment(make_nrc, """\
            machine foo.domain.com login bar password pass #comment
            machine bar.domain.com login foo password pass
            """)

    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_comment_at_end_of_machine_line_pass_has_hash(self, make_nrc):
        self._test_comment(make_nrc, """\
            machine foo.domain.com login bar password #pass #comment
            machine bar.domain.com login foo password pass
            """, '#pass')

    @unittest.skipUnless(os.name == 'posix', 'POSIX only test')
    @unittest.skipIf(pwd is None, 'security check requires pwd module')
    @support.os_helper.skip_unless_working_chmod
    def test_non_anonymous_security(self):
        # This test is incomplete since we are normally not run as root and
        # therefore can't test the file ownership being wrong.
        content = """
            machine foo.domain.com login bar password pass
            default login foo password pass
        """
        mode = 0o662

        # Use ~/.netrc and login is not anon
        with self.assertRaises(netrc.NetrcParseError):
            NetrcBuilder.use_default_netrc_in_home(content, mode=mode)

        # Don't use default file
        nrc = NetrcBuilder.use_file_argument(content, mode=mode)
        self.assertEqual(nrc.hosts['foo.domain.com'],
                            ('bar', '', 'pass'))

    @unittest.skipUnless(os.name == 'posix', 'POSIX only test')
    @unittest.skipIf(pwd is None, 'security check requires pwd module')
    @support.os_helper.skip_unless_working_chmod
    @support.subTests('make_nrc', ALL_STRATEGIES)
    def test_anonymous_security(self, make_nrc):
        # This test is incomplete since we are normally not run as root and
        # therefore can't test the file ownership being wrong.
        content = """\
            machine foo.domain.com login anonymous password pass
        """
        mode = 0o662

        # When it's anonymous, file permissions are not bypassed
        nrc = make_nrc(content, mode=mode)
        self.assertEqual(nrc.hosts['foo.domain.com'],
                         ('anonymous', '', 'pass'))

    @unittest.skipUnless(os.name == 'posix', 'POSIX only test')
    @unittest.skipIf(pwd is None, 'security check requires pwd module')
    @support.os_helper.skip_unless_working_chmod
    def test_anonymous_security_with_default(self):
        # This test is incomplete since we are normally not run as root and
        # therefore can't test the file ownership being wrong.
        content = """\
            machine foo.domain.com login anonymous password pass
            default login foo password pass
        """
        mode = 0o622

        # "foo" is not anonymous, therefore the security check is triggered when we fallback to default netrc
        with self.assertRaises(netrc.NetrcParseError):
            NetrcBuilder.use_default_netrc_in_home(content, mode=mode)

        # Security check isn't triggered if the file is passed as environment variable or argument
        for make_nrc in (NetrcBuilder.use_file_argument, NetrcBuilder.use_netrc_envvar):
            nrc = make_nrc(content, mode=mode)
            self.assertEqual(nrc.hosts['foo.domain.com'],
                            ('anonymous', '', 'pass'))


if __name__ == "__main__":
    unittest.main()
