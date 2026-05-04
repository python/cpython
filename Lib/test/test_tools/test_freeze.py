"""Sanity-check tests for the "freeze" tool."""

import sys
import textwrap
import unittest

from test import support
from test.support import os_helper
from test.test_tools import imports_under_tool, skip_if_missing

skip_if_missing('freeze')
with imports_under_tool('freeze', 'test'):
    import freeze as helper

@support.requires_zlib()
@unittest.skipIf(sys.platform.startswith('win'), 'not supported on Windows')
@unittest.skipIf(sys.platform == 'darwin' and sys._framework,
        'not supported for frameworks builds on macOS')
@support.skip_if_buildbot('not all buildbots have enough space')
# gh-103053: Skip test if Python is built with Profile Guided Optimization
# (PGO), since the test is just too slow in this case.
@unittest.skipIf(support.check_cflags_pgo(),
                 'test is too slow with PGO')
class TestFreeze(unittest.TestCase):

    @support.requires_resource('cpu') # Building Python is slow
    def test_freeze_simple_script(self):
        script = textwrap.dedent("""
            import sys
            print('running...')
            sys.exit(0)
            """)
        with os_helper.temp_dir() as outdir:
            outdir, scriptfile, python = helper.prepare(script, outdir)
            executable = helper.freeze(python, scriptfile, outdir)
            text = helper.run(executable)
        self.assertEqual(text, 'running...')
