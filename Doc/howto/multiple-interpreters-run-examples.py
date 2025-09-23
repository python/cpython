import os
import os.path
import shutil
import subprocess
import sys
import tempfile
from textwrap import dedent
import traceback


HOWTO_DIR = os.path.dirname(__file__)
HOWTO_FILE = os.path.join(HOWTO_DIR, 'multiple-interpreters.rst')

VERBOSITY = 3


def get_examples(lines):
    examples = {}
    current = None
    expected = None
    start = None
    for lno, line in enumerate(lines, 1):
        if current is None:
            if line.endswith('::'):
                # It *might* be an example.
                current = []
        else:
            if not current:
                if line.strip():
                    if line == '    from concurrent import interpreters':
                        # It *is* an example.
                        current.append(line)
                        expected = []
                        start = lno
                    else:
                        # It wasn't actually an example.
                        current = None
            elif not line.strip() or line.startswith('    '):
                # The example continues.
                current.append(line)
                before, sep, after = line.partition('# prints: ')
                if sep:
                    assert not before.strip(), before
                    expected.append(after)
            else:
                # We've reached the end of the example.
                assert not current[-1].strip(), current
                example = dedent(os.linesep.join(current))
                expected = ''.join(f'{l}{os.linesep}' for l in expected)
                examples[start] = (example, expected)
                current = expected = start = None

                if line.endswith('::'):
                    # It *might* be an example.
                    current = []
    if current:
        if current[-1].strip():
            current.append('')
        example = dedent(os.linesep.join(current))
        expected = ''.join(f'{l}{os.linesep}' for l in expected)
        examples[start] = (example, expected)
        current = expected = start = None
    return examples


def write_example(examplesdir, name, text):
    filename = os.path.join(examplesdir, f'example-{name}.py')
    with open(filename, 'w') as outfile:
        outfile.write(text)
    return filename, name, text


def run_example(filename, expected):
    if isinstance(expected, str):
        rc = 0
        stdout = expected
        stderr = ''
    else:
        rc, stdout, stderr = expected

    proc = subprocess.run(
        [sys.executable, filename],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        cwd=os.path.dirname(filename),
    )
    if proc.returncode != rc:
        if proc.returncode != 0 and proc.stderr:
            print(proc.stderr)
        raise AssertionError((proc.returncode, rc))
    assert proc.stdout == stdout, (proc.stdout, stdout)
    assert proc.stderr == stderr, (proc.stderr, stderr)


#######################################
# the script

def parse_args(argv=sys.argv[1:], prog=sys.argv[0]):
    import argparse
    parser = argparse.ArgumentParser(prog=prog)

    parser.add_argument('--dry-run', dest='dryrun', action='store_true')

    parser.add_argument('-v', '--verbose', action='count', default=0)
    parser.add_argument('-q', '--quiet', action='count', default=0)

    parser.add_argument('requested', nargs='*')

    args = parser.parse_args(argv)
    ns = vars(args)

    args.verbosity = max(0, VERBOSITY + ns.pop('verbose') - ns.pop('quiet'))

    requested = []
    for req in args.requested:
        for i in req.replace(',', ' ').split():
            requested.append(int(i))
    args.requested = requested or None

    return ns


def main(requested=None, *, verbosity=VERBOSITY, dryrun=False):
    with open(HOWTO_FILE) as infile:
        examples = get_examples(l.rstrip(os.linesep) for l in infile)
    examplesdir = tempfile.mkdtemp(prefix='multi-interp-howto-')
    try:
        if requested:
            requested = set(requested)
            _req = f', {len(requested)} requested'
        else:
            _req = ''
        summary = f'# ({len(examples)} found{_req})'
        print(summary)

        failed = []
        for i, (lno, (text, expected)) in enumerate(examples.items(), 1):
            if requested and i not in requested:
                continue
            name = 'multiinterp-{i}'
            print()
            print('#'*60)
            print(f'# example {i}  ({os.path.relpath(HOWTO_FILE)}:{lno})')
            print('#'*60)
            print()
            if verbosity > VERBOSITY or (verbosity == VERBOSITY and dryrun):
                print(text)
                print('----')
                if expected.rstrip():
                    print(expected.rstrip(os.linesep))
                print('----')
            if dryrun:
                continue
            filename, _, _ = write_example(examplesdir, name, text)
            try:
                run_example(filename, expected)
            except Exception:
                traceback.print_exc()
                failed.append(str(i))

        req = f'/{len(requested)}' if requested else ''
        if failed:
            print()
            print(f'{len(failed)} failed: {",".join(failed)}')
            print(summary)
        elif verbosity > VERBOSITY or dryrun:
            print()
            print(f'{len(failed)} failed')
            print(summary)

        return len(failed)
    finally:
        shutil.rmtree(examplesdir)


if __name__ == '__main__':
    kwargs = parse_args()
    ec = main(**kwargs)
    sys.exit(ec)
