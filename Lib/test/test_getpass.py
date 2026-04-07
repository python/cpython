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

    def check_raw_input(self, inputs, expect_result, prompt='Password: '):
        mock_input = StringIO(inputs)
        mock_output = StringIO()
        result = getpass._raw_input(prompt, mock_output, mock_input, '*')
        self.assertEqual(result, expect_result)
        return mock_output.getvalue()

    def test_null_char(self):
        self.check_raw_input('pass\x00word\n', 'password')

    def test_raw_input_with_echo_char(self):
        output = self.check_raw_input('my1pa$$word!\n', 'my1pa$$word!')
        self.assertEqual('Password: ************', output)

    def test_control_chars_with_echo_char(self):
        output = self.check_raw_input('pass\twd\b\n', 'pass\tw')
        # After backspace: refresh rewrites prompt + 6 echo chars
        self.assertEqual(
            'Password: *******'           # initial prompt + 7 echo chars
            '\r' + ' ' * 17 + '\r'        # clear line (10 prompt + 7 prev)
            'Password: ******',           # rewrite prompt + 6 echo chars
            output
        )

    def test_kill_ctrl_u_with_echo_char(self):
        # Ctrl+U (KILL) should clear the entire line
        output = self.check_raw_input('foo\x15bar\n', 'bar')
        # Should show "***" then refresh to clear, then show "***" for "bar"
        self.assertIn('***', output)
        # Display refresh uses \r to rewrite the line including prompt
        self.assertIn('\r', output)

    def test_werase_ctrl_w_with_echo_char(self):
        # Ctrl+W (WERASE) should delete the previous word
        self.check_raw_input('hello world\x17end\n', 'hello end')

    def test_ctrl_w_display_preserves_prompt(self):
        # Reproducer from gh-138577: type "hello world", Ctrl+W
        # Display must show "Password: ******" not "******rd: ***********"
        output = self.check_raw_input('hello world\x17\n', 'hello ')
        # The final visible state should be "Password: ******"
        # Verify prompt is rewritten during refresh, not overwritten by stars
        self.assertEndsWith(output, 'Password: ******')

    def test_ctrl_a_insert_display_preserves_prompt(self):
        # Reproducer from gh-138577: type "abc", Ctrl+A, type "x"
        # Display must show "Password: ****" not "****word: ***"
        output = self.check_raw_input('abc\x01x\n', 'xabc')
        # The final visible state should be "Password: ****"
        self.assertEndsWith(output, 'Password: ****\x08\x08\x08')

    def test_lnext_ctrl_v_with_echo_char(self):
        # Ctrl+V (LNEXT) should insert the next character literally
        self.check_raw_input('test\x16\x15more\n', 'test\x15more')

    def test_ctrl_a_move_to_start_with_echo_char(self):
        # Ctrl+A should move cursor to start
        self.check_raw_input('end\x01start\n', 'startend')

    def test_ctrl_a_cursor_position(self):
        # After Ctrl+A, cursor is at position 0.
        # Refresh writes backspaces to move cursor from end to start.
        output = self.check_raw_input('abc\x01\n', 'abc')
        self.assertEndsWith(output, 'Password: ***\x08\x08\x08')

    def test_ctrl_a_on_empty(self):
        # Ctrl+A on empty line should be a no-op
        self.check_raw_input('\x01hello\n', 'hello')

    def test_ctrl_a_already_at_start(self):
        # Double Ctrl+A should be same as single Ctrl+A
        self.check_raw_input('abc\x01\x01start\n', 'startabc')

    def test_ctrl_a_then_backspace(self):
        # Backspace after Ctrl+A should do nothing (cursor at 0)
        self.check_raw_input('abc\x01\x7f\n', 'abc')

    def test_ctrl_e_move_to_end_with_echo_char(self):
        # Ctrl+E should move cursor to end
        self.check_raw_input('start\x01X\x05end\n', 'Xstartend')

    def test_ctrl_e_cursor_position(self):
        # After Ctrl+A then Ctrl+E, cursor is back at end.
        # Refresh has no backspaces since cursor is at end.
        output = self.check_raw_input('abc\x01\x05\n', 'abc')
        self.assertEndsWith(output, 'Password: ***')

    def test_ctrl_e_on_empty(self):
        # Ctrl+E on empty line should be a no-op
        self.check_raw_input('\x05hello\n', 'hello')

    def test_ctrl_e_already_at_end(self):
        # Ctrl+E when already at end should be a no-op
        self.check_raw_input('abc\x05more\n', 'abcmore')

    def test_ctrl_a_then_ctrl_e(self):
        # Ctrl+A then Ctrl+E should return cursor to end, typing appends
        self.check_raw_input('abc\x01\x05def\n', 'abcdef')

    def test_ctrl_k_kill_forward_with_echo_char(self):
        # Ctrl+K should kill from cursor to end
        self.check_raw_input('delete\x01\x0bkeep\n', 'keep')

    def test_ctrl_c_interrupt_with_echo_char(self):
        # Ctrl+C should raise KeyboardInterrupt
        with self.assertRaises(KeyboardInterrupt):
            self.check_raw_input('test\x03more', '')

    def test_ctrl_d_eof_with_echo_char(self):
        # Ctrl+D twice should cause EOF
        self.check_raw_input('test\x04\x04', 'test')

    def test_backspace_at_start_with_echo_char(self):
        # Backspace at start should do nothing
        self.check_raw_input('\x7fhello\n', 'hello')

    def test_ctrl_k_at_end_with_echo_char(self):
        # Ctrl+K at end should do nothing
        self.check_raw_input('hello\x0b\n', 'hello')

    def test_ctrl_w_on_empty_with_echo_char(self):
        # Ctrl+W on empty line should do nothing
        self.check_raw_input('\x17hello\n', 'hello')

    def test_ctrl_u_on_empty_with_echo_char(self):
        # Ctrl+U on empty line should do nothing
        self.check_raw_input('\x15hello\n', 'hello')

    def test_multiple_ctrl_operations_with_echo_char(self):
        # Test combination: type, move, insert, delete
        # "world", Ctrl+A, "hello ", Ctrl+E, "!", Ctrl+A, Ctrl+K, "start"
        self.check_raw_input('world\x01hello \x05!\x01\x0bstart\n', 'start')

    def test_ctrl_w_multiple_words_with_echo_char(self):
        # Ctrl+W should delete only the last word
        self.check_raw_input('one two three\x17\n', 'one two ')

    def test_ctrl_v_then_ctrl_c_with_echo_char(self):
        # Ctrl+V should make Ctrl+C literal (not interrupt)
        self.check_raw_input('test\x16\x03end\n', 'test\x03end')


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
