"Test InteractiveConsole and InteractiveInterpreter from code module"
import sys
import unittest
from textwrap import dedent
from contextlib import ExitStack
from unittest import mock
from test import support

code = support.import_module('code')


class TestInteractiveConsole(unittest.TestCase):

    def setUp(self):
        self.console = code.InteractiveConsole()
        self.mock_sys()

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

    def test_ps1(self):
        self.infunc.side_effect = EOFError('Finished')
        self.console.interact()
        self.assertEqual(self.sysmod.ps1, '>>> ')

    def test_ps2(self):
        self.infunc.side_effect = EOFError('Finished')
        self.console.interact()
        self.assertEqual(self.sysmod.ps2, '... ')

    def test_console_stderr(self):
        self.infunc.side_effect = ["'antioch'", "", EOFError('Finished')]
        self.console.interact()
        for call in list(self.stdout.method_calls):
            if 'antioch' in ''.join(call[1]):
                break
        else:
            raise AssertionError("no console stdout")

    def test_syntax_error(self):
        self.infunc.side_effect = ["undefined", EOFError('Finished')]
        self.console.interact()
        for call in self.stderr.method_calls:
            if 'NameError' in ''.join(call[1]):
                break
        else:
            raise AssertionError("No syntax error from console")

    def test_sysexcepthook(self):
        self.infunc.side_effect = ["raise ValueError('')",
                                    EOFError('Finished')]
        hook = mock.Mock()
        self.sysmod.excepthook = hook
        self.console.interact()
        self.assertTrue(hook.called)

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


if __name__ == "__main__":
    unittest.main()
