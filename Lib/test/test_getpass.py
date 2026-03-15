import getpass
import os
import unittest
from io import BytesIO, StringIO, TextIOWrapper
from unittest import mock
from test import support

try:
    import termios
except ImportError:
    termios = None
try:
    import pwd
except ImportError:
    pwd = None

@mock.patch('os.environ')
class GetpassGetuserTest(unittest.TestCase):

    def test_username_takes_username_from_env(self, environ):
        expected_name = 'some_name'
        environ.get.return_value = expected_name
        self.assertEqual(expected_name, getpass.getuser())

    def test_username_priorities_of_env_values(self, environ):
        environ.get.return_value = None
        try:
            getpass.getuser()
        except OSError:  # in case there's no pwd module
            pass
        except KeyError:
            # current user has no pwd entry
            pass
        self.assertEqual(
            environ.get.call_args_list,
            [mock.call(x) for x in ('LOGNAME', 'USER', 'LNAME', 'USERNAME')])

    def test_username_falls_back_to_pwd(self, environ):
        expected_name = 'some_name'
        environ.get.return_value = None
        if pwd:
            with mock.patch('os.getuid') as uid, \
                    mock.patch('pwd.getpwuid') as getpw:
                uid.return_value = 42
                getpw.return_value = [expected_name]
                self.assertEqual(expected_name,
                                 getpass.getuser())
                getpw.assert_called_once_with(42)
        else:
            self.assertRaises(OSError, getpass.getuser)


class GetpassRawinputTest(unittest.TestCase):

    def test_flushes_stream_after_prompt(self):
        # see issue 1703
        stream = mock.Mock(spec=StringIO)
        input = StringIO('input_string')
        getpass._raw_input('some_prompt', stream, input=input)
        stream.flush.assert_called_once_with()

    def test_uses_stderr_as_default(self):
        input = StringIO('input_string')
        prompt = 'some_prompt'
        with mock.patch('sys.stderr') as stderr:
            getpass._raw_input(prompt, input=input)
            stderr.write.assert_called_once_with(prompt)

    @mock.patch('sys.stdin')
    def test_uses_stdin_as_default_input(self, mock_input):
        mock_input.readline.return_value = 'input_string'
        getpass._raw_input(stream=StringIO())
        mock_input.readline.assert_called_once_with()

    @mock.patch('sys.stdin')
    def test_uses_stdin_as_different_locale(self, mock_input):
        stream = TextIOWrapper(BytesIO(), encoding="ascii")
        mock_input.readline.return_value = "HasÅ‚o: "
        getpass._raw_input(prompt="HasÅ‚o: ",stream=stream)
        mock_input.readline.assert_called_once_with()


    def test_raises_on_empty_input(self):
        input = StringIO('')
        self.assertRaises(EOFError, getpass._raw_input, input=input)

    def test_trims_trailing_newline(self):
        input = StringIO('test\n')
        self.assertEqual('test', getpass._raw_input(input=input))


# Some of these tests are a bit white-box.  The functional requirement is that
# the password input be taken directly from the tty, and that it not be echoed
# on the screen, unless we are falling back to stderr/stdin.

# Some of these might run on platforms without termios, but play it safe.
@unittest.skipUnless(termios, 'tests require system with termios')
class UnixGetpassTest(unittest.TestCase):

    def test_uses_tty_directly(self):
        with mock.patch('os.open') as open, \
                mock.patch('io.FileIO') as fileio, \
                mock.patch('io.TextIOWrapper') as textio:
            # By setting open's return value to None the implementation will
            # skip code we don't care about in this test.  We can mock this out
            # fully if an alternate implementation works differently.
            open.return_value = None
            getpass.unix_getpass()
            open.assert_called_once_with('/dev/tty',
                                         os.O_RDWR | os.O_NOCTTY)
            fileio.assert_called_once_with(open.return_value, 'w+')
            textio.assert_called_once_with(fileio.return_value)

    def test_resets_termios(self):
        with mock.patch('os.open') as open, \
                mock.patch('io.FileIO'), \
                mock.patch('io.TextIOWrapper'), \
                mock.patch('termios.tcgetattr') as tcgetattr, \
                mock.patch('termios.tcsetattr') as tcsetattr:
            open.return_value = 3
            fake_attrs = [255, 255, 255, 255, 255]
            tcgetattr.return_value = list(fake_attrs)
            getpass.unix_getpass()
            tcsetattr.assert_called_with(3, mock.ANY, fake_attrs)

    def test_falls_back_to_fallback_if_termios_raises(self):
        with mock.patch('os.open') as open, \
                mock.patch('io.FileIO') as fileio, \
                mock.patch('io.TextIOWrapper') as textio, \
                mock.patch('termios.tcgetattr'), \
                mock.patch('termios.tcsetattr') as tcsetattr, \
                mock.patch('getpass.fallback_getpass') as fallback:
            open.return_value = 3
            fileio.return_value = BytesIO()
            tcsetattr.side_effect = termios.error
            getpass.unix_getpass()
            fallback.assert_called_once_with('Password: ',
                                             textio.return_value)

    def test_flushes_stream_after_input(self):
        # issue 7208
        with mock.patch('os.open') as open, \
                mock.patch('io.FileIO'), \
                mock.patch('io.TextIOWrapper'), \
                mock.patch('termios.tcgetattr'), \
                mock.patch('termios.tcsetattr'):
            open.return_value = 3
            mock_stream = mock.Mock(spec=StringIO)
            getpass.unix_getpass(stream=mock_stream)
            mock_stream.flush.assert_called_with()

    def test_falls_back_to_stdin(self):
        with mock.patch('os.open') as os_open, \
                mock.patch('sys.stdin', spec=StringIO) as stdin:
            os_open.side_effect = IOError
            stdin.fileno.side_effect = AttributeError
            with support.captured_stderr() as stderr:
                with self.assertWarns(getpass.GetPassWarning):
                    getpass.unix_getpass()
            stdin.readline.assert_called_once_with()
            self.assertIn('Warning', stderr.getvalue())
            self.assertIn('Password:', stderr.getvalue())

    def test_echo_char_replaces_input_with_asterisks(self):
        mock_result = '*************'
        with mock.patch('os.open') as os_open, \
                mock.patch('io.FileIO'), \
                mock.patch('io.TextIOWrapper') as textio, \
                mock.patch('termios.tcgetattr'), \
                mock.patch('termios.tcsetattr'), \
                mock.patch('getpass._raw_input') as mock_input:
            os_open.return_value = 3
            mock_input.return_value = mock_result

            result = getpass.unix_getpass(echo_char='*')
            mock_input.assert_called_once_with('Password: ', textio(),
                                               input=textio(), echo_char='*',
                                               term_ctrl_chars=mock.ANY)
            self.assertEqual(result, mock_result)

    def test_raw_input_with_echo_char(self):
        passwd = 'my1pa$$word!'
        mock_input = StringIO(f'{passwd}\n')
        mock_output = StringIO()
        with mock.patch('sys.stdin', mock_input), \
                mock.patch('sys.stdout', mock_output):
            result = getpass._raw_input('Password: ', mock_output, mock_input,
                                        '*')
        self.assertEqual(result, passwd)
        self.assertEqual('Password: ************', mock_output.getvalue())

    def test_control_chars_with_echo_char(self):
        passwd = 'pass\twd\b'
        expect_result = 'pass\tw'
        mock_input = StringIO(f'{passwd}\n')
        mock_output = StringIO()
        with mock.patch('sys.stdin', mock_input), \
                mock.patch('sys.stdout', mock_output):
            result = getpass._raw_input('Password: ', mock_output, mock_input,
                                        '*')
        self.assertEqual(result, expect_result)
        self.assertEqual('Password: *******\x08 \x08', mock_output.getvalue())

    def test_kill_ctrl_u_with_echo_char(self):
        # Ctrl+U (KILL) should clear the entire line
        passwd = 'foo\x15bar'  # Type "foo", hit Ctrl+U, type "bar"
        expect_result = 'bar'
        mock_input = StringIO(f'{passwd}\n')
        mock_output = StringIO()
        result = getpass._raw_input('Password: ', mock_output, mock_input,
                                    '*')
        self.assertEqual(result, expect_result)
        # Should show "***" then clear all 3, then show "***" for "bar"
        output = mock_output.getvalue()
        self.assertIn('***', output)
        # Should have backspaces to clear the "foo" part
        self.assertIn('\b', output)

    def test_werase_ctrl_w_with_echo_char(self):
        # Ctrl+W (WERASE) should delete the previous word
        passwd = 'hello world\x17end'  # Type "hello world", hit Ctrl+W, type "end"
        expect_result = 'hello end'
        mock_input = StringIO(f'{passwd}\n')
        mock_output = StringIO()
        result = getpass._raw_input('Password: ', mock_output, mock_input,
                                    '*')
        self.assertEqual(result, expect_result)

    def test_lnext_ctrl_v_with_echo_char(self):
        # Ctrl+V (LNEXT) should insert the next character literally
        passwd = 'test\x16\x15more'  # Type "test", hit Ctrl+V, then Ctrl+U (literal), type "more"
        expect_result = 'test\x15more'  # Should contain literal Ctrl+U
        mock_input = StringIO(f'{passwd}\n')
        mock_output = StringIO()
        result = getpass._raw_input('Password: ', mock_output, mock_input,
                                    '*')
        self.assertEqual(result, expect_result)

    def test_ctrl_a_move_to_start_with_echo_char(self):
        # Ctrl+A should move cursor to start
        # Type "end", Ctrl+A, type "start", result should be "startend"
        passwd = 'end\x01start'
        expect_result = 'startend'
        mock_input = StringIO(f'{passwd}\n')
        mock_output = StringIO()
        result = getpass._raw_input('Password: ', mock_output, mock_input,
                                    '*')
        self.assertEqual(result, expect_result)

    def test_ctrl_e_move_to_end_with_echo_char(self):
        # Ctrl+E should move cursor to end
        # Type "start", Ctrl+A, "X", Ctrl+E, "end" -> "Xstartend"
        passwd = 'start\x01X\x05end'
        expect_result = 'Xstartend'
        mock_input = StringIO(f'{passwd}\n')
        mock_output = StringIO()
        result = getpass._raw_input('Password: ', mock_output, mock_input,
                                    '*')
        self.assertEqual(result, expect_result)

    def test_ctrl_k_kill_forward_with_echo_char(self):
        # Ctrl+K should kill from cursor to end
        # Type "hello world", Ctrl+A, move 6 chars right (type 6 random chars then backspace 6),
        # Actually simpler: type "deleteworld", Ctrl+A, 6 chars forward somehow...
        # Let me use a different approach: "hello\x01\x0b" = type "hello", Ctrl+A, Ctrl+K
        # This should delete everything, resulting in ""
        passwd = 'delete\x01\x0bkeep'  # "delete", Ctrl+A, Ctrl+K, "keep"
        expect_result = 'keep'
        mock_input = StringIO(f'{passwd}\n')
        mock_output = StringIO()
        result = getpass._raw_input('Password: ', mock_output, mock_input,
                                    '*')
        self.assertEqual(result, expect_result)

    def test_ctrl_c_interrupt_with_echo_char(self):
        # Ctrl+C should raise KeyboardInterrupt
        passwd = 'test\x03more'  # Type "test", hit Ctrl+C
        mock_input = StringIO(passwd)
        mock_output = StringIO()
        with self.assertRaises(KeyboardInterrupt):
            getpass._raw_input('Password: ', mock_output, mock_input, '*')

    def test_ctrl_d_eof_with_echo_char(self):
        # Ctrl+D twice should cause EOF
        passwd = 'test\x04\x04'  # Type "test", hit Ctrl+D twice
        mock_input = StringIO(passwd)
        mock_output = StringIO()
        result = getpass._raw_input('Password: ', mock_output, mock_input, '*')
        self.assertEqual(result, 'test')

    def test_backspace_at_start_with_echo_char(self):
        # Backspace at start should do nothing
        passwd = '\x7fhello'  # Backspace, then "hello"
        expect_result = 'hello'
        mock_input = StringIO(f'{passwd}\n')
        mock_output = StringIO()
        result = getpass._raw_input('Password: ', mock_output, mock_input, '*')
        self.assertEqual(result, expect_result)

    def test_ctrl_k_at_end_with_echo_char(self):
        # Ctrl+K at end should do nothing
        passwd = 'hello\x0b'  # Type "hello", Ctrl+K at end
        expect_result = 'hello'
        mock_input = StringIO(f'{passwd}\n')
        mock_output = StringIO()
        result = getpass._raw_input('Password: ', mock_output, mock_input, '*')
        self.assertEqual(result, expect_result)

    def test_ctrl_w_on_empty_with_echo_char(self):
        # Ctrl+W on empty line should do nothing
        passwd = '\x17hello'  # Ctrl+W, then "hello"
        expect_result = 'hello'
        mock_input = StringIO(f'{passwd}\n')
        mock_output = StringIO()
        result = getpass._raw_input('Password: ', mock_output, mock_input, '*')
        self.assertEqual(result, expect_result)

    def test_ctrl_u_on_empty_with_echo_char(self):
        # Ctrl+U on empty line should do nothing
        passwd = '\x15hello'  # Ctrl+U, then "hello"
        expect_result = 'hello'
        mock_input = StringIO(f'{passwd}\n')
        mock_output = StringIO()
        result = getpass._raw_input('Password: ', mock_output, mock_input, '*')
        self.assertEqual(result, expect_result)

    def test_multiple_ctrl_operations_with_echo_char(self):
        # Test combination: type, move, insert, delete
        # "world", Ctrl+A, "hello ", Ctrl+E, "!", Ctrl+A, Ctrl+K, "start"
        passwd = 'world\x01hello \x05!\x01\x0bstart'
        expect_result = 'start'
        mock_input = StringIO(f'{passwd}\n')
        mock_output = StringIO()
        result = getpass._raw_input('Password: ', mock_output, mock_input, '*')
        self.assertEqual(result, expect_result)

    def test_ctrl_w_multiple_words_with_echo_char(self):
        # Ctrl+W should delete only the last word
        passwd = 'one two three\x17'  # Delete "three"
        expect_result = 'one two '
        mock_input = StringIO(f'{passwd}\n')
        mock_output = StringIO()
        result = getpass._raw_input('Password: ', mock_output, mock_input, '*')
        self.assertEqual(result, expect_result)

    def test_ctrl_v_then_ctrl_c_with_echo_char(self):
        # Ctrl+V should make Ctrl+C literal (not interrupt)
        passwd = 'test\x16\x03end'  # Type "test", Ctrl+V, Ctrl+C, "end"
        expect_result = 'test\x03end'  # Should contain literal Ctrl+C
        mock_input = StringIO(f'{passwd}\n')
        mock_output = StringIO()
        result = getpass._raw_input('Password: ', mock_output, mock_input, '*')
        self.assertEqual(result, expect_result)


class GetpassEchoCharTest(unittest.TestCase):

    def test_accept_none(self):
        getpass._check_echo_char(None)

    @support.subTests('echo_char', ["*", "A", " "])
    def test_accept_single_printable_ascii(self, echo_char):
        getpass._check_echo_char(echo_char)

    def test_reject_empty_string(self):
        self.assertRaises(ValueError, getpass.getpass, echo_char="")

    @support.subTests('echo_char', ["***", "AA", "aA*!"])
    def test_reject_multi_character_strings(self, echo_char):
        self.assertRaises(ValueError, getpass.getpass, echo_char=echo_char)

    @support.subTests('echo_char', [
        '\N{LATIN CAPITAL LETTER AE}',  # non-ASCII single character
        '\N{HEAVY BLACK HEART}',        # non-ASCII multibyte character
    ])
    def test_reject_non_ascii(self, echo_char):
        self.assertRaises(ValueError, getpass.getpass, echo_char=echo_char)

    @support.subTests('echo_char', [
        ch for ch in map(chr, range(0, 128))
        if not ch.isprintable()
    ])
    def test_reject_non_printable_characters(self, echo_char):
        self.assertRaises(ValueError, getpass.getpass, echo_char=echo_char)

    # TypeError Rejection
    @support.subTests('echo_char', [b"*", 0, 0.0, [], {}])
    def test_reject_non_string(self, echo_char):
        self.assertRaises(TypeError, getpass.getpass, echo_char=echo_char)


if __name__ == "__main__":
    unittest.main()
