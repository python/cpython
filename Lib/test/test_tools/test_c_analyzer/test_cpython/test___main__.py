import sys
import unittest

from .. import tool_imports_for_tests
with tool_imports_for_tests():
    from c_analyzer.variables import info
    from cpython import SOURCE_DIRS
    from cpython.supported import IGNORED_FILE
    from cpython.known import DATA_FILE as KNOWN_FILE
    from cpython.__main__ import (
            cmd_check, cmd_show, parse_args, main,
            )


TYPICAL = [
        (info.Variable.from_parts('src1/spam.c', None, 'var1', 'const char *'),
         True,
         ),
        (info.Variable.from_parts('src1/spam.c', 'ham', 'initialized', 'int'),
         True,
         ),
        (info.Variable.from_parts('src1/spam.c', None, 'var2', 'PyObject *'),
         False,
         ),
        (info.Variable.from_parts('src1/eggs.c', 'tofu', 'ready', 'int'),
         True,
         ),
        (info.Variable.from_parts('src1/spam.c', None, 'freelist', '(PyTupleObject *)[10]'),
         False,
         ),
        (info.Variable.from_parts('src1/sub/ham.c', None, 'var1', 'const char const *'),
         True,
         ),
        (info.Variable.from_parts('src2/jam.c', None, 'var1', 'int'),
         True,
         ),
        (info.Variable.from_parts('src2/jam.c', None, 'var2', 'MyObject *'),
         False,
         ),
        (info.Variable.from_parts('Include/spam.h', None, 'data', 'const int'),
         True,
         ),
        ]


class CMDBase(unittest.TestCase):

    maxDiff = None

#    _return_known_from_file = None
#    _return_ignored_from_file = None
    _return_find = ()

    @property
    def calls(self):
        try:
            return self._calls
        except AttributeError:
            self._calls = []
            return self._calls

#    def _known_from_file(self, *args):
#        self.calls.append(('_known_from_file', args))
#        return self._return_known_from_file or {}
#
#    def _ignored_from_file(self, *args):
#        self.calls.append(('_ignored_from_file', args))
#        return self._return_ignored_from_file or {}

    def _find(self, known, ignored, skip_objects=False):
        self.calls.append(('_find', (known, ignored, skip_objects)))
        return self._return_find

    def _show(self, *args):
        self.calls.append(('_show', args))

    def _print(self, *args):
        self.calls.append(('_print', args))


class CheckTests(CMDBase):

    def test_defaults(self):
        self._return_find = []

        cmd_check('check',
                  _find=self._find,
                  _show=self._show,
                  _print=self._print,
                  )

        self.assertEqual(
                self.calls[0],
                ('_find', (KNOWN_FILE, IGNORED_FILE, False)),
                )

    def test_all_supported(self):
        self._return_find = [(v, s) for v, s in TYPICAL if s]
        dirs = ['src1', 'src2', 'Include']

        cmd_check('check',
                  known='known.tsv',
                  ignored='ignored.tsv',
                  _find=self._find,
                  _show=self._show,
                  _print=self._print,
                  )

        self.assertEqual(self.calls, [
            ('_find', ('known.tsv', 'ignored.tsv', False)),
            #('_print', ('okay',)),
            ])

    def test_some_unsupported(self):
        self._return_find = TYPICAL

        with self.assertRaises(SystemExit) as cm:
            cmd_check('check',
                      known='known.tsv',
                      ignored='ignored.tsv',
                      _find=self._find,
                      _show=self._show,
                      _print=self._print,
                      )

        unsupported = [v for v, s in TYPICAL if not s]
        self.assertEqual(self.calls, [
            ('_find', ('known.tsv', 'ignored.tsv', False)),
            ('_print', ('ERROR: found unsupported global variables',)),
            ('_print', ()),
            ('_show', (sorted(unsupported),)),
            ('_print', (' (3 total)',)),
            ])
        self.assertEqual(cm.exception.code, 1)


class ShowTests(CMDBase):

    def test_defaults(self):
        self._return_find = []

        cmd_show('show',
                 _find=self._find,
                 _show=self._show,
                 _print=self._print,
                 )

        self.assertEqual(
                self.calls[0],
                ('_find', (KNOWN_FILE, IGNORED_FILE, False)),
                )

    def test_typical(self):
        self._return_find = TYPICAL

        cmd_show('show',
                 known='known.tsv',
                 ignored='ignored.tsv',
                 _find=self._find,
                 _show=self._show,
                 _print=self._print,
                 )

        supported = [v for v, s in TYPICAL if s]
        unsupported = [v for v, s in TYPICAL if not s]
        self.assertEqual(self.calls, [
            ('_find', ('known.tsv', 'ignored.tsv', False)),
            ('_print', ('supported:',)),
            ('_print', ('----------',)),
            ('_show', (sorted(supported),)),
            ('_print', (' (6 total)',)),
            ('_print', ()),
            ('_print', ('unsupported:',)),
            ('_print', ('------------',)),
            ('_show', (sorted(unsupported),)),
            ('_print', (' (3 total)',)),
            ])


class ParseArgsTests(unittest.TestCase):

    maxDiff = None

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
            'ignored': IGNORED_FILE,
            'known': KNOWN_FILE,
            #'dirs': SOURCE_DIRS,
            })

    def test_check_full_args(self):
        cmd, cmdkwargs = parse_args('cg', [
            'check',
            '--ignored', 'spam.tsv',
            '--known', 'eggs.tsv',
            #'dir1',
            #'dir2',
            #'dir3',
            ])

        self.assertEqual(cmd, 'check')
        self.assertEqual(cmdkwargs, {
            'ignored': 'spam.tsv',
            'known': 'eggs.tsv',
            #'dirs': ['dir1', 'dir2', 'dir3']
            })

    def test_show_no_args(self):
        cmd, cmdkwargs = parse_args('cg', [
            'show',
            ])

        self.assertEqual(cmd, 'show')
        self.assertEqual(cmdkwargs, {
            'ignored': IGNORED_FILE,
            'known': KNOWN_FILE,
            #'dirs': SOURCE_DIRS,
            'skip_objects': False,
            })

    def test_show_full_args(self):
        cmd, cmdkwargs = parse_args('cg', [
            'show',
            '--ignored', 'spam.tsv',
            '--known', 'eggs.tsv',
            #'dir1',
            #'dir2',
            #'dir3',
            ])

        self.assertEqual(cmd, 'show')
        self.assertEqual(cmdkwargs, {
            'ignored': 'spam.tsv',
            'known': 'eggs.tsv',
            #'dirs': ['dir1', 'dir2', 'dir3'],
            'skip_objects': False,
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
