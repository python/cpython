import argparse
from collections import deque
import fnmatch
import os
import re
import sys


def find(patterns, line):
    for pat in patterns:
        if pat.search(line) is not None:
            return True
    return False

def findall(patterns, line):
    if len(patterns) == 1:
        for m in patterns[0].finditer(line):
            yield m.group()
        return

    i = 0
    while True:
        begin = len(line) + 1
        end = -1
        best_span = (len(line) + 1, 1)
        for pat in patterns:
            m = pat.search(line, i)
            if m is not None:
                # Find the longest of the first matches.
                best_span = min(best_span, (m.start(), -m.end()))
                if best_span == (i, -len(line)):
                    # Matches the rest of line.
                    break
        start = best_span[0]
        end = -best_span[1]
        if end < 0:
            break
        yield line[start:end]
        i = end
        if start == end:
            i += 1

def grep(opts, patterns, file, filename):
    def print_line(sep, ln, line):
        if opts.filename:
            print(filename, end=sep)
        if opts.line_number:
            print(ln, end=sep)
        print(line)

    fast_exit = opts.quiet or opts.filename_only is not None
    prev_lines = deque(maxlen=opts.before_context)
    after_lines = 0
    next_group = None
    total_count = 0
    found = False
    for ln, line in enumerate(file, 1):
        line = line.removesuffix('\n')
        if opts.only_matching:
            for match in findall(patterns, line):
                print_line(':', ln, match)
                found = True
        else:
            matches = find(patterns, line)
            if opts.invert_match:
                matches = not matches
            if matches:
                found = True
                if fast_exit:
                    break
                elif opts.count:
                    total_count += 1
                else:
                    if next_group and ln > next_group:
                        print(opts.group_separator)
                    for ln2, line2 in enumerate(prev_lines, ln - len(prev_lines)):
                        print_line('-', ln2, line2)
                    prev_lines.clear()
                    print_line(':', ln, line)
                    after_lines = opts.after_context
                    next_group = (None if opts.group_separator is None else
                                  ln + after_lines + opts.before_context + 1)
            elif not (fast_exit or opts.count):
                if after_lines:
                    print_line('-', ln, line)
                    after_lines -= 1
                else:
                    prev_lines.append(line)
    if fast_exit:
        if opts.filename_only == found:
            print(filename)
    elif opts.count:
        if opts.filename:
            print(filename, end=':')
        print(total_count)
    return found

def read_from_file(filename):
    with open(filename, encoding=sys.stdin.encoding,
              errors=sys.stdin.errors) as f:
        return [line.removesuffix('\n') for line in f.readlines()]

def make_parser():
    parser = argparse.ArgumentParser(add_help=False)
    # Add --help option explicitly to avoid conflict in the -h option.
    parser.add_argument('--help',
            action='help', default=argparse.SUPPRESS,
            help='show this help message and exit')

    parser.set_defaults(label='(standard input)', group_separator='--')

    parser.add_argument('files',
            nargs=argparse.REMAINDER,
            metavar='FILES',
            help='Files to search.')

    grp = parser.add_argument_group('Matching Control')
    grp.add_argument('-e', '--regexp',
            action='append', dest='patterns',
            metavar='PATTERN',
            help='Use PATTERN as the pattern.')
    grp.add_argument('-f', '--file',
            action='extend', dest='patterns', type=read_from_file,
            metavar='PATTERN_FILE',
            help='Obtain patterns from PATTERN_FILE, one per line.')
    grp.add_argument('-F', '--fixed-strings',
            action='store_true',
            help='Interpret patterns as fixed strings.')
    grp.add_argument('-i', '--ignore-case',
            action='store_true',
            help='Ignore case distinctions in patterns and input data.')
    grp.add_argument('--no-ignore-case',
            action='store_false', dest='ignore_case',
            help='Do not ignore case distinctions in patterns and input data.  '
            'This is the default.')
    grp.add_argument('-v', '--invert-match',
            action='store_true',
            help='Invert the sense of matching, to select non-matching lines.')
    grp.add_argument('-w', '--word-regexp',
            action='store_true',
            help='Select only those lines containing matches that form whole words.')
    grp.add_argument('-x', '--line-regexp',
            action='store_true',
            help='Select only those matches that exactly match the whole line.')

    grp = parser.add_argument_group('Output Control')
    grp.add_argument('-c', '--count',
            action='store_true',
            help='Suppress normal output; instead print a count of matching '
            'lines for each input file.')
    grp.add_argument('-L', '--files-without-match',
            action='store_false', dest='filename_only', default=None,
            help='Suppress normal output; instead print the name of each input '
            'file from which no output would normally have been printed.')
    grp.add_argument('-l', '--files-with-match',
            action='store_true', dest='filename_only',
            help='Suppress normal output; instead print the name of each input '
            'file from which output would normally have been printed.')
    grp.add_argument('-o', '--only-matching',
            action='store_true',
            help='Print only the matched (non-empty) parts of a matching line, '
            'with each such part on a separate output line.')
    grp.add_argument('-q', '--quiet',
            action='store_true',
            help='Quiet; do not write anything to standard output.  '
            'Exit immediately with zero status if any match is found.')

    grp = parser.add_argument_group('Output Line Prefix Control')
    grp.add_argument('-H', '--with-filename',
            action='store_true', dest='filename', default=None,
            help='Print the file name for each match.  '
            'This is the default when there is more than one file to search.')
    grp.add_argument('-h', '--no-filename',
            action='store_false', dest='filename',
            help='Suppress the prefixing of file names on output.  '
            'This is the default when there is only one file (or only standard '
            'input) to search.')
    grp.add_argument('-n', '--line-number',
            action='store_true',
            help='Prefix each line of output with the 1-based line number '
            'within its input file.')

    grp = parser.add_argument_group('Context Line Control')
    grp.add_argument('-A', '--after-context',
            type=int, metavar='NUM',
            help='Print NUM lines of trailing context after matching lines.')
    grp.add_argument('-B', '--before-context',
            type=int, metavar='NUM',
            help='Print NUM lines of leading context before matching lines.')
    grp.add_argument('-C', '--context',
            type=int, default=0, metavar='NUM',
            help='Print NUM lines of output context.')

    grp = parser.add_argument_group('File and Directory Selection')
    grp.add_argument('-r', '--recursive',
            action='store_true',
            help='Read all files under each directory, recursively.')
    grp.add_argument('--include',
            action='append',
            metavar='GLOB',
            help='Search only files whose base name matches GLOB (using wildcard matching).')
    return parser

def parse_args():
    parser = make_parser()
    opts = parser.parse_args()

    # By default print filenames only if more than one file is specified.
    if opts.filename is None:
        opts.filename = len(opts.files) > 1
    # -C sets -A and -B if they are not specified explicitly.
    if opts.after_context is None:
        opts.after_context = opts.context
    if opts.before_context is None:
        opts.before_context = opts.context
    # -q, -l, -L and -c suppresses normal output.
    if opts.quiet or opts.filename_only is not None or opts.count:
        opts.only_matching = False
        opts.after_context = opts.before_context = 0
    # -vo suppresses normal output.
    if opts.only_matching and opts.invert_match:
        opts.only_matching = False
        opts.quiet = True
    # -o suppresses context output.
    if opts.only_matching:
        opts.after_context = opts.before_context = 0
    # Only print group separator for non-zero context.
    if not (opts.after_context or opts.before_context):
        opts.group_separator = None
    return opts

def compile_patterns(opts):
    patterns = opts.patterns or []
    if opts.fixed_strings:
        patterns = [re.escape(pat) for pat in patterns]
    if opts.line_regexp:
        patterns = [fr'\A(?:{pat})\Z' for pat in patterns]
    elif opts.word_regexp:
        patterns = [fr'\b(?:{pat})\b' for pat in patterns]
    flags = re.IGNORECASE if opts.ignore_case else 0
    return [re.compile(pat, flags) for pat in patterns]

def filter_file(opts, filename):
    return not opts.include or any(fnmatch.fnmatch(filename, pat)
                                   for pat in opts.include)

def iter_files(opts):
    for arg in opts.files or ['-']:
        if arg == '-':
            yield None
        elif os.path.isfile(arg):
            if filter_file(opts, os.path.basename(arg)):
                yield arg
        elif os.path.isdir(arg):
            if not opts.recursive:
                print(f'error: {arg!r} is a directory', file=sys.stderr)
                continue
            for dirpath, dirnames, filenames in os.walk(arg):
                for filename in filenames:
                    if filter_file(opts, filename):
                        yield os.path.join(dirpath, filename)
        else:
            print(f'error: {arg!r} has unsupported type', file=sys.stderr)

def main():
    opts = parse_args()
    patterns = compile_patterns(opts)

    found = False
    for filename in iter_files(opts):
        if filename is None:
            found |= grep(opts, patterns, sys.stdin, opts.label)
        else:
            with open(filename, encoding=sys.stdin.encoding,
                      errors=sys.stdin.errors) as f:
                found |= grep(opts, patterns, f, filename)
        if found and opts.quiet:
            break
    return found


if __name__ == '__main__':
    try:
        found = main()
    except SystemExit:
        raise
    except BaseException as e:
        print(f'error: {e!r}', file=sys.stderr)
        sys.exit(2)
    sys.exit(0 if found else 1)
