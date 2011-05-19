"""Tests for distutils.cmd."""
import os

from packaging.command.cmd import Command
from packaging.dist import Distribution
from packaging.errors import PackagingOptionError
from packaging.tests import support, unittest


class MyCmd(Command):
    def initialize_options(self):
        pass


class CommandTestCase(support.LoggingCatcher,
                      unittest.TestCase):

    def setUp(self):
        super(CommandTestCase, self).setUp()
        dist = Distribution()
        self.cmd = MyCmd(dist)

    def test_make_file(self):
        cmd = self.cmd

        # making sure it raises when infiles is not a string or a list/tuple
        self.assertRaises(TypeError, cmd.make_file,
                          infiles=1, outfile='', func='func', args=())

        # making sure execute gets called properly
        def _execute(func, args, exec_msg, level):
            self.assertEqual(exec_msg, 'generating out from in')
        cmd.force = True
        cmd.execute = _execute
        cmd.make_file(infiles='in', outfile='out', func='func', args=())

    def test_dump_options(self):
        cmd = self.cmd
        cmd.option1 = 1
        cmd.option2 = 1
        cmd.user_options = [('option1', '', ''), ('option2', '', '')]
        cmd.dump_options()

        wanted = ["command options for 'MyCmd':", '  option1 = 1',
                  '  option2 = 1']
        msgs = self.get_logs()
        self.assertEqual(msgs, wanted)

    def test_ensure_string(self):
        cmd = self.cmd
        cmd.option1 = 'ok'
        cmd.ensure_string('option1')

        cmd.option2 = None
        cmd.ensure_string('option2', 'xxx')
        self.assertTrue(hasattr(cmd, 'option2'))

        cmd.option3 = 1
        self.assertRaises(PackagingOptionError, cmd.ensure_string, 'option3')

    def test_ensure_string_list(self):
        cmd = self.cmd
        cmd.option1 = 'ok,dok'
        cmd.ensure_string_list('option1')
        self.assertEqual(cmd.option1, ['ok', 'dok'])

        cmd.yes_string_list = ['one', 'two', 'three']
        cmd.yes_string_list2 = 'ok'
        cmd.ensure_string_list('yes_string_list')
        cmd.ensure_string_list('yes_string_list2')
        self.assertEqual(cmd.yes_string_list, ['one', 'two', 'three'])
        self.assertEqual(cmd.yes_string_list2, ['ok'])

        cmd.not_string_list = ['one', 2, 'three']
        cmd.not_string_list2 = object()
        self.assertRaises(PackagingOptionError,
                          cmd.ensure_string_list, 'not_string_list')

        self.assertRaises(PackagingOptionError,
                          cmd.ensure_string_list, 'not_string_list2')

    def test_ensure_filename(self):
        cmd = self.cmd
        cmd.option1 = __file__
        cmd.ensure_filename('option1')
        cmd.option2 = 'xxx'
        self.assertRaises(PackagingOptionError, cmd.ensure_filename, 'option2')

    def test_ensure_dirname(self):
        cmd = self.cmd
        cmd.option1 = os.path.dirname(__file__) or os.curdir
        cmd.ensure_dirname('option1')
        cmd.option2 = 'xxx'
        self.assertRaises(PackagingOptionError, cmd.ensure_dirname, 'option2')


def test_suite():
    return unittest.makeSuite(CommandTestCase)

if __name__ == '__main__':
    unittest.main(defaultTest='test_suite')
