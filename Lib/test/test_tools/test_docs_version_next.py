"""Sanity-check tests for the "docs/replace_versoin_next" tool."""

from pathlib import Path
import unittest

from test.test_tools import basepath
from test.support.import_helper import DirsOnSysPath, import_module
from test.support import os_helper
from test import support

print(basepath)
tooldir = Path(basepath) / 'Doc' / 'tools'
print(tooldir)

with DirsOnSysPath(str(tooldir)):
    import sys
    print(sys.path)
    version_next = import_module('version_next')

TO_CHANGE = """
Directives to change
--------------------

Here, all occurences of NEXT (lowercase) should be changed:

.. versionadded:: next

.. versionchanged:: next

.. deprecated:: next

.. deprecated-removed:: next 4.0

whitespace:

..   versionchanged:: next

.. versionchanged  :: next

    .. versionadded:: next

arguments:

.. versionadded:: next
    Foo bar

.. versionadded:: next as ``previousname``
"""

UNCHANGED = """
Unchanged
---------

Here, the word "next" should NOT be changed:

.. versionchanged:: NEXT

..versionchanged:: NEXT

... versionchanged:: next

foo .. versionchanged:: next

.. otherdirective:: next

.. VERSIONCHANGED: next

.. deprecated-removed: 3.0 next
"""

EXPECTED_CHANGED = TO_CHANGE.replace('next', 'VER')


class TestVersionNext(unittest.TestCase):
    maxDiff = len(TO_CHANGE + UNCHANGED) * 10

    def test_freeze_simple_script(self):
        with os_helper.temp_dir() as testdir:
            path = Path(testdir)
            path.joinpath('source.rst').write_text(TO_CHANGE + UNCHANGED)
            path.joinpath('subdir').mkdir()
            path.joinpath('subdir/change.rst').write_text(
                '.. versionadded:: next')
            path.joinpath('subdir/keep.not-rst').write_text(
                '.. versionadded:: next')
            path.joinpath('subdir/keep.rst').write_text(
                'nothing to see here')
            args = ['VER', testdir]
            if support.verbose:
                args.append('-vv')
            version_next.main(args)
            self.assertEqual(path.joinpath('source.rst').read_text(),
                             EXPECTED_CHANGED + UNCHANGED)
            self.assertEqual(path.joinpath('subdir/change.rst').read_text(),
                             '.. versionadded:: VER')
            self.assertEqual(path.joinpath('subdir/keep.not-rst').read_text(),
                             '.. versionadded:: next')
            self.assertEqual(path.joinpath('subdir/keep.rst').read_text(),
                             'nothing to see here')
