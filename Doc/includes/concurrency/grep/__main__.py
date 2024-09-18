import argparse
import sys

from . import Options, grep
from ._implementations import IMPLEMENTATIONS, impl_from_name, resolve_impl


def parse_args(argv=sys.argv[1:], prog=sys.argv[0]):
    parser = argparse.ArgumentParser(
        prog=prog,
        usage=('[--help] [--concurrency MODEL] [--cf] [-rivHhLloqs] '
               '[-e] REGEX FILE [FILE ...]'),
        add_help=False,
    )

    parser.add_argument('--help', action='help', default=argparse.SUPPRESS,
                        help='show this help message and exit')

    concurrencyopts = parser.add_argument_group(title='concurrency')
    concurrencyopts.add_argument('--concurrency', dest='impl',
                                 metavar=f'{{{",".join(sorted(IMPLEMENTATIONS))}}}')
    parser.set_defaults(impl='sequential')
    concurrencyopts.add_argument('--cf', '--concurrent-futures', dest='cf',
                                 action='store_const', const=True)
    concurrencyopts.add_argument('--cf-alt', dest='cf',
                                 action='store_const', const='alt')

    grepopts = parser.add_argument_group(title='grep')
    Options.add_cli(grepopts, 'opts')

    regexopts = grepopts.add_mutually_exclusive_group(required=True)
    regexopts.add_argument('-e', '--regexp', dest='regex', metavar='REGEX')
    regexopts.add_argument('regex', nargs='?', metavar='REGEX')
    grepopts.add_argument('files', nargs='+', metavar='FILE')

    args = parser.parse_args()
    ns = vars(args)

    impl = ns.pop('impl')
    cf = ns.pop('cf')
    opts = ns.pop('opts')
    pat = ns.pop('regex')
    files = ns.pop('files')
    assert not ns, ns

    try:
        impl = impl_from_name(impl, cf=cf)
    except ValueError as exc:
        parser.error(str(exc))

    return impl, opts, pat, files


def render_matches(matches, opts):
    matches = iter(matches)

    # Handle the first match.
    blank = False
    for filename, line, match in matches:
        if opts.quiet:
            blank = True
        elif opts.filesonly:
            yield filename
        elif opts.showonlymatch:
            if opts.invertmatch:
                blank = True
            elif opts.showfilename is False:
                yield match
            elif opts.showfilename:
                yield f'{filename}: {match}'
            else:
                try:
                    second = next(matches)
                except StopIteration:
                    yield match
                else:
                    yield f'{filename}: {match}'
                    filename, _, match = second
                    yield f'{filename}: {match}'
        else:
            if opts.showfilename is False:
                yield line
            elif opts.showfilename:
                yield f'{filename}: {line}'
            else:
                try:
                    second = next(matches)
                except StopIteration:
                    yield line
                else:
                    yield f'{filename}: {line}'
                    filename, line, _ = second
                    yield f'{filename}: {line}'
        break

    if blank:
        return

    # Handle the remaining matches.
    if opts.filesonly:
        for filename, _, _ in matches:
            yield filename
    elif opts.showonlymatch:
        if opts.showfilename is False:
            for filename, _, match in matches:
                yield match
        else:
            for filename, _, match in matches:
                yield f'{filename}: {match}'
    else:
        if opts.showfilename is False:
            for filename, line, _ in matches:
                yield line
        else:
            for filename, line, _ in matches:
                yield f'{filename}: {line}'


def main(opts, pat, filenames, impl='sequential', kind=None, cf=None):
    impl = resolve_impl(impl, kind, cf)

    # needed for step 5:
    matched = False
    def iter_matches(matches):
        nonlocal matched
        matches = iter(matches)
        for match in matches:
            matched = True
            yield match
            break
        yield from matches

    # steps 1-3:
    matches = grep(pat, opts, *filenames, impl=impl)
    # step 4:
    matches = iter_matches(matches)
    for line in render_matches(matches, opts):
        print(line)

    # step 5:
    return 0 if matched else 1


if __name__ == '__main__':
    impl, opts, pat, files = parse_args(prog='grep')
    rc = main(opts, pat, files, impl)
    sys.exit(rc)
