import sys
import unittest

from . import cg
from .cg.__main__ import parse_args, main


class ParseArgsTests(unittest.TestCase):

    def test_no_args(self):
        self.errmsg = None
        def fail(msg):
            self.errmsg = msg
            sys.exit(msg)

        with self.assertRaises(SystemExit):
            parse_args('cg', [], _fail=fail)

        self.assertEqual(self.errmsg, 'missing command')

    def test_check_no_args(self):
        cmd, cmdkwargs = parse_args('cg', [
            'check',
            ])

        self.assertEqual(cmd, 'check')
        self.assertEqual(cmdkwargs, {
            'ignored': cg.IGNORED_FILE,
            'known': cg.KNOWN_FILE,
            'dirs': cg.SOURCE_DIRS,
            })

    def test_check_full_args(self):
        cmd, cmdkwargs = parse_args('cg', [
            'check',
            '--ignored', 'spam.tsv',
            '--known', 'eggs.tsv',
            'dir1',
            'dir2',
            'dir3',
            ])

        self.assertEqual(cmd, 'check')
        self.assertEqual(cmdkwargs, {
            'ignored': 'spam.tsv',
            'known': 'eggs.tsv',
            'dirs': ['dir1', 'dir2', 'dir3']
            })

    def test_show_no_args(self):
        cmd, cmdkwargs = parse_args('cg', [
            'show',
            ])

        self.assertEqual(cmd, 'show')
        self.assertEqual(cmdkwargs, {
            'ignored': cg.IGNORED_FILE,
            'known': cg.KNOWN_FILE,
            'dirs': cg.SOURCE_DIRS,
            })

    def test_show_full_args(self):
        cmd, cmdkwargs = parse_args('cg', [
            'show',
            '--ignored', 'spam.tsv',
            '--known', 'eggs.tsv',
            'dir1',
            'dir2',
            'dir3',
            ])

        self.assertEqual(cmd, 'show')
        self.assertEqual(cmdkwargs, {
            'ignored': 'spam.tsv',
            'known': 'eggs.tsv',
            'dirs': ['dir1', 'dir2', 'dir3']
            })


def new_stub_commands(*names):
    calls = []
    def cmdfunc(cmd, **kwargs):
        calls.append((cmd, kwargs))
    commands = {name: cmdfunc for name in names}
    return commands, calls


class MainTests(unittest.TestCase):

    def test_no_command(self):
        with self.assertRaises(ValueError):
            main(None, {})

    def test_check(self):
        commands, calls = new_stub_commands('check', 'show')

        cmdkwargs = {
            'ignored': 'spam.tsv',
            'known': 'eggs.tsv',
            'dirs': ['dir1', 'dir2', 'dir3'],
            }
        main('check', cmdkwargs, _COMMANDS=commands)

        self.assertEqual(calls, [
            ('check', cmdkwargs),
            ])

    def test_show(self):
        commands, calls = new_stub_commands('check', 'show')

        cmdkwargs = {
            'ignored': 'spam.tsv',
            'known': 'eggs.tsv',
            'dirs': ['dir1', 'dir2', 'dir3'],
            }
        main('show', cmdkwargs, _COMMANDS=commands)

        self.assertEqual(calls, [
            ('show', cmdkwargs),
            ])
