#!/usr/bin/env python3
"""
Replace `.. versionchanged:: next` lines in docs files by the given version.

Applies to all the VersionChange directives. For deprecated-removed, only
handle the first argument (deprecation version, not the removal version).

"""

import re
import sys
import argparse
from pathlib import Path

DIRECTIVE_RE = re.compile(
    r'''
        (?P<before>
            \s*\.\.\s+
            (version(added|changed|removed)|deprecated(-removed)?)
            \s*::\s*
        )
        next
        (?P<after>
            .*
        )
    ''',
    re.VERBOSE | re.DOTALL,
)

basedir = (Path(__file__)
           .parent  # cpython/Tools/doc
           .parent  # cpython/Tools
           .parent  # cpython
           .resolve()
           )
docdir = basedir / 'Doc'

parser = argparse.ArgumentParser(
    description=__doc__,
    formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('version',
                    help='String to replace "next" with.')
parser.add_argument('directory', type=Path, nargs='?',
                    help=f'Directory to process. Default: {docdir}',
                    default=docdir)
parser.add_argument('--verbose', '-v', action='count', default=0)

def main(argv):
    args = parser.parse_args(argv)
    version = args.version
    if args.verbose:
        print(
            f'Updating "next" versions in {args.directory} to {version!r}',
            file=sys.stderr)
    for path in Path(args.directory).glob('**/*.rst'):
        num_changed_lines = 0
        lines = []
        with open(path) as file:
            for lineno, line in enumerate(file, start=1):
                if match := DIRECTIVE_RE.fullmatch(line):
                    line = match['before'] + version + match['after']
                    num_changed_lines += 1
                lines.append(line)
        if num_changed_lines:
            if args.verbose:
                print(f'Updating file {path} ({num_changed_lines} changes)',
                    file=sys.stderr)
            with open(path, 'w') as file:
                file.writelines(lines)
        else:
            if args.verbose > 1:
                print(f'Unchanged file {path}', file=sys.stderr)


if __name__ == '__main__':
    main(sys.argv[1:])
