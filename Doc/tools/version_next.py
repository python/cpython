#!/usr/bin/env python3
"""
Replace `.. versionchanged:: next` lines in docs files by the given version.

Run this at release time to replace `next` with the just-released version
in the sources.

No backups are made; add/commit to Git before running the script.

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

doc_dir = (Path(__file__)
           .parent  # cpython/Doc/tools
           .parent  # cpython/Doc
           .resolve()
           )

parser = argparse.ArgumentParser(
    description=__doc__,
    formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument('version',
                    help='String to replace "next" with. Usually `x.y`, '
                        + 'but can be anything.')
parser.add_argument('directory', type=Path, nargs='?',
                    help=f'Directory to process. Default: {doc_dir}',
                    default=doc_dir)
parser.add_argument('--verbose', '-v', action='count', default=0,
                    help='Increase verbosity. Can be repeated (`-vv`).')


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
        with open(path, encoding='utf-8') as file:
            for lineno, line in enumerate(file, start=1):
                try:
                    if match := DIRECTIVE_RE.fullmatch(line):
                        line = match['before'] + version + match['after']
                        num_changed_lines += 1
                    lines.append(line)
                except Exception as exc:
                    exc.add_note(f'processing line {path}:{lineno}')
                    raise
        if num_changed_lines:
            if args.verbose:
                print(f'Updating file {path} ({num_changed_lines} changes)',
                      file=sys.stderr)
            with open(path, 'w', encoding='utf-8') as file:
                file.writelines(lines)
        else:
            if args.verbose > 1:
                print(f'Unchanged file {path}', file=sys.stderr)


if __name__ == '__main__':
    main(sys.argv[1:])
