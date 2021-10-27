"""Sanity-check tests for the "freeze" tool."""

import sys
import textwrap
import unittest

from test import support

from . import imports_under_tool, skip_if_missing
skip_if_missing('freeze')
with imports_under_tool('freeze', 'test'):
    import freeze as helper


@unittest.skipIf(sys.platform.startswith('win'), 'not supported on Windows')
@support.skip_if_buildbot('not all buildbots have enough space')
class TestFreeze(unittest.TestCase):

    def test_freeze_simple_script(self):
        script = textwrap.dedent("""
            import sys
            print('running...')
            sys.exit(0)
            """)
        outdir, scriptfile, python = helper.prepare(script)

        executable = helper.freeze(python, scriptfile, outdir)
        text = helper.run(executable)
        self.assertEqual(text, 'running...')
