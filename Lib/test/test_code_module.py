"Test InteractiveConsole and InteractiveInterpreter from code module"
import sys
import traceback
import unittest
from textwrap import dedent
from contextlib import ExitStack
from unittest import mock
from test.support import import_helper


code = import_helper.import_module('code')


class MockSys:

    def mock_sys(self):
        "Mock system environment for InteractiveConsole"
        # use exit stack to match patch context managers to addCleanup
        stack = ExitStack()
        self.addCleanup(stack.close)
        self.infunc = stack.enter_context(mock.patch('code.input',
                                          create=True))
        self.stdout = stack.enter_context(mock.patch('code.sys.stdout'))
        self.stderr = stack.enter_context(mock.patch('code.sys.stderr'))
        prepatch = mock.patch('code.sys', wraps=code.sys, spec=code.sys)
        self.sysmod = stack.enter_context(prepatch)
        if sys.excepthook is sys.__excepthook__:
            self.sysmod.excepthook = self.sysmod.__excepthook__
        del self.sysmod.ps1
        del self.sysmod.ps2


class TestInteractiveConsole(unittest.TestCase, MockSys):
    maxDiff = None

    def setUp(self):
        self.console = code.InteractiveConsole()
        self.mock_sys()

    def test_ps1(self):
        self.infunc.side_effect = EOFError('Finished')
        self.console.interact()
        self.assertEqual(self.sysmod.ps1, '>>> ')
        self.sysmod.ps1 = 'custom1> '
        self.console.interact()
        self.assertEqual(self.sysmod.ps1, 'custom1> ')

    def test_ps2(self):
        self.infunc.side_effect = EOFError('Finished')
        self.console.interact()
        self.assertEqual(self.sysmod.ps2, '... ')
        self.sysmod.ps1 = 'custom2> '
        self.console.interact()
        self.assertEqual(self.sysmod.ps1, 'custom2> ')

    def test_console_stderr(self):
        self.infunc.side_effect = ["'antioch'", "", EOFError('Finished')]
        self.console.interact()
        for call in list(self.stdout.method_calls):
            if 'antioch' in ''.join(call[1]):
                break
        else:
            raise AssertionError("no console stdout")

    def test_syntax_error(self):
        self.infunc.side_effect = ["def f():",
                                   "    x = ?",
                                   "",
                                    EOFError('Finished')]
        self.console.interact()
        output = ''.join(''.join(call[1]) for call in self.stderr.method_calls)
        output = output[output.index('(InteractiveConsole)'):]
        output = output[:output.index('\nnow exiting')]
        self.assertEqual(output.splitlines()[1:], [
            '  File "<console>", line 2',
            '    x = ?',
            '        ^',
            'SyntaxError: invalid syntax'])
        self.assertIs(self.sysmod.last_type, SyntaxError)
        self.assertIs(type(self.sysmod.last_value), SyntaxError)
        self.assertIsNone(self.sysmod.last_traceback)
        self.assertIsNone(self.sysmod.last_value.__traceback__)
        self.assertIs(self.sysmod.last_exc, self.sysmod.last_value)

    def test_indentation_error(self):
        self.infunc.side_effect = ["  1", EOFError('Finished')]
        self.console.interact()
        output = ''.join(''.join(call[1]) for call in self.stderr.method_calls)
        output = output[output.index('(InteractiveConsole)'):]
        output = output[:output.index('\nnow exiting')]
        self.assertEqual(output.splitlines()[1:], [
            '  File "<console>", line 1',
            '    1',
            'IndentationError: unexpected indent'])
        self.assertIs(self.sysmod.last_type, IndentationError)
        self.assertIs(type(self.sysmod.last_value), IndentationError)
        self.assertIsNone(self.sysmod.last_traceback)
        self.assertIsNone(self.sysmod.last_value.__traceback__)
        self.assertIs(self.sysmod.last_exc, self.sysmod.last_value)

    def test_unicode_error(self):
        self.infunc.side_effect = ["'\ud800'", EOFError('Finished')]
        self.console.interact()
        output = ''.join(''.join(call[1]) for call in self.stderr.method_calls)
        output = output[output.index('(InteractiveConsole)'):]
        output = output[output.index('\n') + 1:]
        self.assertTrue(output.startswith('UnicodeEncodeError: '), output)
        self.assertIs(self.sysmod.last_type, UnicodeEncodeError)
        self.assertIs(type(self.sysmod.last_value), UnicodeEncodeError)
        self.assertIsNone(self.sysmod.last_traceback)
        self.assertIsNone(self.sysmod.last_value.__traceback__)
        self.assertIs(self.sysmod.last_exc, self.sysmod.last_value)

    def test_sysexcepthook(self):
        self.infunc.side_effect = ["def f():",
                                   "    raise ValueError('BOOM!')",
                                   "",
                                   "f()",
                                    EOFError('Finished')]
        hook = mock.Mock()
        self.sysmod.excepthook = hook
        self.console.interact()
        hook.assert_called()
        hook.assert_called_with(self.sysmod.last_type,
                                self.sysmod.last_value,
                                self.sysmod.last_traceback)
        self.assertIs(self.sysmod.last_type, ValueError)
        self.assertIs(type(self.sysmod.last_value), ValueError)
        self.assertIs(self.sysmod.last_traceback, self.sysmod.last_value.__traceback__)
        self.assertIs(self.sysmod.last_exc, self.sysmod.last_value)
        self.assertEqual(traceback.format_exception(self.sysmod.last_exc), [
            'Traceback (most recent call last):\n',
            '  File "<console>", line 1, in <module>\n',
            '  File "<console>", line 2, in f\n',
            'ValueError: BOOM!\n'])

    def test_sysexcepthook_syntax_error(self):
        self.infunc.side_effect = ["def f():",
                                   "    x = ?",
                                   "",
                                    EOFError('Finished')]
        hook = mock.Mock()
        self.sysmod.excepthook = hook
        self.console.interact()
        hook.assert_called()
        hook.assert_called_with(self.sysmod.last_type,
                                self.sysmod.last_value,
                                self.sysmod.last_traceback)
        self.assertIs(self.sysmod.last_type, SyntaxError)
        self.assertIs(type(self.sysmod.last_value), SyntaxError)
        self.assertIsNone(self.sysmod.last_traceback)
        self.assertIsNone(self.sysmod.last_value.__traceback__)
        self.assertIs(self.sysmod.last_exc, self.sysmod.last_value)
        self.assertEqual(traceback.format_exception(self.sysmod.last_exc), [
            '  File "<console>", line 2\n',
            '    x = ?\n',
            '        ^\n',
            'SyntaxError: invalid syntax\n'])

    def test_sysexcepthook_indentation_error(self):
        self.infunc.side_effect = ["  1", EOFError('Finished')]
        hook = mock.Mock()
        self.sysmod.excepthook = hook
        self.console.interact()
        hook.assert_called()
        hook.assert_called_with(self.sysmod.last_type,
                                self.sysmod.last_value,
                                self.sysmod.last_traceback)
        self.assertIs(self.sysmod.last_type, IndentationError)
        self.assertIs(type(self.sysmod.last_value), IndentationError)
        self.assertIsNone(self.sysmod.last_traceback)
        self.assertIsNone(self.sysmod.last_value.__traceback__)
        self.assertIs(self.sysmod.last_exc, self.sysmod.last_value)
        self.assertEqual(traceback.format_exception(self.sysmod.last_exc), [
            '  File "<console>", line 1\n',
            '    1\n',
            'IndentationError: unexpected indent\n'])

    def test_sysexcepthook_crashing_doesnt_close_repl(self):
        self.infunc.side_effect = ["1/0", "a = 123", "print(a)", EOFError('Finished')]
        self.sysmod.excepthook = 1
        self.console.interact()
        self.assertEqual(['write', ('123', ), {}], self.stdout.method_calls[0])
        error = "".join(call.args[0] for call in self.stderr.method_calls if call[0] == 'write')
        self.assertIn("Error in sys.excepthook:", error)
        self.assertEqual(error.count("'int' object is not callable"), 1)
        self.assertIn("Original exception was:", error)
        self.assertIn("division by zero", error)

    def test_sysexcepthook_raising_BaseException(self):
        self.infunc.side_effect = ["1/0", "a = 123", "print(a)", EOFError('Finished')]
        s = "not so fast"
        def raise_base(*args, **kwargs):
            raise BaseException(s)
        self.sysmod.excepthook = raise_base
        self.console.interact()
        self.assertEqual(['write', ('123', ), {}], self.stdout.method_calls[0])
        error = "".join(call.args[0] for call in self.stderr.method_calls if call[0] == 'write')
        self.assertIn("Error in sys.excepthook:", error)
        self.assertEqual(error.count("not so fast"), 1)
        self.assertIn("Original exception was:", error)
        self.assertIn("division by zero", error)

    def test_sysexcepthook_raising_SystemExit_gets_through(self):
        self.infunc.side_effect = ["1/0"]
        def raise_base(*args, **kwargs):
            raise SystemExit
        self.sysmod.excepthook = raise_base
        with self.assertRaises(SystemExit):
            self.console.interact()

    def test_banner(self):
        # with banner
        self.infunc.side_effect = EOFError('Finished')
        self.console.interact(banner='Foo')
        self.assertEqual(len(self.stderr.method_calls), 3)
        banner_call = self.stderr.method_calls[0]
        self.assertEqual(banner_call, ['write', ('Foo\n',), {}])

        # no banner
        self.stderr.reset_mock()
        self.infunc.side_effect = EOFError('Finished')
        self.console.interact(banner='')
        self.assertEqual(len(self.stderr.method_calls), 2)

    def test_exit_msg(self):
        # default exit message
        self.infunc.side_effect = EOFError('Finished')
        self.console.interact(banner='')
        self.assertEqual(len(self.stderr.method_calls), 2)
        err_msg = self.stderr.method_calls[1]
        expected = 'now exiting InteractiveConsole...\n'
        self.assertEqual(err_msg, ['write', (expected,), {}])

        # no exit message
        self.stderr.reset_mock()
        self.infunc.side_effect = EOFError('Finished')
        self.console.interact(banner='', exitmsg='')
        self.assertEqual(len(self.stderr.method_calls), 1)

        # custom exit message
        self.stderr.reset_mock()
        message = (
            'bye! \N{GREEK SMALL LETTER ZETA}\N{CYRILLIC SMALL LETTER ZHE}'
            )
        self.infunc.side_effect = EOFError('Finished')
        self.console.interact(banner='', exitmsg=message)
        self.assertEqual(len(self.stderr.method_calls), 2)
        err_msg = self.stderr.method_calls[1]
        expected = message + '\n'
        self.assertEqual(err_msg, ['write', (expected,), {}])


    def test_cause_tb(self):
        self.infunc.side_effect = ["raise ValueError('') from AttributeError",
                                    EOFError('Finished')]
        self.console.interact()
        output = ''.join(''.join(call[1]) for call in self.stderr.method_calls)
        expected = dedent("""
        AttributeError

        The above exception was the direct cause of the following exception:

        Traceback (most recent call last):
          File "<console>", line 1, in <module>
        ValueError
        """)
        self.assertIn(expected, output)
        self.assertIs(self.sysmod.last_type, ValueError)
        self.assertIs(type(self.sysmod.last_value), ValueError)
        self.assertIs(self.sysmod.last_traceback, self.sysmod.last_value.__traceback__)
        self.assertIsNotNone(self.sysmod.last_traceback)
        self.assertIs(self.sysmod.last_exc, self.sysmod.last_value)

    def test_context_tb(self):
        self.infunc.side_effect = ["try: ham\nexcept: eggs\n",
                                    EOFError('Finished')]
        self.console.interact()
        output = ''.join(''.join(call[1]) for call in self.stderr.method_calls)
        expected = dedent("""
        Traceback (most recent call last):
          File "<console>", line 1, in <module>
        NameError: name 'ham' is not defined

        During handling of the above exception, another exception occurred:

        Traceback (most recent call last):
          File "<console>", line 2, in <module>
        NameError: name 'eggs' is not defined
        """)
        self.assertIn(expected, output)
        self.assertIs(self.sysmod.last_type, NameError)
        self.assertIs(type(self.sysmod.last_value), NameError)
        self.assertIs(self.sysmod.last_traceback, self.sysmod.last_value.__traceback__)
        self.assertIsNotNone(self.sysmod.last_traceback)
        self.assertIs(self.sysmod.last_exc, self.sysmod.last_value)


class TestInteractiveConsoleLocalExit(unittest.TestCase, MockSys):

    def setUp(self):
        self.console = code.InteractiveConsole(local_exit=True)
        self.mock_sys()

    @unittest.skipIf(sys.flags.no_site, "exit() isn't defined unless there's a site module")
    def test_exit(self):
        # default exit message
        self.infunc.side_effect = ["exit()"]
        self.console.interact(banner='')
        self.assertEqual(len(self.stderr.method_calls), 2)
        err_msg = self.stderr.method_calls[1]
        expected = 'now exiting InteractiveConsole...\n'
        self.assertEqual(err_msg, ['write', (expected,), {}])


if __name__ == "__main__":
    unittest.main()
