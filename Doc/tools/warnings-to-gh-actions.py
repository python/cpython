#!/usr/bin/env python3

"""
Convert Sphinx warning messages to GitHub Actions.

Converts lines like:
    .../Doc/library/cgi.rst:98: WARNING: reference target not found
to:
    ::warning file=.../Doc/library/cgi.rst,line=98::reference target not found

Non-matching lines are echoed unchanged.

see: https://docs.github.com/en/actions/using-workflows/workflow-commands-for-github-actions#setting-a-warning-message
"""

import re
import sys

pattern = re.compile(r'(?P<file>[^:]+):(?P<line>\d+): WARNING: (?P<msg>.+)')

for line in sys.stdin:
    if match := pattern.fullmatch(line.strip()):
        print('::warning file={file},line={line}::{msg}'.format_map(match))
    else:
        print(line)
