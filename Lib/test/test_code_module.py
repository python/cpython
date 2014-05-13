"Test InteractiveConsole and InteractiveInterpreter from code module"
import sys
import unittest
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
        self.assertEqual(len(self.stderr.method_calls), 2)
        banner_call = self.stderr.method_calls[0]
        self.assertEqual(banner_call, ['write', ('Foo\n',), {}])

        # no banner
        self.stderr.reset_mock()
        self.infunc.side_effect = EOFError('Finished')
        self.console.interact(banner='')
        self.assertEqual(len(self.stderr.method_calls), 1)


def test_main():
    support.run_unittest(TestInteractiveConsole)

if __name__ == "__main__":
    unittest.main()
